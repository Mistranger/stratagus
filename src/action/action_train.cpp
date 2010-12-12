//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name action_train.cpp - The building train action. */
//
//      (c) Copyright 1998-2005 by Lutz Sammer and Jimmy Salmon
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
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
//      $Id$

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "stratagus.h"
#include "video.h"
#include "sound.h"
#include "unitsound.h"
#include "unittype.h"
#include "animation.h"
#include "player.h"
#include "unit.h"
#include "actions.h"
#include "missile.h"
#include "sound.h"
#include "ai.h"
#include "interface.h"
#include "ui.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Unit can handle order.
**
**  @param unit   Newly trained unit.
**  @param order  New order for the unit.
**
**  @return  1 if the the unit can do it, 0 otherwise.
*/
static int CanHandleOrder(CUnit &unit, COrderPtr order)
{
	if (order->Action == UnitActionResource) {
		//  Check if new unit can harvest.
		if (!unit.Type->Harvester) {
			return 0;
		}
		//  Also check if new unit can harvest this specific resource.
		CUnit *goal = order->GetGoal();
		if (goal && !unit.Type->ResInfo[goal->Type->GivesResource]) {
			return 0;
		}
		return 1;
	}
	if (order->Action == UnitActionAttack && !unit.Type->CanAttack) {
		return 0;
	}
	if (order->Action == UnitActionBoard && unit.Type->UnitType != UnitTypeLand) {
		return 0;
	}
	return 1;
}

/**
**  Unit trains unit!
**
**  @param unit  Unit that trains.
*/
void HandleActionTrain(CUnit &unit)
{
	// First entry
	if (!unit.SubAction) {
		unit.Data.Train.Ticks = 0;
		unit.SubAction = 1;
	}

	unit.Type->Animations->Train ?
		UnitShowAnimation(unit, unit.Type->Animations->Train) :
		UnitShowAnimation(unit, unit.Type->Animations->Still);

	if (unit.Wait) {
		unit.Wait--;
		return;
	}

	CPlayer *player = unit.Player;
	CUnitType &ntype = *unit.CurrentOrder()->Arg1.Type;
	const int cost = ntype.Stats[player->Index].Costs[TimeCost];
	unit.Data.Train.Ticks += SpeedTrain;
	// FIXME: Should count down
	if (unit.Data.Train.Ticks >= cost) {

		unit.Data.Train.Ticks = cost;

		//
		// Check if there are still unit slots.
		//
		if (NumUnits >= UnitMax) {
			unit.Wait = CYCLES_PER_SECOND / 6;
			return;
		}

		//
		// Check if enough supply available.
		//
		const int food = player->CheckLimits(ntype);
		if (food < 0) {
			if (food == -3 && unit.Player->AiEnabled) {
				AiNeedMoreSupply(*unit.Player);
			}

			unit.Data.Train.Ticks = cost;
			unit.Wait = CYCLES_PER_SECOND / 6;
			return;
		}

		CUnit *nunit = MakeUnit(ntype, player);
		if (nunit != NoUnitP) {
			const CUnitType *type = unit.Type;
			nunit->tilePos = unit.tilePos;

			// DropOutOnSide set unit to belong to the building
			// training it. This was an ugly hack, setting X and Y is enough,
			// no need to add the unit only to be removed.
			nunit->tilePos = unit.tilePos;

			// New unit might supply food
			UpdateForNewUnit(*nunit, 0);

			/* Auto Group Add */
			if (!unit.Player->AiEnabled && unit.GroupId) {
				int num = 0;
				while(!(unit.GroupId & (1 << num))) {
					++num;
				}
				AddToGroup(&nunit, 1, num);
			}

			DropOutOnSide(*nunit, LookingW, type->TileWidth, type->TileHeight);

			// Set life span
			if (type->DecayRate) {
				nunit->TTL = GameCycle + type->DecayRate * 6 * CYCLES_PER_SECOND;
			}

			player->Notify(NotifyYellow, nunit->tilePos.x, nunit->tilePos.y,
				_("New %s ready"), nunit->Type->Name.c_str());
			if (player == ThisPlayer) {
				PlayUnitSound(*nunit, VoiceReady);
			}
			if (unit.Player->AiEnabled) {
				AiTrainingComplete(unit, *nunit);
			}

			if (unit.OrderCount == 1) {
				unit.ClearAction();
			} else {
				unit.OrderFlush = 1;
				unit.SubAction = 0;
			}

			if (!CanHandleOrder(*nunit, &unit.NewOrder)) {
				DebugPrint("Wrong order for unit\n");

				// Tell the unit to move instead of trying any funny stuff.
				*(nunit->CurrentOrder()) = unit.NewOrder;
				nunit->CurrentOrder()->Action = UnitActionMove;
				nunit->CurrentOrder()->ClearGoal();
			} else {
				if (unit.NewOrder.HasGoal()) {
					if (unit.NewOrder.GetGoal()->Destroyed) {
						// FIXME: perhaps we should use another goal?
						DebugPrint("Destroyed unit in train unit\n");
						unit.NewOrder.ClearGoal();
						unit.NewOrder.Action = UnitActionStill;
					}
				}
				*(nunit->CurrentOrder()) = unit.NewOrder;
			}
			if (IsOnlySelected(unit)) {
				UI.ButtonPanel.Update();
			}
			return;
		} else {
			player->Notify(NotifyYellow, unit.tilePos.x, unit.tilePos.y,
				_("Unable to train %s"), ntype.Name.c_str());
		}
	}
	unit.Wait = CYCLES_PER_SECOND / 6;
}

//@}
