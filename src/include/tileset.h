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
/**@name tileset.h	-	The tileset headerfile. */
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

#ifndef __TILESET_H__
#define __TILESET_H__

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include "etlib/hash.h"

/*----------------------------------------------------------------------------
--	Documentation
----------------------------------------------------------------------------*/

/**
**	@struct _tileset_ tileset.h
**
**	\#include "tileset.h"
**
**	typedef struct _tileset_ Tileset;
**
**	This structure contains all informations about the tileset of the map.
**	It defines the look and properties of the tiles. Currently only one
**	tileset pro map is supported. In the future it is planned to support
**	multiple tilesets on the same map. Also is planned to support animated
**	tiles.
**	Currently the tilesize is fixed to 32x32 pixels, to support later other
**	sizes please use always the constants ::TileSizeX and ::TileSizeY.
**
**	The tileset structure members:
**
**	Tileset::Ident
**
**		Unique identifier (FE.: tileset-summer, tileset-winter) for the
**		tileset. Used by the map to define which tileset should be used.
**		Like always the identifier should only be used during
**		configuration and not during runtime!
**		@see WorldMap, WorldMap::TerrainName.
**
**	Tileset::Class
**
**		Identifier for the tileset class. All exchangable tilesets
**		should have the same class. Can be used by the level editor.
**
**	Tileset::Name
**
**		Long name of the tileset. Can be used by the level editor.
**
**	Tileset::File
**
**		Name of the graphic file, containing all tiles. Following
**		widths are supported:
**			@li 512 pixel: 16 tiles pro row 
**			@li 527 pixel: 16 tiles pro row with 1 pixel gap
**			@li 626 pixel: 19 tiles pro row with 1 pixel gap
**
**	Tileset::PaletteFile
**
**		Name of the palette file, containing the RGB colors to use
**		as global sytem palette.
**		@see GlobalPalette, VideoSetPalette.
**
**	Tileset::NumTiles
**
**		The number of different tiles in the tables.
**
**	Tileset::Table
**
**		Table to map the abstract level (PUD) tile numbers, to tile
**		numbers in the graphic file (Tileset::File).
**		FE. 16 (solid light water) in pud to 328 in png. 
**
**	Tileset::FlagsTable
**
**		Table of the tile flags used by the editor.
**		@see MapField::Flags
**
**	Tileset::BasicNameTable
**
**		Index to name of the basic tile type. FE. "light-water".
**		If the index is 0, the tile is not used.
**		@see Tileset::TileNames
**
**	Tileset::MixedNameTable
**
**		Index to name of the mixed tile type. FE. "light-water".
**		If this index is 0, the tile is a solid tile.
**		@see Tileset::TileNames
**
**	Tileset::TileTypeTable
**
**		Lookup table of the tile type. Maps the graphic file tile
**		number back to a tile type (::TileTypeWood, ::TileTypeWater,
**		...)
**
**		@note The creation of this table is currently hardcoded in
**		the engine. It should be calculated from the flags in the
**		tileset configuration (CCL). And it is created for the map
**		and not for the tileset.
**
**		@note I'm not sure if this table is needed in the future.
**
**		@see TileType.
**
**	Tileset::AnimationTable
**
**		Contains the animation of tiles.
**
**		@note This is currently not used.
**
**	Tileset::NumNames
**
**		Number of different tile names.
**
**	Tileset::TileNames
**
**		The different tile names. FE "light-grass", "dark-water".
**
**	Tileset::ExtraTrees[6]
**
**		This are six extra tile numbers, which are needed for lumber
**		chopping.
**
**	Tileset::TopOneTree
**
**		The tile number of tile only containing the top part of a tree.
**		Is created on the map by lumber chopping.
**
**	Tileset::MidOneTree
**
**		The tile number of tile only containing the connection of
**		the top part to the bottom part of tree.
**		Is created on the map by lumber chopping.
**
**	Tileset::BotOneTree
**
**		The tile number of tile only containing the bottom part of a
**		tree. Is created on the map by lumber chopping.
**
**	Tileset::RemovedTree
**
**		The tile number of the tile placed where trees are removed.
**		Is created on the map by lumber chopping.
**
**	Tileset::GrowingTree[2]
**
**		Contains the tile numbers of a growing tree from small to big.
**		@note Not yet used.
**
**	Tilset::WoodTable[16]
**
**		Table for wood removable. This table contains the tile which
**		is placed after a tree removement, depending on the surrounding.
**
**	Tileset::ExtraRocks[6]
**
**		This are six extra tile numbers, which are needed if rocks are
**		destroyed.
**
**	Tileset::TopOneRock
**
**		The tile number of tile only containing the top part of a rock.
**		Is created on the map by destroying rocks.
**
**	Tileset::MidOneRock
**
**		The tile number of tile only containing the connection of
**		the top part to the bottom part of a rock.
**		Is created on the map by destroying rocks.
**
**	Tileset::BotOneRock
**
**		The tile number of tile only containing the bottom part of a
**		rock.  Is created on the map by destroying rocks.
**
**	Tileset::RemovedRock
**
**		The tile number of the tile placed where rocks are removed.
**		Is created on the map by destroying rocks.
**
**	Tileset::RockTable[20]
**
**		Table for rock removable. Depending on the surrinding this
**		table contains the new tile to be placed.
**
**	@todo	Johns: I don't think this table or routines look correct.
**		But they work correct.
**
**	Tileset::HumanWallTable
**
**		Table of human wall tiles, index depends on the surroundings.
**
**	Tileset::OrcWallTable
**
**		Table of orc wall tiles, index depends on the surroundings.
**
**	Tileset::ItemsHash
**
**		Hash table of item numbers to unit names.
*/

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

#define TileSizeX	32		/// Size of a tile in X
#define TileSizeY	32		/// Size of a tile in Y

// This is only used for tile cache size
#define MaxTilesInTileset	5056	/// Current limit of tiles in tileset

/**
**	These are used for lookup tiles types
**	mainly used for the FOW implementation of the seen woods/rocks
**
**	@todo FIXME: I think this can be removed, we can use the flags?
**	I'm not sure, if we have seen and real time to considere.
*/
typedef enum _tile_type_ {
    TileTypeUnknown,			/// Unknown tile type
    TileTypeWood,			/// Any wood tile
    TileTypeRock,			/// Any rock tile
    TileTypeCoast,			/// Any coast tile
    TileTypeHumanWall,			/// Any human wall tile
    TileTypeOrcWall,			/// Any orc wall tile
    TileTypeWater,			/// Any water tile
} TileType;

    ///	Tileset definition
typedef struct _tileset_ {
    char*	Ident;			/// Tileset identifier
    char*	File;			/// CCL file containing tileset data
    char*	Class;			/// Class for future extensions
    char*	Name;			/// Nice name to display
    char*	ImageFile;		/// File containing image data
    char*	PaletteFile;		/// File containing the global palette

    int		NumTiles;		/// Number of tiles in the tables
    unsigned short*	Table;		/// Pud to internal conversion table
    unsigned short*	FlagsTable;	/// Flag table for editor

    unsigned char*	BasicNameTable;	/// Basic tile name/type
    unsigned char*	MixedNameTable;	/// Mixed tile name/type

    // FIXME: currently hardcoded
    unsigned char*	TileTypeTable;	/// For fast lookup of tile type
    // FIXME: currently unsupported
    unsigned short*	AnimationTable;	/// Tile animation sequences

    int		NumNames;		/// Number of different tile names
    char**	TileNames;		/// Tile names/types

    unsigned	ExtraTrees[6];		/// Extra tree tiles for removing
    unsigned	TopOneTree;		/// Tile for one tree top
    unsigned	MidOneTree;		/// Tile for one tree middle
    unsigned	BotOneTree;		/// Tile for one tree bottom
    int		RemovedTree;		/// Tile placed where trees are gone
    unsigned	GrowingTree[2];		/// Growing tree tiles
    int		WoodTable[16];		/// Table for tree removable

    unsigned	ExtraRocks[6];		/// Extra rock tiles for removing
    unsigned	TopOneRock;		/// Tile for one rock top
    unsigned	MidOneRock;		/// Tile for one rock middle
    unsigned	BotOneRock;		/// Tile for one rock bottom
    int		RemovedRock;		/// Tile placed where rocks are gone
    int		RockTable[20];		/// Removed rock placement table

    unsigned	HumanWallTable[16];	/// Human wall placement table
    unsigned	OrcWallTable[16];	/// Orc wall placement table

    hashtable(char*,128) ItemsHash;	/// Items hash table
} Tileset;

// FIXME: this #define's should be removed

enum _tileset_nr_ {
    TilesetSummer,			/// Reference number for summer
    TilesetWinter,			/// Reference number for winter
    TilesetWasteland,			/// Reference number for wasteland
    TilesetSwamp,			/// Reference number for swamp
};

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern char** TilesetWcNames;		/// Mapping wc-number 2 symbol

extern int NumTilesets;			/// Number of available tilesets
extern Tileset** Tilesets;		/// Tileset information

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

extern void LoadTileset(void);		/// Load tileset definition
extern void SaveTilesets(FILE*);	/// Save the tileset configuration
extern void CleanTilesets(void);	/// Cleanup the tileset module

extern void TilesetCclRegister(void);	/// Register CCL features for tileset

//@}

#endif // !__TILESET_H__
