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
/**@name freecraft.h	-	The main header file. */
//
//	(c) Copyright 1998-2002 by Lutz Sammer
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

#ifndef __FREECRAFT_H__
#define __FREECRAFT_H__

//@{

/*============================================================================
==	Config definitions
============================================================================*/

#define noDEBUG				/// Define to include debug code
#define noFLAG_DEBUG			/// Define to include map flag debug

#define noUSE_THREAD			/// Remove no for version with thread

#define noUSE_SDL			/// Remove no for sdl support
#define noUSE_SDLA			/// Remove no for sdl audio support
#define noUSE_X11			/// Remove no for x11 support
#define noUSE_SVGALIB			/// Remove no for svgalib support
#define noUSE_WINCE			/// Remove no for win-ce video support

/**
**	Define this to support load of compressed (gzip) pud files
**	and other data files. (If defined you need libz)
**	Comment if you have problems with gzseek, ... and other gz functions.
*/
#define noUSE_ZLIB

/**
**	Define this to support load of compressed (libbz2) pud files
**	and other data files. (If defined you need libbz2)
*/
#define noUSE_BZ2LIB

/**
**	Define this to support data files stored in a single zip archive or
**	multiple archives. (If defined you need libzziplib)
*/
#define noUSE_ZZIPLIB

//
//	Default speed for many things, set it higher for faster actions.
//
#define SPEED_MINE	1		/// speed factor for mine gold
#define SPEED_GOLD	1		/// speed factor for getting gold
#define SPEED_CHOP	1		/// speed factor for chop
#define SPEED_WOOD	1		/// speed factor for getting wood
#define SPEED_HAUL	1		/// speed factor for haul oil
#define SPEED_OIL	1		/// speed factor for getting oil
#define SPEED_BUILD	1		/// speed factor for building
#define SPEED_TRAIN	1		/// speed factor for training
#define SPEED_UPGRADE	1		/// speed factor for upgrading
#define SPEED_RESEARCH	1		/// speed factor for researching

/*============================================================================
==	Debug definitions
============================================================================*/

#define _C_	,			/// Debug , for non GNU-C compiler


#ifdef DEBUG	// {

/**
**	Include code only if debugging.
*/
#define IfDebug(code)	code

/**
**	Debug check condition
*/
#define DebugCheck(cond)	do{ if( cond ) { \
	fprintf(stderr,"DebugCheck at %s:%d\n",__FILE__,__LINE__); \
	abort(); } }while( 0 )

#ifndef __GNUC__	// { disable GNU C Compiler features

#define __attribute__(args)		// does nothing

#endif	// }

#if defined(__GNUC__)	// {

#if __GNUC__==2 && __GNUC_MINOR__==96

#warning "GCC 2.96 can't compile FreeCraft, downgrade to GCC 2.95"

#endif

#if __GNUC__>=3

//	It looks that GCC 3.xx is becoming nutty:
//	__FUNCTION__	can't be concated in the future.
//	__func__	Is defined by ISO C99 as
//		static const char __func__[] = "function-name";

#define __FUNCTION__ "Wrong compiler"
#warning "GCC 3.XX is not supported, downgrade to GCC 2.95"

#endif

#endif	// } __GNUC__>=3

#ifdef _MSC_VER	// { m$ auto detection

#define inline __inline			// fix m$ brain damage
#define alloca _alloca			// I hope this works with all VC..

#ifndef __FUNCTION__
    // I don't know, but eVC didn't has it, even it is documented
#define __FUNCTION__ __FILE__ ":" /* __LINE__ */

#endif	// } m$

/**
**	Print debug information of level 0.
*/
static inline void DebugLevel0(const char* fmt,...) {};
/**
**	Print debug information of level 1
*/
static inline void DebugLevel1(const char* fmt,...) {};
/**
**	Print debug information of level 2
*/
static inline void DebugLevel2(const char* fmt,...) {};
/**
**	Print debug information of level 3
*/
static inline void DebugLevel3(const char* fmt,...) {};
/**
**	Print debug information of level 0 with function name.
*/
static inline void DebugLevel0Fn(const char* fmt,...) {};
/**
**	Print debug information of level 1 with function name.
*/
static inline void DebugLevel1Fn(const char* fmt,...) {};
/**
**	Print debug information of level 2 with function name.
*/
static inline void DebugLevel2Fn(const char* fmt,...) {};
/**
**	Print debug information of level 3 with function name.
*/
static inline void DebugLevel3Fn(const char* fmt,...) {};

#else	// }{ _MSC_VER

/**
**	Print debug information of level 0.
*/
#define DebugLevel0(fmt,args...)	printf(fmt,##args)

/**
**	Print debug information of level 1.
*/
#define DebugLevel1(fmt,args...)	printf(fmt,##args)

/**
**	Print debug information of level 2.
*/
#define DebugLevel2(fmt,args...)	printf(fmt,##args)

/**
**	Print debug information of level 3.
*/
#define DebugLevel3(fmt,args...)	/* TURNED OFF: printf(fmt,##args) */

/**
**	Print debug information of level 0 with function name.
*/
#define DebugLevel0Fn(fmt,args...)	printf(__FUNCTION__": "fmt,##args)

/**
**	Print debug information of level 1 with function name.
*/
#define DebugLevel1Fn(fmt,args...)	printf(__FUNCTION__": "fmt,##args)

/**
**	Print debug information of level 2 with function name.
*/
#define DebugLevel2Fn(fmt,args...)	printf(__FUNCTION__": "fmt,##args)

/**
**	Print debug information of level 3 with function name.
*/
#define DebugLevel3Fn(fmt,args...)	/* TURNED OFF: printf(__FUNCTION__": "fmt,##args) */

#endif	// } !_MSC_VER

#else	// }{ DEBUG

#define IfDebug(code)
#define DebugCheck(cond)

#ifdef _MSC_VER	// { m$ auto detection

static inline void DebugLevel0(const char* fmt,...) {};
static inline void DebugLevel1(const char* fmt,...) {};
static inline void DebugLevel2(const char* fmt,...) {};
static inline void DebugLevel3(const char* fmt,...) {};
static inline void DebugLevel0Fn(const char* fmt,...) {};
static inline void DebugLevel1Fn(const char* fmt,...) {};
static inline void DebugLevel2Fn(const char* fmt,...) {};
static inline void DebugLevel3Fn(const char* fmt,...) {};

#else	// }{ _MSC_VER

#define DebugLevel0(fmt...)
#define DebugLevel1(fmt...)
#define DebugLevel2(fmt...)
#define DebugLevel3(fmt...)
#define DebugLevel0Fn(fmt...)
#define DebugLevel1Fn(fmt...)
#define DebugLevel2Fn(fmt...)
#define DebugLevel3Fn(fmt...)

#endif	// } !_MSC_VER

#endif	// } !DEBUG

#ifdef REFS_DEBUG	// {

/**
**	Debug check condition
*/
#define RefsDebugCheck(cond)	do{ if( cond ) { \
	fprintf(stderr,"DebugCheck at %s:%d\n",__FILE__,__LINE__); \
	abort(); } }while( 0 )

#else	// }{ REFS_DEBUG

#define RefsDebugCheck(cond)	/* disabled */

#endif	// } !REFS_DEBUG

/*============================================================================
==	Storage types
============================================================================*/

#define global				/// defines global visible names

#ifdef DEBUG
#define local				/// defines local visible names
#else
#define local static
#endif

/*============================================================================
==	Definitions
============================================================================*/

/*----------------------------------------------------------------------------
--	General
----------------------------------------------------------------------------*/

#ifndef VERSION
#define VERSION	"1.17pre1"		/// Engine version shown
#endif

#ifndef FreeCraftMajorVerion
    /// FreeCraft major version
#define FreeCraftMajorVersion	1
    /// FreeCraft minor version (maximal 99)
#define FreeCraftMinorVersion	17
    /// FreeCraft patch level (maximal 99)
#define FreeCraftPatchLevel	0
    /// FreeCraft version (1,2,3) -> 10203
#define FreeCraftVersion \
	(FreeCraftMajorVersion*10000+FreeCraftMinorVersion*100 \
	+FreeCraftPatchLevel)

    /// FreeCraft printf format string
#define FreeCraftFormatString	"%d.%d.%d"
    /// FreeCraft printf format arguments
#define FreeCraftFormatArgs(v)	(v)/10000,((v)/100)%100,(v)%100
#endif

#ifndef FREECRAFT_LIB_PATH
#define FREECRAFT_LIB_PATH "data"	/// where to find the data files
#endif
#ifndef FREECRAFT_HOME_PATH
#define FREECRAFT_HOME_PATH ".freecraft"/// data files in user home dir
#endif

#define MAGIC_FOR_NEW_UNITS	85	/// magic value, new units start with
#define DEMOLISH_DAMAGE		400	/// damage for demolish attack

/*----------------------------------------------------------------------------
--	Some limits
----------------------------------------------------------------------------*/

#define TilesetMax	4		/// How many tilesets are supported
#define PlayerMax	16		/// How many players are supported
#define UnitTypeMax	0xFF		/// How many unit types supported
#define UpgradeMax	256		/// How many upgrades supported
#define UnitMax		2048		/// How many units supported

/*----------------------------------------------------------------------------
--	MacOS X fixes
----------------------------------------------------------------------------*/

#if defined(__APPLE__)

#define MenuKey FreeCraftMenuKey
#define HideCursor FreeCraftHideCursor
#define InitCursor FreeCraftInitCursor

#endif // defined(__APPLE__)

/*----------------------------------------------------------------------------
--	Screen
----------------------------------------------------------------------------*/

// FIXME: this values should go into a general ui structure

#define noGRID		1		/// Map is shown with a grid, if 1

#define DEFAULT_VIDEO_WIDTH	640	/// Default video width
#define DEFAULT_VIDEO_HEIGHT	480	/// Default video height

// This is for 1600x1200
#define MAXMAP_W	50		/// maximum map width in tiles on screen
#define MAXMAP_H	40		/// maximum map height in tiles

#define MINIMAP_W	128		/// minimap width in pixels
#define MINIMAP_H	128		/// minimap height in pixels

    /// scrolling area (<= 10 y)
#define SCROLL_UP	10
    /// scrolling area (>= VideoHeight-11 y)
#define SCROLL_DOWN	(VideoHeight-11)
    /// scrolling area (<= 10 y)
#define SCROLL_LEFT	10
    /// scrolling area (>= VideoWidth-11 x)
#define SCROLL_RIGHT	(VideoWidth-11)

    /// mouse scrolling magnify
#define MOUSE_SCROLL_SPEED	3

    /// keyboard scrolling magnify
#define KEY_SCROLL_SPEED	3

    /// frames per second to display (original 30-40)
#define FRAMES_PER_SECOND	30	// 1/30s

    /// must redraw flags
enum MustRedraw_e {
    RedrawEverything	= -1,		/// must redraw everything
    RedrawNothing	= 0,		/// nothing to do
    RedrawMinimap	= 1,		/// Minimap area
    RedrawMap		= 2,		/// Map area
    RedrawCursor	= 4,		/// Cursor changed
    RedrawResources	= 8,		/// Resources
    RedrawMessage	= 16,		/// Message
    RedrawStatusLine	= 32,		/// Statusline
    RedrawInfoPanel	= 64,		/// Unit description
    RedrawButtonPanel	= 128,		/// Unit buttons
    RedrawFiller1	= 256,		/// Filler1: Border on right side
    RedrawMinimapBorder	= 512,		/// Area around minimap
    RedrawCosts		= 1024,		/// Costs in status line
    RedrawMenuButton	= 2048,		/// Area above minimap
    RedrawMinimapCursor	= 4096,		/// Minimap cursor changed
    RedrawMenu		= 8192,		/// Menu
};

    /// Must redraw all maps
#define RedrawMaps		(RedrawMinimap|RedrawMap)
    /// Must redraw all cursors
#define RedrawCursors		(RedrawMinimapCursor|RedrawCursor)
    /// Must redraw all panels
#define RedrawPanels		(RedrawInfoPanel|RedrawButtonPanel)

#ifdef _MSC_VER	// { m$ auto detection

/**
**	Show load progress.
**	FIXME: Some time this should be shown in tile screen.
*/
static inline void ShowLoadProgress(const char* fmt,...) {};

#else	// }{ _MSC_VER

/**
**	Show load progress.
**	FIXME: Some time this should be shown in tile screen.
*/
#define ShowLoadProgress(fmt,args...)	//printf(fmt,##args)

#endif	// } !_MSC_VER

    /// mainscreen width (default 640)
extern int VideoWidth;

    /// mainscreen height (default 480)
extern int VideoHeight;

    /// invalidated map
extern enum MustRedraw_e MustRedraw;

    /// counts frames
extern int FrameCounter;

    /// counts quantity of slow frames
extern int SlowFrameCounter;

/*----------------------------------------------------------------------------
--	Convert
----------------------------------------------------------------------------*/

extern int Screen2MapX(int x);		/// Convert screen pixel to map tile
extern int Screen2MapY(int y);		/// Convert screen pixel to map tile
extern int Map2ScreenX(int x);		/// Convert map tile to screen pixel
extern int Map2ScreenY(int y);		/// Convert map tile to screen pixel

/*----------------------------------------------------------------------------
--	clone.c
----------------------------------------------------------------------------*/

/**
**	SyncRand():	should become a syncron rand on all machines
**			for network play.
*/
#define NoSyncRand()	rand()

/**
**	MyRand():	rand only used on this computer.
*/
#define MyRand()	rand()

extern char* TitleScreen;		/// file for title screen
extern char* MenuBackground;		/// file for menu background
extern char* MenuBackgroundWithTitle;	/// file for menu with title
extern char* TitleMusic;		/// file for title music
extern char* MenuMusic;			/// file for menu music
extern char* FreeCraftLibPath;		/// location of freecraft data

extern int SpeedMine;			/// speed factor for mine gold
extern int SpeedGold;			/// speed factor for getting gold
extern int SpeedChop;			/// speed factor for chop
extern int SpeedWood;			/// speed factor for getting wood
extern int SpeedHaul;			/// speed factor for haul oil
extern int SpeedOil;			/// speed factor for getting oil
extern int SpeedBuild;			/// speed factor for building
extern int SpeedTrain;			/// speed factor for training
extern int SpeedUpgrade;		/// speed factor for upgrading
extern int SpeedResearch;		/// speed factor for researching

//FIXME: all game global options should be moved in structure like `TheUI'
extern int OptionUseDepletedMines;      /// use depleted mines or destroy them

extern unsigned SyncRandSeed;		/// sync random seed value.

extern void LoadGame(char*);		/// Load saved game back
extern void SaveGame(const char*);	/// Save game for later load

extern void SaveAll(void);		/// Call all modules to save states
extern void LoadAll(void);		/// Load all data back

extern int SyncRand(void);		/// Syncron rand

extern int main1(int argc,char* argv[]);/// init freecraft
extern volatile void Exit(int err);	/// exit freecraft

extern void UpdateDisplay(void);	/// game display update
extern void InitModules(void);		/// Initinalize all modules
extern void LoadModules(void);		/// Load all modules
extern void CleanModules(void);		/// Cleanup all modules
extern void GameMainLoop(void);		/// game main loop

     /// strdup + strcat
extern char* strdcat(const char* l, const char* r);
     /// strdup + strcat + strcat
extern char* strdcat3(const char* l, const char *m, const char* r);

/*============================================================================
==	Misc
============================================================================*/

#ifndef max
    /// max macro
#define max(n1,n2)	(((n1)<(n2)) ? (n2) : (n1))
#endif

    /// bits macro
#define BitsOf(n)	(sizeof(n)*8)

    /// How long stay in a gold-mine
#define MINE_FOR_GOLD	(UnitTypeGoldMine->_Costs[TimeCost]/SpeedMine)
    /// How long stay in a gold-deposit
#define WAIT_FOR_GOLD	(UnitTypeGoldMine->_Costs[TimeCost]/SpeedGold)
    /// How much I must chop for 1 wood
#define CHOP_FOR_WOOD	(52/SpeedChop)
    /// How long stay in a wood-deposit
#define WAIT_FOR_WOOD	(100/SpeedWood)
    /// How long stay in a oil-well
#define HAUL_FOR_OIL	(100/SpeedHaul)
    /// How long stay in a oil-deposit
#define WAIT_FOR_OIL	(100/SpeedOil)

    /// How many resource get the player back if canceling building
#define CancelBuildingCostsFactor	75
    /// How many resource get the player back if canceling training
#define CancelTrainingCostsFactor	100
    /// How many resource get the player back if canceling research
#define CancelResearchCostsFactor	100
    /// How many resource get the player back if canceling upgrade
#define CancelUpgradeCostsFactor	100

    /// How near could a hall or gold-depot be build to a goldmine
#define GOLDMINE_DISTANCE	3
    /// How near could a oil-depot be build to a oil-patch
#define OILPATCH_DISTANCE	3

    /// How near we could repair an unit
#define REPAIR_RANGE		1

//@}

#endif	// !__FREECRAFT_H__
