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
/**@name ccl.c - The craft configuration language. */
//
//      (c) Copyright 1998-2004 by Lutz Sammer and Jimmy Salmon
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
--		Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>

#include "stratagus.h"

#include "iocompat.h"

#include "iolib.h"
#include "ccl.h"
#include "missile.h"
#include "depend.h"
#include "upgrade.h"
#include "construct.h"
#include "unit.h"
#include "map.h"
#include "pud.h"
#include "ccl_sound.h"
#include "ui.h"
#include "interface.h"
#include "font.h"
#include "pathfinder.h"
#include "ai.h"
#include "campaign.h"
#include "trigger.h"
#include "settings.h"
#include "editor.h"
#include "sound.h"
#include "sound_server.h"
#include "netconnect.h"
#include "network.h"
#include "cdaudio.h"
#include "spells.h"


/*----------------------------------------------------------------------------
--		Variables
----------------------------------------------------------------------------*/

/// Uncomment this to enable additionnal check on GC operations
// #define DEBUG_GC

global lua_State* Lua;

global char* CclStartFile;              /// CCL start file
global char* GameName;                  /// Game Preferences
global int   CclInConfigFile;           /// True while config file parsing

global char* Tips[MAX_TIPS + 1];        /// Array of tips
global int   ShowTips;                  /// Show tips at start of level
global int   CurrentTip;                /// Current tip to display

/*----------------------------------------------------------------------------
--		Functions
----------------------------------------------------------------------------*/

/**
**  FIXME: docu
*/
local void lstop(lua_State *l, lua_Debug *ar)
{
	(void)ar;  // unused arg.
	lua_sethook(l, NULL, 0, 0);
	luaL_error(l, "interrupted!");
}

/**
**  FIXME: docu
*/
local void laction(int i)
{
	// if another SIGINT happens before lstop,
	// terminate process (default action)
	signal(i, SIG_DFL);
	lua_sethook(Lua, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}

/**
**  FIXME: docu
*/
local void l_message(const char *pname, const char *msg)
{
	if (pname) {
		fprintf(stderr, "%s: ", pname);
	}
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

/**
**  FIXME: docu
**
**  @param status  FIXME: docu
**
**  @return        FIXME: docu
*/
local int report(int status)
{
	const char* msg;

	if (status) {
		msg = lua_tostring(Lua, -1);
		if (msg == NULL) {
			msg = "(error with no message)";
		}
		l_message(NULL, msg);
		lua_pop(Lua, 1);
	}
	return status;
}

/**
**  Call a lua function
**
**  @param narg   Number of arguments
**  @param clear  Clear the return value(s)
**
**  @return       FIXME: docu
*/
global int LuaCall(int narg, int clear)
{
	int status;
	int base;

	base = lua_gettop(Lua) - narg;      // function index
	lua_pushliteral(Lua, "_TRACEBACK");
	lua_rawget(Lua, LUA_GLOBALSINDEX);  // get traceback function
	lua_insert(Lua, base);              // put it under chunk and args
	signal(SIGINT, laction);
	status = lua_pcall(Lua, narg, (clear ? 0 : LUA_MULTRET), base);
	signal(SIGINT, SIG_DFL);
	lua_remove(Lua, base);              // remove traceback function

	return report(status);
}

/**
**  Load a file and execute it
**
**  @param file  File to load and execute
**
**  @return      FIXME: docu
*/
global int LuaLoadFile(const char* file)
{
	int status;
	int size;
	int read;
	int location;
	char* buf;
	CLFile* fp;

	if (!(fp = CLopen(file, CL_OPEN_READ))) {
		perror("Can't open file");
		return -1;
	}
					
	size = 10000;
	buf = (char*)malloc(size);
	location = 0;
	while ((read = CLread(fp, &buf[location], size - location))) {
		location += read;
		size = size * 2;
		buf = (char*)realloc(buf, size);
		if (!buf) {
			fprintf(stderr, "Out of memory\n");
			ExitFatal(-1);
		}
	}
	CLclose(fp);

	if (!(status = luaL_loadbuffer(Lua, buf, location, file))) {
		LuaCall(0, 1);
	} else {
		report(status);
	}
	free(buf);
	return status;
}

/**
**  FIXME: docu
*/
local int CclLoad(lua_State* l)
{
	char buf[1024];

	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	LibraryFileName(LuaToString(l, 1), buf);
	if (LuaLoadFile(buf) == -1) {
		lua_pushfstring(l, "Load failed: %s", LuaToString(l, 1));
		lua_error(l);
	}
	return 0;
}

/**
**  FIXME: docu
*/
global const char* LuaToString(lua_State* l, int narg)
{
	luaL_checktype(l, narg, LUA_TSTRING);
	return lua_tostring(l, narg);
}

/**
**  FIXME: docu
*/
global lua_Number LuaToNumber(lua_State* l, int narg)
{
	luaL_checktype(l, narg, LUA_TNUMBER);
	return lua_tonumber(l, narg);
}

/**
**  FIXME: docu
*/
global int LuaToBoolean(lua_State* l, int narg)
{
	luaL_checktype(l, narg, LUA_TBOOLEAN);
	return lua_toboolean(l, narg);
}

/**
**		Perform CCL garbage collection
**
**		@param fast		set this flag to disable slow GC ( during game )
*/
global void CclGarbageCollect(int fast)
{
}

/*............................................................................
..		Config
............................................................................*/

/**
**		Return the stratagus library path.
**
**		@return				Current libray path.
*/
local int CclStratagusLibraryPath(lua_State* l)
{
	lua_pushstring(l, StratagusLibPath);
	return 1;
}

/**
**		Return the stratagus game-cycle
**
**		@return				Current game cycle.
*/
local int CclGameCycle(lua_State* l)
{
	lua_pushnumber(l, GameCycle);
	return 1;
}

/**
**	  Return of game name.
**
**	  @param gamename		SCM name. (nil reports only)
**
**	  @return				Old game name.
*/
local int CclSetGameName(lua_State* l)
{
	char* old;
	int args;

	args = lua_gettop(l);
	if (args > 1 || (args == 1 && (!lua_isnil(l, 1) && !lua_isstring(l, 1)))) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	old = NULL;
	if (GameName) {
		old = strdup(GameName);
	}
	if (args == 1 && !lua_isnil(l, 1)) {
		if (GameName) {
			free(GameName);
			GameName = NULL;
		}

		GameName = strdup(lua_tostring(l, 1));
	}

	lua_pushstring(l, old);
	free(old);
	return 1;
}

/**
**		Set the stratagus game-cycle
*/
local int CclSetGameCycle(lua_State* l)
{
	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	GameCycle = LuaToNumber(l, 1);
	return 0;
}

/**
**		Set the game paused or unpaused
*/
local int CclSetGamePaused(lua_State* l)
{
	if (lua_gettop(l) != 1 || (!lua_isnumber(l, 1) && !lua_isboolean(l, 1))) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	if (lua_isboolean(l, 1)) {
		GamePaused = lua_toboolean(l, 1);
	} else {
		GamePaused = lua_tonumber(l, 1);
	}
	return 0;
}

/**
**		Set the video sync speed
*/
local int CclSetVideoSyncSpeed(lua_State* l)
{
	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	VideoSyncSpeed = LuaToNumber(l, 1);
	return 0;
}

/**
**		Set the local player name
*/
local int CclSetLocalPlayerName(lua_State* l)
{
	const char* str;

	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	str = LuaToString(l, 1);
	strncpy(LocalPlayerName, str, sizeof(LocalPlayerName) - 1);
	LocalPlayerName[sizeof(LocalPlayerName) - 1] = '\0';
	return 0;
}

/**
**		Set God mode.
**
**		@return				The old mode.
*/
local int CclSetGodMode(lua_State* l)
{
	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	lua_pushboolean(l, GodMode);
	GodMode = LuaToBoolean(l, 1);
	return 0;
}

/**
**		Enable/disable Showing the tips at the start of a level.
**
**		@param flag		True = turn on, false = off.
**		@return				The old state of tips displayed.
*/
local int CclSetShowTips(lua_State* l)
{
	int old;

	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	old = ShowTips;
	ShowTips = LuaToBoolean(l, 1);

	lua_pushboolean(l, old);
	return 1;
}

/**
**		Set the current tip number.
**
**		@param tip		Tip number.
**		@return				The old tip number.
*/
local int CclSetCurrentTip(lua_State* l)
{
	lua_Number old;

	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	old = CurrentTip;
	CurrentTip = LuaToNumber(l, 1);
	if (CurrentTip >= MAX_TIPS || Tips[CurrentTip] == NULL) {
		CurrentTip = 0;
	}

	lua_pushnumber(l, old);
	return 1;
}

/**
**		Add a new tip to the list of tips.
**
**		@param tip		A new tip to be displayed before level.
**
**		@todo		FIXME:		Memory for tips is never freed.
**				FIXME:		Make Tips dynamic.
*/
local int CclAddTip(lua_State* l)
{
	int i;
	const char* str;

	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	str = LuaToString(l, 1);
	for (i = 0; i < MAX_TIPS; ++i) {
		if (Tips[i] && !strcmp(str, Tips[i])) {
			break;
		}
		if (Tips[i] == NULL) {
			Tips[i] = strdup(str);
			break;
		}
	}

	lua_pushstring(l, str);
	return 1;
}

/**
**		Set resource harvesting speed.
**
**		@param resource		Name of resource.
**		@param speed		Speed factor of harvesting resource.
*/
local int CclSetSpeedResourcesHarvest(lua_State* l)
{
	int i;
	const char* resource;

	if (lua_gettop(l) != 2) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	resource = LuaToString(l, 1);
	for (i = 0; i < MaxCosts; ++i) {
		if (!strcmp(resource, DefaultResourceNames[i])) {
			SpeedResourcesHarvest[i] = LuaToNumber(l, 2);
			return 0;
		}
	}
	lua_pushfstring(l, "Resource not found: %s", resource);
	lua_error(l);

	return 0;
}

/**
**		Set resource returning speed.
**
**		@param resource		Name of resource.
**		@param speed		Speed factor of returning resource.
*/
local int CclSetSpeedResourcesReturn(lua_State* l)
{
	int i;
	const char* resource;

	if (lua_gettop(l) != 2) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	resource = LuaToString(l, 1);
	for (i = 0; i < MaxCosts; ++i) {
		if (!strcmp(resource, DefaultResourceNames[i])) {
			SpeedResourcesReturn[i] = LuaToNumber(l, 2);
			return 0;
		}
	}
	lua_pushfstring(l, "Resource not found: %s", resource);
	lua_error(l);

	return 0;
}

/**
**		For debug increase building speed.
*/
local int CclSetSpeedBuild(lua_State* l)
{
	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	SpeedBuild = LuaToNumber(l, 1);

	lua_pushnumber(l, SpeedBuild);
	return 1;
}

/**
**		For debug increase training speed.
*/
local int CclSetSpeedTrain(lua_State* l)
{
	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	SpeedTrain = LuaToNumber(l, 1);

	lua_pushnumber(l, SpeedTrain);
	return 1;
}

/**
**		For debug increase upgrading speed.
*/
local int CclSetSpeedUpgrade(lua_State* l)
{
	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	SpeedUpgrade = LuaToNumber(l, 1);

	lua_pushnumber(l, SpeedUpgrade);
	return 1;
}

/**
**		For debug increase researching speed.
*/
local int CclSetSpeedResearch(lua_State* l)
{
	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	SpeedResearch = LuaToNumber(l, 1);

	lua_pushnumber(l, SpeedResearch);
	return 1;
}

/**
**		For debug increase all speeds.
*/
local int CclSetSpeeds(lua_State* l)
{
	int i;
	lua_Number s;

	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	s = LuaToNumber(l, 1);
	for (i = 0; i < MaxCosts; ++i) {
		SpeedResourcesHarvest[i] = s;
		SpeedResourcesReturn[i] = s;
	}
	SpeedBuild = SpeedTrain = SpeedUpgrade = SpeedResearch = s;

	lua_pushnumber(l, s);
	return 1;
}

/**
**		Define default resources for a new player.
*/
local int CclDefineDefaultResources(lua_State* l)
{
	int i;
	int args;

	args = lua_gettop(l);
	for (i = 0; i < MaxCosts && i < args; ++i) {
		DefaultResources[i] = LuaToNumber(l, i + 1);
	}
	return 0;
}

/**
**		Define default resources for a new player with low resources.
*/
local int CclDefineDefaultResourcesLow(lua_State* l)
{
	int i;
	int args;

	args = lua_gettop(l);
	for (i = 0; i < MaxCosts && i < args; ++i) {
		DefaultResourcesLow[i] = LuaToNumber(l, i + 1);
	}
	return 0;
}

/**
**		Define default resources for a new player with mid resources.
*/
local int CclDefineDefaultResourcesMedium(lua_State* l)
{
	int i;
	int args;

	args = lua_gettop(l);
	for (i = 0; i < MaxCosts && i < args; ++i) {
		DefaultResourcesMedium[i] = LuaToNumber(l, i + 1);
	}
	return 0;
}

/**
**		Define default resources for a new player with high resources.
*/
local int CclDefineDefaultResourcesHigh(lua_State* l)
{
	int i;
	int args;

	args = lua_gettop(l);
	for (i = 0; i < MaxCosts && i < args; ++i) {
		DefaultResourcesHigh[i] = LuaToNumber(l, i + 1);
	}
	return 0;
}

/**
**		Define default incomes for a new player.
*/
local int CclDefineDefaultIncomes(lua_State* l)
{
	int i;
	int args;

	args = lua_gettop(l);
	for (i = 0; i < MaxCosts && i < args; ++i) {
		DefaultIncomes[i] = LuaToNumber(l, i + 1);
	}
	return 0;
}

/**
**		Define default action for the resources.
*/
local int CclDefineDefaultActions(lua_State* l)
{
	int i;
	int args;

	for (i = 0; i < MaxCosts; ++i) {
		free(DefaultActions[i]);
		DefaultActions[i] = NULL;
	}
	args = lua_gettop(l);
	for (i = 0; i < MaxCosts && i < args; ++i) {
		DefaultActions[i] = strdup(LuaToString(l, i + 1));
	}
	return 0;
}

/**
**		Define default names for the resources.
*/
local int CclDefineDefaultResourceNames(lua_State* l)
{
	int i;
	int args;

	for (i = 0; i < MaxCosts; ++i) {
		free(DefaultResourceNames[i]);
		DefaultResourceNames[i] = NULL;
	}
	args = lua_gettop(l);
	for (i = 0; i < MaxCosts && i < args; ++i) {
		DefaultResourceNames[i] = strdup(LuaToString(l, i + 1));
	}
	return 0;
}

/**
**		Define default names for the resources.
*/
local int CclDefineDefaultResourceAmounts(lua_State* l)
{
	int i;
	int j;
	const char* value;
	int args;

	args = lua_gettop(l);
	if (args & 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	for (j = 0; j < args; ++j) {
		value = LuaToString(l, j + 1);
		for (i = 0; i < MaxCosts; ++i) {
			if (!strcmp(value, DefaultResourceNames[i])) {
				++j;
				DefaultResourceAmounts[i] = LuaToNumber(l, j + 1);
				break;
			}
		}
		if (i == MaxCosts) {
			lua_pushfstring(l, "Resource not found: %s", value);
			lua_error(l);
		}
	}
	return 0;
}

/**
**		Debug unit slots.
*/
local int CclUnits(lua_State* l)
{
	Unit** slot;
	int freeslots;
	int destroyed;
	int nullrefs;
	int i;
	static char buf[80];

	if (lua_gettop(l) != 0) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	i = 0;
	slot = UnitSlotFree;
	while (slot) {						// count the free slots
		++i;
		slot = (void*)*slot;
	}
	freeslots = i;

	//
	//		Look how many slots are used
	//
	destroyed = nullrefs = 0;
	for (slot = UnitSlots; slot < UnitSlots + MAX_UNIT_SLOTS; ++slot) {
		if (*slot && (*slot < (Unit*)UnitSlots ||
				*slot > (Unit*)(UnitSlots + MAX_UNIT_SLOTS))) {
			if ((*slot)->Destroyed) {
				++destroyed;
			} else if (!(*slot)->Refs) {
				++nullrefs;
			}
		}
	}

	sprintf(buf, "%d free, %d(%d) used, %d, destroyed, %d null",
		freeslots, MAX_UNIT_SLOTS - 1 - freeslots, NumUnits, destroyed, nullrefs);
	SetStatusLine(buf);
	fprintf(stderr, "%d free, %d(%d) used, %d destroyed, %d null\n",
		freeslots, MAX_UNIT_SLOTS - 1 - freeslots, NumUnits, destroyed, nullrefs);

	lua_pushnumber(l, destroyed);
	return 1;
}

/**
**		Compiled with sound.
*/
local int CclWithSound(lua_State* l)
{
#ifdef WITH_SOUND
	lua_pushboolean(l, 1);
#else
	lua_pushboolean(l, 0);
#endif
	return 1;
}

/**
**		Get Stratagus home path.
*/
local int CclGetStratagusHomePath(lua_State* l)
{
	const char* cp;
	char* buf;

	cp = getenv("HOME");
	buf = alloca(strlen(cp) + strlen(GameName) + sizeof(STRATAGUS_HOME_PATH) + 3);
	strcpy(buf, cp);
	strcat(buf, "/");
	strcat(buf, STRATAGUS_HOME_PATH);
	strcat(buf, "/");
	strcat(buf, GameName);

	lua_pushstring(l, buf);
	return 1;
}

/**
**		Get Stratagus library path.
*/
local int CclGetStratagusLibraryPath(lua_State* l)
{
	lua_pushstring(l, STRATAGUS_LIB_PATH);
	return 1;
}

/*............................................................................
..		Tables
............................................................................*/

/**
**		Load a pud. (Try in library path first)
**
**		@param file		filename of pud.
**
**		@return				FIXME: Nothing.
*/
local int CclLoadPud(lua_State* l)
{
	const char* name;
	char buffer[1024];

	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	name = LuaToString(l, 1);
	LoadPud(LibraryFileName(name, buffer), &TheMap);

	// FIXME: LoadPud should return an error
	return 0;
}

/**
**		Load a map. (Try in library path first)
**
**		@param file		filename of map.
**
**		@return				FIXME: Nothing.
*/
local int CclLoadMap(lua_State* l)
{
	const char* name;
	char buffer[1024];

	if (lua_gettop(l) != 1) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	}
	name = LuaToString(l, 1);
	if (strcasestr(name, ".pud")) {
		LoadPud(LibraryFileName(name, buffer), &TheMap);
	}

	// FIXME: LoadPud should return an error
	return 0;
}

/*............................................................................
..		Commands
............................................................................*/

/**
**		Send command to ccl.
**
**		@param command		Zero terminated command string.
*/
global int CclCommand(const char* command)
{
	int status;

	if (!(status = luaL_loadbuffer(Lua, command, strlen(command), command))) {
		LuaCall(0, 1);
	} else {
		report(status);
	}
	return status;
}

/*............................................................................
..		Setup
............................................................................*/

/**
**		Initialize ccl and load the config file(s).
*/
global void InitCcl(void)
{
	Lua = lua_open();
	luaopen_base(Lua);
	luaopen_table(Lua);
	luaopen_string(Lua);
	luaopen_math(Lua);
	luaopen_debug(Lua);
	lua_settop(Lua, 0);			// discard any results

	lua_register(Lua, "LibraryPath", CclStratagusLibraryPath);
	lua_register(Lua, "GameCycle", CclGameCycle);
	lua_register(Lua, "SetGameName", CclSetGameName);
	lua_register(Lua, "SetGameCycle", CclSetGameCycle);
	lua_register(Lua, "SetGamePaused", CclSetGamePaused);
	lua_register(Lua, "SetVideoSyncSpeed", CclSetVideoSyncSpeed);
	lua_register(Lua, "SetLocalPlayerName", CclSetLocalPlayerName);
	lua_register(Lua, "SetGodMode", CclSetGodMode);

	lua_register(Lua, "SetShowTips", CclSetShowTips);
	lua_register(Lua, "SetCurrentTip", CclSetCurrentTip);
	lua_register(Lua, "AddTip", CclAddTip);

	lua_register(Lua, "SetSpeedResourcesHarvest", CclSetSpeedResourcesHarvest);
	lua_register(Lua, "SetSpeedResourcesReturn", CclSetSpeedResourcesReturn);
	lua_register(Lua, "SetSpeedBuild", CclSetSpeedBuild);
	lua_register(Lua, "SetSpeedTrain", CclSetSpeedTrain);
	lua_register(Lua, "SetSpeedUpgrade", CclSetSpeedUpgrade);
	lua_register(Lua, "SetSpeedResearch", CclSetSpeedResearch);
	lua_register(Lua, "SetSpeeds", CclSetSpeeds);

	lua_register(Lua, "DefineDefaultResources", CclDefineDefaultResources);
	lua_register(Lua, "DefineDefaultResourcesLow", CclDefineDefaultResourcesLow);
	lua_register(Lua, "DefineDefaultResourcesMedium", CclDefineDefaultResourcesMedium);
	lua_register(Lua, "DefineDefaultResourcesHigh", CclDefineDefaultResourcesHigh);
	lua_register(Lua, "DefineDefaultIncomes", CclDefineDefaultIncomes);
	lua_register(Lua, "DefineDefaultActions", CclDefineDefaultActions);
	lua_register(Lua, "DefineDefaultResourceNames", CclDefineDefaultResourceNames);
	lua_register(Lua, "DefineDefaultResourceAmounts", CclDefineDefaultResourceAmounts);

	lua_register(Lua, "Load", CclLoad);

	NetworkCclRegister();
	IconCclRegister();
	MissileCclRegister();
	PlayerCclRegister();
	TilesetCclRegister();
	MapCclRegister();
	PathfinderCclRegister();
	ConstructionCclRegister();
	DecorationCclRegister();
	UnitTypeCclRegister();
	UpgradesCclRegister();
	DependenciesCclRegister();
	SelectionCclRegister();
	GroupCclRegister();
	UnitCclRegister();
	SoundCclRegister();
	FontsCclRegister();
	UserInterfaceCclRegister();
	AiCclRegister();
	CampaignCclRegister();
	TriggerCclRegister();
	CreditsCclRegister();
	ObjectivesCclRegister();
	SpellCclRegister();

	EditorCclRegister();

	lua_register(Lua, "LoadPud", CclLoadPud);
	lua_register(Lua, "LoadMap", CclLoadMap);

	lua_register(Lua, "Units", CclUnits);

	lua_register(Lua, "WithSound", CclWithSound);
	lua_register(Lua, "GetStratagusHomePath", CclGetStratagusHomePath);
	lua_register(Lua, "GetStratagusLibraryPath",
		CclGetStratagusLibraryPath);
}

/**
**		Load user preferences
*/
local void LoadPreferences1(void)
{
	FILE* fd;
	char buf[1024];

#ifdef USE_WIN32
	strcpy(buf, "preferences1.lua");
#else
	sprintf(buf, "%s/%s/preferences1.lua", getenv("HOME"), STRATAGUS_HOME_PATH);
#endif

	fd = fopen(buf, "r");
	if (fd) {
		fclose(fd);
		LuaLoadFile(buf);
	}
}

/**
**		Load user preferences
*/
local void LoadPreferences2(void)
{
	FILE* fd;
	char buf[1024];

#ifdef USE_WIN32
	sprintf(buf, "%s/preferences2.lua", GameName);
#else
	sprintf(buf, "%s/%s/%s/preferences2.lua", getenv("HOME"),
		STRATAGUS_HOME_PATH, GameName);
#endif

	fd = fopen(buf, "r");
	if (fd) {
		fclose(fd);
		LuaLoadFile(buf);
	}
}

/**
**		Save user preferences
*/
global void SavePreferences(void)
{
	FILE* fd;
	char buf[1024];
	int i;

	//
	//			preferences1.ccl
	//			This file is loaded before stratagus.ccl
	//

#ifdef USE_WIN32
	strcpy(buf, "preferences1.lua");
#else
	sprintf(buf, "%s/%s", getenv("HOME"), STRATAGUS_HOME_PATH);
	mkdir(buf, 0777);
	strcat(buf, "/preferences1.lua");
#endif

	fd = fopen(buf, "w");
	if (!fd) {
		return;
	}

	fprintf(fd, "--- -----------------------------------------\n");
	fprintf(fd, "--- $Id$\n");

	fprintf(fd, "SetVideoResolution(%d, %d)\n", VideoWidth, VideoHeight);
	fprintf(fd, "SetGroupKeys(\"");

	i = 0;
	while (UiGroupKeys[i]) {
		if (UiGroupKeys[i] != '"') {
			fprintf(fd, "%c", UiGroupKeys[i]);
		} else {
			fprintf(fd, "\\\"");
		}
		++i;
	}
	fprintf(fd, "\")\n");

	fclose(fd);

	//
	//			preferences2.ccl
	//			This file is loaded after stratagus.ccl
	//

#ifdef USE_WIN32
	sprintf(buf, "%s/preferences2.lua", GameName);
#else
	sprintf(buf, "%s/%s/%s/preferences2.lua", getenv("HOME"),
		STRATAGUS_HOME_PATH, GameName);
#endif

	fd = fopen(buf, "w");
	if (!fd) {
		return;
	}

	fprintf(fd, "--- -----------------------------------------\n");
	fprintf(fd, "--- $Id$\n");

	// Global options
	if (OriginalFogOfWar) {
		fprintf(fd, "OriginalFogOfWar()\n");
	} else {
		fprintf(fd, "AlphaFogOfWar()\n");
	}
	fprintf(fd, "SetVideoFullScreen(%s)\n", VideoFullScreen ? "true" : "false");
#if 0
	// FIXME: Uncomment when this is configurable in the menus
	fprintf(fd, "SetContrast(%d)\n", TheUI.Contrast);
	fprintf(fd, "SetBrightness(%d)\n", TheUI.Brightness);
	fprintf(fd, "SetSaturation(%d)\n", TheUI.Saturation);
#endif
	fprintf(fd, "SetLocalPlayerName(\"%s\")\n", LocalPlayerName);

	// Game options
	fprintf(fd, "SetShowTips(%s)\n", ShowTips ? "true" : "false");
	fprintf(fd, "SetCurrentTip(%d)\n", CurrentTip);

	fprintf(fd, "SetFogOfWar(%s)\n", !TheMap.NoFogOfWar ? "true" : "false");
	fprintf(fd, "SetShowCommandKey(%s)\n", ShowCommandKey ? "true" : "false");

	// Speeds
	fprintf(fd, "SetVideoSyncSpeed(%d)\n", VideoSyncSpeed);
	fprintf(fd, "SetMouseScrollSpeed(%d)\n", SpeedMouseScroll);
	fprintf(fd, "SetKeyScrollSpeed(%d)\n", SpeedKeyScroll);

	// Sound options
	if (!SoundOff) {
		fprintf(fd, "SoundOn()\n");
	} else {
		fprintf(fd, "SoundOff()\n");
	}
#ifdef WITH_SOUND
	fprintf(fd, "SetSoundVolume(%d)\n", GlobalVolume);
	if (!MusicOff) {
		fprintf(fd, "MusicOn()\n");
	} else {
		fprintf(fd, "MusicOff()\n");
	}
	fprintf(fd, "SetMusicVolume(%d)\n", MusicVolume);
#ifdef USE_CDAUDIO
	buf[0] = '\0';
	switch (CDMode) {
		case CDModeAll:
			strcpy(buf, "all");
			break;
		case CDModeRandom:
			strcpy(buf, "random");
			break;
		case CDModeDefined:
			strcpy(buf, "defined");
			break;
		case CDModeStopped:
		case CDModeOff:
			strcpy(buf, "off");
			break;
		default:
			break;
	}
	if (buf[0]) {
		fprintf(fd, "SetCdMode(\"%s\")\n", buf);
	}
#endif
#endif

	fclose(fd);
}

/**
**		Load stratagus config file.
*/
global void LoadCcl(void)
{
	char* file;
	char buf[1024];

	//
	//		Load and evaluate configuration file
	//
	CclInConfigFile = 1;
	file = LibraryFileName(CclStartFile, buf);
	if (access(buf, R_OK)) {
		printf("Maybe you need to specify another gamepath with '-d /path/to/datadir'?\n");
		ExitFatal(-1);
	}

	ShowLoadProgress("Script %s\n", file);
	LoadPreferences1();
	LuaLoadFile(file);
	LoadPreferences2();
	CclInConfigFile = 0;
	CclGarbageCollect(0);				// Cleanup memory after load
}

/**
**		Save CCL Module.
**
**		@param file		Save file.
*/
global void SaveCcl(CLFile* file)
{
#if 0
	SCM list;
	extern SCM oblistvar;

	CLprintf(file, "\n;;; -----------------------------------------\n");
	CLprintf(file, ";;; MODULE: CCL $Id$\n\n");

	for (list = oblistvar; gh_list_p(list); list = gh_cdr(list)) {
		SCM sym;

		sym = gh_car(list);
		if (symbol_boundp(sym, NIL)) {
			SCM value;
			CLprintf(file, ";;(define %s\n", get_c_string(sym));
			value = symbol_value(sym, NIL);
			CLprintf(file, ";;");
			lprin1CL(value, file);
			CLprintf(file, "\n");
#ifdef DEBUG
		} else {
			CLprintf(file, ";;%s unbound\n", get_c_string(sym));
#endif
		}
	}
#endif
}

//@}
