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
/**@name deco.h 	-	Mechanism for all 2D decorations on screen */
//
//	(c) Copyright 2002 by Lutz Sammer and Stephan Rasenberg
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

#ifndef __DECO_H__
#define __DECO_H__

//@{

/*----------------------------------------------------------------------------
--      Declarations
----------------------------------------------------------------------------*/

/**
**	
**/
typedef struct DecorationSingle {
  // next single-tile decoration belonging to the same decoration
  struct DecorationSingle *nxt;
  // exact 2x2 tile area
  char *tiles;
  // 16bit bitmask which denote the area in above tiles overlapped by this deco
  unsigned int lefttopmask, righttopmask, leftbottommask, rightbottommask;
  // the bit index (bity4+bitx) of the left-top in the first tile
  // @note  bity4 is a multiple of 4 tiles (so no multiple needed)
  int bitx, bity4;
  // left-top pixel position
  int topleftx, toplefty;
} DecorationSingle;

/**
**	A 2D decoration as is supported by this mechanism, add any to be draw
**	element (sprite/line/button/etc..) as a 2D decoration following this
**	structu, so the mechanism can use it and automaticly update any other
**	decoration overlapping it.
**
**	draw	= an user given function that draws the decoration using
**	          some vidoe functions based on the clip rectangle ClipX1,..
**      data    = an user given data-type given to above function, to be able
**	          to provide a generic draw-function independent of its data.
**	x,y,w,h = dimension as given to DecorationAdd..
**	          @note now needed outside, but might be removed in the future
**	l       = decoration level as given to DecorationAdd (internal use only)
**	singles = The sub-decoration type, as this decoration might be split
**	          into multiple small/fixed-sized data-type (internal use only)
**	prv	= prev decoration based on depth-level (internal use only)
**	nxt	= next decoration based on depth-level (internal use only)
**/
typedef struct Deco {
  void *data;
  void (*drawclip)(void *data);
  struct DecorationSingle *singles;
  struct Deco *prv, *nxt;
  int x, y, w, h;
  int level;
} Deco;

/**
**
**
**/

/*----------------------------------------------------------------------------
--      Functions
----------------------------------------------------------------------------*/


extern void DecorationInit(void);

extern Deco* DecorationAdd(void *data,void (*drawclip)(void *data),
	int level, int x, int y, int w, int h );
extern Deco* DecorationMove(Deco *d, int x, int y, int w, int h);
extern void DecorationRemove(Deco *d);
extern void DecorationRemoveLevels(int min, int max);

extern void DecorationMark(Deco *d);

extern void DecorationRefreshDisplay(void);
extern void DecorationUpdateDisplay(void);

//@}

#endif	// !__DECO_H__
