//       _________ __                 __                               
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ \ 
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/ 
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   Stratagus - A free fantasy real time strategy game engine
//
/**@name edmap.c	-	Editor map functions. */
//
//	(c) Copyright 2002 by Lutz Sammer
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
#include <limits.h>

#include "stratagus.h"
#include "editor.h"
#include "map.h"
#include "minimap.h"
#include "player.h"

/*----------------------------------------------------------------------------
--	Defines
----------------------------------------------------------------------------*/

#define TH_QUAD_M	0xFFFF0000	/// Top half quad mask
#define BH_QUAD_M	0x0000FFFF	/// Bottom half quad mask
#define LH_QUAD_M	0xFF00FF00	/// Left half quad mask
#define RH_QUAD_M	0x00FF00FF	/// Right half quad mask

    /// Callback for changed tile (with direction mask)
local void EditorTileChanged2(int x, int y, int d);

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Get quad from tile.
**
**	A quad is a 32 bit value defining the content of the tile.
**
**	A tile is split into 4 parts, the basic tile type of this part
**	is stored as 8bit value in the quad.
**
**	ab
**	cd -> abcd
**
**	If the tile is 100% light grass the value is 0x5555.
**	If the tile is 3/4 light grass and dark grass in upper left corner
**	the value is 0x6555.
**
**	@param x	X map tile position
**	@param y	Y map tile position
**
**	@return		the 'quad' of the tile.
**
**	@todo	Make a lookup table to speed up the things.
*/
local unsigned QuadFromTile(int x, int y)
{
    int tile;
    int i;
    unsigned quad;
    unsigned mix;

    //
    //  find the abstact tile number
    //
    tile = TheMap.Fields[y * TheMap.Width + x].Tile;
    for (i = 0; i < TheMap.Tileset->NumTiles; ++i) {
	if (tile == TheMap.Tileset->Table[i]) {
	    break;
	}
    }
    DebugCheck(i == TheMap.Tileset->NumTiles);

    quad = TheMap.Tileset->BasicNameTable[i];
    mix = TheMap.Tileset->MixedNameTable[i];

    DebugLevel3Fn("Tile %d:%04x %d,%d\n" _C_ tile _C_ i _C_ quad _C_ mix);

    if (!mix) {				// a solid tile
	return quad | (quad << 8) | (quad << 16) | (quad << 24);
    }
    //
    //  Mixed tiles, mix together
    //
    switch ((i & 0x00F0) >> 4) {
	case 0:
	    return (quad << 24) | (mix << 16) | (mix << 8) | mix;
	case 1:
	    return (mix << 24) | (quad << 16) | (mix << 8) | mix;
	case 2:
	    return (quad << 24) | (quad << 16) | (mix << 8) | mix;
	case 3:
	    return (mix << 24) | (mix << 16) | (quad << 8) | mix;
	case 4:
	    return (quad << 24) | (mix << 16) | (quad << 8) | mix;
	case 5:
	    return (quad << 24) | (quad << 16) | (quad << 8) | mix;
	case 6:
	    return (quad << 24) | (quad << 16) | (quad << 8) | mix;
	case 7:
	    return (mix << 24) | (mix << 16) | (mix << 8) | quad;
	case 8:
	    return (quad << 24) | (mix << 16) | (mix << 8) | quad;
	case 9:
	    return (mix << 24) | (quad << 16) | (mix << 8) | quad;
	case 10:
	    return (quad << 24) | (quad << 16) | (mix << 8) | quad;
	case 11:
	    return (mix << 24) | (mix << 16) | (quad << 8) | quad;
	case 12:
	    return (quad << 24) | (mix << 16) | (quad << 8) | quad;
	case 13:
	    return (mix << 24) | (quad << 16) | (quad << 8) | quad;
    }

    DebugCheck( 1 );

    return quad | (quad << 8) | (quad << 16) | (quad << 24);
}

/**
**	Find a tile path.
**
**	@param base	Start tile type.
**	@param goal	Goal tile type.
**	@param length	Best found path length.
**	@param marks	Already visited tile types.
**	@param tile	Tile pointer.
*/
local int FindTilePath(int base, int goal, int length, char* marks, int* tile)
{
    int i;
    int l;
    int j;
    int n;

    //
    //  Find any mixed tile
    //
    l = INT_MAX;
    for (i = 0; i < TheMap.Tileset->NumTiles;) {
	// goal found.
	if (base == TheMap.Tileset->BasicNameTable[i]
		&& goal == TheMap.Tileset->MixedNameTable[i]) {
	    *tile = i;
	    return length;
	}
	// goal found.
	if (goal == TheMap.Tileset->BasicNameTable[i]
		&& base == TheMap.Tileset->MixedNameTable[i]) {
	    *tile = i;
	    return length;
	}

	// possible path found
	if (base == TheMap.Tileset->BasicNameTable[i]
		&& TheMap.Tileset->MixedNameTable[i]) {
	    j = TheMap.Tileset->MixedNameTable[i];
	    if (!marks[j]) {
		marks[j] = j;
		n = FindTilePath(j, goal, length + 1, marks, &n);
		marks[j] = 0;
		if (n < l) {
		    *tile = i;
		    l = n;
		}
	    }
	// possible path found
	} else if (TheMap.Tileset->BasicNameTable[i]
		&& base == TheMap.Tileset->MixedNameTable[i]) {
	    j = TheMap.Tileset->BasicNameTable[i];
	    if (!marks[j]) {
		marks[j] = j;
		n = FindTilePath(j, goal, length + 1, marks, &n);
		marks[j] = 0;
		if (n < l) {
		    *tile = i;
		    l = n;
		}
	    }
	}
	// Advance solid or mixed.
	if (!TheMap.Tileset->MixedNameTable[i]) {
	    i += 16;
	} else {
	    i += 256;
	}
    }
    return l;
}

/**
**	Get tile from quad.
**
**	@param fixed	Part can't be changed.
**	@param quad	Quad of the tile type.
**	@return		Best matching tile.
*/
local int TileFromQuad(unsigned fixed, unsigned quad)
{
    int i;
    unsigned type1;
    unsigned type2;
    int base;
    int direction;
    //                 0  1  2  3   4  5  6  7   8  9  A   B  C   D  E  F
    char table[16] = { 0, 7, 3, 11, 1, 9, 5, 13, 0, 8, 4, 12, 2, 10, 6, 0 };

    DebugLevel3Fn("%x %x\n" _C_ fixed _C_ quad);

    //
    //  Get tile type from fixed.
    //
    while (!(type1 = (fixed & 0xFF))) {
	fixed >>= 8;
	if (!fixed) {
	    abort();
	}
    }
    fixed >>= 8;
    while (!(type2 = (fixed & 0xFF)) && fixed) {
	fixed >>= 8;
    }
    //
    //  Need an second type.
    //
    if (!type2 || type2 == type1) {
	fixed = quad;
	while ((type2 = (fixed & 0xFF)) == type1 && fixed) {
	    fixed >>= 8;
	}
	if (type1 == type2) {		// Oooh a solid tile.
	  find_solid:
	    //
	    //  Find the solid tile
	    //
	    for (i = 0; i < TheMap.Tileset->NumTiles;) {
		if (type1 == TheMap.Tileset->BasicNameTable[i]
			&& !TheMap.Tileset->MixedNameTable[i]) {
		    break;
		}
		// Advance solid or mixed.
		if (!TheMap.Tileset->MixedNameTable[i]) {
		    i += 16;
		} else {
		    i += 256;
		}
	    }
	    DebugCheck(i >= TheMap.Tileset->NumTiles);
	    return i;
	}
    } else {
	char *marks;

	marks = alloca(TheMap.Tileset->NumNames);
	memset(marks, 0, TheMap.Tileset->NumNames);
	marks[type1] = type1;
	marks[type2] = type2;

	//
	//      What fixed tile-type should replace the non useable tile-types.
	//      FIXME: write a loop.
	//
	fixed = (quad >> 0) & 0xFF;
	if (fixed != type1 && fixed != type2) {
	    quad &= 0xFFFFFF00;
	    if (FindTilePath(type1, fixed, 0, marks, &i)
		    < FindTilePath(type2, fixed, 0, marks, &i)) {
		quad |= type1 << 0;
	    } else {
		quad |= type2 << 0;
	    }
	}
	fixed = (quad >> 8) & 0xFF;
	if (fixed != type1 && fixed != type2) {
	    quad &= 0xFFFF00FF;
	    if (FindTilePath(type1, fixed, 0, marks, &i)
		    < FindTilePath(type2, fixed, 0, marks, &i)) {
		quad |= type1 << 8;
	    } else {
		quad |= type2 << 8;
	    }
	}
	fixed = (quad >> 16) & 0xFF;
	if (fixed != type1 && fixed != type2) {
	    quad &= 0xFF00FFFF;
	    if (FindTilePath(type1, fixed, 0, marks, &i)
		    < FindTilePath(type2, fixed, 0, marks, &i)) {
		quad |= type1 << 16;
	    } else {
		quad |= type2 << 16;
	    }
	}
	fixed = (quad >> 24) & 0xFF;
	if (fixed != type1 && fixed != type2) {
	    quad &= 0x00FFFFFF;
	    if (FindTilePath(type1, fixed, 0, marks, &i)
		    < FindTilePath(type2, fixed, 0, marks, &i)) {
		quad |= type1 << 24;
	    } else {
		quad |= type2 << 24;
	    }
	}
    }

    DebugLevel3Fn("type1 %x type2 %x\n" _C_ type1 _C_ type2);

    //
    //  Need a mixed tile
    //
    for (i = 0; i < TheMap.Tileset->NumTiles;) {
	if (type1 == TheMap.Tileset->BasicNameTable[i]
		&& type2 == TheMap.Tileset->MixedNameTable[i]) {
	    break;
	}
	if (type2 == TheMap.Tileset->BasicNameTable[i]
		&& type1 == TheMap.Tileset->MixedNameTable[i]) {
	    // Other mixed
	    type1 ^= type2;
	    type2 ^= type1;
	    type1 ^= type2;
	    break;
	}
	// Advance solid or mixed.
	if (!TheMap.Tileset->MixedNameTable[i]) {
	    i += 16;
	} else {
	    i += 256;
	}
    }

    if (i >= TheMap.Tileset->NumTiles) {
	char *marks;

	DebugLevel3Fn("No good mix found\n");
	//
	//      Find the best tile path.
	//
	marks = alloca(TheMap.Tileset->NumNames);
	memset(marks, 0, TheMap.Tileset->NumNames);
	marks[type1] = type1;
	if (FindTilePath(type1, type2, 0, marks, &i) == INT_MAX) {
	    DebugLevel0Fn("Huch, no mix found!!!!!!!!!!!\n");
	    goto find_solid;
	}
	if (type1 == TheMap.Tileset->MixedNameTable[i]) {
	    // Other mixed
	    type1 ^= type2;
	    type2 ^= type1;
	    type1 ^= type2;
	}
    }

    base = i;

    direction = 0;
    if (((quad >> 24) & 0xFF) == type1) {
	direction |= 8;
    }
    if (((quad >> 16) & 0xFF) == type1) {
	direction |= 4;
    }
    if (((quad >> 8) & 0xFF) == type1) {
	direction |= 2;
    }
    if (((quad >> 0) & 0xFF) == type1) {
	direction |= 1;
    }

    DebugLevel3Fn("%08x %x %x %d\n" _C_ quad _C_ type1 _C_ type2 _C_
	direction);

    return base | (table[direction] << 4);
}

/**
**	Change tile from abstract tile-type.
**
**	@param x	X map tile coordinate.
**	@param y	Y map tile coordinate.
**	@param tile	Abstract tile type to edit.
*/
global void ChangeTile(int x, int y, int tile)
{
    MapField *mf;

    DebugCheck(x < 0 || y < 0 || x >= TheMap.Width || y >= TheMap.Height);
    DebugCheck(tile < 0 || tile >= TheMap.Tileset->NumTiles);

    mf = &TheMap.Fields[y * TheMap.Width + x];
    mf->Tile = mf->SeenTile = TheMap.Tileset->Table[tile];
}

#define D_UP	8		/// Go up allowed
#define D_DOWN	4		/// Go down allowed
#define D_LEFT	2		/// Go left allowed
#define D_RIGHT	1		/// Go right allowed

/**
**	Editor change tile.
**
**	@param x	X map tile coordinate.
**	@param y	Y map tile coordinate.
**	@param tile	Tile type to edit.
**	@param d	Fix direction flag 8 up, 4 down, 2 left, 1 right.
*/
local void EditorChangeTile(int x, int y, int tile, int d)
{
    MapField* mf;

    DebugCheck(x < 0 || y < 0 || x >= TheMap.Width || y >= TheMap.Height);

    ChangeTile(x, y, tile);

    //
    //  Change the flags
    //
    mf = &TheMap.Fields[y * TheMap.Width + x];
    mf->Flags &= ~(MapFieldHuman | MapFieldLandAllowed | MapFieldCoastAllowed |
	MapFieldWaterAllowed | MapFieldNoBuilding | MapFieldUnpassable |
	MapFieldWall | MapFieldRocks | MapFieldForest);

    mf->Flags |= TheMap.Tileset->FlagsTable[tile];

    UpdateMinimapSeenXY(x, y);
    UpdateMinimapXY(x, y);

    EditorTileChanged2(x, y, d);
}

/**
**	Update surroundings for tile changes.
**
**	@param x	Map X tile position of change.
**	@param y	Map Y tile position of change.
**	@param d	Fix direction flag 8 up, 4 down, 2 left, 1 right.
*/
local void EditorTileChanged2(int x, int y, int d)
{
    unsigned quad;
    unsigned q2;
    unsigned u;
    int tile;
    MapField* mf;

    quad = QuadFromTile(x, y);
    DebugLevel3Fn("%d,%d %08x %d\n" _C_ x _C_ y _C_ quad _C_
	    TheMap.Fields[y * TheMap.Width + x].Tile);

    //
    //  Change the surrounding
    //

    //
    //	Special case 1) Walls.
    //
    mf = &TheMap.Fields[y * TheMap.Width + x];
    if (mf->Flags & MapFieldWall) {
	if (mf->Flags & MapFieldHuman) {
	    mf->Value = UnitTypeHumanWall->_HitPoints;
	} else {
	    mf->Value = UnitTypeOrcWall->_HitPoints;
	}
        MapFixWallTile(x + 0, y + 0);
	MapFixWallTile(x + 1, y + 0);
	MapFixWallTile(x + 0, y + 1);
	MapFixWallTile(x - 1, y + 0);
	MapFixWallTile(x + 0, y - 1);
	return;
    }

    if (d&D_UP && y) {
	//
	//      Insert into the bottom the new tile.
	//
	q2 = QuadFromTile(x, y - 1);
	u = (q2 & TH_QUAD_M) | ((quad >> 16) & BH_QUAD_M);
	if (u != q2) {
	    DebugLevel3Fn("U+    %08x -> %08x\n" _C_ q2 _C_ u);
	    tile = TileFromQuad(u & BH_QUAD_M, u);
	    DebugLevel3Fn("= %08x\n" _C_ tile);
	    EditorChangeTile(x, y - 1, tile, d&~D_DOWN);
	}
    }
    if (d&D_DOWN && y < TheMap.Height - 1) {
	//
	//      Insert into the top the new tile.
	//
	q2 = QuadFromTile(x, y + 1);
	u = (q2 & BH_QUAD_M) | ((quad << 16) & TH_QUAD_M);
	if (u != q2) {
	    DebugLevel3Fn("D+    %08x -> %08x\n" _C_ q2 _C_ u);
	    tile = TileFromQuad(u & TH_QUAD_M, u);
	    EditorChangeTile(x, y + 1, tile, d&~D_UP);
	}
    }
    if (d&D_LEFT && x) {
	//
	//      Insert into the left the new tile.
	//
	q2 = QuadFromTile(x - 1, y);
	u = (q2 & LH_QUAD_M) | ((quad >> 8) & RH_QUAD_M);
	if (u != q2) {
	    DebugLevel3Fn("L+    %08x -> %08x\n" _C_ q2 _C_ u);
	    tile = TileFromQuad(u & RH_QUAD_M, u);
	    EditorChangeTile(x - 1, y, tile, d&~D_RIGHT);
	}
    }
    if (d&D_RIGHT && x < TheMap.Width - 1) {
	//
	//      Insert into the right the new tile.
	//
	q2 = QuadFromTile(x + 1, y);
	u = (q2 & RH_QUAD_M) | ((quad << 8) & LH_QUAD_M);
	if (u != q2) {
	    DebugLevel3Fn("R+    %08x -> %08x\n" _C_ q2 _C_ u);
	    tile = TileFromQuad(u & LH_QUAD_M, u);
	    EditorChangeTile(x + 1, y, tile, d&~D_LEFT);
	}
    }
}

/**
**	Update surroundings for tile changes.
**
**	@param x	Map X tile position of change.
**	@param y	Map Y tile position of change.
*/
global void EditorTileChanged(int x, int y)
{
    EditorTileChanged2(x, y, 0xF);
}

/**
**	Make random map
**      FIXME: vladi: we should have parameters control here...
*/

/**
**	TileFill
**
**	@param x	X map tile coordinate for area center.
**	@param y	Y map tile coordinate for area center.
**	@param tile	Tile type to edit.
**	@param size	Size of surrounding rectangle.
**
**      TileFill( centerx, centery, tile_type_water, map_width )
**      will fill map with water...
*/
local void TileFill(int x, int y, int tile, int size)
{
    int ix;
    int ax;
    int iy;
    int ay;


    ix = x - size / 2;
    ax = x + size / 2;
    iy = y - size / 2;
    ay = y + size / 2;

    if ( ix < 0 ) {
	ix = 0;
    }
    if ( ax >= TheMap.Width ) {
	ax = TheMap.Width - 1;
    }
    if ( iy < 0 ) {
	iy = 0;
    }
    if ( ay >= TheMap.Height ) {
	ay = TheMap.Height - 1;
    }

    for( x = ix; x <= ax; x++ ) {
	for( y = iy; y <= ay; y++ ) {
	    EditorChangeTile( x, y, tile, 15 );
	}
    }
}

#define WATER_TILE      0x10
#define COAST_TILE      0x30
#define GRASS_TILE      0x50
#define WOOD_TILE       0x70
#define ROCK_TILE       0x80

/**
**	FIXME: docu
*/
local void EditorRandomizeTile( int tile, int count, int max_size )
{
    int mx;
    int my;
    int i;
    int rx;
    int ry;
    int rz;

    mx = TheMap.Width;
    my = TheMap.Height;

    for( i = 0; i < count; i++ ) {
	rx = rand() % (mx / 2);
	ry = rand() % (my / 2);
	rz = rand() % max_size + 1;
    
	TileFill( rx, ry, tile, rz );
	TileFill( mx - rx - 1, ry, tile, rz );
	TileFill( rx, my - ry - 1, tile, rz );
	TileFill( mx - rx - 1, mx - ry - 1, tile, rz );
    }
}

/**
**	FIXME: docu
*/
local void EditorRandomizeUnit( const char *unit_type, int count, int value )
{
    int mx;
    int my;
    int i;
    int rx;
    int ry;
    int tile;
    int z;
    int tw;
    int th;
    UnitType* type;
    Unit *unit;

    mx = TheMap.Width;
    my = TheMap.Height;
    type = UnitTypeByIdent(unit_type);
    tw = type->TileWidth;
    th = type->TileHeight;

    for( i = 0; i < count; i++ ) {
	rx = rand() % (mx / 2 - tw + 1);
	ry = rand() % (my / 2 - th + 1);
	tile = GRASS_TILE;
	z = type->TileHeight;

	// FIXME: vladi: the idea is simple: make proper land for unit(s) :)
	// FIXME: handle units larger than 1 square
	TileFill( rx, ry, tile, z*2 );
	TileFill( mx - rx - 1, ry, tile, z*2 );
	TileFill( rx, my - ry - 1, tile, z*2 );
	TileFill( mx - rx - 1, mx - ry - 1, tile, z*2 );

	// FIXME: can overlap units
	unit=MakeUnitAndPlace( rx, ry , type, &Players[15]);
	unit->Value=value;
	unit=MakeUnitAndPlace( mx - rx - tw, ry, type, &Players[15]);
	unit->Value=value;
	unit=MakeUnitAndPlace( rx, my - ry - th, type, &Players[15]);
	unit->Value=value;
	unit=MakeUnitAndPlace( mx - rx - tw, mx - ry - th, type, &Players[15]);
	unit->Value=value;
    }
}

/**
**	Destroy all units
*/
local void EditorDestroyAllUnits(void)
{
    Unit* unit;

    while( NumUnits != 0 ) {
	unit=Units[0];
	RemoveUnit( unit,NULL ); 
	UnitLost( unit ); 
	UnitClearOrders( unit );
	ReleaseUnit( unit ); 
    }
}

/**
**	Create a random map
*/
global void EditorCreateRandomMap(void)
{
    int mz;
 
    mz = TheMap.Width > TheMap.Height ? TheMap.Width : TheMap.Height;

    // make water-base
    TileFill( 0, 0, WATER_TILE, mz * 3 );
    // remove all units
    EditorDestroyAllUnits();

    EditorRandomizeTile( COAST_TILE, 10, 16 );
    EditorRandomizeTile( GRASS_TILE, 20, 16 );
    EditorRandomizeTile( WOOD_TILE,  60,  4 );
    EditorRandomizeTile( ROCK_TILE,  30,  2 );

    EditorRandomizeUnit( "unit-gold-mine",  5,  50000 );
}

//@}
