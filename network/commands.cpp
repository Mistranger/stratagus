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
/**@name commands.c	-	Global command handler - network support. */
//
//	(c) Copyright 2000-2003 by Lutz Sammer, Andreas Arens, and Jimmy Salmon.
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

//----------------------------------------------------------------------------
//	Includes
//----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "stratagus.h"
#include "unit.h"
#include "map.h"
#include "actions.h"
#include "player.h"
#include "network.h"
#include "netconnect.h"
#include "campaign.h"			// for CurrentMapPath
#include "ccl.h"
#include "ccl_helpers.h"
#include "commands.h"
#include "interface.h"
#include "iocompat.h"
#include "settings.h"

//----------------------------------------------------------------------------
//	Declaration
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//	Structures
//----------------------------------------------------------------------------

/**
**	LogEntry typedef
*/
typedef struct _log_entry_ LogEntry;

/**
**	LogEntry structure.
*/
struct _log_entry_ {
#if defined(USE_GUILE) || defined(USE_SIOD)
    int		GameCycle;
#elif defined(USE_LUA)
    unsigned long   GameCycle;
#endif
    int		UnitNumber;
    char*	UnitIdent;
    char*	Action;
    int		Flush;
    int		PosX;
    int		PosY;
    int		DestUnitNumber;
    char*	Value;
    int		Num;
#if defined(USE_GUILE) || defined(USE_SIOD)
    int		SyncRandSeed;
#elif defined(USE_LUA)
    unsigned	SyncRandSeed;
#endif
    LogEntry*	Next;
};

/**
**	Multiplayer Player definition
*/
typedef struct _multiplayer_player_ {
    char*	Name;
    int		Race;
    int		Team;
    int 	Type;
} MPPlayer;

/**
**	Full replay structure (definition + logs)
*/
typedef struct _full_replay_ {
    char* 	Comment1;
    char* 	Comment2;
    char* 	Comment3;
    char*	Date;
    char*	Map;
    char*	MapPath;
#if defined(USE_GUILE) || defined(USE_SIOD)
    int		MapId;
#elif defined(USE_LUA)
    unsigned	MapId;
#endif

    int		Type;
    int		Race;
    int		LocalPlayer;
    MPPlayer	Players[PlayerMax];

    int 	Resource;
    int		NumUnits;
    int		TileSet;
    int		NoFow;
    int		RevealMap;
    int		GameType;
    int		Opponents;
    int		Engine[3];
    int		Network[3];
    LogEntry*	Commands;
} FullReplay;

//----------------------------------------------------------------------------
//	Constants
//----------------------------------------------------------------------------

#if defined(USE_GUILE) || defined(USE_SIOD)
/// Description of the LogEntry structure
static IOStructDef LogEntryStructDef = {
    "LogEntry",
    sizeof(LogEntry),
    -1,
    {
	{"`next", 		NULL, 		&((LogEntry*)0)->Next, 		NULL},
	{"game-cycle",		&IOInt,		&((LogEntry*)0)->GameCycle,	NULL},
	{"unit-number",		&IOInt,		&((LogEntry*)0)->UnitNumber,	NULL},
	{"unit-ident",		&IOString,	&((LogEntry*)0)->UnitIdent,	NULL},
	{"action",		&IOString,	&((LogEntry*)0)->Action,	NULL},
	{"flush",		&IOInt,		&((LogEntry*)0)->Flush,		NULL},
	{"posx",		&IOInt,		&((LogEntry*)0)->PosX,		NULL},
	{"posy",		&IOInt,		&((LogEntry*)0)->PosY,		NULL},
	{"dest-unit-number",	&IOInt,		&((LogEntry*)0)->DestUnitNumber,NULL},
	{"value",		&IOString,	&((LogEntry*)0)->Value,		NULL},
	{"Num",			&IOInt,		&((LogEntry*)0)->Num,		NULL},
	{"SyncRandSeed",	&IOInt,		&((LogEntry*)0)->SyncRandSeed,	NULL},
	{0, 0, 0, 0}
    }
};

static IOStructDef MPPlayerStructDef = {
    "MPPlayer",
    sizeof(MPPlayer),
    PlayerMax,
    {
	{"name",		&IOString,	&((MPPlayer*)0)->Name,	NULL},
	{"race",		&IOInt,		&((MPPlayer*)0)->Race,	NULL},
	{"team",		&IOInt,		&((MPPlayer*)0)->Team,	NULL},
	{"type",		&IOInt,		&((MPPlayer*)0)->Type,	NULL},
	{0, 0, 0, 0}
    }
};

static IOStructDef FullReplayStructDef = {
    "FullReplay",
    sizeof(FullReplay),
    -1,
    {
	{"comment-1",		&IOString,	&((FullReplay*)0)->Comment1,	NULL},
	{"comment-2",		&IOString,	&((FullReplay*)0)->Comment2,	NULL},
	{"comment-3",		&IOString,	&((FullReplay*)0)->Comment3,	NULL},
	{"date",		&IOString,	&((FullReplay*)0)->Date,	NULL},
	{"map",			&IOString,	&((FullReplay*)0)->Map,	NULL},
	{"mappath",		&IOString,	&((FullReplay*)0)->MapPath,	NULL},
	{"mapid",		&IOInt,		&((FullReplay*)0)->MapId,	NULL},
	{"type",		&IOInt,		&((FullReplay*)0)->Type,	NULL},
	{"race",		&IOInt,		&((FullReplay*)0)->Race,	NULL},
	{"local-player",	&IOInt,		&((FullReplay*)0)->LocalPlayer,	NULL},
	{"players",		&IOStructArray,	&((FullReplay*)0)->Players,	(void*)&MPPlayerStructDef},
	{"resource",		&IOInt,		&((FullReplay*)0)->Resource,	NULL},
	{"num-units",		&IOInt,		&((FullReplay*)0)->NumUnits,	NULL},
	{"tileset",		&IOInt,		&((FullReplay*)0)->TileSet,	NULL},
	{"no-fow",		&IOInt,		&((FullReplay*)0)->NoFow,	NULL},
	{"reveal-map",		&IOInt,		&((FullReplay*)0)->RevealMap,	NULL},
	{"game-type",		&IOInt,		&((FullReplay*)0)->GameType,	NULL},
	{"Opponents",		&IOInt,		&((FullReplay*)0)->Opponents,	NULL},
	{"engine",		&IOIntArray,	&((FullReplay*)0)->Engine,	(void*)3},
	{"network",		&IOIntArray,	&((FullReplay*)0)->Network,	(void*)3},
	{"commands",		&IOLinkedList,	&((FullReplay*)0)->Commands,	(void*)&LogEntryStructDef},
	{0, 0, 0, 0}
    }
};
#endif

//----------------------------------------------------------------------------
//	Variables
//----------------------------------------------------------------------------

global int CommandLogDisabled;		/// True if command log is off
global ReplayType ReplayGameType;	/// Replay game type
local int DisabledLog;			/// Disabled log for replay
local int DisabledShowTips;		/// Disabled show tips
local CLFile* LogFile;			/// Replay log file
local unsigned long NextLogCycle;	/// Next log cycle number
local int InitReplay;			/// Initialize replay
local FullReplay* CurrentReplay;
local LogEntry* ReplayStep;

local void AppendLog(LogEntry* log, CLFile* dest);

//----------------------------------------------------------------------------
//	Log commands
//----------------------------------------------------------------------------

/**@name log */
//@{

/**
**	Allocate & fill a new FullReplay structure, from GameSettings.
**
**	@return	A new FullReplay structure
*/
local FullReplay* StartReplay(void)
{
    FullReplay* replay;
    char* s;
    time_t now;
    char* s1;

    replay = calloc(1, sizeof(FullReplay));

    time(&now);
    s = ctime(&now);
    if ((s1 = strchr(s, '\n'))) {
	*s1 = '\0';
    }

    replay->Comment1 = strdup("Generated by Stratagus Version " VERSION "");
    replay->Comment2 = strdup("Visit http://Stratagus.Org for more information");
    replay->Comment3 = strdup("$Id$");

    if (GameSettings.NetGameType == SettingsSinglePlayerGame) {
	replay->Type = ReplaySinglePlayer;
	replay->Race = GameSettings.Presets[0].Race;
    } else {
	int i;

	replay->Type = ReplayMultiPlayer;

	for (i = 0; i < PlayerMax; ++i) {
	    replay->Players[i].Name = strdup(Players[i].Name);
	    replay->Players[i].Race = GameSettings.Presets[i].Race;
	    replay->Players[i].Team = GameSettings.Presets[i].Team;
	    replay->Players[i].Type = GameSettings.Presets[i].Type;
	}

	replay->LocalPlayer = ThisPlayer->Player;
    }

    replay->Date = strdup(s);
    replay->Map = strdup(TheMap.Description);
    replay->MapId = (signed int)TheMap.Info->MapUID;
    replay->MapPath = strdup(CurrentMapPath);
    replay->Resource = GameSettings.Resources;
    replay->NumUnits = GameSettings.NumUnits;
    replay->TileSet = GameSettings.Terrain;
    replay->NoFow = GameSettings.NoFogOfWar;
    replay->GameType = GameSettings.GameType;
    replay->Opponents = GameSettings.Opponents;

    replay->Engine[0] = StratagusMajorVersion;
    replay->Engine[1] = StratagusMinorVersion;
    replay->Engine[2] = StratagusPatchLevel;

    replay->Network[0] = NetworkProtocolMajorVersion;
    replay->Network[1] = NetworkProtocolMinorVersion;
    replay->Network[2] = NetworkProtocolPatchLevel;
    return replay;
}

/**
**	FIXME: docu
*/
local void ApplyReplaySettings(void)
{
    int i;

    if (CurrentReplay->Type == ReplayMultiPlayer) {
	ExitNetwork1();
	NetPlayers = 2;
	GameSettings.NetGameType = SettingsMultiPlayerGame;

	ReplayGameType = ReplayMultiPlayer;
	for (i = 0; i < PlayerMax; ++i) {
	    GameSettings.Presets[i].Race = CurrentReplay->Players[i].Race;
	    GameSettings.Presets[i].Team = CurrentReplay->Players[i].Team;
	    GameSettings.Presets[i].Type = CurrentReplay->Players[i].Type;
	}

	NetLocalPlayerNumber = CurrentReplay->LocalPlayer;
    } else {
	GameSettings.NetGameType = SettingsSinglePlayerGame;
	GameSettings.Presets[0].Race = CurrentReplay->Race;
	ReplayGameType = ReplaySinglePlayer;
    }

    strcpy(CurrentMapPath, CurrentReplay->MapPath);
    GameSettings.Resources = CurrentReplay->Resource;
    GameSettings.NumUnits = CurrentReplay->NumUnits;
    GameSettings.Terrain = CurrentReplay->TileSet;
    TheMap.NoFogOfWar = GameSettings.NoFogOfWar = CurrentReplay->NoFow;
    FlagRevealMap = GameSettings.RevealMap = CurrentReplay->RevealMap;
    GameSettings.Opponents = CurrentReplay->Opponents;

    // FIXME : check engine version
    // FIXME : FIXME: check network version
    // FIXME : check mapid
}

/**
**	FIXME: docu
*/
local void DeleteReplay(FullReplay* replay)
{
    LogEntry* log;
    LogEntry* next;
    int i;

#define cond_free(x) { if (x) { free(x); } }

    cond_free(replay->Comment1);
    cond_free(replay->Comment2);
    cond_free(replay->Comment3);
    cond_free(replay->Date);
    cond_free(replay->Map);
    cond_free(replay->MapPath);

    for (i = 0; i < PlayerMax; ++i) {
	cond_free(replay->Players[i].Name);
    }

    log = replay->Commands;
    while (log) {
	cond_free(log->UnitIdent);
	cond_free(log->Action);
	cond_free(log->Value);
	next = log->Next;
	free(log);
	log = next;
    }

#undef cond_free

    free(replay);
}

/**
**	Output the FullReplay list to dest file
**
**	@param dest	The file to output to
*/
local void SaveFullLog(CLFile* dest)
{
#if defined(USE_GUILE) || defined(USE_SIOD)
    // FIXME : IOStartSaving(dest);
    IOLoadingMode = 0;
    IOOutFile = dest;
    IOTabLevel = 2;

    CLprintf(dest, "(replay-log (quote\n");
    IOStructPtr(SCM_UNSPECIFIED, (void*)&CurrentReplay, (void*)&FullReplayStructDef);
    CLprintf(dest, "))\n");
    // FIXME : IODone();
#elif defined(USE_LUA)
    LogEntry* log;
    int i;

    CLprintf(dest, "ReplayLog( {\n");
    CLprintf(dest, "  Comment1 = \"%s\",\n", CurrentReplay->Comment1);
    CLprintf(dest, "  Comment2 = \"%s\",\n", CurrentReplay->Comment2);
    CLprintf(dest, "  Comment3 = \"%s\",\n", CurrentReplay->Comment3);
    CLprintf(dest, "  Date = \"%s\",\n", CurrentReplay->Date);
    CLprintf(dest, "  Map = \"%s\",\n", CurrentReplay->Map);
    CLprintf(dest, "  MapPath = \"%s\",\n", CurrentReplay->MapPath);
    CLprintf(dest, "  MapId = %u,\n", CurrentReplay->MapId);
    CLprintf(dest, "  Type = %d,\n", CurrentReplay->Type);
    CLprintf(dest, "  Race = %d,\n", CurrentReplay->Race);
    CLprintf(dest, "  LocalPlayer = %d,\n", CurrentReplay->LocalPlayer);
    CLprintf(dest, "  Players = {\n");
    for (i = 0; i < PlayerMax; ++i) {
	if (CurrentReplay->Players[i].Name) {
	    CLprintf(dest, "    { Name = \"%s\",\n", CurrentReplay->Players[i].Name);
	} else {
	    CLprintf(dest, "    {\n");
	}
	CLprintf(dest, "      Race = %d,\n", CurrentReplay->Players[i].Race);
	CLprintf(dest, "      Team = %d,\n", CurrentReplay->Players[i].Team);
	CLprintf(dest, "      Type = %d }%s", CurrentReplay->Players[i].Type,
	    i != PlayerMax - 1 ? ",\n" : "\n");
    }
    CLprintf(dest, "  },\n");
    CLprintf(dest, "  Resource = %d,\n", CurrentReplay->Resource);
    CLprintf(dest, "  NumUnits = %d,\n", CurrentReplay->NumUnits);
    CLprintf(dest, "  TileSet = %d,\n", CurrentReplay->TileSet);
    CLprintf(dest, "  NoFow = %d,\n", CurrentReplay->NoFow);
    CLprintf(dest, "  RevealMap = %d,\n", CurrentReplay->RevealMap);
    CLprintf(dest, "  GameType = %d,\n", CurrentReplay->GameType);
    CLprintf(dest, "  Opponents = %d,\n", CurrentReplay->Opponents);
    CLprintf(dest, "  Engine = { %d, %d, %d },\n",
	CurrentReplay->Engine[0], CurrentReplay->Engine[1], CurrentReplay->Engine[2]);
    CLprintf(dest, "  Network = { %d, %d, %d }\n",
	CurrentReplay->Network[0], CurrentReplay->Network[1], CurrentReplay->Network[2]);
    CLprintf(dest, "} )\n");
    log = CurrentReplay->Commands;
    while (log) {
	AppendLog(log, dest);
	log = log->Next;
    }
#endif
}

/**
**	Append the LogEntry structure at the end of currentLog, and to LogFile
**
**	@param dest	The file to output to
*/
local void AppendLog(LogEntry* log, CLFile* dest)
{
    LogEntry** last;

    // Append to linked list
    last = &CurrentReplay->Commands;
    while (*last) {
	last = &(*last)->Next;
    }

    *last = log;
    log->Next = 0;

    // Append to file
    if (!dest) {
	return;
    }

    // FIXME : IOStartSaving(dest);

#if defined(USE_GUILE) || defined(USE_SIOD)
    IOLoadingMode = 0;
    IOOutFile = dest;
    IOTabLevel = 2;
    CLprintf(dest, "(log (quote ");
    IOLinkedList(SCM_UNSPECIFIED, (void*)&log, (void*)&LogEntryStructDef);
    CLprintf(dest,"))\n");
#elif defined(USE_LUA)
    CLprintf(dest, "Log( { ");
    CLprintf(dest, "GameCycle = %lu, ", log->GameCycle);
    if (log->UnitNumber != -1) {
	CLprintf(dest, "UnitNumber = %d, ", log->UnitNumber);
    }
    if (log->UnitIdent) {
	CLprintf(dest, "UnitIdent = \"%s\", ", log->UnitIdent);
    }
    CLprintf(dest, "Action = \"%s\", ", log->Action);
    CLprintf(dest, "Flush = %d, ", log->Flush);
    if (log->PosX != -1 || log->PosY != -1) {
	CLprintf(dest, "PosX = %d, PosY = %d, ", log->PosX, log->PosY);
    }
    if (log->DestUnitNumber != -1) {
	CLprintf(dest, "DestUnitNumber = %d, ", log->DestUnitNumber);
    }
    if (log->Value) {
	CLprintf(dest, "Value = \"%s\", ", log->Value);
    }
    if (log->Num != -1) {
	CLprintf(dest, "Num = %d, ", log->Num);
    }
    CLprintf(dest, "SyncRandSeed = %u } )\n", log->SyncRandSeed);
#endif
    CLflush(dest);

    // FIXME : IODone();
}

/**
**	Log commands into file.
**
**	This could later be used to recover, crashed games.
**
**	@param action	Command name (move,attack,...).
**	@param unit	Unit that receive the command.
**	@param flush	Append command or flush old commands.
**	@param x	optional X map position.
**	@param y	optional y map position.
**	@param dest	optional destination unit.
**	@param value	optional command argument (unit-type,...).
**	@param num	optional number argument
*/
global void CommandLog(const char* action, const Unit* unit, int flush,
    int x, int y, const Unit* dest, const char* value, int num)
{
    LogEntry* log;

    if (CommandLogDisabled) {		// No log wanted
	return;
    }

    //
    //	Create and write header of log file. The player number is added
    //  to the save file name, to test more than one player on one computer.
    //
    if (!LogFile) {
	char buf[PATH_MAX];

#ifdef USE_WIN32
	strcpy(buf, GameName);
	mkdir(buf);
	strcat(buf, "/logs");
	mkdir(buf);
#else
	sprintf(buf, "%s/%s", getenv("HOME"), STRATAGUS_HOME_PATH);
	mkdir(buf, 0777);
	strcat(buf, "/");
	strcat(buf, GameName);
	mkdir(buf, 0777);
	strcat(buf, "/logs");
	mkdir(buf, 0777);
#endif

	sprintf(buf, "%s/log_of_stratagus_%d.log", buf, ThisPlayer->Player);
	LogFile = CLopen(buf, CL_OPEN_WRITE);
	if (!LogFile) {
	    // don't retry for each command
	    CommandLogDisabled = 0;
	    return;
	}

	if (CurrentReplay) {
	    SaveFullLog(LogFile);
	}
    }

    if (!CurrentReplay) {
	CurrentReplay = StartReplay();

	SaveFullLog(LogFile);
    }

    if (!action) {
	return;
    }

    log = (LogEntry*)malloc(sizeof(LogEntry));

    //
    //	Frame, unit, (type-ident only to be better readable).
    //
    log->GameCycle = GameCycle;

    log->UnitNumber = (unit ? UnitNumber(unit) : -1);
    log->UnitIdent = (unit ? strdup(unit->Type->Ident) : NULL);

    log->Action = strdup(action);
    log->Flush = flush;

    //
    //	Coordinates given.
    //
    log->PosX = x;
    log->PosY = y;

    //
    //	Destination given.
    //
    log->DestUnitNumber = (dest ? UnitNumber(dest) : -1);

    //
    //	Value given.
    //
    log->Value = (value ? strdup(value) : NULL);

    //
    //	Number given.
    //
    log->Num = num;

    log->SyncRandSeed = (signed)SyncRandSeed;

    // Append it to ReplayLog list
    AppendLog(log, LogFile);
}

/**
**	Parse log
*/
#if defined(USE_GUILE) || defined(USE_SIOD)
local SCM CclLog(SCM list)
{
    LogEntry* log;
    LogEntry** last;

    DebugCheck(!CurrentReplay);

    IOLoadingMode = 1;

    log = NULL;
    IOLinkedList(list, (void*)&log, (void*)&LogEntryStructDef);

    // Append to linked list
    last = &CurrentReplay->Commands;
    while (*last) {
	last = &(*last)->Next;
    }

    *last = log;

    return SCM_UNSPECIFIED;
}
#elif defined(USE_LUA)
local int CclLog(lua_State* l)
{
    LogEntry* log;
    LogEntry** last;
    const char* value;

    if (lua_gettop(l) != 1 || !lua_istable(l, 1)) {
	lua_pushstring(l, "incorrect argument");
	lua_error(l);
    }

    DebugCheck(!CurrentReplay);

    log = calloc(1, sizeof(LogEntry));
    log->UnitNumber = -1;
    log->PosX = -1;
    log->PosY = -1;
    log->DestUnitNumber = -1;
    log->Num = -1;

    lua_pushnil(l);
    while (lua_next(l, 1) != 0) {
	value = LuaToString(l, -2);
	if (!strcmp(value, "GameCycle")) {
	    log->GameCycle = LuaToNumber(l, -1);
	} else if (!strcmp(value, "UnitNumber")) {
	    log->UnitNumber = LuaToNumber(l, -1);
	} else if (!strcmp(value, "UnitIdent")) {
	    log->UnitIdent = strdup(LuaToString(l, -1));
	} else if (!strcmp(value, "Action")) {
	    log->Action = strdup(LuaToString(l, -1));
	} else if (!strcmp(value, "Flush")) {
	    log->Flush = LuaToNumber(l, -1);
	} else if (!strcmp(value, "PosX")) {
	    log->PosX = LuaToNumber(l, -1);
	} else if (!strcmp(value, "PosY")) {
	    log->PosY = LuaToNumber(l, -1);
	} else if (!strcmp(value, "DestUnitNumber")) {
	    log->DestUnitNumber = LuaToNumber(l, -1);
	} else if (!strcmp(value, "Value")) {
	    log->Value = strdup(LuaToString(l, -1));
	} else if (!strcmp(value, "Num")) {
	    log->Num = LuaToNumber(l, -1);
	} else if (!strcmp(value, "SyncRandSeed")) {
	    log->SyncRandSeed = LuaToNumber(l, -1);
	} else {
	    lua_pushfstring(l, "Unsupported key: %s", value);
	    lua_error(l);
	}
	lua_pop(l, 1);
    }

    // Append to linked list
    last = &CurrentReplay->Commands;
    while (*last) {
	last = &(*last)->Next;
    }

    *last = log;

    return 0;
}
#endif

/**
**	Parse replay-log
*/
#if defined(USE_GUILE) || defined(USE_SIOD)
local SCM CclReplayLog(SCM list)
{
    FullReplay* replay;

    DebugCheck(CurrentReplay != NULL);

    IOLoadingMode = 1;
    replay = 0;
    IOStructPtr(list, (void*)&replay, (void*)&FullReplayStructDef);

    CurrentReplay = replay;

    // Apply CurrentReplay settings.
    ApplyReplaySettings();

    return SCM_UNSPECIFIED;
}
#elif defined(USE_LUA)
local int CclReplayLog(lua_State* l)
{
    FullReplay* replay;
    const char* value;
    int j;

    if (lua_gettop(l) != 1 || !lua_istable(l, 1)) {
	lua_pushstring(l, "incorrect argument");
	lua_error(l);
    }

    DebugCheck(CurrentReplay != NULL);

    replay = calloc(1, sizeof(FullReplay));

    lua_pushnil(l);
    while (lua_next(l, 1) != 0) {
	value = LuaToString(l, -2);
	if (!strcmp(value, "Comment1")) {
	    replay->Comment1 = strdup(LuaToString(l, -1));
	} else if (!strcmp(value, "Comment2")) {
	    replay->Comment2 = strdup(LuaToString(l, -1));
	} else if (!strcmp(value, "Comment3")) {
	    replay->Comment3 = strdup(LuaToString(l, -1));
	} else if (!strcmp(value, "Date")) {
	    replay->Date = strdup(LuaToString(l, -1));
	} else if (!strcmp(value, "Map")) {
	    replay->Map = strdup(LuaToString(l, -1));
	} else if (!strcmp(value, "MapPath")) {
	    replay->MapPath = strdup(LuaToString(l, -1));
	} else if (!strcmp(value, "MapId")) {
	    replay->MapId = LuaToNumber(l, -1);
	} else if (!strcmp(value, "Type")) {
	    replay->Type = LuaToNumber(l, -1);
	} else if (!strcmp(value, "Race")) {
	    replay->Race = LuaToNumber(l, -1);
	} else if (!strcmp(value, "LocalPlayer")) {
	    replay->LocalPlayer = LuaToNumber(l, -1);
	} else if (!strcmp(value, "Players")) {
	    if (!lua_istable(l, -1) || luaL_getn(l, -1) != PlayerMax) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	    }
	    for (j = 0; j < PlayerMax; ++j) {
		int top;

		lua_rawgeti(l, -1, j + 1);
		if (!lua_istable(l, -1)) {
		    lua_pushstring(l, "incorrect argument");
		    lua_error(l);
		}
		top = lua_gettop(l);
		lua_pushnil(l);
		while (lua_next(l, top) != 0) {
		    value = LuaToString(l, -2);
		    if (!strcmp(value, "Name")) {
			replay->Players[j].Name = strdup(LuaToString(l, -1));
		    } else if (!strcmp(value, "Race")) {
			replay->Players[j].Race = LuaToNumber(l, -1);
		    } else if (!strcmp(value, "Team")) {
			replay->Players[j].Team = LuaToNumber(l, -1);
		    } else if (!strcmp(value, "Type")) {
			replay->Players[j].Type = LuaToNumber(l, -1);
		    } else {
			lua_pushfstring(l, "Unsupported key: %s", value);
			lua_error(l);
		    }
		    lua_pop(l, 1);
		}
		lua_pop(l, 1);
	    }
	} else if (!strcmp(value, "Resource")) {
	    replay->Resource = LuaToNumber(l, -1);
	} else if (!strcmp(value, "NumUnits")) {
	    replay->NumUnits = LuaToNumber(l, -1);
	} else if (!strcmp(value, "TileSet")) {
	    replay->TileSet = LuaToNumber(l, -1);
	} else if (!strcmp(value, "NoFow")) {
	    replay->NoFow = LuaToNumber(l, -1);
	} else if (!strcmp(value, "RevealMap")) {
	    replay->RevealMap = LuaToNumber(l, -1);
	} else if (!strcmp(value, "GameType")) {
	    replay->GameType = LuaToNumber(l, -1);
	} else if (!strcmp(value, "Opponents")) {
	    replay->Opponents = LuaToNumber(l, -1);
	} else if (!strcmp(value, "Engine")) {
	    if (!lua_istable(l, -1) || luaL_getn(l, -1) != 3) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	    }
	    lua_rawgeti(l, -1, 1);
	    replay->Engine[0] = LuaToNumber(l, -1);
	    lua_pop(l, 1);
	    lua_rawgeti(l, -1, 2);
	    replay->Engine[1] = LuaToNumber(l, -1);
	    lua_pop(l, 1);
	    lua_rawgeti(l, -1, 3);
	    replay->Engine[2] = LuaToNumber(l, -1);
	    lua_pop(l, 1);
	} else if (!strcmp(value, "Network")) {
	    if (!lua_istable(l, -1) || luaL_getn(l, -1) != 3) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	    }
	    lua_rawgeti(l, -1, 1);
	    replay->Network[0] = LuaToNumber(l, -1);
	    lua_pop(l, 1);
	    lua_rawgeti(l, -1, 2);
	    replay->Network[1] = LuaToNumber(l, -1);
	    lua_pop(l, 1);
	    lua_rawgeti(l, -1, 3);
	    replay->Network[2] = LuaToNumber(l, -1);
	    lua_pop(l, 1);
	} else {
	    lua_pushfstring(l, "Unsupported key: %s", value);
	    lua_error(l);
	}
	lua_pop(l, 1);
    }

    CurrentReplay = replay;

    // Apply CurrentReplay settings.
    ApplyReplaySettings();

    return 0;
}
#endif

/**
**	Save generated replay
**
**	@param file	file to save to.
*/
global void SaveReplayList(CLFile* file)
{
    SaveFullLog(file);
}

/**
**	Load a log file to replay a game
**
**	@param name	name of file to load.
*/
global int LoadReplay(char* name)
{
    CleanReplayLog();
    ReplayGameType = ReplaySinglePlayer;

#if defined(USE_GUILE) || defined(USE_SIOD)
    vload(name, 0, 1);
#elif defined(USE_LUA)
    LuaLoadFile(name);
#endif

    NextLogCycle = ~0UL;
    if (!CommandLogDisabled) {
	CommandLogDisabled = 1;
	DisabledLog = 1;
    }
    if (ShowTips) {
	ShowTips = 0;
	DisabledShowTips = 1;
    } else {
	DisabledShowTips = 0;
    }
    GameObserve = 1;
    InitReplay = 1;

    return 0;
}

/**
**	End logging
*/
global void EndReplayLog(void)
{
    if (LogFile) {
	CLclose(LogFile);
	LogFile = NULL;
    }
    if (CurrentReplay) {
	DeleteReplay(CurrentReplay);
	CurrentReplay = NULL;
    }
    ReplayStep = NULL;
}

/**
**	Clean replay log
*/
global void CleanReplayLog(void)
{
    if (CurrentReplay) {
	DeleteReplay(CurrentReplay);
	CurrentReplay = 0;
    }
    ReplayStep = NULL;

//    if (DisabledLog) {
	CommandLogDisabled = 0;
	DisabledLog = 0;
//    }
    if (DisabledShowTips) {
	ShowTips = 1;
	DisabledShowTips = 0;
    }
    GameObserve = 0;
    NetPlayers = 0;
    ReplayGameType = ReplayNone;
}

/**
**	Do next replay
*/
local void DoNextReplay(void)
{
    int unit;
    const char* action;
    int flags;
    int posx;
    int posy;
    const char* val;
    int num;
    Unit* dunit;

    DebugCheck(ReplayStep == 0);

    NextLogCycle = ReplayStep->GameCycle;

    if (NextLogCycle != GameCycle) {
	return;
    }

    unit = ReplayStep->UnitNumber;
    action = ReplayStep->Action;
    flags = ReplayStep->Flush;
    posx = ReplayStep->PosX;
    posy = ReplayStep->PosY;
    dunit = (ReplayStep->DestUnitNumber != -1 ? UnitSlots[ReplayStep->DestUnitNumber] : NoUnitP);
    val = ReplayStep->Value;
    num = ReplayStep->Num;

    DebugCheck(unit != -1 && strcmp(ReplayStep->UnitIdent, UnitSlots[unit]->Type->Ident));

    if (((signed)SyncRandSeed) != ReplayStep->SyncRandSeed) {
#ifdef DEBUG
	if (!ReplayStep->SyncRandSeed) {
	    // Replay without the 'sync info
	    NotifyPlayer(ThisPlayer, NotifyYellow, 0, 0, "No sync info for this replay !");
	} else {
	    NotifyPlayer(ThisPlayer, NotifyYellow, 0, 0, "Replay got out of sync !");
	    ReplayStep = 0;
	    NextLogCycle = ~0UL;
	    return;
	}
#else
    	NotifyPlayer(ThisPlayer, NotifyYellow, 0, 0, "Replay got out of sync !");
	ReplayStep = 0;
	NextLogCycle = ~0UL;
	return;
#endif
    }

    if (!strcmp(action, "stop")) {
	SendCommandStopUnit(UnitSlots[unit]);
    } else if (!strcmp(action, "stand-ground")) {
	SendCommandStandGround(UnitSlots[unit], flags);
    } else if (!strcmp(action, "follow")) {
	SendCommandFollow(UnitSlots[unit], dunit, flags);
    } else if (!strcmp(action, "move")) {
	SendCommandMove(UnitSlots[unit], posx, posy, flags);
    } else if (!strcmp(action, "repair")) {
	SendCommandRepair(UnitSlots[unit], posx, posy, dunit, flags);
    } else if (!strcmp(action, "attack")) {
	SendCommandAttack(UnitSlots[unit], posx, posy, dunit, flags);
    } else if (!strcmp(action, "attack-ground")) {
	SendCommandAttackGround(UnitSlots[unit], posx, posy, flags);
    } else if (!strcmp(action, "patrol")) {
	SendCommandPatrol(UnitSlots[unit], posx, posy, flags);
    } else if (!strcmp(action, "board")) {
	SendCommandBoard(UnitSlots[unit], posx, posy, dunit, flags);
    } else if (!strcmp(action, "unload")) {
	SendCommandUnload(UnitSlots[unit], posx, posy, dunit, flags);
    } else if (!strcmp(action, "build")) {
	SendCommandBuildBuilding(UnitSlots[unit], posx, posy, UnitTypeByIdent(val), flags);
    } else if (!strcmp(action, "dismiss")) {
	SendCommandDismiss(UnitSlots[unit]);
    } else if (!strcmp(action, "resource-loc")) {
	SendCommandResourceLoc(UnitSlots[unit], posx, posy, flags);
    } else if (!strcmp(action, "resource")) {
	SendCommandResource(UnitSlots[unit], dunit, flags);
    } else if (!strcmp(action, "return")) {
	SendCommandReturnGoods(UnitSlots[unit], dunit, flags);
    } else if (!strcmp(action, "train")) {
	SendCommandTrainUnit(UnitSlots[unit], UnitTypeByIdent(val), flags);
    } else if (!strcmp(action, "cancel-train")) {
	SendCommandCancelTraining(UnitSlots[unit], num, val ? UnitTypeByIdent(val) : NULL);
    } else if (!strcmp(action, "upgrade-to")) {
	SendCommandUpgradeTo(UnitSlots[unit], UnitTypeByIdent(val), flags);
    } else if (!strcmp(action, "cancel-upgrade-to")) {
	SendCommandCancelUpgradeTo(UnitSlots[unit]);
    } else if (!strcmp(action, "research")) {
	SendCommandResearch(UnitSlots[unit],UpgradeByIdent(val), flags);
    } else if (!strcmp(action, "cancel-research")) {
	SendCommandCancelResearch(UnitSlots[unit]);
    } else if (!strcmp(action, "spell-cast")) {
	SendCommandSpellCast(UnitSlots[unit], posx, posy, dunit, num, flags);
    } else if (!strcmp(action, "auto-spell-cast")) {
	SendCommandAutoSpellCast(UnitSlots[unit], num, posx);
    } else if (!strcmp(action, "diplomacy")) {
	int state;
	if (!strcmp(val, "neutral")) {
	    state = DiplomacyNeutral;
	} else if (!strcmp(val, "allied")) {
	    state = DiplomacyAllied;
	} else if (!strcmp(val, "enemy")) {
	    state = DiplomacyEnemy;
	} else if (!strcmp(val, "crazy")) {
	    state = DiplomacyCrazy;
	} else {
	    DebugLevel0Fn("Invalid diplomacy command: %s" _C_ val);
	    state = -1;
	}
	SendCommandDiplomacy(posx, state, posy);
    } else if (!strcmp(action, "shared-vision")) {
	int state;
	state = atoi(val);
	if (state != 0 && state != 1) {
	    DebugLevel0Fn("Invalid shared vision command: %s" _C_ val);
	    state = 0;
	}
	SendCommandSharedVision(posx, state, posy);
    } else if (!strcmp(action, "input")) {
	if (val[0] == '(') {
	    CclCommand(val);
	} else {
	    HandleCheats(val);
	}
    } else if (!strcmp(action, "quit")) {
	CommandQuit(posx);
    } else {
	DebugLevel0Fn("Invalid action: %s" _C_ action);
    }

    ReplayStep = ReplayStep->Next;
    NextLogCycle = ReplayStep ? (unsigned)ReplayStep->GameCycle : ~0UL;
}

/**
**	Replay user commands from log each cycle
*/
local void ReplayEachCycle(void)
{
    if (!CurrentReplay) {
	return;
    }
    if (InitReplay) {
	int i;
	for (i = 0; i < PlayerMax; ++i) {
	    if (CurrentReplay->Players[i].Name) {
		PlayerSetName(&Players[i], CurrentReplay->Players[i].Name);
	    }
	}
	ReplayStep = CurrentReplay->Commands;
	NextLogCycle = (ReplayStep ? (unsigned)ReplayStep->GameCycle : ~0UL);
	InitReplay = 0;
    }

    if (!ReplayStep) {
	return;
    }

    if (NextLogCycle != ~0UL && NextLogCycle != GameCycle) {
	return;
    }

    do {
	DoNextReplay();
    } while (ReplayStep &&
	    (NextLogCycle == ~0UL || NextLogCycle == GameCycle));

    if (!ReplayStep) {
	SetMessage("End of replay");
	GameObserve = 0;
    }
}

/**
**	Replay user commands from log each cycle, single player games
*/
global void SinglePlayerReplayEachCycle(void)
{
    if (ReplayGameType == ReplaySinglePlayer) {
	ReplayEachCycle();
    }
}

/**
**	Replay user commands from log each cycle, multiplayer games
*/
global void MultiPlayerReplayEachCycle(void)
{
    if (ReplayGameType == ReplayMultiPlayer) {
	ReplayEachCycle();
    }
}
//@}

//----------------------------------------------------------------------------
//	Send game commands, maybe over the network.
//----------------------------------------------------------------------------

/**@name send */
//@{

/**
**	Send command: Unit stop.
**
**	@param unit	pointer to unit.
*/
global void SendCommandStopUnit(Unit* unit)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("stop", unit, FlushCommands, -1, -1, NoUnitP, NULL, -1);
	CommandStopUnit(unit);
    } else {
	NetworkSendCommand(MessageCommandStop, unit, 0, 0, NoUnitP, 0, FlushCommands);
    }
}

/**
**	Send command: Unit stand ground.
**
**	@param unit	pointer to unit.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandStandGround(Unit* unit, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("stand-ground", unit, flush, -1, -1, NoUnitP, NULL, -1);
	CommandStandGround(unit, flush);
    } else {
	NetworkSendCommand(MessageCommandStand, unit, 0, 0, NoUnitP, 0, flush);
    }
}

/**
**	Send command: Follow unit to position.
**
**	@param unit	pointer to unit.
**	@param dest	follow this unit.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandFollow(Unit* unit, Unit* dest, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("follow", unit, flush, -1, -1, dest, NULL, -1);
	CommandFollow(unit, dest, flush);
    } else {
	NetworkSendCommand(MessageCommandFollow, unit, 0, 0, dest, 0, flush);
    }
}

/**
**	Send command: Move unit to position.
**
**	@param unit	pointer to unit.
**	@param x	X map tile position to move to.
**	@param y	Y map tile position to move to.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandMove(Unit* unit, int x, int y, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("move", unit, flush, x, y, NoUnitP, NULL, -1);
	CommandMove(unit, x, y, flush);
    } else {
	NetworkSendCommand(MessageCommandMove, unit, x, y, NoUnitP, 0, flush);
    }
}

/**
**	Send command: Unit repair.
**
**	@param unit	pointer to unit.
**	@param x	X map tile position to repair.
**	@param y	Y map tile position to repair.
**	@param dest	Unit to be repaired.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandRepair(Unit* unit, int x, int y, Unit* dest, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("repair", unit, flush, x, y, dest, NULL, -1);
	CommandRepair(unit, x, y, dest, flush);
    } else {
	NetworkSendCommand(MessageCommandRepair, unit, x, y, dest, 0, flush);
    }
}

/**
**	Send command: Unit attack unit or at position.
**
**	@param unit	pointer to unit.
**	@param x	X map tile position to attack.
**	@param y	Y map tile position to attack.
**	@param attack	or !=NoUnitP unit to be attacked.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandAttack(Unit* unit, int x, int y, Unit* attack, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("attack", unit, flush, x, y, attack, NULL, -1);
	CommandAttack(unit, x, y, attack, flush);
    } else {
	NetworkSendCommand(MessageCommandAttack, unit, x, y, attack, 0, flush);
    }
}

/**
**	Send command: Unit attack ground.
**
**	@param unit	pointer to unit.
**	@param x	X map tile position to fire on.
**	@param y	Y map tile position to fire on.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandAttackGround(Unit* unit, int x, int y, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("attack-ground", unit, flush, x, y, NoUnitP, NULL, -1);
	CommandAttackGround(unit, x, y, flush);
    } else {
	NetworkSendCommand(MessageCommandGround, unit, x, y, NoUnitP, 0, flush);
    }
}

/**
**	Send command: Unit patrol between current and position.
**
**	@param unit	pointer to unit.
**	@param x	X map tile position to patrol between.
**	@param y	Y map tile position to patrol between.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandPatrol(Unit* unit, int x, int y, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("patrol", unit, flush, x, y, NoUnitP, NULL, -1);
	CommandPatrolUnit(unit, x, y, flush);
    } else {
	NetworkSendCommand(MessageCommandPatrol, unit, x, y, NoUnitP, 0, flush);
    }
}

/**
**	Send command: Unit board unit.
**
**	@param unit	pointer to unit.
**	@param x	X map tile position (unused).
**	@param y	Y map tile position (unused).
**	@param dest	Destination to be boarded.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandBoard(Unit* unit, int x, int y, Unit* dest, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("board", unit, flush, x, y, dest, NULL, -1);
	CommandBoard(unit, dest, flush);
    } else {
	NetworkSendCommand(MessageCommandBoard, unit, x, y, dest, 0, flush);
    }
}

/**
**	Send command: Unit unload unit.
**
**	@param unit	pointer to unit.
**	@param x	X map tile position of unload.
**	@param y	Y map tile position of unload.
**	@param what	Passagier to be unloaded.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandUnload(Unit* unit, int x, int y, Unit* what, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("unload", unit, flush, x, y, what, NULL, -1);
	CommandUnload(unit, x, y, what, flush);
    } else {
	NetworkSendCommand(MessageCommandUnload, unit, x, y, what, 0, flush);
    }
}

/**
**	Send command: Unit builds building at position.
**
**	@param unit	pointer to unit.
**	@param x	X map tile position of construction.
**	@param y	Y map tile position of construction.
**	@param what	pointer to unit-type of the building.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandBuildBuilding(Unit* unit, int x, int y,
    UnitType* what, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("build", unit, flush, x, y, NoUnitP, what->Ident, -1);
	CommandBuildBuilding(unit, x, y, what, flush);
    } else {
	NetworkSendCommand(MessageCommandBuild, unit, x, y, NoUnitP, what, flush);
    }
}

/**
**	Send command: Cancel this building construction.
**
**	@param unit	pointer to unit.
**	@param worker	Worker which should stop.
*/
global void SendCommandDismiss(Unit* unit)
{
    // FIXME: currently unit and worker are same?
    if (NetworkFildes == (Socket)-1) {
	CommandLog("dismiss", unit, FlushCommands, -1, -1, NULL, NULL, -1);
	CommandDismiss(unit);
    } else {
	NetworkSendCommand(MessageCommandDismiss, unit, 0, 0, NULL, 0,
	    FlushCommands);
    }
}

/**
** 	Send command: Unit harvests a location (wood for now).
**
**	@param unit	pointer to unit.
**	@param x	X map tile position where to harvest.
**	@param y	Y map tile position where to harvest.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandResourceLoc(Unit* unit, int x, int y, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("resource-loc", unit, flush, x, y, NoUnitP, NULL, -1);
	CommandResourceLoc(unit, x, y, flush);
    } else {
	NetworkSendCommand(MessageCommandResourceLoc, unit, x, y, NoUnitP, 0, flush);
    }
}

/**
**	Send command: Unit harvest resources
**
**	@param unit	pointer to unit.
**	@param dest	pointer to destination (oil-platform,gold mine).
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandResource(Unit* unit, Unit* dest, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("resource", unit, flush, -1, -1, dest, NULL, -1);
	CommandResource(unit, dest, flush);
    } else {
	NetworkSendCommand(MessageCommandResource, unit, 0, 0, dest, 0, flush);
    }
}

/**
**	Send command: Unit return goods.
**
**	@param unit	pointer to unit.
**	@param goal	pointer to destination of the goods. (NULL=search best)
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandReturnGoods(Unit* unit, Unit* goal, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("return", unit, flush, -1, -1, goal, NULL, -1);
	CommandReturnGoods(unit, goal, flush);
    } else {
	NetworkSendCommand(MessageCommandReturn, unit, 0, 0, goal, 0, flush);
    }
}

/**
**	Send command: Building/unit train new unit.
**
**	@param unit	pointer to unit.
**	@param what	pointer to unit-type of the unit to be trained.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandTrainUnit(Unit* unit, UnitType* what, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("train", unit, flush, -1, -1, NoUnitP, what->Ident, -1);
	CommandTrainUnit(unit, what, flush);
    } else {
	NetworkSendCommand(MessageCommandTrain, unit, 0, 0, NoUnitP, what, flush);
    }
}

/**
**	Send command: Cancel training.
**
**	@param unit	Pointer to unit.
**	@param slot	Slot of training queue to cancel.
**	@param type	Unit-type of unit to cancel.
*/
global void SendCommandCancelTraining(Unit* unit, int slot, const UnitType* type)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("cancel-train", unit, FlushCommands, -1, -1, NoUnitP,
		type ? type->Ident : NULL, slot);
	CommandCancelTraining(unit, slot, type);
    } else {
	NetworkSendCommand(MessageCommandCancelTrain, unit, slot, 0, NoUnitP,
	    type, FlushCommands);
    }
}

/**
**	Send command: Building starts upgrading to.
**
**	@param unit	pointer to unit.
**	@param what	pointer to unit-type of the unit upgrade.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandUpgradeTo(Unit* unit, UnitType* what, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("upgrade-to", unit, flush, -1, -1, NoUnitP, what->Ident, -1);
	CommandUpgradeTo(unit, what, flush);
    } else {
	NetworkSendCommand(MessageCommandUpgrade, unit, 0, 0, NoUnitP, what, flush);
    }
}

/**
**	Send command: Cancel building upgrading to.
**
**	@param unit	pointer to unit.
*/
global void SendCommandCancelUpgradeTo(Unit* unit)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("cancel-upgrade-to", unit, FlushCommands,
	    -1, -1, NoUnitP, NULL, -1);
	CommandCancelUpgradeTo(unit);
    } else {
	NetworkSendCommand(MessageCommandCancelUpgrade, unit,
	    0, 0, NoUnitP, NULL, FlushCommands);
    }
}

/**
**	Send command: Building/unit research.
**
**	@param unit	pointer to unit.
**	@param what	research-type of the research.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandResearch(Unit* unit, Upgrade* what, int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("research", unit, flush, -1, -1, NoUnitP, what->Ident, -1);
	CommandResearch(unit, what, flush);
    } else {
	NetworkSendCommand(MessageCommandResearch, unit,
	    what-Upgrades, 0, NoUnitP, NULL, flush);
    }
}

/**
**	Send command: Cancel Building/unit research.
**
**	@param unit	pointer to unit.
*/
global void SendCommandCancelResearch(Unit* unit)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("cancel-research", unit, FlushCommands, -1, -1, NoUnitP, NULL, -1);
	CommandCancelResearch(unit);
    } else {
	NetworkSendCommand(MessageCommandCancelResearch, unit,
	    0, 0, NoUnitP, NULL, FlushCommands);
    }
}

/**
**	Send command: Unit spell cast on position/unit.
**
**	@param unit	pointer to unit.
**	@param x	X map tile position where to cast spell.
**	@param y	Y map tile position where to cast spell.
**	@param dest	Cast spell on unit (if exist).
**	@param spellid  Spell type id.
**	@param flush	Flag flush all pending commands.
*/
global void SendCommandSpellCast(Unit* unit, int x, int y, Unit* dest, int spellid,
    int flush)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("spell-cast", unit, flush, x, y, dest, NULL, spellid);
	CommandSpellCast(unit, x, y, dest, SpellTypeById(spellid), flush);
    } else {
	NetworkSendCommand(MessageCommandSpellCast + spellid,
	    unit, x, y, dest, NULL, flush);
    }
}

/**
**	Send command: Unit auto spell cast.
**
**	@param unit	pointer to unit.
**	@param spellid  Spell type id.
**	@param on	1 for auto cast on, 0 for off.
*/
global void SendCommandAutoSpellCast(Unit* unit, int spellid, int on)
{
    if (NetworkFildes == (Socket)-1) {
	CommandLog("auto-spell-cast", unit, FlushCommands, on, -1, NoUnitP,
	    NULL, spellid);
	CommandAutoSpellCast(unit, on ? SpellTypeById(spellid) : NULL);
    } else {
	NetworkSendCommand(MessageCommandSpellCast + spellid,
	    unit, on, -1, NoUnitP, NULL, FlushCommands);
    }
}

/**
**	Send command: Diplomacy changed.
**
**	@param player	Player which changes his state.
**	@param state	New diplomacy state.
**	@param opponent	Opponent.
*/
global void SendCommandDiplomacy(int player, int state, int opponent)
{
    if (NetworkFildes == (Socket)-1) {
	switch (state) {
	    case DiplomacyNeutral:
		CommandLog("diplomacy", NoUnitP, 0, player, opponent,
		    NoUnitP, "neutral", -1);
		break;
	    case DiplomacyAllied:
		CommandLog("diplomacy", NoUnitP, 0, player, opponent,
		    NoUnitP, "allied", -1);
		break;
	    case DiplomacyEnemy:
		CommandLog("diplomacy", NoUnitP, 0, player, opponent,
		    NoUnitP, "enemy", -1);
		break;
	    case DiplomacyCrazy:
		CommandLog("diplomacy", NoUnitP, 0, player, opponent,
		    NoUnitP, "crazy", -1);
		break;
	}
	CommandDiplomacy(player, state, opponent);
    } else {
	NetworkSendExtendedCommand(ExtendedMessageDiplomacy,
	    -1, player, state, opponent, 0);
    }
}

/**
**	Send command: Shared vision changed.
**
**	@param player	Player which changes his state.
**	@param state	New shared vision state.
**	@param opponent	Opponent.
*/
global void SendCommandSharedVision(int player, int state, int opponent)
{
    if (NetworkFildes == (Socket)-1) {
	if (state == 0) {
	    CommandLog("shared-vision", NoUnitP, 0, player, opponent,
		NoUnitP, "0", -1);
	} else {
	    CommandLog("shared-vision", NoUnitP, 0, player, opponent,
		NoUnitP, "1", -1);
	}
	CommandSharedVision(player, state, opponent);
    } else {
	NetworkSendExtendedCommand(ExtendedMessageSharedVision,
	    -1, player, state, opponent, 0);
    }
}

global void NetworkCclRegister(void)
{
#if defined(USE_GUILE) || defined(USE_SIOD)
    gh_new_procedure1_0("log", CclLog);
    gh_new_procedure1_0("replay-log", CclReplayLog);
#elif defined(USE_LUA)
    lua_register(Lua, "Log", CclLog);
    lua_register(Lua, "ReplayLog", CclReplayLog);
#endif
}

//@}

//----------------------------------------------------------------------------
//	Parse the message, from the network.
//----------------------------------------------------------------------------

/**@name parse */
//@{

/**
**	Parse a command (from network).
**
**	@param msgnr	Network message type
**	@param unum	Unit number (slot) that receive the command.
**	@param x	optional X map position.
**	@param y	optional y map position.
**	@param dstnr	optional destination unit.
*/
global void ParseCommand(unsigned char msgnr, UnitRef unum,
    unsigned short x, unsigned short y, UnitRef dstnr)
{
    Unit* unit;
    Unit* dest;
    int id;
    int status;

    DebugLevel3Fn(" %d cycle %lu\n" _C_ msgnr _C_ GameCycle);

    unit = UnitSlots[unum];
    DebugCheck(!unit);
    //
    //	Check if unit is already killed?
    //
    if (unit->Destroyed) {
	DebugLevel0Fn(" destroyed unit skipping %d\n" _C_ UnitNumber(unit));
	return;
    }

    DebugCheck(!unit->Type);

    status = (msgnr & 0x80) >> 7;

    // Note: destroyed destination unit is handled by the action routines.

    switch (msgnr & 0x7F) {
	case MessageSync:
	    return;
	case MessageQuit:
	    return;
	case MessageChat:
	    return;

	case MessageCommandStop:
	    CommandLog("stop", unit, FlushCommands, -1, -1, NoUnitP, NULL, -1);
	    CommandStopUnit(unit);
	    break;
	case MessageCommandStand:
	    CommandLog("stand-ground", unit, status, -1, -1, NoUnitP, NULL, -1);
	    CommandStandGround(unit, status);
	    break;
	case MessageCommandFollow:
	    dest = NoUnitP;
	    if (dstnr != (unsigned short)0xFFFF) {
		dest = UnitSlots[dstnr];
		DebugCheck(!dest || !dest->Type);
	    }
	    CommandLog("follow", unit, status, -1, -1, dest, NULL, -1);
	    CommandFollow(unit, dest, status);
	    break;
	case MessageCommandMove:
	    CommandLog("move", unit, status, x, y, NoUnitP, NULL, -1);
	    CommandMove(unit, x, y, status);
	    break;
	case MessageCommandRepair:
	    dest = NoUnitP;
	    if (dstnr != (unsigned short)0xFFFF) {
		dest = UnitSlots[dstnr];
		DebugCheck(!dest || !dest->Type);
	    }
	    CommandLog("repair", unit, status, x, y, dest, NULL, -1);
	    CommandRepair(unit, x, y, dest, status);
	    break;
	case MessageCommandAttack:
	    dest = NoUnitP;
	    if (dstnr != (unsigned short)0xFFFF) {
		dest = UnitSlots[dstnr];
		DebugCheck(!dest || !dest->Type);
	    }
	    CommandLog("attack", unit, status, x, y, dest, NULL, -1);
	    CommandAttack(unit, x, y, dest, status);
	    break;
	case MessageCommandGround:
	    CommandLog("attack-ground", unit, status, x, y, NoUnitP, NULL, -1);
	    CommandAttackGround(unit, x, y, status);
	    break;
	case MessageCommandPatrol:
	    CommandLog("patrol", unit, status, x, y, NoUnitP, NULL, -1);
	    CommandPatrolUnit(unit, x, y, status);
	    break;
	case MessageCommandBoard:
	    dest = NoUnitP;
	    if (dstnr != (unsigned short)0xFFFF) {
		dest = UnitSlots[dstnr];
		DebugCheck(!dest || !dest->Type);
	    }
	    CommandLog("board", unit, status, x, y, dest, NULL, -1);
	    CommandBoard(unit, dest, status);
	    break;
	case MessageCommandUnload:
	    dest = NoUnitP;
	    if (dstnr != (unsigned short)0xFFFF) {
		dest = UnitSlots[dstnr];
		DebugCheck(!dest || !dest->Type);
	    }
	    CommandLog("unload", unit, status, x, y, dest, NULL, -1);
	    CommandUnload(unit, x, y, dest, status);
	    break;
	case MessageCommandBuild:
	    CommandLog("build", unit, status, x, y, NoUnitP, UnitTypes[dstnr]->Ident,
		-1);
	    CommandBuildBuilding(unit, x, y, UnitTypes[dstnr], status);
	    break;
	case MessageCommandDismiss:
	    CommandLog("dismiss", unit, FlushCommands, -1, -1, NULL, NULL, -1);
	    CommandDismiss(unit);
	    break;
	case MessageCommandResourceLoc:
	    CommandLog("resource-loc", unit, status, x, y, NoUnitP, NULL, -1);
	    CommandResourceLoc(unit, x, y, status);
	    break;
	case MessageCommandResource:
	    dest = NoUnitP;
	    if (dstnr != (unsigned short)0xFFFF) {
		dest = UnitSlots[dstnr];
		DebugCheck(!dest || !dest->Type);
	    }
	    CommandLog("resource", unit, status, -1, -1, dest, NULL, -1);
	    CommandResource(unit, dest, status);
	    break;
	case MessageCommandReturn:
	    dest = NoUnitP;
	    if (dstnr != (unsigned short)0xFFFF) {
		dest = UnitSlots[dstnr];
		DebugCheck(!dest || !dest->Type);
	    }
	    CommandLog("return", unit, status, -1, -1, dest, NULL, -1);
	    CommandReturnGoods(unit, dest, status);
	    break;
	case MessageCommandTrain:
	    CommandLog("train", unit, status, -1, -1, NoUnitP,
		UnitTypes[dstnr]->Ident, -1);
	    CommandTrainUnit(unit, UnitTypes[dstnr], status);
	    break;
	case MessageCommandCancelTrain:
	    // We need (short)x for the last slot -1
	    if (dstnr != (unsigned short)0xFFFF) {
		CommandLog("cancel-train", unit, FlushCommands, -1, -1, NoUnitP,
		    UnitTypes[dstnr]->Ident, (short)x);
		CommandCancelTraining(unit, (short)x, UnitTypes[dstnr]);
	    } else {
		CommandLog("cancel-train", unit, FlushCommands, -1, -1, NoUnitP,
		    NULL, (short)x);
		CommandCancelTraining(unit, (short)x, NULL);
	    }
	    break;
	case MessageCommandUpgrade:
	    CommandLog("upgrade-to", unit, status, -1, -1, NoUnitP,
		UnitTypes[dstnr]->Ident, -1);
	    CommandUpgradeTo(unit, UnitTypes[dstnr], status);
	    break;
	case MessageCommandCancelUpgrade:
	    CommandLog("cancel-upgrade-to", unit, FlushCommands, -1, -1, NoUnitP,
		NULL, -1);
	    CommandCancelUpgradeTo(unit);
	    break;
	case MessageCommandResearch:
	    CommandLog("research", unit, status, -1, -1, NoUnitP,
		Upgrades[x].Ident, -1);
	    CommandResearch(unit,Upgrades+x, status);
	    break;
	case MessageCommandCancelResearch:
	    CommandLog("cancel-research", unit, FlushCommands, -1, -1, NoUnitP,
		NULL, -1);
	    CommandCancelResearch(unit);
	    break;
	default:
	    id = (msgnr&0x7f) - MessageCommandSpellCast;
	    if (y != (unsigned short)0xFFFF) {
		dest = NoUnitP;
		if (dstnr != (unsigned short)0xFFFF) {
		    dest = UnitSlots[dstnr];
		    DebugCheck(!dest || !dest->Type);
		}
		CommandLog("spell-cast", unit, status, x, y, dest, NULL, id);
		CommandSpellCast(unit, x, y, dest, SpellTypeById(id), status);
	    } else {
		CommandLog("auto-spell-cast", unit, status,x, -1, NoUnitP, NULL, id);
		CommandAutoSpellCast(unit, x ? SpellTypeById(id) : NULL);
	    }
	    break;
    }
}

/**
**	Parse an extended command (from network).
**
**	@param type	Network extended message type
**	@param status	Bit 7 of message type
**	@param arg1	Messe argument 1
**	@param arg2	Messe argument 2
**	@param arg3	Messe argument 3
**	@param arg4	Messe argument 4
*/
global void ParseExtendedCommand(unsigned char type, int status,
    unsigned char arg1, unsigned short arg2, unsigned short arg3,
    unsigned short arg4)
{
    DebugLevel3Fn(" %d cycle %lu\n" _C_ type _C_ GameCycle);

    // Note: destroyed units are handled by the action routines.

    switch (type) {
	case ExtendedMessageDiplomacy:
	    switch (arg3) {
		case DiplomacyNeutral:
		    CommandLog("diplomacy", NoUnitP, 0, arg2, arg4,
			NoUnitP, "neutral", -1);
		    break;
		case DiplomacyAllied:
		    CommandLog("diplomacy", NoUnitP, 0, arg2, arg4,
			NoUnitP, "allied", -1);
		    break;
		case DiplomacyEnemy:
		    CommandLog("diplomacy", NoUnitP, 0, arg2, arg4,
			NoUnitP, "enemy", -1);
		    break;
		case DiplomacyCrazy:
		    CommandLog("diplomacy", NoUnitP, 0, arg2, arg4,
			NoUnitP, "crazy", -1);
		    break;
	    }
	    CommandDiplomacy(arg2,arg3,arg4);
	    break;
	case ExtendedMessageSharedVision:
	    if (arg3 == 0) {
		CommandLog("shared-vision", NoUnitP, 0, arg2, arg4,
		    NoUnitP, "0", -1);
	    } else {
		CommandLog("shared-vision", NoUnitP, 0, arg2, arg4,
		    NoUnitP, "1", -1);
	    }
	    CommandSharedVision(arg2, arg3, arg4);
	    break;
	default:
	    DebugLevel0Fn("Unknown extended message %u/%s %u %u %u %u\n" _C_
		type _C_ status ? "flush" : "-" _C_
		arg1 _C_ arg2 _C_ arg3 _C_ arg4);
	    break;
    }
}

//@}
