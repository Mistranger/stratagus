//       _________ __                 __                               
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ \ 
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/ 
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   Stratagus - A free fantasy real time strategy game engine
//
/**@name action_spellcast.c	-	The spell cast action. */
//
//	(c) Copyright 2000-2002 by Vladi Belperchinov-Shabanski
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; version 2 dated June, 1991.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//
//	$Id$

/*
** This is inherited from action_attack.c, actually spell casting will
** be considered a `special' case attack action... //Vladi
*/

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "stratagus.h"
#include "video.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "missile.h"
#include "actions.h"
#include "pathfinder.h"
#include "sound.h"
#include "tileset.h"
#include "map.h"
#include "spells.h"
#include "interface.h"

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

#if 0

/**
**	Animate unit spell cast (it is attack really)!
**
**	@param unit	Unit, for that spell cast/attack animation is played.
*/
global void AnimateActionSpellCast(Unit * unit)
{
    int flags;

    if (unit->Type->Animations) {
	DebugCheck(!unit->Type->Animations->Attack);

	flags = UnitShowAnimation(unit, unit->Type->Animations->Attack);

	if ((flags & AnimationSound)) {	
	    PlayUnitSound(unit, VoiceAttacking);	// FIXME: spell sound?
	}

	if (flags & AnimationMissile) {	// FIXME: should cast spell ?
	    FireMissile(unit);		// we should not get here ?? 
	}
    }
}

#endif

/**
**	Handle moving to the target.
**
**	@param unit	Unit, for that the spell cast is handled.
*/
local void SpellMoveToTarget(Unit* unit)
{
    Unit* goal;
    int err;

    // Unit can't move
    err = 1;
    if( unit->Type->Animations && unit->Type->Animations->Move ) {
	err = DoActionMove(unit);
	if (!unit->Reset) {
	    return;
	}
    }

    // when reached DoActionMove changes unit action
    // FIXME: use return codes from pathfinder
    goal = unit->Orders[0].Goal;

    if (goal && MapDistanceToUnit(unit->X, unit->Y, goal)
	    <= unit->Orders[0].RangeX) {

	// there is goal and it is in range
	unit->State = 0;
	if( !unit->Type->Building ) {
	    // FIXME: buildings could have directions
	    UnitHeadingFromDeltaXY(unit,
		goal->X+(goal->Type->TileWidth-1)/2-unit->X,
		goal->Y+(goal->Type->TileHeight-1)/2-unit->Y);
	}
	unit->SubAction++;		// cast the spell
	return;
    } else if (!goal && MapDistance(unit->X, unit->Y, unit->Orders[0].X,
	    unit->Orders[0].Y) <= unit->Orders[0].RangeX) {
	// there is no goal and target spot is in range
	unit->State = 0;
	if( !unit->Type->Building ) {
	    // FIXME: buildings could have directions
	    UnitHeadingFromDeltaXY(unit,
		unit->Orders[0].X
			+((SpellType*)unit->Orders[0].Arg1)->Range-unit->X,
		unit->Orders[0].Y
			+((SpellType*)unit->Orders[0].Arg1)->Range-unit->Y);
	}
	unit->SubAction++;		// cast the spell
	return;
    } else if (err) {
	// goal/spot out of range -- move to target
	unit->Orders[0].Action=UnitActionStill;
	unit->State=unit->SubAction=0;

	if( unit->Orders[0].Goal ) {	// Release references
	    RefsDebugCheck(!unit->Orders[0].Goal->Refs);
	    if (!--unit->Orders[0].Goal->Refs) {
		RefsDebugCheck(!unit->Orders[0].Goal->Destroyed);
		ReleaseUnit(unit->Orders[0].Goal);
	    }
	    unit->Orders[0].Goal=NoUnitP;
	}
    }
    DebugCheck(unit->Type->Vanishes || unit->Destroyed);
}

/**
**	Unit casts a spell!
**
**	@param unit	Unit, for that the spell cast is handled.
*/
global void HandleActionSpellCast(Unit * unit)
{
    int flags;
    const SpellType *spell;

    DebugLevel3Fn("%d %d,%d+%d+%d\n" _C_
	UnitNumber(unit) _C_ unit->Orders[0].X _C_ unit->Orders[0].Y _C_
	unit->Orders[0].RangeX _C_ unit->Orders[0].RangeY);

    switch (unit->SubAction) {

    case 0:				// first entry.
	//
	//	Check if we can cast the spell.
	//
	spell = unit->Orders[0].Arg1;
	if( !CanCastSpell(unit,spell,unit->Orders[0].Goal,
		unit->Orders[0].X,unit->Orders[0].Y) ) {

	    //
	    //	Notify player about this problem
	    //
	    if (unit->Mana < spell->ManaCost) {
		NotifyPlayer(unit->Player,NotifyYellow,unit->X,unit->Y,
			"%s: not enough mana for spell: %s",
			unit->Type->Name, spell->Name);
	    } else {
		NotifyPlayer(unit->Player,NotifyYellow,unit->X,unit->Y,
			"%s: can't cast spell: %s",
			unit->Type->Name, spell->Name);
	    }

	    if( unit->Player->Ai ) {
		DebugLevel0Fn("FIXME: need we an AI callback?\n");
	    }
	    unit->Orders[0].Action = UnitActionStill;
	    unit->SubAction = 0;
	    unit->Wait = 1;
	    if (unit->Orders[0].Goal) {
		RefsDebugCheck(!unit->Orders[0].Goal->Refs);
		if (!--unit->Orders[0].Goal->Refs) {
		    RefsDebugCheck(!unit->Orders[0].Goal->Destroyed);
		    ReleaseUnit(unit->Orders[0].Goal);
		}
		unit->Orders[0].Goal=NoUnitP;
	    }
	    return;
	}
	NewResetPath(unit);
	unit->Value=0;		// repeat spell on next pass? (defaults to `no')
	unit->SubAction=1;
	// FALL THROUGH
    case 1:				// Move to the target.
	if( (spell = unit->Orders[0].Arg1)->Range != 0x7F ) {
	    SpellMoveToTarget(unit);
	    break;
	} else {
	    unit->SubAction=2;
	}
	// FALL THROUGH
    case 2:				// Cast spell on the target.
	// FIXME: should use AnimateActionSpellCast here
	if (unit->Type->Animations && unit->Type->Animations->Attack ) {
	    flags=UnitShowAnimation(unit, unit->Type->Animations->Attack);
	    if( flags&AnimationMissile ) {
		// FIXME: what todo, if unit/goal is removed?
		if (unit->Orders[0].Goal
			&& unit->Orders[0].Goal->Orders[0].Action==UnitActionDie) {
		    unit->Value = 0;
		} else {
		    spell = unit->Orders[0].Arg1;
		    unit->Value = SpellCast(unit,spell,unit->Orders[0].Goal,
			    unit->Orders[0].X,unit->Orders[0].Y);
		}
	    }
	    if ( !(flags&AnimationReset) ) {	// end of animation
		return;
	    }
	} else {
	    // FIXME: what todo, if unit/goal is removed?
	    if (unit->Orders[0].Goal
		    && unit->Orders[0].Goal->Orders[0].Action==UnitActionDie) {
		unit->Value = 0;
	    } else {
		spell = unit->Orders[0].Arg1;
		unit->Value = SpellCast(unit,spell,unit->Orders[0].Goal,
			unit->Orders[0].X,unit->Orders[0].Y);
	    }
	}
	if (!unit->Value) {
	    unit->Orders[0].Action = UnitActionStill;
	    unit->SubAction = 0;
	    unit->Wait = 1;
	    if (unit->Orders[0].Goal) {
		RefsDebugCheck(!unit->Orders[0].Goal->Refs);
		if (!--unit->Orders[0].Goal->Refs) {
		    if (unit->Orders[0].Goal->Destroyed) {
			ReleaseUnit(unit->Orders[0].Goal);
		    }
		}
		unit->Orders[0].Goal=NoUnitP;
	    }
	}
	break;

    default:
	unit->SubAction = 0;		// Reset path, than move to target
	break;
    }
}

//@}
