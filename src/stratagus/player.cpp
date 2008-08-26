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
/**@name player.cpp - The players. */
//
//      (c) Copyright 1998-2007 by Lutz Sammer, Jimmy Salmon, Nehal Mistry
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
-- Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "stratagus.h"
#include "video.h"
#include "sound.h"
#include "unitsound.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "map.h"
#include "ai.h"
#include "network.h"
#include "netconnect.h"
#include "interface.h"
#include "iolib.h"
#include "ui.h"
#include "actions.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

int NumPlayers;                  /// How many player slots used
CPlayer Players[PlayerMax];       /// All players in play
CPlayer *ThisPlayer;              /// Player on this computer
PlayerRace PlayerRaces;          /// Player races

bool NoRescueCheck;               /// Disable rescue check

/**
**  Colors used for minimap.
*/
SDL_Color *PlayerColorsRGB[PlayerMax];
Uint32 *PlayerColors[PlayerMax];

std::string PlayerColorNames[PlayerMax];

/**
**  Which indexes to replace with player color
*/
int PlayerColorIndexStart;
int PlayerColorIndexCount;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Init players.
*/
void InitPlayers(void)
{
	for (int p = 0; p < PlayerMax; ++p) {
		Players[p].Index = p;
		if (!Players[p].Type) {
			Players[p].Type = PlayerNobody;
		}
		for (int x = 0; x < PlayerColorIndexCount; ++x) {
			PlayerColors[p][x] = Video.MapRGB(TheScreen->format,
				PlayerColorsRGB[p][x].r,
				PlayerColorsRGB[p][x].g, PlayerColorsRGB[p][x].b);
		}
	}
}

/**
**  Clean up players.
*/
void CleanPlayers(void)
{
	ThisPlayer = NULL;
	for (unsigned int i = 0; i < PlayerMax; ++i) {
		Players[i].Clear();
	}
	NumPlayers = 0;
	NoRescueCheck = false;
}

#ifdef DEBUG
void FreePlayerColors(void)
{
	for (int i = 0; i < PlayerMax; ++i) {
		delete[] Players[i].UnitColors.Colors;
		delete[] PlayerColorsRGB[i];
		PlayerColorsRGB[i] = NULL;
		delete[] PlayerColors[i];
		PlayerColors[i] = NULL;
	}
}
#endif

/**
**  Clean up the PlayerRaces names.
*/
void CleanRaces(void)
{
	for (unsigned int i = 0; i < PlayerRaces.Count; ++i) {
		PlayerRaces.Name[i].clear();
		PlayerRaces.Display[i].clear();
	}
	PlayerRaces.Count = 0;
}

/**
**  Save state of players to file.
**
**  @param file  Output file.
**
**  @note FIXME: Not completely saved.
*/
void SavePlayers(CFile *file)
{
	int j;
	Uint8 r, g, b;

	file->printf("\n--------------------------------------------\n");
	file->printf("--- MODULE: players\n\n");

	//
	//  Dump all players
	//
	for (int i = 0; i < NumPlayers; ++i) {
		CPlayer *p = &Players[i];
		file->printf("Player(%d,\n", i);
		file->printf("  \"name\", \"%s\",\n", p->Name.c_str());
		file->printf("  \"type\", ");
		switch (p->Type) {
			case PlayerNeutral:       file->printf("\"neutral\",");         break;
			case PlayerNobody:        file->printf("\"nobody\",");          break;
			case PlayerComputer:      file->printf("\"computer\",");        break;
			case PlayerPerson:        file->printf("\"person\",");          break;
			case PlayerRescuePassive: file->printf("\"rescue-passive\",");break;
			case PlayerRescueActive:  file->printf("\"rescue-active\","); break;
			default:                  file->printf("%d,",p->Type);break;
		}
		file->printf(" \"race\", \"%s\",", PlayerRaces.Name[p->Race].c_str());
		file->printf(" \"ai-name\", \"%s\",\n", p->AiName.c_str());
		file->printf("  \"team\", %d,", p->Team);

		file->printf(" \"enemy\", \"");
		for (j = 0; j < PlayerMax; ++j) {
			file->printf("%c",(p->Enemy & (1 << j)) ? 'X' : '_');
		}
		file->printf("\", \"allied\", \"");
		for (j = 0; j < PlayerMax; ++j) {
			file->printf("%c", (p->Allied & (1 << j)) ? 'X' : '_');
		}
		file->printf("\", \"shared-vision\", \"");
		for (j = 0; j < PlayerMax; ++j) {
			file->printf("%c", (p->SharedVision & (1 << j)) ? 'X' : '_');
		}
		file->printf("\",\n  \"start\", {%d, %d},\n", p->StartX,
			p->StartY);

		// Resources
		file->printf("  \"resources\", {");
		for (j = 0; j < MaxCosts; ++j) {
			if (j) {
				if (j == MaxCosts / 2) {
					file->printf("\n ");
				} else {
					file->printf(" ");
				}
			}
			file->printf("\"%s\", %d,", DefaultResourceNames[j].c_str(),
				p->Resources[j]);
		}
		// Last Resources
		file->printf("},\n  \"last-resources\", {");
		for (j = 0; j < MaxCosts; ++j) {
			if (j) {
				if (j == MaxCosts / 2) {
					file->printf("\n ");
				} else {
					file->printf(" ");
				}
			}
			file->printf("\"%s\", %d,", DefaultResourceNames[j].c_str(),
				p->LastResources[j]);
		}
		// Incomes
		file->printf("},\n  \"incomes\", {");
		for (j = 0; j < MaxCosts; ++j) {
			if (j) {
				if (j == MaxCosts / 2) {
					file->printf("\n ");
				} else {
					file->printf(" ");
				}
			}
			file->printf("\"%s\", %d,", DefaultResourceNames[j].c_str(),
				p->Incomes[j]);
		}
		// Revenue
		file->printf("},\n  \"revenue\", {");
		for (j = 0; j < MaxCosts; ++j) {
			if (j) {
				if (j == MaxCosts / 2) {
					file->printf("\n ");
				} else {
					file->printf(" ");
				}
			}
			file->printf("\"%s\", %d,", DefaultResourceNames[j].c_str(),
				p->Revenue[j]);
		}

		// UnitTypesCount done by load units.

		file->printf("},\n  \"%s\",\n", p->AiEnabled ?
			"ai-enabled" : "ai-disabled");

		// Ai done by load ais.
		// Units done by load units.
		// TotalNumUnits done by load units.
		// NumBuildings done by load units.

		file->printf(" \"supply\", %d,",p->Supply);
		file->printf(" \"unit-limit\", %d,", p->UnitLimit);
		file->printf(" \"building-limit\", %d,", p->BuildingLimit);
		file->printf(" \"total-unit-limit\", %d,", p->TotalUnitLimit);

		file->printf("\n  \"score\", %d,", p->Score);
		file->printf("\n  \"total-units\", %d,", p->TotalUnits);
		file->printf("\n  \"total-buildings\", %d,", p->TotalBuildings);
		file->printf("\n  \"total-resources\", {");
		for (j = 0; j < MaxCosts; ++j) {
			if (j) {
				file->printf(" ");
			}
			file->printf("%d,", p->TotalResources[j]);
		}
		file->printf("},");
		file->printf("\n  \"total-razings\", %d,", p->TotalRazings);
		file->printf("\n  \"total-kills\", %d,", p->TotalKills);

		SDL_GetRGB(p->Color, TheScreen->format, &r, &g, &b);
		file->printf("\n  \"color\", { %d, %d, %d },", r, g, b);

		// UnitColors done by init code.
		// Allow saved by allow.

		file->printf("\n  \"timers\", {");
		for (j = 0; j < UpgradeMax; ++j) {
			if (j) {
				file->printf(" ,");
			}
			file->printf("%d", p->UpgradeTimers.Upgrades[j]);
		}
		file->printf("})\n\n");
	}

	DebugPrint("FIXME: HAVE TO ADD AUTO ATTACK TARGETS SAVE/LOAD SUPPORT!!!!!\n");
	DebugPrint("FIXME: must save unit-stats?\n");

	//
	//  Dump local variables
	//
	file->printf("SetThisPlayer(%d)\n\n", ThisPlayer->Index);
}

/**
**  Create a new player.
**
**  @param type  Player type (Computer,Human,...).
*/
void CreatePlayer(int type)
{
	int team;
	int i;
	CPlayer *player;

	if (NumPlayers == PlayerMax) { // already done for bigmaps!
		return;
	}
	player = &Players[NumPlayers];
	player->Index = NumPlayers;

	//  Allocate memory for the "list" of this player's units.
	//  FIXME: brutal way, as we won't need UnitMax for this player...
	//  FIXME: ARI: is this needed for 'PlayerNobody' ??
	//  FIXME: A: Johns: currently we need no init for the nobody player.
	memset(player->Units, 0, sizeof(player->Units));

	player->AutoAttackTargets.Units.clear();

	//
	//  Take first slot for person on this computer,
	//  fill other with computer players.
	//
	if (type == PlayerPerson && !NetPlayers) {
		if (!ThisPlayer) {
			ThisPlayer = player;
		} else {
			type = PlayerComputer;
		}
	}
	if (NetPlayers && NumPlayers == NetLocalPlayerNumber) {
		ThisPlayer = &Players[NetLocalPlayerNumber];
	}

	if (NumPlayers == PlayerMax) {
		static int already_warned;

		if (!already_warned) {
			DebugPrint("Too many players\n");
			already_warned = 1;
		}
		return;
	}

	//
	//  Make simple teams:
	//  All person players are enemies.
	//
	switch (type) {
		case PlayerNeutral:
		case PlayerNobody:
		default:
			team = 0;
			player->SetName("Neutral");
			break;
		case PlayerComputer:
			team = 1;
			player->SetName("Computer");
			break;
		case PlayerPerson:
			team = 2 + NumPlayers;
			player->SetName("Person");
			break;
		case PlayerRescuePassive:
		case PlayerRescueActive:
			// FIXME: correct for multiplayer games?
			player->SetName("Computer");
			team = 2 + NumPlayers;
			break;
	}
	DebugPrint("CreatePlayer name %s\n" _C_ player->Name.c_str());

	player->Type = type;
	player->Race = 0;
	player->Team = team;
	player->Enemy = 0;
	player->Allied = 0;
	player->AiName = "ai-passive";

	//
	//  Calculate enemy/allied mask.
	//
	for (i = 0; i < NumPlayers; ++i) {
		switch (type) {
			case PlayerNeutral:
			case PlayerNobody:
			default:
				break;
			case PlayerComputer:
				// Computer allied with computer and enemy of all persons.
				if (Players[i].Type == PlayerComputer) {
					player->Allied |= (1 << i);
					Players[i].Allied |= (1 << NumPlayers);
				} else if (Players[i].Type == PlayerPerson ||
						Players[i].Type == PlayerRescueActive) {
					player->Enemy |= (1 << i);
					Players[i].Enemy |= (1 << NumPlayers);
				}
				break;
			case PlayerPerson:
				// Humans are enemy of all?
				if (Players[i].Type == PlayerComputer ||
						Players[i].Type == PlayerPerson) {
					player->Enemy |= (1 << i);
					Players[i].Enemy |= (1 << NumPlayers);
				} else if (Players[i].Type == PlayerRescueActive ||
						Players[i].Type == PlayerRescuePassive) {
					player->Allied |= (1 << i);
					Players[i].Allied |= (1 << NumPlayers);
				}
				break;
			case PlayerRescuePassive:
				// Rescue passive are allied with persons
				if (Players[i].Type == PlayerPerson) {
					player->Allied |= (1 << i);
					Players[i].Allied |= (1 << NumPlayers);
				}
				break;
			case PlayerRescueActive:
				// Rescue active are allied with persons and enemies of computer
				if (Players[i].Type == PlayerComputer) {
					player->Enemy |= (1 << i);
					Players[i].Enemy |= (1 << NumPlayers);
				} else if (Players[i].Type == PlayerPerson) {
					player->Allied |= (1 << i);
					Players[i].Allied |= (1 << NumPlayers);
				}
				break;
		}
	}

	//
	//  Initial default incomes.
	//
	for (i = 0; i < MaxCosts; ++i) {
		player->Incomes[i] = DefaultIncomes[i];
	}

	memset(player->UnitTypesCount, 0, sizeof(player->UnitTypesCount));

	player->Supply = 0;
	player->Demand = 0;
	player->NumBuildings = 0;
	player->TotalNumUnits = 0;
	player->Score = 0;

	player->Color = PlayerColors[NumPlayers][0];

	if (Players[NumPlayers].Type == PlayerComputer ||
			Players[NumPlayers].Type == PlayerRescueActive) {
		player->AiEnabled = 1;
	} else {
		player->AiEnabled = 0;
	}

	++NumPlayers;
}

/**
**  Change player side.
**
**  @param side    New side (Race).
*/
void CPlayer::SetSide(int side)
{
	Assert((unsigned int) side < PlayerRaces.Count);
	Assert(!PlayerRaces.Name[side].empty());

	this->Race = side;
}

/**
**  Change player name.
**
**  @param name    New name.
*/
void CPlayer::SetName(const std::string &name)
{
	Name = name;
}

/**
**  Clear all player data excepts members which don't change.
**
**  The fields that are not cleared are 
**  UnitLimit, BuildingLimit, TotalUnitLimit and Allow.
*/
void CPlayer::Clear()
{
	Index = 0;
	Name.clear();
	Type = 0;
	Race = 0;
	AiName.clear();
	Team = 0;
	Enemy = 0;
	Allied = 0;
	SharedVision = 0;
	StartX = 0;
	StartY = 0;
	memset(Resources, 0, sizeof(Resources));
	memset(LastResources, 0, sizeof(LastResources));
	memset(Incomes, 0, sizeof(Incomes));
	memset(Revenue, 0, sizeof(Revenue));
	memset(UnitTypesCount, 0, sizeof(UnitTypesCount));
	AiEnabled = 0;
	Ai = 0;
	memset(Units, 0, sizeof(Units));
	TotalNumUnits = 0;
	NumBuildings = 0;
	Supply = 0;
	Demand = 0;
	// FIXME: can't clear limits since it's initialized already
//	UnitLimit = 0;
//	BuildingLimit = 0;
//	TotalUnitLimit = 0;
	Score = 0;
	TotalUnits = 0;
	TotalBuildings = 0;
	memset(TotalResources, 0, sizeof(TotalResources));
	TotalRazings = 0;
	TotalKills = 0;
	Color = 0;
	UpgradeTimers.Clear();
	AutoAttackTargets.Units.clear();
}

/*----------------------------------------------------------------------------
--  Resource management
----------------------------------------------------------------------------*/

/**
**  Change the player resource.
**
**  @param resource  Resource to change.
**  @param value     How many of this resource.
*/
void CPlayer::SetResource(int resource, int value)
{
	this->Resources[resource] = value;
}

/**
**  Check if the unit-type didn't break any unit limits.
**
**  @param type    Type of unit.
**
**  @return        True if enough, negative on problem.
**
**  @note The return values of the PlayerCheck functions are inconsistent.
*/
int CPlayer::CheckLimits(const CUnitType *type) const
{
	//
	//  Check game limits.
	//
	if (NumUnits < UnitMax) {
		if (type->Building && NumBuildings >= BuildingLimit) {
			Notify(NotifyYellow, -1, -1, _("Building Limit Reached"));
			return -1;
		}
		if (!type->Building && (TotalNumUnits - NumBuildings) >= UnitLimit) {
			Notify(NotifyYellow, -1, -1, _("Unit Limit Reached"));
			return -2;
		}
		if (this->Demand + type->Demand > this->Supply && type->Demand) {
			Notify(NotifyYellow, -1, -1, _("Insufficient Supply, increase Supply."));
			return -3;
		}
		if (TotalNumUnits >= TotalUnitLimit) {
			Notify(NotifyYellow, -1, -1, _("Total Unit Limit Reached"));
			return -4;
		}
		if (UnitTypesCount[type->Slot] >=  Allow.Units[type->Slot]) {
			Notify(NotifyYellow, -1, -1, _("Limit of %d reached for this unit type"),
				Allow.Units[type->Slot]);
			return -6;
		}
		return 1;
	} else {
		Notify(NotifyYellow, -1, -1, _("Cannot create more units."));
		if (AiEnabled) {
			// AiNoMoreUnits(player, type);
		}
		return -5;
	}
}

/**
**  Check if enough resources for are available.
**
**  @param costs   How many costs.
**
**  @return        False if all enough, otherwise a bit mask.
**
**  @note The return values of the PlayerCheck functions are inconsistent.
*/
int CPlayer::CheckCosts(const int *costs) const
{
	int err = 0;
	for (int i = 1; i < MaxCosts; ++i) {
		if (this->Resources[i] < costs[i]) {
			Notify(NotifyYellow, -1, -1, "Not enough %s...%s more %s.",
				DefaultResourceNames[i].c_str(), DefaultActions[i].c_str(), DefaultResourceNames[i].c_str());

			err |= 1 << i;
		}
	}
	return err;
}

/**
**  Check if enough resources for new unit is available.
**
**  @param type    Type of unit.
**
**  @return        False if all enough, otherwise a bit mask.
*/
int CPlayer::CheckUnitType(const CUnitType *type) const
{
	return this->CheckCosts(type->Stats[this->Index].Costs);
}

/**
**  Add costs to the resources
**
**  @param costs   How many costs.
*/
void CPlayer::AddCosts(const int *costs)
{
	for (int i = 1; i < MaxCosts; ++i) {
		this->Resources[i] += costs[i];
	}
}

/**
**  Add the costs of an unit type to resources
**
**  @param type    Type of unit.
*/
void CPlayer::AddUnitType(const CUnitType *type)
{
	// FIXME: a player could make money by upgrading and than cancel
	AddCosts(type->Stats[this->Index].Costs);
}

/**
**  Add a factor of costs to the resources
**
**  @param costs   How many costs.
**  @param factor  Factor of the costs to apply.
*/
void CPlayer::AddCostsFactor(const int *costs, int factor)
{
	for (int i = 1; i < MaxCosts; ++i) {
		this->Resources[i] += costs[i] * factor / 100;
	}
}

/**
**  Subtract costs from the resources
**
**  @param costs   How many costs.
*/
void CPlayer::SubCosts(const int *costs)
{
	for (int i = 1; i < MaxCosts; ++i) {
		this->Resources[i] -= costs[i];
	}
}

/**
**  Substract the costs of new unit from resources
**
**  @param type    Type of unit.
*/
void CPlayer::SubUnitType(const CUnitType *type)
{
	this->SubCosts(type->Stats[this->Index].Costs);
}

/**
**  Substract a factor of costs from the resources
**
**  @param costs   How many costs.
**  @param factor  Factor of the costs to apply.
*/
void CPlayer::SubCostsFactor(const int *costs, int factor)
{
	for (int i = 1; i < MaxCosts; ++i) {
		this->Resources[i] -= costs[i] * 100 / factor;
	}
}

/**
**  Have unit of type.
**
**  @param type    Type of unit.
**
**  @return        How many exists, false otherwise.
*/
int CPlayer::HaveUnitTypeByType(const CUnitType *type) const
{
	return UnitTypesCount[type->Slot];
}

/**
**  Have unit of type.
**
**  @param ident   Identifier of unit-type that should be lookuped.
**
**  @return        How many exists, false otherwise.
**
**  @note This function should not be used during run time.
*/
int CPlayer::HaveUnitTypeByIdent(const std::string &ident) const
{
	return UnitTypesCount[UnitTypeByIdent(ident)->Slot];
}

/**
**  Initialize the Ai for all players.
*/
void PlayersInitAi(void)
{
	for (int player = 0; player < NumPlayers; ++player) {
		if (Players[player].AiEnabled) {
			AiInit(&Players[player]);
		}
	}
}

/**
**  Handle AI of all players each game cycle.
*/
void PlayersEachCycle(void)
{
	for (int player = 0; player < NumPlayers; ++player) {
		CPlayer *p = &Players[player];
		if(p->AutoAttackTargets.size() > 0) {
			CUnitCache &autoatacktargets = p->AutoAttackTargets;
			/* both loops can not be connected !!!! */
			for(unsigned int i = 0; i < autoatacktargets.size(); ++i) {
				CUnit *aatarget = autoatacktargets[i];
				if(!aatarget->IsAliveOnMap() ||
					Map.Field(aatarget->X, aatarget->Y)->Guard[player] == 0) {
					autoatacktargets.Units.erase(autoatacktargets.Units.begin() + i);
					aatarget->RefsDecrease();
				}
			}
			if(autoatacktargets.size() > 0) {
				for (int j = 0; j < p->TotalNumUnits; ++j) {
					CUnit *guard = p->Units[j];
					bool stand_ground = 
						guard->Orders[0]->Action == UnitActionStandGround;
					if (guard->Type->CanAttack && 
								(stand_ground || guard->IsIdle()) &&
								 !guard->IsUnusable()) {
						DebugPrint("%d: Gaurd %d (%s) call autoattack\n"
							 _C_ p->Index _C_ guard->Slot _C_ guard->Type->Name.c_str()); 
						AutoAttack(guard, autoatacktargets, stand_ground);
					}
				}
			}
		}	
		if (p->AiEnabled) {
			AiEachCycle(p);
		}
	}
}

/**
**  Handle AI of a player each second.
**
**  @param player  the player to update AI
*/
void PlayersEachSecond(int player)
{
	if ((GameCycle / CYCLES_PER_SECOND) % 10 == 0) {
		for (int res = 0; res < MaxCosts; ++res) {
			Players[player].Revenue[res] =
				Players[player].Resources[res] -
				Players[player].LastResources[res];
			Players[player].Revenue[res] *= 6;  // estimate per minute
			Players[player].LastResources[res] =
				Players[player].Resources[res];
		}
	}
	if (Players[player].AiEnabled) {
		AiEachSecond(&Players[player]);
	}
}

#ifndef USE_OPENGL
/**
**  Change current color set to new player.
**
**  FIXME: use function pointer here.
**
**  @param player  Pointer to player.
**  @param sprite  The sprite in which the colors should be changed.
*/
void GraphicPlayerPixels(CPlayer *player, const CGraphic *sprite)
{
	Assert(PlayerColorIndexCount);

	SDL_LockSurface(sprite->Surface);
	SDL_SetColors(sprite->Surface, player->UnitColors.Colors,
		PlayerColorIndexStart, PlayerColorIndexCount);
	if (sprite->SurfaceFlip) {
		SDL_SetColors(sprite->SurfaceFlip,
			player->UnitColors.Colors, PlayerColorIndexStart, PlayerColorIndexCount);
	}
	SDL_UnlockSurface(sprite->Surface);
}
#endif

/**
**  Setup the player colors for the current palette.
**
**  @todo  FIXME: could be called before PixelsXX is setup.
*/
void SetPlayersPalette(void)
{
	for (int i = 0; i < PlayerMax; ++i) {
		delete[] Players[i].UnitColors.Colors;
		Players[i].UnitColors.Colors = new SDL_Color[PlayerColorIndexCount];
		memcpy(Players[i].UnitColors.Colors, PlayerColorsRGB[i],
			sizeof(SDL_Color) * PlayerColorIndexCount);
	}
}

/**
**  Output debug informations for players.
*/
void DebugPlayers(void)
{
#ifdef DEBUG
	int i;
	const char *playertype;

	DebugPrint("Nr   Color   I Name     Type         Race    Ai\n");
	DebugPrint("--  -------- - -------- ------------ ------- -----\n");
	for (i = 0; i < PlayerMax; ++i) {
		if (Players[i].Type == PlayerNobody) {
			continue;
		}
		switch (Players[i].Type) {
			case 0: playertype = "Don't know 0"; break;
			case 1: playertype = "Don't know 1"; break;
			case 2: playertype = "neutral     "; break;
			case 3: playertype = "nobody      "; break;
			case 4: playertype = "computer    "; break;
			case 5: playertype = "person      "; break;
			case 6: playertype = "rescue pas. "; break;
			case 7: playertype = "rescue akt. "; break;
			default : playertype = "?unknown?   "; break;
		}
		DebugPrint("%2d: %8.8s %c %-8.8s %s %7s %s\n" _C_ i _C_ PlayerColorNames[i].c_str() _C_
			ThisPlayer == &Players[i] ? '*' :
				Players[i].AiEnabled ? '+' : ' ' _C_
			Players[i].Name.c_str() _C_ playertype _C_
			PlayerRaces.Name[Players[i].Race].c_str() _C_
			Players[i].AiName.c_str());
	}
#endif
}

/**
**  Notify player about a problem.
**
**  @param type    Problem type
**  @param x       Map X tile position
**  @param y       Map Y tile position
**  @param fmt     Message format
**  @param ...     Message varargs
**
**  @note The parameter type, isn't yet used.
**  @todo FIXME: We must also notfiy allied players.
*/
void CPlayer::Notify(int type, int x, int y, const char *fmt, ...) const
{
	char temp[128];
	va_list va;

	// Notify me, and my TEAM members
	if (this != ThisPlayer && !IsTeamed(ThisPlayer)) {
		return;
	}

	va_start(va, fmt);
	temp[sizeof(temp) - 1] = '\0';
	vsnprintf(temp, sizeof(temp) - 1, fmt, va);
	va_end(va);

	if (x != -1) {
		UI.Minimap.AddEvent(x, y);
	}
	if (this == ThisPlayer) {
		SetMessageEvent(x, y, "%s", temp);
	} else {
		SetMessageEvent(x, y, "(%s): %s", Name.c_str(), temp);
	}

}

/**
**  Check if the player is an enemy
*/
bool CPlayer::IsEnemy(const CPlayer *x) const
{
	return IsEnemy(x->Index);
}

/**
**  Check if the unit is an enemy
*/
bool CPlayer::IsEnemy(const CUnit *x) const
{
	return IsEnemy(x->Player);
}

/**
**  Check if the player is an ally
*/
bool CPlayer::IsAllied(const CPlayer *x) const
{
	return (Allied & (1 << x->Index)) != 0;
}

/**
**  Check if the unit is an ally
*/
bool CPlayer::IsAllied(const CUnit *x) const
{
	return IsAllied(x->Player);
}

/**
**  Check if the player shares vision with the player
*/
bool CPlayer::IsSharedVision(const CPlayer *x) const
{
	return (SharedVision & (1 << x->Index)) != 0;
}

/**
**  Check if the player shares vision with the unit
*/
bool CPlayer::IsSharedVision(const CUnit *x) const
{
	return IsSharedVision(x->Player);
}

/**
**  Check if the both players share vision
*/
bool CPlayer::IsBothSharedVision(const CPlayer *x) const
{
	return (SharedVision & (1 << x->Index)) != 0 &&
		(x->SharedVision & (1 << Index)) != 0;
}

/**
**  Check if the player and the unit share vision
*/
bool CPlayer::IsBothSharedVision(const CUnit *x) const
{
	return IsBothSharedVision(x->Player);
}

/**
**  Check if the player is teamed
*/
bool CPlayer::IsTeamed(const CPlayer *x) const
{
	return Team == x->Team;
}

/**
**  Check if the unit is teamed
*/
bool CPlayer::IsTeamed(const CUnit *x) const
{
	return IsTeamed(x->Player);
}

//@}
