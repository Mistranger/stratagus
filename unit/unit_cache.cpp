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
/**@name unitcache.c	-	The unit cache. */
//
//	Cache to find units faster from position.
//	Sort of trivial implementation, since most queries are on a single tile.
//	Unit is just inserted in a double linked list for every tile it's on.
//
//	(c) Copyright 1998-2003 by Lutz Sammer(older implementations) and Crestez Leonard
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
//	    along with this program; if not, write to the Free Software
//	    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//	    02111-1307, USA.
//
//		$Id$

//@{

/*----------------------------------------------------------------------------
--		Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"

#ifdef UNIT_ON_MAP

/*----------------------------------------------------------------------------
--		Includes
----------------------------------------------------------------------------*/

#include "unit.h"
#include "map.h"

//*****************************************************************************
//		Convert calls to internal
//****************************************************************************/

/**
**		Insert new unit into cache.
**
**		@param unit		Unit pointer to place in cache.
*/
global void UnitCacheInsert(Unit* unit)
{
	MapField* mf;

	DebugCheck(unit->Next);
	DebugLevel3Fn("%d,%d %d %s\n" _C_ unit->X _C_ unit->Y _C_ unit->Slot _C_ unit->Type->Name);

	mf = TheMap.Fields + unit->Y * TheMap.Width + unit->X;
	unit->Next = mf->UnitCache;
	mf->UnitCache = unit;
	DebugLevel3Fn("%d,%d %p %p\n" _C_ unit->X _C_ unit->Y _C_ unit _C_ unit->Next);
}

/**
**		Remove unit from cache.
**
**		@param unit		Unit pointer to remove from cache.
*/
global void UnitCacheRemove(Unit* unit)
{
	DebugLevel3Fn("%d,%d %d %s\n" _C_ unit->X _C_ unit->Y _C_ unit->Slot _C_ unit->Type->Name);

	Unit* prev;
	prev = TheMap.Fields[unit->Y * TheMap.Width + unit->X].UnitCache;
	if (prev == unit) {
		TheMap.Fields[unit->Y * TheMap.Width + unit->X].UnitCache = unit->Next;
		unit->Next = 0;
		return;
	}
	for (; prev; prev = prev->Next) {
		if (prev->Next == unit) {
			prev->Next = unit->Next;
			unit->Next = 0;
			return;
		}
	}
	DebugLevel0Fn("Try to remove unit not in cache.\n");
}

/**
**		Change unit in cache.
**
**		@param unit		Unit pointer to change in cache.
*/
global void UnitCacheChange(Unit* unit)
{
	UnitCacheRemove(unit);				// must remove first
	UnitCacheInsert(unit);
}

/**
**		Select units in rectangle range.
**
**		@param x1		Left column of selection rectangle
**		@param y1		Top row of selection rectangle
**		@param x2		Right column of selection rectangle
**		@param y2		Bottom row of selection rectangle
**		@param table		All units in the selection rectangle
**
**		@return				Returns the number of units found
*/
//#include "rdtsc.h"
global int UnitCacheSelect(int x1, int y1, int x2, int y2, Unit** table)
{
	int x;
	int y;
	int n;
	int i;
	Unit* unit;
	MapField* mf;
//  int ts0 = rdtsc(), ts1;

	DebugLevel3Fn("%d,%d %d,%d\n" _C_ x1 _C_ y1 _C_ x2 _C_ y2);
	//
	//		Units are inserted by origin position
	//
	x = x1 - 4;
	if (x < 0) {
		x = 0;				// Only for current unit-cache !!
	}
	y = y1 - 4;
	if (y < 0) {
		y = 0;
	}

	//
	//		Reduce to map limits. FIXME: should the caller check?
	//
	if (x2 > TheMap.Width) {
		x2 = TheMap.Width;
	}
	if (y2 > TheMap.Height) {
		y2 = TheMap.Height;
	}

	for (n = 0; y < y2; ++y) {
		mf = TheMap.Fields + y * TheMap.Width + x;
		for (i = x; i < x2; ++i) {

			for (unit = mf->UnitCache; unit; unit = unit->Next) {
				//
				//		Remove units, outside range.
				//
				if (unit->X + unit->Type->TileWidth <= x1 || unit->X > x2 ||
						unit->Y + unit->Type->TileHeight <= y1 || unit->Y > y2) {
					continue;
				}
				table[n++] = unit;
			}
			++mf;
		}
	}

//  ts1 = rdtsc();
//  printf("UnitCacheSelect on %dx%d took %d cycles\n", x2 - x1, y2 - y1, ts1 - ts0);

	return n;
}

/**
**		Select units on map tile.
**
**		@param x		Map X tile position
**		@param y		Map Y tile position
**		@param table		All units in the selection rectangle
**
**		@return				Returns the number of units found
*/
global int UnitCacheOnTile(int x, int y, Unit** table)
{
	return UnitCacheSelect(x, y, x + 1, y + 1, table);
}

/**
**		Select unit on X,Y of type naval,fly,land.
**
**		@param x		Map X tile position.
**		@param y		Map Y tile position.
**		@param type		UnitType::UnitType, naval,fly,land.
**
**		@return				Unit, if an unit of correct type is on the field.
*/
global Unit* UnitCacheOnXY(int x, int y, unsigned type)
{
	Unit* table[UnitMax];
	int n;

	n = UnitCacheOnTile(x, y, table);
	while (n--) {
		if ((unsigned)table[n]->Type->UnitType == type) {
			break;
		}
	}
	if (n > -1) {
		return table[n];
	} else {
		return NoUnitP;
	}
}

/**
**		Print unit-cache statistic.
*/
global void UnitCacheStatistic(void)
{
}

/**
**		Initialize unit-cache.
*/
global void InitUnitCache(void)
{
}

#endif		// } UNIT_ON_MAP
