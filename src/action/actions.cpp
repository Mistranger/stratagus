//       _________ __                 __                               
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/ 
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   Stratagus - A free fantasy real time strategy game engine
//
/**@name actions.c	-	The actions. */
//
//	(c) Copyright 1998,2000-2002 by Lutz Sammer
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

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "stratagus.h"
#include "video.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "actions.h"
#include "interface.h"
#include "map.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

global unsigned SyncHash;	    /// Hash calculated to find sync failures

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Check if the unit can still *see* the goal or not
**	This first of all checks dead/dying units, and then
**	Invisibility, cloaking, going under fow and the like.
**
**
**
*/
global int GoalGone(const Unit* unit, const Unit* goal)
{
    //
    //  Check for dead/removed goals.
    //
    if (    //  Unit is marked destroyed
	    goal->Destroyed ||
	    //  Unit has 0 hp, dying
	    !goal->HP ||
	    //  Unit has action die, doesn't really live?
	    goal->Orders[0].Action == UnitActionDie ||
	    //  Unit is removed (inside something.)
	    goal->Removed) {
	return 1;
    }
    //
    //	Check if we have an unit for this goal.
    //
    if (unit) {
	//	SharedVision makes all the rest of the checks meaningless
	if (IsSharedVision(unit->Player, goal) || unit->Player==goal->Player) {
	    return 0;
	} else {
	    int x;
	    int y;
	    //  Goal is invisible (by spell)
	    if (goal->Invisible) {
		return 1;
	    }
	    //  Goal is cloaked for this player
	    if (!(goal->Visible & (1 << unit->Player->Player))) {
		return 1;
	    }
	    //
	    //	Check if under fog of war.
	    //	Don't bother for goals visible under fog.
	    //
	    if (goal->Type->VisibleUnderFog) {
		return 0;
	    }
	    for (x = goal->X; x < goal->X + goal->Type->TileWidth; x++) {
		for (y = goal->Y; y < goal->Y + goal->Type->TileHeight; y++) {
		    if (IsMapFieldVisible(unit->Player, x, y)) {
			return 0;
		    }
		}
	    }
	    return 1;
	}
    } else {
	return 0;
    }
}

/*----------------------------------------------------------------------------
--	Animation
----------------------------------------------------------------------------*/

/**
**	Show unit animation.
**		Returns animation flags.
**
**	@param unit		Unit of the animation.
**	@param animation	Animation script to handle.
**
**	@return			The flags of the current script step.
*/
global int UnitShowAnimation(Unit* unit, const Animation* animation)
{
    int state;
    int flags;

    if (!(state = unit->State)) {
	unit->Frame = 0;
	UnitUpdateHeading(unit);		// FIXME: remove this!!
    }

    DebugLevel3Fn("State %2d " _C_ state);
    DebugLevel3("Flags %2d Pixel %2d Frame %2d Wait %3d " _C_
	animation[state].Flags _C_ animation[state].Pixel _C_
	animation[state].Frame _C_ animation[state].Sleep);
    DebugLevel3("Heading %d +%d,%d\n" _C_ unit->Direction _C_ unit->IX _C_ unit->IY);
    
    if (unit->Frame < 0) {
	unit->Frame -= animation[state].Frame;
    } else {
	unit->Frame += animation[state].Frame;
    }
    unit->IX += animation[state].Pixel;
    unit->IY += animation[state].Pixel;
    unit->Wait = animation[state].Sleep;
    if (unit->Slow) {			// unit is slowed down
	unit->Wait <<= 1;
    }
    if (unit->Haste && unit->Wait > 1) {	// unit is accelerated
	unit->Wait >>= 1;
    }

    // Anything changed the display?
    if ((animation[state].Frame || animation[state].Pixel)) {
        CheckUnitToBeDrawn(unit);
    }

    flags = animation[state].Flags;
    if (flags & AnimationReset) {	// Reset can check for other actions
	unit->Reset = 1;
    }
    if (flags & AnimationRestart) {	// Restart animation script
	unit->State = 0;
    } else {
	++unit->State;			// Advance to next script
    }

    return flags;
}

/*----------------------------------------------------------------------------
--	Actions
----------------------------------------------------------------------------*/

/**
**	Unit does nothing!
**
**	@param unit	Unit pointer for none action.
*/
local void HandleActionNone(Unit* unit __attribute__((unused)))
{
    DebugLevel1Fn("FIXME: Should not happen!\n");
    DebugLevel1Fn("FIXME: Unit (%d) %s has action none.!\n" _C_
	UnitNumber(unit) _C_ unit->Type->Ident);
}

/**
**	Unit has notwritten function.
**
**	@param unit	Unit pointer for notwritten action.
*/
local void HandleActionNotWritten(Unit* unit __attribute__((unused)))
{
    DebugLevel1Fn("FIXME: Not written!\n");
    DebugLevel1Fn("FIXME: Unit (%d) %s has action %d.!\n" _C_
	UnitNumber(unit) _C_ unit->Type->Ident _C_ unit->Orders[0].Action);
}

/**
**	Jump table for actions.
**
**	@note can move function into unit structure.
*/
local void (*HandleActionTable[256])(Unit*) = {
    HandleActionNone,
    HandleActionStill,
    HandleActionStandGround,
    HandleActionFollow,
    HandleActionMove,
    HandleActionAttack,
    HandleActionAttack,		// HandleActionAttackGround,
    HandleActionDie,
    HandleActionSpellCast,
    HandleActionTrain,
    HandleActionUpgradeTo,
    HandleActionResearch,
    HandleActionBuilded,
    HandleActionBoard,
    HandleActionUnload,
    HandleActionPatrol,
    HandleActionBuild,
    HandleActionRepair,
    HandleActionResource,
    HandleActionReturnGoods,
    HandleActionNotWritten,
    HandleActionNotWritten,

    // Enough for the future ?
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
    HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
};

/**
**	Increment a unit's health
**
**	@param	unit	the unit to operate on
**	@param	amount  the amount of time to make up for.(in cycles) 
*/
local void HandleRegenerations(Unit* unit)
{
    int f;
    //	Mana
    if (unit->Type->CanCastSpell && unit->Mana!=unit->Type->_MaxMana) {
	unit->Mana++;

	if (unit->Mana > unit->Type->_MaxMana) {
	    unit->Mana = unit->Type->_MaxMana;
	}
	if (unit->Selected) {
	    MustRedraw |= RedrawInfoPanel;
	}
    }

    f = 0;
    //  Burn
    if (!unit->Removed && !unit->Destroyed && unit->Stats->HitPoints
	    && unit->Orders[0].Action!=UnitActionBuilded ) {
	f = (100 * unit->HP) / unit->Stats->HitPoints;
	if (f <= unit->Type->BurnPercent) {
	    HitUnit(NoUnitP, unit, unit->Type->BurnDamageRate);
	    f = 1;
	} else {
	    f = 0;
	}
    } 

    //  Health doesn't regenerate while burning.
    if ((!f) && unit->Stats) {
	//  Unit may not have stats assigned to it
	if (unit->Stats->RegenerationRate && unit->HP<unit->Stats->HitPoints) {
	    unit->HP += unit->Stats->RegenerationRate;
	    if (unit->HP > unit->Stats->HitPoints) {
		unit->HP = unit->Stats->HitPoints;
	    }
	    if (unit->Selected) {
		MustRedraw|=RedrawInfoPanel;
	    }
	}
    }

    // Shields and stuff?
}

/**
**	Handle things about the unit that decay over time
**
**	@param	unit	the unit that the decay is handled for
**	@param	amount  the amount of time to make up for.(in cycles) 
**
*/
local void HandleBuffs(Unit* unit, int amount)
{
    int deadunit;
    int flag;

    deadunit = 0;
    //
    //	Look if the time to live is over.
    //
    if (unit->TTL && unit->TTL < (GameCycle - unit->HP)) {
	DebugLevel0Fn("Unit must die %lu %lu!\n" _C_ unit->TTL _C_ GameCycle);
	//
	//	Hit unit does some funky stuff...
	//
	unit->HP -= amount;
	if (unit->HP < 0) {
	    LetUnitDie(unit);
	}
	if (unit->Selected) {
	    MustRedraw |= RedrawInfoPanel;
	}
    }

    // some frames delayed done my color cycling
    flag = 1;
    //
    // decrease spells effects time, if end redraw unit.
    //
    
    //	Bloodlust
    if (unit->Bloodlust) {
	unit->Bloodlust -= amount;
	if (unit->Bloodlust < 0) {
	    unit->Bloodlust = 0 ;
	    if (!flag) {
		flag = CheckUnitToBeDrawn(unit);
	    }
	}
    }
    //	Haste
    if (unit->Haste) {
	unit->Haste -= amount;
	if (unit->Haste < 0) {
	    unit->Haste = 0;
	    if (!flag) {
		flag = CheckUnitToBeDrawn(unit);
	    }
	}
    }
    //	Slow
    if (unit->Slow) {
	unit->Slow -= amount;
	if (unit->Slow < 0) {
	    unit->Slow = 0;
	    if (!flag) {
		flag = CheckUnitToBeDrawn(unit);
	    }
	}
    }
    //	Invisible
    if (unit->Invisible) {
	unit->Invisible -= amount;
	if (unit->Invisible < 0) {
	    unit->Invisible = 0;
	    if (!flag) {
		flag = CheckUnitToBeDrawn(unit);
	    }
	}
    }
    //	Unholy armor
    if (unit->UnholyArmor) {
	unit->UnholyArmor -= amount;
	if (unit->UnholyArmor < 0) {
	    unit->UnholyArmor = 0;
	    if (!flag) {
		flag = CheckUnitToBeDrawn(unit);
	    }
	}
    }
}

/**
**	Handle the action of an unit.
**
**	@param unit	Pointer to handled unit.
*/
local void HandleUnitAction(Unit* unit)
{
    int z;
	
    //
    //	If current action is breakable proceed with next one.
    //
    if (unit->Reset) {
	unit->Reset = 0;
	//
	//	o Look if we have a new order and old finished.
	//	o Or the order queue should be flushed.
	//
	if (unit->OrderCount > 1 &&
		(unit->Orders[0].Action == UnitActionStill || unit->OrderFlush)) {

	    if (unit->Removed) {	// FIXME: johns I see this as an error
		DebugLevel0Fn("Flushing removed unit\n");
		// This happens, if building with ALT+SHIFT.
		return;
	    }

	    //
	    //	Release pending references.
	    //
	    if (unit->Orders[0].Goal) {
		//  If mining decrease the active count on the resource.
		if (unit->Orders[0].Action == UnitActionResource &&
			unit->SubAction == 60) {
		    //  FIXME: SUB_GATHER_RESOURCE ?
		    unit->Orders[0].Goal->Data.Resource.Active--;
		    DebugCheck(unit->Orders[0].Goal->Data.Resource.Active < 0);
		}
		//  Still shouldn't have a reference
		DebugCheck(unit->Orders[0].Action == UnitActionStill);
		RefsDecrease(unit->Orders->Goal);
	    }
	    if (unit->CurrentResource) {
		if (unit->Type->ResInfo[unit->CurrentResource]->LoseResources &&
			unit->Value < unit->Type->ResInfo[unit->CurrentResource]->ResourceCapacity) {
		    unit->Value = 0;
		}
	    }

	    //
	    //	Shift queue with structure assignment.
	    //
	    unit->OrderCount--;
	    unit->OrderFlush = 0;
	    for (z = 0; z < unit->OrderCount; ++z) {
		unit->Orders[z] = unit->Orders[z + 1];
	    }
	    memset(unit->Orders + z, 0, sizeof(*unit->Orders));

	    //
	    //	Note subaction 0 should reset.
	    //
	    unit->SubAction = unit->State = 0;
	    unit->Wait = 1;

	    if (IsOnlySelected(unit)) {// update display for new action
		SelectedUnitChanged();
		MustRedraw |= RedrawInfoPanel;
	    }
	}
    }

    //
    //	Select action. FIXME: should us function pointers in unit structure.
    //
    HandleActionTable[unit->Orders[0].Action](unit);
}

/**
**	Update the actions of all units each game cycle.
**
**	IDEA:	to improve the preformance use slots for waiting.
*/
global void UnitActions(void)
{
    Unit* table[UnitMax];
    Unit** tpos;
    Unit** tend;
    Unit* unit;
    int blinkthiscycle;
    int buffsthiscycle;
    int regenthiscycle;

    buffsthiscycle = !(GameCycle % CYCLES_PER_SECOND);
    regenthiscycle = !(GameCycle % CYCLES_PER_SECOND);
    blinkthiscycle = !(GameCycle % CYCLES_PER_SECOND);

    //
    //	Must copy table, units could be removed.
    //
    tend = table + NumUnits;
    memcpy(table, Units, sizeof(Unit*) * NumUnits);

    //
    //	Check for dead/destroyed units in table...
    //
    for (tpos = table; tpos < tend; ++tpos) {
	unit = *tpos;
    	if (unit->Destroyed) {		// Ignore destroyed units
	    DebugLevel0Fn("Destroyed unit %d in table, should be ok\n" _C_
		UnitNumber(unit));
	    unit = 0;
	    continue;
	}
    }

    //
    //	Check for things that only happen every few cycles
    //	(faster in their own loops.)
    //

    //	1) Blink flag.
    if (blinkthiscycle) {
	for (tpos = table; tpos < tend; ++tpos) {
	    if ((unit = *tpos)) {
		if (unit->Blink) {
		    --unit->Blink;
		}
	    }
	}
    }

    //  2) Buffs...
    if (buffsthiscycle) {
	for (tpos = table; tpos < tend; ++tpos) {
	    if ((unit = *tpos)) {
		HandleBuffs(unit, CYCLES_PER_SECOND);
	    }
	}
    }

    //	3) Increase health mana, burn and stuff
    if (regenthiscycle) {
	for (tpos = table; tpos < tend; ++tpos) {
	    if ((unit = *tpos)) {
		HandleRegenerations(unit);
	    }
	}
    }

    //
    //	Do all actions
    //
    for (tpos = table; tpos < tend; ++tpos) {
	unit = *tpos;
	if (!tpos) {
	    continue;
	}

#if defined(UNIT_ON_MAP) && 0		// debug unit store
	 { const Unit* list;

	list = TheMap.Fields[unit->Y * TheMap.Width + unit->X].Here.Units;
	while (list) {				// find the unit
	    if (list == unit) {
		break;
	    }
	    list = list->Next;
	}
	if (!unit->Removed) {
	    if (!list && (!unit->Type->Vanishes &&
		    !unit->Orders[0].Action == UnitActionDie)) {
		DebugLevel0Fn("!removed not on map %d\n" _C_ UnitNumber(unit));
		abort();
	    }
	} else if (list) {
	    DebugLevel0Fn("remove on map %d\n" _C_ UnitNumber(unit));
	    abort();
	}
	list = unit->Next;
	while (list) {
	    if (list->X != unit->X || list->Y != unit->Y) {
		DebugLevel0Fn("Wrong X,Y %d %d,%d\n" _C_ UnitNumber(list) _C_
		    list->X _C_ list->Y);
		abort();
	    }
	    list = list->Next;
	} }
#endif

	if (--unit->Wait) {		// Wait until counter reached
	    continue;
	}

	HandleUnitAction(unit);
	DebugCheck(*tpos != unit);	// Removed is evil.

#ifdef DEBUG_ACTIONS
	//
	//	Dump the unit to find the network unsyncron bug.
	//
	{
	static FILE* logf;

	if (!logf) {
	    time_t now;
	    char buf[256];

	    sprintf(buf, "log_of_fc_%d.log", ThisPlayer->Player);
	    logf = fopen(buf, "wb");
	    if (!logf) {
		return;
	    }
	    fprintf(logf, ";;; Log file generated by Stratagus Version "
		    VERSION "\n");
	    time(&now);
	    fprintf(logf, ";;;\tDate: %s", ctime(&now));
	    fprintf(logf, ";;;\tMap: %s\n\n", TheMap.Description);
	}

	fprintf(logf, "%lu: ", GameCycle);
	fprintf(logf, "%d %s S%d/%d-%d P%d Refs %d: %X %d,%d %d,%d\n",
	    UnitNumber(unit), unit->Type ? unit->Type->Ident : "unit-killed",
	    unit->State, unit->SubAction,
	    unit->Orders[0].Action,
	    unit->Player ? unit->Player->Player : -1, unit->Refs,SyncRandSeed,
	    unit->X, unit->Y, unit->IX, unit->IY);
		
	// SaveUnit(unit,logf);
	fflush(NULL);
	}
#endif
	//
	//	Calculate some hash.
	//
	SyncHash = (SyncHash << 5) | (SyncHash >> 27);
	SyncHash ^= unit->Orders[0].Action << 18;
	SyncHash ^= unit->State << 12;
	SyncHash ^= unit->SubAction << 6;
	SyncHash ^= unit->Refs << 3;
    }
}

/**
**	Handle Marking and Unmarking of Cloak
**
**	@note Must be handled outside the drawing fucntions and
**	@note all at once.
*/
global void HandleCloak(void)
{
    Unit* unit;
    int i;

    for (i = 0; i < NumUnits; ++i) {
	unit = Units[i]; 
	if (unit->Type->PermanentCloak) {
	    if ((unit->Visible & (1 << ThisPlayer->Player))) {
		CheckUnitToBeDrawn(unit);
	    }
	    unit->Visible = 0;
	}
    }
    for (i = 0; i < NumUnits; ++i) {
	unit = Units[i];
	if (unit->Type->DetectCloak && !unit->Removed &&
		unit->Orders[0].Action != UnitActionBuilded) {
	    MapDetectCloakedUnits(unit);
	}
    }
}
 
//@}
