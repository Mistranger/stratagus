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
/**@name editloop.c	-	The editor main loop. */
//
//	(c) Copyright 2002 by Lutz Sammer
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

#include "freecraft.h"
#include "video.h"
#include "map.h"
#include "minimap.h"
#include "settings.h"
#include "network.h"
#include "sound_server.h"
#include "ui.h"
#include "interface.h"
#include "font.h"
#include "editor.h"
#include "campaign.h"
#include "menus.h"
#include "sound.h"
#include "pud.h"
#include "iolib.h"
#include "iocompat.h"

#include "ccl.h"

extern void PreMenuSetup(void);		/// FIXME: not here!
extern void DoScrollArea(enum _scroll_state_ state, int fast);

extern struct {
    const char*	File[PlayerMaxRaces];	/// Resource filename one for each race
    int		Width;			/// Width of button
    int		Height;			/// Height of button
    Graphic*	Sprite;			/// Sprite : FILLED
} MenuButtonGfx;

/*----------------------------------------------------------------------------
--	Defines
----------------------------------------------------------------------------*/

#define UNIT_ICON_X (ICON_WIDTH + 7)		/// Unit mode icon
#define UNIT_ICON_Y (0)				/// Unit mode icon
#define TILE_ICON_X (ICON_WIDTH * 2 + 16)	/// Tile mode icon
#define TILE_ICON_Y (2)				/// Tile mode icon

#define TILE_WIDTH 32				/// Tile mode icon
#define TILE_HEIGHT 32				/// Tile mode icon

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

global char EditorRunning;		/// True editor is running

local enum _editor_state_ {
    EditorSelecting,			/// Select
    EditorEditTile,			/// Edit tiles
    EditorEditUnit,			/// Edit units
} EditorState;				/// Current editor state

local char TileToolRandom;		/// Tile tool draws random
local char TileToolDecoration;		/// Tile tool draws with decorations
local int TileCursorSize;		/// Tile cursor size 1x1 2x2 ... 4x4
local int TileCursor;			/// Tile type number

enum _mode_buttons_ {
    SelectButton=201,			/// Select mode button
    UnitButton,				/// Unit mode button
    TileButton,				/// Tile mode button
};

global char** EditorUnitTypes;		/// Sorted editor unit-type table

local int UnitIndex;			/// Unit icon draw index
global int MaxUnitIndex;		/// Max unit icon draw index
local int CursorUnitIndex;		/// Unit icon under cursor
local int SelectedUnitIndex;		/// Unit type to draw

local int CursorPlayer;			/// Player under the cursor
local int SelectedPlayer;		/// Player selected for draw

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Edit
----------------------------------------------------------------------------*/

/**
**	Get tile number.
**
**	@param basic	Basic tile number
**	@param random	Return random tile
**	@param filler	Get a decorated tile.
**
**	@return		Tile number used in pud.
**
**	@todo	FIXME: Solid tiles are here still hardcoded.
*/
local int GetTileNumber(int basic, int random, int filler)
{
    int tile;
    int i;
    int n;

    tile = 16 + basic * 16;
    if (random) {
	for (n = i = 0; i < 16; ++i) {
	    if (!TheMap.Tileset->Table[tile + i]) {
		if (!filler) {
		    break;
		}
	    } else {
		++n;
	    }
	}
	n = MyRand() % n;
	i = -1;
	do {
	    while( ++i < 16 && !TheMap.Tileset->Table[tile + i]) {
	    }
	} while( i < 16 && n-- );
	DebugCheck( i == 16 );
	return tile + i;
    }
    if (filler) {
	for (i = 0; i < 16 && TheMap.Tileset->Table[tile + i]; ++i) {
	}
	for (; i < 16 && !TheMap.Tileset->Table[tile + i]; ++i) {
	}
	if (i != 16) {
	    return tile + i;
	}
    }
    return tile;
}

/**
**	Edit tile.
**
**	@param x	X map tile coordinate.
**	@param y	Y map tile coordinate.
**	@param tile	Tile type to edit.
*/
local void EditTile(int x, int y, int tile)
{
    MapField* mf;

    DebugCheck(x < 0 || y < 0 || x >= TheMap.Width || y >= TheMap.Height);

    ChangeTile(x, y, GetTileNumber(tile, TileToolRandom, TileToolDecoration));

    //
    //  Change the flags
    //
    mf = &TheMap.Fields[y * TheMap.Width + x];
    mf->Flags &= ~(MapFieldHuman | MapFieldLandAllowed | MapFieldCoastAllowed |
	MapFieldWaterAllowed | MapFieldNoBuilding | MapFieldUnpassable |
	MapFieldWall | MapFieldRocks | MapFieldForest);

    mf->Flags |= TheMap.Tileset->FlagsTable[16 + tile * 16];

    UpdateMinimapSeenXY(x, y);
    UpdateMinimapXY(x, y);

    EditorTileChanged(x, y);

#if 0
    //
    //  Fix the wood tiles.
    //
    if (mf->Flags & MapFieldForest) {
	if( y ) {
	    ChangeTile(x, y - 1, GetTileNumber(tile, TileToolRandom,
		TileToolDecoration));
	    (mf-TheMap.Width)->Flags |=
		TheMap.Tileset->FlagsTable[16 + tile * 16];
	} else {
	    ChangeTile(x, y + 1, GetTileNumber(tile, TileToolRandom,
		TileToolDecoration));
	    mf[+TheMap.Width].Flags |=
		TheMap.Tileset->FlagsTable[16 + tile * 16];
	}
	MapFixWoodTile(x + 1, y + 0);
	MapFixWoodTile(x + 0, y + 1);
	MapFixWoodTile(x - 1, y + 0);
	MapFixWoodTile(x + 0, y - 1);
	MapFixWoodTile(x + 0, y + 0);
    } else if (mf->Flags & MapFieldRocks) {
	if( y ) {
	    ChangeTile(x, y - 1, GetTileNumber(tile, TileToolRandom,
		TileToolDecoration));
	    (mf-TheMap.Width)->Flags |=
		TheMap.Tileset->FlagsTable[16 + tile * 16];
	} else {
	    ChangeTile(x, y + 1, GetTileNumber(tile, TileToolRandom,
		TileToolDecoration));
	    mf[+TheMap.Width].Flags |=
		TheMap.Tileset->FlagsTable[16 + tile * 16];
	}
	MapFixRockTile(x + 1, y + 0);
	MapFixRockTile(x + 0, y + 1);
	MapFixRockTile(x - 1, y + 0);
	MapFixRockTile(x + 0, y - 1);
	MapFixRockTile(x + 0, y + 0);
    } else if (mf->Flags & MapFieldWall) {
	if (mf->Flags & MapFieldHuman) {
	    mf->Value = UnitTypeHumanWall->_HitPoints;
	} else {
	    mf->Value = UnitTypeOrcWall->_HitPoints;
	}
	MapFixWallTile(x + 0, y + 0);
	MapFixWallTile(x + 1, y + 0);
	MapFixWallTile(x + 0, y + 1);
	MapFixWallTile(x - 1, y + 0);
	MapFixWallTile(x + 0, y - 1);
    }
#endif
}

/**
**	Edit tiles.
**
**	@param x	X map tile coordinate.
**	@param y	Y map tile coordinate.
**	@param tile	Tile type to edit.
**	@param size	Size of rectangle
*/
local void EditTiles(int x, int y, int tile, int size)
{
    int ex;
    int ey;
    int i;

    ex = x + size;
    if (ex > TheMap.Width) {
	ex = TheMap.Width;
    }
    ey = y + size;
    if (ey > TheMap.Height) {
	ey = TheMap.Height;
    }
    while (y < ey) {
	for (i = x; i < ex; ++i) {
	    EditTile(i, y, tile);
	}
	++y;
    }
}

/**
**	Edit unit.
**
**	@param x	X map tile coordinate.
**	@param y	Y map tile coordinate.
**	@param type	Unit type to edit.
**	@param player	Player owning the unit.
**
**	@todo	FIXME: Check if the player has already a start-point.
*/
local void EditUnit(int x, int y, UnitType* type, Player* player)
{
    Unit *unit;

    unit = MakeUnitAndPlace(x, y, type, player);
    if (type->OilPatch || type->GivesOil) {
	unit->Value = 50000;
    }
    if (unit->Type->GoldMine) {
	unit->Value = 100000;
    }
}

/*----------------------------------------------------------------------------
--	Display
----------------------------------------------------------------------------*/

/**
**	Draw tile icons.
**
**	@todo for the start the solid tiles are hardcoded
**	If we have more solid tiles, than they fit into the panel, we need
**	some new ideas.
*/
local void DrawTileIcons(void)
{
    unsigned char** tiles;
    int x;
    int y;
    int i;

    x = TheUI.InfoPanelX + 46;
    y = TheUI.InfoPanelY + 4 + ICON_HEIGHT + 11;

    if( CursorOn == CursorOnButton &&
	    ButtonUnderCursor >= 300 && ButtonUnderCursor < 306 ) {
	VideoDrawRectangle(ColorGray, x - 42,
		y - 3 + (ButtonUnderCursor - 300) * 20, 100, 20);
    }

    VideoDrawTextCentered(x, y, GameFont, "1x1");
    VideoDraw(MenuButtonGfx.Sprite,
	MBUTTON_GEM_SQUARE + (TileCursorSize == 1 ? 2 : 0), x + 40, y - 3);
    y += 20;
    VideoDrawTextCentered(x, y, GameFont, "2x2");
    VideoDraw(MenuButtonGfx.Sprite,
	MBUTTON_GEM_SQUARE + (TileCursorSize == 2 ? 2 : 0), x + 40, y - 3);
    y += 20;
    VideoDrawTextCentered(x, y, GameFont, "3x3");
    VideoDraw(MenuButtonGfx.Sprite,
	MBUTTON_GEM_SQUARE + (TileCursorSize == 3 ? 2 : 0), x + 40, y - 3);
    y += 20;
    VideoDrawTextCentered(x, y, GameFont, "4x4");
    VideoDraw(MenuButtonGfx.Sprite,
	MBUTTON_GEM_SQUARE + (TileCursorSize == 4 ? 2 : 0), x + 40, y - 3);
    y += 20;
    VideoDrawTextCentered(x, y, GameFont, "Random");
    VideoDraw(MenuButtonGfx.Sprite,
	MBUTTON_GEM_SQUARE + (TileToolRandom ? 2 : 0), x + 40, y - 3);
    y += 20;
    VideoDrawTextCentered(x, y, GameFont, "Filler");
    VideoDraw(MenuButtonGfx.Sprite,
	MBUTTON_GEM_SQUARE + (TileToolDecoration ? 2 : 0), x + 40, y - 3);
    y += 20;

    tiles = TheMap.Tiles;

    y = TheUI.ButtonPanelY + 4;
    i = 0;

    while (y < TheUI.ButtonPanelY + 100) {
	x = TheUI.ButtonPanelX + 4;
	while (x < TheUI.ButtonPanelX + 144) {
	    VideoDrawTile(tiles[TheMap.Tileset->Table[0x10 + i * 16]], x, y);
	    VideoDrawRectangle(ColorGray, x, y, 32, 32);
	    if (TileCursor == i) {
		VideoDrawRectangle(ColorGreen, x + 1, y + 1, 30, 30);

	    }
	    if (CursorOn == CursorOnButton && ButtonUnderCursor == i + 100) {
		VideoDrawRectangle(ColorWhite, x - 1, y - 1, 34, 34);
	    }
	    x += 34;
	    i++;
	}
	y += 34;
    }
}

/**
**	Draw unit icons.
*/
local void DrawUnitIcons(void)
{
    int x;
    int y;
    int i;
    int j;
    int percent;
    char buf[256];
    Icon *icon;

    x = TheUI.InfoPanelX + 16;
    y = TheUI.InfoPanelY + 4 + ICON_HEIGHT + 10;

    for (i = 0; i < PlayerMax; ++i) {
	if (i == PlayerMax / 2) {
	    y += 18;
	}
	VideoDrawRectangle(
	    i==CursorPlayer ? ColorWhite : ColorGray,
	    x + i % 8 * 18, y, 15, 15);
	VideoFillRectangle(Players[i].Color, x + 1 + i % 8 * 18, y + 1, 13,
	    13);
	if( i==SelectedPlayer ) {
	    VideoDrawRectangle(ColorGreen, x + 1 + i % 8 * 18, y + 1, 13, 13);
	}
	sprintf(buf,"%d",i);
	VideoDrawTextCentered(x + i % 8 * 18 + 8, y + 5, SmallFont, buf);
    }

    x = TheUI.InfoPanelX + 4;
    y += 18 * 1 + 4;
    if( SelectedPlayer != -1 ) {
	i=sprintf(buf,"Plyr %d %s ",SelectedPlayer,
		RaceWcNames[TheMap.Info->PlayerSide[SelectedPlayer]]);
	// Players[SelectedPlayer].RaceName);

	switch( TheMap.Info->PlayerType[SelectedPlayer] ) {
	    case PlayerNeutral:
		strcat(buf,"Neutral");
		break;
	    case PlayerNobody:
	    default:
		strcat(buf,"Nobody");
		break;
	    case PlayerPerson:
		strcat(buf,"Person");
		break;
	    case PlayerComputer:
	    case PlayerRescuePassive:
	    case PlayerRescueActive:
		strcat(buf,"Computer");
		break;
	}

	VideoDrawText(x, y, GameFont, buf);
    }

    //
    //	Draw the unit selection buttons.
    //
    x = TheUI.InfoPanelX + 10;
    y = TheUI.InfoPanelY + 140;

    VideoDrawText(x + 28 * 0, y, GameFont, "Un");
    VideoDraw(MenuButtonGfx.Sprite,
	MBUTTON_GEM_SQUARE + (1 ? 2 : 0), x + 28 * 0, y + 16);
    VideoDrawText(x + 28 * 1, y, GameFont, "Bu");
    VideoDraw(MenuButtonGfx.Sprite,
	MBUTTON_GEM_SQUARE + (1 ? 2 : 0), x + 28 * 1, y + 16);
    VideoDrawText(x + 28 * 2, y, GameFont, "He");
    VideoDraw(MenuButtonGfx.Sprite,
	MBUTTON_GEM_SQUARE + (1 ? 2 : 0), x + 28 * 2, y + 16);
    VideoDrawText(x + 28 * 3, y, GameFont, "La");
    VideoDraw(MenuButtonGfx.Sprite,
	MBUTTON_GEM_SQUARE + (1 ? 2 : 0), x + 28 * 3, y + 16);
    VideoDrawText(x + 28 * 4, y, GameFont, "Wa");
    VideoDraw(MenuButtonGfx.Sprite,
	MBUTTON_GEM_SQUARE + (1 ? 2 : 0), x + 28 * 4, y + 16);
    VideoDrawText(x + 28 * 5, y, GameFont, "Ai");
    VideoDraw(MenuButtonGfx.Sprite,
	MBUTTON_GEM_SQUARE + (1 ? 2 : 0), x + 28 * 5, y + 16);
#if 0
    j = 0;
    for (i = 0; EditorUnitTypes[i]; ++i) {
	const UnitType *type;

	if ((type = UnitTypeByIdent(EditorUnitTypes[i]))) {
	    if (type->Building) {
		++j;
	    }
	}
    }
    sprintf(buf, "Buildings %d\n", j);
    VideoDrawText(x, y, GameFont, buf);
    y += 16;

    j = 0;
    for (i = 0; EditorUnitTypes[i]; ++i) {
	const UnitType *type;

	if ((type = UnitTypeByIdent(EditorUnitTypes[i]))) {
	    if (!type->Building && type->UnitType == UnitTypeLand) {
		++j;
	    }
	}
    }
    sprintf(buf, "Land units %d\n", j);
    VideoDrawText(x, y, GameFont, buf);
    y += 16;

    j = 0;
    for (i = 0; EditorUnitTypes[i]; ++i) {
	const UnitType *type;

	if ((type = UnitTypeByIdent(EditorUnitTypes[i]))) {
	    if (!type->Building && type->UnitType == UnitTypeFly) {
		++j;
	    }
	}
    }
    sprintf(buf, "Sea units %d\n", j);
    VideoDrawText(x, y, GameFont, buf);
    y += 16;

    j = 0;
    for (i = 0; EditorUnitTypes[i]; ++i) {
	const UnitType *type;

	if ((type = UnitTypeByIdent(EditorUnitTypes[i]))) {
	    if (!type->Building && type->UnitType == UnitTypeNaval) {
		++j;
	    }
	}
    }
    sprintf(buf, "Air units %d\n", j);
    VideoDrawText(x, y, GameFont, buf);

#endif

    //
    //	Scroll bar for units. FIXME: drag not supported.
    //
    x = TheUI.ButtonPanelX + 4;
    y = TheUI.ButtonPanelY + 4;
    j = 176-8;

    PushClipping();
    SetClipping(0,0,x + j - 20,VideoHeight-1);
    VideoDrawClip(MenuButtonGfx.Sprite, MBUTTON_S_HCONT, x - 2, y);
    PopClipping();
    if (TheUI.ButtonPanelX + 4 < CursorX
	    && CursorX < TheUI.ButtonPanelX + 24
	    && TheUI.ButtonPanelY + 4 < CursorY
	    && CursorY < TheUI.ButtonPanelY + 24
	    && MouseButtons&LeftButton) {
	VideoDraw(MenuButtonGfx.Sprite, MBUTTON_LEFT_ARROW + 1, x - 2, y);
    } else {
	VideoDraw(MenuButtonGfx.Sprite, MBUTTON_LEFT_ARROW, x - 2, y);
    }
    if (TheUI.ButtonPanelX + 176 - 24 < CursorX
	    && CursorX < TheUI.ButtonPanelX + 176 - 4
	    && TheUI.ButtonPanelY + 4 < CursorY
	    && CursorY < TheUI.ButtonPanelY + 24
	    && MouseButtons&LeftButton) {
	VideoDraw(MenuButtonGfx.Sprite, MBUTTON_RIGHT_ARROW + 1, x + j - 20, y);
    } else {
	VideoDraw(MenuButtonGfx.Sprite, MBUTTON_RIGHT_ARROW, x + j - 20, y);
    }

    percent = UnitIndex * 100 / (MaxUnitIndex / 9 * 9);
    i = (percent * (j - 54)) / 100;
    VideoDraw(MenuButtonGfx.Sprite, MBUTTON_S_KNOB, x + 18 + i, y + 1);

    //
    //	Draw the unit icons.
    //
    y = TheUI.ButtonPanelY + 24;

    i = UnitIndex;
    while( y < TheUI.ButtonPanelY
	    + TheUI.ButtonPanel.Graphic->Height - ICON_HEIGHT ) {
	if( !EditorUnitTypes[i] ) {
	    break;
	}
	x = TheUI.ButtonPanelX + 10;
	while( x < TheUI.ButtonPanelX + 146 ) {
	    if( !EditorUnitTypes[i] ) {
		break;
	    }
	    icon = UnitTypeByIdent(EditorUnitTypes[i])->Icon.Icon;
	    VideoDrawSub(icon->Graphic, icon->X, icon->Y, icon->Width,
		icon->Height, x, y);

	    VideoDrawRectangle(ColorGray, x, y, icon->Width, icon->Height);
	    if( i==SelectedUnitIndex ) {
		VideoDrawRectangle(ColorGreen, x + 1, y + 1,
			icon->Width - 2, icon->Height - 2);
	    }
	    if( i==CursorUnitIndex ) {
		VideoDrawRectangle(ColorWhite,x - 1, y - 1,
			icon->Width + 2, icon->Height + 2);
	    }

	    x += ICON_WIDTH + 8;
	    ++i;
	}
	y += ICON_HEIGHT + 2;
    }
}

/**
**	Draw a tile icon
**
**	@param tilenum	Tile number to display
**	@param x	X display position
**	@param y	Y display position
**	@param flags	State of the icon (::IconActive,::IconClicked,...)
*/
local void DrawTileIcon(unsigned tilenum,unsigned x,unsigned y,unsigned flags)
{
    int color;

    color= (flags&IconActive) ? ColorGray : ColorBlack;

    VideoDrawRectangleClip(color,x,y,TILE_WIDTH+7,TILE_HEIGHT+7);
    VideoDrawRectangleClip(ColorBlack,x+1,y+1,TILE_WIDTH+5,TILE_HEIGHT+5);

    VideoDrawVLine(ColorGray,x+TILE_WIDTH+4,y+5,TILE_HEIGHT-1);	// _|
    VideoDrawVLine(ColorGray,x+TILE_WIDTH+5,y+5,TILE_HEIGHT-1);
    VideoDrawHLine(ColorGray,x+5,y+TILE_HEIGHT+4,TILE_WIDTH+1);
    VideoDrawHLine(ColorGray,x+5,y+TILE_HEIGHT+5,TILE_WIDTH+1);

    color= (flags&IconClicked) ? ColorGray : ColorWhite;
    VideoDrawHLine(color,x+5,y+3,TILE_WIDTH+1);
    VideoDrawHLine(color,x+5,y+4,TILE_WIDTH+1);
    VideoDrawVLine(color,x+3,y+3,TILE_HEIGHT+3);
    VideoDrawVLine(color,x+4,y+3,TILE_HEIGHT+3);

    if( flags&IconClicked ) {
	++x; ++y;
    }

    x+=4;
    y+=4;
    VideoDrawTile(TheMap.Tiles[TheMap.Tileset->Table[tilenum]], x, y);

    if( flags&IconSelected ) {
	VideoDrawRectangleClip(ColorGreen,x,y,TILE_WIDTH,TILE_HEIGHT);
    }
}

/**
**	Draw the editor panels.
*/
local void DrawEditorPanel(void)
{
    int x;
    int y;
    Icon *icon;

    x = TheUI.InfoPanelX + 4;
    y = TheUI.InfoPanelY + 4;

    //
    //  Select / Units / Tiles
    //
    icon = IconByIdent("icon-human-patrol-land");
    DebugCheck(!icon);
    DrawUnitIcon(Players, icon,
        (ButtonUnderCursor == SelectButton ? IconActive : 0) |
	    (EditorState==EditorSelecting ? IconSelected : 0),
	x, y);
    icon = IconByIdent("icon-footman");
    DebugCheck(!icon);
    DrawUnitIcon(Players, icon,
        (ButtonUnderCursor == UnitButton ? IconActive : 0) |
	    (EditorState==EditorEditUnit ? IconSelected : 0),
	x + UNIT_ICON_X, y + UNIT_ICON_Y);

    DrawTileIcon(0x10 + 4 * 16, x + TILE_ICON_X, y + TILE_ICON_Y,
	(ButtonUnderCursor == TileButton ? IconActive : 0) |
	    (EditorState==EditorEditTile ? IconSelected : 0));

    switch (EditorState) {
	case EditorSelecting:
	    break;
	case EditorEditTile:
	    DrawTileIcons();
	    break;
	case EditorEditUnit:
	    DrawUnitIcons();
	    break;
    }
}

/**
**	Draw special cursor on map.
**
**	@todo support for bigger cursors (2x2, 3x3 ...)
*/
local void DrawMapCursor(void)
{
    int v;
    int x;
    int y;

    //
    //  Draw map cursor
    //
    v = TheUI.ActiveViewport;
    if (v != -1 && !CursorBuilding) {
	x = Viewport2MapX(v, CursorX);
	y = Viewport2MapY(v, CursorY);
	x = Map2ViewportX(v, x);
	y = Map2ViewportY(v, y);
	if (EditorState == EditorEditTile) {
	    int i;
	    int j;

	    for (j = 0; j < TileCursorSize; ++j) {
		int ty;

		ty = y + j * TileSizeY;
		if (ty >= TheUI.VP[v].EndY) {
		    break;
		}
		for (i = 0; i < TileCursorSize; ++i) {
		    int tx;

		    tx = x + i * TileSizeX;
		    if (tx >= TheUI.VP[v].EndX) {
			break;
		    }
		    VideoDrawTile(TheMap.Tiles[TheMap.Tileset->Table[0x10 +
				TileCursor * 16]], tx, ty);
		}
	    }
	    SetClipping (TheUI.VP[v].X, TheUI.VP[v].Y,
		TheUI.VP[v].EndX, TheUI.VP[v].EndY);
	    VideoDrawRectangleClip(ColorWhite, x, y, TileSizeX * TileCursorSize,
		TileSizeY * TileCursorSize);
	    SetClipping(0,0,VideoWidth-1,VideoHeight-1);
	} else {
	    VideoDrawRectangle(ColorWhite, x, y, TileSizeX, TileSizeY);
	}
    }
}

/**
**	Draw editor info.
*/
local void DrawEditorInfo(void)
{
    int v;
    int x;
    int y;
    unsigned flags;
    char buf[256];

    v = TheUI.LastClickedVP;
    x = y = 0;
    if (v != -1) {
	x = Viewport2MapX(v, CursorX);
	y = Viewport2MapY(v, CursorY);
    }

    sprintf(buf, "Editor: (%d %d)", x, y);
    VideoDrawText(TheUI.ResourceX + 2, TheUI.ResourceY + 2, GameFont, buf);

    flags = TheMap.Fields[x + y * TheMap.Width].Flags;
    sprintf(buf, "%02X|%04X|%c%c%c%c%c%c%c%c%c%c%c%c%c",
	TheMap.Fields[x + y * TheMap.Width].Value, flags,
	flags & MapFieldUnpassable	? 'u' : '-',
	flags & MapFieldNoBuilding	? 'n' : '-',
	flags & MapFieldHuman		? 'h' : '-',
	flags & MapFieldWall		? 'w' : '-',
	flags & MapFieldRocks		? 'r' : '-',
	flags & MapFieldForest		? 'f' : '-',
	flags & MapFieldLandAllowed	? 'L' : '-',
	flags & MapFieldCoastAllowed	? 'C' : '-',
	flags & MapFieldWaterAllowed	? 'W' : '-',
	flags & MapFieldLandUnit	? 'l' : '-',
	flags & MapFieldAirUnit		? 'a' : '-',
	flags & MapFieldSeaUnit		? 's' : '-',
	flags & MapFieldBuilding	? 'b' : '-');
    VideoDrawText(TheUI.ResourceX + 152, TheUI.ResourceY + 2, GameFont, buf);
    sprintf(buf, "Tile: %d", TheMap.Fields[x + y * TheMap.Width].Tile);
    VideoDrawText(TheUI.ResourceX + 302, TheUI.ResourceY + 2, GameFont, buf);
}

/**
**	Show info about unit.
**
**	@param unit	Unit pointer.
*/
local void ShowUnitInfo(const Unit* unit)
{
    char buf[256];
    int i;

    i=sprintf(buf, "#%d '%s' Player:#%d %s", UnitNumber(unit),
	unit->Type->Name, unit->Player->Player,
	unit->Active ? "active" : "passive");
    if( unit->Type->OilPatch || unit->Type->GivesOil
	    || unit->Type->GoldMine ) {
	sprintf(buf+i," Amount %d\n",unit->Value);
    }
    SetStatusLine(buf);
}

/**
**	Update editor display.
*/
global void EditorUpdateDisplay(void)
{
    int i;

    VideoLockScreen();			// { prepare video write

    HideAnyCursor();			// remove cursor (when available)

    DrawMapArea();			// draw the map area

    if (CursorOn==CursorOnMap) {
	DrawMapCursor();			// cursor on map
    }

    //
    //  Menu button
    //
    if (TheUI.MenuButton.Graphic) {
	VideoDrawSub(TheUI.MenuButton.Graphic, 0, 0,
	    TheUI.MenuButton.Graphic->Width, TheUI.MenuButton.Graphic->Height,
	    TheUI.MenuButtonX, TheUI.MenuButtonY);
    }
    DrawMenuButton(MBUTTON_MAIN,
	    (ButtonUnderCursor == 0 ? MenuButtonActive : 0)|
	    (GameMenuButtonClicked ? MenuButtonClicked : 0),
	    128, 19,
	    TheUI.MenuButtonX+24,TheUI.MenuButtonY+2,
	    GameFont,"Menu (~<F10~>)");

    //
    //  Minimap border
    //
    if (TheUI.Minimap.Graphic) {
	VideoDrawSub(TheUI.Minimap.Graphic, 0, 0, TheUI.Minimap.Graphic->Width,
	    TheUI.Minimap.Graphic->Height, TheUI.MinimapX, TheUI.MinimapY);
    }
    //
    //  Minimap
    //
    i = TheUI.LastClickedVP;
    if (i >= 0) {
	DrawMinimap(TheUI.VP[i].MapX, TheUI.VP[i].MapY);
	DrawMinimapCursor(TheUI.VP[i].MapX, TheUI.VP[i].MapY);
    }
    //
    //  Info panel
    //
    if (TheUI.InfoPanel.Graphic) {
	VideoDrawSub(TheUI.InfoPanel.Graphic, 0, 0,
	    TheUI.InfoPanel.Graphic->Width, TheUI.InfoPanel.Graphic->Height/4,
	    TheUI.InfoPanelX, TheUI.InfoPanelY);
    }
    //
    //  Button panel
    //
    if (TheUI.ButtonPanel.Graphic) {
	VideoDrawSub(TheUI.ButtonPanel.Graphic, 0, 0,
	    TheUI.ButtonPanel.Graphic->Width,
	    TheUI.ButtonPanel.Graphic->Height, TheUI.ButtonPanelX,
	    TheUI.ButtonPanelY);
    }
    DrawEditorPanel();

    //
    //  Resource
    //
    if (TheUI.Resource.Graphic) {
	VideoDrawSub(TheUI.Resource.Graphic, 0, 0,
	    TheUI.Resource.Graphic->Width, TheUI.Resource.Graphic->Height,
	    TheUI.ResourceX, TheUI.ResourceY);
    }
    DrawEditorInfo();

    //
    //  Filler
    //
    if (TheUI.Filler1.Graphic) {
	VideoDrawSub(TheUI.Filler1.Graphic, 0, 0, TheUI.Filler1.Graphic->Width,
	    TheUI.Filler1.Graphic->Height, TheUI.Filler1X, TheUI.Filler1Y);
    }
    //
    //  Status line
    //
    DrawStatusLine();

#if 0
    if (TheUI.StatusLine.Graphic) {
	VideoDrawSub(TheUI.StatusLine.Graphic, 0, 0,
	    TheUI.StatusLine.Graphic->Width, TheUI.StatusLine.Graphic->Height,
	    TheUI.StatusLineX, TheUI.StatusLineY);
    }
#endif

    DrawAnyCursor();

    VideoUnlockScreen();		// } end write access

    // FIXME: For now update everything each frame

    // refresh entire screen, so no further invalidate needed
    InvalidateAreaAndCheckCursor(0, 0, VideoWidth, VideoHeight);
    RealizeVideoMemory();
}

/*----------------------------------------------------------------------------
--	Input / Keyboard / Mouse
----------------------------------------------------------------------------*/

/**
**	Callback for input.
*/
local void EditorCallbackButtonUp(unsigned button)
{
    DebugLevel3Fn("Pressed %8x %8x\n" _C_ MouseButtons _C_ dummy);

    if( (1<<button) == LeftButton && GameMenuButtonClicked==1 ) {
	GameMenuButtonClicked=0;
	if( ButtonUnderCursor==0 ) {
	    ProcessMenu("menu-editor",1);
	}
    }
}

/**
**	Called if mouse button pressed down.
**
**	@param button	Mouse button number (0 left, 1 middle, 2 right)
*/
local void EditorCallbackButtonDown(unsigned button __attribute__ ((unused)))
{
    int i;
    int percent = UnitIndex * 100 / (MaxUnitIndex / 9 * 9);
    int j = (percent * (176 - 8 - 54)) / 100;

    DebugLevel3Fn("%x %x\n" _C_ button _C_ MouseButtons);

    //
    //  Click on menu button
    //
    if (CursorOn == CursorOnButton && ButtonUnderCursor == 0
	    && (MouseButtons & LeftButton) && !GameMenuButtonClicked) {
	PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
	GameMenuButtonClicked = 1;
	return;
    }
    //
    //  Click on minimap
    //
    if (CursorOn == CursorOnMinimap) {
	if (MouseButtons & LeftButton) {	// enter move mini-mode
	    i = TheUI.LastClickedVP;
	    MapViewportSetViewpoint(i,
		ScreenMinimap2MapX(CursorX) - TheUI.VP[i].MapWidth / 2,
		ScreenMinimap2MapY(CursorY) - TheUI.VP[i].MapHeight / 2);
	}
	return;
    }
    //
    //  Click on mode area
    //
    if (CursorOn == CursorOnButton) {
	if (ButtonUnderCursor == SelectButton) {
	    CursorBuilding = NULL;
	    EditorState = EditorSelecting;
	    return;
	}
	if (ButtonUnderCursor == UnitButton) {
	    EditorState = EditorEditUnit;
	    return;
	}
	if (ButtonUnderCursor == TileButton) {
	    CursorBuilding = NULL;
	    EditorState = EditorEditTile;
	    return;
	}
    }
    //
    //  Click on tile area
    //
    if (CursorOn == CursorOnButton && ButtonUnderCursor >= 100
	    && EditorState == EditorEditTile) {
	switch (ButtonUnderCursor) {
	    case 300:
		TileCursorSize = 1;
		return;
	    case 301:
		TileCursorSize = 2;
		return;
	    case 302:
		TileCursorSize = 3;
		return;
	    case 303:
		TileCursorSize = 4;
		return;
	    case 304:
		TileToolRandom ^= 1;
		return;
	    case 305:
		TileToolDecoration ^= 1;
		return;
	}
	if (TheMap.Tileset->BasicNameTable[
		16 + (ButtonUnderCursor - 100) * 16]) {
	    TileCursor = ButtonUnderCursor - 100;
	}
	return;
    }
    //
    //  Click on unit area
    //
    if (EditorState == EditorEditUnit) {
	if (TheUI.ButtonPanelX + 4 < CursorX
		&& CursorX < TheUI.ButtonPanelX + 4 + 18 + j
		&& TheUI.ButtonPanelY + 4 < CursorY
		&& CursorY < TheUI.ButtonPanelY + 24) {

	    if (UnitIndex - 9 >= 0) {
		UnitIndex -= 9;
	    }
	    return;
	}
	if (TheUI.ButtonPanelX + 4 + 18 + j + 18 < CursorX
		&& CursorX < TheUI.ButtonPanelX + 176 - 4
		&& TheUI.ButtonPanelY + 4 < CursorY
		&& CursorY < TheUI.ButtonPanelY + 24) {

	    if (UnitIndex + 9 <= MaxUnitIndex) {
		UnitIndex += 9;
	    }
	    return;
	}
	if (CursorUnitIndex != -1) {
	    SelectedUnitIndex = CursorUnitIndex;
	    CursorBuilding = UnitTypeByIdent(EditorUnitTypes[CursorUnitIndex]);
	    ThisPlayer = Players + SelectedPlayer;
	    return;
	}
	if (CursorPlayer != -1) {
	    SelectedPlayer = CursorPlayer;
	    return;
	}
    }

    //
    //	Right click on a resource
    //
    if (EditorState == EditorSelecting) {
	if( (MouseButtons&RightButton && UnitUnderCursor) ) {
	    if( UnitUnderCursor->Type->GoldMine ||
		UnitUnderCursor->Type->OilPatch ||
		UnitUnderCursor->Type->GivesOil ) {
		EditorEditResource();
		return;
	    }
	    if( !UnitUnderCursor->Type->Building && UnitUnderCursor->HP > 0 ) {
		EditorEditAiProperties();
	    }
	}
    }

    //
    //  Click on map area
    //
    if (CursorOn == CursorOnMap) {
	i = GetViewport(CursorX, CursorY);
	if (TheUI.LastClickedVP != i) {	// New viewport
	    TheUI.LastClickedVP = i;
	    MustRedraw = RedrawMinimapCursor | RedrawMap;
	}

	if (EditorState == EditorEditTile) {
	    EditTiles(Viewport2MapX(TheUI.ActiveViewport, CursorX),
		Viewport2MapY(TheUI.ActiveViewport, CursorY), TileCursor,
		TileCursorSize);
	}
	if (EditorState == EditorEditUnit && CursorBuilding) {
	    if (CanBuildUnitType(NULL, CursorBuilding,
		    Viewport2MapX(TheUI.ActiveViewport, CursorX),
		    Viewport2MapY(TheUI.ActiveViewport, CursorY))) {
		PlayGameSound(GameSounds.PlacementSuccess.Sound,
		    MaxSampleVolume);
		EditUnit(Viewport2MapX(TheUI.ActiveViewport,CursorX),
		    Viewport2MapY(TheUI.ActiveViewport, CursorY),
		    CursorBuilding, Players + SelectedPlayer);
	    } else {
		SetStatusLine("Unit can't be placed here.");
		PlayGameSound(GameSounds.PlacementError.Sound,
		    MaxSampleVolume);
	    }
	}
    }
}

/**
**	Handle key down.
**
**	@param key	Key scancode.
**	@param keychar	Character code.
*/
local void EditorCallbackKeyDown(unsigned key, unsigned keychar)
{
    if (HandleKeyModifiersDown(key, keychar)) {
	return;
    }

    switch (key) {
	case 'f'&0x1F:
	case 'f':
	case 'F':			// ALT+F, CTRL+F toggle fullscreen
	    if( !(KeyModifiers&(ModifierAlt|ModifierControl)) ) {
		break;
	    }
	    ToggleFullScreen();
	    break;

	case 's':			// ALT s F11 save pud menu
	case 'S':
	case KeyCodeF11:
	    if (EditorSave()) {
		SetStatusLine("Pud saved");
	    }
	    InterfaceState = IfaceStateNormal;
	    break;

	case 'v':		// 'v' Viewport
	    if (KeyModifiers & ModifierControl) {
		CycleViewportMode(-1);
	    } else {
		CycleViewportMode(1);
	    }
	    break;

	case 'x'&0x1F:
	case 'x':
	case 'X':			// ALT+X, CTRL+X: Exit editor
	    if( !(KeyModifiers&(ModifierAlt|ModifierControl)) ) {
		break;
	    }
	    Exit(0);

	case KeyCodeDelete:	// Delete
	    if( UnitUnderCursor ) {
		Unit* unit;

		RemoveUnit(unit = UnitUnderCursor);
		UnitLost(unit);
		ReleaseUnit(unit);
		SetStatusLine("Unit deleted");
	    }
	    break;

	case KeyCodeF10:
	    ProcessMenu("menu-editor",1);
	    break;

	case KeyCodeUp:		// Keyboard scrolling
	case KeyCodeKP8:
	    KeyScrollState |= ScrollUp;
	    break;
	case KeyCodeDown:
	case KeyCodeKP2:
	    KeyScrollState |= ScrollDown;
	    break;
	case KeyCodeLeft:
	case KeyCodeKP4:
	    KeyScrollState |= ScrollLeft;
	    break;
	case KeyCodeRight:
	case KeyCodeKP6:
	    KeyScrollState |= ScrollRight;
	    break;

	default:
	    DebugLevel3("Key %d\n" _C_ key);
	    return;
    }
    return;
}

/**
**	Handle key up.
**
**	@param key	Key scancode.
**	@param keychar	Character code.
*/
local void EditorCallbackKeyUp(unsigned key, unsigned keychar)
{
    if (HandleKeyModifiersUp(key, keychar)) {
	return;
    }

    switch (key) {
	case KeyCodeUp:			// Keyboard scrolling
	case KeyCodeKP8:
	    KeyScrollState &= ~ScrollUp;
	    break;
	case KeyCodeDown:
	case KeyCodeKP2:
	    KeyScrollState &= ~ScrollDown;
	    break;
	case KeyCodeLeft:
	case KeyCodeKP4:
	    KeyScrollState &= ~ScrollLeft;
	    break;
	case KeyCodeRight:
	case KeyCodeKP6:
	    KeyScrollState &= ~ScrollRight;
	    break;
	default:
	    break;
    }
}

/**
**	Callback for input.
*/
local void EditorCallbackKey2(unsigned dummy1 __attribute__((unused)),
	unsigned dummy2 __attribute__((unused)))
{
    DebugLevel3Fn("Pressed %8x %8x %8x\n" _C_ MouseButtons _C_ dummy1 _C_
	    dummy2);
}

/**
**	Callback for input.
*/
local void EditorCallbackKey3(unsigned dummy1 __attribute__((unused)),
	unsigned dummy2 __attribute__((unused)))
{
    DebugLevel3Fn("Repeated %8x %8x %8x\n" _C_ MouseButtons _C_ dummy1 _C_
	    dummy2);
}

/**
**	Callback for input movement of the cursor.
**
**	@param x	Screen X position.
**	@param y	Screen Y position.
*/
local void EditorCallbackMouse(int x, int y)
{
    int i;
    int bx;
    int by;
    enum _cursor_on_ OldCursorOn;
    int viewport;
    char buf[256];

    DebugLevel3Fn("Moved %d,%d\n" _C_ x _C_ y);

    HandleCursorMove(&x, &y);		// Reduce to screen

    //
    //	Drawing tiles on map.
    //
    if( CursorOn == CursorOnMap && (MouseButtons&LeftButton)
		&& (EditorState == EditorEditTile
		    || EditorState == EditorEditUnit) ) {

	//
	//	Scroll the map
	//
	if( !(FrameCounter % SpeedMouseScroll) ) {
	    viewport = TheUI.LastClickedVP;
	    if (CursorX < TheUI.VP[viewport].X) {
		MapViewportSetViewpoint(viewport,
		    TheUI.VP[viewport].MapX - 1, TheUI.VP[viewport].MapY);
	    } else if (CursorX >= TheUI.VP[viewport].EndX) {
		MapViewportSetViewpoint(viewport,
		    TheUI.VP[viewport].MapX + 1, TheUI.VP[viewport].MapY);
	    }

	    if (CursorY < TheUI.VP[viewport].Y) {
		MapViewportSetViewpoint(viewport,
		    TheUI.VP[viewport].MapX, TheUI.VP[viewport].MapY - 1);
	    } else if (CursorY >= TheUI.VP[viewport].EndY) {
		MapViewportSetViewpoint(viewport,
		    TheUI.VP[viewport].MapX, TheUI.VP[viewport].MapY + 1);
	    }
	}

	//
	//	Scroll the map, if cursor moves outside the viewport.
	//
	RestrictCursorToViewport();

	if (EditorState == EditorEditTile) {
	    EditTiles(Viewport2MapX(TheUI.LastClickedVP, CursorX),
		Viewport2MapY(TheUI.LastClickedVP, CursorY), TileCursor,
		TileCursorSize);
	} else if (EditorState == EditorEditUnit && CursorBuilding) {
	    if (CanBuildUnitType(NULL, CursorBuilding,
		    Viewport2MapX(TheUI.LastClickedVP, CursorX),
		    Viewport2MapY(TheUI.LastClickedVP, CursorY))) {
		EditUnit(Viewport2MapX(TheUI.LastClickedVP, CursorX),
		    Viewport2MapY(TheUI.LastClickedVP, CursorY),
		    CursorBuilding, Players + SelectedPlayer);
	    }
	}

#if 0
	// FIXME: should scroll the map!
	viewport = GetViewport(x, y);
	if (viewport >= 0 && viewport == TheUI.ActiveViewport) {
	    EditTiles(Viewport2MapX(TheUI.ActiveViewport, CursorX),
		Viewport2MapY(TheUI.ActiveViewport, CursorY), TileCursor,
		TileCursorSize);
	}
#endif
	return;
    }

    //
    //	Minimap move viewpoint
    //
    if( CursorOn==CursorOnMinimap && (MouseButtons&LeftButton) ) {
	Viewport *vp;

	RestrictCursorToMinimap();
	vp = &TheUI.VP[TheUI.LastClickedVP];
	MapViewportSetViewpoint (TheUI.LastClickedVP
		,ScreenMinimap2MapX (CursorX) - vp->MapWidth/2
		,ScreenMinimap2MapY (CursorY) - vp->MapHeight/2);
	return;
    }


    OldCursorOn = CursorOn;

    MouseScrollState = ScrollNone;
    GameCursor = TheUI.Point.Cursor;
    CursorOn = -1;
    CursorPlayer = -1;
    CursorUnitIndex = -1;
    ButtonUnderCursor = -1;

    //
    //	Minimap
    //
    if( x>=TheUI.MinimapX+24 && x<TheUI.MinimapX+24+MINIMAP_W
	    && y>=TheUI.MinimapY+2 && y<TheUI.MinimapY+2+MINIMAP_H ) {
	CursorOn=CursorOnMinimap;

    }

    //
    //  Handle edit unit area
    //
    if (EditorState == EditorEditUnit) {
	by = TheUI.InfoPanelY + 4 + ICON_HEIGHT + 10;
	bx = TheUI.InfoPanelX + 16;
	for( i = 0; i< PlayerMax; ++i ) {
	    if( i == PlayerMax / 2 ) {
		bx = TheUI.InfoPanelX + 16;
		by += 18;
	    }
	    if (bx < x && x < bx + 15 && by < y && y < by + 15) {
		sprintf(buf,"Select player #%d",i);
		SetStatusLine(buf);
		CursorPlayer = i;
		//ButtonUnderCursor = i + 100;
		//CursorOn = CursorOnButton;
		return;
	    }
	    bx += 18;
	}

	i = UnitIndex;
	by = TheUI.ButtonPanelY + 24;
	while (by < TheUI.ButtonPanelY
		+ TheUI.ButtonPanel.Graphic->Height - ICON_HEIGHT) {
	    if( !EditorUnitTypes[i] ) {
		break;
	    }
	    bx = TheUI.ButtonPanelX + 10;
	    while (bx < TheUI.ButtonPanelX + 146) {
		if( !EditorUnitTypes[i] ) {
		    break;
		}
		if (bx < x && x < bx + ICON_WIDTH
			&& by < y && y < by + ICON_HEIGHT) {
		    sprintf(buf,"%s \"%s\"",
			    UnitTypeByIdent(EditorUnitTypes[i])->Ident,
			    UnitTypeByIdent(EditorUnitTypes[i])->Name);
		    SetStatusLine(buf);
		    CursorUnitIndex = i;
		    //ButtonUnderCursor = i + 100;
		    //CursorOn = CursorOnButton;
		    return;
		}
		bx += ICON_WIDTH + 8;
		i++;
	    }
	    by += ICON_HEIGHT + 2;
	}
    }

    //
    //  Handle tile area
    //
    if (EditorState == EditorEditTile) {
	i = 0;
	bx = TheUI.InfoPanelX + 4;
	by = TheUI.InfoPanelY + 4 + ICON_HEIGHT + 10;

	while( i < 6 ) {
	    if (bx < x && x < bx + 100 && by < y && y < by + 18) {
		ButtonUnderCursor = i + 300;
		CursorOn = CursorOnButton;
		return;
	    }
	    ++i;
	    by += 20;
	}

	i = 0;
	by = TheUI.ButtonPanelY + 4;
	while (by < TheUI.ButtonPanelY + 100) {
	    bx = TheUI.ButtonPanelX + 4;
	    while (bx < TheUI.ButtonPanelX + 144) {
		if (bx < x && x < bx + 32 && by < y && y < by + 32) {
		    int j;

		    // FIXME: i is wrong, must find the solid type
		    j = TheMap.Tileset->BasicNameTable[i * 16 + 16];
		    SetStatusLine(TheMap.Tileset->TileNames[j]);
		    ButtonUnderCursor = i + 100;
		    CursorOn = CursorOnButton;
		    return;
		}
		bx += 34;
		i++;
	    }
	    by += 34;
	}
    }

    //
    //  Handle buttons
    //
    if (TheUI.InfoPanelX + 4 < CursorX
	    && CursorX < TheUI.InfoPanelX + 4 + ICON_WIDTH+7
	    && TheUI.InfoPanelY + 4 < CursorY
	    && CursorY < TheUI.InfoPanelY + 4 + ICON_HEIGHT+7) {
	// FIXME: what is this button?
	ButtonUnderCursor = SelectButton;
	CursorOn = CursorOnButton;
	SetStatusLine("Select mode");
	return;
    }
    if (TheUI.InfoPanelX + 4 + UNIT_ICON_X < CursorX
	    && CursorX < TheUI.InfoPanelX + 4 + UNIT_ICON_X + ICON_WIDTH+7
	    && TheUI.InfoPanelY + 4 + UNIT_ICON_Y < CursorY
	    && CursorY < TheUI.InfoPanelY + 4 + UNIT_ICON_Y + ICON_HEIGHT+7) {
	ButtonUnderCursor = UnitButton;
	CursorOn = CursorOnButton;
	SetStatusLine("Unit mode");
	return;
    }
    if (TheUI.InfoPanelX + 4 + TILE_ICON_X < CursorX
	    && CursorX < TheUI.InfoPanelX + 4 + TILE_ICON_X + TILE_WIDTH+7
	    && TheUI.InfoPanelY + 4 + TILE_ICON_Y < CursorY
	    && CursorY < TheUI.InfoPanelY + 4 + TILE_ICON_Y + TILE_HEIGHT+7) {
	ButtonUnderCursor = TileButton;
	CursorOn = CursorOnButton;
	SetStatusLine("Tile mode");
	return;
    }
    for (i = 0; i < sizeof(TheUI.Buttons) / sizeof(*TheUI.Buttons); ++i) {
	if (x < TheUI.Buttons[i].X
		|| x > TheUI.Buttons[i].X + TheUI.Buttons[i].Width
		|| y < TheUI.Buttons[i].Y
		|| y > TheUI.Buttons[i].Y + TheUI.Buttons[i].Height) {
	    continue;
	}
	DebugLevel3("On button %d\n" _C_ i);
	ButtonUnderCursor = i;
	CursorOn = CursorOnButton;
	return;
    }

    //
    //  Minimap
    //
    if (x >= TheUI.MinimapX + 24 && x < TheUI.MinimapX + 24 + MINIMAP_W
	    && y >= TheUI.MinimapY + 2 && y < TheUI.MinimapY + 2 + MINIMAP_H) {
	CursorOn = CursorOnMinimap;
	return;
    }
    //
    //  Map
    //
    if (x >= TheUI.MapArea.X && x <= TheUI.MapArea.EndX && y >= TheUI.MapArea.Y
	    && y <= TheUI.MapArea.EndY) {
	viewport = GetViewport(x, y);
	if (viewport >= 0 && viewport != TheUI.ActiveViewport) {
	    TheUI.ActiveViewport = viewport;
	    DebugLevel0Fn("active viewport changed to %d.\n" _C_ viewport);
	}
	CursorOn = CursorOnMap;
    }
    //
    //	Look if there is an unit under the cursor.
    //
    if (CursorOn == CursorOnMap) {
	viewport = TheUI.ActiveViewport;
	UnitUnderCursor = UnitOnScreen(NULL,
	    CursorX - TheUI.VP[viewport].X
		+ TheUI.VP[viewport].MapX * TileSizeX,
	    CursorY - TheUI.VP[viewport].Y
		+ TheUI.VP[viewport].MapY * TileSizeY);
	if( UnitUnderCursor ) {
	    ShowUnitInfo(UnitUnderCursor);
	    return;
	}
    } else {
	UnitUnderCursor = NULL;
    }
    //
    //  Scrolling Region Handling
    //
    if (HandleMouseScrollArea(x,y)) {
	return;
    }

    //  Not reached if cursor is inside the scroll area

    ClearStatusLine();
}

/**
**	Callback for exit.
*/
local void EditorCallbackExit(void)
{
    DebugLevel3Fn("Exit\n");
}

/**
**	Create editor.
*/
local void CreateEditor(void)
{
    int i;
    int n;
    char *file;
    char *s;
    char buf[PATH_MAX];
    CLFile *clf;
    extern LISP fast_load(LISP lfname, LISP noeval);

    //
    //  Load and evaluate the editor configuration file
    //  FIXME: the CLopen is very slow and repeats the work of LibraryFileName.
    //
    file = LibraryFileName(EditorStartFile, buf);
    if ((clf = CLopen(file))) {
	CLclose(clf);
	ShowLoadProgress("Script %s\n", file);
	if ((s = strrchr(file, '.')) && s[1] == 'C') {
	    fast_load(gh_str02scm(file), NIL);
	} else {
	    vload(file, 0, 1);
	}
	user_gc(SCM_BOOL_F);		// Cleanup memory after load
    }

    FlagRevealMap = 1;			// editor without fog and all visible
    TheMap.NoFogOfWar = 1;
    if (!*CurrentMapPath) {		// new map!
	InitUnitTypes();
	UpdateStats();

	//
	//      Inititialize TheMap / Players.
	//
	DebugCheck(TheMap.Info);

	TheMap.Info = calloc(1, sizeof(MapInfo));
	TheMap.Info->Description = strdup("http://FreeCraft.Org");
	TheMap.Info->MapTerrain = 0;
	TheMap.Info->MapTerrainName =
	    strdup(Tilesets[TheMap.Info->MapTerrain]->Ident);
	TheMap.Info->MapWidth = TheMap.Info->MapHeight = 256;

	for (i = 0; i < PlayerMax; ++i) {
	    if (i == PlayerNumNeutral) {
		CreatePlayer(PlayerNeutral);
		TheMap.Info->PlayerType[i] = PlayerNeutral;
	    } else {
		CreatePlayer(PlayerNobody);
		TheMap.Info->PlayerType[i] = PlayerNobody;
	    }
	    TheMap.Info->PlayerGold[i] = Players[i].Resources[GoldCost];
	    TheMap.Info->PlayerWood[i] = Players[i].Resources[WoodCost];
	    TheMap.Info->PlayerOil[i] = Players[i].Resources[OilCost];
	}

	strncpy(TheMap.Description, TheMap.Info->Description, 32);
	TheMap.Width = TheMap.Info->MapWidth;
	TheMap.Height = TheMap.Info->MapHeight;
	TheMap.Fields = calloc(TheMap.Width * TheMap.Height, sizeof(MapField));
	TheMap.Visible[0] = calloc(TheMap.Width * TheMap.Height / 8, 1);
	InitUnitCache();

	TheMap.Terrain = TheMap.Info->MapTerrain;
	TheMap.TerrainName = strdup(Tilesets[TheMap.Info->MapTerrain]->Ident);
	TheMap.Tileset = Tilesets[TheMap.Info->MapTerrain];

	for (i = 0; i < TheMap.Width * TheMap.Height; ++i) {
	    TheMap.Fields[i].Tile = TheMap.Fields[i].SeenTile = 0;
	    TheMap.Fields[i].Tile = TheMap.Fields[i].SeenTile =
		TheMap.Tileset->Table[16];
	    TheMap.Fields[i].Flags = TheMap.Tileset->FlagsTable[16];
	}
	GameSettings.Resources = SettingsResourcesMapDefault;
	CreateGame(NULL, &TheMap);
    } else {
	CreateGame(CurrentMapPath, &TheMap);
    }
    FlagRevealMap = 0;

    //
    //	Place the start points, which the loader discarded.
    //
    for (i = 0; i < PlayerMax; ++i) {
	if (Players[i].Type != PlayerNobody) {
	    // FIXME: must support more races
	    switch (Players[i].Race) {
		case PlayerRaceHuman:
		    MakeUnitAndPlace(Players[i].StartX, Players[i].StartY,
			UnitTypeByWcNum(WC_StartLocationHuman), Players + i);
		    break;
		case PlayerRaceOrc:
		    MakeUnitAndPlace(Players[i].StartX, Players[i].StartY,
			UnitTypeByWcNum(WC_StartLocationOrc), Players + i);
		    break;
	    }
	} else if (Players[i].StartX | Players[i].StartY) {
	    DebugLevel0Fn("Player nobody has a start position\n");
	}
    }

    if (!EditorUnitTypes) {
	//
	//  Build editor unit-type tables.
	//
	i = 0;
	while (UnitTypeWcNames[i]) {
	    ++i;
	}
	n = i + 1;
	EditorUnitTypes = malloc(sizeof(char *) * n);
	for (i = 0; i < n; ++i) {
	    EditorUnitTypes[i] = UnitTypeWcNames[i];
	}
	MaxUnitIndex = n - 1;
    }

    if (1) {
	ProcessMenu("menu-editor-tips", 1);
	InterfaceState = IfaceStateNormal;
    }
}

/**
**	Save a pud from editor.
**
**	@param file	Save the level to this file.
**
**	@todo	FIXME: Check if the pud is valid, contains no failures.
**		Alteast two players, one human slot, every player a startpoint
**		...
*/
global void EditorSavePud(const char *file)
{
    int i;

    for (i = 0; i < NumUnits; ++i) {
	const UnitType *type;

	type = Units[i]->Type;
	if (type == UnitTypeByWcNum(WC_StartLocationHuman)
		|| type == UnitTypeByWcNum(WC_StartLocationOrc)) {
	    // FIXME: Startpoints sets the land-unit flag.
	    TheMap.Fields[Units[i]->X + Units[i]->Y * TheMap.Width].Flags &=
		~MapFieldLandUnit;
	}
    }
    SavePud(file, &TheMap);
    for (i = 0; i < NumUnits; ++i) {
	const UnitType *type;

	type = Units[i]->Type;
	if (type == UnitTypeByWcNum(WC_StartLocationHuman)
		|| type == UnitTypeByWcNum(WC_StartLocationOrc)) {
	    // FIXME: Startpoints sets the land-unit flag.
	    TheMap.Fields[Units[i]->X + Units[i]->Y * TheMap.Width].Flags |=
		MapFieldLandUnit;
	}
    }
}

/*----------------------------------------------------------------------------
--	Editor main loop
----------------------------------------------------------------------------*/

/**
**	Editor main event loop.
*/
global void EditorMainLoop(void)
{
    EventCallback callbacks;

    CreateEditor();

    SetVideoSync();

    callbacks.ButtonPressed = EditorCallbackButtonDown;
    callbacks.ButtonReleased = EditorCallbackButtonUp;
    callbacks.MouseMoved = EditorCallbackMouse;
    callbacks.MouseExit = EditorCallbackExit;
    callbacks.KeyPressed = EditorCallbackKeyDown;
    callbacks.KeyReleased = EditorCallbackKeyUp;
    callbacks.KeyRepeated = EditorCallbackKey3;

    callbacks.NetworkEvent = NetworkEvent;
    callbacks.SoundReady = WriteSound;

    GameCursor = TheUI.Point.Cursor;
    InterfaceState = IfaceStateNormal;
    EditorState = EditorSelecting;
    TheUI.LastClickedVP = 0;
    TileCursorSize = 1;

    EditorRunning = 1;
    while (EditorRunning) {
	EditorUpdateDisplay();

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
	    ColorCycle();
	}

	WaitEventsOneFrame(&callbacks);
    }

    //
    //	Restore all for menu
    //
    CleanModules();
    CleanFonts();

    LoadCcl();			// Reload the main config file

    PreMenuSetup();

    InterfaceState = IfaceStateMenu;
    GameCursor = TheUI.Point.Cursor;

    VideoLockScreen();
    VideoClearScreen();
    VideoUnlockScreen();
    Invalidate();
}

local void paul(void)
{
}

//@}
