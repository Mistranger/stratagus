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
/**@name ai_force.cpp - AI force functions. */
//
//      (c) Copyright 2001-2005 by Lutz Sammer and Jimmy Salmon
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
#include <string.h>

#include "stratagus.h"
#include "unittype.h"
#include "unit.h"
#include "ai_local.h"
#include "actions.h"
#include "map.h"
#include "depend.h"
#include "pathfinder.h"
#include "player.h"

/*----------------------------------------------------------------------------
--  Types
----------------------------------------------------------------------------*/

template <const bool IN_REACT_RANGE>
struct AiForceEnemyFinder {
	const CUnit *enemy;

	inline bool found(void) {
		return enemy != NULL;
	}

	inline bool operator() (const CUnit *const unit)
	{
		if(IN_REACT_RANGE) {
			if (unit->Type->CanAttack)
				enemy = AttackUnitsInReactRange(*unit);
		} else {
			if (unit->Type->CanAttack)
				enemy = AttackUnitsInDistance(*unit, MaxMapWidth);
		}
		return enemy == NULL;
	}

	AiForceEnemyFinder(int force): enemy(NULL) {
		AiPlayer->Force[force].Units.for_each_if(*this);
	}

	AiForceEnemyFinder(AiForce *force):	enemy(NULL) {
		force->Units.for_each_if(*this);
	}

};

struct AiForceAttackSender {

	inline void operator() (CUnit *const unit) {
	// this may be problem if units are in bunker and we want sent
	// them to attack
	if (unit->Container == NULL) {
			// To avoid lot of CPU consuption, send them with a small time difference.
			unit->Wait = delta;
			++delta;
			if (unit->Type->CanTransport() && unit->BoardCount > 0) {
				CommandUnload(*unit, goalPos, NULL, FlushCommands);
			} else if (unit->Type->CanAttack) {
				CommandAttack(*unit, goalPos,  NULL, FlushCommands);
			} else /*if (force->State == 2) */{
				CommandMove(*unit, goalPos, FlushCommands);
			}
		}
	}

	//
	//  Send all units in the force to enemy at x,y.
	//
	AiForceAttackSender(int force, const Vec2i &pos) : goalPos(pos), delta(0) {
		DebugPrint("%d: Attacking with force #%d\n" _C_ AiPlayer->Player->Index _C_ force);
		AiForce *fptr = &AiPlayer->Force[force];
		fptr->Attacking = true;
		fptr->State = AI_FORCE_STATE_ATTACKING;
		fptr->Units.for_each(*this);
	}

	AiForceAttackSender(AiForce *force, const Vec2i &pos) :
		 goalPos(pos), delta(0) {
		DebugPrint("%d: Attacking with force #%lu\n" _C_ AiPlayer->Player->Index
			 _C_ (long unsigned int)(force  - &(AiPlayer->Force[0])));
		force->Attacking = true;
		force->State = AI_FORCE_STATE_ATTACKING;
		force->Units.for_each(*this);
	}

	Vec2i goalPos;
	int delta;
};



/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

int UnitTypeEquivs[UnitTypeMax + 1]; /// equivalence between unittypes

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Remove any equivalence between unittypes
*/
void AiResetUnitTypeEquiv(void)
{
	for (int i = 0; i <= UnitTypeMax; ++i) {
		UnitTypeEquivs[i] = i;
	}
}

/**
**  Make two unittypes equivalents from the AI's point of vue
**
**  @param a  the first unittype
**  @param b  the second unittype
*/
void AiNewUnitTypeEquiv(CUnitType *a, CUnitType *b)
{
	int find;
	int replace;
	int i;

	find = UnitTypeEquivs[a->Slot];
	replace = UnitTypeEquivs[b->Slot];

	// Always record equivalences with the lowest unittype.
	if (find < replace) {
		i = find;
		find = replace;
		replace = i;
	}

	// Then just find & replace in UnitTypeEquivs...
	for (i = 0; i <= UnitTypeMax; ++i) {
		if (UnitTypeEquivs[i] == find) {
			UnitTypeEquivs[i] = replace;
		}
	}
}


/**
**  Find All unittypes equivalent to a given one
**
**  @param unittype  the unittype to find equivalence for
**  @param result    int array which will hold the result. (Size UnitTypeMax+1)
**
**  @return          the number of unittype found
*/
int AiFindUnitTypeEquiv(const CUnitType &unittype, int *result)
{
	const int search = UnitTypeEquivs[unittype.Slot];
	int count = 0;

	for (int i = 0; i < UnitTypeMax + 1; ++i) {
		if (UnitTypeEquivs[i] == search) {
			// Found one
			result[count] = i;
			count++;
		}
	}
	return count;
}

/**
**  Find All unittypes equivalent to a given one, and which are available
**  UnitType are returned in the prefered order ( ie palladin >> knight... )
**
**  @param unittype     The unittype to find equivalence for
**  @param usableTypes  int array which will hold the result. (Size UnitTypeMax+1)
**
**  @return             the number of unittype found
*/
int AiFindAvailableUnitTypeEquiv(const CUnitType &unittype, int *usableTypes)
{
	// 1 - Find equivalents
	int usableTypesCount = AiFindUnitTypeEquiv(unittype, usableTypes);

	// 2 - Remove unavailable unittypes
	for (int i = 0; i < usableTypesCount; ) {
		if (!CheckDependByIdent(AiPlayer->Player, UnitTypes[usableTypes[i]]->Ident)) {
			// Not available, remove it
			usableTypes[i] = usableTypes[usableTypesCount - 1];
			--usableTypesCount;
		} else {
			++i;
		}
	}

	// 3 - Sort by level
	// We won't have usableTypesCount>4, so simple sort should do it
	for (int i = 0; i < usableTypesCount - 1; ++i) {
		int bestlevel = UnitTypes[usableTypes[i]]->Priority;
		for (int j = i + 1; j < usableTypesCount; ++j) {
			const int curlevel = UnitTypes[usableTypes[j]]->Priority;

			if (curlevel > bestlevel) {
				std::swap(usableTypes[j], usableTypes[i]);
				bestlevel = curlevel;
			}
		}
	}
	return usableTypesCount;
}

/* =========================== FORCES ========================== */

struct AiForceCounter {
	unsigned int *data;//[UnitTypeMax + 1];

	inline void operator() (CUnit *const unit) {
		data[UnitTypeEquivs[unit->Type->Slot]]++;
	}

	AiForceCounter(CUnitCache &units, unsigned int *d, const size_t len): data(d)
	{
		memset(data, 0, len);
		units.for_each(*this);
	}
};

void AiForce::CountTypes(unsigned int *counter, const size_t len) {
	AiForceCounter(Units, counter, len);
}

/**
**  Check if the units belongs to the force/base.
**
**  @param type   Type to check.
**
**  @return       True if it fits, false otherwise.
*/
bool AiForce::IsBelongsTo(const CUnitType *type)
{
	AiUnitType *aitype;
	bool flag = false;
	int slot;
	unsigned int counter[UnitTypeMax + 1];

	//
	// Count units in force.
	//
	CountTypes(counter, sizeof(counter));

	//
	// Look what should be in the force.
	//
	Completed = true;
	for (unsigned int i = 0; i < UnitTypes.size(); ++i) {
		aitype = &UnitTypes[i];
		slot = aitype->Type->Slot;
		if (aitype->Want > counter[slot]) {
			if (UnitTypeEquivs[type->Slot] == slot) {
				if (aitype->Want - 1 > counter[slot]) {
					Completed = false;
				}
				flag = true;
			} else {
				Completed = false;
			}
		}
	}
	return flag;
}

/**
**  Ai clean units in a force.
**
*/
void AiForce::Clean(void) {

	CUnit *aiunit;
	unsigned int i = 0;
	//
	// Release all killed units.
	//
	while (i != Units.size()) {
		aiunit = Units[i];
		if (!aiunit->IsAlive()) {
			aiunit->GroupId = 0;
			aiunit->RefsDecrease();
			Units.Remove(i);
			continue;
		}
		++i;
	}

}

void AiForce::Attack(const Vec2i &pos)
{
	Vec2i goalPos(pos);
	Clean();

	Attacking = false;
	if (Units.size() > 0) {
		Attacking = true;

		if (goalPos.x == -1 || goalPos.y == -1) {
			/* Search in entire map */
			const CUnit *enemy = AiForceEnemyFinder<false>(this).enemy;
			if (enemy) {
				goalPos = enemy->tilePos;
			}
		}

		this->GoalPos = goalPos;

		if (goalPos.x == -1 || goalPos.y == -1) {
			DebugPrint("%d: Need to plan an attack with transporter\n" _C_ AiPlayer->Player->Index);
			if (State == AI_FORCE_STATE_WAITING && !PlanAttack()) {
				DebugPrint("%d: Can't transport\n" _C_ AiPlayer->Player->Index);
				Attacking = false;
			}
			return;
		}

		//
		//  Send all units in the force to enemy.
		//
		AiForceAttackSender(this, goalPos);
	}

}

AiForceManager::AiForceManager() {
	forces.resize(3);
	memset(script, -1, AI_MAX_FORCES * sizeof(char));
}

unsigned int AiForceManager::FindFreeForce(int role)
{
	/* find free force */
	unsigned int f = 0;
	while (f < forces.size() && (forces[f].State > AI_FORCE_STATE_FREE)) {
		++f;
	};
	if (f == forces.size()) {
		forces.resize(f + 1);
	}
	forces[f].State = AI_FORCE_STATE_WAITING;
	forces[f].Role = role;
	return f;
}

/**
**  Cleanup units in forces.
*/
void AiForceManager::Clean(void)
{
	for(unsigned int i = 0; i < forces.size(); ++i)
	{
		forces[i].Clean();
	}
}

/**
**  Ai assign unit to force.
**
**  @param unit  Unit to assign to force.
*/
bool AiForceManager::Assign(CUnit &unit)
{
	//
	// Check to which force it belongs
	//
	for(unsigned int i = 0; i < forces.size(); ++i)
	{
		AiForce *force = &forces[i];
		// No troops for attacking force
		if (force->IsAttacking()) {
			continue;
		}
		if (unit.GroupId == 0 && force->IsBelongsTo(unit.Type)) {
			force->Insert(unit);
			unit.GroupId = i + 1;
			return true;
		}
	}
	return false;
}

void AiForceManager::CheckUnits(int *counter)
{
	int attacking[UnitTypeMax];
	const int *unit_types_count = AiPlayer->Player->UnitTypesCount;

	memset(attacking, 0, sizeof(attacking));

	// Look through the forces what is missing.
	for (unsigned int i = 0; i < forces.size(); ++i)
	{
		AiForce *force = &forces[i];

		if (force->State > AI_FORCE_STATE_FREE && force->IsAttacking()) {
			for (unsigned int j = 0; j < forces[i].Size(); ++j) {
				const CUnit *unit = forces[i].Units[j];
				attacking[unit->Type->Slot]++;
			}
		}
	}

	// create missing units
	for (unsigned int i = 0; i < forces.size(); ++i)
	{
		AiForce *force = &forces[i];

		// No troops for attacking force
		if (force->State == AI_FORCE_STATE_FREE || force->IsAttacking()) {
			continue;
		}
		for (unsigned int j = 0; j < force->UnitTypes.size(); ++j) {
			const AiUnitType *aiut = &force->UnitTypes[j];
			const int t = aiut->Type->Slot;
			const int x = aiut->Want;
			const int requested = x - (unit_types_count[t] + counter[t] - attacking[t]);

			if (requested > 0) {  // Request it.
				AiAddUnitTypeRequest(*aiut->Type, requested);
				counter[t] += requested;
				force->Completed = false;
			}
			counter[t] -= x;
		}
	}
}

/**
**  Cleanup units in forces.
*/
void AiCleanForces()
{
	AiPlayer->Force.Clean();
}

/**
**  Ai assign unit to force.
**
**  @param unit  Unit to assign to force.
*/
bool AiAssignToForce(CUnit &unit)
{
	return AiPlayer->Force.Assign(unit);
}

/**
**  Assign free units to force.
*/
void AiAssignFreeUnitsToForce()
{
	const int n = AiPlayer->Player->TotalNumUnits;
	CUnit **table = AiPlayer->Player->Units;
	AiCleanForces();
	for (int i = 0; i < n; ++i) {
		CUnit &unit = *table[i];
		if (unit.Active && unit.GroupId == 0)
		{
			AiPlayer->Force.Assign(unit);
		}
	}
}

/**
**
*/
static void AiAttack(unsigned int &force, const Vec2i &pos)
{
	// Move the force to a free position so it can be used for a new
	// attacking party
	if (!AiPlayer->Force[force].Defending) {
		unsigned int f = AiPlayer->Force.FindFreeForce();
		AiPlayer->Force[f] = AiPlayer->Force[force];
		for (unsigned int j = 0; j < AiPlayer->Force[force].Size(); ++j) {
			CUnit *aiunit = AiPlayer->Force[force].Units[j];
			aiunit->GroupId = f + 1;
		}
		AiPlayer->Force[force].Reset();
		force = f;
	}

	AiPlayer->Force[force].Attack(pos);
}

/**
**  Attack at position with force.
**
**  @param force  Force number to attack with.
**  @param x      X tile map position to be attacked.
**  @param y      Y tile map position to be attacked.
*/
void AiAttackWithForceAt(unsigned int force, int x, int y)
{
	const Vec2i pos = {x, y};

	if (!(force < AI_MAX_FORCES)) {
		DebugPrint("Force out of range: %d" _C_ force);
		return ;
	}

	if (!Map.Info.IsPointOnMap(pos)) {
		DebugPrint("(%d, %d) not in the map(%d, %d)" _C_ pos.x _C_ pos.y
			_C_ Map.Info.MapWidth _C_ Map.Info.MapHeight);
		return ;
	}
	AiAttack(force, pos);
}

/**
**  Attack opponent with force.
**
**  @param force  Force number to attack with.
*/
void AiAttackWithForce(unsigned int force)
{
	if (!(force < AI_MAX_FORCES)) {
		DebugPrint("Force out of range: %d" _C_ force);
		return ;
	}
	const Vec2i invalidPos = {-1, -1};
	AiAttack(force, invalidPos);
}

/**
**  Attack opponent with forces.
**	Merge forces in array into one attack force and attack with it
**  Merge is make because units in one force help each other during attack
**
**  @param forces  Array with Force numbers to attack with (array should be finished with -1).
*/
void AiAttackWithForces(int *forces)
{
	const Vec2i invalidPos = {-1, -1};
	bool found = false;
	unsigned int f = AiPlayer->Force.FindFreeForce();

	AiPlayer->Force[f].Reset();

	for(int i = 0; forces[i] != -1; ++i) {
		int force = forces[i];
		if (!AiPlayer->Force[force].Defending)
		{
			found = true;

			//Fixme: this is triky but should work
			AiPlayer->Force[f].Role = AiPlayer->Force[force].Role;

			for (unsigned int j = 0; j < AiPlayer->Force[force].Units.size(); ++j) {
				CUnit *aiunit = AiPlayer->Force[force].Units[j];
				aiunit->GroupId = f + 1;
				AiPlayer->Force[f].Units.Insert(aiunit);
			}

			for (unsigned int j = 0; j < AiPlayer->Force[force].UnitTypes.size(); ++j) {
				AiPlayer->Force[f].UnitTypes.push_back(AiPlayer->Force[force].UnitTypes[j]);
			}

			AiPlayer->Force[force].Reset();
		} else {
			AiPlayer->Force[force].Attack(invalidPos);
		}
	}

	if (found) {
		AiPlayer->Force[f].Attack(invalidPos);
	} else {
		AiPlayer->Force[f].Reset(true);
	}
}


/**
**  Load all unit before attack.
**
**  @param aiForce force to group.
*/
static void AiGroupAttackerForTransport(AiForce *aiForce)
{
	Assert(aiForce->State == AI_FORCE_STATE_BOARDING);

	unsigned int nbToTransport = 0;
	unsigned int transporterIndex = 0;
	bool goNext = true;

	for (; transporterIndex < aiForce->Size(); ++transporterIndex) {
		const CUnit *unit = aiForce->Units[transporterIndex];

		if (unit->Type->CanTransport() && unit->Type->MaxOnBoard - unit->BoardCount > 0) {
			nbToTransport = unit->Type->MaxOnBoard - unit->BoardCount;
			break;
		}
	}
	if (transporterIndex == aiForce->Size()) {
		aiForce->State++;
		return ;
	}
	for (unsigned int i = 0; i < aiForce->Size(); ++i) {
		const CUnit &unit = *aiForce->Units[i];
		const CUnit &transporter = *aiForce->Units[transporterIndex];

		if (CanTransport(transporter, unit) && unit.Container == NULL) {
			goNext = false;
		}
	}
	if (goNext == true) {
		aiForce->State++;
		return ;
	}
	for (unsigned int i = 0; i < aiForce->Size(); ++i) {
		CUnit &unit = *aiForce->Units[i];
		CUnit &transporter = *aiForce->Units[transporterIndex];

		if (transporter.IsIdle() && unit.CurrentOrder()->GetGoal() == &transporter) {
			CommandFollow(transporter, unit, 0);
		}
		if (CanTransport(transporter, unit) && unit.IsIdle() && unit.Container == NULL) {
			CommandBoard(unit, transporter, FlushCommands);
			CommandFollow(transporter, unit, 0);
			if (--nbToTransport == 0) { // full : nxt transporter.
				for (++transporterIndex; transporterIndex < aiForce->Size(); ++transporterIndex) {
					const CUnit &nextTransporter = *aiForce->Units[transporterIndex];

					if (nextTransporter.Type->CanTransport()) {
						nbToTransport = nextTransporter.Type->MaxOnBoard - nextTransporter.BoardCount;
						break ;
					}
				}
				if (transporterIndex == aiForce->Size()) { // No more transporter.
					break ;
				}
			}
		}
	}
}

/**
** Force on attack ride. We attack until there is no unit or enemy left.
**
** @param force Force pointer.
*/
void AiForce::Update()
{
	const CUnit *unit = NULL;

	if (Size() == 0) {
		Attacking = false;
		if (!Defending && State > 0) {
			DebugPrint("%d: Attack force #%lu was destroyed, giving up\n"
				_C_ AiPlayer->Player->Index _C_ (long unsigned int)(this  - &(AiPlayer->Force[0])));
			Reset(true);
		}
		return;
	}

	Attacking = false;
	for (unsigned int i = 0; i < Size(); ++i) {
		CUnit *aiunit = Units[i];
		if (aiunit->Type->CanAttack) {
			Attacking = true;
			break;
		}
	}
	if (Attacking == false) {
		if (!Defending && State > 0) {
			DebugPrint("%d: Attack force #%lu has lost all agresive units, giving up\n"
				_C_ AiPlayer->Player->Index _C_ (long unsigned int)(this  - &(AiPlayer->Force[0])));
			Reset(true);
		}
		return ;
	}

#if 0
	if (State == AI_FORCE_STATE_WAITING) {
		if (!AiPlanAttack(force)) {
			DebugPrint("Can't transport, look for walls\n");
			if (!AiFindWall(force)) {
				Attacking = false;
				return ;
			}
		}
		State = AI_FORCE_STATE_BOARDING;
	}
#endif

	if (State == AI_FORCE_STATE_BOARDING) {
		AiGroupAttackerForTransport(this);
		return ;
	}


	// Find a unit that isn't idle
	unit = NoUnitP;
	if (State == AI_FORCE_STATE_ATTACKING) {
		for (unsigned int i = 0; i < Size(); ++i) {
			CUnit *aiunit = Units[i];
			if (!aiunit->IsIdle()) {
				// Found an idle unit, use it if we find nothing better
				if (unit == NoUnitP) {
					unit = aiunit;
				}
				// If the unit has a goal use it
				if (aiunit->CurrentOrder()->HasGoal()) {
					unit = aiunit;
					break;
				}
			}
		}
	}
	if (unit != NoUnitP) {
		// Give idle units a new goal
		// FIXME: may not be a good goal
		COrderPtr order = unit->CurrentOrder();
		Vec2i pos;
		if (order->HasGoal()) {
			pos = order->GetGoal()->tilePos;
		} else if (order->goalPos.x != -1 && order->goalPos.y != -1) {
			pos = order->goalPos;
		} else {
			pos = this->GoalPos;
		}
		for (unsigned int i = 0; i < Size(); ++i) {
			CUnit *aiunit = Units[i];
			if (!aiunit->IsIdle()) {
				continue;
			}
			if (aiunit->Type->CanAttack) {
				CommandAttack(*aiunit, pos, NULL, FlushCommands);
			} else if (aiunit->Type->CanTransport()) {
				const Vec2i startPos = {aiunit->Player->StartX, aiunit->Player->StartY};
				// FIXME : Retrieve unit blocked (transport previously full)
				CommandMove(*aiunit, startPos, FlushCommands);
			} else {
				CommandMove(*aiunit, pos, FlushCommands);
			}
		}
	} else { // Everyone is idle, find a new target
		Vec2i pos;
//FIXME: rb - I don't know if AI can use transport now
#if 0
		if (State == AI_FORCE_STATE_ATTACKING) {
			unit = NULL;

			for (unsigned int i = 0; i < Units.size(); ++i) {
				CUnit *aiunit = Units[i];
				if (aiunit->Type->CanAttack) {
					unit = AttackUnitsInDistance(aiunit, MaxMapWidth);
					break;
				}
			}
			if (!unit) {
				// No enemy found, give up
				// FIXME: should the force go home or keep trying to attack?
				DebugPrint("Attack force can't find a target, giving up\n");
				Attacking = false;
				return;
			}
			pos = unit->tilePos;
		} else {
			pos = this->GoalPos;
		}
		for (unsigned int i = 0; i < Units.size(); ++i) {
			CUnit *aiunit = Units[i];

			if (aiunit->Type->CanTransport() && aiunit->BoardCount > 0) {
				CommandUnload(aiunit, pos, NULL, FlushCommands);
			} else if (aiunit->Type->CanAttack) {
				CommandAttack(aiunit, pos, NULL, FlushCommands);
			} else if (force->State == 2) {
				CommandMove(aiunit, pos, FlushCommands);
			}
		}
		force->State = 3;
#else
		if (State == AI_FORCE_STATE_ATTACKING) {

			unit = AiForceEnemyFinder<false>(this).enemy;

			if (!unit) {
				// No enemy found, give up
				// FIXME: should the force go home or keep trying to attack?
				DebugPrint("%d: Attack force #%lu can't find a target, giving up\n"
					_C_ AiPlayer->Player->Index _C_ (long unsigned int)(this  - &(AiPlayer->Force[0])));
				Attacking = false;
				return;
			}
			pos = unit->tilePos;
		} else {
			pos = this->GoalPos;
		}
		AiForceAttackSender(this, pos);
#endif
	}
}

void AiForceManager::Update(void)
{
	for(unsigned int f = 0; f < forces.size(); ++f)
	{
		AiForce *force = &forces[f];
		//
		//  Look if our defenders still have enemies in range.
		//
		if (force->Defending) {
			force->Clean();
			//
			//  Look if still enemies in attack range.
			//
			if(!AiForceEnemyFinder<true>(force).found()) {
				DebugPrint("%d:FIXME: not written, should send force #%d home\n"
					_C_ AiPlayer->Player->Index _C_ f);
				force->Defending = false;
				force->Attacking = false;
			}
		} else if (force->Attacking) {
			force->Clean();
			force->Update();
		}
	}
}

/**
**  Entry point of force manager, perodic called.
**
** @todo FIXME: is this really needed anymore
*/
void AiForceManager(void)
{
	AiPlayer->Force.Update();
	AiAssignFreeUnitsToForce();
}

//@}
