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
/**@name action_board.c	-	The board action. */
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

#include "stratagus.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "actions.h"
#include "interface.h"
#include "pathfinder.h"

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Move to transporter.
**
**	@param unit	Pointer to unit, moving to transporter.
**
**	@return		>0 remaining path length, 0 wait for path, -1
**			reached goal, -2 can't reach the goal.
*/
local int MoveToTransporter(Unit* unit)
{
    int i,x,y;

    x=unit->X;
    y=unit->Y;
    i=DoActionMove(unit);
    // We have to reset a lot, or else they will circle each other and stuff.
    if (x!=unit->X||y!=unit->Y) {
	unit->Orders[0].RangeX=1;
	unit->Orders[0].RangeY=1;
        NewResetPath(unit);
    }
    // New code has this as default.
    DebugCheck( unit->Orders[0].Action!=UnitActionBoard );
    return i;
}

/**
**	Wait for transporter.
**
**	@param unit	Pointer to unit.
**	@return		True if ship arrived/present, False otherwise.
*/
local int WaitForTransporter(Unit* unit)
{
    Unit* trans;

    unit->Wait=6;
    unit->Reset=1;

    trans=unit->Orders[0].Goal;

    if( !trans || !trans->Type->Transporter ) {
	// FIXME: destination destroyed??
        DebugLevel2Fn("TRANSPORTER NOT REACHED %d,%d\n" _C_ unit->X _C_ unit->Y);
        return 0;
    }

    if( trans->Destroyed ) {
	DebugLevel0Fn("Destroyed transporter\n");
	RefsDebugCheck( !trans->Refs );
	if( !--trans->Refs ) {
	    ReleaseUnit(trans);
	}
	unit->Orders[0].Goal=NoUnitP;
	return 0;
    } else if( trans->Removed ||
	    !trans->HP || trans->Orders[0].Action==UnitActionDie ) {
	DebugLevel0Fn("Unusable transporter\n");
	RefsDebugCheck( !trans->Refs );
	--trans->Refs;
	RefsDebugCheck( !trans->Refs );
	unit->Orders[0].Goal=NoUnitP;
	return 0;
    }

    if( MapDistanceBetweenUnits(unit,trans)==1 ) {
	DebugLevel3Fn("Enter transporter\n");
	return 1;
    }

    //
    //	FIXME: any enemies in range attack them, while waiting.
    //

    DebugLevel3Fn("TRANSPORTER NOT REACHED %d,%d\n" _C_ unit->X _C_ unit->Y);
    //  n0b0dy: This means we have to search with a smaller range.
    //  It happens only when you reach the shore,and the transporter
    //  is not there. The unit searches with a big range, so it thinks
    //  it's there. This is why we reset the search. The transporter
    //  should be a lot closer now, so it's not as bad as it seems.
    unit->SubAction=0;
    unit->Orders[0].RangeX=1;
    unit->Orders[0].RangeY=1;
    //Uhh wait a bit.
    unit->Wait=10;

    return 0;
}

/**
**	Enter the transporter.
**
**	@param unit	Pointer to unit.
*/
local void EnterTransporter(Unit* unit)
{
    Unit* transporter;

    unit->Wait=1;
    unit->Orders[0].Action=UnitActionStill;
    unit->SubAction=0;

    transporter=unit->Orders[0].Goal;
    if( transporter->Destroyed ) {
	DebugLevel0Fn("Destroyed transporter\n");
	RefsDebugCheck( !transporter->Refs );
	if( !--transporter->Refs ) {
	    ReleaseUnit(transporter);
	}
	unit->Orders[0].Goal=NoUnitP;
	return;
    } else if( transporter->Removed ||
	    !transporter->HP || transporter->Orders[0].Action==UnitActionDie ) {
	DebugLevel0Fn("Unuseable transporter\n");
	RefsDebugCheck( !transporter->Refs );
	--transporter->Refs;
	RefsDebugCheck( !transporter->Refs );
	unit->Orders[0].Goal=NoUnitP;
	return;
    }

    RefsDebugCheck( !transporter->Refs );
    --transporter->Refs;
    RefsDebugCheck( !transporter->Refs );
    unit->Orders[0].Goal=NoUnitP;

    //
    //		Place the unit inside the transporter.
    //
   
    if (transporter->InsideCount<transporter->Type->MaxOnBoard) {
    	RemoveUnit(unit,transporter);
    	//Don't make anything funny after going out of the transporter.
    	// FIXME: This is probably wrong, but it works for me (n0b0dy)
    	unit->OrderCount=1;
    	unit->Orders[0].Action=UnitActionStill;
    	if( IsOnlySelected(transporter) ) {
	    SelectedUnitChanged();
	    MustRedraw|=RedrawInfoPanel;
	}
	return;
    }
    DebugLevel0Fn("No free slot in transporter\n");
}

/**
**	The unit boards a transporter.
**
**	@todo FIXME:
**      While waiting for the transporter the units must defend themselfs.
**
**	@param unit	Pointer to unit.
*/
global void HandleActionBoard(Unit* unit)
{
    int i;
    Unit* goal;

    DebugLevel3Fn("%p(%d) SubAction %d\n"
	    _C_ unit _C_ UnitNumber(unit) _C_ unit->SubAction);

    switch( unit->SubAction ) {
	//
	//	Wait for transporter
	//
	case 201:
	    // FIXME: show still animations
	    DebugLevel3Fn("Waiting\n");
	    if( WaitForTransporter(unit) ) {
		unit->SubAction=202;
	    }
	    break;
	//
	//	Enter transporter
	//
	case 202:
	    EnterTransporter(unit);
	    break;
	//
	//	Move to transporter
	//
	case 0:
		NewResetPath(unit);
		unit->SubAction=1;
		// FALL THROUGH
        default:
	    if( unit->SubAction<=200 ) {
		// FIXME: if near transporter wait for enter
		if( (i=MoveToTransporter(unit)) ) {
		    if( i==PF_UNREACHABLE ) {
			if( ++unit->SubAction==200 ) {
			    unit->Orders[0].Action=UnitActionStill;
			    if( (goal=unit->Orders[0].Goal) ) {
				RefsDebugCheck(!goal->Refs);
				if( !--goal->Refs ) {
				    RefsDebugCheck(!goal->Destroyed);
				    if( goal->Destroyed ) {
					ReleaseUnit(goal);
				    }
				}
				unit->Orders[0].Goal=NoUnitP;
			    }
			    unit->SubAction=0;
			} else {
                            //
                            // Try with a bigger range.
                            //
                            if( unit->Orders[0].RangeX <= TheMap.Width
                 	        || unit->Orders[0].RangeX <= TheMap.Height) {
		                unit->Orders[0].RangeX++;
		                unit->Orders[0].RangeY++;
                                unit->SubAction--;
                            }
			}
		    } else if( i==PF_REACHED ) {
			unit->SubAction=201;
		    }
		}
	    }
	    break;
    }
}

//@}
