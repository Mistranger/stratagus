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
/**@name loadgame.c	-	Load game. */
//
//	(c) Copyright 2001-2003 by Lutz Sammer, Andreas Arens
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

#include "stratagus.h"
#include "icons.h"
#include "cursor.h"
#include "construct.h"
#include "unittype.h"
#include "upgrade.h"
#include "depend.h"
#include "interface.h"
#include "missile.h"
#include "tileset.h"
#include "map.h"
#include "ccl.h"
#include "ui.h"
#include "ai.h"
#include "campaign.h"
#include "trigger.h"
#include "actions.h"
#include "minimap.h"
#include "commands.h"
#include "sound_server.h"
#include "font.h"
#include "menus.h"
#include "pathfinder.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Cleanup modules.
**
**	Call each module to clean up.
*/
global void CleanModules(void)
{
    SCM var;

    EndReplayLog();
    CleanMessages();

    CleanIcons();
    CleanCursors();
    // CleanMenus();
    CleanUserInterface();
    CleanCampaign();
    CleanTriggers();
    CleanAi();
    CleanPlayers();
    CleanConstructions();
    CleanDecorations();
    CleanUnitTypes();
    CleanUnits();
    CleanSelections();
    CleanGroups();
    CleanUpgrades();
    CleanDependencies();
    CleanButtons();
    CleanMissileTypes();
    CleanMissiles();
    CleanTilesets();
    CleanMap();
    CleanReplayLog();
    CleanCclCredits();
    CleanSpells();
    FreeVisionTable();
#ifdef HIERARCHIC_PATHFINDER
    PfHierClean ();
#endif
    FreeAStar();

    //
    //	Free our protected objects, AI scripts, unit-type properties.
    //
    var = gh_symbol2scm("*ccl-protect*");
    setvar(var, NIL, NIL);
}

/**
**	Initialize all modules.
**
**	Call each module to initialize.
*/
global void InitModules(void)
{
    GameCycle = 0;
    FastForwardCycle = 0;
    SyncHash = 0;

    CallbackMusicOn();
    InitSyncRand();
    InitIcons();
    InitVideoCursors();
    InitUserInterface(ThisPlayer->RaceName);
    InitMenus(ThisPlayer->Race);
    InitPlayers();
    InitMissileTypes();
    InitMissiles();
    InitConstructions();
    // InitDecorations();

    // LUDO : 0 = don't reset player stats ( units level , upgrades, ... ) !
    InitUnitTypes(0);

    InitUnits();
    InitSelections();
    InitGroups();
    InitSpells();
    InitUpgrades();
    InitDependencies();

    InitButtons();

    InitAiModule();

#ifdef HIERARCHIC_PATHFINDER
    PfHierInitialize();
#endif
    InitMap();
    InitMapFogOfWar();			// build tables for fog of war
}

/**
**	Load all.
**
**	Call each module to load additional files (graphics,sounds).
*/
global void LoadModules(void)
{
    char* s;

    LoadIcons();
    LoadCursors(ThisPlayer->RaceName);
    LoadUserInterface();
    // LoadPlayers();
    LoadMissileSprites();
    LoadConstructions();
    LoadDecorations();
    LoadUnitTypes();

    LoadUnitSounds();
    MapUnitSounds();
    InitAStar();
#ifdef WITH_SOUND
    if (SoundFildes != -1) {
	//FIXME: must be done after map is loaded
	if (InitSoundServer()) {
	    SoundOff = 1;
	    SoundFildes = -1;
	} else {
	    // must be done after sounds are loaded
	    InitSoundClient();
	}
    }
#endif

    LoadRGB(GlobalPalette,
	    s = strdcat3(StratagusLibPath, "/graphics/",
		TheMap.Tileset->PaletteFile));
    free(s);
    VideoCreatePalette(GlobalPalette);
    CreateMinimap();

    SetDefaultTextColors(TheUI.NormalFontColor, TheUI.ReverseFontColor);

    // LoadButtons();
}

/**
**	Load a game to file.
**
**	@param filename	File name to be loaded.
**
**	@note	Later we want to store in a more compact binary format.
*/
global void LoadGame(char* filename)
{
    int old_siod_verbose_level;
    unsigned long game_cycle;

    CleanModules();

    old_siod_verbose_level = siod_verbose_level;
    siod_verbose_level = 4;
    CclGarbageCollect(0);
    siod_verbose_level = old_siod_verbose_level;
    InitVisionTable();
    gh_load(filename);
    CclGarbageCollect(0);

    game_cycle = GameCycle;
    // FIXME: log should be loaded from the save game
    CommandLogDisabled = 1;

    InitModules();
    LoadModules();

    GameCycle = game_cycle;
    SelectionChanged();
    MustRedraw = RedrawEverything;
}

/**
**	Load all game data.
**
**	Test function for the later load/save functions.
*/
global void LoadAll(void)
{
#if 1
    SaveGame("save_file_of_stratagus0.ccl");
    LoadGame("save_file_of_stratagus0.ccl");
    SaveGame("save_file_of_stratagus1.ccl");
    LoadGame("save_file_of_stratagus1.ccl");
    SaveGame("save_file_of_stratagus2.ccl");
    LoadGame("save_file_of_stratagus2.ccl");
#endif
    //LoadGame ("save_file_of_stratagus.ccl");
}

//@}
