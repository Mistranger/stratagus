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
/**@name ccl_pathfinder.c	-	pathfinder ccl functions. */
//
//	(c) Copyright 2000-2003 by Lutz Sammer, Fabrice Rossi, Latimerius.
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
--		Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"
#include "video.h"
#include "tileset.h"
#include "map.h"
#include "sound_id.h"
#include "unitsound.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "ccl.h"
#include "pathfinder.h"

/*----------------------------------------------------------------------------
--		Functions
----------------------------------------------------------------------------*/

/**
**		Enable a*.
*/
local int CclAStar(lua_State* l)
{
	const char* value;
	int i;
	int j;
	int args;

	args = lua_gettop(l);
	for (j = 0; j < args; ++j) {
		value = LuaToString(l, j + 1);
		if (!strcmp(value, "fixed-unit-cost")) {
			++j;
			i = LuaToNumber(l, j + 1);
			if (i <= 3) {
				PrintFunction();
				fprintf(stdout, "Fixed unit crossing cost must be strictly > 3\n");
			} else {
				AStarFixedUnitCrossingCost = i;
			}
		} else if (!strcmp(value, "moving-unit-cost")) {
			++j;
			i = LuaToNumber(l, j + 1);
			if (i <= 3) {
				PrintFunction();
				fprintf(stdout, "Moving unit crossing cost must be strictly > 3\n");
			} else {
				AStarMovingUnitCrossingCost = i;
			}
		} else if (!strcmp(value, "know-unseen-terrain")) {
			AStarKnowUnknown = 1;
		} else if (!strcmp(value, "dont-know-unseen-terrain")) {
			AStarKnowUnknown = 0;
		} else if (!strcmp(value, "unseen-terrain-cost")) {
			++j;
			i = LuaToNumber(l, j + 1);
			if (i < 0) {
				PrintFunction();
				fprintf(stdout, "Unseen Terrain Cost must be non-negative\n");
			} else {
				AStarUnknownTerrainCost = i;
			}
		} else {
			lua_pushfstring(l, "Unsupported tag: %s", value);
			lua_error(l);
		}
	}

	return 0;
}

#ifdef HIERARCHIC_PATHFINDER
local int CclPfHierShowRegIds(lua_State* l)
{
	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	PfHierShowRegIds = LuaToBoolean(l, 1);
	return 0;
}

local int CclPfHierShowGroupIds(lua_State* l)
{
	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	PfHierShowGroupIds = LuaToBoolean(l, 1);
	return 0;
}
#else
local int CclPfHierShowRegIds(lua_State* l)
{
	return 0;
}

local int CclPfHierShowGroupIds(lua_State* l)
{
	return 0;
}

#ifdef MAP_REGIONS
global void MapSplitterDebug(void);

local int CclDebugRegions(lua_State* l)
{
	MapSplitterDebug();
	return 0;
}
#endif // MAP_REGIONS

#endif


/**
**		Register CCL features for pathfinder.
*/
global void PathfinderCclRegister(void)
{
	lua_register(Lua, "AStar", CclAStar);
#ifdef MAP_REGIONS
	lua_register(Lua, "DebugRegions", CclDebugRegions);
#endif // MAP_REGIONS
	lua_register(Lua, "PfShowRegids", CclPfHierShowRegIds);
	lua_register(Lua, "PfShowGroupids", CclPfHierShowGroupIds);
}

//@}
