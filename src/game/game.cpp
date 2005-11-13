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
/**@name game.cpp - The game set-up and creation. */
//
//      (c) Copyright 1998-2005 by Lutz Sammer, Andreas Arens, and
//                                 Jimmy Salmon
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
#include "map.h"
#include "tileset.h"
#include "minimap.h"
#include "player.h"
#include "unit.h"
#include "unittype.h"
#include "upgrade.h"
#include "pathfinder.h"
#include "ui.h"
#include "font.h"
#include "sound.h"
#include "sound_server.h"
#include "menus.h"
#include "depend.h"
#include "interface.h"
#include "cursor.h"
#include "spells.h"
#include "construct.h"
#include "actions.h"
#include "network.h"
#include "netconnect.h"
#include "missile.h"
#include "settings.h"
#include "campaign.h"
#include "trigger.h"
#include "commands.h"
#include "iocompat.h"

#include "script.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

Settings GameSettings;  /// Game Settings
static int LcmPreventRecurse;   /// prevent recursion through LoadGameMap

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Map loading/saving
----------------------------------------------------------------------------*/

/**
**  Load a Stratagus map.
**
**  @param mapname   map filename
**  @param map       map loaded
*/
static void LoadStratagusMap(const char *mapname, CMap *map)
{
	char mapfull[PATH_MAX];
	CFile file;

	if (file.open(mapname, CL_OPEN_READ) == -1) {
		strcpy(mapfull, StratagusLibPath);
		strcat(mapfull, "/");
		strcat(mapfull, mapname);
	} else {
		strcpy(mapfull, mapname);
		file.close();
	}

	if (LcmPreventRecurse) {
		fprintf(stderr, "recursive use of load Stratagus map!\n");
		ExitFatal(-1);
	}
	InitPlayers();
	LcmPreventRecurse = 1;
	if (LuaLoadFile(mapfull) == -1) {
		fprintf(stderr, "Can't load lua file: %s\n", mapfull);
		ExitFatal(-1);
	}
	LcmPreventRecurse = 0;

#if 0
	// Not true if multiplayer levels!
	if (!ThisPlayer) { /// ARI: bomb if nothing was loaded!
		fprintf(stderr, "%s: invalid Stratagus map\n", mapname);
		ExitFatal(-1);
	}
#endif
	if (!Map.Info.MapWidth || !Map.Info.MapHeight) {
		fprintf(stderr, "%s: invalid Stratagus map\n", mapname);
		ExitFatal(-1);
	}
	Map.Info.Filename = new_strdup(mapname);
}

/**
**  Save a Stratagus map.
**
**  @param mapname   map filename
**  @param map       map to save
**  @param writeTerrain   write the tiles map in the .sms
*/
int SaveStratagusMap(const char *mapname, CMap *map, int writeTerrain)
{
	gzFile f;
	int i, j;
	const char *type[] = {"", "", "neutral", "nobody", 
		"computer", "person", "rescue-passive", "rescue-active"};
	char mapsetup[PATH_MAX];
	char *mapsetupname;
	char *extension;
	int numplayers, topplayer;

	if (!map->Info.MapWidth || !map->Info.MapHeight) {
		fprintf(stderr, "%s: invalid Stratagus map\n", mapname);
		ExitFatal(-1);
	}

	numplayers = 0;
	topplayer = PlayerMax - 2;
	strcpy(mapsetup, mapname);
	extension = strstr(mapsetup, ".smp");
	if (!extension) {
		fprintf(stderr, "%s: invalid Statagus map filename\n", mapname);
	}
	memcpy(extension, ".sms", 4 * sizeof(char));

	// Write the map presentation file
	if( !(f=gzopen(mapname, strcasestr(mapname,".gz") ? "wb9" : "wb0")) ) {
		fprintf(stderr,"Can't save map `%s'\n", mapname);
		return -1;
	}
	gzprintf(f, "-- Stratagus Map Presentation\n");
	gzprintf(f, "-- File generated by the stratagus builtin editor.\n");
	// MAPTODO Copyright notice in generated file
	gzprintf(f, "-- File licensed under the GNU GPL version 2.\n\n");

	gzprintf(f, "DefinePlayerTypes(");
	gzprintf(f, "\"%s\"", type[map->Info.PlayerType[0]]);
	while (topplayer > 0 && map->Info.PlayerType[topplayer] == PlayerNobody) {
		--topplayer;
	}
	for (i = 1; i <= topplayer; ++i) {
		gzprintf(f, ", \"%s\"", type[map->Info.PlayerType[i]]);
		++numplayers;
	}
	gzprintf(f, ")\n");
	
	gzprintf(f, "PresentMap(\"%s\", %d, %d, %d, %d)\n",
			map->Info.Description, numplayers, map->Info.MapWidth, map->Info.MapHeight,
			map->Info.MapUID + 1);

	mapsetupname = strrchr(mapsetup, '/');
	if (!mapsetupname)
		mapsetupname = mapsetup;
	gzprintf(f, "DefineMapSetup(GetCurrentLuaPath()..\"%s\")\n", mapsetupname);
	gzclose(f);

	// Write the map setup file
	if ( !(f = gzopen(mapsetup, strcasestr(mapsetup,".gz") ? "wb9" : "wb0")) ) {
		fprintf(stderr,"Can't save map `%s' (no .smp extension)\n", mapname);
		return -1;
	}
	gzprintf(f, "-- Stratagus Map Setup\n");
	gzprintf(f, "-- File generated by the stratagus builtin editor.\n");
	// MAPTODO Copyright notice in generated file
	gzprintf(f, "-- File licensed under the GNU GPL version 2.\n\n");
	
	gzprintf(f, "-- player configuration\n");
	for (i = 0; i < PlayerMax; ++i) {
		gzprintf(f, "SetStartView(%d, %d, %d)\n", i, Players[i].StartX, Players[i].StartY);
		gzprintf(f, "SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
				i, DefaultResourceNames[WoodCost], 
				Players[i].Resources[WoodCost]);
		gzprintf(f, "SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
				i, DefaultResourceNames[GoldCost], 
				Players[i].Resources[GoldCost]);
		gzprintf(f, "SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
				i, DefaultResourceNames[OilCost], 
				Players[i].Resources[OilCost]);
		gzprintf(f, "SetPlayerData(%d, \"RaceName\", \"%s\")\n",
				i, PlayerRaces.Name[Players[i].Race]);
		gzprintf(f, "SetAiType(%d, \"%s\")\n",
				i, Players[i].AiName);
	}
	gzprintf(f, "\n");

	gzprintf(f, "-- load tilesets\n");
	gzprintf(f, "LoadTileModels(\"%s\")\n\n", map->TileModelsFileName);
	
	if (writeTerrain) {
		gzprintf(f, "-- Tiles Map\n");
		for (i = 0; i < map->Info.MapHeight; ++i) {
			for (j = 0; j < map->Info.MapWidth; ++j) {
				int tile;
				int n;
			
				tile = map->Fields[j+i*map->Info.MapWidth].Tile;
				for (n=0; n < map->Tileset.NumTiles && tile != map->Tileset.Table[n]; ++n) {
				}
				gzprintf(f, "SetTile(%3d, %d, %d)\n", n, j, i);
			}
		}
	}

	gzprintf(f, "\n--allow\n");

	gzprintf(f, "-- place units\n");
	for (i = 0; i < NumUnits; ++i) {
		gzprintf(f, "unit= CreateUnit(\"%s\", %d, {%d, %d})\n",
			Units[i]->Type->Ident,
			Units[i]->Player->Index,
			Units[i]->X, Units[i]->Y);
		if (Units[i]->Type->GivesResource) {
			gzprintf(f, "SetResourcesHeld(unit, %d)\n", Units[i]->ResourcesHeld);
		}
	}
	gzprintf(f, "\n\n");

	gzclose(f);
	return 1;
}

/**
**  Load any map.
**
**  @param filename  map filename
**  @param map       map loaded
*/
static void LoadMap(const char *filename, CMap *map)
{
	const char *tmp;

	tmp = strrchr(filename, '.');
	if (tmp) {
#ifdef USE_ZLIB
		if (!strcmp(tmp, ".gz")) {
			while (tmp - 1 > filename && *--tmp != '.') {
			}
		}
#endif
#ifdef USE_BZ2LIB
		if (!strcmp(tmp, ".bz2")) {
			while (tmp - 1 > filename && *--tmp != '.') {
			}
		}
#endif
		if (!strcmp(tmp, ".smp")) {
			if (!map->Info.Filename) {
				// The map info hasn't been loaded yet => do it now
				LoadStratagusMapInfo(filename);
			}
			Assert(map->Info.Filename);
			map->Create();
			LoadStratagusMap(map->Info.Filename, map);
			return;
		}
	}

	fprintf(stderr, "Unrecognized map format\n");
	ExitFatal(-1);
}

/*----------------------------------------------------------------------------
--  Game creation
----------------------------------------------------------------------------*/

/**
**  Free for all
*/
static void GameTypeFreeForAll(void)
{
	int i;
	int j;

	for (i = 0; i < PlayerMax - 1; ++i) {
		for (j = 0; j < PlayerMax - 1; ++j) {
			if (i != j) {
				CommandDiplomacy(i, DiplomacyEnemy, j);
			}
		}
	}
}

/**
**  Top vs Bottom
*/
static void GameTypeTopVsBottom(void)
{
	int i;
	int j;
	int top;
	int middle;

	middle = Map.Info.MapHeight / 2;
	for (i = 0; i < PlayerMax - 1; ++i) {
		top = Players[i].StartY <= middle;
		for (j = 0; j < PlayerMax - 1; ++j) {
			if (i != j) {
				if ((top && Players[j].StartY <= middle) ||
						(!top && Players[j].StartY > middle)) {
					CommandDiplomacy(i, DiplomacyAllied, j);
					Players[i].SharedVision |= (1 << j);
				} else {
					CommandDiplomacy(i, DiplomacyEnemy, j);
				}
			}
		}
	}
}

/**
**  Left vs Right
*/
static void GameTypeLeftVsRight(void)
{
	int i;
	int j;
	int left;
	int middle;

	middle = Map.Info.MapWidth / 2;
	for (i = 0; i < PlayerMax - 1; ++i) {
		left = Players[i].StartX <= middle;
		for (j = 0; j < PlayerMax - 1; ++j) {
			if (i != j) {
				if ((left && Players[j].StartX <= middle) ||
						(!left && Players[j].StartX > middle)) {
					CommandDiplomacy(i, DiplomacyAllied, j);
					Players[i].SharedVision |= (1 << j);
				} else {
					CommandDiplomacy(i, DiplomacyEnemy, j);
				}
			}
		}
	}
}

/**
**  Man vs Machine
*/
static void GameTypeManVsMachine(void)
{
	int i;
	int j;

	for (i = 0; i < PlayerMax - 1; ++i) {
		if (Players[i].Type != PlayerPerson && Players[i].Type != PlayerComputer) {
			continue;
		}
		for (j = 0; j < PlayerMax - 1; ++j) {
			if (i != j) {
				if (Players[i].Type == Players[j].Type) {
					CommandDiplomacy(i, DiplomacyAllied, j);
					Players[i].SharedVision |= (1 << j);
				} else {
					CommandDiplomacy(i, DiplomacyEnemy, j);
				}
			}
		}
	}
}

/**
**  Man vs Machine whith Humans on a Team
*/
static void GameTypeManTeamVsMachine(void)
{
	int i;
	int j;

	for (i = 0; i < PlayerMax - 1; ++i) {
		if (Players[i].Type != PlayerPerson && Players[i].Type != PlayerComputer) {
			continue;
		}
		for (j = 0; j < PlayerMax - 1; ++j) {
			if (i != j) {
				if (Players[i].Type == Players[j].Type) {
					CommandDiplomacy(i, DiplomacyAllied, j);
					Players[i].SharedVision |= (1 << j);
				} else {
					CommandDiplomacy(i, DiplomacyEnemy, j);
				}
			}
		}
		if (Players[i].Type == PlayerPerson) {
			Players[i].Team = 2;
		} else {
			Players[i].Team = 1;
		}
	}
}

/**
**  CreateGame.
**
**  Load map, graphics, sounds, etc
**
**  @param filename  map filename
**  @param map       map loaded
**
**  @todo FIXME: use in this function InitModules / LoadModules!!!
*/
void CreateGame(const char *filename, CMap *map)
{
	int i;
	int j;

	if (SaveGameLoading) {
		SaveGameLoading = 0;
		// Load game, already created game with Init/LoadModules
		return;
	}

	InitVisionTable(); // build vision table for fog of war
	InitPlayers();
	
	if (!Map.Info.Filename && filename) {
		char path[PATH_MAX];
		
		Assert(filename);
		LibraryFileName(filename, path);
		if(strcasestr(filename, ".smp")) {
			LuaLoadFile(path);
		}
	}

	for (i = 0; i < PlayerMax; ++i) {
		int p;
		int aiopps;

		p = Map.Info.PlayerType[i];
		aiopps = 0;
		// Single player games only:
		// ARI: FIXME: convert to a preset array to share with network game code
		if (GameSettings.Opponents != SettingsPresetMapDefault) {
			if (p == PlayerPerson && ThisPlayer != NULL) {
				p = PlayerComputer;
			}
			if (p == PlayerComputer) {
				if (aiopps < GameSettings.Opponents) {
					++aiopps;
				} else {
					p = PlayerNobody;
				}
			}
		}
		// Network games only:
		if (GameSettings.Presets[i].Type != SettingsPresetMapDefault) {
			p = GameSettings.Presets[i].Type;
		}
		CreatePlayer(p);
	}

	if (filename) {
		// FIXME: LibraryFile here?
		if (CurrentMapPath != filename) {
			//  strcpy is not safe if parameters overlap.
			//  Or at least this is what valgrind says.
			strcpy(CurrentMapPath, filename);
		}

		//
		// Load the map.
		//
		InitUnitTypes(1);
		LoadMap(filename, map);
	}

	GameCycle = 0;
	FastForwardCycle = 0;
	SyncHash = 0;
	InitSyncRand();

	if (IsNetworkGame()) { // Prepare network play
		DebugPrint("Client setup: Calling InitNetwork2\n");
		InitNetwork2();
	} else {
		if (LocalPlayerName && strcmp(LocalPlayerName, "Anonymous")) {
		  ThisPlayer->Name = new_strdup(LocalPlayerName);
		}
	}

	if (GameIntro.Title) {
		ShowIntro(&GameIntro);
	} else {
		CallbackMusicOn();
	}
#if 0
	GamePaused = 1;
#endif

	if (FlagRevealMap) {
		Map.Reveal();
	}

	if (GameSettings.Resources != SettingsResourcesMapDefault) {
		for (j = 0; j < PlayerMax; ++j) {
			if (Players[j].Type == PlayerNobody) {
				continue;
			}
			for (i = 1; i < MaxCosts; ++i) {
				switch (GameSettings.Resources) {
					case SettingsResourcesLow:
						Players[j].Resources[i] = DefaultResourcesLow[i];
						break;
					case SettingsResourcesMedium:
						Players[j].Resources[i] = DefaultResourcesMedium[i];
						break;
					case SettingsResourcesHigh:
						Players[j].Resources[i] = DefaultResourcesHigh[i];
						break;
					default:
						break;
				}
			}
		}
	}

	//
	// Setup game types
	//
	// FIXME: implement more game types
	if (GameSettings.GameType != SettingsGameTypeMapDefault) {
		switch (GameSettings.GameType) {
			case SettingsGameTypeMelee:
				break;
			case SettingsGameTypeFreeForAll:
				GameTypeFreeForAll();
				break;
			case SettingsGameTypeTopVsBottom:
				GameTypeTopVsBottom();
				break;
			case SettingsGameTypeLeftVsRight:
				GameTypeLeftVsRight();
				break;
			case SettingsGameTypeManVsMachine:
				GameTypeManVsMachine();
				break;
			case SettingsGameTypeManTeamVsMachine:
				GameTypeManTeamVsMachine();

			// Future game type ideas
#if 0
			case SettingsGameTypeOneOnOne:
				break;
			case SettingsGameTypeCaptureTheFlag:
				break;
			case SettingsGameTypeGreed:
				break;
			case SettingsGameTypeSlaughter:
				break;
			case SettingsGameTypeSuddenDeath:
				break;
			case SettingsGameTypeTeamMelee:
				break;
			case SettingsGameTypeTeamCaptureTheFlag:
				break;
#endif
		}
	}

	//
	// Graphic part
	//
	SetPlayersPalette();
	InitIcons();
	LoadIcons();

	// FIXME: Race only known in single player game:
	InitMenus(ThisPlayer->Race);
	LoadCursors(PlayerRaces.Name[ThisPlayer->Race]);

	InitMissileTypes();
#ifndef DYNAMIC_LOAD
	LoadMissileSprites();
#endif
	InitConstructions();
	LoadConstructions();
	LoadUnitTypes();
	LoadDecorations();

	InitSelections();

	UI.Minimap.Create();   // create minimap for pud
	Map.Init();
	PreprocessMap();   // Adjust map for use

	InitUserInterface(PlayerRaces.Name[ThisPlayer->Race]); // Setup the user interface
	UI.Load(); // Load the user interface grafics

	//
	// Sound part
	//
	LoadUnitSounds();
	MapUnitSounds();
	if (SoundEnabled()) {
		if (InitSoundServer()) {
			SetEffectsEnabled(false);
			SetMusicEnabled(false);
		} else {
			InitSoundClient();
		}
	}

	//
	// Spells
	//
	InitSpells();

	//
	// Init units' groups
	//
	InitGroups();

	//
	// Init players?
	//
	DebugPlayers();
	PlayersInitAi();

	//
	// Upgrades
	//
	InitUpgrades();

	//
	// Dependencies
	//
	InitDependencies();

	//
	// Buttons (botpanel)
	//
	InitButtons();

	//
	// Triggers
	//
	InitTriggers();

	SetDefaultTextColors(UI.NormalFontColor, UI.ReverseFontColor);

#if 0
	if (!UI.SelectedViewport) {
		UI.SelectedViewport = UI.Viewports;
	}
#endif
	UI.SelectedViewport->Center(
		ThisPlayer->StartX, ThisPlayer->StartY, TileSizeX / 2, TileSizeY / 2);

	//
	// Various hacks wich must be done after the map is loaded.
	//
	// FIXME: must be done after map is loaded
	InitAStar();
	//
	// FIXME: The palette is loaded after the units are created.
	// FIXME: This loops fixes the colors of the units.
	//
	for (i = 0; i < NumUnits; ++i) {
		// I don't really think that there can be any rescued
		// units at this point.
		if (Units[i]->RescuedFrom) {
			Units[i]->Colors = &Units[i]->RescuedFrom->UnitColors;
		} else {
			Units[i]->Colors = &Units[i]->Player->UnitColors;
		}
	}

	GameResult = GameNoResult;

	CommandLog(NULL, NoUnitP, FlushCommands, -1, -1, NoUnitP, NULL, -1);
	Video.ClearScreen();
}

/**
**  Init Game Setting to default values
**
**  @todo  FIXME: this should not be executed for restart levels!
*/
void InitSettings(void)
{
	int i;

	if (RestartScenario) {
		Map.NoFogOfWar = GameSettings.NoFogOfWar;
		FlagRevealMap = GameSettings.RevealMap;
		return;
	}

	for (i = 0; i < PlayerMax; ++i) {
		GameSettings.Presets[i].Race = SettingsPresetMapDefault;
		GameSettings.Presets[i].Team = SettingsPresetMapDefault;
		GameSettings.Presets[i].Type = SettingsPresetMapDefault;
	}
	GameSettings.Resources = SettingsResourcesMapDefault;
	GameSettings.NumUnits = SettingsNumUnitsMapDefault;
	GameSettings.Opponents = SettingsPresetMapDefault;
	GameSettings.Terrain = SettingsPresetMapDefault;
	GameSettings.GameType = SettingsPresetMapDefault;
	GameSettings.NetGameType = SettingsSinglePlayerGame;
}

//@}
