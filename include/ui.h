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
/**@name ui.h		-	The user interface header file. */
//
//	(c) Copyright 1999-2002 by Lutz Sammer
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

#ifndef __UI_H__
#define __UI_H__

//@{

// FIXME: this only the start of the new user interface
// FIXME: all user interface variables should go here and be configurable

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include "video.h"
#include "upgrade_structs.h"
#include "cursor.h"

/*----------------------------------------------------------------------------
--	Definitons
----------------------------------------------------------------------------*/

/**
**	Defines the default SVGALIB mouse speed adjust (must be > 0)
*/
#define MOUSEADJUST	15

/**
**	Defines the default SVGALIB mouse speed scale
*/
#define MOUSESCALE	1

    /// MACRO - HARDCODED NUMBER OF BUTTONS on screen
#define MaxButtons	19

    /// typedef for buttons on screen themselves
typedef struct _button_ Button;

    /// buttons on screen themselves
struct _button_ {
    int		X;			/// x coordinate on the screen
    int		Y;			/// y coordinate on the screen
    int	    Width;			/// width of the button on the screen
    int	    Height;			/// height of the button on the screen
};

#ifdef SPLIT_SCREEN_SUPPORT

#define MAX_NUM_VIEWPORTS 4		/// Number of supported viewports

typedef struct _viewport_ Viewport;	/// Viewport typedef

/**
**	A map viewport.
**
**	A part of the map displayed on sceen.
**
**	Viewport::X Viewport::Y
**	Viewport::EndX Viewport::EndY
**
**		upper left corner of this viewport is located at pixel
**		coordinates (X, Y) with respect to upper left corner of
**		freecraft's window, similarly lower right corner of this
**		viewport is (EndX, EndY) pixels away from the UL corner of
**		freecraft's window.
**
**	Viewport::MapX Viewport::MapY
**	Viewport::MapWidth Viewport::MapHeight
**
**		Tile coordinates of UL corner of this viewport with respect to
**		UL corner of the whole map.
**
**	@todo mixing unsigned and int is dangerous!!!
*/
struct _viewport_ {
    int X;			/// Screen pixel left corner x coordinate
    int Y;			/// Screen pixel upper corner y coordinate
    unsigned EndX;		/// Screen pixel right x coordinate
    unsigned EndY;		/// Screen pixel bottom y coordinate

    unsigned MapX;		/// Map tile left corner x coordinate
    unsigned MapY;		/// Map tile upper corner y coordinate
    unsigned MapWidth;		/// Width in map tiles
    unsigned MapHeight;		/// Height in map tiles
};

/**
**	Enumeration of the different predefined viewport configurations.
*/
typedef enum {
    VIEWPORT_SINGLE,		/// Old single viewport
    VIEWPORT_SPLIT_HORIZ,	/// Two viewports split horizontal
    VIEWPORT_SPLIT_HORIZ3,	/// Three viewports split horiontal
    VIEWPORT_SPLIT_VERT,	/// Two viewports split vertical
    VIEWPORT_QUAD,		/// Four viewports split symmetric
    NUM_VIEWPORT_MODES		/// Number of different viewports.
} ViewportMode;

#endif /* SPLIT_SCREEN_SUPPORT */

/**
**	Defines the user interface.
*/
typedef struct _ui_ {
    // to select the correct user interface.
    char*	Name;			/// interface name to select
    unsigned	Width;			/// useable for this width
    unsigned	Height;			/// useable for this height

    int		Contrast;		/// General Contrast
    int		Brightness;		/// General Brightness
    int		Saturation;		/// General Saturation

    int		MouseScroll;		/// Enable mouse scrolling
    int		KeyScroll;		/// Enable keyboard scrolling
	/// Middle mouse button map move with reversed directions
    char	ReverseMouseMove;

    int		WarpX;			/// Cursor warp X position
    int		WarpY;			/// Cursor warp Y position

    int		MouseAdjust;		/// Mouse speed adjust
    int		MouseScale;		/// Mouse speed scale

    //	Fillers
    GraphicConfig Filler1;		/// filler 1 graphic
    int		Filler1X;		/// filler 1 X position
    int		Filler1Y;		/// filler 1 Y position

    //	Resource line
    GraphicConfig Resource;		/// Resource background
    int		ResourceX;		/// Resource X position
    int		ResourceY;		/// Resource Y position

    int		OriginalResources;	/// original resource mode

    struct {
	GraphicConfig Icon;		/// icon image
	int	IconRow;		/// icon image row (frame)
	int	IconX;			/// icon X position
	int	IconY;			/// icon Y position
	int	IconW;			/// icon W position
	int	IconH;			/// icon H position
	int	TextX;			/// text X position
	int	TextY;			/// text Y position
    }		Resources[MaxCosts];	/// Icon+Text of all resources

    GraphicConfig FoodIcon;		/// units icon image
    int		FoodIconRow;		/// units icon image row (frame)
    int		FoodIconX;		/// units icon X position
    int		FoodIconY;		/// units icon Y position
    int		FoodIconW;		/// units icon W position
    int		FoodIconH;		/// units icon H position
    int		FoodTextX;		/// units text X position
    int		FoodTextY;		/// units text Y position

    GraphicConfig ScoreIcon;		/// score icon image
    int		ScoreIconRow;		/// score icon image row (frame)
    int		ScoreIconX;		/// score icon X position
    int		ScoreIconY;		/// score icon Y position
    int		ScoreIconW;		/// score icon W position
    int		ScoreIconH;		/// score icon H position
    int		ScoreTextX;		/// score text X position
    int		ScoreTextY;		/// score text Y position

    // Info panel
    GraphicConfig InfoPanel;		/// Info panel background
    int		InfoPanelX;		/// Info panel screen X position
    int		InfoPanelY;		/// Info panel screen Y position
    int		InfoPanelW;		/// Info panel width
    int		InfoPanelH;		/// Info panel height

    // Complete bar
    int		CompleteBarColor;	/// color for complete bar
    int		CompleteBarX;		/// complete bar X position
    int		CompleteBarY;		/// complete bar Y position
    int		CompleteTextX;		/// complete text X position
    int		CompleteTextY;		/// complete text Y position

    // Button panel
    GraphicConfig ButtonPanel;		/// Button panel background
    int		ButtonPanelX;		/// Button panel screen X position
    int		ButtonPanelY;		/// Button panel screen Y position

#ifdef SPLIT_SCREEN_SUPPORT
    ViewportMode	ViewportMode;	/// Current viewport mode
    int		ActiveViewport;		/// Contains (or last contained) pointer
					/// (index into VP[])
    int		LastClickedVP;		/// FIXME: Docu
    int		NumViewports;		/// viewports currently used
    Viewport	VP[MAX_NUM_VIEWPORTS];	/// FIXME: Docu
    /* Map* attributes of Viewport are unused here */
    Viewport	MapArea;		/// geometry of the whole map area
#else /* SPLIT_SCREEN_SUPPORT */
    // The map
    int		MapX;			/// big map screen X position
    int		MapY;			/// big map screen Y position
	/// map width for current mode (MapX+14*32-1 for 640x480)
    unsigned	MapEndX;
	/// map height for current mode (MapY+14*32-1 for 640x480)
    unsigned	MapEndY;
#endif /* SPLIT_SCREEN_SUPPORT */

    // The menu button
    GraphicConfig MenuButton;		/// menu button background
    int		MenuButtonX;		/// menu button screen X position
    int		MenuButtonY;		/// menu button screen Y position

    // The minimap
    GraphicConfig Minimap;		/// minimap panel background
    int		MinimapX;		/// minimap screen X position
    int		MinimapY;		/// minimap screen Y position

    // The status line
    GraphicConfig StatusLine;		/// Status line background
    int		StatusLineX;		/// status line screeen X position
    int		StatusLineY;		/// status line screeen Y position

	/// all buttons (1 Menu, 9 Group, 9 Command)
    Button	Buttons[MaxButtons];
	/// used for displaying unit training queues
    Button	Buttons2[6];

    //
    //	Cursors used.
    //
    CursorConfig	Point;		/// General pointing cursor
    CursorConfig	Glass;		/// HourGlass, system is waiting
    CursorConfig	Cross;		/// Multi-select cursor.
    CursorConfig	YellowHair;	/// Yellow action,attack cursor.
    CursorConfig	GreenHair;	/// Green action,attack cursor.
    CursorConfig	RedHair;	/// Red action,attack cursor.
    CursorConfig	Scroll;		/// Cursor for scrolling map around.

    CursorConfig	ArrowE;		/// Cursor pointing east
    CursorConfig	ArrowNE;	/// Cursor pointing north east
    CursorConfig	ArrowN;		/// Cursor pointing north
    CursorConfig	ArrowNW;	/// Cursor pointing north west
    CursorConfig	ArrowW;		/// Cursor pointing west
    CursorConfig	ArrowSW;	/// Cursor pointing south west
    CursorConfig	ArrowS;		/// Cursor pointing south
    CursorConfig	ArrowSE;	/// Cursor pointing south east

// FIXME: could use different sounds/speach for the errors
// Is in gamesounds?
//    SoundConfig	PlacementError;		/// played on placements errors
//    SoundConfig	PlacementSuccess;	/// played on placements success
//    SoundConfig	Click;			/// click noice used often

    GraphicConfig	GameMenuePanel;	/// Panel for in game menue
    GraphicConfig	Menue1Panel;	/// Panel for menue (unused)
    GraphicConfig	Menue2Panel;	/// Panel for menue (unused)
    GraphicConfig	VictoryPanel;	/// Panel for victory message
    GraphicConfig	ScenarioPanel;	/// Panel for scenary message

} UI;

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern UI TheUI;			/// The user interface
extern UI** UI_Table;			/// All available user interfaces

extern char RightButtonAttacks;		/// right button 0 move, 1 attack.
extern char FancyBuildings;		/// Mirror buildings 1 yes, 0 now.

extern int SpeedKeyScroll;		/// Keyboard Scrolling Speed, in Frames
extern int SpeedMouseScroll;		/// Mouse Scrolling Speed, in Frames

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

extern void InitUserInterface(const char*);	/// initialize the ui
extern void LoadUserInterface(void);		/// load ui graphics
extern void SaveUserInterface(FILE*);		/// save the ui state
extern void CleanUserInterface(void);		/// clean up the ui
extern void UserInterfaceCclRegister(void);	/// register ccl features

    /// Called if the mouse is moved in Normal interface state
extern void UIHandleMouseMove(int x,int y);
    /// Called if any mouse button is pressed down
extern void UIHandleButtonDown(unsigned button);
    /// Called if any mouse button is released up
extern void UIHandleButtonUp(unsigned button);

    /// Restrict mouse cursor to minimap
extern void RestrictCursorToMinimap(void);

#ifdef SPLIT_SCREEN_SUPPORT

    /// FIXME: Short one line docu
extern int GetViewport(int, int);
    /// FIXME: Short one line docu
extern int MapTileGetViewport(int, int);
    /// FIXME: Short one line docu
extern void CycleViewportMode(int);
    /// FIXME: Short one line docu
extern void SetViewportMode(ViewportMode mode);

#endif /* SPLIT_SCREEN_SUPPORT */

//@}

#endif	// !__UI_H__
