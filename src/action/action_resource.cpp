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
/**@name action_resource.cpp - The generic resource action. */
//
//      (c) Copyright 2001-2005 by Crestez Leonard and Jimmy Salmon
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"

#include "action/action_resource.h"

#include "animation.h"
#include "interface.h"
#include "iolib.h"
#include "map.h"
#include "pathfinder.h"
#include "player.h"
#include "script.h"
#include "sound.h"
#include "ui.h"
#include "unit.h"
#include "unittype.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

#define SUB_START_RESOURCE 0
#define SUB_MOVE_TO_RESOURCE 5
#define SUB_UNREACHABLE_RESOURCE 31
#define SUB_START_GATHERING 55
#define SUB_GATHER_RESOURCE 60
#define SUB_STOP_GATHERING 65
#define SUB_MOVE_TO_DEPOT 70
#define SUB_UNREACHABLE_DEPOT 100
#define SUB_RETURN_RESOURCE 120

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/* static */ COrder* COrder::NewActionResource(CUnit &harvester, const Vec2i &pos)
{
	COrder_Resource *order = new COrder_Resource(harvester);
	Vec2i ressourceLoc;

	//  Find the closest piece of wood next to a tile where the unit can move
	if (!FindTerrainType(0, (harvester.Type->MovementMask), 1, 20, *harvester.Player, pos, &ressourceLoc)) {
		DebugPrint("FIXME: Give up???\n");
	}
	// Max Value > 1
	if ((MyAbs(ressourceLoc.x - pos.x) | MyAbs(ressourceLoc.y - pos.y)) > 1) {
		if (!FindTerrainType(0, MapFieldForest, 0, 20, *harvester.Player, ressourceLoc, &ressourceLoc)) {
			DebugPrint("FIXME: Give up???\n");
		}
	} else {
		// The destination is next to a reachable tile.
		ressourceLoc = pos;
	}
	order->goalPos = ressourceLoc;
	order->CurrentResource = WoodCost; // Hard-coded resource.
	order->Range = 1;
	return order;
}

/* static */ COrder* COrder::NewActionResource(CUnit &harvester, CUnit &mine)
{
	COrder_Resource *order = new COrder_Resource(harvester);

	order->SetGoal(&mine);
	order->Resource.Mine = &mine;
	harvester.AssignWorkerToMine(mine);
	order->Resource.Pos = mine.tilePos + mine.Type->GetHalfTileSize();
	order->CurrentResource = mine.Type->GivesResource;
	order->Range = 1;
	return order;
}

/* static */ COrder* COrder::NewActionReturnGoods(CUnit &harvester, CUnit *depot)
{
	COrder_Resource *order = new COrder_Resource(harvester);

	// Destination could be killed. NETWORK!
	if (depot && depot->Destroyed) {
		depot = NULL;
	}
	order->Range = 1;
	order->CurrentResource = harvester.CurrentResource;
	order->DoneHarvesting = true;

	if (depot == NULL) {
		depot = FindDeposit(harvester, 1000, harvester.CurrentResource);
	}
	order->Depot = depot;
	if (depot) {
		order->UnitGotoGoal(harvester, depot, 70); //SUB_MOVE_TO_DEPOT);
	}
	return order;
}


Vec2i COrder_Resource::GetHarvestLocation() const
{
	if (this->Resource.Mine != NULL) {
		return this->Resource.Mine->tilePos;
	} else {
		return this->Resource.Pos;
	}
}

bool COrder_Resource::IsGatheringStarted() const
{
	return this->State > SUB_START_GATHERING;
}

bool COrder_Resource::IsGatheringFinished() const
{
	return this->State >= SUB_STOP_GATHERING;
}

bool COrder_Resource::IsGatheringWaiting() const
{
	return this->State == SUB_START_GATHERING && this->worker->Wait != 0;
}

COrder_Resource::~COrder_Resource()
{
	CUnit *mine = this->Resource.Mine;

	if (mine && mine->IsAlive()) {
		worker->DeAssignWorkerFromMine(*mine);
	}
	if (this->HasGoal() && this->GetGoal()->IsAlive()) {
		// If mining decrease the active count on the resource.
		if (this->State == SUB_GATHER_RESOURCE ) {
			CUnit *goal = this->GetGoal();

			goal->Resource.Active--;
			Assert(goal->Resource.Active >= 0);
		}
	}
}

/* virtual */ void COrder_Resource::Save(CFile &file, const CUnit &unit) const
{
	file.printf("{\"action-resource\",");
	if (this->Finished) {
		file.printf(" \"finished\",");
	}
	file.printf(" \"range\", %d,", this->Range);
	if (this->HasGoal()) {
		CUnit &goal = *this->GetGoal();
		if (goal.Destroyed) {
			/* this unit is destroyed so it's not in the global unit
			 * array - this means it won't be saved!!! */
			printf ("FIXME: storing destroyed Goal - loading will fail.\n");
		}
		file.printf(" \"goal\", \"%s\",", UnitReference(goal).c_str());
	}
	file.printf(" \"tile\", {%d, %d},", this->goalPos.x, this->goalPos.y);

	Assert(this->worker != NULL && worker->IsAlive());
	file.printf(" \"worker\", \"%s\",", UnitReference(worker).c_str());
	file.printf(" \"current-res\", %d,", this->CurrentResource);

	file.printf(" \"res-pos\", {%d, %d},", this->Resource.Pos.x, this->Resource.Pos.y);
	if (this->Resource.Mine != NULL) {
		file.printf(" \"res-mine\", \"%s\",", UnitReference(this->Resource.Mine).c_str());
	}
	if (this->Depot != NULL) {
		file.printf(" \"res-depot\", \"%s\",", UnitReference(this->Depot).c_str());
	}
	if (this->DoneHarvesting) {
		file.printf(" \"done-harvesting\",");
	}
	file.printf(" \"timetoharvest\", %d,", this->TimeToHarvest);
	file.printf(" \"state\", %d", this->State);
	file.printf("}");
}

/* virtual */ bool COrder_Resource::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	if (!strcmp(value, "current-res")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->CurrentResource = LuaToNumber(l, -1);
		lua_pop(l, 1);
	} else if (!strcmp(value, "done-harvesting")) {
		lua_rawgeti(l, -1, j + 1);
		this->DoneHarvesting = true;
		lua_pop(l, 1);
	} else if (!strcmp(value, "res-depot")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->Depot = CclGetUnitFromRef(l);
		lua_pop(l, 1);
	} else if (!strcmp(value, "res-mine")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->Resource.Mine = CclGetUnitFromRef(l);
		lua_pop(l, 1);
	} else if (!strcmp(value, "res-pos")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		CclGetPos(l, &this->Resource.Pos.x , &this->Resource.Pos.y);
		lua_pop(l, 1);
	} else if (!strcmp(value, "state")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->State = LuaToNumber(l, -1);
		lua_pop(l, 1);
	} else if (!strcmp(value, "timetoharvest")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->TimeToHarvest = LuaToNumber(l, -1);
		lua_pop(l, 1);
	} else if (!strcmp(value, "worker")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->worker = CclGetUnitFromRef(l);
		lua_pop(l, 1);
	} else {
		return false;
	}
	return true;
}

/* virtual */ PixelPos COrder_Resource::Show(const CViewport& vp, const PixelPos& lastScreenPos) const
{
	PixelPos targetPos;

	if (this->HasGoal()) {
		targetPos = vp.MapToScreenPixelPos(this->GetGoal()->GetMapPixelPosCenter());
	} else {
		targetPos = vp.TilePosToScreen_Center(this->goalPos);
	}
	Video.FillCircleClip(ColorYellow, lastScreenPos, 2);
	Video.DrawLineClip(ColorYellow, lastScreenPos, targetPos);
	Video.FillCircleClip(ColorYellow, targetPos, 3);
	return targetPos;
}


/* virtual */ bool COrder_Resource::OnAiHitUnit(CUnit &unit, CUnit *attacker, int /* damage*/)
{
	if (this->IsGatheringFinished()) {
		// Normal return to depot
		return true;
	}
	if (this->IsGatheringStarted()  && unit.ResourcesHeld > 0) {
		// escape to Depot with what you have
		this->DoneHarvesting = true;
		return true;
	}
	return false;
}



/**
**  Move unit to terrain.
**
**  @return      1 if reached, -1 if unreacheable, 0 if on the way.
*/
int COrder_Resource::MoveToResource_Terrain(CUnit &unit)
{
	Vec2i pos = unit.CurrentOrder()->goalPos;

	// Wood gone, look somewhere else.
	if ((Map.Info.IsPointOnMap(pos) == false || Map.ForestOnMap(pos) == false) && (!unit.IX) && (!unit.IY)) {
		if (!FindTerrainType(unit.Type->MovementMask, MapFieldForest, 0, 16,
				*unit.Player, unit.CurrentOrder()->goalPos, &pos)) {
			// no wood in range
			return -1;
		} else {
			this->goalPos = pos;
		}
	}
	switch (DoActionMove(unit)) {
		case PF_UNREACHABLE:
			unit.Wait = 10;
			if (FindTerrainType(unit.Type->MovementMask, MapFieldForest, 0, 9999,
					*unit.Player, unit.tilePos, &pos)) {
				this->goalPos = pos;
				DebugPrint("Found a better place to harvest %d,%d\n" _C_ pos.x _C_ pos.y);
				// FIXME: can't this overflow? It really shouldn't, since
				// x and y are really supossed to be reachable, checked thorugh a flood fill.
				// I don't know, sometimes stuff happens.
				return 0;
			}
			return -1;
		case PF_REACHED:
			return 1;
		default:
			return 0;
	}
}

/**
**  Move unit to unit resource.
**
**  @return      1 if reached, -1 if unreacheable, 0 if on the way.
*/
int COrder_Resource::MoveToResource_Unit(CUnit &unit)
{
	const CUnit *goal = this->GetGoal();
	Assert(goal);

	switch (DoActionMove(unit)) { // reached end-point?
		case PF_UNREACHABLE:
			return -1;
		case PF_REACHED:
			break;
		default:
			// Goal gone or something.
			if (unit.Anim.Unbreakable || goal->IsVisibleAsGoal(*unit.Player)) {
				return 0;
			}
			break;
	}
	return 1;
}

/**
**  Move unit to resource.
**
**  @param unit  Pointer to unit.
**
**  @return      1 if reached, -1 if unreacheable, 0 if on the way.
*/
int COrder_Resource::MoveToResource(CUnit &unit)
{
	const ResourceInfo &resinfo = *unit.Type->ResInfo[unit.CurrentResource];

	if (resinfo.TerrainHarvester) {
		return MoveToResource_Terrain(unit);
	} else {
		return MoveToResource_Unit(unit);
	}
}

void COrder_Resource::UnitGotoGoal(CUnit &unit, CUnit *const goal, int state)
{
	if (this->GetGoal() != goal) {
		this->SetGoal(goal);
		if (goal) {
			this->goalPos.x = this->goalPos.y = -1;
		}
	}
	this->Range = 1;
	this->State = state;
	unit.State = 0;
	if (state == SUB_MOVE_TO_DEPOT || state == SUB_MOVE_TO_RESOURCE) {
		unit.pathFinderData->output.Cycles = 0; //moving counter
	}
}

/**
**  Start harvesting the resource.
**
**  @param unit  Pointer to unit.
**
**  @return      TRUE if ready, otherwise FALSE.
*/
int COrder_Resource::StartGathering(CUnit &unit)
{
	CUnit *goal;
	const ResourceInfo &resinfo = *unit.Type->ResInfo[unit.CurrentResource];
	Assert(!unit.IX);
	Assert(!unit.IY);

	if (resinfo.TerrainHarvester) {
		// This shouldn't happend?
#if 0
		if (!Map.ForestOnMap(unit.Orders->goalPos)) {
			DebugPrint("Wood gone, just like that?\n");
			return 0;
		}
#endif
		UnitHeadingFromDeltaXY(unit, this->goalPos - unit.tilePos);
		if (resinfo.WaitAtResource) {
			this->TimeToHarvest = resinfo.WaitAtResource / SpeedResourcesHarvest[resinfo.ResourceId];
		} else {
			this->TimeToHarvest = 1;
		}
		this->DoneHarvesting = 0;
		return 1;
	}

	goal = this->GetGoal();

	// Target is dead, stop getting resources.
	if (!goal->IsVisibleAsGoal(*unit.Player)) {
		// Find an alternative, but don't look too far.
		this->goalPos.x = -1;
		this->goalPos.y = -1;
		if ((goal = UnitFindResource(unit, unit.tilePos, 15, unit.CurrentResource, unit.Player->AiEnabled))) {
			this->State = SUB_START_RESOURCE;
			this->SetGoal(goal);
		} else {
			this->ClearGoal();
			this->Finished = true;
		}
		return 0;
	}

	// FIXME: 0 can happen, if to near placed by map designer.
	Assert(unit.MapDistanceTo(*goal) <= 1);

	// Update the heading of a harvesting unit to looks straight at the resource.
	if (goal) {
		UnitHeadingFromDeltaXY(unit, goal->tilePos - unit.tilePos + goal->Type->GetHalfTileSize());
	}

	// If resource is still under construction, wait!
	if ((goal->Type->MaxOnBoard &&
		 goal->Resource.Active >= goal->Type->MaxOnBoard) ||
			goal->CurrentAction() == UnitActionBuilt) {
		// FIXME: Determine somehow when the resource will be free to use
		// FIXME: Could we somehow find another resource? Think minerals
		// FIXME: We should add a flag for that, and a limited range.
		// FIXME: Think minerals in st*rcr*ft!!
		// However the CPU usage is really low (no pathfinding stuff).
		unit.Wait = 10;
		return 0;
	}

	// Place unit inside the resource
	if (!resinfo.HarvestFromOutside) {
		if (goal->Variable[MAXHARVESTERS_INDEX].Value == 0 || goal->Variable[MAXHARVESTERS_INDEX].Value > goal->InsideCount) {
			this->ClearGoal();
			unit.Remove(goal);
		} else if (goal->Variable[MAXHARVESTERS_INDEX].Value <= goal->InsideCount) {
			//Resource is full, wait
			unit.Wait = 10;
			return 0;
		}
	}

	// Activate the resource
	goal->Resource.Active++;

	if (resinfo.WaitAtResource) {
		this->TimeToHarvest = resinfo.WaitAtResource / SpeedResourcesHarvest[resinfo.ResourceId];
	} else {
		this->TimeToHarvest = 1;
	}
	this->DoneHarvesting = 0;
	return 1;
}

/**
**  Animate a unit that is harvesting
**
**  @param unit  Unit to animate
*/
static void AnimateActionHarvest(CUnit &unit)
{
	Assert(unit.Type->Animations->Harvest[unit.CurrentResource]);
	UnitShowAnimation(unit, unit.Type->Animations->Harvest[unit.CurrentResource]);
}

/**
**  Find something else to do when the resource is exhausted.
**  This is called from GatherResource when the resource is empty.
**
**  @param unit    pointer to harvester unit.
**  @param source  pointer to resource unit.
*/
void COrder_Resource::LoseResource(CUnit &unit, const CUnit &source)
{
	CUnit *depot;
	const ResourceInfo &resinfo = *unit.Type->ResInfo[unit.CurrentResource];

	Assert((unit.Container == &source && !resinfo.HarvestFromOutside)
		|| (!unit.Container && resinfo.HarvestFromOutside));

	if (resinfo.HarvestFromOutside) {
		this->ClearGoal();
	}

	// Continue to harvest if we aren't fully loaded
	if (resinfo.HarvestFromOutside && unit.ResourcesHeld < resinfo.ResourceCapacity){
		if (unit.Container) {
			DropOutOnSide(unit, LookingW, &source);
		}
		const Vec2i pos = unit.tilePos;
		CUnit *goal = UnitFindResource(unit, pos, 15, unit.CurrentResource, 1);

		if (goal) {
			CUnit *mine = this->Resource.Mine;

			if (mine) {
				unit.DeAssignWorkerFromMine(*mine);
			}
			unit.AssignWorkerToMine(*goal);
			this->SetGoal(goal);
			this->Resource.Mine = goal;
			this->Range = 1;
			this->goalPos = goal->tilePos;
			this->State = SUB_MOVE_TO_RESOURCE;
			unit.State = 0;
			return;
		}
	}

	// If we are fully loaded first search for a depot.
	if (unit.ResourcesHeld && (depot = FindDeposit(unit, 1000, unit.CurrentResource))) {
		if (unit.Container) {
			DropOutNearest(unit, depot->tilePos + depot->Type->GetHalfTileSize(), &source);
		}
		// Remember where it mined, so it can look around for another resource.
		//
		//FIXME!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//unit.CurrentOrder()->Arg1.ResourcePos = (unit.X << 16) | unit.Y;
		this->DoneHarvesting = true;
		UnitGotoGoal(unit, depot, SUB_MOVE_TO_DEPOT);
		DebugPrint("%d: Worker %d report: Resource is exhausted, Going to depot\n"
			_C_ unit.Player->Index _C_ unit.Slot);
		return;
	}
	// No depot found, or harvester empty
	// Dump the unit outside and look for something to do.
	if (unit.Container) {
		Assert(!resinfo.HarvestFromOutside);
		DropOutOnSide(unit, LookingW, &source);
	}
	this->goalPos.x = -1;
	this->goalPos.y = -1;
	//use depot as goal
	depot = UnitFindResource(unit, unit.tilePos, 15, unit.CurrentResource, unit.Player->AiEnabled);
	if (depot) {
		DebugPrint("%d: Worker %d report: Resource is exhausted, Found another resource.\n"
			_C_ unit.Player->Index _C_ unit.Slot);
		this->State = SUB_START_RESOURCE;
		unit.State = 0;
		this->SetGoal(depot);
	} else {
		DebugPrint("%d: Worker %d report: Resource is exhausted, Just sits around confused.\n"
			_C_ unit.Player->Index _C_ unit.Slot);
		this->Finished = true;
		unit.State = 0;
	}
}



/**
**  Gather the resource
**
**  @param unit  Pointer to unit.
**
**  @return      non-zero if ready, otherwise zero.
*/
int COrder_Resource::GatherResource(CUnit &unit)
{
	CUnit *source = 0;
	const ResourceInfo &resinfo = *unit.Type->ResInfo[unit.CurrentResource];
	int addload;

	if (!resinfo.HarvestFromOutside && unit.Container != NULL) {
		UnitShowAnimation(*unit.Container, unit.Container->Type->Animations->Harvest[unit.CurrentResource]);
	}

	if (resinfo.HarvestFromOutside || resinfo.TerrainHarvester) {
		AnimateActionHarvest(unit);
	} else {
		unit.Anim.CurrAnim = NULL;
	}

	this->TimeToHarvest--;

	if (this->DoneHarvesting) {
		Assert(resinfo.HarvestFromOutside || resinfo.TerrainHarvester);
		return !unit.Anim.Unbreakable;
	}

	// Target gone?
	if (resinfo.TerrainHarvester && !Map.ForestOnMap(this->goalPos)) {
		if (!unit.Anim.Unbreakable) {
			// Action now breakable, move to resource again.
			this->State = SUB_MOVE_TO_RESOURCE;
			// Give it some reasonable look while searching.
			// FIXME: which frame?
			unit.Frame = 0;
		}
		return 0;
		// No wood? Freeze!!!
	}

	while (!this->DoneHarvesting && this->TimeToHarvest < 0) {
		//FIXME: rb - how should it look for WaitAtResource == 0
		if (resinfo.WaitAtResource) {
			this->TimeToHarvest += resinfo.WaitAtResource / SpeedResourcesHarvest[resinfo.ResourceId];
		} else {
			this->TimeToHarvest += 1;
		}

		// Calculate how much we can load.
		if (resinfo.ResourceStep) {
			addload = resinfo.ResourceStep;
		} else {
			addload = resinfo.ResourceCapacity;
		}
		// Make sure we don't bite more than we can chew.
		if (unit.ResourcesHeld + addload > resinfo.ResourceCapacity) {
			addload = resinfo.ResourceCapacity - unit.ResourcesHeld;
		}

		if (resinfo.TerrainHarvester) {
			unit.ResourcesHeld += addload;

			if (addload && unit.ResourcesHeld == resinfo.ResourceCapacity) {
				Map.ClearTile(MapFieldForest, this->goalPos);
			}
		} else {
			if (resinfo.HarvestFromOutside) {
				source = this->GetGoal();
			} else {
				source = unit.Container;
			}

			Assert(source);
			Assert(source->ResourcesHeld <= 655350);
			bool is_visible = source->IsVisibleAsGoal(*unit.Player);
			// Target is not dead, getting resources.
			if (is_visible) {
				// Don't load more that there is.
				if (addload > source->ResourcesHeld) {
					addload = source->ResourcesHeld;
				}
				unit.ResourcesHeld += addload;
				source->ResourcesHeld -= addload;
			}

			// End of resource: destroy the resource.
			// FIXME: implement depleted resources.
			if ((!is_visible) || (source->ResourcesHeld == 0)) {
				if (unit.Anim.Unbreakable) {
					return 0;
				}
				DebugPrint("%d: Worker %d report: Resource is destroyed\n" _C_ unit.Player->Index _C_ unit.Slot);
				bool dead = source->CurrentAction() == UnitActionDie;

				// Improved version of DropOutAll that makes workers go to the depot.
				LoseResource(unit, *source);
				int i;
				CUnit *uins;
				for (i = source->InsideCount, uins = source->UnitInside;
										i; --i, uins = uins->NextContained) {
					if (uins->CurrentOrder()->Action == UnitActionResource) {
						COrder_Resource &order = *static_cast<COrder_Resource*>(uins->CurrentOrder());

						order.LoseResource(*uins, *source);
					}
				}
				// Don't destroy the resource twice.
				// This only happens when it's empty.
				if (!dead) {
					LetUnitDie(*source);
					// FIXME: make the workers inside look for a new resource.
				}
				source = NULL;
				return 0;
			}
		}
		if (resinfo.TerrainHarvester) {
			if (unit.ResourcesHeld == resinfo.ResourceCapacity) {
				// Mark as complete.
				this->DoneHarvesting = true;
			}
			return 0;
		} else {
			if (resinfo.HarvestFromOutside) {
				if ((unit.ResourcesHeld == resinfo.ResourceCapacity) || (source == NULL)) {
					// Mark as complete.
					this->DoneHarvesting = true;
				}
				return 0;
			} else {
				return unit.ResourcesHeld == resinfo.ResourceCapacity && source;
			}
		}
	}
	return 0;
}

int GetNumWaitingWorkers(const CUnit &mine)
{
	int ret = 0;
	CUnit *worker = mine.Resource.Workers;

	for (int i = 0; NULL != worker; worker = worker->NextWorker, ++i)
	{
		Assert(worker->CurrentAction() == UnitActionResource);
		COrder_Resource &order = *static_cast<COrder_Resource*>(worker->CurrentOrder());

		if (order.IsGatheringWaiting()) {
			ret++;
		}
		Assert(i <= mine.Resource.Assigned);
	}
	return ret;
}

/**
**  Stop gathering from the resource, go home.
**
**  @param unit  Poiner to unit.
**
**  @return      TRUE if ready, otherwise FALSE.
*/
int COrder_Resource::StopGathering(CUnit &unit)
{
	CUnit *source = 0;
	const ResourceInfo &resinfo = *unit.Type->ResInfo[unit.CurrentResource];

	if (!resinfo.TerrainHarvester) {
		if (resinfo.HarvestFromOutside) {
			source = this->GetGoal();
			this->ClearGoal();
		} else {
			source = unit.Container;
		}
		source->Resource.Active--;
		Assert(source->Resource.Active >= 0);
		//Store resource position.
		this->Resource.Mine = source;

		if (source->Type->MaxOnBoard) {
			int count = 0;
			CUnit *worker = source->Resource.Workers;
			CUnit *next = NULL;
			for(; NULL != worker; worker = worker->NextWorker)
			{
				Assert(worker->CurrentAction() == UnitActionResource);
				COrder_Resource& order = *static_cast<COrder_Resource*>(worker->CurrentOrder());
				if (worker != &unit && order.IsGatheringWaiting()) {
					count++;
					if (next) {
						if (next->Wait > worker->Wait) {
							next = worker;
						}
					} else {
						next = worker;
					}
				}
			}
			if (next) {
				if (!unit.Player->AiEnabled) {
					DebugPrint("%d: Worker %d report: Unfreez resource gathering of %d <Wait %d> on %d [Assigned: %d Waiting %d].\n"
						_C_ unit.Player->Index _C_ unit.Slot
						_C_ next->Slot _C_ next->Wait
						_C_ source->Slot _C_ source->Resource.Assigned
						_C_ count);
				}
				next->Wait = 0;
				//source->Data.Resource.Waiting = count - 1;
				//Assert(source->Data.Resource.Assigned >= source->Data.Resource.Waiting);
				//StartGathering(next);
			}
		}
	} else {
		// Store resource position.
		this->Resource.Pos = unit.tilePos;
		Assert(this->Resource.Mine == NULL);
	}

#ifdef DEBUG
	if (!unit.ResourcesHeld) {
		DebugPrint("Unit %d is empty???\n" _C_ unit.Slot);
	}
#endif

	// Find and send to resource deposit.
	CUnit *depot = FindDeposit(unit, 1000, unit.CurrentResource);
	if (!depot || !unit.ResourcesHeld) {
		if (!(resinfo.HarvestFromOutside || resinfo.TerrainHarvester)) {
			Assert(unit.Container);
			DropOutOnSide(unit, LookingW, source);
		}
		CUnit *mine = this->Resource.Mine;

		if (mine) {
			unit.DeAssignWorkerFromMine(*mine);
			this->Resource.Mine = NULL;
		}

		DebugPrint("%d: Worker %d report: Can't find a resource [%d] deposit.\n"
				_C_ unit.Player->Index _C_ unit.Slot _C_ unit.CurrentResource);
		this->Finished = true;
		return 0;
	} else {
		if (!(resinfo.HarvestFromOutside || resinfo.TerrainHarvester)) {
			Assert(unit.Container);
			DropOutNearest(unit, depot->tilePos + depot->Type->GetHalfTileSize(), source);
		}
		UnitGotoGoal(unit, depot, SUB_MOVE_TO_DEPOT);
	}
	if (IsOnlySelected(unit)) {
		SelectedUnitChanged();
	}
#if 1
	return 1;
#endif
}

extern void AiNewDepotRequest(CUnit &worker);

/**
**  Move to resource depot
**
**  @param unit  Pointer to unit.
**
**  @return      TRUE if reached, otherwise FALSE.
*/
int COrder_Resource::MoveToDepot(CUnit &unit)
{
	const ResourceInfo &resinfo = *unit.Type->ResInfo[unit.CurrentResource];
	CUnit &goal = *this->GetGoal();
	CPlayer &player = *unit.Player;
	Assert(&goal);

	switch (DoActionMove(unit)) { // reached end-point?
		case PF_UNREACHABLE:
			return -1;
		case PF_REACHED:
			break;
		default:
			if (unit.Anim.Unbreakable || goal.IsVisibleAsGoal(player)) {
				return 0;
			}
			break;
	}

	//
	// Target is dead, stop getting resources.
	//
	if (!goal.IsVisibleAsGoal(player)) {
		DebugPrint("%d: Worker %d report: Destroyed depot\n" _C_ player.Index _C_ unit.Slot);

		unit.CurrentOrder()->ClearGoal();

		CUnit *depot = FindDeposit(unit, 1000, unit.CurrentResource);

		if (depot) {
			UnitGotoGoal(unit, depot, SUB_MOVE_TO_DEPOT);
			DebugPrint("%d: Worker %d report: Going to new deposit.\n" _C_ player.Index _C_ unit.Slot);
		} else {
			DebugPrint("%d: Worker %d report: Can't find a new resource deposit.\n"
				_C_ player.Index _C_ unit.Slot);

			// FIXME: perhaps we should choose an alternative
			this->Finished = true;
		}
		return 0;
	}

	// Not ready
	if (player.AiEnabled && unit.pathFinderData->output.Cycles > 300) {
		AiNewDepotRequest(unit);
	}

	// If resource depot is still under construction, wait!
	if (goal.CurrentAction() == UnitActionBuilt) {
		unit.Wait = 10;
		return 0;
	}

	this->ClearGoal();
	unit.Wait = resinfo.WaitAtDepot;

	// Place unit inside the depot
	if (unit.Wait) {
		unit.Remove(&goal);
		unit.Anim.CurrAnim = NULL;
	}

	// Update resource.
	const int rindex = resinfo.FinalResource;
	player.Resources[rindex] += (unit.ResourcesHeld * player.Incomes[rindex]) / 100;
	if (player.MaxResources[rindex] != -1) {
		player.Resources[rindex] = std::min(player.Resources[rindex], player.MaxResources[rindex]);
	}
	player.TotalResources[rindex] += (unit.ResourcesHeld * player.Incomes[rindex]) / 100;
	unit.ResourcesHeld = 0; 

	if (unit.Wait) {
		unit.Wait /= SpeedResourcesReturn[resinfo.ResourceId];
		if (unit.Wait) {
			unit.Wait--;
		}
	}
	return 1;
}

/**
**  Wait in depot, for the resources stored.
**
**  @param unit  Pointer to unit.
**
**  @return      TRUE if ready, otherwise FALSE.
*/
bool COrder_Resource::WaitInDepot(CUnit &unit)
{
	const ResourceInfo &resinfo = *unit.Type->ResInfo[unit.CurrentResource];
	const CUnit *depot = ResourceDepositOnMap(unit.tilePos, resinfo.ResourceId);

	//Assert(depot);

	// Range hardcoded. don't stray too far though
	if (resinfo.TerrainHarvester) {
		Vec2i pos = this->Resource.Pos;

		if (FindTerrainType(unit.Type->MovementMask, MapFieldForest, 0, 10, *unit.Player, pos, &pos)) {
			if (depot) {
				DropOutNearest(unit, pos, depot);
			}
			this->goalPos = pos;
		} else {
			if (depot) {
				DropOutOnSide(unit, LookingW, depot);
			}
			this->Finished = true;
			return false;
		}
	} else {
		CUnit *mine = this->Resource.Mine;
		const int range = (mine ? 15 : 1000);
		const Vec2i pos = mine ? mine->tilePos : unit.tilePos;
		CUnit *goal = UnitFindResource(unit, pos, range, unit.CurrentResource, unit.Player->AiEnabled, depot);

		if (goal) {
			if (depot) {
				DropOutNearest(unit, goal->tilePos + goal->Type->GetHalfTileSize(), depot);
			}

			if (goal != mine) {
				if (mine) {
					unit.DeAssignWorkerFromMine(*mine);
				}
				unit.AssignWorkerToMine(*goal);
				this->Resource.Mine = goal;
			}
			this->SetGoal(goal);
			this->Range = 1;
			this->goalPos.x = this->goalPos.y = -1;
		} else {
			DebugPrint("%d: Worker %d report: [%d,%d] Resource gone near [%d,%d] in range %d. Sit and play dumb.\n"
				_C_ unit.Player->Index _C_ unit.Slot
				_C_ unit.tilePos.x _C_ unit.tilePos.y
				_C_ pos.x _C_ pos.y _C_ range);
			if (depot) {
				DropOutOnSide(unit, LookingW, depot);
			}
			if (mine) {
				unit.DeAssignWorkerFromMine(*mine);
				this->Resource.Mine = NULL;
			}
			this->Finished = true;
			return false;
		}
	}
	return true;
}

void COrder_Resource::DropResource(CUnit &unit)
{
	if (unit.CurrentResource) {
		const ResourceInfo &resinfo = *unit.Type->ResInfo[unit.CurrentResource];
		if (resinfo.LoseResources && unit.ResourcesHeld < resinfo.ResourceCapacity) {
			unit.ResourcesHeld = 0;
		} else {
			//FIXME: Add support for droping resource on map
			unit.ResourcesHeld = 0;
		}
		if (!resinfo.TerrainHarvester) {
			CUnit *mine = this->Resource.Mine;
			if (mine) {
				unit.DeAssignWorkerFromMine(*mine);
			}
		}
		//fast clean both resource data: pos and mine
		this->Resource.Mine = NULL;
		unit.CurrentResource = 0;
	}
}

/**
**  Give up on gathering.
**
**  @param unit  Pointer to unit.
*/
void COrder_Resource::ResourceGiveUp(CUnit &unit)
{
	DebugPrint("%d: Worker %d report: Gave up on resource gathering.\n" _C_ unit.Player->Index _C_ unit.Slot);
	this->ClearGoal();
	DropResource(unit);
	this->Finished = true;
}

/**
**  Initialize
**
**  return false if action is canceled, true otherwise.
*/
bool COrder_Resource::ActionResourceInit(CUnit &unit)
{
	Assert(this->State == SUB_START_RESOURCE);

	CUnit *const goal = this->GetGoal();
	int newres;

	if (goal) {
		newres = goal->Type->GivesResource;
	} else {
		//FIXME: hardcoded wood
		newres = WoodCost;
	}
	if (newres != unit.CurrentResource) {
		DropResource(unit);
	}
	unit.CurrentResource = newres;
	if (newres == 0) {
		unit.ResourcesHeld = 0;
		ResourceGiveUp(unit);
		return false;
	}
	this->CurrentResource = newres;
	if (goal && this->Resource.Mine != goal) {
		CUnit *mine = this->Resource.Mine;

		if (mine) {
			unit.DeAssignWorkerFromMine(*mine);
			this->Resource.Mine = NULL;
		}
		if (goal->CurrentAction() != UnitActionBuilt) {
			unit.AssignWorkerToMine(*goal);
			this->Resource.Mine = goal;
		}
	}
	UnitGotoGoal(unit, goal, SUB_MOVE_TO_RESOURCE);
	return true;
}

/**
**  Control the unit action: getting a resource.
**
**  This the generic function for oil, gold, ...
**
**  @param unit  Pointer to unit.
*/
void COrder_Resource::Execute(CUnit &unit)
{
	if (unit.Wait) {
		// FIXME: show idle animation while we wait?
		unit.Wait--;
		return;
	}

	// Let's start mining.
	if (this->State == SUB_START_RESOURCE) {
		if (ActionResourceInit(unit) == false) {
			return;
		}
	}

	// Move to the resource location.
	if (SUB_MOVE_TO_RESOURCE <= this->State && this->State < SUB_UNREACHABLE_RESOURCE) {
		const int ret = MoveToResource(unit);

		switch (ret) {
			case -1: { // Can't Reach
				this->State++;
				unit.Wait = 5;
				return;
			}
			case 1: { // Reached
				this->State = SUB_START_GATHERING;
				break;
			}
			case 0: // Move along.
				return;
			default: {
				Assert(0);
				break;
			}
		}
	}

	// Resource seems to be unreachable
	if (this->State == SUB_UNREACHABLE_RESOURCE) {
		ResourceGiveUp(unit);
		return;
	}

	// Start gathering the resource
	if (this->State == SUB_START_GATHERING) {
		if (StartGathering(unit)) {
			this->State = SUB_GATHER_RESOURCE;
		} else {
			return;
		}
	}

	// Gather the resource.
	if (this->State == SUB_GATHER_RESOURCE) {
		if (GatherResource(unit)) {
			this->State = SUB_STOP_GATHERING;
		} else {
			return;
		}
	}

	// Stop gathering the resource.
	if (this->State == SUB_STOP_GATHERING) {
		if (StopGathering(unit)) {
			this->State = SUB_MOVE_TO_DEPOT;
			unit.pathFinderData->output.Cycles = 0; //moving counter
		} else
			return;
	}

	// Move back home.
	if (SUB_MOVE_TO_DEPOT <= this->State && this->State < SUB_UNREACHABLE_DEPOT) {
		const int ret = MoveToDepot(unit);

		switch (ret) {
			case -1: { // Can't Reach
				this->State++;
				unit.Wait = 5;
				return;
			}
			case 1: { // Reached
				this->State = SUB_RETURN_RESOURCE;
				return;
			}
			case 0: // Move along.
				return;
			default: {
				Assert(0);
				return;
			}
		}
	}

	// Depot seems to be unreachable
	if (this->State == SUB_UNREACHABLE_DEPOT) {
		ResourceGiveUp(unit);
		return;
	}

	// Unload resources at the depot.
	if (this->State == SUB_RETURN_RESOURCE) {
		if (WaitInDepot(unit)) {
			this->State = SUB_START_RESOURCE;

			// It's posible, though very rare that the unit's goal blows up
			// this cycle, but after this unit. Thus, next frame the unit
			// will start mining a destroyed site. If, on the otherhand we
			// are already in SUB_MOVE_TO_RESOURCE then we can handle it.
			// So, we pass through SUB_START_RESOURCE the very instant it
			// goes out of the depot.
//			HandleActionResource(order, unit);
		}
	}
}

//@}
