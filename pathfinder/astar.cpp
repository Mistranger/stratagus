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
/**@name astar.c	-	The a* path finder routines. */
//
//	(c) Copyright 1999-2003 by Lutz Sammer,Fabrice Rossi, Russell Smith
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
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"
#include "player.h"
#include "unit.h"

#include "pathfinder.h"

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

typedef struct _node_ {
    char	Direction;	/// Direction for trace back
    char	InGoal;         /// is this point in the goal
    int		CostFromStart;	/// Real costs to reach this point
} Node;

typedef struct _open_ {
    int		X;		/// X coordinate
    int		Y;		/// Y coordinate
    int		O;		/// Offset into matrix
    int		Costs;		/// complete costs to goal
} Open;

/// heuristic cost fonction for a star
#define AStarCosts(sx,sy,ex,ey) (abs(sx-ex)+abs(sy-ey))
// Other heuristic functions
// #define AStarCosts(sx,sy,ex,ey) 0
// #define AStarCosts(sx,sy,ex,ey) isqrt((abs(sx-ex)*abs(sx-ex))+(abs(sy-ey)*abs(sy-ey)))
// #define AStarCosts(sx,sy,ex,ey) max(abs(sx-ex),abs(sy-ey))
/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

//  Convert heading into direction.
//                            //  N NE  E SE  S SW  W NW
global const int Heading2X[9] = {  0,+1,+1,+1, 0,-1,-1,-1, 0 };
global const int Heading2Y[9] = { -1,-1, 0,+1,+1,+1, 0,-1, 0 };
global const int XY2Heading[3][3] = { {7,6,5},{0,0,4},{1,2,3}};
/// cost matrix
local Node *AStarMatrix;
/// a list of close nodes, helps to speed up the matrix cleaning
local int *CloseSet;
local int Threshold;
local int OpenSetMaxSize;
local int AStarMatrixSize;
#define MAX_CLOSE_SET_RATIO 4
#define MAX_OPEN_SET_RATIO 8	// 10,16 to small

/// see pathfinder.h
global int AStarFixedUnitCrossingCost=MaxMapWidth*MaxMapHeight;
global int AStarMovingUnitCrossingCost=2;
global int AStarOn=0;
global int AStarKnowUnknown=0;
global int AStarUnknownTerrainCost=100;

/**
**	The Open set is handled by a Heap stored in a table
**	0 is the root
**	node i left son is at 2*i+1 and right son is at 2*i+2
*/

/// The set of Open nodes
local Open *OpenSet;
/// The size of the open node set
local int OpenSetSize;

/**
**	Init A* data structures
*/
global void InitAStar(void)
{
    if( AStarOn ) {
	if( !AStarMatrix ) {
	    AStarMatrixSize=sizeof(Node)*TheMap.Width*TheMap.Height;
	    AStarMatrix=(Node *)calloc(TheMap.Width*TheMap.Height,sizeof(Node));
	    Threshold=TheMap.Width*TheMap.Height/MAX_CLOSE_SET_RATIO;
	    CloseSet=(int *)malloc(sizeof(int)*Threshold);
	    OpenSetMaxSize=TheMap.Width*TheMap.Height/MAX_OPEN_SET_RATIO;
	    OpenSet=(Open *)malloc(sizeof(Open)*OpenSetMaxSize);
	}
    }
}

/**
**	Free A* data structure
*/
global void FreeAStar(void)
{
    if( AStarOn ) {
	if( AStarMatrix ) {
	    free(AStarMatrix);
	    AStarMatrix=NULL;
	    free(CloseSet);
	    free(OpenSet);
	}
    }
}

/**
**	Prepare path finder.
*/
local void AStarPrepare(void)
{
    memset(AStarMatrix,0,AStarMatrixSize);
}

/**
**	Clean up the AStarMatrix
*/
local void AStarCleanUp(int num_in_close)
{
    int i;

    if( num_in_close>=Threshold ) {
	AStarPrepare();
    } else {
	for( i=0; i<num_in_close; ++i ) {
	  AStarMatrix[CloseSet[i]].CostFromStart=0;
	  AStarMatrix[CloseSet[i]].InGoal=0;
	}
    }
}

/**
**	Find the best node in the current open node set
**	Returns the position of this node in the open node set (always 0 in the
**	current heap based implementation)
*/
#define AStarFindMinimum() 0
#if 0
local int AStarFindMinimum()
{
    return 0;
}
#endif

/**
**	Remove the minimum from the open node set (and update the heap)
**	pos is the position of the minimum (0 in the heap based implementation)
*/
local void AStarRemoveMinimum(int pos)
{
    int i;
    int j;
    int end;
    Open swap;

    if( --OpenSetSize ) {
	OpenSet[pos]=OpenSet[OpenSetSize];
	// now we exchange the new root with its smallest child until the
	// order is correct
	i=0;
	end=(OpenSetSize>>1)-1;
	while( i<=end ) {
	    j=(i<<1)+1;
	    if( j<OpenSetSize-1 && OpenSet[j].Costs>=OpenSet[j+1].Costs ) {
		++j;
	    }
	    if( OpenSet[i].Costs>OpenSet[j].Costs ) {
		swap=OpenSet[i];
		OpenSet[i]=OpenSet[j];
		OpenSet[j]=swap;
		i=j;
	    } else {
		break;
	    }
	}
    }
}

/**
**	Add a new node to the open set (and update the heap structure)
**	Returns Pathfinder failed
*/
local int AStarAddNode(int x,int y,int o,int costs)
{
    int i;
    int j;
    Open swap;

    i=OpenSetSize;
    if( OpenSetSize>=OpenSetMaxSize ) {
	fprintf(stderr, "A* internal error: raise Open Set Max Size "
		"(current value %d)\n",OpenSetMaxSize);
	return PF_FAILED;
    }
    OpenSet[i].X=x;
    OpenSet[i].Y=y;
    OpenSet[i].O=o;
    OpenSet[i].Costs=costs;
    OpenSetSize++;
    while( i>0 ) {
	j=(i-1)>>1;
	if( OpenSet[i].Costs<OpenSet[j].Costs ) {
	    swap=OpenSet[i];
	    OpenSet[i]=OpenSet[j];
	    OpenSet[j]=swap;
	    i=j;
	} else {
	    break;
	}
    }

    return 0;
}

/**
**	Change the cost associated to an open node. The new cost MUST BE LOWER
**	than the old one in the current heap based implementation.
*/
local void AStarReplaceNode(int pos,int costs)
{
    int i;
    int j;
    Open swap;

    i=pos;
    OpenSet[pos].Costs=costs;
    // we need to go up, as the cost can only decrease
    while( i>0 ) {
	j=(i-1)>>1;
	if( OpenSet[i].Costs<OpenSet[j].Costs ) {
	    swap=OpenSet[i];
	    OpenSet[i]=OpenSet[j];
	    OpenSet[j]=swap;
	    i=j;
	} else {
	    break;
	}
    }
}


/**
**	Check if a node is already in the open set.
**	Return -1 if not found and the position of the node in the table if found.
*/
local int AStarFindNode(int eo)
{
    int i;

    for( i=0; i<OpenSetSize; ++i ) {
	if( OpenSet[i].O==eo ) {
	    return i;
	}
    }
    return -1;
}

/**
**	Compute the cost of crossing tile (dx,dy)
**	-1 -> impossible to cross
**	 0 -> no induced cost, except move
**	>0 -> costly tile
*/
local int CostMoveTo(Unit* unit, int ex,int ey,int mask,int current_cost) {
    int j;
    int cost;
    Unit* goal;

    cost=0;
    j=TheMap.Fields[ex+ey*TheMap.Width].Flags&mask;
    if( j && (AStarKnowUnknown
	    || IsMapFieldExplored(unit->Player,ex,ey)) ) {
	if( j&~(MapFieldLandUnit|MapFieldAirUnit|MapFieldSeaUnit) ) {
	    // we can't cross fixed units and other unpassable things
	    return -1;
	}
	goal=UnitCacheOnXY(ex,ey,unit->Type->UnitType);
	if( !goal ) {
	    // Shouldn't happen, mask says there is something on this tile
	    DebugCheck( 1 );
	    return -1;
	}
	if( goal->Moving ) {
	    // moving unit are crossable
	    cost+=AStarMovingUnitCrossingCost;
	} else {
	    // for non moving unit Always Fail
	    // FIXME: Need support for moving a fixed unit to add cost
	    return -1;
	    cost+=AStarFixedUnitCrossingCost;
	}
    }
    // Add cost of crossing unknown tiles if required
    if( !AStarKnowUnknown && !IsMapFieldExplored(unit->Player,ex,ey) ) {
	// Tend against unknown tiles.
	cost+=AStarUnknownTerrainCost;
    }
    return cost;
}

/**
**	MarkAStarGoal
*/
local int AStarMarkGoal(Unit* unit, int gx, int gy, int gw, int gh, int range, int mask, int* num_in_close)
{
    int cx[4]; 
    int cy[4];
    int steps;
    int cycle;
    int x;
    int y;
    int goal_reachable;
    int quad;
    int eo;
    int filler;

    goal_reachable=0;

    if( range == 0 && gw == 0 && gh == 0 ) {
	if( CostMoveTo(unit,gx,gy,mask,AStarFixedUnitCrossingCost)>=0 ) {
	    AStarMatrix[gx+gy*TheMap.Width].InGoal=1;
	    return 1;
	} else {
	    return 0;
	}
    }

    // Mark top, bottom, left, right

    // Mark Top and Bottom of Goal
    for(x=gx;x<=gx+gw;x++) {
	if( x >= 0 && x < TheMap.Width) {
	    if( gy-range >= 0 && CostMoveTo(unit,x,gy-range,mask,AStarFixedUnitCrossingCost)>=0 ) {
		AStarMatrix[(gy-range)*TheMap.Width+x].InGoal=1;
		goal_reachable=1;
		if( *num_in_close<Threshold ) {
                   CloseSet[(*num_in_close)++]=(gy-range)*TheMap.Width+x;
		}						    
	    }
	    if( gy+range+gh < TheMap.Height && CostMoveTo(unit,x,gy+gh+range,mask,AStarFixedUnitCrossingCost)>=0 ) {
		AStarMatrix[(gy+range+gh)*TheMap.Width+x].InGoal=1;
		if( *num_in_close<Threshold ) {
		    CloseSet[(*num_in_close)++]=(gy+range+gh)*TheMap.Width+x;
		}
		goal_reachable=1;
	    }
	}
    }

    for(y=gy;y<=gy+gh;y++) {
	if( y >= 0 && y < TheMap.Height) {
	    if( gx-range >=0 && CostMoveTo(unit,gx-range,y,mask,AStarFixedUnitCrossingCost)>=0 ) {
		AStarMatrix[y*TheMap.Width+gx-range].InGoal=1;
		if( *num_in_close<Threshold ) {
		    CloseSet[(*num_in_close)++]=y*TheMap.Width+gx-range;
		}
		goal_reachable=1;
	    }
	    if( gx+gw+range < TheMap.Width && CostMoveTo(unit,gx+gw+range,y,mask,AStarFixedUnitCrossingCost)>=0 ) {
		AStarMatrix[y*TheMap.Width+gx+gw+range].InGoal=1;
		if( *num_in_close<Threshold ) {
		    CloseSet[(*num_in_close)++]=y*TheMap.Width+gx+gw+range;
		}
		goal_reachable=1;
	    }
	}
    }

    // Mark Goal Border in Matrix

    // Mark Edges of goal

    steps=0;
    // Find place to start. (for marking curves)
    while(VisionTable[0][steps] != range && VisionTable[1][steps] == 0 && VisionTable[2][steps] == 0) {
	steps++;
    }
    // 0 - Top right Quadrant
    cx[0] = gx+gw;
    cy[0] = gy-VisionTable[0][steps];
    // 1 - Top left Quadrant
    cx[1] = gx;
    cy[1] = gy-VisionTable[0][steps];
    // 2 - Bottom Left Quadrant
    cx[2] = gx;
    cy[2] = gy+VisionTable[0][steps]+gh;
    // 3 - Bottom Right Quadrant
    cx[3] = gx+gw;
    cy[3] = gy+VisionTable[0][steps]+gh;

    steps++;  // Move past blank marker
    while(VisionTable[1][steps] != 0 || VisionTable[2][steps] != 0 ) {
	// Loop through for repeat cycle
	cycle=0;
	while( cycle++ < VisionTable[0][steps] ) {
	    // If we travelled on an angle, mark down as well.
	    if( VisionTable[1][steps] == VisionTable[2][steps] ) {
		// do down
		quad = 0;
		while( quad < 4 ) {
		    if( quad < 2 ) {
			filler=1;
		    } else {
			filler=-1;
		    }
		    if( cx[quad] >= 0 && cx[quad] < TheMap.Width && cy[quad]+filler >= 0 && 
			cy[quad]+filler < TheMap.Height &&
			CostMoveTo(unit,cx[quad],cy[quad]+filler,mask,AStarFixedUnitCrossingCost)>=0 ) {
			eo=(cy[quad]+filler)*TheMap.Width+cx[quad];
			AStarMatrix[eo].InGoal=1;
			if( *num_in_close<Threshold ) {
			    CloseSet[(*num_in_close)++]=eo;
			}
			goal_reachable=1;
		    }
		    quad++;
		}
	    }
		
	    cx[0]+=VisionTable[1][steps];
	    cy[0]+=VisionTable[2][steps];
	    cx[1]-=VisionTable[1][steps];
	    cy[1]+=VisionTable[2][steps];
	    cx[2]-=VisionTable[1][steps];
	    cy[2]-=VisionTable[2][steps];
	    cx[3]+=VisionTable[1][steps];
	    cy[3]-=VisionTable[2][steps];
	    
	    // Mark Actually Goal curve change
	    quad = 0;
	    while( quad < 4 ) {
		if( cx[quad] >= 0 && cx[quad] < TheMap.Width && cy[quad] >= 0 &&
		    cy[quad] < TheMap.Height &&
		    CostMoveTo(unit,cx[quad],cy[quad],mask,AStarFixedUnitCrossingCost)>=0 ) {
		    eo=cy[quad]*TheMap.Width+cx[quad];
		    AStarMatrix[eo].InGoal=1;
		    if( *num_in_close<Threshold ) {
			CloseSet[(*num_in_close)++]=eo;
		    }
		    goal_reachable=1;
		}
		quad++;
	    }
	}
	steps++;
    }
    return goal_reachable;
}
/**
**	Find path.
*/
global int AStarFindPath(Unit* unit, int gx, int gy, int gw, int gh, int range, char* path)
{
    int i;
    int j;
    int o;
    int ex;
    int ey;
    int eo;
    int x;
    int y;
    int px;
    int py;
    int shortest;
    int counter;
    int new_cost;
    int cost_to_goal;
    int path_length;
    int num_in_close;
    int mask;

    DebugLevel3Fn("%d %d,%d->%d,%d\n" _C_
	    UnitNumber(unit) _C_
	    unit->X _C_ unit->Y _C_ x _C_ y);

    OpenSetSize=0;
    num_in_close=0;
    mask=UnitMovementMask(unit);
    x=unit->X;
    y=unit->Y;

    // if goal is not directory reachable, punch out
    if( !AStarMarkGoal(unit, gx, gy, gw, gh, range, mask, &num_in_close) ) {
	AStarCleanUp(num_in_close);
	return PF_UNREACHABLE;
    }

    eo=y*TheMap.Width+x;
    // it is quite important to start from 1 rather than 0, because we use
    // 0 as a way to represent nodes that we have not visited yet.
    AStarMatrix[eo].CostFromStart=1;
    // 8 to say we are came from nowhere.
    AStarMatrix[eo].Direction=8;

    // place start point in open, it that failed, try another pathfinder
    if( AStarAddNode(x,y,eo,1+AStarCosts(x,y,gx,gy)) == PF_FAILED ) {
	AStarCleanUp(num_in_close);
	return PF_FAILED;
    }
    if( num_in_close<Threshold ) {
	CloseSet[num_in_close++]=OpenSet[0].O;
    }
    if( AStarMatrix[eo].InGoal ) {
	AStarCleanUp(num_in_close);
	return PF_REACHED;
    }

    counter=TheMap.Width*TheMap.Height;

    while( 1 ) {
	//
	//	Find the best node of from the open set
	//
	shortest=AStarFindMinimum();
	x=OpenSet[shortest].X;
	y=OpenSet[shortest].Y;
	o=OpenSet[shortest].O;
	cost_to_goal=OpenSet[shortest].Costs-AStarMatrix[o].CostFromStart;

	AStarRemoveMinimum(shortest);

	//
	//	If we have reached the goal, then exit.
	if( AStarMatrix[o].InGoal==1 ) {
	    ex=x;
	    ey=y;
	    DebugLevel3Fn("a star goal reached\n");
	    break;
	}

	//
	//	If we have looked too long, then exit.
	//
	if( !counter-- ) {
	    //
	    //	Select a "good" point from the open set.
	    //		Nearest point to goal.
	    DebugLevel0Fn("%d way too long\n" _C_ UnitNumber(unit));
	    AStarCleanUp(num_in_close);
	    return PF_FAILED;
	}

	DebugLevel3("Best point in Open Set: %d %d (%d)\n" _C_ x _C_ y _C_ OpenSetSize);
	//
	//	Generate successors of this node.

	// Node that this node was generated from.
	px=x-Heading2X[(int)AStarMatrix[x+TheMap.Width*y].Direction];
	py=y-Heading2Y[(int)AStarMatrix[x+TheMap.Width*y].Direction];

	for( i=0; i<8; ++i ) {
	    ex=x+Heading2X[i];
	    ey=y+Heading2Y[i];

	    // Don't check the tile we came from, it's not going to be better
	    // Should reduce load on A*
	    
	    if( ex==px && ey==py ) {
		continue;
	    }
	    //
	    //	Outside the map or can't be entered.
	    //
	    if( ex<0 || ex>=TheMap.Width ) {
		continue;
	    }
	    if( ey<0 || ey>=TheMap.Height ) {
		continue;
	    }
	    // if the point is "move to"-able an
	    // if we have not reached this point before,
	    // or if we have a better path to it, we add it to open set
	    new_cost=CostMoveTo(unit,ex,ey,mask,AStarMatrix[o].CostFromStart);
	    if( new_cost==-1 ) {
		// uncrossable tile
		continue;
	    }

	    // Add a cost for walking to make paths more realistic for the user.
	    new_cost+=abs(Heading2X[i])+abs(Heading2Y[i])+1;
	    eo=ey*TheMap.Width+ex;
	    new_cost+=AStarMatrix[o].CostFromStart;
	    if( AStarMatrix[eo].CostFromStart==0 ) {
		// we are sure the current node has not been already visited
		AStarMatrix[eo].CostFromStart=new_cost;
		AStarMatrix[eo].Direction=i;
		if( AStarAddNode(ex,ey,eo,AStarMatrix[eo].CostFromStart+AStarCosts(ex,ey,gx,gy)) == PF_FAILED ) {
		    AStarCleanUp(num_in_close);
		    DebugLevel3Fn("Tiles Visited: %d\n" _C_ (TheMap.Height*TheMap.Width)-counter);
		    return PF_FAILED;
		}
		// we add the point to the close set
		if( num_in_close<Threshold ) {
		    CloseSet[num_in_close++]=eo;
		}
	    } else if( new_cost<AStarMatrix[eo].CostFromStart ) {
		// Already visited node, but we have here a better path
		// I know, it's redundant (but simpler like this)
		AStarMatrix[eo].CostFromStart=new_cost;
		AStarMatrix[eo].Direction=i;
		// this point might be already in the OpenSet
		j=AStarFindNode(eo);
		if( j==-1 ) {
		    if( AStarAddNode(ex,ey,eo,
				 AStarMatrix[eo].CostFromStart+
				 AStarCosts(ex,ey,gx,gy)) == PF_FAILED ) {
			AStarCleanUp(num_in_close);
			DebugLevel3Fn("Tiles Visited: %d\n" _C_ (TheMap.Height*TheMap.Width)-counter);
			return PF_FAILED;
		    }
		} else {
		    AStarReplaceNode(j,AStarMatrix[eo].CostFromStart+
				     AStarCosts(ex,ey,gx,gy));
		}
		// we don't have to add this point to the close set
	    }
	}
	if( OpenSetSize<=0 ) {		// no new nodes generated
	    DebugLevel3Fn("%d unreachable\n" _C_ UnitNumber(unit));
	    DebugLevel3Fn("Tiles Visited: %d\n" _C_ (TheMap.Height*TheMap.Width)-counter);
	    AStarCleanUp(num_in_close);
	    return PF_UNREACHABLE;
	}
    }
    // now we need to backtrack
    path_length=0;
    x=unit->X;
    y=unit->Y;
    gx=ex;
    gy=ey;
    i=0;
    while( ex!=x || ey!=y ) {
	eo=ey*TheMap.Width+ex;
	i=AStarMatrix[eo].Direction;
	ex-=Heading2X[i];
	ey-=Heading2Y[i];
	path_length++;
    }

    // gy = Path length to cache
    // gx = Current place in path
    ex=gx;
    ey=gy;
    gy=path_length;
    gx=path_length;
    if( gy>MAX_PATH_LENGTH ) {
	gy=MAX_PATH_LENGTH;
    }

    // Now we have the length, calculate the cached path.
    while( (ex!=x || ey!=y) && path!=NULL ) {
	eo=ey*TheMap.Width+ex;
	i=AStarMatrix[eo].Direction;
	DebugLevel3("%d %d %d %d (%d,%d)\n" _C_ x _C_ y _C_ ex _C_ ey _C_ Heading2X[i] _C_ Heading2Y[i]);
	ex-=Heading2X[i];
	ey-=Heading2Y[i];
	--gx;
        if( gx<gy ) {
	    path[gy-gx-1]=i;
	} 
    }

    // let's clean up the matrix now
    AStarCleanUp(num_in_close);
    DebugLevel3Fn("Tiles Visited: %d\n" _C_ (TheMap.Height*TheMap.Width)-counter);
    return path_length;
}

/**
**	Returns the next element of a path.
**
**	@param unit	Unit that wants the path element.
**	@param pxd	Pointer for the x direction.
**	@param pyd	Pointer for the y direction.
**
**	@return		>0 remaining path length, 0 wait for path, -1
**			reached goal, -2 can't reach the goal.
*/
global int NextPathElement(Unit* unit,int* pxd,int *pyd)
{
    int result;

    // Attempt to use path cache
    // FIXME: If there is a goal, it may have moved, ruining the cache
    *pxd=0;
    *pyd=0;

    if( unit->Data.Move.Length <= 0 ) {
        result=NewPath(unit);
        if( result==PF_UNREACHABLE ) {
	    unit->Data.Move.Length=0;
	    return result;
	}
	if( result==PF_REACHED ) {
	    return result;
	}
    }
    *pxd=Heading2X[(int)unit->Data.Move.Path[(int)unit->Data.Move.Length-1]];
    *pyd=Heading2Y[(int)unit->Data.Move.Path[(int)unit->Data.Move.Length-1]];
    result=unit->Data.Move.Length;
    unit->Data.Move.Length--;
    if( !CheckedCanMoveToMask(*pxd+unit->X,*pyd+unit->Y,UnitMovementMask(unit)) ) {
	result=NewPath(unit);
	if( result>0 ) {
	    *pxd=Heading2X[(int)unit->Data.Move.Path[(int)unit->Data.Move.Length-1]];
	    *pyd=Heading2Y[(int)unit->Data.Move.Path[(int)unit->Data.Move.Length-1]];
	    if( !CheckedCanMoveToMask(*pxd+unit->X,*pyd+unit->Y,UnitMovementMask(unit)) ) {
		// There may be unit in the way, Astar may allow you to walk onto it.
		result=PF_UNREACHABLE;
		*pxd=0;
		*pyd=0;
	    } else {
		result=unit->Data.Move.Length;
		unit->Data.Move.Length--;
	    }
	}
    }
    return result;
}

//@}
