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
/**@name minimap.c	-	The minimap. */
//
//	(c) Copyright 1998-2003 by Lutz Sammer and Jimmy Salmon
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
#include "video.h"
#include "tileset.h"
#include "map.h"
#include "minimap.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "ui.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

local Graphic* MinimapTerrainGraphic;	/// generated minimap terrain
#ifdef USE_SDL_SURFACE
local SDL_Surface* MinimapGraphic;	/// generated minimap
#else
local VMemType* MinimapGraphic;		/// generated minimap
#endif
local int* Minimap2MapX;		/// fast conversion table
local int* Minimap2MapY;		/// fast conversion table
local int Map2MinimapX[MaxMapWidth];	/// fast conversion table
local int Map2MinimapY[MaxMapHeight];	/// fast conversion table

//	MinimapScale:
//	32x32 64x64 96x96 128x128 256x256 512x512 ...
//	  *4    *2    *4/3   *1	     *1/2    *1/4
local int MinimapScaleX;		/// Minimap scale to fit into window
local int MinimapScaleY;		/// Minimap scale to fit into window
global int MinimapX;			/// Minimap drawing position x offset
global int MinimapY;			/// Minimap drawing position y offset

global int MinimapWithTerrain = 1;	/// display minimap with terrain
global int MinimapFriendly = 1;		/// switch colors of friendly units
global int MinimapShowSelected = 1;	/// highlight selected units

local int OldMinimapCursorX;		/// Save MinimapCursorX
local int OldMinimapCursorY;		/// Save MinimapCursorY
local int OldMinimapCursorW;		/// Save MinimapCursorW
local int OldMinimapCursorH;		/// Save MinimapCursorH
local int OldMinimapCursorSize;		/// Saved image size

local void* OldMinimapCursorImage;	/// Saved image behind cursor

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Update minimap at map position x,y. This is called when the tile image
**	of a tile changes.
**
**	@todo	FIXME: this is not correct should use SeenTile.
**
**	@param tx	Tile X position, where the map changed.
**	@param ty	Tile Y position, where the map changed.
**
**	FIXME: this can surely be sped up??
*/
global void UpdateMinimapXY(int tx, int ty)
{
    int mx;
    int my;
    int x;
    int y;
    int scalex;
    int scaley;

    scalex = MinimapScaleX / MINIMAP_FAC;
    if (scalex == 0) {
	scalex = 1;
    }
    scaley = MinimapScaleY / MINIMAP_FAC;
    if (scaley == 0) {
	scaley = 1;
    }
    //
    //	Pixel 7,6 7,14, 15,6 15,14 are taken for the minimap picture.
    //
    ty *= TheMap.Width;
    for (my = MinimapY; my < TheUI.MinimapH - MinimapY; ++my) {
	y = Minimap2MapY[my];
	if (y < ty) {
	    continue;
	}
	if (y > ty) {
	    break;
	}

	for (mx = MinimapX; mx < TheUI.MinimapW - MinimapX; ++mx) {
	    int tile;

	    x = Minimap2MapX[mx];
	    if (x < tx) {
		continue;
	    }
	    if (x > tx) {
		break;
	    }

	    tile = TheMap.Fields[x + y].Tile;

#ifdef USE_SDL_SURFACE
	    // FIXME: todod
#else
	    ((unsigned char*)MinimapTerrainGraphic->Frames)[mx + my * TheUI.MinimapW] =
		TheMap.Tiles[tile][7 + (mx % scalex) * 8 + (6 + (my % scaley) * 8) * TileSizeX];
#endif
	}
    }
}

/**
**	Update a mini-map from the tiles of the map.
**
**	@todo	FIXME: this is not correct should use SeenTile.
**
**	FIXME: this can surely be sped up??
*/
global void UpdateMinimapTerrain(void)
{
    int mx;
    int my;
    int scalex;
    int scaley;

    if (!(scalex = (MinimapScaleX / MINIMAP_FAC))) {
	scalex = 1;
    }
    if (!(scaley = (MinimapScaleY / MINIMAP_FAC))) {
	scaley = 1;
    }

    //
    //	Pixel 7,6 7,14, 15,6 15,14 are taken for the minimap picture.
    //
    for (my = MinimapY; my < TheUI.MinimapH - MinimapY; ++my) {
	for (mx = MinimapX; mx < TheUI.MinimapW - MinimapX; ++mx) {
	    int tile;

	    tile = TheMap.Fields[Minimap2MapX[mx] + Minimap2MapY[my]].Tile;

#ifdef USE_SDL_SURFACE
	    // FIXME: todo
#else
	    ((unsigned char*)MinimapTerrainGraphic->Frames)[mx + my * TheUI.MinimapW] =
		TheMap.Tiles[tile][7 + (mx % scalex) * 8 + (6 + (my % scaley) * 8) * TileSizeX];
#endif
	}
    }
}

/**
**	Create a mini-map from the tiles of the map.
**
**	@todo 	Scaling and scrolling the minmap is currently not supported.
*/
global void CreateMinimap(void)
{
    int n;
    int x;
    int y;

    if (TheMap.Width > TheMap.Height) {	// Scale to biggest value.
	n = TheMap.Width;
    } else {
	n = TheMap.Height;
    }
    MinimapScaleX = (TheUI.MinimapW * MINIMAP_FAC + n - 1) / n;
    MinimapScaleY = (TheUI.MinimapH * MINIMAP_FAC + n - 1) / n;

    MinimapX = ((TheUI.MinimapW * MINIMAP_FAC) / MinimapScaleX - TheMap.Width) / 2;
    MinimapY = ((TheUI.MinimapH * MINIMAP_FAC) / MinimapScaleY - TheMap.Height) / 2;
    MinimapX = (TheUI.MinimapW - (TheMap.Width * MinimapScaleX) / MINIMAP_FAC) / 2;
    MinimapY = (TheUI.MinimapH - (TheMap.Height * MinimapScaleY) / MINIMAP_FAC) / 2;

    DebugLevel0Fn("MinimapScale %d %d (%d %d), X off %d, Y off %d\n" _C_
	MinimapScaleX / MINIMAP_FAC _C_ MinimapScaleY / MINIMAP_FAC _C_
	MinimapScaleX _C_ MinimapScaleY _C_
	MinimapX _C_ MinimapY);

    //
    //	Calculate minimap fast lookup tables.
    //
    // FIXME: this needs to be recalculated during map load - the map size
    // might have changed!
    Minimap2MapX = calloc(sizeof(int), TheUI.MinimapW * TheUI.MinimapH);
    Minimap2MapY = calloc(sizeof(int), TheUI.MinimapW * TheUI.MinimapH);
    for (n = MinimapX; n < TheUI.MinimapW - MinimapX; ++n) {
	Minimap2MapX[n] = ((n - MinimapX) * MINIMAP_FAC) / MinimapScaleX;
    }
    for (n = MinimapY; n < TheUI.MinimapH - MinimapY; ++n) {
	Minimap2MapY[n] = (((n - MinimapY) * MINIMAP_FAC) / MinimapScaleY) * TheMap.Width;
    }
    for (n = 0; n < TheMap.Width; ++n) {
	Map2MinimapX[n] = (n * MinimapScaleX) / MINIMAP_FAC;
    }
    for (n = 0; n < TheMap.Height; ++n) {
	Map2MinimapY[n] = (n * MinimapScaleY) / MINIMAP_FAC;
    }

    MinimapTerrainGraphic = NewGraphic(8, TheUI.MinimapW, TheUI.MinimapH);

#ifdef USE_SDL_SURFACE
    Uint32 rmask, gmask, bmask, amask;
// FIXME: use defines for this?
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    // FIXME: does NewGraphic do this for MinimapTerrainGraphic??
    // FIXME: what depth?
    MinimapGraphic = SDL_CreateRGBSurface(SDL_SWSURFACE, TheUI.MinimapW,
	TheUI.MinimapH, 8, rmask, gmask, bmask, amask);
#else
    memset(MinimapTerrainGraphic->Frames, 0, TheUI.MinimapW * TheUI.MinimapH);
    MinimapGraphic = calloc(TheUI.MinimapW * TheUI.MinimapH, sizeof(VMemType));
#endif

    // FIXME: looks too complicated
    for (y = 0; y < TheUI.MinimapH; ++y) {
	for (x = 0; x < TheUI.MinimapW; ++x) {
#ifdef USE_SDL_SURFACE
	    SDL_Color c;
	    int b;

	    c = TheUI.MinimapPanel.Graphic->Surface->format->palette->colors[
		x + (TheUI.MinimapPosX - TheUI.MinimapPanelX) + (y + TheUI.MinimapPosY - 
		TheUI.MinimapPanelY) * TheUI.MinimapPanel.Graphic->Width];

	    b = MinimapGraphic->format->BytesPerPixel;

	    // FIXME: rgb: todo: ack

//	    ((char*)MinimapGraphic->pixels)[x * b + y * TheUI.MinimapW * b] = 
//		VideoMapRGB(c.r, c.g, c.b);
#else
	    Palette p;
	    // this only copies the panel background... honest.
	    p = GlobalPalette[
		((unsigned char*)TheUI.MinimapPanel.Graphic->Frames)[
		    x + (TheUI.MinimapPosX - TheUI.MinimapPanelX) +
		    (y + TheUI.MinimapPosY - TheUI.MinimapPanelY) *
		    TheUI.MinimapPanel.Graphic->Width]];
	    MinimapGraphic[x + y * TheUI.MinimapW] = VideoMapRGB(p.r, p.g, p.b);
#endif
	}
    }
    if (!TheUI.MinimapTransparent) {
	// make only the inner part which is going to be used black
	for (y = MinimapY; y < TheUI.MinimapH - MinimapY; ++y) {
	    for (x = MinimapX; x < TheUI.MinimapW - MinimapX; ++x) {
#ifdef USE_SDL_SURFACE
		// FIXME: todo
//		((char*)MinimapGraphic->pixels)[x + y * TheUI.MinimapW] = ColorBlack;
#else
		MinimapGraphic[x + y * TheUI.MinimapW] = ColorBlack;
#endif
	    }
	}
    }

    UpdateMinimapTerrain();
}

/**
**	Destroy mini-map.
*/
global void DestroyMinimap(void)
{
    VideoSaveFree(MinimapTerrainGraphic);
    MinimapTerrainGraphic = NULL;
    if (MinimapGraphic) {
	free(MinimapGraphic);
	MinimapGraphic = NULL;
    }
    free(Minimap2MapX);
    Minimap2MapX = NULL;
    free(Minimap2MapY);
    Minimap2MapY = NULL;
}

/**
**	Update the minimap
**	@note This one of the hot-points in the program optimize and optimize!
*/
global void UpdateMinimap(void)
{
    static int red_phase;
    int red_phase_changed;
    int mx;
    int my;
    UnitType* type;
    Unit** table;
    Unit* unit;
    int w;
    int h;
    int h0;

    red_phase_changed = red_phase != (int)((FrameCounter / FRAMES_PER_SECOND) & 1);
    if (red_phase_changed) {
	red_phase = !red_phase;
    }

    //
    //	Draw the terrain (or make it black again)
    //
    for (my = 0; my < TheUI.MinimapH; ++my) {
	for (mx = 0; mx < TheUI.MinimapW; ++mx) {
	    int visiontype; // 0 unexplored, 1 explored, >1 visible.
	    if (ReplayRevealMap) {
		visiontype = 2;
	    } else {
		visiontype = IsTileVisible(ThisPlayer, Minimap2MapX[mx], Minimap2MapY[my] / TheMap.Width);
	    }
#ifdef USE_SDL_SURFACE
	    // FIXME: todo
#else
	    if (MinimapWithTerrain && (visiontype > 1 || (visiontype == 1 && ((mx & 1) == (my & 1))))) {
		Palette p;
		p = GlobalPalette[
		    ((unsigned char*)MinimapTerrainGraphic->Frames)[mx + my * TheUI.MinimapW]];
		MinimapGraphic[mx + my * TheUI.MinimapW] = VideoMapRGB(p.r, p.g, p.b);
	    } else if (visiontype > 0) {
		MinimapGraphic[mx + my * TheUI.MinimapW] = ColorBlack;
	    }
#endif
	}
    }

    //
    //	Draw units on map
    //	FIXME: I should rewrite this completely
    //	FIXME: make a bitmap of the units, and update it with the moves
    //	FIXME: and other changes
    //

    table = &CorpseList;

#ifdef USE_SDL_SURFACE
    // FIXME: todo
    (int)type = (int)unit = w = h = h0 = 0;
#else

    while (*table) {
	VMemType color;

	// Only for buildings?
	if (!(*table)->Type->Building) {
	    table = &(*table)->Next;
	    continue;
	}
	if (!UnitDrawableOnMap(*table) && (*table)->SeenState != 3
		&& !(*table)->SeenDestroyed && (type = (*table)->SeenType) ) {
	    if( (*table)->Player->Player == PlayerNumNeutral ) {
		color = VideoMapRGB((*table)->Type->NeutralMinimapColorRGB.D24.a,
		        (*table)->Type->NeutralMinimapColorRGB.D24.b,
			(*table)->Type->NeutralMinimapColorRGB.D24.c);
	    } else {
		color = (*table)->Player->Color;
	    }

	    mx = 1 + MinimapX + Map2MinimapX[(*table)->X];
	    my = 1 + MinimapY + Map2MinimapY[(*table)->Y];
	    w = Map2MinimapX[type->TileWidth];
	    if (mx + w >= TheUI.MinimapW) {	// clip right side
		w = TheUI.MinimapW - mx;
	    }
	    h0 = Map2MinimapY[type->TileHeight];
	    if (my + h0 >= TheUI.MinimapH) {	// clip bottom side
		h0 = TheUI.MinimapH - my;
	    }
	    while (w-- >= 0) {
		h = h0;
		while (h-- >= 0) {
		    MinimapGraphic[mx + w + (my + h) * TheUI.MinimapW] = color;
		}
	    }
	}
	table = &(*table)->Next;
    }

    for (table = Units; table < Units + NumUnits; ++table) {
	VMemType color;

	unit = *table;

	//
	//	If the unit is not known on map don't draw it.
	//	This function should cover shared vision and stuff.
	//
	if (!UnitDrawableOnMap(unit) && !ReplayRevealMap) {
	    continue;
	}

	type = unit->Type;
	//
	//  FIXME: We should force unittypes to have a certain color on the minimap.
	//
	if (unit->Player->Player == PlayerNumNeutral) {
	    color = VideoMapRGB((*table)->Type->NeutralMinimapColorRGB.D24.a,
		(*table)->Type->NeutralMinimapColorRGB.D24.b,
		(*table)->Type->NeutralMinimapColorRGB.D24.c);
	} else if (unit->Player == ThisPlayer) {
	    if (unit->Attacked && unit->Attacked + ATTACK_BLINK_DURATION > GameCycle &&
		    (red_phase || unit->Attacked + ATTACK_RED_DURATION > GameCycle)) {
		color = ColorRed;
	    } else if (MinimapShowSelected && unit->Selected) {
		color = ColorWhite;
	    } else {
		color = ColorGreen;
	    }
	} else {
	    color = unit->Player->Color;
	}

	mx = 1 + MinimapX + Map2MinimapX[unit->X];
	my = 1 + MinimapY + Map2MinimapY[unit->Y];
	w = Map2MinimapX[type->TileWidth];
	if (mx + w >= TheUI.MinimapW) {		// clip right side
	    w = TheUI.MinimapW - mx;
	}
	h0 = Map2MinimapY[type->TileHeight];
	if (my + h0 >= TheUI.MinimapH) {	// clip bottom side
	    h0 = TheUI.MinimapH - my;
	}
	while (w-- >= 0) {
	    h = h0;
	    while (h-- >= 0) {
		MinimapGraphic[mx + w + (my + h) * TheUI.MinimapW] = color;
	    }
	}

    }
#endif
}

/**
**	Draw the mini-map with current viewpoint.
**
**	@param vx	View point X position.
**	@param vy	View point Y position.
*/
global void DrawMinimap(int vx __attribute__((unused)),
	int vy __attribute__((unused)))
{
#ifdef USE_SDL_SURFACE
    // FIXME: todo
#else
    int i;
    int j;

    switch (VideoBpp) {
	case 8: {
	    VMemType8* v;

	    v = VideoMemory8 + TheUI.MinimapPosY * VideoWidth + TheUI.MinimapPosX;
	    for (i = 0; i < TheUI.MinimapH; ++i) {
		for (j = 0; j < TheUI.MinimapW; ++j) {
		    v[j] = MinimapGraphic[i * TheUI.MinimapW + j].D8;
		}
		v += VideoWidth;
	    }
	    break;
	}
	case 15:
	case 16: {
	    VMemType16* v;

	    v = VideoMemory16 + TheUI.MinimapPosY * VideoWidth + TheUI.MinimapPosX;
	    for (i = 0; i < TheUI.MinimapH; ++i) {
		for (j = 0; j < TheUI.MinimapW; ++j) {
		    v[j] = MinimapGraphic[i * TheUI.MinimapW + j].D16;
		}
		v += VideoWidth;
	    }
	    break;
	}
	case 24: {
	    VMemType24* v;

	    v = VideoMemory24 + TheUI.MinimapPosY * VideoWidth + TheUI.MinimapPosX;
	    for (i = 0; i < TheUI.MinimapH; ++i) {
		for (j = 0; j < TheUI.MinimapW; ++j) {
		    v[j] = MinimapGraphic[i * TheUI.MinimapW + j].D24;
		}
		v += VideoWidth;
	    }
	    break;
	}
	case 32: {
	    VMemType32* v;

	    v = VideoMemory32 + TheUI.MinimapPosY * VideoWidth + TheUI.MinimapPosX;
	    for (i = 0; i < TheUI.MinimapH; ++i) {
		for (j = 0; j < TheUI.MinimapW; ++j) {
		    v[j] = MinimapGraphic[i * TheUI.MinimapW + j].D32;
		}
		v += VideoWidth;
	    }
	    break;
	}
    }
#endif
}

/**
**	Hide minimap cursor.
*/
global void HideMinimapCursor(void)
{
    if (OldMinimapCursorW) {
	LoadCursorRectangle(OldMinimapCursorImage,
	    OldMinimapCursorX, OldMinimapCursorY,
	    OldMinimapCursorW, OldMinimapCursorH);
	OldMinimapCursorW = 0;
    }
}

/**
**	Draw minimap cursor.
**
**	@param vx	View point X position.
**	@param vy	View point Y position.
*/
global void DrawMinimapCursor(int vx, int vy)
{
    int x;
    int y;
    int w;
    int h;
    int i;

    // Determine and save region below minimap cursor
    OldMinimapCursorX = x =
	TheUI.MinimapPosX + MinimapX + (vx * MinimapScaleX) / MINIMAP_FAC;
    OldMinimapCursorY = y =
	TheUI.MinimapPosY + MinimapY + (vy * MinimapScaleY) / MINIMAP_FAC;
    OldMinimapCursorW = w =
	(TheUI.SelectedViewport->MapWidth * MinimapScaleX) / MINIMAP_FAC;
    OldMinimapCursorH = h =
	(TheUI.SelectedViewport->MapHeight * MinimapScaleY) / MINIMAP_FAC;
#ifdef USE_SDL_SURFACE
    i = (w + 1 + h) * 2 * TheScreen->format->BytesPerPixel;
#else
    i = (w + 1 + h) * 2 * VideoTypeSize;
#endif
    if (OldMinimapCursorSize < i) {
	if (OldMinimapCursorImage) {
	    OldMinimapCursorImage = realloc(OldMinimapCursorImage, i);
	} else {
	    OldMinimapCursorImage = malloc(i);
	}
	DebugLevel3("Cursor memory %d\n" _C_ i);
	OldMinimapCursorSize = i;
    }
    SaveCursorRectangle(OldMinimapCursorImage, x, y, w, h);

    // Draw cursor as rectangle (Note: unclipped, as it is always visible)
#ifdef USE_SDL_SURFACE
    VideoDrawTransRectangle(TheUI.ViewportCursorColor, x, y, w, h, 50);
#else
    VideoDraw50TransRectangle(TheUI.ViewportCursorColor, x, y, w, h);
#endif
}

/**
**	Convert minimap cursor X position to tile map coordinate.
**
**	@param x	Screen X pixel coordinate.
**	@return		Tile X coordinate.
*/
global int ScreenMinimap2MapX(int x)
{
    int tx;

    tx = (((x - TheUI.MinimapPosX - MinimapX) * MINIMAP_FAC) / MinimapScaleX);
    if (tx < 0) {
	return 0;
    }
    return tx < TheMap.Width ? tx : TheMap.Width - 1;
}

/**
**	Convert minimap cursor Y position to tile map coordinate.
**
**	@param y	Screen Y pixel coordinate.
**	@return		Tile Y coordinate.
*/
global int ScreenMinimap2MapY(int y)
{
    int ty;

    ty = (((y - TheUI.MinimapPosY - MinimapY) * MINIMAP_FAC) / MinimapScaleY);
    if (ty < 0) {
	return 0;
    }
    return ty < TheMap.Height ? ty : TheMap.Height - 1;
}

//@}
