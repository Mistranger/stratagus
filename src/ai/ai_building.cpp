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
/**@name ai_building.c	-	AI building functions. */
//
//      (c) Copyright 2001,2002 by Lutz Sammer
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
//      $Id$

#ifdef NEW_AI	// {

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "freecraft.h"

#include "unit.h"
#include "map.h"
#include "pathfinder.h"
#include "ai_local.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Check if the surrounding are free.
**
**	@param worker	Worker to build.
**	@param type	Type of building.
**	@param x	X map tile position for the building.
**	@param y	Y map tile position for the building.
**
**	@return		True if the surrounding is free, false otherwise.
**
**	@note		Can be faster written.
*/
local int AiCheckSurrounding(const Unit* worker,const UnitType* type,
	int x, int y)
{
    int i;
    int h;
    int w;

    h=type->TileHeight+2;
    w=type->TileWidth+2;
    --x;
    --y;

    for( i=0; i<w; ++i ) {		// Top row
	if( (x+i)<0 || (x+i)>TheMap.Width ) {	// FIXME: slow, worse,...
	    continue;
	}
	if( !((x+i)==worker->X && y==worker->Y) && y>=0
		&& TheMap.Fields[x+i+y*TheMap.Width].Flags&(MapFieldUnpassable
		|MapFieldWall|MapFieldRocks|MapFieldForest|MapFieldBuilding) ) {
	    return 0;
	}				// Bot row
	if( !((x+i)==worker->X && (y+h)==worker->Y) && (y+h)<TheMap.Height
		&& TheMap.Fields[x+i+(y+h)*TheMap.Width].Flags&
		(MapFieldUnpassable|MapFieldWall|MapFieldRocks
		|MapFieldForest|MapFieldBuilding) ) {
	    return 0;
	}
    }

    ++y;
    h-=2;
    for( i=0; i<h; ++i ) {		// Left row
	if( (y+i)<0 || (y+i)>TheMap.Height ) {	// FIXME: slow, worse,...
	    continue;
	}
	if( !(x==worker->X && (y+i)==worker->Y) && x>=0
		&& TheMap.Fields[x+(y+i)*TheMap.Width].Flags&
		(MapFieldUnpassable|MapFieldWall|MapFieldRocks
		|MapFieldForest|MapFieldBuilding) ) {
	    return 0;
	}				// Right row
	if( !((x+w)==worker->X && (y+i)==worker->Y) && (x+w)<TheMap.Width
		&& TheMap.Fields[x+w+(y+i)*TheMap.Width].Flags&
		(MapFieldUnpassable|MapFieldWall|MapFieldRocks
		|MapFieldForest|MapFieldBuilding) ) {
	    return 0;
	}
    }
    return 1;
}

#if 0
/**
**      Find free building place.
**
**	@param worker	Worker to build building.
**	@param type	Type of building.
**	@param dx	Pointer for X position returned.
**	@param dy	Pointer for Y position returned.
**	@param flag	Flag if surrounding must be free.
**	@return		True if place found, false if no found.
**
**	@note	This can be done faster, use flood fill.
*/
local int AiFindBuildingPlace2(const Unit * worker, const UnitType * type,
	int *dx, int *dy,int flag)
{
    int wx, wy, x, y, addx, addy;
    int end, state;

    wx = worker->X;
    wy = worker->Y;
    x = wx;
    y = wy;
    addx = 1;
    addy = 1;

    state = 0;
    end = y + addy - 1;
    for (;;) {				// test rectangles around the place
	switch (state) {
	case 0:
	    if (y++ == end) {
		++state;
		end = x + addx++;
	    }
	    break;
	case 1:
	    if (x++ == end) {
		++state;
		end = y - addy++;
	    }
	    break;
	case 2:
	    if (y-- == end) {
		++state;
		end = x - addx++;
	    }
	    break;
	case 3:
	    if (x-- == end) {
		state = 0;
		end = y + addy++;
		if( addx>=TheMap.Width && addy>=TheMap.Height ) {
		    return 0;
		}
	    }
	    break;
	}

	// FIXME: this check outside the map could be speeded up.
	if (y < 0 || x < 0 || y >= TheMap.Height || x >= TheMap.Width) {
	    continue;
	}
	if (CanBuildUnitType(worker, type, x, y)
		&& (!flag || AiCheckSurrounding(worker,type, x, y))
		&& PlaceReachable(worker, x, y, 1) ) {
	    *dx=x;
	    *dy=y;
	    return 1;
	}
    }
    return 0;
}

#endif

/**
**      Find free building place. (flood fill version)
**
**	@param worker	Worker to build building.
**	@param type	Type of building.
**	@param ox	Original X position to try building
**	@param oy	Original Y position to try building
**	@param dx	Pointer for X position returned.
**	@param dy	Pointer for Y position returned.
**	@param flag	Flag if surrounding must be free.
**	@return		True if place found, false if no found.
*/
local int AiFindBuildingPlace2(const Unit * worker, const UnitType * type,
	int ox, int oy, int *dx, int *dy,int flag)
{
    static const int xoffset[]={  0,-1,+1, 0, -1,+1,-1,+1 };
    static const int yoffset[]={ -1, 0, 0,+1, -1,-1,+1,+1 };
    struct {
	unsigned short X;
	unsigned short Y;
    } * points;
    int size;
    int x;
    int y;
    int rx;
    int ry;
    int mask;
    int wp;
    int rp;
    int ep;
    int i;
    int w;
    unsigned char* m;
    unsigned char* matrix;

    points=alloca(TheMap.Width*TheMap.Height);
    size=TheMap.Width*TheMap.Height/sizeof(*points);

    x=ox;
    y=oy;
    //
    //	Look if we can build at current place.
    //
    if (CanBuildUnitType(worker, type, x, y)
	    && (!flag || AiCheckSurrounding(worker,type, x, y)) ) {
	*dx=x;
	*dy=y;
	return 1;
    }

    //
    //	Make movement matrix.
    //
    matrix=CreateMatrix();
    w=TheMap.Width+2;

    mask=UnitMovementMask(worker);
    // Ignore all possible mobile units.
    mask&=~(MapFieldLandUnit|MapFieldAirUnit|MapFieldSeaUnit);

    points[0].X=x;
    points[0].Y=y;
    // also use the bottom right
    if( type->TileWidth>1 &&
	x+type->TileWidth-1<TheMap.Width && y+type->TileHeight-1<TheMap.Height ) {
	points[1].X=x+type->TileWidth-1;
	points[1].Y=y+type->TileWidth-1;
	ep=wp=2;				// start with two points
    } else {
	ep=wp=1;				// start with one point
    }
    matrix+=w+w+2;
    rp=0;
    matrix[x+y*w]=1;				// mark start point

    //
    //	Pop a point from stack, push all neightbors which could be entered.
    //
    for( ;; ) {
	while( rp!=ep ) {
	    rx=points[rp].X;
	    ry=points[rp].Y;
	    for( i=0; i<8; ++i ) {		// mark all neighbors
		x=rx+xoffset[i];
		y=ry+yoffset[i];
		m=matrix+x+y*w;
		if( *m ) {			// already checked
		    continue;
		}

		//
		//	Look if we can build here.
		//
		if (CanBuildUnitType(worker, type, x, y)
			&& (!flag || AiCheckSurrounding(worker,type, x, y)) ) {
		    *dx=x;
		    *dy=y;
		    return 1;
		}

		if( CanMoveToMask(x,y,mask) ) {	// reachable
		    *m=1;
		    points[wp].X=x;		// push the point
		    points[wp].Y=y;
		    if( ++wp>=size ) {		// round about
			wp=0;
		    }
		} else {			// unreachable
		    *m=99;
		}
	    }

	    if( ++rp>=size ) {			// round about
		rp=0;
	    }
	}

	//
	//	Continue with next frame.
	//
	if( rp==wp ) {			// unreachable, no more points available
	    break;
	}
	ep=wp;
    }

    return 0;
}

/**
**      Find building place for hall. (flood fill version)
**
**      The best place:
**              1) near to goldmine.
**              !2) near to wood.
**              !3) near to worker and must be reachable.
**              4) no enemy near it.
**              5) no hall already near
**              !6) enough gold in mine
**
**	@param worker	Worker to build building.
**	@param type	Type of building.
**	@param dx	Pointer for X position returned.
**	@param dy	Pointer for Y position returned.
**
**	@return		True if place found, false if not found.
**
**	@todo	FIXME: This is slow really slow, using two flood fills, is not
**		a perfect solution.
*/
local int AiFindHallPlace(const Unit * worker, const UnitType * type,
	int *dx, int *dy)
{
    static const int xoffset[]={  0,-1,+1, 0, -1,+1,-1,+1 };
    static const int yoffset[]={ -1, 0, 0,+1, -1,-1,+1,+1 };
    struct {
	unsigned short X;
	unsigned short Y;
    } * points;
    int size;
    int x;
    int y;
    int rx;
    int ry;
    int mask;
    int wp;
    int rp;
    int ep;
    int i;
    int w;
    unsigned char* m;
    unsigned char* morg;
    unsigned char* matrix;
    Unit* mine;
    int destx;
    int desty;

    destx=x=worker->X;
    desty=y=worker->Y;
    size=TheMap.Width*TheMap.Height/4;
    points=alloca(size*sizeof(*points));

    //
    //	Make movement matrix. FIXME: can create smaller matrix.
    //
    morg=MakeMatrix();
    w=TheMap.Width+2;
    matrix=morg+w+w+2;

    points[0].X=x;
    points[0].Y=y;
    rp=0;
    matrix[x+y*w]=1;			// mark start point
    ep=wp=1;				// start with one point

    mask=UnitMovementMask(worker);

    //
    //	Pop a point from stack, push all neighbors which could be entered.
    //
    for( ;; ) {
	while( rp!=ep ) {
	    rx=points[rp].X;
	    ry=points[rp].Y;
	    for( i=0; i<8; ++i ) {		// mark all neighbors
		x=rx+xoffset[i];
		y=ry+yoffset[i];
		m=matrix+x+y*w;
		if( *m ) {			// already checked
		    continue;
		}

		//
		//	Look if there is a mine
		//
		if ( mine=GoldMineOnMap(x,y) ) {
		    int buildings;
		    int j;
		    int minx;
		    int maxx;
		    int miny;
		    int maxy;
		    int nunits;
		    Unit* units[UnitMax];

		    buildings=0;

		    //
		    //  Check units around mine
		    //
		    minx=mine->X-5;
		    if( minx<0 ) minx=0;
		    miny=mine->Y-5;
		    if( miny<0 ) miny=0;
		    maxx=mine->X+mine->Type->TileWidth+5;
		    if( maxx>TheMap.Width ) maxx=TheMap.Width;
		    maxy=mine->Y+mine->Type->TileHeight+5;
		    if( maxy>TheMap.Height ) maxy=TheMap.Height;

		    nunits=SelectUnits(minx,miny,maxx,maxy,units);
		    for( j=0; j<nunits; ++j ) {
			// Enemy near mine
			if( AiPlayer->Player->Enemy&(1<<units[j]->Player->Player) ) {
			    break;
			}
			// Town hall near mine
			if( units[j]->Type->StoresGold ) {
			    break;
			}
			// Town hall may not be near but we may be using it, check
			// for 2 buildings near it and assume it's been used
			if( units[j]->Type->Building && !units[j]->Type->GoldMine ) {
			    ++buildings;
			    if( buildings==2 ) {
				break;
			    }
			}
		    }
		    if( j==nunits ) {
			if( AiFindBuildingPlace2(worker,type,x,y,dx,dy,0) ) {
			    free(morg);
			    return 1;
			}
		    }
		}

		if( CanMoveToMask(x,y,mask) ) {	// reachable
		    *m=1;
		    points[wp].X=x;		// push the point
		    points[wp].Y=y;
		    if( ++wp>=size ) {		// round about
			wp=0;
		    }
		} else {			// unreachable
		    *m=99;
		}
	    }
	    if( ++rp>=size ) {			// round about
		rp=0;
	    }
	}

	//
	//	Continue with next frame.
	//
	if( rp==wp ) {			// unreachable, no more points available
	    break;
	}
	ep=wp;
    }

    free(morg);
    return 0;
}

/**
**      Find free building place for lumber mill. (flood fill version)
**
**	@param worker	Worker to build building.
**	@param type	Type of building.
**	@param dx	Pointer for X position returned.
**	@param dy	Pointer for Y position returned.
**	@return		True if place found, false if not found.
**
**	@todo	FIXME: This is slow really slow, using two flood fills, is not
**		a perfect solution.
*/
local int AiFindLumberMillPlace(const Unit * worker, const UnitType * type,
	int *dx, int *dy)
{
    static const int xoffset[]={  0,-1,+1, 0, -1,+1,-1,+1 };
    static const int yoffset[]={ -1, 0, 0,+1, -1,-1,+1,+1 };
    struct {
	unsigned short X;
	unsigned short Y;
    } * points;
    int size;
    int x;
    int y;
    int rx;
    int ry;
    int mask;
    int wp;
    int rp;
    int ep;
    int i;
    int w;
    unsigned char* m;
    unsigned char* morg;
    unsigned char* matrix;

    x=worker->X;
    y=worker->Y;
    size=TheMap.Width*TheMap.Height/4;
    points=alloca(size*sizeof(*points));

    //
    //	Make movement matrix.
    //
    morg=MakeMatrix();
    w=TheMap.Width+2;
    matrix=morg+w+w+2;

    points[0].X=x;
    points[0].Y=y;
    rp=0;
    matrix[x+y*w]=1;			// mark start point
    ep=wp=1;				// start with one point

    mask=UnitMovementMask(worker);

    //
    //	Pop a point from stack, push all neightbors which could be entered.
    //
    for( ;; ) {
	while( rp!=ep ) {
	    rx=points[rp].X;
	    ry=points[rp].Y;
	    for( i=0; i<8; ++i ) {		// mark all neighbors
		x=rx+xoffset[i];
		y=ry+yoffset[i];
		m=matrix+x+y*w;
		if( *m ) {			// already checked
		    continue;
		}

		//
		//	Look if there is wood
		//
		if ( ForestOnMap(x,y) ) {
		    if( AiFindBuildingPlace2(worker,type,x,y,dx,dy,0) ) {
			free(morg);
			return 1;
		    }
		}

		if( CanMoveToMask(x,y,mask) ) {	// reachable
		    *m=1;
		    points[wp].X=x;		// push the point
		    points[wp].Y=y;
		    if( ++wp>=size ) {		// round about
			    wp=0;
		    }
		} else {			// unreachable
		    *m=99;
		}
	    }

	    if( ++rp>=size ) {			// round about
		    rp=0;
	    }
	}

	//
	//	Continue with next frame.
	//
	if( rp==wp ) {			// unreachable, no more points available
	    break;
	}
	ep=wp;
    }

    free(morg);
    return 0;
}

/**
**      Find free building place.
**
**	@param worker	Worker to build building.
**	@param type	Type of building.
**	@param dx	Pointer for X position returned.
**	@param dy	Pointer for Y position returned.
**	@return		True if place found, false if no found.
**
**	@todo	Better and faster way to find building place of oil platforms
**		Special routines for special buildings.
*/
global int AiFindBuildingPlace(const Unit * worker, const UnitType * type,
	int *dx, int *dy)
{

    //
    //	Find a good place for a new hall
    //
    if( type->StoresGold && AiFindHallPlace(worker,type,dx,dy) ) {
	return 1;
    }

    //
    //	Find a place near wood for a lumber mill
    //
    if( type->StoresWood && AiFindLumberMillPlace(worker,type,dx,dy) ) {
	return 1;
    }

    //
    //	Platforms can only be build on oil patches
    //
    if( !type->GivesOil && AiFindBuildingPlace2(worker,type,worker->X,worker->Y,dx,dy,1) ) {
	return 1;
    }

    // FIXME: Should do this if all units can't build better!
    return AiFindBuildingPlace2(worker,type,worker->X,worker->Y,dx,dy,0);

    // return 0;
}

//@}

#endif // } NEW_AI
