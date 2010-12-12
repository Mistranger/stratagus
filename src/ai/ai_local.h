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
/**@name ai_local.h - The local AI header file. */
//
//      (c) Copyright 2000-2005 by Lutz Sammer and Antonis Chaniotis.
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

#ifndef __AI_LOCAL_H__
#define __AI_LOCAL_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <vector>

#include "upgrade_structs.h"
#include "unit.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUnit;
class CUnitType;
class CUpgrade;
class CPlayer;

/**
**  Ai Type structure.
*/
class CAiType {
public:
	CAiType() {}

	std::string Name;     /// Name of this ai
	std::string Race;     /// for this race
	std::string Class;    /// class of this ai

#if 0
	// nice flags
	unsigned char AllExplored : 1; /// Ai sees unexplored area
	unsigned char AllVisbile : 1;  /// Ai sees invisibile area
#endif

	std::string Script;       /// Main script
};

/**
**  AI unit-type table with counter in front.
*/
class AiRequestType {
public:
	AiRequestType() : Count(0), Type(NULL) {}

	unsigned int Count;  /// elements in table
	CUnitType *Type;     /// the type
};

/**
**  Ai unit-type in a force.
*/
class AiUnitType {
public:
	AiUnitType() : Want(0), Type(NULL) {}

	unsigned int Want; /// number of this unit-type wanted
	CUnitType *Type; /// unit-type self
};

/**
**  Roles for forces
*/
enum AiForceRole {
	AiForceRoleAttack = 0, /// Force should attack
	AiForceRoleDefend      /// Force should defend
};

#define AI_FORCE_STATE_FREE			-1
#define AI_FORCE_STATE_WAITING		0
#define AI_FORCE_STATE_BOARDING 	1
//#define AI_FORCE_STATE_OERATIONAL 	2
#define AI_FORCE_STATE_ATTACKING	3

/**
**  Define an AI force.
**
**  A force is a group of units belonging together.
*/
class AiForce {
	friend class AiForceManager;

	bool IsBelongsTo(const CUnitType *type);
	void Insert(CUnit &unit)
	{
		Units.Insert(&unit);
		unit.RefsIncrease();
	}

	void Update(void);
public:
	AiForce() : Completed(false), Defending(false), Attacking(false),
		Role(0), State(AI_FORCE_STATE_FREE),
		MustTransport(false)
	{
		GoalPos.x = GoalPos.y = 0;
	}

	void Remove(CUnit &unit)
	{
		if (Units.Remove(&unit)) {
			unit.GroupId = 0;
			unit.RefsDecrease();
		}
	}

	/**
	**  Reset the force. But don't change its role and its demand.
	*/
	void Reset(bool types = false) {
		Completed = false;
		Defending = false;
		Attacking = false;
		if (types) {
			UnitTypes.clear();
			State = AI_FORCE_STATE_FREE;
		} else {
			State = AI_FORCE_STATE_WAITING;
		}
		Units.clear();
		GoalPos.x = GoalPos.y = 0;
		MustTransport = false;
	}
	inline size_t Size(void)
	{
		return Units.size();
	}

	inline bool IsAttacking(void) const
	{
		return (!Defending && Attacking);
	}

	void CountTypes(unsigned int *counter, const size_t len);

	bool Completed;     /// Flag saying force is complete build
	bool Defending;     /// Flag saying force is defending
	bool Attacking;     /// Flag saying force is attacking
	char Role;          /// Role of the force

	std::vector<AiUnitType> UnitTypes; /// Count and types of unit-type
	CUnitCache Units;						/// Units in the force

	//
	// If attacking
	//
	int State;/// Attack state
	Vec2i GoalPos; /// Attack point tile map position
	bool MustTransport;/// Flag must use transporter

	void Attack(const Vec2i &pos);
	void Clean(void);
	int PlanAttack(void);
};

	// forces
#define AI_MAX_FORCES 10                    /// How many forces are supported
//#define AI_MAX_ATTACKING_FORCES 30          /// Attacking forces (max supported 32)
/**
**  AI force manager.
**
**  A Forces container for the force manager to handle
*/
class AiForceManager {
	std::vector<AiForce> forces;
	char script[AI_MAX_FORCES];
public:
	AiForceManager();

	inline size_t Size() const
	{
		return forces.size();
	}

	AiForce &operator[](unsigned int index) {
		return forces[index];
	}

	int getIndex(AiForce *force) {
		for (unsigned int i = 0; i < forces.size(); ++i) {
			if (force == &forces[i]) return i;
		}
		return -1;
	}


	inline unsigned int getScriptForce(unsigned int index) {
		if (script[index] == -1) {
			script[index] = FindFreeForce();
		}
		return script[index];
	}

	void Clean();
	bool Assign(CUnit &unit);
	void Update();
	unsigned int FindFreeForce(int role = AiForceRoleAttack);
	void CheckUnits(int *counter);
};

/**
**  AI build queue.
**
**  List of orders for the resource manager to handle
*/
class AiBuildQueue {
public:
	AiBuildQueue() : Want(0), Made(0), Type(NULL), Wait(0), X(-1), Y(-1)  {}

	unsigned int Want;  /// requested number
	unsigned int Made;  /// built number
	CUnitType *Type;    /// unit-type
	unsigned long Wait; /// wait until this cycle
	short int X;              /// build near x pos on map
	short int Y;              /// build near y pos on map
};

/**
**  AI exploration request
*/
class AiExplorationRequest {
public:
	AiExplorationRequest(const Vec2i& pos, int mask) : pos(pos), Mask(mask) { }

	Vec2i pos;          /// pos on map
	int Mask;           /// mask ( ex: MapFieldLandUnit )
};

/**
**  AI transport request
*/
class AiTransportRequest {
public:
	AiTransportRequest() : Unit(NULL) {}

	CUnit *Unit;
	CUnit::COrder Order;
};

/**
**  AI variables.
*/
class PlayerAi {
public:
	PlayerAi() : Player(NULL), AiType(NULL),
		SleepCycles(0), NeededMask(0), NeedSupply(false),
		ScriptDebug(false), LastExplorationGameCycle(0),
		LastCanNotMoveGameCycle(0),	LastRepairBuilding(0)
	{
		memset(Reserve, 0, sizeof(Reserve));
		memset(Used, 0, sizeof(Used));
		memset(Needed, 0, sizeof(Needed));
		memset(Collect, 0, sizeof(Collect));
		memset(TriedRepairWorkers, 0, sizeof(TriedRepairWorkers));
	}

	CPlayer *Player;               /// Engine player structure
	CAiType *AiType;               /// AI type of this player AI
	// controller
	std::string Script;            /// Script executed
	unsigned long SleepCycles;     /// Cycles to sleep

	AiForceManager	Force;		/// Forces controlled by AI

	// resource manager
	int Reserve[MaxCosts]; /// Resources to keep in reserve
	int Used[MaxCosts];    /// Used resources
	int Needed[MaxCosts];  /// Needed resources
	int Collect[MaxCosts]; /// Collect % of resources
	int NeededMask;        /// Mask for needed resources
	bool NeedSupply;       /// Flag need food
	bool ScriptDebug;              /// Flag script debuging on/off

	std::vector<AiExplorationRequest> FirstExplorationRequest;/// Requests for exploration
	unsigned long LastExplorationGameCycle;         /// When did the last explore occur?
	std::vector<AiTransportRequest> TransportRequests;/// Requests for transport
	unsigned long LastCanNotMoveGameCycle;          /// Last can not move cycle
	std::vector<AiRequestType> UnitTypeRequests;    /// unit-types to build/train request,priority list
	std::vector<CUnitType *> UpgradeToRequests;     /// Upgrade to unit-type requested and priority list
	std::vector<CUpgrade *> ResearchRequests;       /// Upgrades requested and priority list
	std::vector<AiBuildQueue> UnitTypeBuilt;        /// What the resource manager should build
	int LastRepairBuilding;                         /// Last building checked for repair in this turn
	unsigned int TriedRepairWorkers[UnitMax];           /// No. workers that failed trying to repair a building
};

/**
**  AI Helper.
**
**  Contains informations needed for the AI. If the AI needs an unit or
**  building or upgrade or spell, it could lookup in this tables to find
**  where it could be trained, built or researched.
*/
class AiHelper {
public:
	/**
	** The index is the unit that should be trained, giving a table of all
	** units/buildings which could train this unit.
	*/
	std::vector<std::vector<CUnitType *> > Train;
	/**
	** The index is the unit that should be build, giving a table of all
	** units/buildings which could build this unit.
	*/
	std::vector<std::vector<CUnitType *> > Build;
	/**
	** The index is the upgrade that should be made, giving a table of all
	** units/buildings which could do the upgrade.
	*/
	std::vector<std::vector<CUnitType *> > Upgrade;
	/**
	** The index is the research that should be made, giving a table of all
	** units/buildings which could research this upgrade.
	*/
	std::vector<std::vector<CUnitType *> > Research;
	/**
	** The index is the unit that should be repaired, giving a table of all
	** units/buildings which could repair this unit.
	*/
	std::vector<std::vector<CUnitType *> > Repair;
	/**
	** The index is the unit-limit that should be solved, giving a table of all
	** units/buildings which could reduce this unit-limit.
	*/
	std::vector<std::vector<CUnitType *> > UnitLimit;
	/**
	** The index is the unit that should be made, giving a table of all
	** units/buildings which are equivalent.
	*/
	std::vector<std::vector<CUnitType *> > Equiv;

	/**
	** The index is the resource id - 1 (we can't mine TIME), giving a table of all
	** units/buildings/mines which can harvest this resource.
	*/
	std::vector<std::vector<CUnitType *> > Refinery;

	/**
	** The index is the resource id - 1 (we can't store TIME), giving a table of all
	** units/buildings/mines which can store this resource.
	*/
	std::vector<std::vector<CUnitType *> > Depots;


};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern std::vector<CAiType *> AiTypes;   /// List of all AI types
extern AiHelper AiHelpers; /// AI helper variables

extern int UnitTypeEquivs[UnitTypeMax + 1]; /// equivalence between unittypes
extern PlayerAi *AiPlayer; /// Current AI player

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

//
// Resource manager
//
	/// Add unit-type request to resource manager
extern void AiAddUnitTypeRequest(CUnitType &type, int count);
	/// Add upgrade-to request to resource manager
extern void AiAddUpgradeToRequest(CUnitType &type);
	/// Add research request to resource manager
extern void AiAddResearchRequest(CUpgrade *upgrade);
	/// Periodic called resource manager handler
extern void AiResourceManager();
	/// Ask the ai to explore around pos
extern void AiExplore(const Vec2i &pos, int exploreMask);
	/// Make two unittypes be considered equals
extern void AiNewUnitTypeEquiv(CUnitType *a, CUnitType *b);
	/// Remove any equivalence between unittypes
extern void AiResetUnitTypeEquiv();
	/// Finds all equivalents units to a given one
extern int AiFindUnitTypeEquiv(const CUnitType &type, int *result);
	/// Finds all available equivalents units to a given one, in the prefered order
extern int AiFindAvailableUnitTypeEquiv(const CUnitType &type, int *result);
extern int AiGetBuildRequestsCount(PlayerAi*,int counter[UnitTypeMax]);

extern void AiNewDepotRequest(CUnit &worker);

//
// Buildings
//
	/// Find nice building place
extern int AiFindBuildingPlace(const CUnit &worker,
	const CUnitType &type, int nx, int ny, Vec2i *dpos);

//
// Forces
//
	/// Cleanup units in force
extern void AiCleanForces(void);
	/// Assign a new unit to a force
extern bool AiAssignToForce(CUnit &unit);
	/// Assign a free units to a force
extern void AiAssignFreeUnitsToForce(void);
	/// Attack with force at position
extern void AiAttackWithForceAt(unsigned int force, int x, int y);
	/// Attack with force
extern void AiAttackWithForce(unsigned int force);
	/// Attack with forces in array
extern void AiAttackWithForces(int *forces);

	/// Periodic called force manager handler
extern void AiForceManager(void);

//
// Plans
//
	/// Find a wall to attack
extern int AiFindWall(AiForce *force);
	/// Plan the an attack
	/// Send explorers around the map
extern void AiSendExplorers(void);
	/// Enemy units in distance
extern int AiEnemyUnitsInDistance(const CPlayer *player, const CUnitType *type,
	const Vec2i& pos, unsigned range);

//
// Magic
//
	/// Check for magic
extern void AiCheckMagic(void);

//@}

#endif // !__AI_LOCAL_H__
