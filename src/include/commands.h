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
/**@name commands.h	-	The commands header file. */
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

#ifndef __COMMANDS_H__
#define __COMMANDS_H__

//@{

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern int CommandLogDisabled;		/// True, if command log is off

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

    /// Replay user commands from log each cycle
extern void ReplayEachCycle(void);
    /// Load replay
extern int LoadReplay(char* name);
    /// End logging
extern void EndReplayLog(void);

/*
**	The send command functions sends a command, if needed over the
**	Network, this is only for user commands. Automatic reactions which
**	are on all computers equal, should use the functions without Send.
*/

    /// Send stop command
extern void SendCommandStopUnit(Unit* unit);
    /// Send stand ground command
extern void SendCommandStandGround(Unit* unit,int flush);
    /// Send follow command
extern void SendCommandFollow(Unit* unit,Unit* dest,int flush);
    /// Send move command
extern void SendCommandMove(Unit* unit,int x,int y,int flush);
    /// Send repair command
extern void SendCommandRepair(Unit* unit,int x,int y,Unit* dest,int flush);
    /// Send attack command
extern void SendCommandAttack(Unit* unit,int x,int y,Unit* dest,int flush);
    /// Send attack ground command
extern void SendCommandAttackGround(Unit* unit,int x,int y,int flush);
    /// Send patrol command
extern void SendCommandPatrol(Unit* unit,int x,int y,int flush);
    /// Send board command
extern void SendCommandBoard(Unit* unit,int x,int y,Unit* dest,int flush);
    /// Send unload command
extern void SendCommandUnload(Unit* unit,int x,int y,Unit* what,int flush);
    /// Send build building command
extern void SendCommandBuildBuilding(Unit*,int,int,UnitType*,int);
    /// Send cancel building command
extern void SendCommandCancelBuilding(Unit* unit,Unit* peon);
    /// Send harvest command
extern void SendCommandHarvest(Unit* unit,int x,int y,int flush);
    /// Send mine gold command
extern void SendCommandMineGold(Unit* unit,Unit* dest,int flush);
    /// Send haul oil command
extern void SendCommandHaulOil(Unit* unit,Unit* dest,int flush);
    /// Send return goods command
extern void SendCommandReturnGoods(Unit* unit,Unit* dest,int flush);
    /// Send train command
extern void SendCommandTrainUnit(Unit* unit,UnitType* what,int flush);
    /// Send cancel training command
extern void SendCommandCancelTraining(Unit* unit,int slot,const UnitType* type);
    /// Send upgrade to command
extern void SendCommandUpgradeTo(Unit* unit,UnitType* what,int flush);
    /// Send cancel upgrade to command
extern void SendCommandCancelUpgradeTo(Unit* unit);
    /// Send research command
extern void SendCommandResearch(Unit* unit,Upgrade* what,int flush);
    /// Send cancel research command
extern void SendCommandCancelResearch(Unit* unit);
    /// Send demolish command
extern void SendCommandDemolish(Unit* unit,int x,int y,Unit* dest,int flush);
    /// Send spell cast command
extern void SendCommandSpellCast(Unit* unit,int x,int y,Unit* dest,int spellid
	,int flush);
    /// Send diplomacy command
extern void SendCommandDiplomacy(int player,int state,int opponent);

    /// Parse a command (from network).
extern void ParseCommand(unsigned char type,UnitRef unum,unsigned short x,
	unsigned short y,UnitRef dest);
    /// Parse an extended command (from network).
extern void ParseExtendedCommand(unsigned char type,int status,
	unsigned char arg1, unsigned short arg2,unsigned short arg3,
	unsigned short arg4);

//@}

#endif	// !__COMMANDS_H__
