//   ___________		     _________		      _____  __
//   \_	  _____/______   ____   ____ \_   ___ \____________ _/ ____\/  |_
//    |    __) \_  __ \_/ __ \_/ __ \/    \  \/\_  __ \__  \\   __\\   __|
//    |     \   |  | \/\  ___/\  ___/\     \____|  | \// __ \|  |   |  |
//    \___  /   |__|    \___  >\___  >\______  /|__|  (____  /__|   |__|
//	  \/		    \/	   \/	     \/		   \/
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   FreeCraft - A free fantasy real time strategy game engine
//
/**@name ccl_pathfinder.c	-	pathfinder ccl functions. */
//
//	(c) Copyright 2000-2002 by Lutz Sammer, Fabrice Rossi, Latimerius.
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
--	Functions
----------------------------------------------------------------------------*/

/**
 **	Enable a*.
*/
local SCM CclAStar(void)
{
    AStarOn=1;
    if(!CclInConfigFile) {
	// allocation is done directly in this alternate case
	InitAStar();
    }
    DebugLevel0("A* is ON :-)\n");

    return SCM_UNSPECIFIED;
}

/**
**	Disable a*.
*/
local SCM CclNoAStar(void)
{
    AStarOn=0;
    if(!CclInConfigFile) {
	FreeAStar();
    }
    DebugLevel0("A* is OFF :-(\n");

    return SCM_UNSPECIFIED;
}

/**
**	Set a* parameter (cost of FIXED unit tile crossing).
*/
local SCM CclAStarSetFixedUCC(SCM cost)
{
    int i;

    i=gh_scm2int(cost);
    if( i<=0) {
	PrintFunction();
	fprintf(stdout,"Fixed unit crossing cost must be strictly positive\n");
	i=TheMap.Width*TheMap.Height;
    }
    AStarFixedUnitCrossingCost=i;

    return SCM_UNSPECIFIED;
}

/**
**	Set a* parameter (cost of MOVING unit tile crossing).
*/
local SCM CclAStarSetMovingUCC(SCM cost)
{
    int i;

    i=gh_scm2int(cost);
    if( i<=0) {
	PrintFunction();
	fprintf(stdout,"Moving unit crossing cost must be strictly positive\n");
	i=1;
    }
    AStarMovingUnitCrossingCost=i;

    return SCM_UNSPECIFIED;
}

#ifdef HIERARCHIC_PATHFINDER
local SCM CclPfHierShowRegIds (SCM flag)
{
    PfHierShowRegIds = gh_scm2bool (flag);
    return SCM_UNSPECIFIED;
}

local SCM CclPfHierShowGroupIds (SCM flag)
{
    PfHierShowGroupIds = gh_scm2bool (flag);
    return SCM_UNSPECIFIED;
}
#else
local SCM CclPfHierShowRegIds (SCM flag __attribute__((unused)))
{
    return SCM_UNSPECIFIED;
}

local SCM CclPfHierShowGroupIds (SCM flag __attribute__((unused)))
{
    return SCM_UNSPECIFIED;
}
#endif


/**
**	Register CCL features for pathfinder.
*/
global void PathfinderCclRegister(void)
{
    gh_new_procedure0_0("a-star",CclAStar);
    gh_new_procedure0_0("no-a-star",CclNoAStar);
    gh_new_procedure1_0("a-star-fixed-unit-cost",CclAStarSetFixedUCC);
    gh_new_procedure1_0("a-star-moving-unit-cost",CclAStarSetMovingUCC);
    gh_new_procedure1_0 ("pf-show-regids!", CclPfHierShowRegIds);
    gh_new_procedure1_0 ("pf-show-groupids!", CclPfHierShowGroupIds);
}

//@}
