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
/**@name mainloop.c	-	The main game loop. */
//
//	(c) Copyright 1998-2003 by Lutz Sammer and Jimmy Salmon
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

//----------------------------------------------------------------------------
//	Includes
//----------------------------------------------------------------------------

#include <stdio.h>
#if defined(DEBUG) && defined(HIERARCHIC_PATHFINDER)
#include <stdlib.h>
#include <setjmp.h>
#endif

#include "freecraft.h"
#include "video.h"
#include "tileset.h"
#include "map.h"
#include "font.h"
#include "sound_id.h"
#include "unitsound.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "cursor.h"
#include "minimap.h"
#include "actions.h"
#include "missile.h"
#include "interface.h"
#include "menus.h"
#include "network.h"
#include "ui.h"
#include "unit.h"
#include "deco.h"
#include "trigger.h"
#include "campaign.h"
#include "sound_server.h"
#include "settings.h"
#include "commands.h"
#include "cdaudio.h"

#ifdef USE_SDLCD
#include "SDL.h"
#include "SDL_thread.h"
#endif

//----------------------------------------------------------------------------
//	Variables
//----------------------------------------------------------------------------

    /// variable set when we are scrolling via keyboard
global enum _scroll_state_ KeyScrollState=ScrollNone;

    /// variable set when we are scrolling via mouse
global enum _scroll_state_ MouseScrollState=ScrollNone;

#if defined(DEBUG) && defined(HIERARCHIC_PATHFINDER)
global jmp_buf MainLoopJmpBuf;		/// Hierarchic pathfinder error exit.
#endif

global EventCallback* Callbacks;	/// Current callbacks
global EventCallback GameCallbacks;	/// Game callbacks
global EventCallback MenuCallbacks;	/// Menu callbacks

//----------------------------------------------------------------------------
//	Functions
//----------------------------------------------------------------------------

/**
**	Move map view point up (north).
**
**	@param step	How many tiles.
*/
local void MoveMapViewPointUp(int step)
{
    Viewport* vp;

    vp=TheUI.SelectedViewport;
    if( vp->MapY>step ) {
	vp->MapY-=step;
    } else {
	vp->MapY=0;
    }
}

/**
**	Move map view point left (west).
**
**	@param step	How many tiles.
*/
local void MoveMapViewPointLeft(int step)
{
    Viewport* vp;

    vp=TheUI.SelectedViewport;
    if( vp->MapX>step ) {
	vp->MapX-=step;
    } else {
	vp->MapX=0;
    }
}

/**
**	Move map view point down (south).
**
**	@param step	How many tiles.
*/
local void MoveMapViewPointDown(int step)
{
    Viewport* vp;

    vp=TheUI.SelectedViewport;
    if( TheMap.Height>vp->MapHeight
	    && vp->MapY<TheMap.Height-vp->MapHeight-step ) {
	vp->MapY+=step;
    } else {
	vp->MapY=TheMap.Height-vp->MapHeight;
    }
}

/**
**	Move map view point right (east).
**
**	@param step	How many tiles.
*/
local void MoveMapViewPointRight(int step)
{
    Viewport* vp;

    vp=TheUI.SelectedViewport;
    if( TheMap.Width>vp->MapWidth
	    && vp->MapX<TheMap.Width-vp->MapWidth-step) {
	vp->MapX+=step;
    } else {
	vp->MapX=TheMap.Width-vp->MapWidth;
    }
}

/**
**	Handle scrolling area.
**
**	@param state	Scroll direction/state.
**	@param fast	Flag scroll faster.
**
**	@todo	Support dynamic acceleration of scroll speed.
**	@todo	If the scroll key is longer pressed the area is scrolled faster.
**	@todo	Scrolling pixel wise.
**
**	StephanR: above needs one row+column of tiles extra to be
**		drawn (clipped), which also needs to be supported
**		by various functions using MustRedrawTile,..
*/
global void DoScrollArea(enum _scroll_state_ state, int fast)
{
    int stepx;
    int stepy;

    if( fast ) {
	stepx=TheUI.SelectedViewport->MapWidth/2;
	stepy=TheUI.SelectedViewport->MapHeight/2;
    } else {		// dynamic: let these variables increase upto fast..
	stepx=stepy=1;
    }

    switch( state ) {
	case ScrollUp:
	    MoveMapViewPointUp(stepy);
	    break;
	case ScrollDown:
	    MoveMapViewPointDown(stepy);
	    break;
	case ScrollLeft:
	    MoveMapViewPointLeft(stepx);
	    break;
	case ScrollLeftUp:
	    MoveMapViewPointLeft(stepx);
	    MoveMapViewPointUp(stepy);
	    break;
	case ScrollLeftDown:
	    MoveMapViewPointLeft(stepx);
	    MoveMapViewPointDown(stepy);
	    break;
	case ScrollRight:
	    MoveMapViewPointRight(stepx);
	    break;
	case ScrollRightUp:
	    MoveMapViewPointRight(stepx);
	    MoveMapViewPointUp(stepy);
	    break;
	case ScrollRightDown:
	    MoveMapViewPointRight(stepx);
	    MoveMapViewPointDown(stepy);
	    break;
	default:
	    return;			// skip marking map
    }
    HandleMouseMove(CursorX,CursorY);	// This recalulates some values
    MarkDrawEntireMap();
    MustRedraw|=RedrawMinimap|RedrawCursors;
}

#ifdef DEBUG	// {

/**
**	FOR DEBUG PURPOSE ONLY, BUT DON'T REMOVE PLEASE !!!
**
**	Will try all kinds of possible linedraw routines (only one time) upon
**	current display, making the job of debugging them more eassier..
*/
global void DebugTestDisplay(void)
{
    static int a=0;
    extern void DebugTestDisplayLines(void);

    if( a ) {
	return;
    }
    a=1;

    //Enable one function-call as type of test to show one time
    DebugTestDisplayLines();
    //DebugTestDisplayVarious();
    //DebugTestDisplayColorCube();

    // put it all on screen (when it is not already there ;)
    InvalidateArea(0,0,VideoWidth,VideoHeight);
}

#endif	// } DEBUG

/**
**	Draw menu button area.
**
**	With debug it shows the used frame time and arrival of network packets.
**
**	@todo	Must be more configurable. Adding diplomacy menu here?
*/
local void DrawMenuButtonArea(void)
{
    if (TheUI.MenuButtonGraphic.Graphic) {
	VideoDrawSub(TheUI.MenuButtonGraphic.Graphic,0,0,
		TheUI.MenuButtonGraphic.Graphic->Width,
		TheUI.MenuButtonGraphic.Graphic->Height,
		TheUI.MenuButtonGraphicX, TheUI.MenuButtonGraphicY);
    }
    if( NetworkFildes==-1 ) {
	if( TheUI.MenuButton.X!=-1 ) {
	    DrawMenuButton(TheUI.MenuButton.Button,
		    (ButtonAreaUnderCursor==ButtonAreaMenu
			&& ButtonUnderCursor==ButtonUnderMenu ? MenuButtonActive : 0) |
		    (GameMenuButtonClicked ? MenuButtonClicked : 0),
		    TheUI.MenuButton.Width, TheUI.MenuButton.Height,
		    TheUI.MenuButton.X,TheUI.MenuButton.Y,
		    GameFont,TheUI.MenuButton.Text,NULL,NULL);
	}
    } else {
	if( TheUI.NetworkMenuButton.X!=-1 ) {
	    DrawMenuButton(TheUI.NetworkMenuButton.Button,
		    (ButtonAreaUnderCursor==ButtonAreaMenu
			&& ButtonUnderCursor==ButtonUnderNetworkMenu ? MenuButtonActive : 0) |
		    (GameMenuButtonClicked ? MenuButtonClicked : 0),
		    TheUI.NetworkMenuButton.Width, TheUI.NetworkMenuButton.Height,
		    TheUI.NetworkMenuButton.X,TheUI.NetworkMenuButton.Y,
		    GameFont,TheUI.NetworkMenuButton.Text,NULL,NULL);
	}
	if( TheUI.NetworkDiplomacyButton.X!=-1 ) {
	    DrawMenuButton(TheUI.NetworkDiplomacyButton.Button,
		    (ButtonAreaUnderCursor==ButtonAreaMenu
			&& ButtonUnderCursor==ButtonUnderNetworkDiplomacy ? MenuButtonActive : 0) |
		    (GameDiplomacyButtonClicked ? MenuButtonClicked : 0),
		    TheUI.NetworkDiplomacyButton.Width, TheUI.NetworkDiplomacyButton.Height,
		    TheUI.NetworkDiplomacyButton.X,TheUI.NetworkDiplomacyButton.Y,
		    GameFont,TheUI.NetworkDiplomacyButton.Text,NULL,NULL);
	}
    }

#ifdef DRAW_DEBUG
    //
    //	Draw line for frame speed.
    //
    { int f;

    f=168*(NextFrameTicks-GetTicks());
    if( VideoSyncSpeed ) {
	f/=(100*1000/CYCLES_PER_SECOND)/VideoSyncSpeed;
    }
    if( f<0 || f>168 ) {
	f=168;
    }
    if( f ) {
	VideoDrawHLine(ColorGreen,TheUI.MenuButtonX,TheUI.MenuButtonY,f);
    }
    if( 168-f ) {
	VideoDrawHLine(ColorRed,TheUI.MenuButtonX+f,TheUI.MenuButtonY,168-f);
    }
    }
    //
    //	Draw line for network speed.
    //
    {
    int i;
    int f;

    if( NetworkLag ) {
	for( i=0; i<PlayerMax; ++i ) {
	    f=16-(16*(NetworkStatus[i]-GameCycle))/(NetworkLag*2);
	    if( f<0 || f>16 ) {
		f=16;
	    }
	    if( f ) {
		VideoDrawHLine(ColorRed,
		    TheUI.MenuButtonX,TheUI.MenuButtonY+1+i,f);
	    }
	    if( 16-f ) {
		VideoDrawHLine(ColorGreen,
			TheUI.MenuButtonX+f,TheUI.MenuButtonY+1+i,16-f);
	    }
	}
    }
    }
#endif
}

/**
**	Draw a map viewport.
**
**	@param vp	Viewport pointer.
**
**	@note	Johns: I think parsing the viewport pointer is faster.
*/
local void DrawMapViewport(Viewport* vp)
{
   Unit* table[UnitMax];
   Missile* missiletable[MAX_MISSILES*9];
   int nunits;
   int nmissiles;
   int i;
   int j;
   int x;
   int y;

#ifdef NEW_DECODRAW
    // Experimental new drawing mechanism, which can keep track of what is
    // overlapping and draw only that what has changed..
    // Every to-be-drawn item added to this mechanism, can be handed by this
    // call.
    if( InterfaceState==IfaceStateNormal ) {
	// DecorationRefreshDisplay();
	DecorationUpdateDisplay();
    }

#else
    if( InterfaceState==IfaceStateNormal ) {
#ifdef NEW_MAPDRAW
	MapUpdateFogOfWar(vp->MapX, vp->MapY);
#else
	int u;

	// FIXME: Johns: this didn't work correct with viewports!
	// FIXME: only needed until flags are correct set
	for( u=0; u < vp->MapHeight; ++u ) {
	    MustRedrawRow[u]=1;
	}
	for (u=0; u<vp->MapHeight*vp->MapWidth; ++u ) {
	    MustRedrawTile[u]=1;
	}
#endif
	//
	//	An unit is tracked, center viewport on this unit.
	//
	if( vp->Unit ) {
	    if( vp->Unit->Destroyed ||
		    vp->Unit->Orders[0].Action==UnitActionDie ) {
		vp->Unit=NoUnitP;
	    } else {
		ViewportCenterViewpoint(vp,vp->Unit->X,vp->Unit->Y);
	    }
	}

	SetClipping(vp->X,vp->Y,vp->EndX,vp->EndY);

	DrawMapBackgroundInViewport(vp,vp->MapX,vp->MapY);
	nunits=FindAndSortUnits(vp,table);
	nmissiles=FindAndSortMissiles(vp,missiletable);

	i=0;
	j=0;
	CurrentViewport=vp;
	while( i<nunits && j<nmissiles ) {
	    if( table[i]->Type->DrawLevel <= missiletable[j]->Type->DrawLevel ) {
		if( UnitVisibleInViewport(vp,table[i]) ) {
		    if( table[i]->Type->Building ) {
			DrawBuilding(table[i]);
		    } else {
			DrawUnit(table[i]);
		    }
		}
		i++;
	    } else {
		x = missiletable[j]->X - vp->MapX * TileSizeX + vp->X;
		y = missiletable[j]->Y - vp->MapY * TileSizeY + vp->Y;
		// FIXME: I should copy SourcePlayer for second level missiles.
		if( missiletable[j]->SourceUnit && missiletable[j]->SourceUnit->Player ) {
		    GraphicPlayerPixels(missiletable[j]->SourceUnit->Player,
		    	missiletable[j]->Type->Sprite);
		}
		switch( missiletable[j]->Type->Class ) {
		    case MissileClassHit:
			VideoDrawNumberClip(x,y,GameFont,missiletable[j]->Damage);
			break;
		    default:
			DrawMissile(missiletable[j]->Type,missiletable[j]->SpriteFrame,x,y);
			break;
		}
		j++;
	    }
	}
	for( ; i<nunits; ++i ) {
	    if ( UnitVisibleInViewport(vp, table[i]) ) {
		if( table[i]->Type->Building ) {
		    DrawBuilding(table[i]);
		} else {
		    DrawUnit(table[i]);
		}
	    }
	}
	for( ; j<nmissiles; ++j ) {
	    x = missiletable[j]->X - vp->MapX * TileSizeX + vp->X;
	    y = missiletable[j]->Y - vp->MapY * TileSizeY + vp->Y;
	    // FIXME: I should copy SourcePlayer for second level missiles.
	    if( missiletable[j]->SourceUnit && missiletable[j]->SourceUnit->Player ) {
		GraphicPlayerPixels(missiletable[j]->SourceUnit->Player,
		    missiletable[j]->Type->Sprite);
	    }
	    switch( missiletable[j]->Type->Class ) {
		case MissileClassHit:
		    VideoDrawNumberClip(x,y,GameFont,missiletable[j]->Damage);
		    break;
		default:
		    DrawMissile(missiletable[j]->Type,missiletable[j]->SpriteFrame,x,y);
		    break;
	    }
	}
	DrawMapFogOfWar(vp, vp->MapX, vp->MapY);
	DrawConsole();
	SetClipping(0,0,VideoWidth-1,VideoHeight-1);
    }

    // Resources over map!
    // FIXME: trick17! must find a better solution
    // FIXME: must take resource end into account
    if( TheUI.MapArea.X<=TheUI.ResourceX && TheUI.MapArea.EndX>=TheUI.ResourceX
	    && TheUI.MapArea.Y<=TheUI.ResourceY
	    && TheUI.MapArea.EndY>=TheUI.ResourceY ) {
	MustRedraw|=RedrawResources;
    }
#endif
}

/**
**	Draw map area
**
**	@todo	Fix the FIXME's and we only need to draw a line between the
**		viewports and show the active viewport.
*/
global void DrawMapArea(void)
{
    Viewport* vp;
    const Viewport* evp;

    // Draw all map viewports
    evp=TheUI.Viewports+TheUI.NumViewports;
    for( vp=TheUI.Viewports; vp<evp; ++vp) {
	DrawMapViewport(vp);
    }

    // if we a single viewport, no need to denote the "selected" one
    if( TheUI.NumViewports==1 ) {
	return;
    }

    //
    //	Separate the viewports and mark the active viewport.
    //
    for( vp=TheUI.Viewports; vp<evp; ++vp ) {
	enum _sys_colors_ color;

	if( vp==TheUI.SelectedViewport ) {
	    color=ColorOrange;
	} else {
	    color=ColorBlack;
	}

	// -
	VideoDrawLine(color, vp->X, vp->Y, vp->EndX, vp->Y);
	// |
	VideoDrawLine(color, vp->X, vp->Y + 1, vp->X, vp->EndY);
	// -
	VideoDrawLine(color, vp->X + 1, vp->EndY, vp->EndX - 1, vp->EndY);
	// |
	VideoDrawLine(color, vp->EndX, vp->Y + 1, vp->EndX, vp->EndY);
    }
}

/**
**	Display update.
**
*	This functions updates everything on screen. The map, the gui, the
**	cursors.
*/
global void UpdateDisplay(void)
{
    MustRedraw&=EnableRedraw;		// Don't redraw disabled parts

    VideoLockScreen();			// prepare video write

    HideAnyCursor();			// remove cursor (when available)

    if( MustRedraw&RedrawMap ) {
	DrawMapArea();
    }

    if( MustRedraw&(RedrawMessage|RedrawMap) ) {
	DrawMessages();
    }

    if( MustRedraw&RedrawFillers ) {
	int i;

	for( i=0; i<TheUI.NumFillers; ++i ) {
	    VideoDrawSub(TheUI.Filler[i].Graphic,0,0
		    ,TheUI.Filler[i].Graphic->Width
		    ,TheUI.Filler[i].Graphic->Height
		    ,TheUI.FillerX[i],TheUI.FillerY[i]);
	}
    }

    if( MustRedraw&RedrawMenuButton ) {
	DrawMenuButtonArea();
    }
    if( MustRedraw&RedrawMinimapBorder ) {
	VideoDrawSub(TheUI.Minimap.Graphic,0,0
		,TheUI.Minimap.Graphic->Width,TheUI.Minimap.Graphic->Height
		,TheUI.MinimapX,TheUI.MinimapY);
    }

    PlayerPixels(Players);		// Reset to default colors

    if( MustRedraw&RedrawMinimap ) {
	// FIXME: redraw only 1* per second!
	// HELPME: Viewpoint rectangle must be drawn faster (if implemented) ?
	DrawMinimap(TheUI.SelectedViewport->MapX, TheUI.SelectedViewport->MapY);
	DrawMinimapCursor(TheUI.SelectedViewport->MapX,
		TheUI.SelectedViewport->MapY);
    } else if( MustRedraw&RedrawMinimapCursor ) {
	HideMinimapCursor();
	DrawMinimapCursor(TheUI.SelectedViewport->MapX,
		TheUI.SelectedViewport->MapY);
    }

    if( MustRedraw&RedrawInfoPanel ) {
	DrawInfoPanel();
	PlayerPixels(Players);		// Reset to default colors
    }
    if( MustRedraw&RedrawButtonPanel ) {
	DrawButtonPanel();
	PlayerPixels(Players);		// Reset to default colors
    }
    if( MustRedraw&RedrawResources ) {
	DrawResources();
    }
    if( MustRedraw&RedrawStatusLine ) {
	DrawStatusLine();
	MustRedraw|=RedrawCosts;
    }
    if( MustRedraw&RedrawCosts ) {
	DrawCosts();
    }
    if( MustRedraw&RedrawTimer ) {
	DrawTimer();
    }

    if( MustRedraw&RedrawMenu ) {
	DrawMenu(CurrentMenu);
    }

    DrawAnyCursor();

    VideoUnlockScreen();		// End write access

    //
    //	Update changes to display.
    //
    if( MustRedraw&RedrawAll ) {
	// refresh entire screen, so no further invalidate needed
	InvalidateAreaAndCheckCursor(0,0,VideoWidth,VideoHeight);
    } else {
	if( MustRedraw&RedrawMap ) {
	    // FIXME: split into small parts see RedrawTile and RedrawRow
	    InvalidateAreaAndCheckCursor(
		     TheUI.MapArea.X,TheUI.MapArea.Y
		    ,TheUI.MapArea.EndX-TheUI.MapArea.X+1
		    ,TheUI.MapArea.EndY-TheUI.MapArea.Y+1);
	}
	if( MustRedraw&RedrawFillers ) {
	    int i;

	    for( i=0; i<TheUI.NumFillers; ++i ) {
		InvalidateAreaAndCheckCursor(
			 TheUI.FillerX[i],TheUI.FillerY[i]
			,TheUI.Filler[i].Graphic->Width
			,TheUI.Filler[i].Graphic->Height);
	    }
	}
	if(MustRedraw&RedrawMenuButton ) {
	    if( NetworkFildes==-1 ) {
		if( TheUI.MenuButton.X!=-1 ) {
		    InvalidateAreaAndCheckCursor(
			    TheUI.MenuButton.X,TheUI.MenuButton.Y,
			    TheUI.MenuButton.Width,
			    TheUI.MenuButton.Height);
		}
	    } else {
		if( TheUI.NetworkMenuButton.X!=-1 ) {
		    InvalidateAreaAndCheckCursor(
			    TheUI.NetworkMenuButton.X,
			    TheUI.NetworkMenuButton.Y,
			    TheUI.NetworkMenuButton.Width,
			    TheUI.NetworkMenuButton.Height);
		}
		if( TheUI.NetworkDiplomacyButton.X!=-1 ) {
		    InvalidateAreaAndCheckCursor(
			    TheUI.NetworkDiplomacyButton.X,
			    TheUI.NetworkDiplomacyButton.Y,
			    TheUI.NetworkDiplomacyButton.Width,
			    TheUI.NetworkDiplomacyButton.Height);
		}
	    }
	}
	if( MustRedraw&RedrawMinimapBorder ) {
	    InvalidateAreaAndCheckCursor(
		 TheUI.MinimapX,TheUI.MinimapY
		,TheUI.Minimap.Graphic->Width,TheUI.Minimap.Graphic->Height);
	} else if( (MustRedraw&RedrawMinimap)
		|| (MustRedraw&RedrawMinimapCursor) ) {
	    // FIXME: Redraws too much of the minimap
	    InvalidateAreaAndCheckCursor(
		     TheUI.MinimapX+24,TheUI.MinimapY+2
		    ,MINIMAP_W,MINIMAP_H);
	}
	if( MustRedraw&RedrawInfoPanel ) {
	    InvalidateAreaAndCheckCursor(
		     TheUI.InfoPanelX,TheUI.InfoPanelY
		    ,TheUI.InfoPanelW,TheUI.InfoPanelH);
	}
	if( MustRedraw&RedrawButtonPanel ) {
	    InvalidateAreaAndCheckCursor(
		     TheUI.ButtonPanelX,TheUI.ButtonPanelY
		    ,TheUI.ButtonPanel.Graphic->Width
		    ,TheUI.ButtonPanel.Graphic->Height);
	}
	if( MustRedraw&RedrawResources ) {
	    InvalidateAreaAndCheckCursor(
		     TheUI.ResourceX,TheUI.ResourceY
		    ,TheUI.Resource.Graphic->Width
		    ,TheUI.Resource.Graphic->Height);
	}
	if( MustRedraw&RedrawStatusLine || MustRedraw&RedrawCosts ) {
	    InvalidateAreaAndCheckCursor(
		     TheUI.StatusLineX,TheUI.StatusLineY
		    ,TheUI.StatusLine.Graphic->Width
		    ,TheUI.StatusLine.Graphic->Height);
	}
	if( MustRedraw&RedrawTimer ) {
	    // FIXME: Invalidate timer area
	}
	if( MustRedraw&RedrawMenu ) {
	    InvalidateMenuAreas();
	}

	// And now as very last.. checking if the cursor needs a refresh
	InvalidateCursorAreas();
    }
}

/**
**	Enable everything to be drawn for next display update.
**	Used at start of mainloop (and possible refresh as user option)
*/
local void EnableDrawRefresh(void)
{
    MustRedraw=RedrawEverything;
    MarkDrawEntireMap();
}

/**
**	Game main loop.
**
**	Unit actions.
**	Missile actions.
**	Players (AI).
**	Cyclic events (color cycle,...)
**	Display update.
**	Input/Network/Sound.
*/
global void GameMainLoop(void)
{
#ifdef DEBUG	// removes the setjmp warnings
    static int showtip;
#else
    int showtip;
#endif
    int RealVideoSyncSpeed;
    
    GameCallbacks.ButtonPressed=(void*)HandleButtonDown;
    GameCallbacks.ButtonReleased=(void*)HandleButtonUp;
    GameCallbacks.MouseMoved=(void*)HandleMouseMove;
    GameCallbacks.MouseExit=(void*)HandleMouseExit;
    GameCallbacks.KeyPressed=HandleKeyDown;
    GameCallbacks.KeyReleased=HandleKeyUp;
    GameCallbacks.KeyRepeated=HandleKeyRepeat;
    GameCallbacks.NetworkEvent=NetworkEvent;
    GameCallbacks.SoundReady=WriteSound;

    Callbacks=&GameCallbacks;

    SetVideoSync();
    EnableDrawRefresh();
    GameCursor=TheUI.Point.Cursor;
    GameRunning=1;

    showtip=0;
    RealVideoSyncSpeed = VideoSyncSpeed;
    if( NetworkFildes==-1 ) {		// Don't show them for net play
	showtip=ShowTips;
    }

    MultiPlayerReplayEachCycle();

    PlaySectionMusic(PlaySectionGame);

    while( GameRunning ) {
#if defined(DEBUG) && defined(HIERARCHIC_PATHFINDER)
	if( setjmp(MainLoopJmpBuf) ) {
	    GamePaused = 1;
	}
#endif
	//
	//	Game logic part
	//
	if( !GamePaused && NetworkInSync && !SkipGameCycle ) {
	    SinglePlayerReplayEachCycle();
	    if( !++GameCycle ) {
		// FIXME: tests with game cycle counter now fails :(
		// FIXME: Should happen in 68 years :)
		fprintf(stderr,"FIXME: *** round robin ***\n");
		fprintf(stderr,"FIXME: *** round robin ***\n");
		fprintf(stderr,"FIXME: *** round robin ***\n");
		fprintf(stderr,"FIXME: *** round robin ***\n");
	    }
	    MultiPlayerReplayEachCycle();
	    NetworkCommands();		// Get network commands
	    UnitActions();		// handle units
	    MissileActions();		// handle missiles
	    PlayersEachCycle();		// handle players
	    UpdateTimer();		// update game timer

	    //
	    //	Work todo each second.
	    //		Split into different frames, to reduce cpu time.
	    //		Increment mana of magic units.
	    //		Update mini-map.
	    //		Update map fog of war.
	    //		Call AI.
	    //		Check game goals.
	    //		Check rescue of units.
	    //
	    switch( GameCycle%CYCLES_PER_SECOND ) {
		case 0:
		    UnitIncrementMana();	// magic units
		    DoRunestones();		// runestones
		    break;
		case 1:
		    UnitIncrementHealth();	// berserker healing
		    break;
		case 2:				// minimap update
		    UpdateMinimap();
		    MustRedraw|=RedrawMinimap;
		    break;
		case 3:				// computer players
		    PlayersEachSecond();
		    break;
		case 4:				// forest grow
		    RegenerateForest();
		    break;
		case 5:				// overtaking units
		    RescueUnits();
		    break;
		case 6:
		    BurnBuildings();		// burn buildings
		    break;
	    }

	    //
	    // Work todo each realtime second.
	    //		Check cd-rom (every 2nd second)
	    // FIXME: Not called while pause or in the user interface.
	    //
	    switch( GameCycle%((CYCLES_PER_SECOND*VideoSyncSpeed/100)+1) ) {
		case 0:				// Check cd-rom
#if defined(USE_SDLCD)
		    if ( !(GameCycle%4) ) {	// every 2nd second
			SDL_CreateThread(CDRomCheck, NULL);
		    }
#elif defined(USE_LIBCDA) || defined(USE_CDDA)
		    CDRomCheck(NULL);
#endif
		    break;
		case 10:
		    if ( !(GameCycle%2) ) {
			PlaySectionMusic(PlaySectionUnknown);
		    }
		    break;
	    }
	}

	TriggersEachCycle();		// handle triggers
	UpdateMessages();		// update messages

	//
	//	Map scrolling
	//
	if( TheUI.MouseScroll && !(FrameCounter%SpeedMouseScroll) ) {
	    DoScrollArea(MouseScrollState, 0);
	}
	if( TheUI.KeyScroll && !(FrameCounter%SpeedKeyScroll) ) {
	    DoScrollArea(KeyScrollState, KeyModifiers&ModifierControl);
	}

	if( !(FrameCounter%COLOR_CYCLE_SPEED) ) {
	    if( ColorCycleAll>=0 ) {
		ColorCycle();
	    } else {
		// FIXME: should only update when needed
		MustRedraw|=RedrawInfoPanel;
	    }
	}

#if defined(DEBUG) && !defined(FLAG_DEBUG)
	MustRedraw|=RedrawMenuButton;
#endif
        if( FastForwardCycle > GameCycle
		&& RealVideoSyncSpeed != VideoSyncSpeed ) {
	    RealVideoSyncSpeed = VideoSyncSpeed;
	    VideoSyncSpeed = 3000;
	}
	if( FastForwardCycle >= GameCycle ) {
	    MustRedraw = RedrawEverything;
	}
	if( MustRedraw /* && !VideoInterrupts */
		&& (FastForwardCycle <= GameCycle || GameCycle <= 10
		    || !(GameCycle & 0x3f)) ) {
	    if( Callbacks==&MenuCallbacks ) {
		MustRedraw|=RedrawMenu;
	    }
	    if( CurrentMenu && CurrentMenu->Panel
		    && !strcmp(CurrentMenu->Panel, ScPanel) ) {
		MustRedraw = RedrawEverything;
	    }

	    //For debuggin only: replace UpdateDisplay by DebugTestDisplay when
	    //			 debugging linedraw routines..
	    //FIXME: this might be better placed somewhere at front of the
	    //	     program, as we now still have a game on the background and
	    //	     need to go through hte game-menu or supply a pud-file
	    UpdateDisplay();
	    //DebugTestDisplay();

	    //
	    // If double-buffered mode, we will display the contains of
	    // VideoMemory. If direct mode this does nothing. In X11 it does
	    // XFlush
	    //
	    RealizeVideoMemory();
#ifndef USE_OPENGL
	    MustRedraw=0;
#endif
	}

	CheckVideoInterrupts();		// look if already an interrupt

	if( FastForwardCycle == GameCycle ) {
	    VideoSyncSpeed = RealVideoSyncSpeed;
	}
	if( FastForwardCycle <= GameCycle || !(GameCycle & 0x3f) ) {
	    WaitEventsOneFrame(Callbacks);
	}
	if( !NetworkInSync ) {
	    NetworkRecover();		// recover network
	}

	if( showtip ) {
	    ProcessMenu("menu-tips",1);
	    InterfaceState = IfaceStateNormal;
	    showtip=0;
	}
    }

    if( Callbacks==&MenuCallbacks ) {
	while( CurrentMenu ) {
	    EndMenu();
	}
    }

    //
    //	Game over
    //
    if( FastForwardCycle > GameCycle ) {
	VideoSyncSpeed = RealVideoSyncSpeed;
    }
    NetworkQuit();
    EndReplayLog();
    if( GameResult==GameDefeat ) {
	fprintf(stderr,"You have lost!\n");
	SetStatusLine("You have lost!");
	ProcessMenu("menu-defeated", 1);
    }
    else if( GameResult==GameVictory ) {
	fprintf(stderr,"You have won!\n");
	SetStatusLine("You have won!");
	ProcessMenu("menu-victory", 1);
    }

    if( GameResult==GameVictory || GameResult==GameDefeat ) {
	PlaySectionMusic(PlaySectionStats);
	ShowStats();
    }

    FlagRevealMap=0;
    ReplayRevealMap=0;
    GamePaused=0;
    GodMode=0;
}

//@}
