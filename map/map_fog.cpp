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
/**@name map_fog.c	-	The map fog of war handling. */
//
//      (c) Copyright 1999-2003 by Lutz Sammer, Vladi Shabanski,
//								  Russell Smith, and Jimmy Salmon
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
//      $Id$

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"
#include "player.h"
#include "unittype.h"
#include "unit.h"
#include "map.h"
#include "minimap.h"
#include "ui.h"

#if defined(DEBUG) && defined(TIMEIT)
#include "rdtsc.h"
#endif

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

#ifdef DEBUG

// Use this to see FOW visibility for every tile
#define noDEBUG_FOG_OF_WAR

#define noTIMEIT						/// defined time function
#endif

	/// True if this is the fog color
#define COLOR_FOG_P(x) ((x) == 239)
	/// Solid fog color number in global palette
#define COLOR_FOG (0)

/**
**		Do unroll 8x
**
**		@param x		Index passed to UNROLL2 incremented by 2.
*/
#define UNROLL8(x)		\
	UNROLL2((x) + 0);		\
	UNROLL2((x) + 2);		\
	UNROLL2((x) + 4);		\
	UNROLL2((x) + 6)

/**
**		Do unroll 16x
**
**		@param x		Index passed to UNROLL8 incremented by 8.
*/
#define UNROLL16(x)		\
	UNROLL8((x) + 0);		\
	UNROLL8((x) + 8)

/**
**		Do unroll 32x
**
**		@param x		Index passed to UNROLL8 incremented by 8.
*/
#define UNROLL32(x)		\
	UNROLL8((x) + 0);		\
	UNROLL8((x) + 8);		\
	UNROLL8((x) + 16);		\
	UNROLL8((x) + 24)

/*----------------------------------------------------------------------------
--		Variables
----------------------------------------------------------------------------*/

global int OriginalFogOfWar;				/// Use original style fog of war
global int FogOfWarOpacity;					/// Fog of war Opacity.

#define MapFieldCompletelyVisible   0x0001  /// Field completely visible
#define MapFieldPartiallyVisible	0x0002  /// Field partially visible

/**
**		Mapping for fog of war tiles.
*/
local const int FogTable[16] = {
	 0,11,10, 2,  13, 6, 0, 3,  12, 0, 4, 1,  8, 9, 7, 0,
};

global unsigned char* VisionTable[3];
global int* VisionLookup;

local unsigned char* VisibleTable;

#ifdef USE_SDL_SURFACE
local SDL_Surface* OnlyFogSurface;
local void (*VideoDrawUnexplored)(const int, int, int);
local void (*VideoDrawFog)(const int, int, int);
local void (*VideoDrawOnlyFog)(int, int);
#else
/**
**		Draw unexplored area function pointer. (display and video mode independ)
*/
local void (*VideoDrawUnexplored)(const GraphicData*, int, int);

/**
**		Draw fog of war function pointer. (display and video mode independ)
*/
local void (*VideoDrawFog)(const GraphicData*, int, int);

/**
**		Draw only fog of war function pointer. (display and video mode independ)
*/
local void (*VideoDrawOnlyFog)(const GraphicData*, int x, int y);
#endif

/*----------------------------------------------------------------------------
--		Functions
----------------------------------------------------------------------------*/

/**
**		Find the Number of Units that can see this square using a long
**		lookup. So when a 225 Viewed square can be calculated properly.
**
**		@param player		Player to mark sight.
**		@param tx		X center position.
**		@param ty		Y center position.
**
**		@return				Number of units that can see this square.
*/
local int LookupSight(const Player* player, int tx, int ty)
{
	int i;
	int visiblecount;
	int range;
	int mapdistance;
	Unit* unit;

	visiblecount=0;
	for (i = 0; i < player->TotalNumUnits; ++i) {
		unit = player->Units[i];
		range = unit->CurrentSightRange;
		mapdistance = MapDistanceToUnit(tx,ty,unit);
		if (mapdistance <= range) {
			++visiblecount;
		}
		if ((tx >= unit->X && tx < unit->X + unit->Type->TileWidth &&
				mapdistance == range + 1) || (ty >= unit->Y &&
				ty < unit->Y + unit->Type->TileHeight &&
				mapdistance == range + 1)) {
			--visiblecount;
		}
		if (visiblecount >= 255) {
			return 255;
		}
	}
	return visiblecount;
}

/**
**		Find out if a field is seen (By player, or by shared vision)
**		This function will return > 1 with no fog of war.
**
**		@param player		Player to check for.
**		@param x		X tile to check.
**		@param y		Y tile to check.
**
**		@return				0 unexplored, 1 explored, > 1 visible.
*/
global unsigned char IsTileVisible(const Player* player, int x, int y)
{
	int i;
	unsigned char visiontype;
	unsigned char* visible;

	visible = TheMap.Fields[y * TheMap.Width + x].Visible;
	visiontype = visible[player->Player];

	if (visiontype > 1) {
		return visiontype;
	}
	if (!player->SharedVision) {
		if (visiontype) {
			return visiontype + TheMap.NoFogOfWar;
		}
		return 0;
	}

	for (i = 0; i < PlayerMax ; ++i) {
		if (player->SharedVision & (1 << i) &&
				(Players[i].SharedVision & (1 << player->Player))) {
			if (visible[i] > 1) {
				return 2;
			}
			visiontype |= visible[i];
		}
	}
	if (visiontype) {
		return visiontype + TheMap.NoFogOfWar;
	}
	return 0;
}

/**
**		Mark a tile's sight. (Explore and make visible.)
**
**		@param player		Player to mark sight.
**		@param x		X tile to mark.
**		@param y		Y tile to mark.
*/
global void MapMarkTileSight(const Player* player, int x, int y)
{
	unsigned char v;

	v = TheMap.Fields[x + y * TheMap.Width].Visible[player->Player];
	switch (v) {
		case 0:		// Unexplored
		case 1:		// Unseen
			//  When there is NoFogOfWar only unexplored tiles are marked.
			if ((!TheMap.NoFogOfWar) || (v == 0)) {
				UnitsOnTileMarkSeen(player, x, y, 0);
			}
			v = 2;
			TheMap.Fields[x + y * TheMap.Width].Visible[player->Player] = v;
			if (IsTileVisible(ThisPlayer, x, y) > 1) {
				MapMarkSeenTile(x, y);
			}
			return;
		case 255:		// Overflow
			DebugLevel0Fn("Visible overflow (Player): %d\n" _C_ player->Player);
			break;
		default:		// seen -> seen
			++v;
			break;
	}
	TheMap.Fields[x + y * TheMap.Width].Visible[player->Player] = v;
}

/**
**		Unmark a tile's sight. (Explore and make visible.)
**
**		@param player		Player to mark sight.
**		@param x		X tile to mark.
**		@param y		Y tile to mark.
*/
global void MapUnmarkTileSight(const Player* player, int x, int y)
{
	unsigned char v;

	v = TheMap.Fields[x + y * TheMap.Width].Visible[player->Player];
	switch (v) {
		case 255:
			// FIXME: (mr-russ) Lookupsight is broken :(
			DebugCheck(1);
			v = LookupSight(player, x, y);
			DebugCheck(v < 254);
			break;
		case 0:		// Unexplored
		case 1:
			// We are at minimum, don't do anything shouldn't happen.
			DebugCheck(1);
			break;
		case 2:
			//  When there is NoFogOfWar units never get unmarked.
			if (!TheMap.NoFogOfWar) {
				UnitsOnTileUnmarkSeen(player, x, y, 0);
			}
			// Check visible Tile, then deduct...
			if (IsTileVisible(ThisPlayer, x, y) > 1) {
				MapMarkSeenTile(x, y);
			}
		default:		// seen -> seen
			v--;
			break;
	}
	TheMap.Fields[x + y * TheMap.Width].Visible[player->Player] = v;
}

/**
**	Mark a tile for cloak detection.
**
**	@param player	Player to mark sight.
**	@param x	X tile to mark.
**	@param y	Y tile to mark.
*/
global void MapMarkTileDetectCloak(const Player* player, int x, int y)
{
	unsigned char v;

	v = TheMap.Fields[x + y * TheMap.Width].VisCloak[player->Player];
	if (v == 0) {
		UnitsOnTileMarkSeen(player, x, y, 1);
	}
	DebugCheck(v == 255);
	++v;
	TheMap.Fields[x + y * TheMap.Width].VisCloak[player->Player] = v;
}

/**
**	Unmark a tile for cloak detection.
**
**	@param player	Player to mark sight.
**	@param x	X tile to mark.
**	@param y	Y tile to mark.
*/
global void MapUnmarkTileDetectCloak(const Player* player, int x, int y)
{
	unsigned char v;

	v = TheMap.Fields[x + y * TheMap.Width].VisCloak[player->Player];
	DebugCheck(v == 0);
	if (v == 1) {
		UnitsOnTileUnmarkSeen(player, x, y, 1);
	}
	--v;
	TheMap.Fields[x + y * TheMap.Width].VisCloak[player->Player] = v;
}

/**
**		Mark the sight of unit. (Explore and make visible.)
**
**		@param player		player to mark the sight for (not unit owner)
**		@param x		x location to mark
**		@param y		y location to mark
**		@param w		width to mark, in square
**		@param h		height to mark, in square
**		@param range		Radius to mark.
**		@param marker		Function to mark or unmark sight
*/
global void MapSight(const Player* player, int x, int y, int w, int h, int range,
	void (*marker)(const Player*, int, int))
{
	int mx;
	int my;
	int cx[4];
	int cy[4];
	int steps;
	int cycle;
	int p;

	// Mark as seen
	if (!range) {						// zero sight range is zero sight range
		DebugLevel0Fn("Zero sight range\n");
		return;
	}

	p = player->Player;

	// Mark Horizontal sight for unit
	for (mx = x - range; mx < x + range + w; ++mx) {
		for (my = y; my < y + h; ++my) {
			if (mx >= 0 && mx < TheMap.Width) {
				marker(player, mx, my);
			}
		}
	}

	// Mark vertical sight for unit (don't remark self) (above unit)
	for (my = y - range; my < y; ++my) {
		for (mx = x; mx < x + w; ++mx) {
			if (my >= 0 && my < TheMap.Width) {
				marker(player, mx, my);
			}
		}
	}

	// Mark vertical sight for unit (don't remark self) (below unit)
	for (my = y + h; my < y + range + h; ++my) {
		for (mx = x; mx < x + w; ++mx) {
			if (my >= 0 && my < TheMap.Width) {
				marker(player, mx, my);
			}
		}
	}

	// Now the cross has been marked, need to use loop to mark in circle
	steps = 0;
	while (VisionTable[0][steps] <= range) {
		// 0 - Top right Quadrant
		cx[0] = x + w - 1;
		cy[0] = y - VisionTable[0][steps];
		// 1 - Top left Quadrant
		cx[1] = x;
		cy[1] = y - VisionTable[0][steps];
		// 2 - Bottom Left Quadrant
		cx[2] = x;
		cy[2] = y + VisionTable[0][steps] + h - 1;
		// 3 - Bottom Right Quadrant
		cx[3] = x + w - 1;
		cy[3] = y + VisionTable[0][steps] + h - 1;
		// loop for steps
		++steps;		// Increment past info pointer
		while (VisionTable[1][steps] != 0 || VisionTable[2][steps] != 0) {
			// Loop through for repeat cycle
			cycle = 0;
			while (cycle++ < VisionTable[0][steps]) {
				cx[0] += VisionTable[1][steps];
				cy[0] += VisionTable[2][steps];
				cx[1] -= VisionTable[1][steps];
				cy[1] += VisionTable[2][steps];
				cx[2] -= VisionTable[1][steps];
				cy[2] -= VisionTable[2][steps];
				cx[3] += VisionTable[1][steps];
				cy[3] -= VisionTable[2][steps];
				if (cx[0] < TheMap.Width && cy[0] >= 0) {
					marker(player, cx[0], cy[0]);
				}
				if (cx[1] >= 0 && cy[1] >= 0) {
					marker(player, cx[1], cy[1]);
				}
				if (cx[2] >= 0 && cy[2] < TheMap.Height) {
					marker(player, cx[2], cy[2]);
				}
				if (cx[3] < TheMap.Width && cy[3] < TheMap.Height) {
					marker(player, cx[3], cy[3]);
				}
			}
			++steps;
		}
	}
}

/**
**		Update fog of war.
*/
global void UpdateFogOfWarChange(void)
{
	int x;
	int y;
	int w;

	DebugLevel0Fn("\n");
	//
	//		Mark all explored fields as visible again.
	//
	if (TheMap.NoFogOfWar) {
		w = TheMap.Width;
		for (y = 0; y < TheMap.Height; ++y) {
			for (x = 0; x < TheMap.Width; ++x) {
				if (IsMapFieldExplored(ThisPlayer, x, y)) {
					MapMarkSeenTile(x, y);
				}
			}
		}
	}
	//
	//	Global seen recount.
	//
	for (x = 0; x < NumUnits; ++x) {
		UnitCountSeen(Units[x]);
	}
	MarkDrawEntireMap();
}

/*----------------------------------------------------------------------------
--		Draw fog solid
----------------------------------------------------------------------------*/

#ifdef USE_SDL_SURFACE

/**
**		Fast draw solid fog of war 16x16 tile for 8 bpp video modes.
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory.
**		@param y		Y position into video memory.
*/
global void VideoDrawFogSDL(const int tile, int x, int y)
{
	int tilepitch;
	SDL_Rect srect;
	SDL_Rect drect;

	tilepitch = TheMap.TileGraphic->Width / TileSizeX;

	srect.x = TileSizeX * (tile % tilepitch);
	srect.y = TileSizeY * (tile / tilepitch);
	srect.w = TileSizeX;
	srect.h = TileSizeY;

	drect.x = x;
	drect.y = y;

	SDL_SetAlpha(TheMap.TileGraphic->Surface, SDL_SRCALPHA | SDL_RLEACCEL, FogOfWarOpacity);
	SDL_BlitSurface(TheMap.TileGraphic->Surface, &srect, TheScreen, &drect);
	SDL_SetAlpha(TheMap.TileGraphic->Surface, SDL_RLEACCEL, 0);
}

global void VideoDrawUnexploredSDL(const int tile, int x, int y)
{
	int tilepitch;
	SDL_Rect srect;
	SDL_Rect drect;

	tilepitch = TheMap.TileGraphic->Width / TileSizeX;

	srect.x = TileSizeX * (tile % tilepitch);
	srect.y = TileSizeY * (tile / tilepitch);
	srect.w = TileSizeX;
	srect.h = TileSizeY;

	drect.x = x;
	drect.y = y;

	SDL_BlitSurface(TheMap.TileGraphic->Surface, &srect, TheScreen, &drect);
}

global void VideoDrawOnlyFogSDL(int x, int y)
{
	SDL_Rect drect;

	drect.x = x;
	drect.y = y;

	SDL_BlitSurface(OnlyFogSurface, NULL, TheScreen, &drect);
	//VideoFillTransRectangleClip(ColorBlack, x, y, TileSizeX, TileSizeY, FogOfWarOpacity);
}

#else

// Routines for 8 bit displays .. --------------------------------------------

/**
**		Fast draw solid fog of war 16x16 tile for 8 bpp video modes.
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory.
**		@param y		Y position into video memory.
*/
global void VideoDraw8Fog16Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType8* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory8 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {				\
			dp[x] = ((VMemType8*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);

		UNROLL16(0);

		sp += TileSizeX;
		dp += da;

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 1);

		UNROLL16(0);

		sp += TileSizeX;
		dp += da;
	}
}

/**
**		Fast draw solid 100% fog of war 16x16 tile for 8 bpp video modes.
**
**		100% fog of war -- i.e. raster		10101.
**										01010 etc...
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw8OnlyFog16Solid(const GraphicData* data __attribute__((unused)),
	int x, int y)
{
	const VMemType8* gp;
	VMemType8* dp;
	int da;

	dp = VideoMemory8 + x + y * VideoWidth;
	gp = dp + VideoWidth * TileSizeX;
	da = VideoWidth;
	while (dp < gp) {
#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 0] = ((VMemType8*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL16(0);
		dp += da;

#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 1] = ((VMemType8*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL16(0);
		dp += da;
	}
}

/**
**		Fast draw solid unexplored 16x16 tile for 8 bpp video modes.
**
**		@param data		pointer to tile graphic data
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw8Unexplored16Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType8* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory8 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {				\
			dp[x] = ((VMemType8*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);		\
		UNROLL1(x + 1);

		UNROLL16(0);
		sp += TileSizeX;
		dp += da;

		UNROLL16(0);
		sp += TileSizeX;
		dp += da;

	}
}

/**
**		Fast draw solid fog of war 32x32 tile for 8 bpp video modes.
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory.
**		@param y		Y position into video memory.
*/
global void VideoDraw8Fog32Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType8* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory8 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {				\
			dp[x] = ((VMemType8*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);

		UNROLL32(0);

		sp += TileSizeX;
		dp += da;

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 1);

		UNROLL32(0);

		sp += TileSizeX;
		dp += da;
	}
}

/**
**		Fast draw solid 100% fog of war 32x32 tile for 8 bpp video modes.
**
**		100% fog of war -- i.e. raster		10101.
**										01010 etc...
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw8OnlyFog32Solid(const GraphicData* data __attribute__((unused)),
	int x, int y)
{
	const VMemType8* gp;
	VMemType8* dp;
	int da;

	dp = VideoMemory8 + x + y * VideoWidth;
	gp = dp + VideoWidth * TileSizeX;
	da = VideoWidth;
	while (dp < gp) {
#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 0] = ((VMemType8*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL32(0);
		dp += da;

#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 1] = ((VMemType8*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL32(0);
		dp += da;
	}
}

/**
**		Fast draw solid unexplored 32x32 tile for 8 bpp video modes.
**
**		@param data		pointer to tile graphic data
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw8Unexplored32Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType8* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory8 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {				\
			dp[x] = ((VMemType8*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);		\
		UNROLL1(x + 1);

		UNROLL32(0);
		sp += TileSizeX;
		dp += da;

		UNROLL32(0);
		sp += TileSizeX;
		dp += da;

	}
}

// Routines for 16 bit displays .. -------------------------------------------

/**
**		Fast draw solid fog of war 16x16 tile for 16 bpp video modes.
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory.
**		@param y		Y position into video memory.
*/
global void VideoDraw16Fog16Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType16* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory16 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {				\
			dp[x] = ((VMemType16*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);

		UNROLL16(0);

		sp += TileSizeX;
		dp += da;

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 1);

		UNROLL16(0);

		sp += TileSizeX;
		dp += da;
	}
}

/**
**		Fast draw solid 100% fog of war 16x16 tile for 16 bpp video modes.
**
**		100% fog of war -- i.e. raster		10101.
**										01010 etc...
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw16OnlyFog16Solid(const GraphicData* data __attribute__((unused)),
	int x, int y)
{
	const VMemType16* gp;
	VMemType16* dp;
	int da;

	dp = VideoMemory16 + x + y * VideoWidth;
	gp = dp + VideoWidth * TileSizeX;
	da = VideoWidth;
	while (dp < gp) {
#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 0] = ((VMemType16*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL16(0);
		dp += da;

#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 1] = ((VMemType16*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL16(0);
		dp += da;
	}
}

/**
**		Fast draw solid unexplored 16x16 tile for 16 bpp video modes.
**
**		@param data		pointer to tile graphic data
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw16Unexplored16Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType16* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory16 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {				\
			dp[x] = ((VMemType16*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);		\
		UNROLL1(x + 1);

		UNROLL16(0);
		sp += TileSizeX;
		dp += da;

		UNROLL16(0);
		sp += TileSizeX;
		dp += da;
	}
}

/**
**		Fast draw solid fog of war 32x32 tile for 16 bpp video modes.
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory.
**		@param y		Y position into video memory.
*/
global void VideoDraw16Fog32Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType16* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory16 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {				\
			dp[x] = ((VMemType16*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);

		UNROLL32(0);

		sp += TileSizeX;
		dp += da;

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 1);

		UNROLL32(0);

		sp += TileSizeX;
		dp += da;
	}
}

/**
**		Fast draw solid 100% fog of war 32x32 tile for 16 bpp video modes.
**
**		100% fog of war -- i.e. raster		10101.
**										01010 etc...
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw16OnlyFog32Solid(const GraphicData* data __attribute__((unused)),
	int x, int y)
{
	const VMemType16* gp;
	VMemType16* dp;
	int da;

	dp = VideoMemory16 + x + y * VideoWidth;
	gp = dp + VideoWidth * TileSizeX;
	da = VideoWidth;
	while (dp < gp) {
#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 0] = ((VMemType16*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL32(0);
		dp += da;

#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 1] = ((VMemType16*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL32(0);
		dp += da;
	}
}

/**
**		Fast draw solid unexplored 32x32 tile for 16 bpp video modes.
**
**		@param data		pointer to tile graphic data
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw16Unexplored32Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType16* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory16 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {				\
			dp[x] = ((VMemType16*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);		\
		UNROLL1(x + 1);

		UNROLL32(0);
		sp += TileSizeX;
		dp += da;

		UNROLL32(0);
		sp += TileSizeX;
		dp += da;
	}
}

// Routines for 24 bit displays .. -------------------------------------------

/**
**		Fast draw solid fog of war 16x16 tile for 24 bpp video modes.
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory.
**		@param y		Y position into video memory.
*/
global void VideoDraw24Fog16Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType24* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory24 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {		\
			dp[x] = ((VMemType24*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);

		UNROLL32(0);

		sp += TileSizeX;
		dp += da;

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 1);

		UNROLL16(0);

		sp += TileSizeX;
		dp += da;
	}
}

/**
**		Fast draw solid 100% fog of war 16x16 tile for 24 bpp video modes.
**
**		100% fog of war -- i.e. raster		10101.
**										01010 etc...
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw24OnlyFog16Solid(const GraphicData* data __attribute__((unused)),
	int x, int y)
{
	const VMemType24* gp;
	VMemType24* dp;
	int da;

	dp = VideoMemory24 + x + y * VideoWidth;
	gp = dp + VideoWidth * TileSizeX;
	da = VideoWidth;
	while (dp < gp) {
#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 0] = ((VMemType24*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL16(0);
		dp += da;

#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 1] = ((VMemType24*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL16(0);
		dp += da;
	}
}

/**
**		Fast draw solid unexplored 16x16 tile for 24 bpp video modes.
**
**		@param data		pointer to tile graphic data
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw24Unexplored16Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType24* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory24 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {				\
			dp[x] = ((VMemType24*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);		\
		UNROLL1(x + 1);

		UNROLL16(0);
		sp += TileSizeX;
		dp += da;

		UNROLL16(0);
		sp += TileSizeX;
		dp += da;
	}
}

/**
**		Fast draw solid fog of war 32x32 tile for 24 bpp video modes.
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory.
**		@param y		Y position into video memory.
*/
global void VideoDraw24Fog32Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType24* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory24 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {		\
			dp[x] = ((VMemType24*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);

		UNROLL32(0);

		sp += TileSizeX;
		dp += da;

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 1);

		UNROLL32(0);

		sp += TileSizeX;
		dp += da;
	}
}

/**
**		Fast draw solid 100% fog of war 32x32 tile for 24 bpp video modes.
**
**		100% fog of war -- i.e. raster		10101.
**										01010 etc...
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw24OnlyFog32Solid(const GraphicData* data __attribute__((unused)),
	int x, int y)
{
	const VMemType24* gp;
	VMemType24* dp;
	int da;

	dp = VideoMemory24 + x + y * VideoWidth;
	gp = dp + VideoWidth * TileSizeX;
	da = VideoWidth;
	while (dp < gp) {
#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 0] = ((VMemType24*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL32(0);
		dp += da;

#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 1] = ((VMemType24*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL32(0);
		dp += da;
	}
}

/**
**		Fast draw solid unexplored 32x32 tile for 24 bpp video modes.
**
**		@param data		pointer to tile graphic data
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw24Unexplored32Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType24* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory24 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {				\
			dp[x] = ((VMemType24*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);		\
		UNROLL1(x + 1);

		UNROLL32(0);
		sp += TileSizeX;
		dp += da;

		UNROLL32(0);
		sp += TileSizeX;
		dp += da;
	}
}

// Routines for 32 bit displays .. -------------------------------------------

/**
**		Fast draw solid fog of war 16x16 tile for 32 bpp video modes.
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory.
**		@param y		Y position into video memory.
*/
global void VideoDraw32Fog16Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType32* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory32 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {		\
			dp[x] = ((VMemType16*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);

		UNROLL16(0);

		sp += TileSizeX;
		dp += da;

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 1);

		UNROLL16(0);

		sp += TileSizeX;
		dp += da;
	}
}

/**
**		Fast draw solid 100% fog of war 16x16 tile for 32 bpp video modes.
**
**		100% fog of war -- i.e. raster		10101.
**										01010 etc...
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw32OnlyFog16Solid(const GraphicData* data __attribute__((unused)),
	int x, int y)
{
	const VMemType32* gp;
	VMemType32* dp;
	int da;

	dp = VideoMemory32 + x + y * VideoWidth;
	gp = dp + VideoWidth * TileSizeX;
	da = VideoWidth;
	while (dp < gp) {
#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 0] = ((VMemType32*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL16(0);
		dp += da;

#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 1] = ((VMemType32*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL16(0);
		dp += da;
	}
}

/**
**		Fast draw solid unexplored 16x16 tile for 32 bpp video modes.
**
**		@param data		pointer to tile graphic data
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw32Unexplored16Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType32* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory32 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {				\
			dp[x] = ((VMemType32*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);		\
		UNROLL1(x + 1);

		UNROLL16(0);
		sp += TileSizeX;
		dp += da;

		UNROLL16(0);
		sp += TileSizeX;
		dp += da;
	}
}

/**
**		Fast draw solid fog of war 32x32 tile for 32 bpp video modes.
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory.
**		@param y		Y position into video memory.
*/
global void VideoDraw32Fog32Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType32* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory32 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {		\
			dp[x] = ((VMemType16*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);

		UNROLL32(0);

		sp += TileSizeX;
		dp += da;

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 1);

		UNROLL32(0);

		sp += TileSizeX;
		dp += da;
	}
}

/**
**		Fast draw solid 100% fog of war 32x32 tile for 32 bpp video modes.
**
**		100% fog of war -- i.e. raster		10101.
**										01010 etc...
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw32OnlyFog32Solid(const GraphicData* data __attribute__((unused)),
	int x, int y)
{
	const VMemType32* gp;
	VMemType32* dp;
	int da;

	dp = VideoMemory32 + x + y * VideoWidth;
	gp = dp + VideoWidth * TileSizeX;
	da = VideoWidth;
	while (dp < gp) {
#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 0] = ((VMemType32*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL32(0);
		dp += da;

#undef UNROLL2
#define UNROLL2(x)				\
		dp[x + 1] = ((VMemType32*)TheMap.TileData->Pixels)[COLOR_FOG];
		UNROLL32(0);
		dp += da;
	}
}

/**
**		Fast draw solid unexplored 32x32 tile for 32 bpp video modes.
**
**		@param data		pointer to tile graphic data
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDraw32Unexplored32Solid(const GraphicData* data, int x, int y)
{
	const unsigned char* sp;
	const unsigned char* gp;
	VMemType32* dp;
	int da;

	sp = data;
	gp = sp + TileSizeY * TileSizeX;
	dp = VideoMemory32 + x + y * VideoWidth;
	da = VideoWidth;

	while (sp < gp) {
#undef UNROLL1
#define UNROLL1(x)		\
		if (COLOR_FOG_P(sp[x])) {				\
			dp[x] = ((VMemType32*)TheMap.TileData->Pixels)[COLOR_FOG];		\
		}

#undef UNROLL2
#define UNROLL2(x)		\
		UNROLL1(x + 0);		\
		UNROLL1(x + 1);

		UNROLL32(0);
		sp += TileSizeX;
		dp += da;

		UNROLL32(0);
		sp += TileSizeX;
		dp += da;
	}
}
#endif


#ifdef USE_OPENGL

// ------------------------------------------------------------------
// Routines for OpenGL .. -------------------------------------------
// ------------------------------------------------------------------

/**
**		Fast draw solid unexplored tile.
**
**		@param data		pointer to tile graphic data
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDrawUnexploredSolidOpenGL(
	const int tile __attribute__((unused)),
	int x __attribute__((unused)), int y __attribute__((unused)))
{
}

/**
**		Fast draw alpha fog of war Opengl.
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDrawFogAlphaOpenGL(
	const int tile __attribute__((unused)),
	int x __attribute__((unused)), int y __attribute__((unused)))
{
}

/**
**		Fast draw 100% fog of war 32x32 tile for 32 bpp video modes.
**
**		100% fog of war -- i.e. raster		10101.
**										01010 etc...
**
**		@param data		pointer to tile graphic data.
**		@param x		X position into video memory
**		@param y		Y position into video memory
*/
global void VideoDrawOnlyFogAlphaOpenGL(int x, int y)
{
	GLint sx;
	GLint ex;
	GLint sy;
	GLint ey;
	Graphic *g;

	g = TheMap.TileGraphic;
	sx = x;
	ex = sx + TileSizeX;
	ey = VideoHeight - y;
	sy = ey - TileSizeY;

	glDisable(GL_TEXTURE_2D);
	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
	glBegin(GL_QUADS);
	glVertex2i(sx, sy);
	glVertex2i(sx, ey);
	glVertex2i(ex, ey);
	glVertex2i(ex, sy);
	glEnd();
	glEnable(GL_TEXTURE_2D);
}
#endif

/*----------------------------------------------------------------------------
--		Old version correct working but not 100% original
----------------------------------------------------------------------------*/

/**
**		Draw fog of war tile.
**
**		@param sx		Offset into fields to current tile.
**		@param sy		Start of the current row.
**		@param dx		X position into video memory.
**		@param dy		Y position into video memory.
*/
local void DrawFogOfWarTile(int sx, int sy, int dx, int dy)
{
	int w;
	int tile;
	int tile2;
	int x;
	int y;

#define IsMapFieldExploredTable(x, y) \
	(VisibleTable[(y) * TheMap.Width + (x)])
#define IsMapFieldVisibleTable(x, y) \
	(VisibleTable[(y) * TheMap.Width + (x)] > 1)

	w = TheMap.Width;
	tile = tile2 = 0;
	x = sx - sy;
	y = sy / TheMap.Width;

	//
	//		Which Tile to draw for fog
	//
	// Investigate tiles around current tile
	// 1 2 3
	// 4 * 5
	// 6 7 8

	if (sy) {
		if (sx != sy) {
			if (!IsMapFieldExploredTable(x - 1, y - 1)) {
				tile2 |= 2;
				tile |= 2;
			} else if (!IsMapFieldVisibleTable(x - 1, y - 1)) {
				tile |= 2;
			}
		}
		if (!IsMapFieldExploredTable(x, y - 1)) {
			tile2 |= 3;
			tile |= 3;
		} else if (!IsMapFieldVisibleTable(x, y - 1)) {
			tile |= 3;
		}
		if (sx != sy + w - 1) {
			if (!IsMapFieldExploredTable(x + 1, y - 1)) {
				tile2 |= 1;
				tile |= 1;
			} else if (!IsMapFieldVisibleTable(x + 1, y - 1)) {
				tile |= 1;
			}
		}
	}

	if (sx != sy) {
		if (!IsMapFieldExploredTable(x - 1, y)) {
			tile2 |= 10;
			tile |= 10;
		} else if (!IsMapFieldVisibleTable(x - 1, y)) {
			tile |= 10;
		}
	}
	if (sx != sy + w - 1) {
		if (!IsMapFieldExploredTable(x + 1, y)) {
			tile2 |= 5;
			tile |= 5;
		} else if (!IsMapFieldVisibleTable(x + 1, y)) {
			tile |= 5;
		}
	}

	if (sy + w < TheMap.Height * w) {
		if (sx != sy) {
			if (!IsMapFieldExploredTable(x - 1, y + 1)) {
				tile2 |= 8;
				tile |= 8;
			} else if (!IsMapFieldVisibleTable(x - 1, y + 1)) {
				tile |= 8;
			}
		}
		if (!IsMapFieldExploredTable(x, y + 1)) {
			tile2 |= 12;
			tile |= 12;
		} else if (!IsMapFieldVisibleTable(x, y + 1)) {
			tile |= 12;
		}
		if (sx != sy + w - 1) {
			if (!IsMapFieldExploredTable(x + 1, y + 1)) {
				tile2 |= 4;
				tile |= 4;
			} else if (!IsMapFieldVisibleTable(x + 1, y + 1)) {
				tile |= 4;
			}
		}
	}

	tile = FogTable[tile];
	tile2 = FogTable[tile2];

	if (ReplayRevealMap) {
		tile2 = 0;
		tile = 0;
	}

	if (IsMapFieldVisibleTable(x, y) || ReplayRevealMap) {
		if (tile) {
#ifdef USE_SDL_SURFACE
			VideoDrawFog(tile, dx, dy);
#else
			VideoDrawFog(TheMap.Tiles[tile], dx, dy);
#endif
//			TheMap.Fields[sx].VisibleLastFrame |= MapFieldPartiallyVisible;
//		} else {
//			TheMap.Fields[sx].VisibleLastFrame |= MapFieldCompletelyVisible;
		}
	} else {
#ifdef USE_SDL_SURFACE
		VideoDrawOnlyFog(dx, dy);
#else
		VideoDrawOnlyFog(TheMap.Tiles[UNEXPLORED_TILE], dx, dy);
#endif
	}
	if (tile2) {
#ifdef USE_SDL_SURFACE
		VideoDrawUnexplored(tile2, dx, dy);
#else
		VideoDrawUnexplored(TheMap.Tiles[tile2], dx, dy);
#endif
/*
		if (tile2 == tile) {				// no same fog over unexplored
//			if (tile != 0xf) {
//				TheMap.Fields[sx].VisibleLastFrame |= MapFieldPartiallyVisible;
//			}
			tile = 0;
		}
*/
	}

#undef IsMapFieldExploredTable
#undef IsMapFieldVisibleTable
}

#ifdef HIERARCHIC_PATHFINDER
#include "pathfinder.h"
// hack
#include "../pathfinder/region_set.h"
#endif		// HIERARCHIC_PATHFINDER

/**
**		Draw the map fog of war.
**
**		@param vp		Viewport pointer.
**		@param x		Map viewpoint x position.
**		@param y		Map viewpoint y position.
*/
global void DrawMapFogOfWar(Viewport* vp, int x, int y)
{
	int sx;
	int sy;
	int dx;
	int ex;
	int dy;
	int ey;
	char* redraw_row;
	char* redraw_tile;
	int p;
	int my;
	int mx;

#ifdef TIMEIT
	u_int64_t sv = rdtsc();
	u_int64_t ev;
	static long mv = 9999999;
#endif

	redraw_row = vp->MustRedrawRow;				// flags must redraw or not
	redraw_tile = vp->MustRedrawTile;

	p = ThisPlayer->Player;

	sx = vp->MapX - 1;
	if (sx < 0) {
		sx = 0;
	}
	ex = vp->MapX + vp->MapWidth + 1;
	if (ex > TheMap.Width) {
		ex = TheMap.Width;
	}
	my = vp->MapY - 1;
	if (my < 0) {
		my = 0;
	}
	ey = vp->MapY + vp->MapHeight + 1;
	if (ey > TheMap.Height) {
		ey = TheMap.Height;
	}
	for (; my < ey; ++my) {
		for (mx = sx; mx < ex; ++mx) {
			VisibleTable[my * TheMap.Width + mx] = IsTileVisible(ThisPlayer, mx, my);
		}
	}

	ex = vp->EndX;
	sy = y * TheMap.Width;
	dy = vp->Y;
	ey = vp->EndY;

	while (dy < ey) {
		if (*redraw_row) {				// row must be redrawn
#if NEW_MAPDRAW > 1
			(*redraw_row)--;
#else
			*redraw_row = 0;
#endif
			sx = x + sy;
			dx = vp->X;
			while (dx < ex) {
				if (*redraw_tile) {
#if NEW_MAPDRAW > 1
					(*redraw_tile)--;
#else
					*redraw_tile = 0;
					mx = (dx - vp->X) / TileSizeX + vp->MapX;
					my = (dy - vp->Y) / TileSizeY + vp->MapY;
					if (VisibleTable[my * TheMap.Width + mx] || ReplayRevealMap) {
						DrawFogOfWarTile(sx, sy, dx, dy);
					} else {
#ifdef USE_OPENGL
						MapDrawTile(UNEXPLORED_TILE, dx, dy);
#else
#ifdef USE_SDL_SURFACE
						VideoFillRectangle(ColorBlack, dx, dy, TileSizeX, TileSizeY);
#else
						VideoDrawTile(TheMap.Tiles[UNEXPLORED_TILE], dx, dy);
#endif
#endif
					}
#endif

// Used to debug NEW_FOW problems
#if !defined(HIERARCHIC_PATHFINDER) && defined(DEBUG_FOG_OF_WAR)
extern int VideoDrawText(int x, int y, unsigned font, const unsigned char* text);
#define GameFont 1
					{
						char seen[7];
						int x = (dx - vp->X) / TileSizeX + vp->MapX;
						int y = (dy - vp->Y) / TileSizeY + vp->MapY;

#if 1
						//  Fog of War Vision
						//  Really long and ugly, shared and own vision:
						//  sprintf(seen,"%d(%d)",TheMap.Fields[y * TheMap.Width + x].Visible[ThisPlayer->Player],IsMapFieldVisible(ThisPlayer,x, y));
						//  Shorter version, but no shared vision:
						sprintf(seen,"%d",TheMap.Fields[y * TheMap.Width + x].Visible[ThisPlayer->Player]);
//						if (TheMap.Fields[y * TheMap.Width + x].Visible[0]) {
							VideoDrawText(dx, dy, GameFont,seen);
//						}
#else
						// Unit Distance Checks
						if (Selected[1] && Selected[0]) {
							sprintf(seen, "%d", MapDistanceBetweenUnits(Selected[0], Selected[1]));
							VideoDrawText(dx, dy, GameFont, seen);
						} else if (Selected[0]) {
							sprintf(seen, "%d", MapDistanceToUnit(x, y,Selected[0]));
							VideoDrawText(dx, dy, GameFont, seen);
						}
#endif
					}
#endif
#if defined(HIERARCHIC_PATHFINDER) && defined(DEBUG) && 0
					{
						char regidstr[8];
						char groupstr[8];
						int regid;
extern int VideoDrawText(int x, int y, unsigned font, const unsigned char* text);
#define GameFont 1

						if (PfHierShowRegIds || PfHierShowGroupIds) {
							regid = MapFieldGetRegId (
								(dx - vp->X) / TileSizeX + vp->MapX,
								(dy - vp->Y) / TileSizeY + vp->MapY);
							if (regid) {
								Region* r = RegionSetFind(regid);
								if (PfHierShowRegIds) {
									snprintf(regidstr, 8, "%d", regid);
									VideoDrawText(dx, dy, GameFont, regidstr);
								}
								if (PfHierShowGroupIds) {
									snprintf(groupstr, 8, "%d", r->GroupId);
									VideoDrawText(dx, dy + 19, GameFont, groupstr);
								}
							}
						}
					}
#endif		// HIERARCHIC_PATHFINDER
				}
				++redraw_tile;
				++sx;
				dx += TileSizeX;
			}
		} else {
			redraw_tile += vp->MapWidth;
		}
		++redraw_row;
		sy += TheMap.Width;
		dy += TileSizeY;
	}

#ifdef TIMEIT
	ev = rdtsc();
	sx = (ev - sv);
	if (sx < mv) {
		mv = sx;
	}

	DebugLevel1("%ld %ld %3ld\n" _C_ (long)sx _C_ mv _C_ (sx * 100) / mv);
#endif
}

/**
**		Initialise the fog of war.
**		Build tables, setup functions.
*/
global void InitMapFogOfWar(void)
{
	VisibleTable = malloc(TheMap.Width * TheMap.Height * sizeof(*VisibleTable));

#ifdef USE_OPENGL
	VideoDrawFog = VideoDrawFogAlphaOpenGL;
	VideoDrawOnlyFog = VideoDrawOnlyFogAlphaOpenGL;
	VideoDrawUnexplored = VideoDrawUnexploredSolidOpenGL;
#else
#ifdef USE_SDL_SURFACE
	//
	//	Generate Only Fog surface.
	//	
	{
		unsigned char r;
		unsigned char g;
		unsigned char b;
		Uint32 color;
		SDL_Surface* s;

		s = SDL_CreateRGBSurface(SDL_SWSURFACE, TileSizeX, TileSizeY,
			32, RMASK, GMASK, BMASK, AMASK);

		// FIXME: Make the color configurable
		SDL_GetRGB(ColorBlack, TheScreen->format, &r, &g, &b);
		color = SDL_MapRGB(s->format, r, g, b);

		SDL_FillRect(s, NULL, color);
		OnlyFogSurface = SDL_DisplayFormat(s);
		SDL_SetAlpha(OnlyFogSurface, SDL_SRCALPHA | SDL_RLEACCEL, FogOfWarOpacity);
		SDL_FreeSurface(s);
	}
	VideoDrawUnexplored = VideoDrawUnexploredSDL;
	VideoDrawFog = VideoDrawFogSDL;
	VideoDrawOnlyFog = VideoDrawOnlyFogSDL;
#else
	if (TileSizeX == 16 && TileSizeY == 16) {
		switch (VideoDepth) {
			case  8:						//  8 bpp video depth
				VideoDrawFog = VideoDraw8Fog16Solid;
				VideoDrawOnlyFog = VideoDraw8OnlyFog16Solid;
				VideoDrawUnexplored = VideoDraw8Unexplored16Solid;
				break;

			case 15:						// 15 bpp video depth
			case 16:						// 16 bpp video depth
				VideoDrawFog = VideoDraw16Fog16Solid;
				VideoDrawOnlyFog = VideoDraw16OnlyFog16Solid;
				VideoDrawUnexplored = VideoDraw16Unexplored16Solid;
				break;
			case 24:						// 24 bpp video depth
				if (VideoBpp == 24) {
					VideoDrawFog = VideoDraw24Fog16Solid;
					VideoDrawOnlyFog = VideoDraw24OnlyFog16Solid;
					VideoDrawUnexplored = VideoDraw24Unexplored16Solid;
					break;
				}
				// FALL THROUGH
			case 32:						// 32 bpp video depth
				VideoDrawFog = VideoDraw32Fog16Solid;
				VideoDrawOnlyFog = VideoDraw32OnlyFog16Solid;
				VideoDrawUnexplored = VideoDraw32Unexplored16Solid;
				break;

			default:
				DebugLevel0Fn("Depth unsupported %d\n" _C_ VideoDepth);
				break;
		}
	} else if (TileSizeX == 32 && TileSizeY == 32) {
		switch (VideoDepth) {
			case  8:						//  8 bpp video depth
				VideoDrawFog = VideoDraw8Fog32Solid;
				VideoDrawOnlyFog = VideoDraw8OnlyFog32Solid;
				VideoDrawUnexplored = VideoDraw8Unexplored32Solid;
				break;

			case 15:						// 15 bpp video depth
			case 16:						// 16 bpp video depth
				VideoDrawFog = VideoDraw16Fog32Solid;
				VideoDrawOnlyFog = VideoDraw16OnlyFog32Solid;
				VideoDrawUnexplored = VideoDraw16Unexplored32Solid;
				break;
			case 24:						// 24 bpp video depth
				if (VideoBpp == 24) {
					VideoDrawFog = VideoDraw24Fog32Solid;
					VideoDrawOnlyFog = VideoDraw24OnlyFog32Solid;
					VideoDrawUnexplored = VideoDraw24Unexplored32Solid;
					break;
				}
				// FALL THROUGH
			case 32:						// 32 bpp video depth
				VideoDrawFog = VideoDraw32Fog32Solid;
				VideoDrawOnlyFog = VideoDraw32OnlyFog32Solid;
				VideoDrawUnexplored = VideoDraw32Unexplored32Solid;
				break;

			default:
				DebugLevel0Fn("Depth unsupported %d\n" _C_ VideoDepth);
				break;
		}
	} else {
		printf("Tile size not supported: (%dx%d)\n", TileSizeX, TileSizeY);
		exit(1);
	}
#endif
#endif
}

/**
**		Cleanup the fog of war.
*/
global void CleanMapFogOfWar(void)
{
	if (VisibleTable) {
		free(VisibleTable);
		VisibleTable = NULL;
	}
	SDL_FreeSurface(OnlyFogSurface);
}

/**
**		Initialize Vision and Goal Tables.
*/
global void InitVisionTable(void)
{
	int* visionlist;
	int maxsize;
	int sizex;
	int sizey;
	int maxsearchsize;
	int i;
	int VisionTablePosition;
	int marker;
	int direction;
	int right;
	int up;
	int repeat;

	// Initialize Visiontable to large size, can't be more entries than tiles.
	VisionTable[0] = malloc(MaxMapWidth * MaxMapWidth * sizeof(int));
	VisionTable[1] = malloc(MaxMapWidth * MaxMapWidth * sizeof(int));
	VisionTable[2] = malloc(MaxMapWidth * MaxMapWidth * sizeof(int));

	VisionLookup = malloc((MaxMapWidth + 2) * sizeof(int));
#ifndef SQUAREVISION
	visionlist = malloc(MaxMapWidth * MaxMapWidth * sizeof(int));
	//*2 as diagonal distance is longer

	maxsize = MaxMapWidth;
	maxsearchsize = MaxMapWidth;
	// Fill in table of map size
	for (sizex = 0; sizex < maxsize; ++sizex) {
		for (sizey = 0; sizey < maxsize; ++sizey) {
			visionlist[sizey * maxsize + sizex] = isqrt(sizex * sizex + sizey * sizey);
		}
	}

	VisionLookup[0] = 0;
	i = 1;
	VisionTablePosition = 0;
	while (i < maxsearchsize) {
		// Set Lookup Table
		VisionLookup[i] = VisionTablePosition;
		// Put in Null Marker
		VisionTable[0][VisionTablePosition] = i;
		VisionTable[1][VisionTablePosition] = 0;
		VisionTable[2][VisionTablePosition] = 0;
		++VisionTablePosition;


		// find i in left column
		marker = maxsize * i;
		direction = 0;
		right = 0;
		up = 0;

		// If not on top row, continue
		do {
			repeat = 0;
			do {
				// search for repeating
				// Test Right
				if ((repeat == 0 || direction == 1) && visionlist[marker + 1] == i) {
					right = 1;
					up = 0;
					++repeat;
					direction = 1;
					++marker;
				} else if ((repeat == 0 || direction == 2) && visionlist[marker - maxsize] == i) {
					up = 1;
					right = 0;
					++repeat;
					direction = 2;
					marker = marker - maxsize;
				} else if ((repeat == 0 || direction == 3) && visionlist[marker + 1 - maxsize] == i &&
						visionlist[marker - maxsize] != i && visionlist[marker + 1] != i) {
					up = 1;
					right = 1;
					++repeat;
					direction = 3;
					marker = marker + 1 - maxsize;
				} else {
					direction = 0;
					break;
				}

			   // search rigth
			   // search up - store as down.
			   // search diagonal
			}  while (direction && marker > (maxsize * 2));
			if (right || up) {
				VisionTable[0][VisionTablePosition] = repeat;
				VisionTable[1][VisionTablePosition] = right;
				VisionTable[2][VisionTablePosition] = up;
				++VisionTablePosition;
			}
		} while (marker > (maxsize * 2));
		++i;
	}

	free(visionlist);
#else
	// Find maximum distance in corner of map.
	maxsize = MaxMapWidth;
	maxsearchsize = isqrt(MaxMapWidth / 2);
	// Mark 1, It's a special case
	// Only Horizontal is marked
	VisionTable[0][0] = 1;
	VisionTable[1][0] = 0;
	VisionTable[2][0] = 0;
	VisionTable[0][1] = 1;
	VisionTable[1][1] = 1;
	VisionTable[2][1] = 0;

	// Mark VisionLookup
	VisionLookup[0] = 0;
	VisionLookup[1] = 0;
	i = 2;
	VisionTablePosition = 1;
	while (i <= maxsearchsize) {
		++VisionTablePosition;
		VisionLookup[i] = VisionTablePosition;
		// Setup Vision Start
		VisionTable[0][VisionTablePosition] = i;
		VisionTable[1][VisionTablePosition] = 0;
		VisionTable[2][VisionTablePosition] = 0;
		// Do Horizontal
		++VisionTablePosition;
		VisionTable[0][VisionTablePosition] = i;
		VisionTable[1][VisionTablePosition] = 1;
		VisionTable[2][VisionTablePosition] = 0;
		// Do Vertical
		++VisionTablePosition;
		VisionTable[0][VisionTablePosition] = i - 1;
		VisionTable[1][VisionTablePosition] = 0;
		VisionTable[2][VisionTablePosition] = 1;
		i++;
	}

	// Update Size of VisionTable to what was used.
	realloc(VisionTable[0], (VisionTablePosition + 2) * sizeof(int));
	realloc(VisionTable[1], (VisionTablePosition + 2) * sizeof(int));
	realloc(VisionTable[2], (VisionTablePosition + 2) * sizeof(int));
#endif
}

/**
**		Clean Up Generated Vision and Goal Tables.
*/
global void FreeVisionTable(void)
{
	// Free Vision Data
	if (VisionTable[0]) {
		free(VisionTable[0]);
		VisionTable[0] = NULL;
	}
	if (VisionTable[1]) {
		free(VisionTable[1]);
		VisionTable[1] = NULL;
	}
	if (VisionTable[2]) {
		free(VisionTable[2]);
		VisionTable[2] = NULL;
	}
	if (VisionLookup) {
		free(VisionLookup);
		VisionLookup = NULL;
	}
}
//@}
