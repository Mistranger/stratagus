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
/**@name game.c		-	The game set-up and creation. */
//
//	(c) Copyright 1998-2003 by Lutz Sammer, Andreas Arens, and
//	                           Jimmy Salmon
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

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"
#include "map.h"
#include "minimap.h"
#include "player.h"
#include "unit.h"
#include "pathfinder.h"
#include "pud.h"
#include "ui.h"
#include "font.h"
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

#include "ccl.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

global Settings GameSettings;		/// Game Settings
global int lcm_prevent_recurse;		/// prevent recursion through LoadGameMap

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Map loading/saving
----------------------------------------------------------------------------*/

/**
**	Load a Stratagus map.
**
**	@param filename	map filename
**	@param map	map loaded
*/
local void LoadStratagusMap(const char* filename,
	WorldMap* map __attribute__((unused)))
{
    DebugLevel3Fn("%p \n" _C_ map);

    if (lcm_prevent_recurse) {
	fprintf(stderr,"recursive use of load Stratagus map!\n");
	ExitFatal(-1);
    }
    InitPlayers();
    lcm_prevent_recurse = 1;
    gh_load((char*)filename);
    lcm_prevent_recurse = 0;

#if 0
    // Not true if multiplayer levels!
    if (!ThisPlayer) {		/// ARI: bomb if nothing was loaded!
	fprintf(stderr,"%s: invalid Stratagus map\n", filename);
	ExitFatal(-1);
    }
    // FIXME: Retrieve map->Info from somewhere... If LoadPud is used in CCL it magically is set there :)
#endif
    if( !TheMap.Width || !TheMap.Height ) {
	fprintf(stderr,"%s: invalid Stratagus map\n", filename);
	ExitFatal(-1);
    }
}

/**
**	Load any map.
**
**	@param filename	map filename
**	@param map	map loaded
*/
local void LoadMap(const char* filename,WorldMap* map)
{
    const char* tmp;

    tmp=strrchr(filename,'.');
    if( tmp ) {
#ifdef USE_ZLIB
	if( !strcmp(tmp,".gz") ) {
	    while( tmp-1>filename && *--tmp!='.' ) {
	    }
	} else
#endif
#ifdef USE_BZ2LIB
	if( !strcmp(tmp,".bz2") ) {
	    while( tmp-1>filename && *--tmp!='.' ) {
	    }
	}
#endif
	if( !strcmp(tmp,".cm")
#ifdef USE_ZLIB
		|| !strcmp(tmp,".cm.gz")
#endif
#ifdef USE_BZ2LIB
		|| !strcmp(tmp,".cm.bz2")
#endif
	) {
	    LoadStratagusMap(filename,map);
	    return;
	}
    }
    // ARI: This bombs out, if no pud, so will be safe.
    if( strcasestr(filename,".pud") ) {
	LoadPud(filename,map);
    } else if( strcasestr(filename,".scm") ) {
	LoadScm(filename,map);
    } else if( strcasestr(filename,".chk") ) {
	LoadChk(filename,map);
    }
}

/*----------------------------------------------------------------------------
--	Game creation
----------------------------------------------------------------------------*/

/**
**	Free for all
*/
local void GameTypeFreeForAll(void)
{
    int i;
    int j;

    for (i=0; i<15; i++) {
	for (j=0; j<15; j++) {
	    if (i != j) {
		CommandDiplomacy(i,DiplomacyEnemy,j);
	    }
	}
    }
}

/**
**	Top vs Bottom
*/
local void GameTypeTopVsBottom(void)
{
    int i;
    int j;
    int top;
    int middle;

    middle = TheMap.Height/2;
    for (i=0; i<15; i++) {
	top = Players[i].StartY <= middle;
	for (j=0; j<15; j++) {
	    if (i != j) {
		if ((top && Players[j].StartY <= middle) ||
			(!top && Players[j].StartY > middle)) {
		    CommandDiplomacy(i,DiplomacyAllied,j);
		    Players[i].SharedVision |= (1<<j);
		} else {
		    CommandDiplomacy(i,DiplomacyEnemy,j);
		}
	    }
	}
    }
}

/**
**	Left vs Right
*/
local void GameTypeLeftVsRight(void)
{
    int i;
    int j;
    int left;
    int middle;

    middle = TheMap.Width/2;
    for (i=0; i<15; i++) {
	left = Players[i].StartX <= middle;
	for (j=0; j<15; j++) {
	    if (i != j) {
		if ((left && Players[j].StartX <= middle) ||
			(!left && Players[j].StartX > middle)) {
		    CommandDiplomacy(i,DiplomacyAllied,j);
		    Players[i].SharedVision |= (1<<j);
		} else {
		    CommandDiplomacy(i,DiplomacyEnemy,j);
		}
	    }
	}
    }
}

/**
**	Man vs Machine
*/
local void GameTypeManVsMachine(void)
{
    int i;
    int j;

    for (i=0; i<15; i++) {
	if (Players[i].Type!=PlayerPerson && Players[i].Type!=PlayerComputer ) {
	    continue;
	}
	for (j=0; j<15; j++) {
	    if (i != j) {
		if (Players[i].Type==Players[j].Type) {
		    CommandDiplomacy(i,DiplomacyAllied,j);
		    Players[i].SharedVision |= (1<<j);
		} else {
		    CommandDiplomacy(i,DiplomacyEnemy,j);
		}
	    }
	}
    }
}

/**
**	CreateGame.
**
**	Load map, graphics, sounds, etc
**
**	@param filename	map filename
**	@param map	map loaded
**
**	@todo 	FIXME: use in this function InitModules / LoadModules!!!
*/
global void CreateGame(char* filename, WorldMap* map)
{
    int i;
    int j;
    char* s;

    if( filename && !*filename ) {
	// Load game, already created game with Init/LoadModules
	return;
    }

    InitVisionTable();			// build vision table for fog of war
    
    if( filename ) {
	s = NULL;
	// FIXME: LibraryFile here?
	strcpy(CurrentMapPath, filename);
	if (filename[0] != '/' && filename[0] != '.') {
	    s = filename = strdcat3(StratagusLibPath, "/", filename);
	}

	//
	//	Load the map.
	//
	InitUnitTypes(1);
	LoadMap(filename, map);

	if (s) {
	    free(s);
	}
    }

    GameCycle = 0;
    FastForwardCycle = 0;
    SyncHash = 0;
    InitSyncRand();

    if( NetworkFildes!=-1 ) {		// Prepare network play
	DebugLevel0Fn("Client setup: Calling InitNetwork2\n");
	InitNetwork2();
    } else {
	if( LocalPlayerName && strcmp(LocalPlayerName,"Anonymous") ) {
          ThisPlayer->Name = strdup(LocalPlayerName);
	}
    }

    if( GameIntro.Title ) {
	ShowIntro(&GameIntro);
    } else {
	CallbackMusicOn();
    }
    //GamePaused=1;

    if( FlagRevealMap ) {
	RevealMap();
    }

    if (GameSettings.Resources != SettingsResourcesMapDefault) {
	for (j = 0; j < PlayerMax; ++j) {
	    if (Players[j].Type == PlayerNobody) {
		continue;
	    }
	    for (i = 1; i < MaxCosts; ++i) {
		switch (GameSettings.Resources) {
		    case SettingsResourcesLow:
			Players[j].Resources[i]=DefaultResourcesLow[i];
			break;
		    case SettingsResourcesMedium:
			Players[j].Resources[i]=DefaultResourcesMedium[i];
			break;
		    case SettingsResourcesHigh:
			Players[j].Resources[i]=DefaultResourcesHigh[i];
			break;
		    default:
			break;
		}
	    }
	}
    }

    //
    //	Setup game types
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
    //	Graphic part
    //
    LoadRGB(GlobalPalette,
	    s=strdcat3(StratagusLibPath,"/graphics/",
		TheMap.Tileset->PaletteFile));
    free(s);
    VideoCreatePalette(GlobalPalette);
    InitIcons();
    LoadIcons();

    // FIXME: Race only known in single player game:
    InitMenus(ThisPlayer->Race);
    LoadCursors(ThisPlayer->RaceName);

    InitMissileTypes();
    LoadMissileSprites();
    InitConstructions();
    LoadConstructions();
    LoadUnitTypes();
    LoadDecorations();

    IfDebug(
	DebugLevel0("Graphics uses %d bytes (%d KB, %d MB)\n"
		_C_ AllocatedGraphicMemory
		_C_ AllocatedGraphicMemory/1024
		_C_ AllocatedGraphicMemory/1024/1024);
	DebugLevel0("Compressed graphics uses %d bytes (%d KB, %d MB)\n"
		_C_ CompressedGraphicMemory
		_C_ CompressedGraphicMemory/1024
		_C_ CompressedGraphicMemory/1024/1024);
    );

    CreateMinimap();			// create minimap for pud
    InitMap();				// setup draw functions
    InitMapFogOfWar();			// build tables for fog of war
    PreprocessMap();			// Adjust map for use
    MapColorCycle();			// Setup color cycle

    InitUserInterface(ThisPlayer->RaceName);	// Setup the user interface
    LoadUserInterface();		// Load the user interface grafics

    //
    //	Sound part
    //
    //FIXME: check if everything is really loaded
    LoadUnitSounds();
    MapUnitSounds();

#ifdef WITH_SOUND
    IfDebug(
	DebugLevel0("Sounds uses %d bytes (%d KB, %d MB)\n"
		_C_ AllocatedSoundMemory
		_C_ AllocatedSoundMemory/1024
		_C_ AllocatedSoundMemory/1024/1024);
	DebugLevel0("Compressed sounds uses %d bytes (%d KB, %d MB)\n"
		_C_ CompressedSoundMemory
		_C_ CompressedSoundMemory/1024
		_C_ CompressedSoundMemory/1024/1024);
    );
#endif

    //
    //	Spells
    //
    InitSpells();

    //
    //  Init units' groups
    //
    InitGroups();

    //
    //	Init players?
    //
    DebugPlayers();
    PlayersInitAi();

    //
    //  Upgrades
    //
    InitUpgrades();

    //
    //  Dependencies
    //
    InitDependencies();

    //
    //  Buttons (botpanel)
    //
    InitButtons();

    //
    //	Triggers
    //
    InitTriggers();

#ifdef WITH_SOUND
    if (SoundFildes!=-1) {
	//FIXME: must be done after map is loaded
	if ( InitSoundServer() ) {
	    SoundOff=1;
	    SoundFildes=-1;
	} else {
	    // must be done after sounds are loaded
	    InitSoundClient();
	}
    }
#endif

    SetDefaultTextColors(TheUI.NormalFontColor,TheUI.ReverseFontColor);

#if 0
    if (!TheUI.SelectedViewport) {
	TheUI.SelectedViewport = TheUI.Viewports;
    }
#endif
    ViewportCenterViewpoint(TheUI.SelectedViewport,
	    ThisPlayer->StartX,ThisPlayer->StartY);

    //
    //	Various hacks wich must be done after the map is loaded.
    //
    //FIXME: must be done after map is loaded
    InitAStar();
#ifdef HIERARCHIC_PATHFINDER
    PfHierInitialize();
#endif // HIERARCHIC_PATHFINDER

    //
    //	FIXME: The palette is loaded after the units are created.
    //	FIXME: This loops fixes the colors of the units.
    //
    for( i=0; i<NumUnits; ++i ) {
        //  I don't really think that there can be any rescued
        //  units at this point.
        if (Units[i]->RescuedFrom) { 
            Units[i]->Colors=&Units[i]->RescuedFrom->UnitColors;
        } else {
            Units[i]->Colors=&Units[i]->Player->UnitColors;
        }
    }

    GameResult=GameNoResult;

    CommandLog(NULL,NoUnitP,FlushCommands,-1,-1,NoUnitP,NULL,-1);
    DestroyCursorBackground();
    VideoLockScreen();
    VideoClearScreen();
    VideoUnlockScreen();
}

/**
**	Init Game Setting to default values
**
**	@todo FIXME: this should not be executed for restart levels!
*/
global void InitSettings(void)
{
    int i;

    if (RestartScenario) {
	TheMap.NoFogOfWar = GameSettings.NoFogOfWar;
	FlagRevealMap = GameSettings.RevealMap;
	return;
    }

    for (i = 0; i < PlayerMax; i++) {
	GameSettings.Presets[i].Race = SettingsPresetMapDefault;
	GameSettings.Presets[i].Team = SettingsPresetMapDefault;
	GameSettings.Presets[i].Type = SettingsPresetMapDefault;
    }
    GameSettings.Resources = SettingsResourcesMapDefault;
    GameSettings.NumUnits = SettingsNumUnitsMapDefault;
    GameSettings.Opponents = SettingsPresetMapDefault;
    GameSettings.Terrain = SettingsPresetMapDefault;
    GameSettings.GameType = SettingsPresetMapDefault;
}

//@}
