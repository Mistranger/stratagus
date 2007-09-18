//     ____                _       __               
//    / __ )____  _____   | |     / /___ ___________
//   / __  / __ \/ ___/   | | /| / / __ `/ ___/ ___/
//  / /_/ / /_/ (__  )    | |/ |/ / /_/ / /  (__  ) 
// /_____/\____/____/     |__/|__/\__,_/_/  /____/  
//                                              
//       A futuristic real-time strategy game.
//          This file is part of Bos Wars.
//
/**@name script_map.cpp - The map ccl functions. */
//
//      (c) Copyright 1999-2007 by Lutz Sammer and Jimmy Salmon
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
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

#include <string.h>
#include <stdio.h>

#include "stratagus.h"
#include "unit.h"
#include "unit_cache.h"
#include "script.h"
#include "map.h"
#include "tileset.h"
#include "minimap.h"
#include "ui.h"
#include "player.h"
#include "iolib.h"
#include "video.h"
#include "version.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Parse a map.
**
**  @param l  Lua state.
*/
static int CclStratagusMap(lua_State *l)
{
	const char *value;
	int args;
	int subargs;

	//
	//  Parse the list: (still everything could be changed!)
	//

	args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		value = LuaToString(l, j + 1);
		++j;

		if (!strcmp(value, "version")) {
			char buf[32];

			value = LuaToString(l, j + 1);
			sprintf(buf, StratagusFormatString, StratagusFormatArgs(StratagusVersion));
			if (strcmp(buf, value)) {
				fprintf(stderr, "Warning not saved with this version.\n");
			}
		} else if (!strcmp(value, "uid")) {
			Map.Info.MapUID = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "description")) {
			Map.Info.Description = LuaToString(l, j + 1);
		} else if (!strcmp(value, "the-map")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, j + 1, k + 1);
				value = LuaToString(l, -1);
				lua_pop(l, 1);
				++k;

				if (!strcmp(value, "size")) {
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument");
					}
					lua_rawgeti(l, -1, 1);
					Map.Info.MapWidth = LuaToNumber(l, -1);
					lua_pop(l, 1);
					lua_rawgeti(l, -1, 2);
					Map.Info.MapHeight = LuaToNumber(l, -1);
					lua_pop(l, 1);
					lua_pop(l, 1);

					delete[] Map.Fields;
					Map.Fields = new CMapField[Map.Info.MapWidth * Map.Info.MapHeight];
					Map.Visible[0] = new unsigned[Map.Info.MapWidth * Map.Info.MapHeight / 2];
					memset(Map.Visible[0], 0, Map.Info.MapWidth * Map.Info.MapHeight / 2 * sizeof(unsigned));
					UnitCache.Init(Map.Info.MapWidth, Map.Info.MapHeight);
					// FIXME: this should be CreateMap or InitMap?
				} else if (!strcmp(value, "fog-of-war")) {
					Map.NoFogOfWar = false;
					--k;
				} else if (!strcmp(value, "no-fog-of-war")) {
					Map.NoFogOfWar = true;
					--k;
				} else if (!strcmp(value, "filename")) {
					lua_rawgeti(l, j + 1, k + 1);
					Map.Info.Filename = LuaToString(l, -1);
					lua_pop(l, 1);
				} else if (!strcmp(value, "map-fields")) {
					int i;
					int subsubargs;
					int subk;

					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument");
					}

					subsubargs = luaL_getn(l, -1);
					if (subsubargs != Map.Info.MapWidth * Map.Info.MapHeight) {
						fprintf(stderr, "Wrong tile table length: %d\n", subsubargs);
					}
					i = 0;
					for (subk = 0; subk < subsubargs; ++subk) {
						int args2;
						int j2;

						lua_rawgeti(l, -1, subk + 1);
						if (!lua_istable(l, -1)) {
							LuaError(l, "incorrect argument");
						}
						args2 = luaL_getn(l, -1);
						j2 = 0;

						lua_rawgeti(l, -1, j2 + 1);
						Map.Fields[i].Tile = LuaToNumber(l, -1);
						lua_pop(l, 1);
						++j2;
						lua_rawgeti(l, -1, j2 + 1);
						Map.Fields[i].SeenTile = LuaToNumber(l, -1);
						lua_pop(l, 1);
						++j2;
						for (; j2 < args2; ++j2) {
							lua_rawgeti(l, -1, j2 + 1);
							value = LuaToString(l, -1);
							lua_pop(l, 1);
							if (!strcmp(value, "explored")) {
								++j2;
								lua_rawgeti(l, -1, j2 + 1);
								Map.Fields[i].Visible[(int)LuaToNumber(l, -1)] = 1;
								lua_pop(l, 1);

							} else if (!strcmp(value, "land")) {
								Map.Fields[i].Flags |= MapFieldLandAllowed;
							} else if (!strcmp(value, "coast")) {
								Map.Fields[i].Flags |= MapFieldCoastAllowed;
							} else if (!strcmp(value, "water")) {
								Map.Fields[i].Flags |= MapFieldWaterAllowed;

							} else if (!strcmp(value, "mud")) {
								Map.Fields[i].Flags |= MapFieldNoBuilding;
							} else if (!strcmp(value, "block")) {
								Map.Fields[i].Flags |= MapFieldUnpassable;

							} else if (!strcmp(value, "ground")) {
								Map.Fields[i].Flags |= MapFieldLandUnit;
							} else if (!strcmp(value, "air")) {
								Map.Fields[i].Flags |= MapFieldAirUnit;
							} else if (!strcmp(value, "sea")) {
								Map.Fields[i].Flags |= MapFieldSeaUnit;
							} else if (!strcmp(value, "building")) {
								Map.Fields[i].Flags |= MapFieldBuilding;

							} else {
							   LuaError(l, "Unsupported tag: %s" _C_ value);
							}
						}
						lua_pop(l, 1);
						++i;
					}
					lua_pop(l, 1);
				} else {
				   LuaError(l, "Unsupported tag: %s" _C_ value);
				}
			}

		} else {
		   LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}

	return 0;
}

/**
**  Reveal the complete map.
**
**  @param l  Lua state.
*/
static int CclRevealMap(lua_State *l)
{
	LuaCheckArgs(l, 0);
	if (CclInConfigFile || !Map.Fields) {
		FlagRevealMap = 1;
	} else {
		Map.Reveal();
	}

	return 0;
}

/**
**  Center the map.
**
**  @param l  Lua state.
*/
static int CclCenterMap(lua_State *l)
{
	LuaCheckArgs(l, 2);
	UI.SelectedViewport->Center(
		LuaToNumber(l, 1), LuaToNumber(l, 2), TileSizeX / 2, TileSizeY / 2);

	return 0;
}

/**
**  Set fog of war on/off.
**
**  @param l  Lua state.
*/
static int CclSetFogOfWar(lua_State *l)
{
	LuaCheckArgs(l, 1);
	Map.NoFogOfWar = !LuaToBoolean(l, 1);
	if (!CclInConfigFile && Map.Fields) {
		UpdateFogOfWarChange();
		// FIXME: save setting in replay log
		//CommandLog("input", NoUnitP, FlushCommands, -1, -1, NoUnitP, "fow off", -1);
	}
	return 0;
}

static int CclGetFogOfWar(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushboolean(l, !Map.NoFogOfWar);
	return 1;
}

/**
**  Enable display of terrain in minimap.
**
**  @param l  Lua state.
*/
static int CclSetMinimapTerrain(lua_State *l)
{
	LuaCheckArgs(l, 1);
	UI.Minimap.WithTerrain = LuaToBoolean(l, 1);
	return 0;
}

/**
**  Fog of war opacity.
**
**  @param l  Lua state.
*/
static int CclSetFogOfWarOpacity(lua_State *l)
{
	int i;

	LuaCheckArgs(l, 1);
	i = LuaToNumber(l, 1);
	if (i < 0 || i > 255) {
		PrintFunction();
		fprintf(stdout, "Opacity should be 0 - 255\n");
		i = 128;
	}
	FogOfWarOpacity = i;

	if (!CclInConfigFile) {
		Map.Init();
	}

	return 0;
}

/**
**  Define Fog graphics
**
**  @param l  Lua state.
*/
static int CclSetFogOfWarGraphics(lua_State *l)
{
	std::string FogGraphicFile;

	LuaCheckArgs(l, 1);
	FogGraphicFile = LuaToString(l, 1);
	if (CMap::FogGraphic) {
		CGraphic::Free(CMap::FogGraphic);
	}
	CMap::FogGraphic = CGraphic::New(FogGraphicFile, TileSizeX, TileSizeY);

	return 0;
}

/**
**  Set a tile
**
**  @param tile   Tile number
**  @param w      X coordinate
**  @param h      Y coordinate
**  @param value  Value of the tile
*/
void SetTile(int tile, int w, int h, int value)
{
	if (w < 0 || w >= Map.Info.MapWidth) {
		fprintf(stderr, "Invalid map width: %d\n", w);
		return;
	}
	if (h < 0 || h >= Map.Info.MapHeight) {
		fprintf(stderr, "Invalid map height: %d\n", h);
		return;
	}
	if (tile < 0 || tile >= Map.Tileset.NumTiles) {
		fprintf(stderr, "Invalid tile number: %d\n", tile);
		return;
	}
	if (value < 0 || value >= 256) {
		fprintf(stderr, "Invalid tile value: %d\n", value);
		return;
	}

	if (Map.Fields) {
		Map.Field(w, h)->Tile = Map.Tileset.Table[tile];
		Map.Field(w, h)->Flags = Map.Tileset.FlagsTable[tile];
		Map.Field(w, h)->Cost = 
			1 << (Map.Tileset.FlagsTable[tile] & MapFieldSpeedMask);
	}
}

/**
**  Define the type of each player available for the map
**
**  @param l  Lua state.
*/
static int CclDefinePlayerTypes(lua_State *l)
{
	const char *type;
	int numplayers;
	int i;

	numplayers = lua_gettop(l); /* Number of players == number of arguments */
	if (numplayers < 2) {
		LuaError(l, "Not enough players");
	}

	for (i = 0; i < numplayers && i < PlayerMax; ++i) {
		if (lua_isnil(l, i + 1)) {
			numplayers = i;
			break;
		}
		type = LuaToString(l, i + 1);
		if (!strcmp(type, "neutral")) {
			Map.Info.PlayerType[i] = PlayerNeutral;
		} else if (!strcmp(type, "nobody")) {
			Map.Info.PlayerType[i] = PlayerNobody;
		} else if (!strcmp(type, "computer")) {
			Map.Info.PlayerType[i] = PlayerComputer;
		} else if (!strcmp(type, "person")) {
			Map.Info.PlayerType[i] = PlayerPerson;
		} else if (!strcmp(type, "rescue-passive")) {
			Map.Info.PlayerType[i] = PlayerRescuePassive;
		} else if (!strcmp(type, "rescue-active")) {
			Map.Info.PlayerType[i] = PlayerRescueActive;
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ type);
		}
	}
	for (i = numplayers; i < PlayerMax - 1; ++i) {
		Map.Info.PlayerType[i] = PlayerNobody;
	}
	if (numplayers < PlayerMax) {
		Map.Info.PlayerType[PlayerMax - 1] = PlayerNeutral;
	}
	return 0;
}

/**
**  Load the lua file which will define the tile models
**
**  @param l  Lua state.
*/
static int CclLoadTileModels(lua_State *l)
{
	char buf[PATH_MAX];

	LuaCheckArgs(l, 1);
	Map.TileModelsFileName = LuaToString(l, 1);
	LibraryFileName(Map.TileModelsFileName.c_str(), buf, sizeof(buf));
	if (LuaLoadFile(buf) == -1) {
		DebugPrint("Load failed: %s\n" _C_ LuaToString(l, 1));
	}
	return 0;
}

/**
**  Register CCL features for map.
*/
void MapCclRegister(void)
{
	lua_register(Lua, "StratagusMap", CclStratagusMap);
	lua_register(Lua, "RevealMap", CclRevealMap);
	lua_register(Lua, "CenterMap", CclCenterMap);

	lua_register(Lua, "SetFogOfWar", CclSetFogOfWar);
	lua_register(Lua, "GetFogOfWar", CclGetFogOfWar);
	lua_register(Lua, "SetMinimapTerrain", CclSetMinimapTerrain);

	lua_register(Lua, "SetFogOfWarGraphics", CclSetFogOfWarGraphics);
	lua_register(Lua, "SetFogOfWarOpacity", CclSetFogOfWarOpacity);

	lua_register(Lua, "LoadTileModels", CclLoadTileModels);
	lua_register(Lua, "DefinePlayerTypes", CclDefinePlayerTypes);
}

//@}
