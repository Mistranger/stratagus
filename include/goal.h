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
/**@name goal.h		-	The game goal headerfile. */
/*
**	(c) Copyright 1999,2000 by Lutz Sammer
**
**	$Id$
*/

#ifndef __GOAL_H__
#define __GOAL_H__

//@{

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

/**
**	All possible game goals.
*/
enum _game_goal_ {
    GoalLastSideWins,			/// the last player with units wins
};

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

global void SetGlobalGoal(int goal);	/// set global game goal
global void CheckGoals(void);		/// test if goals reached

//@}

#endif	// !__GOAL_H__
