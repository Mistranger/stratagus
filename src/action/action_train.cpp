//   ___________		     _________		      _____  __
//   \_	  _____/______   ____   ____ \_   ___ \____________ _/ ____\/  |_
//    |    __) \_  __ \_/ __ \_/ __ \/    \  \/\_  __ \__  \\   __\\   __\ 
//    |     \   |  | \/\  ___/\  ___/\     \____|  | \// __ \|  |   |  |
//    \___  /   |__|    \___  >\___  >\______  /|__|  (____  /__|   |__|
//	  \/		    \/	   \/	     \/		   \/
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   FreeCraft - A free fantasy real time strategy game engine
//
/**@name action_train.c -	The building train action. */
//
//	(c) Copyright 1998,2000-2002 by Lutz Sammer
//
//	FreeCraft is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation; only version 2 of the License.
//
//	FreeCraft is distributed in the hope that it will be useful,
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

#include "freecraft.h"
#include "video.h"
#include "sound_id.h"
#include "unitsound.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "actions.h"
#include "missile.h"
#include "sound.h"
#include "ai.h"
#include "interface.h"

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Unit trains unit!
**
**	@param unit	Unit that trains.
*/
global void HandleActionTrain(Unit* unit)
{
    Unit* nunit;
    const UnitType* type;
    Player* player;

    player=unit->Player;
    //
    //	First entry
    //
    if( !unit->SubAction ) {
	unit->Data.Train.Ticks=0;
	unit->Data.Train.What[0]=unit->Orders[0].Type;
	unit->Data.Train.Count=1;
	unit->SubAction=1;
    }
    unit->Data.Train.Ticks+=SpeedTrain;
    // FIXME: Should count down
    if( unit->Data.Train.Ticks
	    >=unit->Data.Train.What[0]
		->Stats[player->Player].Costs[TimeCost] ) {
	//
	//	Check if there are still unit slots.
	//
	if( NumUnits>=UnitMax  ) {
	    unit->Data.Train.Ticks=unit->Data.Train.What[0]
		    ->Stats[player->Player].Costs[TimeCost];
	    unit->Reset=1;
	    unit->Wait=FRAMES_PER_SECOND/6;
	    return;
	}

	//
	//	Check if enough food available.
	//
	if( player->Food<
		player->NumFoodUnits+unit->Data.Train.What[0]->Demand ) {

	    // FIXME: GameMessage
	    if( player==ThisPlayer ) {
		// FIXME: PlayVoice :), see task.txt
		SetMessage("Not enough food...build more farms.");
	    } else if( unit->Player->Ai ) {
		AiNeedMoreFarms(unit,unit->Orders[0].Type);
	    }

	    unit->Data.Train.Ticks=unit->Data.Train.What[0]
		    ->Stats[player->Player].Costs[TimeCost];
	    unit->Reset=1;
	    unit->Wait=FRAMES_PER_SECOND/6;
	    return;
	}

	nunit=MakeUnit(unit->Data.Train.What[0],player);
	nunit->X=unit->X;
	nunit->Y=unit->Y;
	type=unit->Type;
	DropOutOnSide(nunit,LookingW,type->TileWidth,type->TileHeight);

	// FIXME: GameMessage
	if( player==ThisPlayer ) {
	    SetMessageEvent( nunit->X, nunit->Y, "New %s ready",
		    nunit->Type->Name);
	    PlayUnitSound(nunit,VoiceReady);
	} else if( unit->Player->Ai ) {
	    AiTrainingComplete(unit,nunit);
	}

	unit->Reset=unit->Wait=1;

	if ( --unit->Data.Train.Count ) {
	    int z;
	    for( z = 0; z < unit->Data.Train.Count ; z++ ) {
		unit->Data.Train.What[z]=unit->Data.Train.What[z+1];
	    }
	    unit->Data.Train.Ticks=0;
	} else {
	    unit->Orders[0].Action=UnitActionStill;
	    unit->SubAction=0;
	}

	//
	//	FIXME: we must check if the units supports the new order.
	//
	if( (unit->NewOrder.Action==UnitActionHaulOil
		    && !nunit->Type->Tanker)
		|| (unit->NewOrder.Action==UnitActionAttack
		    && !nunit->Type->CanAttack) ) {
	    DebugLevel0Fn("Wrong order for unit\n");
	    unit->Orders[0].Action=UnitActionStill;
	} else {
	    if( unit->NewOrder.Goal ) {
		if( unit->NewOrder.Goal->Destroyed ) {
		    // FIXME: perhaps we should use another goal?
		    DebugLevel0Fn("Destroyed unit in train unit\n");
		    RefsDebugCheck( !unit->NewOrder.Goal->Refs );
		    if( !--unit->NewOrder.Goal->Refs ) {
			ReleaseUnit(unit->NewOrder.Goal);
		    }
		    unit->NewOrder.Goal=NoUnitP;
		    unit->NewOrder.Action=UnitActionStill;
		}
	    }

	    nunit->Orders[0]=unit->NewOrder;

	    //
	    // FIXME: Pending command uses any references?
	    //
	    if( nunit->Orders[0].Goal ) {
		RefsDebugCheck( !nunit->Orders[0].Goal->Refs );
		nunit->Orders[0].Goal->Refs++;
	    }
	}

	if( IsOnlySelected(unit) ) {
	    UpdateButtonPanel();
	    MustRedraw|=RedrawPanels;
	}

	return;
    }

    if( IsOnlySelected(unit) ) {
	MustRedraw|=RedrawInfoPanel;
    }

    unit->Reset=1;
    unit->Wait=FRAMES_PER_SECOND/6;
}

//@}
