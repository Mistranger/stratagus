//   ___________		     _________		      _____  __
//   \_	  _____/______	 ____	____ \_	  ___ \____________ _/ ____\/  |_
//    |	   __) \_  __ \_/ __ \_/ __ \/	  \  \/\_  __ \__  \\	__\\   __\ 
//    |	    \	|  | \/\  ___/\	 ___/\	   \____|  | \// __ \|	|   |  |
//    \___  /	|__|	\___  >\___  >\______  /|__|  (____  /__|   |__|
//	  \/		    \/	   \/	     \/		   \/
//  ______________________			     ______________________
//			  T H E	  W A R	  B E G I N S
//	   FreeCraft - A free fantasy real time strategy game engine
//
/**@name map_draw.c	-	The map drawing.
**
**	@todo FIXME: Johns: More to come: zooming, scaling, 64x64 tiles...
*/
//
//	(c) Copyright 1999-2002 by Lutz Sammer
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
--	Documentation
----------------------------------------------------------------------------*/

/**
**	@def USE_SMART_TILECACHE
**
**	If USE_SMART_TILECACHE is defined, the code is compiled with
**	the smart-tile-cache support. With the smart-tile-cache support a
**	tile is only converted once to the video format, each consequent access
**	use the ready converted image form the video memory.
**
**	Nothing is cached between frames. Slow with hardware video memory.
**
**	@see USE_TILECACHE
*/

/**
**	@def noUSE_TILECACHE
**
**	If USE_TILECACHE is defined, the code is compiled with the tile-cache
**	support. With the tile-cache support a tile is only converted once to
**	video format, each consequent access use the ready converted image form
**	the cache memory. The LRU cache is of TileCacheSize.
**
**	@see TileCacheSize @see USE_SMART_TILECACHE
*/

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freecraft.h"
#include "tileset.h"
#include "video.h"
#include "map.h"
#include "player.h"
#include "pathfinder.h"
#include "ui.h"
#include "deco.h"

#include "etlib/dllist.h"
#if defined(DEBUG) && defined(TIMEIT)
#include "rdtsc.h"
#endif

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

#define noUSE_TILECACHE			/// defined use tile cache
#define USE_SMART_TILECACHE		/// defined use a smart tile cache

#ifdef DEBUG
#define noTIMEIT			/// defined time function
#endif

#ifdef USE_TILECACHE	// {

/**
**	Cache managment structure.
*/
typedef struct _tile_cache {
    struct dl_node	DlNode;		/// double linked list for lru
    unsigned		Tile;		/// for this tile (0 can't be cached)
    unsigned char	Buffer[1];	/// memory
} TileCache;

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/**
**	Contains pointer, if the tile is cached.
**
**	@note FIXME: could save memory here and only use how many tiles exits.
**
**	@see MaxTilesInTileset
*/
local TileCache* TileCached[MaxTilesInTileset];

/**
**	Number of tile caches.
**
**	@todo FIXME: Not make this configurable by ccl
*/
global int TileCacheSize=196;

/**
**	Last recent used cache tiles.
*/
local DL_LIST(TileCacheLRU);

#endif	// } USE_TILECACHE

#ifdef USE_SMART_TILECACHE

/**
**	Contains pointer, to last video position, where this tile was drawn.
**
**	@note FIXME: could save memory here and only use how many tiles exits.
**
**	@see MaxTilesInTileset
*/
local void* TileCached[MaxTilesInTileset];

#endif

/**
**	Low word contains pixel data for 16 bit modes.
**
**	@note FIXME: should or could be moved into video part?
*/
local unsigned int PixelsLow[256];

/**
**	High word contains pixel data for 16 bit modes.
**
**	@note FIXME: should or could be moved into video part?
*/
local unsigned int PixelsHigh[256];

/**
**	Flags must redraw map row.
**	MustRedrawRow[y]!=0 This line of tiles must be redrawn.
**
**	@see MAXMAP_W 
*/
global char MustRedrawRow[MAXMAP_W];

/**
**	Flags must redraw tile.
**	MustRedrawRow[x+y*::MapWidth]!=0 This tile must be redrawn.
**
**	@see MAXMAP_W @see MAXMAP_H
*/
global char MustRedrawTile[MAXMAP_W*MAXMAP_H];

/**
**	Fast draw tile function pointer.
**
**	Draws tiles display and video mode independ
*/
global void (*VideoDrawTile)(const GraphicData*,int,int);

/**
**	Fast draw tile function pointer with cache support.
**
**	Draws tiles display and video mode independ
*/
global void (*MapDrawTile)(int,int,int);

#ifdef NEW_DECODRAW
/**
**	Decoration as registered for decoration mechanism to draw map tiles
*/
local Deco *mapdeco = NULL;
#endif

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

#if GRID==1
    /// Draw less for grid display
#define GRID_SUB	TileSizeX
#else
    /// Draw all (no grid enabled)
#define GRID_SUB	0
#endif

/**
**	Do unroll 8x
**
**	@param x	Index passed to UNROLL2 incremented by 2.
*/
#define UNROLL8(x)	\
    UNROLL2((x)+0);	\
    UNROLL2((x)+2);	\
    UNROLL2((x)+4);	\
    UNROLL2((x)+6)

/**
**	Do unroll 16x
**
**	@param x	Index passed to UNROLL8 incremented by 8.
*/
#define UNROLL16(x)	\
    UNROLL8((x)+ 0);	\
    UNROLL8((x)+ 8)

/**
**	Do unroll 24x
**
**	@param x	Index passed to UNROLL8 incremented by 8.
*/
#define UNROLL24(x)	\
    UNROLL8((x)+ 0);	\
    UNROLL8((x)+ 8);	\
    UNROLL8((x)+16)

/**
**	Do unroll 32x
**
**	@param x	Index passed to UNROLL8 incremented by 8.
*/
#define UNROLL32(x)	\
    UNROLL8((x)+ 0);	\
    UNROLL8((x)+ 8);	\
    UNROLL8((x)+16);	\
    UNROLL8((x)+24)

/*----------------------------------------------------------------------------
--	Draw tile
----------------------------------------------------------------------------*/

/**
**	Fast draw 32x32 tile for 8 bpp video modes.
**
**	@param data	pointer to tile graphic data
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@note This is a hot spot in the program.
**	(50% cpu time was needed for this, now only 32%)
**
**	@see GRID
*/
global void VideoDraw8Tile32(const unsigned char* data,int x,int y)
{
    const unsigned char* sp;
    const unsigned char* ep;
    VMemType8* dp;
    int da;

    sp=data;
    ep=sp+TileSizeY*TileSizeX-GRID_SUB;
    da=VideoWidth;
    dp=VideoMemory8+x+y*VideoWidth;

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)		\
	dp[x+0]=((VMemType8*)TheMap.TileData->Pixels)[sp[x+0]]; \
	dp[x+1]=((VMemType8*)TheMap.TileData->Pixels)[sp[x+1]]

	UNROLL32(0);
#if GRID==1
	dp[31]=((VMemType8*)TheMap.TileData->Pixels)[0];
#endif
	sp+=TileSizeX;
	dp+=da;
    }

#if GRID==1
    for( da=TileSizeX; da--; ) {	// with grid no need to be fast
	dp[da]=((VMemType8*)TheMap.TileData->Pixels)[0];
    }
#endif
}

/**
**	Fast draw 32x32 tile for 16 bpp video modes.
**
**	@param data	pointer to tile graphic data
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@note This is a hot spot in the program.
**	(50% cpu time was needed for this, now only 32%)
**
**	@see GRID
*/
global void VideoDraw16Tile32(const unsigned char* data,int x,int y)
{
    const unsigned char* sp;
    const unsigned char* ep;
    VMemType16* dp;
    int da;

    sp=data;
    ep=sp+TileSizeY*TileSizeX-GRID_SUB;
    da=VideoWidth;
    dp=VideoMemory16+x+y*VideoWidth;

    IfDebug(
	if( ((long)sp)&1 ) {
	    DebugLevel0("Not aligned memory\n");
	}
	if( ((long)dp)&3 ) {
	    DebugLevel0("Not aligned memory\n");
	}
    );

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)		\
	*(unsigned int*)(dp+x+0)=PixelsLow[sp[x+0]]|PixelsHigh[sp[x+1]]

	UNROLL32(0);
#if GRID==1
	dp[31]=Pixels16[0];
#endif
	sp+=TileSizeX;
	dp+=da;
    }

#if GRID==1
    for( da=TileSizeX; da--; ) {	// with grid no need to be fast
	dp[da]=Pixels16[0];
    }
#endif
}

/**
**	Fast draw 32x32 tile for 24 bpp video modes.
**
**	@param data	pointer to tile graphic data
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@note This is a hot spot in the program.
**	(50% cpu time was needed for this, now only 32%)
**
**	@see GRID
*/
global void VideoDraw24Tile32(const unsigned char* data,int x,int y)
{
    const unsigned char* sp;
    const unsigned char* ep;
    VMemType24* dp;
    int da;

    sp=data;
    ep=sp+TileSizeY*TileSizeX-GRID_SUB;
    da=VideoWidth;
    dp=VideoMemory24+x+y*VideoWidth;

    IfDebug(
	if( ((long)sp)&1 ) {
	    DebugLevel0("Not aligned memory\n");
	}
	if( ((long)dp)&3 ) {
	    DebugLevel0("Not aligned memory\n");
	}
    );

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)		\
	dp[x+0]=((VMemType24*)TheMap.TileData->Pixels)[sp[x+0]];	\
	dp[x+1]=((VMemType24*)TheMap.TileData->Pixels)[sp[x+1]]

	UNROLL32(0);
#if GRID==1
	dp[31]=Pixels24[0];
#endif
	sp+=TileSizeX;
	dp+=da;
    }

#if GRID==1
    for( da=TileSizeX; da--; ) {	// with grid no need to be fast
	dp[da]=Pixels24[0];
    }
#endif
}

/**
**	Fast draw 32x32 tile for 32 bpp video modes.
**
**	@param data	pointer to tile graphic data
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@note This is a hot spot in the program.
**	(50% cpu time was needed for this, now only 32%)
**
**	@see GRID
*/
global void VideoDraw32Tile32(const unsigned char* data,int x,int y)
{
    const unsigned char* sp;
    const unsigned char* ep;
    VMemType32* dp;
    int da;

    sp=data;
    ep=sp+TileSizeY*TileSizeX-GRID_SUB;
    da=VideoWidth;
    dp=VideoMemory32+x+y*VideoWidth;

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)		\
	dp[x+0]=((VMemType32*)TheMap.TileData->Pixels)[sp[x+0]];	\
	dp[x+1]=((VMemType32*)TheMap.TileData->Pixels)[sp[x+1]]

	UNROLL32(0);
#if GRID==1
	dp[31]=((VMemType32*)TheMap.TileData->Pixels)[0];
#endif
	sp+=TileSizeX;
	dp+=da;
    }

#if GRID==1
    for( da=TileSizeX; da--; ) {	// with grid no need to be fast
	dp[da]=((VMemType32*)TheMap.TileData->Pixels)[0];
    }
#endif
}

#ifdef NEW_DECODRAW
/**
**	Draw TileSizeX x TileSizeY clipped for XX bpp video modes.
**	(needed for decoration mechanism, which wants to draw tile partly)
**	FIXME: this separate function is only needed for compatibility with
**             variable VideoDrawTile, can be replaced by MapDrawXXTileClip
**
**	@param data	pointer to tile graphic data
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
local void VideoDrawXXTileClip(const unsigned char* data,int x,int y)
{
  VideoDrawRawClip( (VMemType*) TheMap.TileData->Pixels,
                    data, x, y, TileSizeX, TileSizeY );
}

/**
**	Draw TileSizeX x TileSizeY clipped for XX bpp video modes.
**	(needed for decoration mechanism, which wants to draw tile partly)
**
**	@param tile	Tile number to draw.
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
local void MapDrawXXTileClip(int tile,int x,int y)
{
    VideoDrawXXTileClip(TheMap.Tiles[tile],x,y);
}
#endif

/*----------------------------------------------------------------------------
--	Draw tile with zoom
----------------------------------------------------------------------------*/

// FIXME: write this

/*----------------------------------------------------------------------------
--	Cache
----------------------------------------------------------------------------*/

#ifdef USE_TILECACHE	// {

/**
**	Draw 32x32 tile for 8 bpp video modes into cache and video memory.
**
**	@param graphic	Graphic structure for the tile
**	@param cache	Cache to fill with tile
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@note This is a hot spot in the program.
**
**	@see GRID
*/
local void FillCache8AndDraw32(const unsigned char* data,VMemType8* cache
	,int x,int y)
{
    const unsigned char* sp;
    const unsigned char* ep;
    int va;
    VMemType8* dp;
    VMemType8* vp;

    sp=data;
    ep=sp+TileSizeY*TileSizeX-GRID_SUB;
    dp=cache;
    va=VideoWidth;
    vp=VideoMemory8+x+y*VideoWidth;

    IfDebug(
	if( ((long)sp)&1 ) {
	    DebugLevel0("Not aligned memory\n");
	}
	if( ((long)dp)&3 ) {
	    DebugLevel0("Not aligned memory\n");
	}
	if( ((long)vp)&3 ) {
	    DebugLevel0("Not aligned video memory\n");
	}
    );

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)	\
	vp[x+0]=dp[x+0]=((VMemType8*)TheMap.TileData->Pixels)[sp[x+0]]; \
	vp[x+0]=dp[x+1]=((VMemType8*)TheMap.TileData->Pixels)[sp[x+1]]

	UNROLL32(0);
#if GRID==1
	vp[31]=dp[31]=((VMemType8*)TheMap.TileData->Pixels)[0];
#endif
	vp+=va;
	sp+=TileSizeX;
	dp+=TileSizeX;
    }

#if GRID==1
    for( va=TileSizeX; va--; ) {	// no need to be fast with grid
	vp[va]=dp[va]=((VMemType8*)TheMap.TileData->Pixels)[0];
    }
#endif
}

/**
**	Draw 32x32 tile for 16 bpp video modes into cache and video memory.
**
**	@param graphic	Graphic structure for the tile
**	@param cache	Cache to fill with tile
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@note This is a hot spot in the program.
**
**	@see GRID
*/
local void FillCache16AndDraw32(const unsigned char* data,VMemType16* cache
	,int x,int y)
{
    const unsigned char* sp;
    const unsigned char* ep;
    int va;
    VMemType16* dp;
    VMemType16* vp;

    sp=data;
    ep=sp+TileSizeY*TileSizeX-GRID_SUB;
    dp=cache;
    va=VideoWidth;
    vp=VideoMemory16+x+y*VideoWidth;

    IfDebug(
	if( ((long)sp)&1 ) {
	    DebugLevel0("Not aligned memory\n");
	}
	if( ((long)dp)&3 ) {
	    DebugLevel0("Not aligned memory\n");
	}
	if( ((long)vp)&3 ) {
	    DebugLevel0("Not aligned video memory\n");
	}
    );

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)	\
	*(unsigned int*)(vp+x+0)=*(unsigned int*)(dp+x+0)	\
		=PixelsLow[sp[x+0]]|PixelsHigh[sp[x+1]]

	UNROLL32(0);
#if GRID==1
	vp[31]=dp[31]=Pixels[0];
#endif
	vp+=va;
	sp+=TileSizeX;
	dp+=TileSizeX;
    }

#if GRID==1
    for( va=TileSizeX; va--; ) {	// no need to be fast with grid
	vp[va]=dp[va]=Pixels[0];
    }
#endif
}

/**
**	Draw 32x32 tile for 24 bpp video modes into cache and video memory.
**
**	@param graphic	Graphic structure for the tile
**	@param cache	Cache to fill with tile
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@note This is a hot spot in the program.
**
**	@see GRID
*/
local void FillCache24AndDraw32(const unsigned char* data,VMemType24* cache
	,int x,int y)
{
    const unsigned char* sp;
    const unsigned char* ep;
    int va;
    VMemType24* dp;
    VMemType24* vp;

    sp=data;
    ep=sp+TileSizeY*TileSizeX-GRID_SUB;
    dp=cache;
    va=VideoWidth;
    vp=VideoMemory24+x+y*VideoWidth;

    IfDebug(
	if( ((long)sp)&1 ) {
	    DebugLevel0("Not aligned memory\n");
	}
	if( ((long)dp)&3 ) {
	    DebugLevel0("Not aligned memory\n");
	}
	if( ((long)vp)&3 ) {
	    DebugLevel0("Not aligned video memory\n");
	}
    );

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)	\
	vp[x+0]=dp[x+0]=((VMemType24*)TheMap.TileData->Pixels)[sp[x+0]]; \
	vp[x+0]=dp[x+1]=((VMemType24*)TheMap.TileData->Pixels)[sp[x+1]]

	UNROLL32(0);
#if GRID==1
	vp[31]=dp[31]=((VMemType24*)TheMap.TileData->Pixels)[0];
#endif
	vp+=va;
	sp+=TileSizeX;
	dp+=TileSizeX;
    }

#if GRID==1
    for( va=TileSizeX; va--; ) {	// no need to be fast with grid
	vp[va]=dp[va]=((VMemType24*)TheMap.TileData->Pixels)[0];
    }
#endif
}

/**
**	Draw 32x32 tile for 32 bpp video modes into cache and video memory.
**
**	@param graphic	Graphic structure for the tile
**	@param cache	Cache to fill with tile
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@note This is a hot spot in the program.
**
**	@see GRID
*/
local void FillCache32AndDraw32(const unsigned char* data,VMemType32* cache
	,int x,int y)
{
    const unsigned char* sp;
    const unsigned char* ep;
    int va;
    VMemType32* dp;
    VMemType32* vp;

    sp=data;
    ep=sp+TileSizeY*TileSizeX-GRID_SUB;
    dp=cache;
    va=VideoWidth;
    vp=VideoMemory32+x+y*VideoWidth;

    IfDebug(
	if( ((long)sp)&1 ) {
	    DebugLevel0("Not aligned memory\n");
	}
	if( ((long)dp)&3 ) {
	    DebugLevel0("Not aligned memory\n");
	}
	if( ((long)vp)&3 ) {
	    DebugLevel0("Not aligned video memory\n");
	}
    );

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)	\
	vp[x+0]=dp[x+0]=((VMemType32*)TheMap.TileData->Pixels)[sp[x+0]]; \
	vp[x+0]=dp[x+1]=((VMemType32*)TheMap.TileData->Pixels)[sp[x+1]]

	UNROLL32(0);
#if GRID==1
	vp[31]=dp[31]=((VMemType32*)TheMap.TileData->Pixels)[0];
#endif
	vp+=va;
	sp+=TileSizeX;
	dp+=TileSizeX;
    }

#if GRID==1
    for( va=TileSizeX; va--; ) {	// no need to be fast with grid
	vp[va]=dp[va]=((VMemType32*)TheMap.TileData->Pixels)[0];
    }
#endif
}

// ---------------------------------------------------------------------------

/**
**	Fast draw 32x32 tile from cache for 8bpp.
**
**	@param graphic	Pointer to cached tile graphic
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@see GRID
*/
local void VideoDraw8Tile32FromCache(const VMemType8* graphic,int x,int y)
{
    const VMemType8* sp;
    const VMemType8* ep;
    VMemType8* dp;
    int da;

    sp=graphic;
    ep=sp+TileSizeY*TileSizeX;
    da=VideoWidth;
    dp=VideoMemory8+x+y*VideoWidth;

    IfDebug(
	if( ((long)dp)&3 ) {
	    DebugLevel0("Not aligned memory\n");
	}
	if( ((long)sp)&3 ) {
	    DebugLevel0("Not aligned memory\n");
	}
    );

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)		\
	*(unsigned long*)(dp+x)=*(unsigned long*)(sp+x)

	UNROLL16(0);
	sp+=TileSizeX;
	dp+=da;
    }
}

/**
**	Fast draw 32x32 tile from cache for 16bpp.
**
**	@param graphic	Pointer to cached tile graphic
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@see GRID
*/
local void VideoDraw16Tile32FromCache(const VMemType16* graphic,int x,int y)
{
    const VMemType16* sp;
    const VMemType16* ep;
    VMemType16* dp;
    int da;

    sp=graphic;
    ep=sp+TileSizeY*TileSizeX;
    da=VideoWidth;
    dp=VideoMemory16+x+y*VideoWidth;

    IfDebug(
	if( ((long)dp)&3 ) {
	    DebugLevel0("Not aligned memory\n");
	}
	if( ((long)sp)&3 ) {
	    DebugLevel0("Not aligned memory\n");
	}
    );

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)		\
	*(unsigned long*)(dp+x)=*(unsigned long*)(sp+x)

	UNROLL32(0);
	sp+=TileSizeX;
	dp+=da;
    }
}

/**
**	Fast draw 32x32 tile from cache for 24bpp.
**
**	@param graphic	Pointer to cached tile graphic
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@see GRID
*/
local void VideoDraw24Tile32FromCache(const VMemType24* graphic,int x,int y)
{
    const VMemType24* sp;
    const VMemType24* ep;
    VMemType24* dp;
    int da;

    sp=graphic;
    ep=sp+TileSizeY*TileSizeX;
    da=VideoWidth;
    dp=VideoMemory24+x+y*VideoWidth;

    IfDebug(
	if( ((long)dp)&3 ) {
	    DebugLevel0("Not aligned memory\n");
	}
	if( ((long)sp)&3 ) {
	    DebugLevel0("Not aligned memory\n");
	}
    );

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)		\
	*(unsigned long*)(dp+x*2+0)=*(unsigned long*)(sp+x*2+0);	\
	*(unsigned long*)(dp+x*2+1)=*(unsigned long*)(sp+x*2+1)

	UNROLL24(0);
	sp+=TileSizeX;
	dp+=da;
    }
}

/**
**	Fast draw 32x32 tile from cache.
**
**	@param graphic	Pointer to cached tile graphic
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@see GRID
*/
local void VideoDraw32Tile32FromCache(const VMemType32* graphic,int x,int y)
{
    const VMemType32* sp;
    const VMemType32* ep;
    VMemType32* dp;
    int da;

    sp=graphic;
    ep=sp+TileSizeY*TileSizeX;
    da=VideoWidth;
    dp=VideoMemory32+x+y*VideoWidth;

    IfDebug(
	if( ((long)dp)&3 ) {
	    DebugLevel0("Not aligned memory\n");
	}
	if( ((long)sp)&3 ) {
	    DebugLevel0("Not aligned memory\n");
	}
    );

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)		\
	*(unsigned long*)(dp+x*2+0)=*(unsigned long*)(sp+x*2+0);	\
	*(unsigned long*)(dp+x*2+1)=*(unsigned long*)(sp+x*2+1)

	UNROLL32(0);
	sp+=TileSizeX;
	dp+=da;
    }
}

// ---------------------------------------------------------------------------

/**
**	Draw 32x32 tile for 8 bpp video modes with cache support.
**
**	@param tile	Tile number to draw.
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
local void MapDraw8Tile32(int tile,int x,int y)
{
    TileCache* cache;

    if( !(cache=TileCached[tile]) ) {
	//
	//	Not cached
	//
	if( TileCacheSize ) {		// enough cache buffers?
	    --TileCacheSize;
	    cache=malloc(
		    sizeof(TileCache)-sizeof(unsigned char)+
		    TileSizeX*TileSizeY*sizeof(VMemType16));
	} else {
	    cache=(void*)TileCacheLRU->last;
	    if( cache->Tile ) {
		TileCached[cache->Tile]=NULL;	// now not cached
	    }
	    dl_remove_last(TileCacheLRU);
	    DebugLevel3("EMPTY CACHE\n");
	}
	TileCached[tile]=cache;
	cache->Tile=tile;
	dl_insert_first(TileCacheLRU,&cache->DlNode);

	FillCache8AndDraw32(TheMap.Tiles[tile],(void*)&cache->Buffer,x,y);
    } else {
	VideoDraw8Tile32FromCache((void*)&cache->Buffer,x,y);
    }
}

/**
**	Draw 32x32 tile for 16 bpp video modes with cache support.
**
**	@param tile	Tile number to draw.
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
local void MapDraw16Tile32(int tile,int x,int y)
{
    TileCache* cache;

    if( !(cache=TileCached[tile]) ) {
	//
	//	Not cached
	//
	if( TileCacheSize ) {		// enough cache buffers?
	    --TileCacheSize;
	    cache=malloc(
		    sizeof(TileCache)-sizeof(unsigned char)+
		    TileSizeX*TileSizeY*sizeof(VMemType16));
	} else {
	    cache=(void*)TileCacheLRU->last;
	    if( cache->Tile ) {
		TileCached[cache->Tile]=NULL;	// now not cached
	    }
	    dl_remove_last(TileCacheLRU);
	    DebugLevel3("EMPTY CACHE\n");
	}
	TileCached[tile]=cache;
	cache->Tile=tile;
	dl_insert_first(TileCacheLRU,&cache->DlNode);

	FillCache16AndDraw32(TheMap.Tiles[tile],(void*)&cache->Buffer,x,y);
    } else {
	VideoDraw16Tile32FromCache((void*)&cache->Buffer,x,y);
    }
}

/**
**	Draw 32x32 tile for 24 bpp video modes with cache support.
**
**	@param tile	Tile number to draw.
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
local void MapDraw24Tile32(int tile,int x,int y)
{
    TileCache* cache;

    if( !(cache=TileCached[tile]) ) {
	//
	//	Not cached
	//
	if( TileCacheSize ) {		// enough cache buffers?
	    --TileCacheSize;
	    cache=malloc(
		    sizeof(TileCache)-sizeof(unsigned char)+
		    TileSizeX*TileSizeY*sizeof(VMemType24));
	} else {
	    cache=(void*)TileCacheLRU->last;
	    if( cache->Tile ) {
		TileCached[cache->Tile]=NULL;	// now not cached
	    }
	    dl_remove_last(TileCacheLRU);
	    DebugLevel3("EMPTY CACHE\n");
	}
	TileCached[tile]=cache;
	cache->Tile=tile;
	dl_insert_first(TileCacheLRU,&cache->DlNode);

	FillCache24AndDraw32(TheMap.Tiles[tile],(void*)&cache->Buffer,x,y);
    } else {
	VideoDraw24Tile32FromCache((void*)&cache->Buffer,x,y);
    }
}

/**
**	Draw 32x32 tile for 32 bpp video modes with cache support.
**
**	@param tile	Tile number to draw.
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
local void MapDraw32Tile32(int tile,int x,int y)
{
    TileCache* cache;

    if( !(cache=TileCached[tile]) ) {
	//
	//	Not cached
	//
	if( TileCacheSize ) {		// enough cache buffers?
	    --TileCacheSize;
	    cache=malloc(
		    sizeof(TileCache)-sizeof(unsigned char)+
		    TileSizeX*TileSizeY*sizeof(VMemType32));
	} else {
	    cache=(void*)TileCacheLRU->last;
	    if( cache->Tile ) {
		TileCached[cache->Tile]=NULL;	// now not cached
	    }
	    dl_remove_last(TileCacheLRU);
	    DebugLevel3("EMPTY CACHE\n");
	}
	TileCached[tile]=cache;
	cache->Tile=tile;
	dl_insert_first(TileCacheLRU,&cache->DlNode);

	FillCache32AndDraw32(TheMap.Tiles[tile],(void*)&cache->Buffer,x,y);
    } else {
	VideoDraw32Tile32FromCache((void*)&cache->Buffer,x,y);
    }
}

#endif	// } USE_TILECACHE

/*----------------------------------------------------------------------------
--	Smart Cache
----------------------------------------------------------------------------*/

#ifdef USE_SMART_TILECACHE	// {

/**
**	Fast draw 32x32 tile for 8 bpp from cache.
**
**	@param graphic	Pointer to cached tile graphic
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@see GRID
*/
local void VideoDraw8Tile32Cached(const VMemType8* graphic,int x,int y)
{
    const VMemType8* sp;
    const VMemType8* ep;
    VMemType8* dp;
    int da;

    sp=graphic;
    da=VideoWidth;
    ep=sp+TileSizeX+TileSizeY*da;
    dp=VideoMemory8+x+y*da;

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)		\
	((unsigned long*)dp)[x+0]=((unsigned long*)sp)[x+0]; \
	((unsigned long*)dp)[x+1]=((unsigned long*)sp)[x+1]

	UNROLL8(0);
	sp+=da;
	dp+=da;

	UNROLL8(0);
	sp+=da;
	dp+=da;
    }
}

/**
**	Fast draw 32x32 tile for 16 bpp from cache.
**
**	@param graphic	Pointer to cached tile graphic
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@see GRID
*/
local void VideoDraw16Tile32Cached(const VMemType16* graphic,int x,int y)
{
    const VMemType16* sp;
    const VMemType16* ep;
    VMemType16* dp;
    int da;

    sp=graphic;
    da=VideoWidth;
    ep=sp+TileSizeY*da;
    dp=VideoMemory16+x+y*da;

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)		\
	*(unsigned long*)(dp+x)=*(unsigned long*)(sp+x)

	UNROLL32(0);
	sp+=da;
	dp+=da;

	UNROLL32(0);
	sp+=da;
	dp+=da;
    }
}

/**
**	Fast draw 32x32 tile for 24 bpp from cache.
**
**	@param graphic	Pointer to cached tile graphic
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@see GRID
*/
local void VideoDraw24Tile32Cached(const VMemType24* graphic,int x,int y)
{
    const VMemType24* sp;
    const VMemType24* ep;
    VMemType24* dp;
    int da;

    sp=graphic;
    da=VideoWidth;
    ep=sp+TileSizeX+TileSizeY*da;
    dp=VideoMemory24+x+y*da;

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)		\
	*((unsigned long*)dp+x+0)=*((unsigned long*)sp+x+0);	\
	*((unsigned long*)dp+x+1)=*((unsigned long*)sp+x+1)

	UNROLL24(0);
	sp+=da;
	dp+=da;

	UNROLL24(0);
	sp+=da;
	dp+=da;
    }
}

/**
**	Fast draw 32x32 tile for 32 bpp from cache.
**
**	@param graphic	Pointer to cached tile graphic
**	@param x	X position into video memory
**	@param y	Y position into video memory
**
**	@see GRID
*/
local void VideoDraw32Tile32Cached(const VMemType32* graphic,int x,int y)
{
    const VMemType32* sp;
    const VMemType32* ep;
    VMemType32* dp;
    int da;

    sp=graphic;
    da=VideoWidth;
    ep=sp+TileSizeX+TileSizeY*da;
    dp=VideoMemory32+x+y*da;

    while( sp<ep ) {			// loop unrolled
#undef UNROLL2
	/// basic unroll code
#define UNROLL2(x)		\
	*(dp+x*2+0)=*(sp+x*2+0);	\
	*(dp+x*2+1)=*(sp+x*2+1)

	UNROLL32(0);
	sp+=da;
	dp+=da;

	UNROLL32(0);
	sp+=da;
	dp+=da;
    }
}

// ---------------------------------------------------------------------------

/**
**	Draw 32x32 tile for 8 bpp video modes with cache support.
**
**	@param tile	Tile number to draw.
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
local void MapDraw8Tile32(int tile,int x,int y)
{
    if( TileCached[tile] ) {
	VideoDraw8Tile32Cached(TileCached[tile],x,y);
    } else {
	VideoDraw8Tile32(TheMap.Tiles[tile],x,y);
	TileCached[tile]=VideoMemory8+x+y*VideoWidth;
    }
}

/**
**	Draw 32x32 tile for 16 bpp video modes with cache support.
**
**	@param tile	Tile number to draw.
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
local void MapDraw16Tile32(int tile,int x,int y)
{
    if( TileCached[tile] ) {
	VideoDraw16Tile32Cached(TileCached[tile],x,y);
    } else {
	VideoDraw16Tile32(TheMap.Tiles[tile],x,y);
	TileCached[tile]=VideoMemory16+x+y*VideoWidth;
    }
}

/**
**	Draw 32x32 tile for 24 bpp video modes with cache support.
**
**	@param tile	Tile number to draw.
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
local void MapDraw24Tile32(int tile,int x,int y)
{
    if( TileCached[tile] ) {
	VideoDraw24Tile32Cached(TileCached[tile],x,y);
    } else {
	VideoDraw24Tile32(TheMap.Tiles[tile],x,y);
	TileCached[tile]=VideoMemory24+x+y*VideoWidth;
    }
}

/**
**	Draw 32x32 tile for 32 bpp video modes with cache support.
**
**	@param tile	Tile number to draw.
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
local void MapDraw32Tile32(int tile,int x,int y)
{
    // FIXME: (johns) Why turned off?
    if( 0 && TileCached[tile] ) {
	VideoDraw32Tile32Cached(TileCached[tile],x,y);
    } else {
	VideoDraw32Tile32(TheMap.Tiles[tile],x,y);
	TileCached[tile]=VideoMemory32+x+y*VideoWidth;
    }
}

/**
**	Draw tile.
**
**	@param tile	Tile number to draw.
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
#ifdef USE_OPENGL
local void MapDrawTileOpenGL(int tile,int x,int y)
{
    GLfloat sx,ex,sy,ey;
    GLfloat stx,etx,sty,ety;
    Graphic *g;
    int t;

    g=TheMap.TileData;
    sx=(GLfloat)x/VideoWidth;
    ex=sx+(GLfloat)TileSizeX/VideoWidth;
    ey=1.0f-(GLfloat)y/VideoHeight;
    sy=ey-(GLfloat)TileSizeY/VideoHeight;

    t=tile%(g->Width/TileSizeX);
    stx=(GLfloat)t*TileSizeX/g->Width*g->TextureWidth;
    etx=(GLfloat)(t*TileSizeX+TileSizeX)/g->Width*g->TextureWidth;
    t=tile/(g->Width/TileSizeX);
    sty=(GLfloat)t*TileSizeY/g->Height*g->TextureHeight;
    ety=(GLfloat)(t*TileSizeY+TileSizeY)/g->Height*g->TextureHeight;

    glBindTexture(GL_TEXTURE_2D, g->TextureNames[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(stx, 1.0f-ety);
    glVertex3f(sx, sy, 0.0f);
    glTexCoord2f(stx, 1.0f-sty);
    glVertex3f(sx, ey, 0.0f);
    glTexCoord2f(etx, 1.0f-sty);
    glVertex3f(ex, ey, 0.0f);
    glTexCoord2f(etx, 1.0f-ety);
    glVertex3f(ex, sy, 0.0f);
    glEnd();
}
#endif

#endif	// } USE_SMART_TILECACHE

/*----------------------------------------------------------------------------
--	Without Cache
----------------------------------------------------------------------------*/

#if !defined(USE_TILECACHE) && !defined(USE_SMART_TILECACHE)	// {

/**
**	Draw 32x32 tile for 8 bpp video modes with cache support.
**
**	@param tile	Tile number to draw.
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
local void MapDraw8Tile32(int tile,int x,int y)
{
    VideoDraw8Tile32(TheMap.Tiles[tile],x,y);
}

/**
**	Draw 32x32 tile for 16 bpp video modes with cache support.
**
**	@param tile	Tile number to draw.
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
local void MapDraw16Tile32(int tile,int x,int y)
{
    VideoDraw16Tile32(TheMap.Tiles[tile],x,y);
}

/**
**	Draw 32x32 tile for 24 bpp video modes with cache support.
**
**	@param tile	Tile number to draw.
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
local void MapDraw24Tile32(int tile,int x,int y)
{
    VideoDraw24Tile32(TheMap.Tiles[tile],x,y);
}

/**
**	Draw 32x32 tile for 32 bpp video modes with cache support.
**
**	@param tile	Tile number to draw.
**	@param x	X position into video memory
**	@param y	Y position into video memory
*/
local void MapDraw32Tile32(int tile,int x,int y)
{
    VideoDraw32Tile32(TheMap.Tiles[tile],x,y);
}

#endif	// }  !defined(USE_TILECACHE) && !defined(USE_SMART_TILECACHE)

/*----------------------------------------------------------------------------
--	Global functions
----------------------------------------------------------------------------*/

/**
**	Called if color cycled.
**	Must mark color cycled tiles as dirty.
*/
global void MapColorCycle(void)
{
    int i;

#ifdef USE_TILECACHE
    TileCache* cache;

    // FIXME: the easy version just remove color cycling tiles from cache.
    for( i=0; i<TheMap.TileCount; ++i ) {
	if( TheMap.Tileset->TileTypeTable[i]==TileTypeWater ) {
	    if( (cache=TileCached[i]) ) {
		DebugLevel3("Flush\n");
		dl_remove(&cache->DlNode);
		dl_insert_last(TileCacheLRU,&cache->DlNode);
		cache->Tile=0;
		TileCached[i]=NULL;
	    }
	}
    }
#endif

    if( VideoBpp==15 || VideoBpp==16 ) {
	//
	//	Convert 16 bit pixel table into two 32 bit tables.
	//
	for( i=0; i<256; ++i ) {
	    PixelsLow[i]=((VMemType16*)TheMap.TileData->Pixels)[i]&0xFFFF;
	    PixelsHigh[i]=(((VMemType16*)TheMap.TileData->Pixels)[i]&0xFFFF)
		    <<16;
	}
    }
}

/**
**	Mark position inside screenmap be drawn for next display update.
**
**	@param x	X map tile position of point in Map to be marked.
**	@param y	Y map tile position of point in Map to be marked.
**
**	@return		True if inside and marked, false otherwise.
*/
global int MarkDrawPosMap(int x, int y)
{
    /* NOTE: latimerius: MarkDrawPosMap() in split screen environment
     * schedules RedrawMap if (x,y) is visible inside *any* of the existing
     * viewports.  Is this OK, johns?  Do you think it would pay having
     * RedrawViewport0, RedrawViewport1 etc. variables and redraw just
     * vp's that actually need redrawing?  We should evaluate this.
     */
    if (GetViewport(x, y) != -1) {
	MustRedraw |= RedrawMap;
	return 1;
    }
    return 0;
}

global int MapAreaVisibleInViewport(int v, int sx, int sy, int ex, int ey)
{
    const Viewport* view;

    view = &TheUI.VP[v];
    return sx >= view->MapX && sy >= view->MapY
	&& ex < view->MapX + view->MapWidth
	&& ey < view->MapY + view->MapHeight;
}

local inline int PointInViewport(int v, int x, int y)
{
    const Viewport* view;

    view = &TheUI.VP[v];
    return view->MapX <= x && x < view->MapX + view->MapWidth
	&& view->MapY <= y && y < view->MapY + view->MapHeight;
}

global int AnyMapAreaVisibleInViewport (int v, int sx, int sy, int ex, int ey)
{
    // FIXME: Can be faster written
    return PointInViewport (v,sx,sy) || PointInViewport (v,sx,ey)
	    || PointInViewport (v,ex,sy) || PointInViewport (v,ex,ey);
}

/**
**	Mark overlapping area with screenmap be drawn for next display update.
**
**	@param sx	X map tile position of area in Map to be marked.
**	@param sy	Y map tile position of area in Map to be marked.
**	@param ex	X map tile position of area in Map to be marked.
**	@param ey	Y map tile position of area in Map to be marked.
**
**	@return		True if overlapping and marked, false otherwise.
**
**	@see MustRedrawRow @see MustRedrawTile.
*/
global int MarkDrawAreaMap(int sx, int sy, int ex, int ey)
{
    if (MapTileGetViewport (sx,sy) != -1 || MapTileGetViewport (ex,ey) != -1) {
	MustRedraw |= RedrawMap;
	return 1;
    }
    return 0;
}

/**
**	Enable entire map be drawn for next display update.
*/
global void MarkDrawEntireMap(void)
{
#ifdef NEW_MAPDRAW
    int i;

    for( i=0; i<MapHeight; ++i ) {
	MustRedrawRow[i]=1;
    }
    for( i=0; i<MapHeight*MapWidth; ++i ) {
	MustRedrawTile[i]=1;
    }
#endif

#ifdef NEW_DECODRAW
// Hmm.. remove levels directly might not be a good idea, but it is faster ;)
//  DecorationRemoveLevels( LevCarLow, LevSkyHighest );
//  DecorationMark( mapdeco );
#endif

    MustRedraw|=RedrawMap;
}


/**
**	Draw the map backgrounds.
**
**	@param x	Map viewpoint x position.
**	@param y	Map viewpoint y position.
**
** StephanR: variables explained below for screen:<PRE>
** *---------------------------------------*
** |					   |
** |	      *-----------------------*	   |<-TheUi.MapY,dy (in pixels)
** |	      |	  |   |	  |   |	  |   |	   |  |
** |	      |	  |   |	  |   |	  |   |	   |  |
** |	      |---+---+---+---+---+---|	   |  |
** |	      |	  |   |	  |   |	  |   |	   |  |MapHeight (in tiles)
** |	      |	  |   |	  |   |	  |   |	   |  |
** |	      |---+---+---+---+---+---|	   |  |
** |	      |	  |   |	  |   |	  |   |	   |  |
** |	      |	  |   |	  |   |	  |   |	   |  |
** |	      *-----------------------*	   |<-ey,TheUI.MapEndY (in pixels)
** |					   |
** |					   |
** *---------------------------------------*
**	      ^			      ^
**	    dx|-----------------------|ex,TheUI.MapEndX (in pixels)
** TheUI.MapX	 MapWidth (in tiles)
** (in pixels)
** </PRE>
*/

global void DrawMapBackgroundInViewport (int v, int x, int y)
{
    int sx;
    int sy;
    int dx;
    int ex;
    int dy;
    int ey;
    const char* redraw_row;
    const char* redraw_tile;
#ifdef TIMEIT
    u_int64_t sv=rdtsc();
    u_int64_t ev;
    static long mv=9999999;
#endif
#ifdef USE_SMART_TILECACHE
    memset(TileCached,0,sizeof(TileCached));
#endif

    redraw_row=MustRedrawRow;		// flags must redraw or not
    redraw_tile=MustRedrawTile;

    ex=TheUI.VP[v].EndX;
    sy=y*TheMap.Width;
    dy=TheUI.VP[v].Y;
    ey=TheUI.VP[v].EndY;

    while( dy<ey ) {
	if( *redraw_row++ ) {		// row must be redrawn
	    sx=x+sy;
	    dx=TheUI.VP[v].X;
	    while( dx<ex ) {
		//
		//	draw only tiles which must be drawn
		//
		if( *redraw_tile++ ) {
		    // FIXME: unexplored fields could be drawn faster
		    MapDrawTile(TheMap.Fields[sx].SeenTile,dx,dy);

		// StephanR: debug-mode denote tiles that are redrawn
		#if NEW_MAPDRAW > 1
		  VideoDrawRectangle( redraw_tile[-1] > 1
				      ? ColorWhite
				      : ColorRed,
				      dx, dy, 32, 32 );
		#endif
		}
		++sx;
		dx+=TileSizeX;
	    }
	} else {
	    redraw_tile += TheUI.VP[v].MapWidth;
	}
	sy+=TheMap.Width;
	dy+=TileSizeY;
    }

#if defined(HIERARCHIC_PATHFINDER) && defined(GRID)
    {
    int xmax = x + (TheUI.VP[v].EndX-TheUI.VP[v].X)/TileSizeX;
    int xmin = x;
    int ymax = y + (TheUI.VP[v].EndY-TheUI.VP[v].Y)/TileSizeY;
    int ymin = y;
    int AreaWidth = AreaGetWidth ();
    int AreaHeight = AreaGetHeight ();
    for ( ; x <= xmax; x++)  {
	if (x%AreaWidth == 0) {
	    int	xx = TheUI.VP[v].X + TileSizeX * (x - xmin) - 1;
	    VideoDrawLineClip (ColorRed, xx, TheUI.VP[v].Y, xx, TheUI.VP[v].EndY);
	}
    }
    for ( ; y <= ymax; y++)  {
	if (y%AreaHeight == 0) {
	    int	yy = TheUI.VP[v].Y + TileSizeY * (y - ymin) - 1;
	    VideoDrawLineClip (ColorRed, TheUI.VP[v].X, yy, TheUI.VP[v].EndX, yy);
	}
    }

    }
#endif

#ifdef TIMEIT
    ev=rdtsc();
    sx=(ev-sv);
    if( sx<mv ) {
	mv=sx;
    }

    DebugLevel1("%ld %ld %3ld\n" _C_ (long)sx _C_ mv _C_ (sx*100)/mv);
#endif
}

#ifdef NEW_DECODRAW
/**
**      Decoration redraw function that will redraw map for set clip rectangle
**
**      @param dummy_data  should be NULL; needed to make callback possible
*/
local void mapdeco_draw( void *dummy_data )
{
  MapField *src;
  int x, y, w, h, nextline;

  extern int ClipX1;
  extern int ClipY1;
  extern int ClipX2;
  extern int ClipY2;

//  VideoDrawRectangle( ColorWhite, ClipX1, ClipY1,
//                      ClipX2-ClipX1+1, ClipY2-ClipY1+1 );
  w = (ClipX2 - ClipX1) / TileSizeX + 1;
  h = (ClipY2 - ClipY1) / TileSizeY + 1;
  x = (ClipX1 - TheUI.MapX) / TileSizeX;
  y = (ClipY1 - TheUI.MapY) / TileSizeY;
  src = TheMap.Fields + MapX + x + (MapY + y) * TheMap.Width;
  x = TheUI.MapX + x * TileSizeX;
  y = TheUI.MapY + y * TileSizeY;
  nextline=TheMap.Width-w;
  do
  {
    int w2=w;
    do
    {
      MapDrawTile(src->SeenTile,x,y);
      x+=TileSizeX;
      src++;
    }
    while ( --w2 );
    y+=TileSizeY;
    src+=nextline;
  }
  while ( --h );
}
#endif

/**
:w
**	Initialise the fog of war.
**	Build tables, setup functions.
**
**	@see VideoBpp
*/
void InitMap(void)
{
#ifdef USE_OPENGL
    MapDrawTile=MapDrawTileOpenGL;
#else

#ifdef NEW_DECODRAW
// StephanR: Using the decoration mechanism we need to support drawing tiles
// clipped, as by only updating a small part of the tile, we don't have to
// redraw items overlapping the remaining part of the tile.. it might need
// some performance increase though, but atleast the video dependent depth is
// not done here, making the switch(VideoBpp) obsolete..
  MapDrawTile=MapDrawXXTileClip;
  VideoDrawTile=VideoDrawXXTileClip;
  mapdeco = DecorationAdd( NULL /* no data to pass to */,
                           mapdeco_draw, LevGround,
                           TheUI.MapX, TheUI.MapY,
                           TheUI.MapEndX-TheUI.MapX+1,
                           TheUI.MapEndY-TheUI.MapY+1 );

#else
    switch( VideoBpp ) {
	case  8:
	    VideoDrawTile=VideoDraw8Tile32;
#if !defined(USE_TILECACHE) && !defined(USE_SMART_TILECACHE)
	    MapDrawTile=MapDraw8Tile32;
#else
	    MapDrawTile=MapDraw8Tile32;
#endif
	    break;
	case 15:
	case 16:
	    VideoDrawTile=VideoDraw16Tile32;
#if !defined(USE_TILECACHE) && !defined(USE_SMART_TILECACHE)
	    MapDrawTile=MapDraw16Tile32;
#else
	    MapDrawTile=MapDraw16Tile32;
#endif
	    break;
	case 24:
	    VideoDrawTile=VideoDraw24Tile32;
#if !defined(USE_TILECACHE) && !defined(USE_SMART_TILECACHE)
	    MapDrawTile=MapDraw24Tile32;
#else
	    MapDrawTile=MapDraw24Tile32;
#endif
	    break;

	case 32:
	    VideoDrawTile=VideoDraw32Tile32;
#if !defined(USE_TILECACHE) && !defined(USE_SMART_TILECACHE)
	    MapDrawTile=MapDraw32Tile32;
#else
	    MapDrawTile=MapDraw32Tile32;
#endif
	    break;

	default:
	    DebugLevel0Fn("Depth unsupported\n");
	    break;
    }
#endif

#endif
}

//@}
