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
/**@name tileset.c	-	The tileset. */
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

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freecraft.h"
#include "tileset.h"
#include "map.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern int WoodTable[16];		/// Table for wood removable.
extern int RockTable[20];		/// Table for rock removable.

/**
**	Mapping of wc numbers to our internal tileset symbols.
**	The numbers are used in puds.
**	0=summer, 1=winter, 2=wasteland, 3=swamp.
*/
global char** TilesetWcNames;

/**
**	Number of available Tilesets.
*/
global int NumTilesets;

/**
**	Tileset information.
**
**	@see TilesetMax, @see NumTilesets
*/
global Tileset** Tilesets;

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Load tileset and setup ::TheMap for this tileset.
**
**	@see TheMap @see Tilesets.
*/
global void LoadTileset(void)
{
    int i;
    int n;
    int tile;
    int gap;
    int tiles_per_row;
    int solid;
    int mixed;
    unsigned char* data;
    char* buf;
    const unsigned short *table;

    //
    //  Find the tileset.
    //
    for (i = 0; i < NumTilesets; ++i) {
	if (!strcmp(TheMap.TerrainName, Tilesets[i]->Ident)) {
	    break;
	}
    }
    if (i == NumTilesets) {
	fprintf(stderr, "Tileset `%s' not available\n", TheMap.TerrainName);
	ExitFatal(-1);
    }
    DebugCheck(i != TheMap.Terrain);
    TheMap.Tileset = Tilesets[i];

    //
    //  Load and prepare the tileset
    //
    buf = alloca(strlen(Tilesets[i]->File) + 9 + 1);
    strcat(strcpy(buf, "graphics/"), Tilesets[i]->File);
    ShowLoadProgress("\tTileset `%s'\n", Tilesets[i]->File);
    TheMap.TileData = LoadGraphic(buf);

    //
    //  Calculate number of tiles in graphic tile
    //
    if (TheMap.TileData->Width == 626) {
	// FIXME: allow 1 pixel gap between the tiles!!
	gap = 1;
	tiles_per_row = (TheMap.TileData->Width + 1) / (TileSizeX + 1);
	TheMap.TileCount = n =
	    tiles_per_row * ((TheMap.TileData->Height + 1) / (TileSizeY + 1));
    } else if (TheMap.TileData->Width == 527) {
	// FIXME: allow 1 pixel gap between the tiles!!
	gap = 1;
	tiles_per_row = (TheMap.TileData->Width + 1) / (TileSizeX + 1);
	TheMap.TileCount = n =
	    tiles_per_row * ((TheMap.TileData->Height + 1) / (TileSizeY + 1));
    } else {
	gap = 0;
	tiles_per_row = TheMap.TileData->Width / TileSizeX;
	TheMap.TileCount = n =
	    tiles_per_row * (TheMap.TileData->Height / TileSizeY);
    }

    DebugLevel2Fn(" %d Tiles in file %s, %d per row\n" _C_ TheMap.
	TileCount _C_ TheMap.Tileset->File _C_ tiles_per_row);

    if (n > MaxTilesInTileset) {
	fprintf(stderr,
	    "Too many tiles in tileset. Increase MaxTilesInTileset and recompile.\n");
	ExitFatal(-1);
    }

    //
    //  Precalculate the graphic starts of the tiles
    //
    data = malloc(n * TileSizeX * TileSizeY);
    TheMap.Tiles = malloc(n * sizeof(*TheMap.Tiles));
    for (i = 0; i < n; ++i) {
	TheMap.Tiles[i] = data + i * TileSizeX * TileSizeY;
    }

    //
    //  Convert the graphic data into faster format
    //
    for (tile = 0; tile < n; ++tile) {
	unsigned char *s;
	unsigned char *d;

	s = ((char *)TheMap.TileData->Frames)
	    + ((tile % tiles_per_row) * (TileSizeX + gap))
	    + ((tile / tiles_per_row) * (TileSizeY +
		gap)) * TheMap.TileData->Width;
	d = TheMap.Tiles[tile];
	if (d != data + tile * TileSizeX * TileSizeY) {
	    abort();
	}
	for (i = 0; i < TileSizeY; ++i) {
	    memcpy(d, s, TileSizeX * sizeof(unsigned char));
	    d += TileSizeX;
	    s += TheMap.TileData->Width;
	}
    }

    free(TheMap.TileData->Frames);	// release old memory
    TheMap.TileData->Frames = data;
    TheMap.TileData->Width = TileSizeX;
    TheMap.TileData->Height = TileSizeY * n;

    //
    //  Build the TileTypeTable
    //
    TheMap.Tileset->TileTypeTable =
	calloc(n, sizeof(*TheMap.Tileset->TileTypeTable));

    table = TheMap.Tileset->Table;
    n = TheMap.Tileset->NumTiles;
    for (i = 0; i < n; ++i) {
	if ((tile = table[i])) {
	    unsigned flags;

	    flags = TheMap.Tileset->FlagsTable[i];
	    if (flags & MapFieldWaterAllowed) {
		TheMap.Tileset->TileTypeTable[tile] = TileTypeWater;
	    } else if (flags & MapFieldCoastAllowed) {
		TheMap.Tileset->TileTypeTable[tile] = TileTypeCoast;
	    } else if (flags & MapFieldWall) {
		if (flags & MapFieldHuman) {
		    TheMap.Tileset->TileTypeTable[tile] = TileTypeHumanWall;
		} else {
		    TheMap.Tileset->TileTypeTable[tile] = TileTypeOrcWall;
		}
	    } else if (flags & MapFieldRocks) {
		TheMap.Tileset->TileTypeTable[tile] = TileTypeRock;
	    } else if (flags & MapFieldForest) {
		TheMap.Tileset->TileTypeTable[tile] = TileTypeWood;
	    }
	}
    }

    //
    //  mark the special tiles
    //
    for (i = 0; i < 6; ++i) {
	if ((tile = TheMap.Tileset->ExtraTrees[i])) {
	    TheMap.Tileset->TileTypeTable[tile] = TileTypeWood;
	}
	if ((tile = TheMap.Tileset->ExtraRocks[i])) {
	    TheMap.Tileset->TileTypeTable[tile] = TileTypeRock;
	}
    }
    if ((tile = TheMap.Tileset->TopOneTree)) {
	TheMap.Tileset->TileTypeTable[tile] = TileTypeWood;
    }
    if ((tile = TheMap.Tileset->MidOneTree)) {
	TheMap.Tileset->TileTypeTable[tile] = TileTypeWood;
    }
    if ((tile = TheMap.Tileset->BotOneTree)) {
	TheMap.Tileset->TileTypeTable[tile] = TileTypeWood;
    }
    if ((tile = TheMap.Tileset->TopOneRock)) {
	TheMap.Tileset->TileTypeTable[tile] = TileTypeRock;
    }
    if ((tile = TheMap.Tileset->MidOneRock)) {
	TheMap.Tileset->TileTypeTable[tile] = TileTypeRock;
    }
    if ((tile = TheMap.Tileset->BotOneRock)) {
	TheMap.Tileset->TileTypeTable[tile] = TileTypeRock;
    }

    //
    //  Build wood removement table.
    //
    n = TheMap.Tileset->NumTiles;
    for (mixed = solid = i = 0; i < n;) {
	if (TheMap.Tileset->BasicNameTable[i]
	    && TheMap.Tileset->MixedNameTable[i]) {
	    if (TheMap.Tileset->FlagsTable[i] & MapFieldForest) {
		mixed = i;
	    }
	    i += 256;
	} else {
	    if (TheMap.Tileset->FlagsTable[i] & MapFieldForest) {
		solid = i;
	    }
	    i += 16;
	}
    }
    WoodTable[ 0] = -1;
    WoodTable[ 1] = TheMap.Tileset->BotOneTree;
    WoodTable[ 2] = -1;
    WoodTable[ 3] = table[mixed + 0x10];
    WoodTable[ 4] = TheMap.Tileset->TopOneTree;
    WoodTable[ 5] = TheMap.Tileset->MidOneTree;
    WoodTable[ 6] = table[mixed + 0x70];
    WoodTable[ 7] = table[mixed + 0x90];
    WoodTable[ 8] = -1;
    WoodTable[ 9] = table[mixed + 0x00];
    WoodTable[10] = -1;
    WoodTable[11] = table[mixed + 0x20];
    WoodTable[12] = table[mixed + 0x30];
    WoodTable[13] = table[mixed + 0x40];
    WoodTable[14] = table[mixed + 0xB0];
    WoodTable[15] = table[mixed + 0xD0];

    //
    //  Build rock removement table.
    //
    for (mixed = solid = i = 0; i < n;) {
	if (TheMap.Tileset->BasicNameTable[i]
	    && TheMap.Tileset->MixedNameTable[i]) {
	    if (TheMap.Tileset->FlagsTable[i] & MapFieldRocks) {
		mixed = i;
	    }
	    i += 256;
	} else {
	    if (TheMap.Tileset->FlagsTable[i] & MapFieldRocks) {
		solid = i;
	    }
	    i += 16;
	}
    }
    RockTable[ 0] = -1;
    RockTable[ 1] = TheMap.Tileset->BotOneRock;
    RockTable[ 2] = -1;
    RockTable[ 3] = table[mixed + 0x10];
    RockTable[ 4] = TheMap.Tileset->TopOneRock;
    RockTable[ 5] = TheMap.Tileset->MidOneRock;
    RockTable[ 6] = table[mixed + 0x70];
    RockTable[ 7] = table[mixed + 0x90];
    RockTable[ 8] = -1;
    RockTable[ 9] = table[mixed + 0x00];
    RockTable[10] = -1;
    RockTable[11] = table[mixed + 0x20];
    RockTable[12] = table[mixed + 0x30];
    RockTable[13] = table[mixed + 0x40];
    RockTable[14] = table[mixed + 0xB0];
    RockTable[15] = table[mixed + 0x80];

    RockTable[16] = table[mixed + 0xC0];
    RockTable[17] = table[mixed + 0x60];
    RockTable[18] = table[mixed + 0xA0];
    RockTable[19] = table[mixed + 0xD0];

    //
    //	FIXME: Build wall replacement tables
    //
    TheMap.Tileset->HumanWallTable[ 0] = 0x090;
    TheMap.Tileset->HumanWallTable[ 1] = 0x830;
    TheMap.Tileset->HumanWallTable[ 2] = 0x810;
    TheMap.Tileset->HumanWallTable[ 3] = 0x850;
    TheMap.Tileset->HumanWallTable[ 4] = 0x800;
    TheMap.Tileset->HumanWallTable[ 5] = 0x840;
    TheMap.Tileset->HumanWallTable[ 6] = 0x820;
    TheMap.Tileset->HumanWallTable[ 7] = 0x860;
    TheMap.Tileset->HumanWallTable[ 8] = 0x870;
    TheMap.Tileset->HumanWallTable[ 9] = 0x8B0;
    TheMap.Tileset->HumanWallTable[10] = 0x890;
    TheMap.Tileset->HumanWallTable[11] = 0x8D0;
    TheMap.Tileset->HumanWallTable[12] = 0x880;
    TheMap.Tileset->HumanWallTable[13] = 0x8C0;
    TheMap.Tileset->HumanWallTable[14] = 0x8A0;
    TheMap.Tileset->HumanWallTable[15] = 0x0B0;

    TheMap.Tileset->OrcWallTable[ 0] = 0x0A0;
    TheMap.Tileset->OrcWallTable[ 1] = 0x930;
    TheMap.Tileset->OrcWallTable[ 2] = 0x910;
    TheMap.Tileset->OrcWallTable[ 3] = 0x950;
    TheMap.Tileset->OrcWallTable[ 4] = 0x900;
    TheMap.Tileset->OrcWallTable[ 5] = 0x940;
    TheMap.Tileset->OrcWallTable[ 6] = 0x920;
    TheMap.Tileset->OrcWallTable[ 7] = 0x960;
    TheMap.Tileset->OrcWallTable[ 8] = 0x970;
    TheMap.Tileset->OrcWallTable[ 9] = 0x9B0;
    TheMap.Tileset->OrcWallTable[10] = 0x990;
    TheMap.Tileset->OrcWallTable[11] = 0x9D0;
    TheMap.Tileset->OrcWallTable[12] = 0x980;
    TheMap.Tileset->OrcWallTable[13] = 0x9C0;
    TheMap.Tileset->OrcWallTable[14] = 0x9A0;
    TheMap.Tileset->OrcWallTable[15] = 0x0C0;
};

/**
**	Save flag part of tileset.
**
**	@param file	File handle for the saved flags.
**	@param flags	Bit field of the flags.
*/
local void SaveTilesetFlags(FILE* file, unsigned flags)
{
    if (flags & MapFieldWaterAllowed) {
	fprintf(file, " 'water");
    }
    if (flags & MapFieldLandAllowed) {
	fprintf(file, " 'land");
    }
    if (flags & MapFieldCoastAllowed) {
	fprintf(file, " 'coast");
    }
    if (flags & MapFieldNoBuilding) {
	fprintf(file, " 'no-building");
    }
    if (flags & MapFieldUnpassable) {
	fprintf(file, " 'unpassable");
    }
    if (flags & MapFieldWall) {
	fprintf(file, " 'wall");
    }
    if (flags & MapFieldRocks) {
	fprintf(file, " 'rock");
    }
    if (flags & MapFieldForest) {
	fprintf(file, " 'forest");
    }
    if (flags & MapFieldLandUnit) {
	fprintf(file, " 'land-unit");
    }
    if (flags & MapFieldAirUnit) {
	fprintf(file, " 'air-unit");
    }
    if (flags & MapFieldSeaUnit) {
	fprintf(file, " 'sea-unit");
    }
    if (flags & MapFieldBuilding) {
	fprintf(file, " 'building");
    }
    if (flags & MapFieldHuman) {
	fprintf(file, " 'human");
    }
}

/**
**	Save solid part of tileset.
**
**	@param file	File handle to save the solid part.
**	@param table	Tile numbers.
**	@param name	Ascii name of solid tile
**	@param flags	Tile attributes.
**	@param start	Start index into table.
*/
local void SaveTilesetSolid(FILE* file, const unsigned short* table,
    const char* name, unsigned flags, int start)
{
    int i;
    int j;
    int n;

    fprintf(file, "  'solid (list \"%s\"", name);
    SaveTilesetFlags(file, flags);
    // Remove empty tiles at end of block
    for (n = 15; n >= 0 && !table[start + n]; n--) {
    }
    i = fprintf(file, "\n    #(");
    for (j = 0; j <= n; ++j) {
	i += fprintf(file, " %3d", table[start + j]);
    }
    i += fprintf(file, "))");

    while ((i += 8) < 80) {
	fprintf(file, "\t");
    }
    fprintf(file, "; %03X\n", start);
}

/**
**	Save mixed part of tileset.
**
**	@param file	File handle to save the mixed part.
**	@param table	Tile numbers.
**	@param name1	First ascii name of mixed tiles.
**	@param name2	Second Ascii name of mixed tiles.
**	@param flags	Tile attributes.
**	@param start	Start index into table.
**	@param end	End of tiles.
*/
local void SaveTilesetMixed(FILE* file, const unsigned short* table,
    const char* name1, const char* name2, unsigned flags, int start, int end)
{
    int x;
    int i;
    int j;
    int n;

    fprintf(file, "  'mixed (list \"%s\" \"%s\"", name1, name2);
    SaveTilesetFlags(file, flags);
    fprintf(file,"\n");
    for (x = 0; x < 0x100; x += 0x10) {
	if (start + x >= end) {		// Check end must be 0x10 aligned
	    break;
	}
	fprintf(file, "    #(");
	// Remove empty slots at end of table
	for (n = 15; n >= 0 && !table[start + x + n]; n--) {
	}
	i = 6;
	for (j = 0; j <= n; ++j) {
	    i += fprintf(file, " %3d", table[start + x + j]);
	}
	if (x == 0xF0 || (start == 0x900 && x == 0xD0)) {
	    i += fprintf(file, "))");
	} else {
	    i += fprintf(file, ")");
	}

	while ((i += 8) < 80) {
	    fprintf(file, "\t");
	}
	fprintf(file, "; %03X\n", start + x);
    }
}

/**
**	Save the tileset.
**
**	@param file	Output file.
**	@param tileset	Save the content of this tileset.
*/
local void SaveTileset(FILE* file, const Tileset* tileset)
{
    const unsigned short* table;
    int i;
    int n;

    fprintf(file, "\n(define-tileset\n  '%s 'class '%s", tileset->Ident,
	tileset->Class);
    fprintf(file, "\n  'name \"%s\"", tileset->Name);
    fprintf(file, "\n  'image \"%s\"", tileset->File);
    fprintf(file, "\n  'palette \"%s\"", tileset->PaletteFile);
    fprintf(file, "\n  ;; Slots descriptions");
    fprintf(file,
	"\n  'slots (list\n  'special (list\t\t;; Can't be in pud\n");
    fprintf(file, "    'extra-trees #( %d %d %d %d %d %d )\n",
	tileset->ExtraTrees[0], tileset->ExtraTrees[1]
	, tileset->ExtraTrees[2], tileset->ExtraTrees[3]
	, tileset->ExtraTrees[4], tileset->ExtraTrees[5]);
    fprintf(file, "    'top-one-tree %d 'mid-one-tree %d 'bot-one-tree %d\n",
	tileset->TopOneTree, tileset->MidOneTree, tileset->BotOneTree);
    fprintf(file, "    'removed-tree %d\n", tileset->RemovedTree);
    fprintf(file, "    'growing-tree #( %d %d )\n", tileset->GrowingTree[0],
	tileset->GrowingTree[1]);
    fprintf(file, "    'extra-rocks #( %d %d %d %d %d %d )\n",
	tileset->ExtraRocks[0], tileset->ExtraRocks[1]
	, tileset->ExtraRocks[2], tileset->ExtraRocks[3]
	, tileset->ExtraRocks[4], tileset->ExtraRocks[5]);
    fprintf(file, "    'top-one-rock %d 'mid-one-rock %d 'bot-one-rock %d\n",
	tileset->TopOneRock, tileset->MidOneRock, tileset->BotOneRock);
    fprintf(file, "    'removed-rock %d )\n", tileset->RemovedRock);

    table = tileset->Table;
    n = tileset->NumTiles;

    for (i = 0; i < n;) {
	//
	//      Mixeds
	//
	if (tileset->BasicNameTable[i] && tileset->MixedNameTable[i]) {
	    SaveTilesetMixed(file, table,
		tileset->TileNames[tileset->BasicNameTable[i]],
		tileset->TileNames[tileset->MixedNameTable[i]],
		tileset->FlagsTable[i], i, n);
	    i += 256;
	    //
	    //      Solids
	    //
	} else {
	    SaveTilesetSolid(file, table,
		tileset->TileNames[tileset->BasicNameTable[i]],
		tileset->FlagsTable[i], i);
	    i += 16;
	}
    }
    fprintf(file, "  )\n");
    fprintf(file, "  ;; Animated tiles\n");
    fprintf(file, "  'animations (list #( ) )\n");
    fprintf(file, "  'objects (list #( ) ))\n");
}

/**
**	Save the current tileset module.
**
**	@param file	Output file.
*/
global void SaveTilesets(FILE* file)
{
    int i;
    char** sp;

    fprintf(file, "\n;;; -----------------------------------------\n");
    fprintf(file,
	";;; MODULE: tileset $Id$\n\n");

    //  Original number to internal tileset name

    i = fprintf(file, "(define-tileset-wc-names");
    for (sp = TilesetWcNames; *sp; ++sp) {
	if (i + strlen(*sp) > 79) {
	    i = fprintf(file, "\n ");
	}
	i += fprintf(file, " '%s", *sp);
    }
    fprintf(file, ")\n");

    // 	Save all loaded tilesets

    for (i = 0; i < NumTilesets; ++i) {
	SaveTileset(file, Tilesets[i]);
    }
}

/**
**	Cleanup the tileset module.
**
**	@note	this didn't frees the configuration memory.
*/
global void CleanTilesets(void)
{
    int i;
    int j;
    char** ptr;

    //
    //	Free the tilesets
    //
    for( i=0; i<NumTilesets; ++i ) {
	free(Tilesets[i]->Ident);
	free(Tilesets[i]->Class);
	free(Tilesets[i]->Name);
	free(Tilesets[i]->File);
	free(Tilesets[i]->PaletteFile);
	free(Tilesets[i]->Table);
	free(Tilesets[i]->FlagsTable);
	free(Tilesets[i]->BasicNameTable);
	free(Tilesets[i]->MixedNameTable);
	free(Tilesets[i]->TileTypeTable);
	free(Tilesets[i]->AnimationTable);
	for( j=0; j<Tilesets[i]->NumNames; ++j ) {
	    free(Tilesets[i]->TileNames[j]);
	}
	free(Tilesets[i]->TileNames);

	free(Tilesets[i]);
    }
    free(Tilesets);
    Tilesets=NULL;
    NumTilesets=0;

    //
    //	Should this be done by the map?
    //
    VideoSaveFree(TheMap.TileData);
    TheMap.TileData=NULL;
    free(TheMap.Tiles);
    TheMap.Tiles=NULL;

    //
    //	Mapping the original tileset numbers in puds to our internal strings
    //
    if( (ptr=TilesetWcNames) ) {	// Free all old names
	while( *ptr ) {
	    free(*ptr++);
	}
	free(TilesetWcNames);

	TilesetWcNames=NULL;
    }
}

//@}
