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
/**@name clone.c	-	The main file. */
/*
**	(c) Copyright 1998-2000 by Lutz Sammer
**
**	$Id$
*/

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#ifdef __CYGWIN__
#include <getopt.h>
#endif

#ifdef __MINGW32__
#include <SDL/SDL.h>
extern int opterr;
extern int optind;
extern int optopt;
extern char *optarg;

extern int getopt(int argc, char *const*argv, const char *opt);
#endif

#include "freecraft.h"

#include "video.h"
#include "image.h"
#include "tileset.h"
#include "map.h"
#include "minimap.h"
#include "sound_id.h"
#include "unitsound.h"
#include "icons.h"
#include "button.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "missile.h"
#include "actions.h"
#include "construct.h"
#include "ai.h"
#include "ccl.h"
#include "cursor.h"
#include "upgrade.h"
#include "depend.h"
#include "font.h"
#include "interface.h"
#include "ui.h"
#include "menus.h"
#include "sound_server.h"
#include "sound.h"
#include "network.h"
#include "pathfinder.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

#ifdef DEBUG
extern unsigned AllocatedGraphicMemory;
extern unsigned CompressedGraphicMemory;
#endif

/*----------------------------------------------------------------------------
--	Speedups
----------------------------------------------------------------------------*/

global int SpeedMine=SPEED_MINE;	/// speed factor for mine gold
global int SpeedGold=SPEED_GOLD;	/// speed factor for getting gold
global int SpeedChop=SPEED_CHOP;	/// speed factor for chop
global int SpeedWood=SPEED_WOOD;	/// speed factor for getting wood
global int SpeedHaul=SPEED_HAUL;	/// speed factor for haul oil
global int SpeedOil=SPEED_OIL;		/// speed factor for getting oil
global int SpeedBuild=SPEED_BUILD;	/// speed factor for building
global int SpeedTrain=SPEED_TRAIN;	/// speed factor for training
global int SpeedUpgrade=SPEED_UPGRADE;	/// speed factor for upgrading
global int SpeedResearch=SPEED_RESEARCH;/// speed factor for researching

/*--------------------------------------------------------------
	Scroll Speeds
--------------------------------------------------------------*/

global int SpeedKeyScroll=KEY_SCROLL_SPEED;
global int SpeedMouseScroll=MOUSE_SCROLL_SPEED;

/*============================================================================
==	DISPLAY
============================================================================*/

// FIXME: move to video header file
global int VideoWidth;			/// window width in pixels
global int VideoHeight;			/// window height in pixels

global int FrameCounter;		/// current frame number
global int SlowFrameCounter;		/// profile, frames out of sync

// FIXME: not the correct place
global enum MustRedraw_e MustRedraw=RedrawEverything;	/// redraw flags

/*----------------------------------------------------------------------------
--	SAVE/LOAD
----------------------------------------------------------------------------*/

/**
**	Test function for the later save functions.
*/
global void SaveAll(void)
{
    FILE* file;
    time_t now;
    const char* name;

    name="save_file_of_freecraft.ccl";
    // FIXME: must choose better name.
    file=fopen(name,"wb");
    if( !file ) {
	fprintf(stderr,"Can't save to `%s'\n",name);
	return;
    }
    fprintf(file,";;; -----------------------------------------\n");
    fprintf(file,";;; Save file generated by FreeCraft Version " VERSION "\n");
    time(&now);
    fprintf(file,";;;\tDate: %s",ctime(&now));
    fprintf(file,";;;\tMap: %s\n",TheMap.Description);
    fprintf(file,";;; -----------------------------------------\n\n\n");

    SaveUnitTypes(file);
    SaveUnits(file);
    SaveMap(file);
    SaveUpgrades(file);
    SaveDependencies(file);
    // FIXME: find all state information which must be saved.

    fclose(file);
}

/*----------------------------------------------------------------------------
--	INIT
----------------------------------------------------------------------------*/

/**
**	Initialise.
**
**		Load all graphics, sounds.
*/
global void FreeCraftInit(void)
{
    char* PalettePath;

    DebugLevel3("Terrain %d\n",TheMap.Terrain);

    // FIXME: must use palette from tileset!!
    // FIXME: this must be extendable!!

    switch( TheMap.Terrain ) {
	case TilesetSummer:
            PalettePath = strdcat(FreeCraftLibPath, "/summer.rgb");
	    break;
	case TilesetWinter:
            PalettePath = strdcat(FreeCraftLibPath, "/winter.rgb");
	    break;
	case TilesetWasteland:
            PalettePath = strdcat(FreeCraftLibPath, "/wasteland.rgb");
	    break;
	case TilesetSwamp:
            PalettePath = strdcat(FreeCraftLibPath, "/swamp.rgb");
	    break;
	default:
	    DebugLevel2("Unknown Terrain %d\n",TheMap.Terrain);
            PalettePath = strdcat(FreeCraftLibPath, "/summer.rgb");
	    break;
    }
    LoadRGB(GlobalPalette, PalettePath);

    VideoCreatePalette(GlobalPalette);

    //
    //	Graphic part
    //
    LoadIcons();
    InitMenus(ThisPlayer->Race);
    LoadImages(ThisPlayer->Race);
    LoadCursors(ThisPlayer->Race);
    LoadTileset();
    InitUnitButtons();
    LoadMissileSprites();
    LoadUnitSprites();
    LoadConstructions();
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

    // FIXME: johns: did this belong into LoadMap?
    CreateMinimap();			// create minimap for pud
    InitMap();				// setup draw functions
    InitMapFogOfWar();			// build tables for fog of war
    PreprocessMap();			// Adjust map for use
    MapColorCycle();			// Setup color cycle

    InitUserInterface();		// Setup the user interface.

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
    //	Network part
    //
    InitNetwork();

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

    MapCenter(ThisPlayer->X,ThisPlayer->Y);
}

/*----------------------------------------------------------------------------
--	Random
----------------------------------------------------------------------------*/

/**
**	Syncron rand.
*/
global int SyncRand(void)
{
    static unsigned rnd = 0x87654321;
    int val;

    val=rnd>>16;

    rnd=rnd*(0x12345678 * 4 + 1) + 1;

    return val;
}

/*----------------------------------------------------------------------------
--	Utility
----------------------------------------------------------------------------*/

/**
**	String duplicate/concatenate (two arguments)
*/
global char *strdcat(const char *l, const char *r) {
    char *res = malloc(strlen(l) + strlen(r) + 1);
    if (res) {
        strcpy(res, l);
        strcat(res, r);
    }
    return res;
}

/**
**	String duplicate/concatenate (three arguments)
*/
global char* strdcat3(const char* l, const char* m, const char* r) {
    char* res = malloc(strlen(l) + strlen(m) + strlen(r) + 1);
    if (res) {
        strcpy(res, l);
	strcat(res, m);
        strcat(res, r);
    }
    return res;
}

/*============================================================================
==	MAIN
============================================================================*/

global char* TitleScreen;		/// Titlescreen show by startup
global char* FreeCraftLibPath;		/// Path for data
global int FlagRevealMap;		/// Flag must reveal the map


local const char* MapName;		/// Filename for the map to load

    /// Name, Version, Copyright
local char NameLine[] =
    "FreeCraft V" VERSION ", (c) 1998-2000 by The FreeCraft Project.";

/**
**	Main, called from guile main.
**
**	@param	argc	Number of arguments.
**	@param	argv	Vector of arguments.
*/
global int main1(int argc __attribute__ ((unused)),
	char** argv __attribute__ ((unused)))
{
    InitVideo();			// setup video display
#ifdef WITH_SOUND
    if( InitSound() ) {			// setup sound card
	SoundOff=1;
	SoundFildes=-1;
    }
#endif

    //
    //	Show title screen.
    //
    SetClipping(0,0,VideoWidth,VideoHeight);
    if( TitleScreen ) {
	DisplayPicture(TitleScreen);
    }
    Invalidate();

    //
    //  Units Memory Management
    //
    InitUnitsMemory();
    UpdateStats();
    InitUnitTypes();

    // 
    //  Inital menues require some gfx..
    //
    LoadRGB(GlobalPalette, strdcat(FreeCraftLibPath, "/summer.rgb"));
    VideoCreatePalette(GlobalPalette);
    LoadFonts();

    // All pre-start menues are orcish - may need to be switched later..
    SetDefaultTextColors(FontYellow,FontWhite);
    InitMenus(1);
    LoadImages(1);
    LoadCursors(1);

    //
    //	Load the map.
    //
    LoadMap(MapName,&TheMap);
    if( FlagRevealMap ) {
	RevealMap();
    }

    FreeCraftInit();
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
    //FIXME: must be done after map is loaded
    if(AStarOn) {
	InitAStar();
    }

    SetStatusLine(NameLine);
    SetMessage("Do it! Do it now!");

    if( ThisPlayer->Race==PlayerRaceHuman ) {
	SetDefaultTextColors(FontWhite,FontYellow);
	// FIXME: Add this again:
	//	PlaySound(SoundBasicHumanVoicesSelected1);
    } else if( ThisPlayer->Race==PlayerRaceOrc ) {
	SetDefaultTextColors(FontYellow,FontWhite);
	// FIXME: Add this again:
	//	PlaySound(SoundBasicOrcVoicesSelected1);
    }
    // FIXME: more races supported

    printf("%s\n  written by Lutz Sammer, Fabrice Rossi, Vladi Shabanski, Patrice Fortier,\n  Jon Gabrielson and others."
#ifdef USE_CCL2
    " SIOD Copyright by George J. Carrette."
#endif
    "\nCompile options "
#ifdef USE_CCL
    "CCL "
#endif
#ifdef USE_CCL2
    "CCL2 "
#endif
#ifdef GUILE_GTK
    "GTK "
#endif
#ifdef USE_THREAD
    "THREAD "
#endif
#ifdef DEBUG
    "DEBUG "
#endif
#ifdef USE_ZLIB
    "ZLIB "
#endif
#ifdef USE_BZ2LIB
    "BZ2LIB "
#endif
#ifdef USE_GLIB
    "GLIB "
#endif
#ifdef USE_SDLA
    "SDL-AUDIO "
#endif
#ifdef USE_SDL
    "SDL "
#endif
#ifdef USE_X11
    "X11 "
#endif
#ifdef WITH_SOUND
    "SOUND "
#endif
    "\n\nDISCLAIMER:\n\
This software is provided as-is.  The author(s) can not be held liable for any\
\ndamage that might arise from the use of this software.\n\
Use it at your own risk.\n"
	,NameLine);

    GameMainLoop();

    return 0;
}

/**
**	Exit clone.
**
**	Called from 'q'.
*/
global volatile void Exit(int err)
{
    IfDebug(
	extern unsigned PfCounterFail;
	extern unsigned PfCounterOk;
	extern unsigned PfCounterDepth;
	extern unsigned PfCounterNotReachable;
    );

    NetworkQuit();
    DebugLevel0("Frames %d, Slow frames %d = %d%%\n"
	,FrameCounter,SlowFrameCounter
	,(SlowFrameCounter*100)/(FrameCounter ? : 1));
    IfDebug(
	UnitCacheStatistic();
	DebugLevel0("Path: Error: %u(%u) OK: %u Depth: %u\n"
		,PfCounterFail,PfCounterNotReachable
		,PfCounterOk,PfCounterDepth);
    );
    fprintf(stderr,"Thanks for playing FreeCraft.\n");
    exit(err);
}

/**
**	Display the usage.
*/
local void Usage(void)
{
    printf("%s\n  written by Lutz Sammer, Fabrice Rossi, Vladi Shabanski, Patrice Fortier,\n  Jon Gabrielson and others."
#ifdef USE_CCL2
    " SIOD Copyright by George J. Carrette."
#endif
"\n\nUsage: clone [OPTIONS] [map.pud|map.pud.gz|map.cm|map.cm.gz]\n\
\t-d datapath\tpath to clone data\n\
\t-c file.ccl\tccl start file\n\
\t-f factor\tComputer units cost factor\n\
\t-h\t\tHelp shows this page\n\
\t-l\t\tEnable command log to \"command.log\"\n\
\t-p players\tNumber of players\n\
\t-n host[:port]\tNetwork argument (port default 6660)\n\
\t-L lag\t\tNetwork lag in # frames\n\
\t-U update\tNetwork update frequence in # frames\n\
\t-s sleep\tNumber of frames for the AI to sleep before they starts\n\
\t-t factor\tComputer units built time factor\n\
\t-v mode\t\tVideo mode (0=default,1=640x480,2=800x600,\n\
\t\t\t\t3=1024x768,4=1600x1200)\n\
\t-D\t\tVideomode depth = pixel pro point (for Win32/TNT)\n\
\t-F\t\tFullscreen videomode (only with SDL supported)\n\
\t-W\t\tWindowed videomode (only with SDL supported)\n\
map is relative to FreeCraftLibPath=datapath, use ./map for relative to cwd\n\
",NameLine);
}

/**
**	The main program: initialise, parse options and arguments.
**
**	@param	argc	Number of arguments.
**	@param	argv	Vector of arguments.
*/
#ifdef __MINGW32__
global int mymain(int argc,char** argv)
#else
global int main(int argc,char** argv)
#endif
{
    //
    //	Setup some defaults.
    //
    FreeCraftLibPath=FREECRAFT_LIB_PATH;
    TitleScreen=strdup("graphic/title.png");
    // FreeCraftLibPath prefixed after it's been decided properly for the next two
#if defined(USE_CCL) || defined(USE_CCL2)
    CclStartFile="ccl/clone.ccl";
    MapName="default.cm";
#else
    MapName="default.pud";	// .gz/.bz2 automatic appended as needed.
#endif

    // FIXME: Parse options before ccl oder after?

    //
    //	Parse commandline
    //
    for( ;; ) {
	switch( getopt(argc,argv,"c:d:f:hln:p:s:t:v:D:FL:U:W?") ) {
#if defined(USE_CCL) || defined(USE_CCL2)
	    case 'c':
		CclStartFile=optarg;
		continue;
#endif
            case 'd':
                FreeCraftLibPath=optarg;
                continue;
	    case 'f':
		AiCostFactor=atoi(optarg);
		continue;
	    case 'l':
		CommandLogEnabled=1;
		continue;
	    case 'p':
		NetPlayers=atoi(optarg);
		continue;
	    case 'n':
		NetworkArg=strdup(optarg);
		continue;
	    case 's':
		AiSleep=atoi(optarg);
		continue;
	    case 't':
		AiTimeFactor=atoi(optarg);
		continue;
	    case 'v':
		switch( atoi(optarg) ) {
		    case 0:
			continue;
		    case 1:
			VideoWidth=640;
			VideoHeight=480;
			continue;
		    case 2:
			VideoWidth=800;
			VideoHeight=600;
			continue;
		    case 3:
			VideoWidth=1024;
			VideoHeight=768;
			continue;
		    case 4:
			VideoWidth=1600;
			VideoHeight=1200;
			continue;
		    default:
			Usage();
			exit(-1);
		}
		continue;

	    case 'L':
		NetworkLag=atoi(optarg);
		continue;
	    case 'U':
		NetworkUpdates=atoi(optarg);
		continue;

	    case 'F':
		VideoFullScreen=1;
		continue;
	    case 'W':
		VideoFullScreen=0;
		continue;
	    case 'D':
		VideoDepth=atoi(optarg);
		continue;

	    case -1:
		break;
	    case '?':
	    case 'h':
	    default:
		Usage();
		exit(-1);
	}
	break;
    }

    if( argc-optind>1 ) {
	fprintf(stderr,"too many files\n");
	Usage();
	exit(-1);
    }

    if( argc-optind ) {
	MapName=argv[optind];
	--argc;
    }

    if (MapName[0] != '/' && MapName[0] != '.') {
        MapName = strdcat3(FreeCraftLibPath, "/", MapName);
    }
#if defined(USE_CCL) || defined(USE_CCL2)
    if (CclStartFile[0] != '/' && CclStartFile[0] != '.') {
        CclStartFile = strdcat3(FreeCraftLibPath, "/", CclStartFile);
    }
    CclInit();				// load configurations!
#endif

    main1(argc,argv);			// CclInit may not return!

    return 0;
}

//@}
