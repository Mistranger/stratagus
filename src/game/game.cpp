//   ___________		     _________		      _____  __
//   \_	  _____/______   ____   ____ \_   ___ \____________ _/ ____\/  |_
//    |    __) \_  __ \_/ __ \_/ __ \/    \  \/\_  __ \__  \\   __\\   __\ 
//    |     \   |  | \/\  ___/\  ___/\     \____|  | \// __ \|  |   |  |
//    \___  /   |__|    \___  >\___  >\______  /|__|  (____  /__|   |__|
//	  \/		    \/	   \/	     \/		   \/
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   FreeCraft - A free fantasy real time strategy game engine
//
/**@name game.c		-	The game set-up and creation. */
//
//	(c) Copyright 1998-2002 by Lutz Sammer, Andreas Arens
//
//	FreeCraft is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation; only version 2 of the License.
//
//	FreeCraft is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	$Id$

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freecraft.h"
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
#include "network.h"
#include "netconnect.h"
#include "missile.h"
#include "settings.h"
#include "campaign.h"
#include "trigger.h"

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
**	Load a FreeCraft map.
**
**	@param filename	map filename
**	@param map	map loaded
*/
local void LoadFreeCraftMap(const char* filename,
	WorldMap* map __attribute__((unused)))
{
    DebugLevel3Fn("%p \n",map);

    if (lcm_prevent_recurse) {
	fprintf(stderr,"recursive use of load clone map!\n");
	exit(-1);
    }
    lcm_prevent_recurse = 1;
    gh_eval_file((char*)filename);
    lcm_prevent_recurse = 0;
    if (!ThisPlayer) {		/// ARI: bomb if nothing was loaded!
	fprintf(stderr,"%s: invalid clone map\n", filename);
	exit(-1);
    }
    // FIXME: Retrieve map->Info from somewhere... If LoadPud is used in CCL it magically is set there :)
}

/**
**	Load any map.
**
**	@param filename	map filename
**	@param map	map loaded
*/
global void LoadMap(const char* filename,WorldMap* map)
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
	    LoadFreeCraftMap(filename,map);
	    return;
	}
    }
    // ARI: This bombs out, if no pud, so will be safe.
    LoadPud(filename,map);
}

/*----------------------------------------------------------------------------
--	Game creation
----------------------------------------------------------------------------*/

/**
**	CreateGame.
**
**	Load map, graphics, sounds, etc
**
**	@param filename	map filename
**	@param map	map loaded
*/
global void CreateGame(char* filename, WorldMap* map)
{
    int i, j;
    char* s;

    if ( filename ) {
	s = NULL;
	// FIXME: LibraryFile here?
	if (filename[0] != '/' && filename[0] != '.') {
	    s = filename = strdcat3(FreeCraftLibPath, "/", filename);
	}
	//
	//	Load the map.
	//
	InitUnitTypes();
	UpdateStats();
	LoadMap(filename, map);

	if (s) {
	    free(s);
	}
	
	//
	//	Network by command line
	//
	if( NetworkFildes!=-1 ) {
	    if( NetPlayers>1 || NetworkArg ) {
		//
		//	Server
		//
		if (NetPlayers > 1) {
		    NetworkServerSetup(map);
		    DebugLevel0Fn("Server setup ready\n");
		    InitNetwork2();
		//
		// Client
		//
		} else if (NetworkArg) {
		    NetworkClientSetup(map);
		    DebugLevel0Fn("Client setup ready\n");
		    InitNetwork2();
		}
	    } else {
		ExitNetwork1();
	    }
	}
    }

    if( GameIntro.Title ) {
	ShowIntro(&GameIntro);
    }

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
			Players[j].Resources[i]=DEFAULT_RESOURCES_LOW[i];
			break;
		    case SettingsResourcesMedium:
			Players[j].Resources[i]=DEFAULT_RESOURCES_MEDIUM[i];
			break;
		    case SettingsResourcesHigh:
			Players[j].Resources[i]=DEFAULT_RESOURCES_HIGH[i];
			break;
		    default:
			break;
		}
	    }
	}
    }

    //
    //	Graphic part
    //
    LoadTileset();
    LoadRGB(GlobalPalette,
	    s=strdcat3(FreeCraftLibPath,"/graphics/",
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
		,AllocatedGraphicMemory
		,AllocatedGraphicMemory/1024
		,AllocatedGraphicMemory/1024/1024);
	DebugLevel0("Compressed graphics uses %d bytes (%d KB, %d MB)\n"
		,CompressedGraphicMemory
		,CompressedGraphicMemory/1024
		,CompressedGraphicMemory/1024/1024);
    );

    CreateMinimap();			// create minimap for pud
    InitMap();				// setup draw functions
    InitMapFogOfWar();			// build tables for fog of war
    PreprocessMap();			// Adjust map for use
    MapColorCycle();			// Setup color cycle

    CleanUserInterface();
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
		,AllocatedSoundMemory
		,AllocatedSoundMemory/1024
		,AllocatedSoundMemory/1024/1024);
	DebugLevel0("Compressed sounds uses %d bytes (%d KB, %d MB)\n"
		,CompressedSoundMemory
		,CompressedSoundMemory/1024
		,CompressedSoundMemory/1024/1024);
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

    // FIXME: make this configurable over GUI config.
    if( ThisPlayer->Race==PlayerRaceHuman ) {
	SetDefaultTextColors(FontWhite,FontYellow);
	// FIXME: Add this again:
	//	PlaySound(SoundBasicHumanVoicesSelected1);
    } else if( ThisPlayer->Race==PlayerRaceOrc ) {
	SetDefaultTextColors(FontYellow,FontWhite);
	// FIXME: Add this again:
	//	PlaySound(SoundBasicOrcVoicesSelected1);
    }
    // FIXME: support more races

    MapCenter(ThisPlayer->X,ThisPlayer->Y);

    //FIXME: must be done after map is loaded
    if(AStarOn) {
	InitAStar();
    }
#ifdef HIERARCHIC_PATHFINDER
    PfHierInitialize ();
#endif /* HIERARCHIC_PATHFINDER */

    GameResult=GameNoResult;
}

/**
**	Init Game Setting to default values
*/
global void InitSettings(void)
{
    int i;

    for (i = 0; i < PlayerMax; i++) {
	GameSettings.Presets[i].Race = SettingsPresetMapDefault;
	GameSettings.Presets[i].Team = SettingsPresetMapDefault;
    }
    GameSettings.Resources = SettingsResourcesMapDefault;
    GameSettings.NumUnits = SettingsNumUnitsMapDefault;
    GameSettings.Opponents = SettingsPresetMapDefault;
    GameSettings.Terrain = SettingsPresetMapDefault;
}

//@}
