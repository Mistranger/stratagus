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
/**@name splitter.h		-	The map splitter headerfile. */
//
//	(c) Copyright 1998-2003 by Vladi Shabanski, Lutz Sammer,
//	                           and Jimmy Salmon
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

#ifndef __SPLITTER_H__
#define __SPLITTER_H__

/*----------------------------------------------------------------------------
--	Constants
----------------------------------------------------------------------------*/

// Should be enough for every one :-)
#define MaxZoneNumber		512	/// Max number of zone ( separated area )
#define MaxRegionNumber		4096	/// Max number of regions ( divisions of zones )

#define NoRegion 		((RegionId)~0UL)

/*----------------------------------------------------------------------------
--	Structures
----------------------------------------------------------------------------*/

/// Region identifier
typedef unsigned short int RegionId;


/// Zone marque list. Must be a global variable, with MarqueId initialised to 0
typedef struct _zone_set_ {
    int		Id;			/// Internal - must be initialised to 0
    int		ZoneCount;		/// N� of marqued zones
    int		Marks[MaxZoneNumber];	/// ZoneMarque[zone] ?= MarqueId
    int		Zones[MaxZoneNumber];	/// List of marqued zones
}ZoneSet;

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/// Initialise the region system
global void MapSplitterInit(void);

/// Free the region system
global void MapSplitterClean(void);

/// Maintain the region system up2date
global void MapSplitterEachCycle(void);

/// Call this when some tiles become available ( no wood / rock / building )
global void MapSplitterTilesCleared(int x0,int y0,int x1,int y1);

/// Call this when some tiles become unavailable ( wood / rock / building appeared )
global void MapSplitterTilesOccuped(int x0,int y0,int x1,int y1);

/// Clear a ZoneSet object ( must be a global object )
global void ZoneSetClear(ZoneSet* m);

global void ZoneSetAddCell(ZoneSet * m,int x,int y);

/// Add a zone to a ZoneSet
global int ZoneSetAddZone(ZoneSet* m, int zone);

/// Add a zoneset into a ZoneSet
global void ZoneSetAddSet(ZoneSet* dst, ZoneSet* src);

/// Check if a zone is in a ZoneSet
global void ZoneSetIntersect(ZoneSet* dst, ZoneSet* src);

global int ZoneSetHasIntersect(ZoneSet* a, ZoneSet* b);

global int ZoneSetContains(ZoneSet* a,int zone);

global void ZoneSetDebug(ZoneSet * set);

global void ZoneSetAddUnitZones(ZoneSet * set,Unit * unit);

global void ZoneSetAddGoalZones(ZoneSet* set,Unit* unit, int gx, int gy,int gw,int gh,int minrange,int maxrange);

/**
** Add zones connected to src to dst
**
*/
global void ZoneSetAddConnected(ZoneSet* dst, ZoneSet * src);

global int ZoneSetFindPath(ZoneSet* src,ZoneSet* dst,int * path,int * pathlen);

// Return a point in destzone connected to srczone
global void ZoneFindConnexion(int destzone,int srczone,int refX,int refY,int* x,int* y);

#endif // __MAP_REGIONS_H__
