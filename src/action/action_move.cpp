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
/**@name action_move.c	-	The move action. */
//
//	(c) Copyright 1998,2001,2002,2003 by Lutz Sammer
//
//	Stratagus is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation; either only 2 of the License.
//
//	Stratagus is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	$Id$

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
#include "tileset.h"
#include "map.h"
#include "actions.h"
#include "pathfinder.h"
#include "sound.h"
#include "interface.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Function
----------------------------------------------------------------------------*/


//#include "rdtsc.h"

/**
**	Generic unit mover.
**
**	@param unit	Unit that moves.
**	@param anim	Animation script for unit.
**
**	@return		>0 remaining path length, 0 wait for path, -1
**			reached goal, -2 can't reach the goal.
*/
local int ActionMoveGeneric(Unit* unit,const Animation* anim)
{
    int xd;
    int yd;
    int state;
    int d;
    int i;
    int x;
    int y;
    Unit *uninside;

    // FIXME: state 0?, should be wrong, should be Reset.
    // FIXME: Reset flag is cleared by HandleUnitAction.
    if( !(state=unit->State) ) {

#ifdef HIERARCHIC_PATHFINDER
	d = PfHierComputePath (unit, &xd, &yd);

#if 0
	{
	int ts0, ts1;
	ts0 = rdtsc ();
	NextPathElement(unit,&xd,&yd);
	ts1 = rdtsc ();
	printf ("old pathfinder: %d cycles\n", ts1-ts0);
	}
#endif
	switch( d ) {
#else /* HIERARCHIC_PATHFINDER */
	switch( d=NextPathElement(unit,&xd,&yd) ) {
#endif /* HIERARCHIC_PATHFINDER */
	    case PF_UNREACHABLE:	// Can't reach, stop
		unit->Reset=unit->Wait=1;
		unit->Moving=0;
		return d;
	    case PF_REACHED:		// Reached goal, stop
		unit->Reset=unit->Wait=1;
		unit->Moving=0;
		return d;
	    case PF_WAIT:		// No path, wait
		unit->Reset=unit->Wait=1;
		unit->Moving=0;
		return d;
	    default:			// On the way moving
		unit->Moving=1;
		break;
	}

	//
	//	Transporter (un)docking?
	//
	if( unit->Type->Transporter
		&& ( (WaterOnMap(unit->X,unit->Y)
		    && CoastOnMap(unit->X+xd,unit->Y+yd))
		|| (CoastOnMap(unit->X,unit->Y)
		    && WaterOnMap(unit->X+xd,unit->Y+yd)) ) ) {
	    PlayUnitSound(unit,VoiceDocking);
	}

	//
	//	Update movement map.
	//
	i=unit->Type->FieldFlags;
	TheMap.Fields[unit->X+unit->Y*TheMap.Width].Flags&=~i;

	UnitCacheRemove(unit);
	x=unit->X+=xd;
	y=unit->Y+=yd;
	UnitCacheInsert(unit);

	TheMap.Fields[x+y*TheMap.Width].Flags|=i;

	MustRedraw|=RedrawMinimap;
	//
	//	Update visible area.
	//
	x+=unit->Type->TileWidth/2;
	y+=unit->Type->TileHeight/2;

	MapMarkNewSight(unit->Player,x,y,unit->CurrentSightRange,xd,yd);
	//  Remove unit from the current selection
	if( unit->Selected && !IsMapFieldVisible(ThisPlayer,unit->X,unit->Y)) {
#ifndef NEW_UI
	    if( NumSelected==1 ) {          //  Remove building cursor
		CancelBuildingMode();
	    }
#endif
	    UnSelectUnit(unit);
	    SelectionChanged();
	}

	//  Remark sight for units inside too.
	if ( unit->Type->Transporter ) {
	    uninside=unit->UnitInside;
	    for( i=unit->InsideCount; i; uninside=uninside->NextContained,i-- ) {
		MapMarkNewSight(uninside->Player,x,y,uninside->CurrentSightRange,xd,yd);
	    }
	}

	//  Reveal Submarines and stuff.
	if( unit->Type->CanSeeSubmarine ) {
	    MarkSubmarineSeen(unit->Player,x,y,unit->Stats->SightRange);
	}

	unit->IX=-xd*TileSizeX;
	unit->IY=-yd*TileSizeY;
	unit->Frame=0;
	UnitHeadingFromDeltaXY(unit,xd,yd);
    } else {
#ifdef NEW_SHIPS
	if( unit->Type->UnitType==UnitTypeLand ) {
	    xd=Heading2X[unit->Direction/NextDirection];
	    yd=Heading2Y[unit->Direction/NextDirection];
	} else {
	    xd=Heading2X[unit->Direction/NextDirection]*2;
	    yd=Heading2Y[unit->Direction/NextDirection]*2;
	}
	d=0;
#else
	xd=Heading2X[unit->Direction/NextDirection];
	yd=Heading2Y[unit->Direction/NextDirection];
	d=0;
#endif
    }

    DebugLevel3Fn(": %d,%d State %2d " _C_ xd _C_ yd _C_ unit->State);
    DebugLevel3("Walk %d Frame %2d Wait %3d Heading %d %d,%d\n"
	    _C_ anim[state].Pixel
	    _C_ anim[state].Frame
	    _C_ anim[state].Sleep
	    _C_ unit->Direction
	    _C_ unit->IX _C_ unit->IY);

    //
    //	Next animation.
    //
    unit->IX+=xd*anim[state].Pixel;
    unit->IY+=yd*anim[state].Pixel;
    if( unit->Frame<0 ) {
	unit->Frame+=-anim[state].Frame;
    } else {
	unit->Frame+=anim[state].Frame;
    }
    unit->Wait=anim[state].Sleep;
    if( unit->Slow ) {			// unit is slowed down
	unit->Wait<<=1;
    }
    if( unit->Haste && unit->Wait>1 ) {	// unit is accelerated
	unit->Wait>>=1;
    }

    //
    //	Any graphic change?
    //
    if( !state || anim[state].Pixel || anim[state].Frame ) {
        CheckUnitToBeDrawn(unit);
    }

    //
    //	Handle the flags.
    //
    if( anim[state].Flags&AnimationReset ) {
	unit->Reset=1;
    }
    if( anim[state].Flags&AnimationRestart ) {
	unit->State=0;
    } else {
	++unit->State;
    }

    return d;
}

/**
**	Unit moves! Generic function called from other actions.
**
**	@param unit	Pointer to unit.
**
**	@return		>0 remaining path length, 0 wait for path, -1
**			reached goal, -2 can't reach the goal.
*/
global int DoActionMove(Unit* unit)
{
    if( unit->Type->Animations && unit->Type->Animations->Move ) {
	DebugLevel3("%s: %p\n" _C_ unit->Type->Ident _C_ unit->Type->Animations );
	return ActionMoveGeneric(unit,unit->Type->Animations->Move);
    }

    DebugLevel0Fn("Warning tried to move an object, which can't move\n");

    return PF_UNREACHABLE;
}

/**
**	Unit move action:
**
**	Move to a place or to an unit (can move).
**	Tries 10x to reach the target, note this are the complete tries.
**	If the target entered another unit, move to it's position.
**	If the target unit is destroyed, continue to move to it's last position.
**
**	@param unit	Pointer to unit.
*/
global void HandleActionMove(Unit* unit)
{
    Unit* goal;

    DebugLevel3Fn("%d: %d %d,%d \n" _C_ UnitNumber(unit) _C_
	    unit->Orders[0].Goal ? UnitNumber(unit->Orders[0].Goal) : -1 _C_
	    unit->Orders[0].X _C_ unit->Orders[0].Y);

    if( !unit->SubAction ) {		// first entry
	unit->SubAction=1;
	NewResetPath(unit);
				    
	DebugCheck( unit->State!=0 );
    }

    // FIXME: (mr-russ) Make a reachable goal here with GoalReachable...

    switch( DoActionMove(unit) ) {	// reached end-point?
	case PF_UNREACHABLE:
	    //
	    //	Some tries to reach the goal
	    //
	    if( unit->Orders[0].RangeX <= TheMap.Width
	        || unit->Orders[0].RangeX <= TheMap.Height) {
		unit->Orders[0].RangeX++;
		unit->Orders[0].RangeY++;
		break;
	    }
	    // FALL THROUGH
	case PF_REACHED:
	    unit->SubAction=0;
	    // Release target, if any.
	    if( (goal=unit->Orders[0].Goal) ) {
		RefsDebugCheck( !goal->Refs );
		if( !--goal->Refs ) {
		    DebugCheck( !goal->Destroyed );
		    ReleaseUnit(goal);
		}
		unit->Orders[0].Goal=NoUnitP;
	    }
	    unit->Orders[0].Action=UnitActionStill;
	    if( unit->Selected ) {	// update display for new action
		SelectedUnitChanged();
	    }
	    return;

	default:
	    break;
    }

    //
    //	Target destroyed?
    //
    if( (goal=unit->Orders[0].Goal) && goal->Destroyed ) {
	DebugLevel0Fn("Goal dead\n");
	unit->Orders[0].X=goal->X+goal->Type->TileWidth/2;
	unit->Orders[0].Y=goal->Y+goal->Type->TileHeight/2;
	unit->Orders[0].Goal=NoUnitP;
	RefsDebugCheck( !goal->Refs );
	if( !--goal->Refs ) {
	    ReleaseUnit(goal);
	}
	NewResetPath(unit);
    }
}

//@}
