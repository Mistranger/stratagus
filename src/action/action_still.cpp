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
/**@name action_still.c - The stand still action. */
//
//      (c) Copyright 1998-2004 by Lutz Sammer and Jimmy Salmon
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
//      $Id$

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "stratagus.h"
#include "missile.h"
#include "unittype.h"
#include "actions.h"
#include "unit.h"
#include "tileset.h"
#include "map.h"
#include "pathfinder.h"

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

/**
**  Unit stands still or stand ground.
**
**  @param unit    Unit pointer for action.
**  @param ground  Flag: true if unit is standing ground.
*/
global void ActionStillGeneric(Unit* unit, int ground)
{
	const UnitType* type;
	Unit* temp;
	Unit* goal;

	DebugLevel3Fn(" %d\n" _C_ UnitNumber(unit));

	if (unit->Removed) { // Removed units, do nothing?
		// If peon is in building or unit is in transporter it is removed.
		unit->Wait = CYCLES_PER_SECOND / 6;
		return;
	}

	//
	// Animations
	//

	type = unit->Type;

	if (unit->SubAction) {
		//
		// Attacking unit in attack range.
		//
		AnimateActionAttack(unit);
	} else {
		//
		// Still animation
		//
		DebugCheck(!type->Animations || !type->Animations->Still);

		UnitShowAnimation(unit, type->Animations->Still);

		//
		// FIXME: this a workaround for some bad code.
		// UnitShowAnimation resets frame.
		// FIXME: the frames are hardcoded they should be configurable
		//
		if (unit->State == 1 && type->GivesResource == GoldCost) {
			unit->Frame = unit->Data.Resource.Active ? 1 : 0;
		}
		if (unit->State == 1 && type->GivesResource == OilCost) {
			unit->Frame = unit->Data.Resource.Active ? 2 : 0;
		}
		UnitMarkSeen(unit);
	}

	if (!unit->Reset) { // animation can't be aborted here
		return;
	}

	//
	// Some units, like critter are moving random around randomly
	//
	if (type->RandomMovementProbability &&
			((SyncRand() % 100) <= type->RandomMovementProbability)) {
		int x;
		int y;

		x = unit->X;
		y = unit->Y;
		switch ((SyncRand() >> 12) & 15) {
			case 0:		x++;				break;
			case 1:		y++;				break;
			case 2:		x--;				break;
			case 3:		y--;				break;
			case 4:		x++; y++;		break;
			case 5:		x--; y++;		break;
			case 6:		y--; x++;		break;
			case 7:		x--; y--;		break;
			default:
				break;
		}
		if (x < 0) {
			x = 0;
		} else if (x >= TheMap.Width) {
			x = TheMap.Width - 1;
		}
		if (y < 0) {
			y = 0;
		} else if (y >= TheMap.Height) {
			y = TheMap.Height - 1;
		}
		if (x != unit->X || y != unit->Y) {
			if (CheckedCanMoveToMask(x, y, TypeMovementMask(type))) {
				// FIXME: Don't use pathfinder for this, costs too much cpu.
				unit->Orders[0].Action = UnitActionMove;
				DebugCheck(unit->Orders[0].Goal);
				unit->Orders[0].Goal = NoUnitP;
				unit->Orders[0].Range = 0;
				unit->Orders[0].X = x;
				unit->Orders[0].Y = y;
				unit->State = 0;
			}
		}
		// NOTE: critter couldn't attack automatic through the return
		return;
	}

	//
	// Auto cast spells
	//
	if (unit->AutoCastSpell && AutoCastSpell(unit, unit->AutoCastSpell)) {
		return;
	}

	//
	// Cowards don't attack unless instructed.
	//
	if (type->CanAttack && !type->Coward) {
		//
		// Normal units react in reaction range.
		//
		if (unit->Stats->Speed && !ground) {
			if ((goal = AttackUnitsInReactRange(unit))) {
				// Weak goal, can choose other unit, come back after attack
				CommandAttack(unit, goal->X, goal->Y, NULL, FlushCommands);
				DebugLevel3Fn(" %d Attacking in range %d\n" _C_
					UnitNumber(unit) _C_ unit->SubAction);
				DebugCheck(unit->SavedOrder.Action != UnitActionStill);
				DebugCheck(unit->SavedOrder.Goal);
				unit->SavedOrder.Action = UnitActionAttack;
				unit->SavedOrder.Range = 0;
				unit->SavedOrder.X = unit->X;
				unit->SavedOrder.Y = unit->Y;
				unit->SavedOrder.Goal = NoUnitP;
			}
		} else if ((goal = AttackUnitsInRange(unit))) {
			DebugLevel3Fn(" %d #%d\n" _C_ UnitNumber(goal) _C_ goal->Refs);
			//
			// Old goal destroyed.
			//
			temp = unit->Orders[0].Goal;
			if (temp && temp->Destroyed) {
				RefsDecrease(temp);
				unit->Orders[0].Goal = temp = NoUnitP;
			}
			if (!unit->SubAction || temp != goal) {
				// New target.
				if (temp) {
					RefsDecrease(temp);
				}
				unit->Orders[0].Goal = goal;
				RefsIncrease(goal);
				unit->Reset = 0;
				unit->State = 0;
				unit->SubAction = 1; // Mark attacking.
				if (type->Stats->Speed) {
					UnitHeadingFromDeltaXY(unit,
						goal->X + (goal->Type->TileWidth - 1) / 2 - unit->X,
						goal->Y + (goal->Type->TileHeight - 1) / 2 - unit->Y);
					AnimateActionAttack(unit);
				}
			}
			return;
		}
	}

	if (unit->SubAction) { // was attacking.
		if ((temp = unit->Orders[0].Goal)) {
			RefsDecrease(temp);
			unit->Orders[0].Goal = NoUnitP;
		}
		unit->SubAction = unit->State = 0; // No attacking, restart
	}

	//
	// Land units are turning left/right.
	//
	if (type->LandUnit) {
		switch ((MyRand() >> 8) & 0x0FF) {
			case 0: // Turn clockwise
				unit->Direction += NextDirection;
				UnitUpdateHeading(unit);
				CheckUnitToBeDrawn(unit);
				break;
			case 1: // Turn counter clockwise
				unit->Direction -= NextDirection;
				UnitUpdateHeading(unit);
				CheckUnitToBeDrawn(unit);
				break;
			default: // does nothing
				break;
		}
		return;
	}

	//
	// Sea and air units are floating up/down.
	//
	if (unit->Type->SeaUnit || unit->Type->AirUnit) {
		unit->IY = (MyRand() >> 15) & 1;
	}
}

/**
**  Unit stands still!
**
**  @param unit  Unit pointer for still action.
*/
global void HandleActionStill(Unit* unit)
{
	ActionStillGeneric(unit, 0);
}

//@}
