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
/**@name ai.cpp - The computer player AI main file. */
//
//      (c) Copyright 2000-2005 by Lutz Sammer and Ludovic Pollet
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

//----------------------------------------------------------------------------
// Documentation
//----------------------------------------------------------------------------

/**
** @page AiModule Module - AI
**
** @section aibasics What is it?
**
** Stratagus uses a very simple scripted AI. There are no optimizations
** yet. The complete AI was written on one weekend.
** Until no AI specialist joins, I keep this AI.
**
** @subsection aiscripted What is scripted AI?
**
** The AI script tells the engine build 4 workers, than build 3 footman,
** than attack the player, than sleep 100 frames.
**
** @section API The AI API
**
** @subsection aimanage Management calls
**
** Manage the inititialse and cleanup of the AI players.
**
** ::InitAiModule(void)
**
** Initialise all global varaibles and structures.
** Called before AiInit, or before game loading.
**
** ::AiInit(::Player)
**
** Called for each player, to setup the AI structures
** Player::Aiin the player structure. It can use Player::AiName to
** select different AI's.
**
** ::CleanAi(void)
**
** Called to release all the memory for all AI structures.
** Must handle self which players contains AI structures.
**
** ::SaveAi(::FILE*)
**
** Save the AI structures of all players to file.
** Must handle self which players contains AI structures.
**
**
** @subsection aipcall Periodic calls
**
** This functions are called regular for all AI players.
**
** ::AiEachCycle(::Player)
**
** Called each game cycle, to handle quick checks, which needs
** less CPU.
**
** ::AiEachSecond(::Player)
**
** Called each second, to handle more CPU intensive things.
**
**
** @subsection aiecall Event call-backs
**
** This functions are called, when some special events happens.
**
** ::AiHelpMe()
**
** Called if an unit owned by the AI is attacked.
**
** ::AiUnitKilled()
**
** Called if an unit owned by the AI is killed.
**
** ::AiNeedMoreSupply()
**
** Called if an trained unit is ready, but not enough food is
** available for it.
**
** ::AiWorkComplete()
**
** Called if an unit has completed its work.
**
** ::AiCanNotBuild()
**
** Called if the AI unit can't build the requested unit-type.
**
** ::AiCanNotReach()
**
** Called if the AI unit can't reach the building place.
**
** ::AiTrainingComplete()
**
** Called if AI unit has completed training a new unit.
**
** ::AiUpgradeToComplete()
**
** Called if AI unit has completed upgrade to new unit-type.
**
** ::AiResearchComplete()
**
** Called if AI unit has completed research of an upgrade or spell.
*/

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "stratagus.h"

#include "player.h"
#include "unit.h"
#include "unittype.h"
#include "upgrade.h"
#include "script.h"
#include "actions.h"
#include "map.h"
#include "pathfinder.h"
#include "ai_local.h"

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

int AiSleepCycles;  /// Ai sleeps # cycles

CAiType *AiTypes;   /// List of all AI types.
AiHelper AiHelpers; /// AI helper variables

PlayerAi *AiPlayer; /// Current AI player

/*----------------------------------------------------------------------------
-- Lowlevel functions
----------------------------------------------------------------------------*/

/**
**  Execute the AI Script.
*/
static void AiExecuteScript(void)
{
	PlayerAi *pai;

	pai = AiPlayer;
	if (pai->Script) {
		lua_pushstring(Lua, "_ai_scripts_");
		lua_gettable(Lua, LUA_GLOBALSINDEX);
		lua_pushstring(Lua, pai->Script);
		lua_rawget(Lua, 1);
		LuaCall(0, 1);
		lua_pop(Lua, 1);
	}
}

/**
**  Check if everything is fine, send new requests to resource manager.
*/
static void AiCheckUnits(void)
{
	int counter[UnitTypeMax];
	int attacking[UnitTypeMax];
	const AiBuildQueue *queue;
	const int *unit_types_count;
	int i;
	int j;
	int n;
	int t;
	int x;
	int e;

	memset(counter, 0, sizeof (counter));
	memset(attacking,0,sizeof(attacking));

	//
	//  Count the already made build requests.
	//
	for (queue = AiPlayer->UnitTypeBuilt; queue; queue = queue->Next) {
		counter[queue->Type->Slot] += queue->Want;
	}

	//
	//  Remove non active units.
	//
	n = AiPlayer->Player->TotalNumUnits;
	for (i = 0; i < n; ++i) {
		if (!AiPlayer->Player->Units[i]->Active) {
			counter[AiPlayer->Player->Units[i]->Type->Slot]--;
		}
	}
	unit_types_count = AiPlayer->Player->UnitTypesCount;

	//
	//  Look if some unit-types are missing.
	//
	n = AiPlayer->UnitTypeRequests.size();
	for (i = 0; i < n; ++i) {
		t = AiPlayer->UnitTypeRequests[i].Type->Slot;
		x = AiPlayer->UnitTypeRequests[i].Count;

		//
		// Add equivalent units
		//
		e = unit_types_count[t];
		if (t < (int)AiHelpers.Equiv.size()) {
			for (j = 0; j < (int)AiHelpers.Equiv[t].size(); ++j) {
				e += unit_types_count[AiHelpers.Equiv[t][j]->Slot];
			}
		}

		if (x > e + counter[t]) {  // Request it.
			AiAddUnitTypeRequest(AiPlayer->UnitTypeRequests[i].Type,
				x - e - counter[t]);
			counter[t] += x - e - counter[t];
		}
		counter[t] -= x;
	}

	//
	// Look through the forces what is missing.
	//
	for (i = AI_MAX_FORCES; i < AI_MAX_ATTACKING_FORCES; ++i) {
		const AiUnit *unit;

		for (unit = AiPlayer->Force[i].Units; unit; unit = unit->Next) {
			attacking[unit->Unit->Type->Slot]++;
		}
	}

	//
	// create missing units
	//
	for (i = 0; i < AI_MAX_FORCES; ++i) {
		const AiUnitType *aiut;

		// No troops for attacking force
		if (!AiPlayer->Force[i].Defending &&
				AiPlayer->Force[i].Attacking) {
			continue;
		}

		for (aiut = AiPlayer->Force[i].UnitTypes; aiut; aiut = aiut->Next) {
			t = aiut->Type->Slot;
			x = aiut->Want;
			if (x > unit_types_count[t] + counter[t] - attacking[t]) {    // Request it.
				AiAddUnitTypeRequest(aiut->Type,
					x - (unit_types_count[t] + counter[t] - attacking[t]));
				counter[t] += x - (unit_types_count[t] + counter[t] - attacking[t]);
				AiPlayer->Force[i].Completed=0;
			}
			counter[t] -= x;
		}
	}

	//
	//  Look if some upgrade-to are missing.
	//
	n = AiPlayer->UpgradeToRequests.size();
	for (i = 0; i < n; ++i) {
		t = AiPlayer->UpgradeToRequests[i]->Slot;
		x = 1;

		//
		//  Add equivalent units
		//
		e = unit_types_count[t];
		if (t < (int)AiHelpers.Equiv.size()) {
			for (j = 0; j < (int)AiHelpers.Equiv[t].size(); ++j) {
				e += unit_types_count[AiHelpers.Equiv[t][j]->Slot];
			}
		}

		if (x > e + counter[t]) {  // Request it.
			AiAddUpgradeToRequest(AiPlayer->UpgradeToRequests[i]);
			counter[t] += x - e - counter[t];
		}
		counter[t] -= x;
	}

	//
	//  Look if some researches are missing.
	//
	n = (int)AiPlayer->ResearchRequests.size();
	for (i = 0; i < n; ++i) {
		if (UpgradeIdAllowed(AiPlayer->Player,
				AiPlayer->ResearchRequests[i]->ID) == 'A') {
			AiAddResearchRequest(AiPlayer->ResearchRequests[i]);
		}
	}
}

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

#if 0
/**
**  Save AI helper sub table.
**
**  @param file     Output file.
**  @param name     Table action name.
**  @param upgrade  True if is an upgrade.
**  @param n        Number of elements in table
**  @param table    unit-type table.
*/
static void SaveAiHelperTable(CFile *file, const char *name, int upgrade, int n,
	AiUnitTypeTable *const *table)
{
	int t;
	int i;
	int j;
	int f;
	int max;

	max = (upgrade ? UpgradeMax : NumUnitTypes);
	for (t = 0; t < max; ++t) {
		// Look if that unit-type can build something
		for (f = i = 0; i < n; ++i) {
			if (table[i]) {
				for (j = 0; j < table[i]->Count; ++j) {
					if (table[i]->Table[j]->Slot == t) {
						if (!f) {
							file->printf("\n  {\"%s\", \"%s\"\n\t", name,
								UnitTypes[t]->Ident);
							f = 4;
						}
						if (upgrade) {
							if (f + strlen(Upgrades[i].Ident) > 78) {
								f = file->printf("\n\t");
							}
							f += file->printf(", \"%s\"", Upgrades[i].Ident);
						} else {
							if (f + strlen(UnitTypes[i]->Ident) > 78) {
								f = file->printf("\n\t");
							}
							f += file->printf(", \"%s\"", UnitTypes[i]->Ident);
						}
					}
				}
			}
		}
		if (f) {
			file->printf("},");
		}
	}
}
#endif

#if 0
/**
**  Save AI helper sub table.
**
**  @param file   Output file.
**  @param name   Table action name.
**  @param n      Number of elements in table
**  @param table  unit-type table.
*/
static void SaveAiEquivTable(CFile *file, const char *name, int n,
	AiUnitTypeTable *const *table)
{
	int i;
	int j;
	int f;

	for (i = 0; i < n; ++i) {
		if (table[i]) {
			file->printf("\n  {\"%s\", \"%s\"\n\t", name, UnitTypes[i]->Ident);
			f = 4;
			for (j = 0; j < table[i]->Count; ++j) {
				if (f + strlen(table[i]->Table[j]->Ident) > 78) {
					f = file->printf("\n\t");
				}
				f += file->printf(", \"%s\"", table[i]->Table[j]->Ident);
			}
			if (i == n - 1) {
				file->printf("}");
			} else {
				file->printf("},");
			}
		}
	}
}
#endif

#if 0 // Not used. Same as SaveAiUnitLimitTable with Defautresource instead of food

/**
**  Save AI helper sub table.
**
**  @param file   Output file.
**  @param name   Table action name.
**  @param n      Number of elements in table
**  @param table  unit-type table.
*/
static void SaveAiCostTable(CFile *file, const char *name, int n,
	AiUnitTypeTable *const *table)
{
	int t;
	int i;
	int j;
	int f;

	for (t = 0; t < NumUnitTypes; ++t) {
		// Look if that unit-type can build something
		for (f = i = 0; i < n; ++i) {
			if (table[i]) {
				for (j = 0; j < table[i]->Count; ++j) {
					if (table[i]->Table[j]->Slot == t) {
						if (!f) {
							file->printf("\n  (list '%s '%s\n\t", name,
								UnitTypes[t]->Ident);
							f = 4;
						}
						if (f + strlen(DefaultResourceNames[i]) > 78) {
							f = file->printf("\n\t");
						}
						f += file->printf("'%s ", DefaultResourceNames[i]);
					}
				}
			}
		}
		if (f) {
			file->printf(")");
		}
	}
}

#endif

#if 0
/**
**  Save AI helper sub table.
**
**  @param file   Output file.
**  @param name   Table action name.
**  @param n      Number of elements in table
**  @param table  unit-type table.
*/
static void SaveAiUnitLimitTable(CFile *file, const char *name, int n,
	AiUnitTypeTable *const *table)
{
	int t;
	int i;
	int j;
	int f;

	for (t = 0; t < NumUnitTypes; ++t) {
		// Look if that unit-type can build something
		for (f = i = 0; i < n; ++i) {
			if (table[i]) {
				for (j = 0; j < table[i]->Count; ++j) {
					if (table[i]->Table[j]->Slot == t) {
						if (!f) {
							file->printf("\n  {\"%s\", \"%s\"\n\t", name,
								UnitTypes[t]->Ident);
							f = 4;
						}
						if (f + strlen("food") > 78) {
							f = file->printf("\n\t");
						}
						f += file->printf(",\"%s\" ", "food");
					}
				}
			}
		}
		if (f) {
			file->printf("},");
		}
	}
}
#endif

#if 0
/**
**  Save AI helper table.
**
**  @param file  Output file.
**  @todo manage correctly ","
*/
static void SaveAiHelper(CFile *file)
{
	file->printf("DefineAiHelper(");
	//
	//  Save build table
	//
	SaveAiHelperTable(file, "build", 0, AiHelpers.BuildCount, AiHelpers.Build);

	//
	//  Save train table
	//
	SaveAiHelperTable(file, "train", 0, AiHelpers.TrainCount, AiHelpers.Train);

	//
	//  Save upgrade table
	//
	SaveAiHelperTable(file, "upgrade", 0, AiHelpers.UpgradeCount, AiHelpers.Upgrade);

	//
	//  Save research table
	//
	SaveAiHelperTable(file, "research", 1, AiHelpers.ResearchCount, AiHelpers.Research);

	//
	//  Save repair table
	//
	SaveAiHelperTable(file, "repair", 0, AiHelpers.RepairCount, AiHelpers.Repair);

	//
	//  Save limits table
	//
	SaveAiUnitLimitTable(file, "unit-limit", AiHelpers.UnitLimitCount,
		AiHelpers.UnitLimit);

	//
	//  Save equivalence table
	//
	SaveAiEquivTable(file, "unit-equiv", AiHelpers.EquivCount, AiHelpers.Equiv);

	file->printf(" )\n\n");
}
#endif

/**
**  Save the AI type. (recursive)
**
**  @param file    Output file.
**  @param aitype  AI type to save.
*/
static void SaveAiType(CFile *file, const CAiType *aitype)
{
	return;
	if (aitype->Next) {
		SaveAiType(file, aitype->Next);
	}
	file->printf("DefineAi(\"%s\", \"%s\", \"%s\", %s)\n\n",
		aitype->Name, aitype->Race ? aitype->Race : "*",
		aitype->Class, aitype->FunctionName);
}

#if 0
/**
**  Save the AI types.
**
**  @param file  Output file.
*/
static void SaveAiTypes(CFile *file)
{
	SaveAiType(file, AiTypes);

	// FIXME: Must save references to other scripts - scheme functions
	// Perhaps we should dump the complete scheme state
}
#endif

/**
**  Save state of player AI.
**
**  @param file   Output file.
**  @param plynr  Player number.
**  @param ai     Player AI.
*/
static void SaveAiPlayer(CFile *file, int plynr, PlayerAi *ai)
{
	int i;
	const AiBuildQueue *queue;

	file->printf("DefineAiPlayer(%d,\n", plynr);
	file->printf("  \"ai-type\", \"%s\",\n", ai->AiType->Name);

	file->printf("  \"script\", \"%s\",\n", ai->Script);
	file->printf("  \"script-debug\", %s,\n", ai->ScriptDebug ? "true" : "false");
	file->printf("  \"sleep-cycles\", %lu,\n", ai->SleepCycles);

	//
	//  All forces
	//
	for (i = 0; i < AI_MAX_ATTACKING_FORCES; ++i) {
		const AiUnitType* aut;
		const AiUnit* aiunit;

		file->printf("  \"force\", {%d, %s%s%s", i,
			ai->Force[i].Completed ? "\"complete\"," : "\"recruit\",",
			ai->Force[i].Attacking ? " \"attack\"," : "",
			ai->Force[i].Defending ? " \"defend\"," : "");

		file->printf(" \"role\", ");
		switch (ai->Force[i].Role) {
			case AiForceRoleAttack:
				file->printf("\"attack\",");
				break;
			case AiForceRoleDefend:
				file->printf("\"defend\",");
				break;
			default:
				file->printf("\"unknown-%d\",", ai->Force[i].Role);
				break;
		}

		file->printf("\n    \"types\", { ");
		for (aut = ai->Force[i].UnitTypes; aut; aut = aut->Next) {
			file->printf("%d, \"%s\", ", aut->Want, aut->Type->Ident);
		}
		file->printf("},\n    \"units\", {");
		for (aiunit = ai->Force[i].Units; aiunit; aiunit = aiunit->Next) {
			file->printf(" %d, \"%s\",", UnitNumber(aiunit->Unit),
				aiunit->Unit->Type->Ident);
		}
		file->printf("},\n    \"state\", %d, \"goalx\", %d, \"goaly\", %d, \"must-transport\", %d,",
			ai->Force[i].State, ai->Force[i].GoalX, ai->Force[i].GoalY, ai->Force[i].MustTransport);
		file->printf("},\n");
	}

	file->printf("  \"reserve\", {");
	for (i = 0; i < MaxCosts; ++i) {
		file->printf("\"%s\", %d, ", DefaultResourceNames[i], ai->Reserve[i]);
	}
	file->printf("},\n");

	file->printf("  \"used\", {");
	for (i = 0; i < MaxCosts; ++i) {
		file->printf("\"%s\", %d, ", DefaultResourceNames[i], ai->Used[i]);
	}
	file->printf("},\n");

	file->printf("  \"needed\", {");
	for (i = 0; i < MaxCosts; ++i) {
		file->printf("\"%s\", %d, ", DefaultResourceNames[i], ai->Needed[i]);
	}
	file->printf("},\n");

	file->printf("  \"collect\", {");
	for (i = 0; i < MaxCosts; ++i) {
		file->printf("\"%s\", %d, ", DefaultResourceNames[i], ai->Collect[i]);
	}
	file->printf("},\n");

	file->printf("  \"need-mask\", {");
	for (i = 0; i < MaxCosts; ++i) {
		if (ai->NeededMask & (1 << i)) {
			file->printf("\"%s\", ", DefaultResourceNames[i]);
		}
	}
	file->printf("},\n");
	if (ai->NeedSupply) {
		file->printf("  \"need-supply\",\n");
	}

	//
	//  Requests
	//
	if (ai->FirstExplorationRequest) {
		AiExplorationRequest* ptr;

		file->printf("  \"exploration\", {");
		ptr = ai->FirstExplorationRequest;
		while (ptr) {
			file->printf("{%d, %d, %d}, ",
				ptr->X, ptr->Y, ptr->Mask);
			ptr = ptr->Next;
		}
		file->printf("},\n");
	}
	file->printf("  \"last-exploration-cycle\", %lu,\n", ai->LastExplorationGameCycle);
	if (ai->TransportRequests) {
		AiTransportRequest *ptr;

		file->printf("  \"transport\", {");
		ptr = ai->TransportRequests;
		while (ptr) {
			file->printf("{%d, ", UnitNumber(ptr->Unit));
			SaveOrder(&ptr->Order, file);
			file->printf("}, ");
			ptr = ptr->Next;
		}
		file->printf("},\n");
	}
	file->printf("  \"last-can-not-move-cycle\", %lu,\n", ai->LastCanNotMoveGameCycle);
	file->printf("  \"unit-type\", {");
	for (i = 0; i < (int)ai->UnitTypeRequests.size(); ++i) {
		file->printf("\"%s\", ", ai->UnitTypeRequests[i].Type->Ident);
		file->printf("%d, ", ai->UnitTypeRequests[i].Count);
	}
	file->printf("},\n");

	file->printf("  \"upgrade\", {");
	for (i = 0; i < (int)ai->UpgradeToRequests.size(); ++i) {
		file->printf("\"%s\", ", ai->UpgradeToRequests[i]->Ident);
	}
	file->printf("},\n");

	file->printf("  \"research\", {");
	for (i = 0; i < (int)ai->ResearchRequests.size(); ++i) {
		file->printf("\"%s\", ", ai->ResearchRequests[i]->Ident);
	}
	file->printf("},\n");

	//
	//  Building queue
	//
	file->printf("  \"building\", {");
	for (queue = ai->UnitTypeBuilt; queue; queue = queue->Next) {
		file->printf("\"%s\", %d, %d, ", queue->Type->Ident, queue->Made, queue->Want);
	}
	file->printf("},\n");

	file->printf("  \"repair-building\", %u,\n", ai->LastRepairBuilding);

	file->printf("  \"repair-workers\", {");
	for (i = 0; i < UnitMax; ++i) {
		if (ai->TriedRepairWorkers[i]) {
			file->printf("%d, %d, ", i, ai->TriedRepairWorkers[i]);
		}
	}
	file->printf("})\n\n");
}

/**
**  Save state of player AIs.
**
**  @param file  Output file.
*/
static void SaveAiPlayers(CFile *file)
{
	for (int p = 0; p < PlayerMax; ++p) {
		if (Players[p].Ai) {
			SaveAiPlayer(file, p, Players[p].Ai);
		}
	}
}

/**
**  Save state of AI to file.
**
**  @param file  Output file.
*/
void SaveAi(CFile *file)
{
	file->printf("\n--- -----------------------------------------\n");
	file->printf("--- MODULE: AI $Id$\n\n");

#if 0
	SaveAiHelper(file);
	SaveAiTypes(file);
#endif
	SaveAiPlayers(file);

	DebugPrint("FIXME: Saving lua function definition isn't supported\n");
}

/**
**  Setup all at start.
**
**  @param player  The player structure pointer.
*/
void AiInit(CPlayer *player)
{
	PlayerAi *pai;
	CAiType *ait;
	char *ainame;

	pai = new PlayerAi;
	if (!pai) {
		fprintf(stderr, "Out of memory.\n");
		exit(0);
	}
	// FIXME: use constructor
	memset(pai, 0, sizeof(*pai));

	pai->Player = player;
	ait = AiTypes;

	ainame = player->AiName;
	DebugPrint("%d - %p - looking for class %s\n" _C_
		player->Index _C_ player _C_ ainame);
	//MAPTODO print the player name (player->Name) instead of the pointer

	//
	//  Search correct AI type.
	//
	if (!ait) {
		DebugPrint("AI: Got no scripts at all! You need at least one dummy fallback script.\n");
		DebugPrint("AI: Look at the DefineAi() documentation.\n");
		Exit(0);
	}
	for (;;) {
		if (ait->Race && strcmp(ait->Race, PlayerRaces.Name[player->Race])) {
			ait = ait->Next;
			if (!ait && ainame) {
				ainame = NULL;
				ait = AiTypes;
			}
			if (!ait) {
				break;
			}
			continue;
		}
		if (ainame && strcmp(ainame, ait->Class)) {
			ait = ait->Next;
			if (!ait && ainame) {
				ainame = NULL;
				ait = AiTypes;
			}
			if (!ait) {
				break;
			}
			continue;
		}
		break;
	}
	if (!ait) {
		DebugPrint("AI: Found no matching ai scripts at all!\n");
		exit(0);
	}
	if (!ainame) {
		DebugPrint("AI: not found!!!!!!!!!!\n");
		DebugPrint("AI: Using fallback:\n");
	}
	DebugPrint("AI: %s:%s with %s:%s\n" _C_ PlayerRaces.Name[player->Race] _C_ 
		ait->Race ? ait->Race : "All" _C_ ainame _C_ ait->Class);

	pai->AiType = ait;
	pai->Script = ait->Script;

	pai->Collect[GoldCost] = 50;
	pai->Collect[WoodCost] = 50;
	pai->Collect[OilCost] = 0;

	player->Ai = pai;
}

/**
**  Initialise global structures of the AI
*/
void InitAiModule(void)
{
	AiResetUnitTypeEquiv();
}

/**
**  Cleanup the AI.
*/
void CleanAi(void)
{
	int i;
	int p;
	PlayerAi *pai;
	void *temp;
	CAiType *aitype;
	AiBuildQueue *queue;
	AiExplorationRequest *request;

	for (p = 0; p < PlayerMax; ++p) {
		if ((pai = (PlayerAi *)Players[p].Ai)) {
			//
			//  Free forces
			//
			for (i = 0; i < AI_MAX_ATTACKING_FORCES; ++i) {
				AiUnitType *aut;
				AiUnit *aiunit;

				for (aut = pai->Force[i].UnitTypes; aut; aut = (AiUnitType *)temp) {
					temp = aut->Next;
					delete aut;
				}
				for (aiunit = pai->Force[i].Units; aiunit; aiunit = (AiUnit *)temp) {
					temp = aiunit->Next;
					delete aiunit;
				}
			}

			//
			//  Free UnitTypeBuilt
			//
			for (queue = pai->UnitTypeBuilt; queue; queue = (AiBuildQueue *)temp) {
				temp = queue->Next;
				delete queue;
			}

			//
			// Free ExplorationRequest list
			//
			while (pai->FirstExplorationRequest) {
				request = pai->FirstExplorationRequest->Next;
				delete pai->FirstExplorationRequest;
				pai->FirstExplorationRequest = request;
			}

			delete pai;
			Players[p].Ai = NULL;
		}
	}

	//
	//  Free AiTypes.
	//
	for (aitype = AiTypes; aitype; aitype = (CAiType *)temp) {
		delete[] aitype->Name;
		delete[] aitype->Race;
		delete[] aitype->Class;
		delete[] aitype->Script;
		delete[] aitype->FunctionName;

		temp = aitype->Next;
		delete aitype;
	}
	AiTypes = NULL;

	//
	//  Free AiHelpers.
	//
	AiHelpers.Train.clear();
	AiHelpers.Build.clear();
	AiHelpers.Upgrade.clear();
	AiHelpers.Research.clear();
	AiHelpers.Repair.clear();
	AiHelpers.UnitLimit.clear();
	AiHelpers.Equiv.clear();

	AiResetUnitTypeEquiv();
}

/*----------------------------------------------------------------------------
-- Support functions
----------------------------------------------------------------------------*/

/**
**  Remove unit-type from build list.
**
**  @param pai   Computer AI player.
**  @param type  Unit-type which is now available.
**  @return      True, if unit-type was found in list.
*/
static int AiRemoveFromBuilt2(PlayerAi *pai, const CUnitType *type)
{
	AiBuildQueue **queue;
	AiBuildQueue *next;

	//
	//  Search the unit-type order.
	//
	for (queue = &pai->UnitTypeBuilt; (next = *queue); queue = &next->Next) {
		Assert(next->Want);
		if (type == next->Type && next->Made) {
			--next->Made;
			if (!--next->Want) {
				*queue = next->Next;
				delete next;
			}
			return 1;
		}
	}
	return 0;
}

/**
**  Remove unit-type from build list.
**
**  @param pai   Computer AI player.
**  @param type  Unit-type which is now available.
*/
static void AiRemoveFromBuilt(PlayerAi *pai, const CUnitType *type)
{
	int i;
	int equivalents[UnitTypeMax + 1];
	int equivalentsCount;

	if (AiRemoveFromBuilt2(pai, type)) {
		return;
	}

	//
	//  This could happen if an upgrade is ready, look for equivalent units.
	//
	equivalentsCount = AiFindUnitTypeEquiv(type, equivalents);
	for (i = 0; i < equivalentsCount; ++i) {
		if (AiRemoveFromBuilt2(pai, UnitTypes[equivalents[i]])) {
			return;
		}
	}

	if (pai->Player == ThisPlayer) {
		DebugPrint
			("My guess is that you built something under ai me. naughty boy!\n");
		return;
	}

	Assert(0);
}

/**
**  Reduce made unit-type from build list.
**
**  @param pai   Computer AI player.
**  @param type  Unit-type which is now available.
**  @return      True if the unit-type could be reduced.
*/
static int AiReduceMadeInBuilt2(const PlayerAi *pai, const CUnitType *type)
{
	AiBuildQueue *queue;
	//
	//  Search the unit-type order.
	//
	for (queue = pai->UnitTypeBuilt; queue; queue = queue->Next) {
		if (type == queue->Type && queue->Made) {
			queue->Made--;
			return 1;
		}
	}
	return 0;
}

/**
**  Reduce made unit-type from build list.
**
**  @param pai   Computer AI player.
**  @param type  Unit-type which is now available.
*/
static void AiReduceMadeInBuilt(const PlayerAi *pai, const CUnitType *type)
{
	int i;
	int equivs[UnitTypeMax + 1];
	int equivnb;

	if (AiReduceMadeInBuilt2(pai, type)) {
		return;
	}
	//
	//  This could happen if an upgrade is ready, look for equivalent units.
	//
	equivnb = AiFindUnitTypeEquiv(type, equivs);

	for (i = 0; i < (int)AiHelpers.Equiv[type->Slot].size(); ++i) {
		if (AiReduceMadeInBuilt2(pai, UnitTypes[equivs[i]])) {
			return;
		}
	}

	Assert(0);
}

/*----------------------------------------------------------------------------
-- Callback Functions
----------------------------------------------------------------------------*/

/**
**  Called if a Unit is Attacked
**
**  @param attacker  Pointer to attacker unit.
**  @param defender  Pointer to unit that is being attacked.
*/
void AiHelpMe(const CUnit *attacker, CUnit *defender)
{
	PlayerAi *pai;
	AiUnit *aiunit;
	int force;

	DebugPrint("%d: %d(%s) attacked at %d,%d\n" _C_
		defender->Player->Index _C_ UnitNumber(defender) _C_
		defender->Type->Ident _C_ defender->X _C_ defender->Y);

	//
	//  Don't send help to scouts (zeppelin,eye of vision).
	//
	if (!defender->Type->CanAttack && defender->Type->UnitType == UnitTypeFly) {
		return;
	}

	AiPlayer = pai = defender->Player->Ai;
	if (pai->Force[0].Attacking) {  // Force 0 busy
		return;
	}

	//
	//  If unit belongs to an attacking force, don't defend it.
	//
	for (force = 0; force < AI_MAX_ATTACKING_FORCES; ++force) {
		if (!pai->Force[force].Attacking) {  // none attacking
			// FIXME, send the force for help
			continue;
		}
		aiunit = pai->Force[force].Units;
		while (aiunit) {
			if (defender == aiunit->Unit) {
				return;
			}
			aiunit = aiunit->Next;
		}
	}

	//
	//  Send force 0 defending, also send force 1 if this is home.
	//
	if (attacker) {
		AiAttackWithForceAt(0, attacker->X, attacker->Y);
		if (!pai->Force[1].Attacking) {  // none attacking
			pai->Force[1].Defending = 1;
			AiAttackWithForceAt(1, attacker->X, attacker->Y);
		}
	} else {
		AiAttackWithForceAt(0, defender->X, defender->Y);
		if (!pai->Force[1].Attacking) {  // none attacking
			pai->Force[1].Defending = 1;
			AiAttackWithForceAt(1, defender->X, defender->Y);
		}
	}
	pai->Force[0].Defending = 1;
}

/**
**  Called if an unit is killed.
**
**  @param unit  Pointer to unit.
*/
void AiUnitKilled(CUnit *unit)
{
	DebugPrint("%d: %d(%s) killed\n" _C_
		unit->Player->Index _C_ UnitNumber(unit) _C_ unit->Type->Ident);

	Assert(unit->Player->Type != PlayerPerson);

	// FIXME: must handle all orders...
	switch (unit->Orders[0]->Action) {
		case UnitActionStill:
		case UnitActionAttack:
		case UnitActionMove:
			break;
		case UnitActionBuilt:
			DebugPrint("%d: %d(%s) killed, under construction!\n" _C_
				unit->Player->Index _C_ UnitNumber(unit) _C_ unit->Type->Ident);
			AiReduceMadeInBuilt(unit->Player->Ai, unit->Type);
			break;
		case UnitActionBuild:
			DebugPrint("%d: %d(%s) killed, with order %s!\n" _C_
				unit->Player->Index _C_ UnitNumber(unit) _C_
				unit->Type->Ident _C_ unit->Orders[0]->Type->Ident);
			if (!unit->Orders[0]->Goal) {
				AiReduceMadeInBuilt(unit->Player->Ai, unit->Orders[0]->Type);
			}
			break;
		default:
			DebugPrint("FIXME: %d: %d(%s) killed, with order %d!\n" _C_
				unit->Player->Index _C_ UnitNumber(unit) _C_
				unit->Type->Ident _C_ unit->Orders[0]->Action);
			break;
	}
}

/**
**  Called if work complete (Buildings).
**
**  @param unit  Pointer to unit that builds the building.
**  @param what  Pointer to unit building that was built.
*/
void AiWorkComplete(CUnit *unit, CUnit *what)
{
	if (unit) {
		DebugPrint("%d: %d(%s) build %s at %d,%d completed\n" _C_
			what->Player->Index _C_ UnitNumber(unit) _C_ unit->Type->Ident _C_
			what->Type->Ident _C_ unit->X _C_ unit->Y);
	} else {
		DebugPrint("%d: building %s at %d,%d completed\n" _C_
			what->Player->Index _C_ what->Type->Ident _C_ what->X _C_ what->Y);
	}

	Assert(what->Player->Type != PlayerPerson);

	AiRemoveFromBuilt(what->Player->Ai, what->Type);
}

/**
**  Called if building can't be build.
**
**  @param unit  Pointer to unit what builds the building.
**  @param what  Pointer to unit-type.
*/
void AiCanNotBuild(CUnit *unit, const CUnitType *what)
{
	DebugPrint("%d: %d(%s) Can't build %s at %d,%d\n" _C_
		unit->Player->Index _C_ UnitNumber(unit) _C_ unit->Type->Ident _C_
		what->Ident _C_ unit->X _C_ unit->Y);

	Assert(unit->Player->Type != PlayerPerson);

	AiReduceMadeInBuilt(unit->Player->Ai, what);
}

/**
**  Called if building place can't be reached.
**
**  @param unit  Pointer to unit what builds the building.
**  @param what  Pointer to unit-type.
*/
void AiCanNotReach(CUnit *unit, const CUnitType *what)
{
	Assert(unit->Player->Type != PlayerPerson);

	AiReduceMadeInBuilt(unit->Player->Ai, what);
}

/**
**  FIXME: docu
*/
static void AiMoveUnitInTheWay(CUnit *unit)
{
	static int dirs[8][2] = {{-1,-1},{-1,0},{-1,1},{0,1},{1,1},{1,0},{1,-1},{0,-1}};
	int ux0;
	int uy0;
	int ux1;
	int uy1;
	int bx0;
	int by0;
	int bx1;
	int by1;
	int x;
	int y;
	int trycount,i;
	CUnit *blocker;
	CUnitType *unittype;
	CUnitType *blockertype;
	CUnit *movableunits[16];
	int movablepos[16][2];
	int movablenb;

	AiPlayer = unit->Player->Ai;

	// No more than 1 move per cycle ( avoid stressing the pathfinder )
	if (GameCycle == AiPlayer->LastCanNotMoveGameCycle) {
		return;
	}

	unittype = unit->Type;

	ux0 = unit->X;
	uy0 = unit->Y;
	ux1 = ux0 + unittype->TileWidth - 1;
	uy1 = uy0 + unittype->TileHeight - 1;

	movablenb = 0;


	// Try to make some unit moves around it
	for (i = 0; i < NumUnits; ++i) {
		blocker = Units[i];

		if (blocker->IsUnusable()) {
			continue;
		}

		if (!blocker->IsIdle()) {
			continue;
		}

		if (blocker->Player != unit->Player) {
			// Not allied
			if (!(blocker->Player->Allied & (1 << unit->Player->Index))) {
				continue;
			}
		}

		blockertype = blocker->Type;

		if (blockertype->UnitType != unittype->UnitType) {
			continue;
		}

		if (!CanMove(blocker)) {
			continue;
		}

		bx0 = blocker->X;
		by0 = blocker->Y;
		bx1 = bx0 + blocker->Type->TileWidth - 1;
		by1 = by0 + blocker->Type->TileHeight - 1;;

		// Check for collision
#define int_min(a,b)  ((a)<(b)?(a):(b))
#define int_max(a,b)  ((a)>(b)?(a):(b))
		if (!((ux0 == bx1 + 1 || ux1 == bx0 - 1) &&
					(int_max(by0, uy0) <= int_min(by1, uy1))) &&
				!((uy0 == by1 + 1 || uy1 == by0 - 1) &&
					(int_max(bx0, ux0) <= int_min(bx1, ux1)))) {
			continue;
		}
#undef int_min
#undef int_max

		if (unit == blocker) {
			continue;
		}

		// Move blocker in a rand dir
		i = SyncRand() & 7;
		trycount = 8;
		while (trycount > 0) {
			i = (i + 1) & 7;
			--trycount;

			x = blocker->X + dirs[i][0];
			y = blocker->Y + dirs[i][1];

			// Out of the map => no !
			if (x < 0 || y < 0 || x >= Map.Info.MapWidth || y >= Map.Info.MapHeight) {
				continue;
			}
			// move to blocker ? => no !
			if (x == ux0 && y == uy0) {
				continue;
			}

			movableunits[movablenb] = blocker;
			movablepos[movablenb][0] = x;
			movablepos[movablenb][1] = y;

			++movablenb;
			trycount = 0;
		}
		if (movablenb >= 16) {
			break;
		}
	}

	// Don't move more than 1 unit.
	if (movablenb) {
		i = SyncRand() % movablenb;
		CommandMove(movableunits[i], movablepos[i][0], movablepos[i][1],
			FlushCommands);
		AiPlayer->LastCanNotMoveGameCycle = GameCycle;
	}
}

/**
**  Called if an unit can't move. Try to move unit in the way
**
**  @param unit  Pointer to unit what builds the building.
*/
void AiCanNotMove(CUnit *unit)
{
	int gx;
	int gy;
	int gw;
	int gh;
	int minrange;
	int maxrange;

	AiPlayer = unit->Player->Ai;

	if (unit->Orders[0]->Goal) {
		gw = unit->Orders[0]->Goal->Type->TileWidth;
		gh = unit->Orders[0]->Goal->Type->TileHeight;
		gx = unit->Orders[0]->Goal->X;
		gy = unit->Orders[0]->Goal->Y;
		maxrange = unit->Orders[0]->Range;
		minrange = unit->Orders[0]->MinRange;
	} else {
		// Take care of non square goals :)
		// If goal is non square, range states a non-existant goal rather
		// than a tile.
		gw = unit->Orders[0]->Width;
		gh = unit->Orders[0]->Height;
		maxrange = unit->Orders[0]->Range;
		minrange = unit->Orders[0]->MinRange;
		gx = unit->Orders[0]->X;
		gy = unit->Orders[0]->Y;
	}

	if (PlaceReachable(unit, gx, gy, gw, gh, minrange, maxrange) ||
			unit->Type->UnitType == UnitTypeFly) {
		// Path probably closed by unit here
		AiMoveUnitInTheWay(unit);
		return;
	}
}

/**
**  Called if the AI needs more farms.
**
**  @param unit  Point to unit.
**  @param what  Pointer to unit-type.
*/
void AiNeedMoreSupply(const CUnit *unit, const CUnitType *what)
{
	Assert(unit->Player->Type != PlayerPerson);

	((PlayerAi*)unit->Player->Ai)->NeedSupply = 1;
}

/**
**  Called if training of an unit is completed.
**
**  @param unit  Pointer to unit making.
**  @param what  Pointer to new ready trained unit.
*/
void AiTrainingComplete(CUnit *unit, CUnit *what)
{
	DebugPrint("%d: %d(%s) training %s at %d,%d completed\n" _C_
		unit->Player->Index _C_ UnitNumber(unit) _C_ unit->Type->Ident _C_
		what->Type->Ident _C_ unit->X _C_ unit->Y);

	Assert(unit->Player->Type != PlayerPerson);

	AiRemoveFromBuilt(unit->Player->Ai, what->Type);

	AiPlayer = unit->Player->Ai;
	AiCleanForces();
	AiAssignToForce(what);
}

/**
**  Called if upgrading of an unit is completed.
**
**  @param unit Pointer to unit working.
**  @param what Pointer to the new unit-type.
*/
void AiUpgradeToComplete(CUnit *unit, const CUnitType *what)
{
	DebugPrint("%d: %d(%s) upgrade-to %s at %d,%d completed\n" _C_
		unit->Player->Index _C_ UnitNumber(unit) _C_ unit->Type->Ident _C_
		what->Ident _C_ unit->X _C_ unit->Y);

	Assert(unit->Player->Type != PlayerPerson);
}

/**
**  Called if reseaching of an unit is completed.
**
**  @param unit  Pointer to unit working.
**  @param what  Pointer to the new upgrade.
*/
void AiResearchComplete(CUnit *unit, const CUpgrade *what)
{
	DebugPrint("%d: %d(%s) research %s at %d,%d completed\n" _C_
		unit->Player->Index _C_ UnitNumber(unit) _C_ unit->Type->Ident _C_
		what->Ident _C_ unit->X _C_ unit->Y);

	Assert(unit->Player->Type != PlayerPerson);

	// FIXME: upgrading knights -> paladins, must rebuild lists!
}

/**
**  This is called for each player, each game cycle.
**
**  @param player  The player structure pointer.
*/
void AiEachCycle(CPlayer *player)
{
	AiTransportRequest *aitr;
	AiTransportRequest *next;

	AiPlayer = player->Ai;

	aitr = AiPlayer->TransportRequests;
	while (aitr) {
		next = aitr->Next;

		aitr->Unit->RefsDecrease();
		if (aitr->Order.Goal) {
			aitr->Order.Goal->RefsDecrease();
		}
		delete aitr;

		aitr = next;
	}
	AiPlayer->TransportRequests = 0;
}

/**
**  This called for each player, each second.
**
**  @param player  The player structure pointer.
*/
void AiEachSecond(CPlayer *player)
{
	AiPlayer = player->Ai;
#ifdef DEBUG
	if (!AiPlayer) {
		return;
	}
#endif

	//
	//  Advance script
	//
	AiExecuteScript();

	//
	//  Look if everything is fine.
	//
	AiCheckUnits();
	//
	//  Handle the resource manager.
	//
	AiResourceManager();
	//
	//  Handle the force manager.
	//
	AiForceManager();
	//
	//  Check for magic actions.
	//
	AiCheckMagic();

	// At most 1 explorer each 5 seconds
	if (GameCycle > AiPlayer->LastExplorationGameCycle + 5 * CYCLES_PER_SECOND) {
		AiSendExplorers();
	}
}

//@}
