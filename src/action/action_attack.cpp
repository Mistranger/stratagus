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
/**@name action_attack.c	-	The attack action. */
//
//	(c) Copyright 1998-2001 by Lutz Sammer
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

/**
**	@todo FIXME:	I should rewrite this action, if only the
**			new orders are supported.
*/

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "stratagus.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "missile.h"
#include "actions.h"
#include "sound.h"
#include "map.h"
#include "pathfinder.h"
#include <string.h>

/*----------------------------------------------------------------------------
--	Defines
----------------------------------------------------------------------------*/

#define WEAK_TARGET	2		/// Weak target, could be changed
#define MOVE_TO_TARGET	4		/// Move to target state
#define ATTACK_TARGET	5		/// Attack target state

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Generic unit attacker.
**
**	@param unit	Unit, for that the attack animation is played.
**	@param attack	Attack animation.
*/
local void DoActionAttackGeneric(Unit* unit,const Animation* attack)
{
    int flags;

    flags = UnitShowAnimation(unit, attack);

    if ((flags & AnimationSound) && (UnitVisibleOnMap(unit) || ReplayRevealMap)) {
	PlayUnitSound(unit, VoiceAttacking);
    }

    if (flags & AnimationMissile) {	// time to fire projectil
	FireMissile(unit);
	unit->Invisible = 0;		// unit is invisible until attacks
    }
}

/**
**	Animate unit attack!
**
**	@param unit	Unit, for that the attack animation is played.
*/
global void AnimateActionAttack(Unit* unit)
{
    if (unit->Type->Animations) {
	DebugCheck(!unit->Type->Animations->Attack);
	DoActionAttackGeneric(unit, unit->Type->Animations->Attack);
    }
}

/**
**	Check for dead goal.
**
**	@warning
**		The caller must check, if he likes the restored SavedOrder!
**
**	@todo
**		If an unit enters an building, than the attack choose an
**		other goal, perhaps it is better to wait for the goal?
**
**	@param unit	Unit using the goal.
**
**	@return		A valid goal, if available.
*/
local Unit* CheckForDeadGoal(Unit* unit)
{
    Unit* goal;

    //
    //	Do we have a goal?
    //
    if ((goal = unit->Orders[0].Goal)) {
	if (goal->Destroyed) {
	    //
	    //	Goal is destroyed
	    //
	    unit->Orders[0].X = goal->X + goal->Type->TileWidth / 2;
	    unit->Orders[0].Y = goal->Y + goal->Type->TileHeight / 2;
	    unit->Orders[0].RangeX = unit->Orders[0].RangeY = 0;

	    DebugLevel0Fn("destroyed unit %d\n" _C_ UnitNumber(goal));
	    RefsDebugCheck(!goal->Refs);
	    if (!--goal->Refs) {
		ReleaseUnit(goal);
	    }

	    unit->Orders[0].Goal = goal = NoUnitP;
	} else if (!goal->HP || goal->Orders[0].Action == UnitActionDie ||
		goal->Removed) {
	    //
	    //	Goal is unusable, dies or has entered a building.
	    //
	    unit->Orders[0].X = goal->X + goal->Type->TileWidth / 2;
	    unit->Orders[0].Y = goal->Y + goal->Type->TileHeight / 2;
	    unit->Orders[0].RangeX = unit->Orders[0].RangeY = 0;

	    RefsDebugCheck(!goal->Refs);
	    --goal->Refs;
	    RefsDebugCheck(!goal->Refs);

	    unit->Orders[0].Goal = goal = NoUnitP;
	}
	if (!goal) {
	    //
	    //	If we have a saved order continue this saved order.
	    //
	    if (unit->SavedOrder.Action != UnitActionStill) {
		unit->Orders[0] = unit->SavedOrder;
		unit->SavedOrder.Action = UnitActionStill;
		unit->SavedOrder.Goal = NoUnitP;

		if (unit->Selected && unit->Player == ThisPlayer) {
		    MustRedraw |= RedrawButtonPanel;
		}
		goal = unit->Orders[0].Goal;
	    }
	    NewResetPath(unit);
	}
    }
    return goal;
}

/**
**	Check for target in range.
**
**	@return		True if command is over.
*/
local int CheckForTargetInRange(Unit* unit)
{
    Unit* goal;
    Unit* temp;
    int wall;

    //
    //	Target is dead?
    //
    goal = CheckForDeadGoal(unit);
    // Fall back to last order, only works if this wasn't attack
    if (unit->Orders[0].Action != UnitActionAttackGround &&
	    unit->Orders[0].Action != UnitActionAttack) {
	unit->State = unit->SubAction = 0;
	return 1;
    }

    //
    //	No goal: if meeting enemy attack it.
    //
    wall = 0;
    if (!goal && !(wall = WallOnMap(unit->Orders[0].X, unit->Orders[0].Y)) &&
	    unit->Orders[0].Action != UnitActionAttackGround) {
	goal=AttackUnitsInReactRange(unit);
	if (goal) {
	    if (unit->SavedOrder.Action == UnitActionStill) {
		// Save current command to continue it later.
		DebugCheck(unit->Orders[0].Goal);
		unit->SavedOrder = unit->Orders[0];
	    }
	    RefsDebugCheck(goal->Destroyed || !goal->Refs);
	    goal->Refs++;
	    unit->Orders[0].Goal = goal;
	    unit->Orders[0].RangeX = unit->Orders[0].RangeY =
		unit->Stats->AttackRange;
	    unit->Orders[0].X = unit->Orders[0].Y = -1;
	    unit->SubAction |= WEAK_TARGET;		// weak target
	    NewResetPath(unit);
	    DebugLevel3Fn("%d in react range %d\n" _C_
		UnitNumber(unit) _C_ UnitNumber(goal));
	}

    //
    //	Have a weak target, try a better target.
    //
    } else if (goal && (unit->SubAction & WEAK_TARGET)) {
	temp = AttackUnitsInReactRange(unit);
	if (temp && temp->Type->Priority > goal->Type->Priority) {
	    RefsDebugCheck(!goal->Refs);
	    goal->Refs--;
	    RefsDebugCheck(!goal->Refs);
	    RefsDebugCheck(temp->Destroyed || !temp->Refs);
	    temp->Refs++;
	    if (unit->SavedOrder.Action == UnitActionStill) {
		// Save current command to come back.
		unit->SavedOrder = unit->Orders[0];
		if ((goal = unit->SavedOrder.Goal)) {
		    DebugLevel0Fn("Have goal to come back %d\n" _C_
			UnitNumber(goal));
		    unit->SavedOrder.X = goal->X + goal->Type->TileWidth / 2;
		    unit->SavedOrder.Y = goal->Y + goal->Type->TileHeight / 2;
		    unit->SavedOrder.RangeX = unit->SavedOrder.RangeY = 0;
		    unit->SavedOrder.Goal = NoUnitP;
		}
	    }
	    unit->Orders[0].Goal = goal = temp;
	    unit->Orders[0].X = unit->Orders[0].Y = -1;
	    NewResetPath(unit);
	}
    }

    DebugCheck(unit->Type->Vanishes || unit->Destroyed || unit->Removed);
    DebugCheck(unit->Orders[0].Action != UnitActionAttack &&
	unit->Orders[0].Action != UnitActionAttackGround);

    return 0;
}

/**
**	FIXME: docu
*/
local void MoveToTarget(Unit* unit)
{
    Unit* goal;
    int err;

    if (!unit->Orders[0].Goal) {
	if (unit->Orders[0].X == -1 || unit->Orders[0].Y == -1) {
	    DebugLevel0Fn("FIXME: Wrong goal position, check where set!\n");
	    unit->Orders[0].X = unit->Orders[0].Y = 0;
	}
    }

    err = DoActionMove(unit);

    if (unit->Reset) {
	//
	//  Look if we have reached the target.
	//
	if (CheckForTargetInRange(unit)) {
	    return;
	}
	goal = unit->Orders[0].Goal;
	if (err >= 0) {
	    //
	    //  Nothing to do, we're on the way moving.
	    //
	    DebugLevel3Fn("Nothing to do.\n");
	    return;
	}
	if (err == PF_REACHED) {
	    //
	    //	Have reached target? FIXME: could use the new return code?
	    //
	    if (goal && MapDistanceBetweenUnits(unit, goal) <=
		    unit->Stats->AttackRange) {
		DebugLevel3Fn("Reached another unit, now attacking it.\n");
		unit->State = 0;
		if (unit->Stats->Speed) {
		    UnitHeadingFromDeltaXY(unit,
			goal->X + (goal->Type->TileWidth - 1) / 2 - unit->X,
			goal->Y + (goal->Type->TileHeight - 1) / 2 - unit->Y);
		    // FIXME: only if heading changes
		    CheckUnitToBeDrawn(unit);
		}
		unit->SubAction++;
		return;
	    }
	    //
	    //  Attacking wall or ground.
	    //
	    if (!goal && (WallOnMap(unit->Orders[0].X, unit->Orders[0].Y) ||
			unit->Orders[0].Action == UnitActionAttackGround) &&
		    MapDistanceToUnit(unit->Orders[0].X, unit->Orders[0].Y, unit) <=
			unit->Stats->AttackRange) {
		DebugLevel3Fn("Reached wall or ground, now attacking it.\n");
		unit->State = 0;
		if (unit->Stats->Speed) {
		    UnitHeadingFromDeltaXY(unit, unit->Orders[0].X - unit->X,
			unit->Orders[0].Y - unit->Y);
		    // FIXME: only if heading changes
		    CheckUnitToBeDrawn(unit);
		}
		unit->SubAction &= WEAK_TARGET;
		unit->SubAction |= ATTACK_TARGET;
		return;
	    }
	} 
	//
	//  Unreachable.
	//
	if (err == PF_UNREACHABLE) {
	    unit->State = unit->SubAction = 0;
	    DebugLevel3Fn("Target not reachable, unit: %d" _C_
		UnitNumber(unit));
	    if (goal) {
		DebugLevel3(", target %d range %d\n" _C_ UnitNumber(goal) _C_
		    unit->Orders[0].RangeX);
	    } else {
		//
		//  When attack-moving we have to allow a bigger range
		//
		DebugLevel3(", (%d,%d) Tring with more range...\n" _C_
		    unit->Orders[0].X _C_ unit->Orders[0].Y);
		if (unit->Orders[0].RangeX < TheMap.Width ||
			unit->Orders[0].RangeY < TheMap.Height) {
		    // Try again with more range
		    unit->Orders[0].RangeX++;
		    unit->Orders[0].RangeY++;
		    return;
		}
	    }
	}
	//
	//  Return to old task?
	//  
	unit->State = unit->SubAction = 0;
	DebugLevel3Fn("Returning to old task.\n");
	if (unit->Orders[0].Goal) {
	    RefsDebugCheck(!unit->Orders[0].Goal->Refs);
	    unit->Orders[0].Goal->Refs--;
	    RefsDebugCheck(!unit->Orders[0].Goal->Refs);
	}
	unit->Orders[0] = unit->SavedOrder;
	NewResetPath(unit);

	// Must finish, if saved command finishes
	unit->SavedOrder.Action = UnitActionStill;
	unit->SavedOrder.Goal = NoUnitP;

	if (unit->Selected && unit->Player == ThisPlayer) {
	    MustRedraw |= RedrawButtonPanel;
	}
	return;
    }
    DebugCheck(unit->Type->Vanishes || unit->Destroyed || unit->Removed);
    DebugCheck(unit->Orders[0].Action != UnitActionAttack  &&
	unit->Orders[0].Action != UnitActionAttackGround);
}

/**
**	Handle attacking the target.
**
**	@param unit	Unit, for that the attack is handled.
*/
local void AttackTarget(Unit* unit)
{
    Unit* goal;
    Unit* temp;

    if (!unit->Orders[0].Goal) {
	if (unit->Orders[0].X == -1 || unit->Orders[0].Y == -1) {
	    DebugLevel0Fn("FIXME: Wrong goal position, check where set!\n");
	    unit->Orders[0].X = unit->Orders[0].Y = 0;
	}
    }

    AnimateActionAttack(unit);
    if (unit->Reset) {
	//
	//	Goal is "weak" or a wall.
	//
	goal = unit->Orders[0].Goal;
	if (!goal && (WallOnMap(unit->Orders[0].X, unit->Orders[0].Y) ||
		unit->Orders[0].Action == UnitActionAttackGround)) {
	    DebugLevel3Fn("attack a wall or ground!!!!\n");
	    return;
	}

	//
	//	Target is dead?
	//
	goal = CheckForDeadGoal(unit);
	// Fall back to last order.
	if (unit->Orders[0].Action != UnitActionAttackGround &&
		unit->Orders[0].Action != UnitActionAttack) {
	    unit->State = unit->SubAction = 0;
	    return;
	}

	//
	//	No target choose one.
	//
	if (!goal) {
	    unit->State = 0;
	    goal = AttackUnitsInReactRange(unit);
	    //
	    //	No new goal, continue way to destination.
	    //
	    if (!goal) {
		unit->SubAction = MOVE_TO_TARGET;
		// Return to old task?
		if (unit->SavedOrder.Action != UnitActionStill) {
		    unit->SubAction = 0;
		    DebugCheck(unit->Orders[0].Goal != NoUnitP);
		    unit->Orders[0] = unit->SavedOrder;
		    NewResetPath(unit);
		    // Must finish, if saved command finishes
		    unit->SavedOrder.Action = UnitActionStill;

		    // This isn't supported
		    DebugCheck(unit->SavedOrder.Goal != NoUnitP);

		    if (unit->Selected && unit->Player == ThisPlayer) {
			MustRedraw |= RedrawButtonPanel;
		    }
		}
		return;
	    }

	    //
	    //	Save current command to come back.
	    //
	    if (unit->SavedOrder.Action == UnitActionStill) {
		unit->SavedOrder = unit->Orders[0];
		if ((temp = unit->SavedOrder.Goal)) {
		    DebugLevel0Fn("Have unit to come back %d?\n" _C_
			UnitNumber(temp));
		    unit->SavedOrder.X = temp->X + temp->Type->TileWidth / 2;
		    unit->SavedOrder.Y = temp->Y + temp->Type->TileHeight / 2;
		    unit->SavedOrder.RangeX = unit->SavedOrder.RangeY = 0;
		    unit->SavedOrder.Goal = NoUnitP;
		}
	    }

	    RefsDebugCheck(goal->Destroyed || !goal->Refs);
	    goal->Refs++;
	    DebugLevel3Fn("%d Unit in react range %d\n" _C_
		UnitNumber(unit) _C_ UnitNumber(goal));
	    unit->Orders[0].Goal = goal;
	    unit->Orders[0].X = unit->Orders[0].Y = -1;
	    unit->Orders[0].RangeX = unit->Orders[0].RangeY =
		unit->Stats->AttackRange;
	    NewResetPath(unit);
	    unit->SubAction |= WEAK_TARGET;

	//
	//	Have a weak target, try a better target.
	//	FIXME: if out of range also try another target quick
	//
	} else if (goal && (unit->SubAction & WEAK_TARGET)) {
	    temp = AttackUnitsInReactRange(unit);
	    if (temp && temp->Type->Priority > goal->Type->Priority) {
		RefsDebugCheck(!goal->Refs);
		goal->Refs--;
		RefsDebugCheck(!goal->Refs);
		RefsDebugCheck(temp->Destroyed || !temp->Refs);
		temp->Refs++;

		if (unit->SavedOrder.Action == UnitActionStill) {
		    // Save current order to come back or to continue it.
		    unit->SavedOrder = unit->Orders[0];
		    if ((goal = unit->SavedOrder.Goal)) {
			DebugLevel0Fn("Have goal to come back %d\n" _C_
			    UnitNumber(goal));
			unit->SavedOrder.X = goal->X + goal->Type->TileWidth / 2;
			unit->SavedOrder.Y = goal->Y + goal->Type->TileHeight / 2;
			unit->SavedOrder.RangeX = unit->SavedOrder.RangeY = 0;
			unit->SavedOrder.Goal = NoUnitP;
		    }
		}
		unit->Orders[0].Goal = goal = temp;
		unit->Orders[0].X = unit->Orders[0].Y = -1;
		NewResetPath(unit);
	    }
	}

	//
	//	Still near to target, if not goto target.
	//
	if (MapDistanceBetweenUnits(unit, goal) > unit->Stats->AttackRange) {
	    if (unit->SavedOrder.Action == UnitActionStill) {
		// Save current order to come back or to continue it.
		unit->SavedOrder = unit->Orders[0];
		if ((temp = unit->SavedOrder.Goal)) {
		    DebugLevel0Fn("Have goal to come back %d\n" _C_
			UnitNumber(temp));
		    unit->SavedOrder.X = temp->X + temp->Type->TileWidth / 2;
		    unit->SavedOrder.Y = temp->Y + temp->Type->TileHeight / 2;
		    unit->SavedOrder.RangeX = unit->SavedOrder.RangeY = 0;
		    unit->SavedOrder.Goal = NoUnitP;
		}
	    }
	    NewResetPath(unit);
	    unit->Frame = 0;
	    unit->State = 0;
	    unit->SubAction &= WEAK_TARGET;
	    unit->SubAction |= MOVE_TO_TARGET;
	}

	//
	//	Turn always to target
	//
	if (unit->Stats->Speed && goal) {
	    UnitHeadingFromDeltaXY(unit,
		goal->X + (goal->Type->TileWidth - 1) / 2 - unit->X,
		goal->Y + (goal->Type->TileHeight - 1) / 2 - unit->Y);
	    // FIXME: only if heading changes
	    CheckUnitToBeDrawn(unit);
	}
    }
}

/**
**	Unit attacks!
**
**	I added a little trick, if SubAction&WEAK_TARGET is true the goal is
**	a weak goal.  This means the unit AI (little AI) could choose a new
**	better goal.
**
**	@todo
**		Lets do some tries to reach the target.
**		If target place is not reachable, choose better goal to reduce
**		the pathfinder load.
**
**	@param unit	Unit, for that the attack is handled.
*/
global void HandleActionAttack(Unit* unit)
{
    DebugLevel3Fn("Attack %d r %d,%d\n" _C_ UnitNumber(unit) _C_
	unit->Orders->RangeX _C_ unit->Orders->RangeY);

    switch (unit->SubAction) {
	//
	//	First entry
	//
	case 0:
	    unit->SubAction = MOVE_TO_TARGET;
	    NewResetPath(unit);
	    //
	    //	FIXME: should use a reachable place to reduce pathfinder time.
	    //
	    DebugCheck(unit->State != 0);
	    //
	    //	Look for target, if already in range. Attack if So
	    //
	    if (CheckForTargetInRange(unit)) {
		unit->SubAction = ATTACK_TARGET;
		return;
	    }

	    // FALL THROUGH
	//
	//	Move near to the target.
	//
	case MOVE_TO_TARGET:
	case MOVE_TO_TARGET + WEAK_TARGET:
	    MoveToTarget(unit);
	    break;

	//
	//	Attack the target.
	//
	case ATTACK_TARGET:
	case ATTACK_TARGET + WEAK_TARGET:
	    AttackTarget(unit);
	    break;

	case WEAK_TARGET:
	    DebugLevel0("FIXME: wrong entry.\n");
	    break;
    }
}

//@}
