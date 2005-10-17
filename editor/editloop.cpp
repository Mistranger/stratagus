//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name editloop.cpp - The editor main loop. */
//
//      (c) Copyright 2002-2005 by Lutz Sammer and Jimmy Salmon
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
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
//      $Id$

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "stratagus.h"
#include "unittype.h"
#include "video.h"
#include "map.h"
#include "tileset.h"
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
#include "iolib.h"
#include "iocompat.h"
#include "commands.h"

#include "script.h"

extern void DoScrollArea(int state, int fast);

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

#define UNIT_ICON_X (IconWidth + 7)       /// Unit mode icon
#define UNIT_ICON_Y (0)                   /// Unit mode icon
#define TILE_ICON_X (IconWidth * 2 + 16)  /// Tile mode icon
#define TILE_ICON_Y (2)                   /// Tile mode icon
#define START_ICON_X (IconWidth * 3 + 16)  /// Start mode icon
#define START_ICON_Y (2)                   /// Start mode icon

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

static int IconWidth;                       /// Icon width in panels
static int IconHeight;                      /// Icon height in panels


char EditorMapLoaded;  /// Map loaded in editor
int TerrainEditable = 1;  /// Is the terrain editable ?  Defaults to yes.
int EditorWriteCompressedMaps = 1;

EditorStateType EditorState;                /// Current editor state.
EditorRunningType EditorRunning;  /// Running State of editor.

static char TileToolRandom;      /// Tile tool draws random
static char TileToolDecoration;  /// Tile tool draws with decorations
static int TileCursorSize;       /// Tile cursor size 1x1 2x2 ... 4x4
static int TileCursor;           /// Tile type number

static int MirrorEdit = 0;           /// Mirror editing enabled
static int UnitPlacedThisPress = 0;  ///Only allow one unit per press

enum _mode_buttons_ {
	SelectButton = 201,  /// Select mode button
	UnitButton,          /// Unit mode button
	TileButton,          /// Tile mode button
	StartButton
};

char **EditorUnitTypes;  /// Sorted editor unit-type table
int MaxUnitIndex;        /// Max unit icon draw index

static char **ShownUnitTypes;       /// Shown editor unit-type table
static int MaxShownUnits;           /// Max unit icon draw index
static char ShowUnitsToSelect;      /// Show units in unit list
static char ShowBuildingsToSelect;  /// Show buildings in unit list
#if 0
static char ShowHeroesToSelect;     /// Show heroes in unit list
#endif
static char ShowAirToSelect;        /// Show air units in unit list
static char ShowLandToSelect;       /// Show land units in unit list
static char ShowWaterToSelect;      /// Show water units in unit list

static int UnitIndex;               /// Unit icon draw index
static int CursorUnitIndex;         /// Unit icon under cursor
static int SelectedUnitIndex;       /// Unit type to draw

static int CursorPlayer;            /// Player under the cursor
static int SelectedPlayer;          /// Player selected for draw

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Edit
----------------------------------------------------------------------------*/

/**
**  Get tile number.
**
**  @param basic   Basic tile number
**  @param random  Return random tile
**  @param filler  Get a decorated tile.
**
**  @return        Tile number used in pud.
**
**  @todo  FIXME: Solid tiles are here still hardcoded.
*/
int GetTileNumber(int basic, int random, int filler)
{
	int tile;
	int i;
	int n;

	tile = 16 + basic * 16;
	if (random) {
		for (n = i = 0; i < 16; ++i) {
			if (!Map.Tileset.Table[tile + i]) {
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
			while (++i < 16 && !Map.Tileset.Table[tile + i]) {
			}
		} while (i < 16 && n--);
		Assert(i != 16);
		return tile + i;
	}
	if (filler) {
		for (i = 0; i < 16 && Map.Tileset.Table[tile + i]; ++i) {
		}
		for (; i < 16 && !Map.Tileset.Table[tile + i]; ++i) {
		}
		if (i != 16) {
			return tile + i;
		}
	}
	return tile;
}

/**
**  Edit tile.
**
**  @param x     X map tile coordinate.
**  @param y     Y map tile coordinate.
**  @param tile  Tile type to edit.
*/
void EditTile(int x, int y, int tile)
{
	CMapField *mf;

	Assert(x >= 0 && y >= 0 && x < Map.Info.MapWidth && y < Map.Info.MapHeight);

	ChangeTile(x, y, GetTileNumber(tile, TileToolRandom, TileToolDecoration));

	//
	// Change the flags
	//
	mf = &Map.Fields[y * Map.Info.MapWidth + x];
	mf->Flags &= ~(MapFieldHuman | MapFieldLandAllowed | MapFieldCoastAllowed |
		MapFieldWaterAllowed | MapFieldNoBuilding | MapFieldUnpassable |
		MapFieldWall | MapFieldRocks | MapFieldForest);

	mf->Flags |= Map.Tileset.FlagsTable[16 + tile * 16];

	UI.Minimap.UpdateSeenXY(x, y);
	UI.Minimap.UpdateXY(x, y);

	EditorTileChanged(x, y);
}

/**
**  Edit tiles (internal, used by EditTiles()).
**
**  @param x     X map tile coordinate.
**  @param y     Y map tile coordinate.
**  @param tile  Tile type to edit.
**  @param size  Size of rectangle
**
**  @bug  This function does not support mirror editing!
*/
void EditTilesInternal(int x, int y, int tile, int size)
{
	int ex;
	int ey;
	int i;

	ex = x + size;
	if (x < 0) {
		x = 0;
	}
	if (ex > Map.Info.MapWidth) {
		ex = Map.Info.MapWidth;
	}
	ey = y + size;
	if (y < 0) {
		y = 0;
	}
	if (ey > Map.Info.MapHeight) {
		ey = Map.Info.MapHeight;
	}
	while (y < ey) {
		for (i = x; i < ex; ++i) {
			EditTile(i, y, tile);
		}
		++y;
	}
}

/**
**  Edit tiles
**
**  @param x     X map tile coordinate.
**  @param y     Y map tile coordinate.
**  @param tile  Tile type to edit.
**  @param size  Size of rectangle
*/
void EditTiles(int x, int y, int tile, int size)
{
	int mx;
	int my;

	mx = Map.Info.MapWidth;
	my = Map.Info.MapHeight;

	EditTilesInternal(x, y, tile, size);

	if (!MirrorEdit) {
		return;
	}

	EditTilesInternal(mx - x - size, y, tile, size);

	if (MirrorEdit == 1) {
		return;
	}

	EditTilesInternal(x, my - y - size, tile, size);
	EditTilesInternal(mx - x - size, my - y - size, tile, size);
}

/**
**  Edit unit (internal, used by EditUnit()).
**
**  @param x       X map tile coordinate.
**  @param y       Y map tile coordinate.
**  @param type    Unit type to edit.
**  @param player  Player owning the unit.
**
**  @todo  FIXME: Check if the player has already a start-point.
**  @bug   This function does not support mirror editing!
*/
static void EditUnitInternal(int x, int y, CUnitType *type, CPlayer *player)
{
	CUnit *unit;
	CBuildRestrictionOnTop *b;

	// FIXME: vladi: should check place when mirror editing is enabled...?
	unit = MakeUnitAndPlace(x, y, type, player);
	b = OnTopDetails(unit, NULL);
	if (b && b->ReplaceOnBuild) {
		int n;
		CUnit *table[UnitMax];

		n = UnitCacheOnTile(x, y, table);
		while (n--) {
			if (table[n]->Type == b->Parent) {
				unit->ResourcesHeld = table[n]->ResourcesHeld; // We capture the value of what is beneath.
				table[n]->Remove(NULL); // Destroy building beneath
				UnitLost(table[n]);
				UnitClearOrders(table[n]);
				table[n]->Release();
				break;
			}
		}

	}
	if (unit != NoUnitP) {
		if (type->GivesResource) {
			unit->ResourcesHeld = DefaultResourceAmounts[type->GivesResource];
		}
	} else {
		DebugPrint("Unable to allocate Unit");
	}
}

/**
**  Edit unit.
**
**  @param x       X map tile coordinate.
**  @param y       Y map tile coordinate.
**  @param type    Unit type to edit.
**  @param player  Player owning the unit.
**
**  @todo  FIXME: Check if the player has already a start-point.
*/
static void EditUnit(int x, int y, CUnitType *type, CPlayer *player)
{
	int mx;
	int my;

	mx = Map.Info.MapWidth;
	my = Map.Info.MapHeight;

	EditUnitInternal(x, y, type, player);

	if (!MirrorEdit) {
		return;
	}

	EditUnitInternal(mx - x - 1, y, type, player);

	if (MirrorEdit == 1) {
		return;
	}

	EditUnitInternal(x, my - y - 1, type, player);
	EditUnitInternal(mx - x - 1, my - y - 1, type, player);
}

/**
**  Calculate the number of unit icons that can be displayed
**
**  @return  Number of unit icons that can be displayed.
*/
static int CalculateUnitIcons(void)
{
	int i;
	int x;
	int count;

	i = 0;
	count = 0;
	x = UI.ButtonPanel.Y + 24;
	while (x < UI.ButtonPanel.Y + UI.ButtonPanel.G->Height - IconHeight) {
		++i;
		x += IconHeight + 2;
	}
	x = UI.ButtonPanel.X + 10;
	while (x < UI.ButtonPanel.X + UI.ButtonPanel.G->Width - IconWidth) {
		count += i;
		x += IconWidth + 8;
	}
	return count;
}

/**
**  Calculate the max height and the max widht of icons,
**  and assign them to IconHeight and IconWidth
*/
static void CalculateMaxIconSize(void)
{
	int i;
	const CUnitType *type;
	const CIcon *icon;

	IconWidth = 0;
	IconHeight = 0;
	for (i = 0; i < MaxUnitIndex; ++i) {
		type = UnitTypeByIdent(EditorUnitTypes[i]);
		Assert(type && type->Icon.Icon);
		icon = type->Icon.Icon;
		if (IconWidth < icon->G->Width) {
			IconWidth = icon->G->Width;
		}
		if (IconHeight < icon->G->Height) {
			IconHeight = icon->G->Height;
		}
	}
}


/**
**  Recalculate the shown units.
*/
static void RecalculateShownUnits(void)
{
	int i;
	int n;
	const CUnitType *type;

	if (!ShownUnitTypes) {
		ShownUnitTypes = new char *[MaxUnitIndex];
	}

	for (n = i = 0; i < MaxUnitIndex; ++i) {
		type = UnitTypeByIdent(EditorUnitTypes[i]);

		if (type->Building && !ShowBuildingsToSelect) {
			continue;
		}
		if (!type->Building && !ShowUnitsToSelect) {
			continue;
		}
#if 0
		if (type->Hero && !ShowHeroesToSelect) {
			continue;
		}
#endif
		if (type->UnitType == UnitTypeLand && !ShowLandToSelect) {
			continue;
		}
		if (type->UnitType == UnitTypeNaval && !ShowWaterToSelect) {
			continue;
		}
		if (type->UnitType == UnitTypeFly && !ShowAirToSelect) {
			continue;
		}

		ShownUnitTypes[n++] = EditorUnitTypes[i];
	}
	MaxShownUnits = n;

	if (UnitIndex >= MaxShownUnits) {
		int count;

		count = CalculateUnitIcons();
		UnitIndex = MaxShownUnits / count * count;
	}
	// Quick & dirty make them invalid
	CursorUnitIndex = -1;
	SelectedUnitIndex = -1;
}

/*----------------------------------------------------------------------------
--  Display
----------------------------------------------------------------------------*/

#define MBUTTON_GEM_SQUARE  24

/**
**  Draw tile icons.
**
**  @todo for the start the solid tiles are hardcoded
**        If we have more solid tiles, than they fit into the panel, we need
**        some new ideas.
*/
static void DrawTileIcons(void)
{
	int x;
	int y;
	int i;

	x = UI.InfoPanel.X + 46;
	y = UI.InfoPanel.Y + 4 + IconHeight + 11;

	if (CursorOn == CursorOnButton &&
			ButtonUnderCursor >= 300 && ButtonUnderCursor < 306) {
		Video.DrawRectangle(ColorGray, x - 42,
				y - 3 + (ButtonUnderCursor - 300) * 20, 100, 20);
	}

	VideoDrawTextCentered(x, y, GameFont, "1x1");
	MenuButtonG->DrawFrame(MBUTTON_GEM_SQUARE + (TileCursorSize == 1 ? 2 : 0), x + 40, y - 3);
	y += 20;
	VideoDrawTextCentered(x, y, GameFont, "2x2");
	MenuButtonG->DrawFrame(MBUTTON_GEM_SQUARE + (TileCursorSize == 2 ? 2 : 0), x + 40, y - 3);
	y += 20;
	VideoDrawTextCentered(x, y, GameFont, "3x3");
	MenuButtonG->DrawFrame(MBUTTON_GEM_SQUARE + (TileCursorSize == 3 ? 2 : 0), x + 40, y - 3);
	y += 20;
	VideoDrawTextCentered(x, y, GameFont, "4x4");
	MenuButtonG->DrawFrame(MBUTTON_GEM_SQUARE + (TileCursorSize == 4 ? 2 : 0), x + 40, y - 3);
	y += 20;
	VideoDrawTextCentered(x, y, GameFont, "Random");
	MenuButtonG->DrawFrame(MBUTTON_GEM_SQUARE + (TileToolRandom ? 2 : 0), x + 40, y - 3);
	y += 20;
	VideoDrawTextCentered(x, y, GameFont, "Filler");
	MenuButtonG->DrawFrame(MBUTTON_GEM_SQUARE + (TileToolDecoration ? 2 : 0), x + 40, y - 3);
	y += 20;

	y = UI.ButtonPanel.Y + 4;
	i = 0;

	while (y < UI.ButtonPanel.Y + 100) {
		x = UI.ButtonPanel.X + 4;
		while (x < UI.ButtonPanel.X + 144) {
			if (!Map.Tileset.Tiles[0x10 + i * 16].BaseTerrain) {
				y = UI.ButtonPanel.Y + 100;
				break;
			}
			Map.TileGraphic->DrawFrameClip(Map.Tileset.Table[0x10 + i * 16], x, y);
			Video.DrawRectangle(ColorGray, x, y, TileSizeX, TileSizeY);
			if (TileCursor == i) {
				Video.DrawRectangleClip(ColorGreen, x + 1, y + 1, TileSizeX-2, TileSizeY-2);

			}
			if (CursorOn == CursorOnButton && ButtonUnderCursor == i + 100) {
				Video.DrawRectangle(ColorWhite, x - 1, y - 1, TileSizeX+2, TileSizeY+2);
			}
			x += TileSizeX+2;
			++i;
		}
		y += TileSizeY+2;
	}
}

/**
**  Draw a table with the players
*/
static void DrawPlayers(void) 
{
	int x;
	int y;
	int i;
	char buf[256];

	x = UI.InfoPanel.X + 8;
	y = UI.InfoPanel.Y + 4 + IconHeight + 10;

	for (i = 0; i < PlayerMax; ++i) {
		if (i == PlayerMax / 2) {
			y += 20;
		}
		if (i == CursorPlayer && Map.Info.PlayerType[i] != PlayerNobody) {
			Video.DrawRectangle(ColorWhite, x + i % 8 * 20, y, 20, 20);
		}
		Video.DrawRectangle(
			i == CursorPlayer && Map.Info.PlayerType[i] != PlayerNobody ?
				ColorWhite : ColorGray,
			x + i % 8 * 20, y, 19, 19);
		if (Map.Info.PlayerType[i] != PlayerNobody) {
			Video.FillRectangle(Players[i].Color, x + 1 + i % 8 * 20, y + 1,
				17, 17);
		}
		if (i == SelectedPlayer) {
			Video.DrawRectangle(ColorGreen, x + 1 + i % 8 * 20, y + 1, 17, 17);
		}
		sprintf(buf, "%d", i);
		VideoDrawTextCentered(x + i % 8 * 20 + 10, y + 7, SmallFont, buf);
	}
	
	x = UI.InfoPanel.X + 4;
	y += 18 * 1 + 4;
	if (SelectedPlayer != -1) {
		i = sprintf(buf,"Plyr %d %s ", SelectedPlayer,
				PlayerRaces.Name[Map. Info.PlayerSide[SelectedPlayer]]);
		// Players[SelectedPlayer].RaceName);

		switch (Map.Info.PlayerType[SelectedPlayer]) {
			case PlayerNeutral:
				strcat(buf, "Neutral");
				break;
			case PlayerNobody:
			default:
				strcat(buf, "Nobody");
				break;
			case PlayerPerson:
				strcat(buf, "Person");
				break;
			case PlayerComputer:
			case PlayerRescuePassive:
			case PlayerRescueActive:
				strcat(buf, "Computer");
				break;
		}

		VideoDrawText(x, y, GameFont, buf);
	}
}


/**
**  Draw unit icons.
*/
static void DrawUnitIcons(void)
{
	int x;
	int y;
	int i;
	int j;
	int percent;
	CIcon *icon;

	//
	// Draw the unit selection buttons.
	//
	x = UI.InfoPanel.X + 10;
	y = UI.InfoPanel.Y + 140;

	VideoDrawText(x + 28 * 0, y, GameFont, "Un");
	MenuButtonG->DrawFrame(MBUTTON_GEM_SQUARE + (ShowUnitsToSelect ? 2 : 0),
		x + 28 * 0, y + 16);
	VideoDrawText(x + 28 * 1, y, GameFont, "Bu");
	MenuButtonG->DrawFrame(MBUTTON_GEM_SQUARE + (ShowBuildingsToSelect ? 2 : 0),
		x + 28 * 1, y + 16);
#if 0
	VideoDrawText(x + 28 * 2, y, GameFont, "He");
	MenuButtonG->DrawFrame(MBUTTON_GEM_SQUARE + (ShowHeroesToSelect ? 2 : 0),
		x + 28 * 2, y + 16);
#endif
	VideoDrawText(x + 28 * 3, y, GameFont, "La");
	MenuButtonG->DrawFrame(MBUTTON_GEM_SQUARE + (ShowLandToSelect ? 2 : 0),
		x + 28 * 3, y + 16);
	VideoDrawText(x + 28 * 4, y, GameFont, "Wa");
	MenuButtonG->DrawFrame(MBUTTON_GEM_SQUARE + (ShowWaterToSelect ? 2 : 0),
		x + 28 * 4, y + 16);
	VideoDrawText(x + 28 * 5, y, GameFont, "Ai");
	MenuButtonG->DrawFrame(MBUTTON_GEM_SQUARE + (ShowAirToSelect ? 2 : 0),
		x + 28 * 5, y + 16);

	//
	// Scroll bar for units. FIXME: drag not supported.
	//
	x = UI.ButtonPanel.X + 4;
	y = UI.ButtonPanel.Y + 4;
	j = 176 - 8;

	PushClipping();
	SetClipping(0, 0, x + j - 20, Video.Height - 1);
	MenuButtonG->DrawFrameClip(MBUTTON_S_HCONT, x - 2, y);
	PopClipping();
	if (UI.ButtonPanel.X + 4 < CursorX
			&& CursorX < UI.ButtonPanel.X + 24
			&& UI.ButtonPanel.Y + 4 < CursorY
			&& CursorY < UI.ButtonPanel.Y + 24
			&& MouseButtons & LeftButton) {
		MenuButtonG->DrawFrame(MBUTTON_LEFT_ARROW + 1, x - 2, y);
	} else {
		MenuButtonG->DrawFrame(MBUTTON_LEFT_ARROW, x - 2, y);
	}
	if (UI.ButtonPanel.X + 176 - 24 < CursorX
			&& CursorX < UI.ButtonPanel.X + 176 - 4
			&& UI.ButtonPanel.Y + 4 < CursorY
			&& CursorY < UI.ButtonPanel.Y + 24
			&& MouseButtons & LeftButton) {
		MenuButtonG->DrawFrame(MBUTTON_RIGHT_ARROW + 1, x + j - 20, y);
	} else {
		MenuButtonG->DrawFrame(MBUTTON_RIGHT_ARROW, x + j - 20, y);
	}

	percent = UnitIndex * 100 / (MaxShownUnits ? MaxShownUnits : 1);
	i = (percent * (j - 54)) / 100;
	MenuButtonG->DrawFrame(MBUTTON_S_KNOB, x + 18 + i, y + 1);

	//
	//  Draw the unit icons.
	//
	y = UI.ButtonPanel.Y + 24;
	i = UnitIndex;
	while (y < UI.ButtonPanel.Y + UI.ButtonPanel.G->Height
			- IconHeight) {
		if (i >= MaxShownUnits) {
			break;
		}
		x = UI.ButtonPanel.X + 10;
		while (x < UI.ButtonPanel.X + UI.ButtonPanel.G->Width
				- IconWidth) {
			if (i >= MaxShownUnits) {
				break;
			}
			icon = UnitTypeByIdent(ShownUnitTypes[i])->Icon.Icon;
			icon->DrawIcon(Players + SelectedPlayer, x, y);

			Video.DrawRectangleClip(ColorGray, x, y, icon->G->Width, icon->G->Height);
			if (i == SelectedUnitIndex) {
				Video.DrawRectangleClip(ColorGreen, x + 1, y + 1,
					icon->G->Width - 2, icon->G->Height - 2);
			}
			if (i == CursorUnitIndex) {
				Video.DrawRectangleClip(ColorWhite,x - 1, y - 1,
					icon->G->Width + 2, icon->G->Height + 2);
			}

			x += IconWidth + 8;
			++i;
		}
		y += IconHeight + 2;
	}
}

/**
**  Draw a tile icon
**
**  @param tilenum  Tile number to display
**  @param x        X display position
**  @param y        Y display position
**  @param flags    State of the icon (::IconActive,::IconClicked,...)
*/
static void DrawTileIcon(unsigned tilenum, unsigned x, unsigned y, unsigned flags)
{
	Uint32 color;

	color = (flags & IconActive) ? ColorGray : ColorBlack;

	Video.DrawRectangleClip(color, x, y, TileSizeX + 7, TileSizeY + 7);
	Video.DrawRectangleClip(ColorBlack, x + 1, y + 1, TileSizeX + 5, TileSizeY + 5);

	Video.DrawVLine(ColorGray, x + TileSizeX + 4, y + 5, TileSizeY - 1); // _|
	Video.DrawVLine(ColorGray, x + TileSizeX + 5, y + 5, TileSizeY - 1);
	Video.DrawHLine(ColorGray, x + 5, y + TileSizeY + 4, TileSizeX + 1);
	Video.DrawHLine(ColorGray, x + 5, y + TileSizeY + 5, TileSizeX + 1);

	color = (flags & IconClicked) ? ColorGray : ColorWhite;
	Video.DrawHLine(color, x + 5, y + 3, TileSizeX + 1);
	Video.DrawHLine(color, x + 5, y + 4, TileSizeX + 1);
	Video.DrawVLine(color, x + 3, y + 3, TileSizeY + 3);
	Video.DrawVLine(color, x + 4, y + 3, TileSizeY + 3);

	if (flags & IconClicked) {
		++x;
		++y;
	}

	x += 4;
	y += 4;
	Map.TileGraphic->DrawFrameClip(Map.Tileset.Table[tilenum], x, y);

	if (flags & IconSelected) {
		Video.DrawRectangleClip(ColorGreen, x, y, TileSizeX, TileSizeY);
	}
}

/**
**  Draw the editor panels.
*/
static void DrawEditorPanel(void)
{
	int x;
	int y;
	CIcon *icon;

	x = UI.InfoPanel.X + 4;
	y = UI.InfoPanel.Y + 4;

	//
	// Select / Units / Tiles
	//
	icon = IconByIdent(EditorSelectIcon);
	Assert(icon);
	// FIXME: wrong button style
	icon->DrawUnitIcon(Players, UI.SingleSelectedButton->Style,
		(ButtonUnderCursor == SelectButton ? IconActive : 0) |
			(EditorState == EditorSelecting ? IconSelected : 0),
		x, y, NULL);
	icon = IconByIdent(EditorUnitsIcon);
	Assert(icon);
	// FIXME: wrong button style
	icon->DrawUnitIcon(Players, UI.SingleSelectedButton->Style,
		(ButtonUnderCursor == UnitButton ? IconActive : 0) |
			(EditorState == EditorEditUnit ? IconSelected : 0),
		x + UNIT_ICON_X, y + UNIT_ICON_Y, NULL);

	if (TerrainEditable) {
		DrawTileIcon(0x10 + 4 * 16, x + TILE_ICON_X, y + TILE_ICON_Y,
			(ButtonUnderCursor == TileButton ? IconActive : 0) |
				(EditorState == EditorEditTile ? IconSelected : 0));
	}

	if (EditorStartUnit) {
		icon = UnitTypeByIdent(EditorStartUnit)->Icon.Icon;
		Assert(icon);
		icon->DrawUnitIcon(Players, UI.SingleSelectedButton->Style,
			(ButtonUnderCursor == StartButton ? IconActive : 0) |
				(EditorState == EditorSetStartLocation ? IconSelected : 0),
			x + START_ICON_X, y + START_ICON_Y, NULL);
	} else {
		//  No unit specified.
		//  Todo : FIXME Should we just warn user to define Start unit ?
		PushClipping();
		x += START_ICON_X + 1;
		y += START_ICON_Y + 1;
		if (ButtonUnderCursor == StartButton) {
			Video.DrawRectangleClip(ColorGray, x - 1, y - 1, IconHeight, IconHeight);
		}
		Video.FillRectangleClip(ColorBlack, x, y, IconHeight - 2, IconHeight - 2);
		Video.DrawLineClip(PlayerColors[SelectedPlayer][0], x, y, x + IconHeight - 2, y + IconHeight - 2);
		Video.DrawLineClip(PlayerColors[SelectedPlayer][0], x, y + IconHeight - 2, x + IconHeight - 2, y);
		PopClipping();
	}

	switch (EditorState) {
		case EditorSelecting:
			break;
		case EditorEditTile:
			DrawTileIcons();
			break;
		case EditorSetStartLocation:
			DrawPlayers();
			break;
		case EditorEditUnit:
			DrawPlayers();
			DrawUnitIcons();
			break;
	}
}

/**
**  Draw special cursor on map.
**
**  @todo support for bigger cursors (2x2, 3x3 ...)
*/
static void DrawMapCursor(void)
{
	int x;
	int y;

	//
	//  Affect CursorBuilding if necessary.
	//  (Menu reset CursorBuilding)
	//
	if (!CursorBuilding) {
		switch (EditorState) {
			case EditorSelecting:
			case EditorEditTile:
				break;
			case EditorEditUnit:
				if (SelectedUnitIndex != -1) {
					CursorBuilding = UnitTypeByIdent(ShownUnitTypes[SelectedUnitIndex]);
				}
				break;
			case EditorSetStartLocation:
				if (EditorStartUnit) {
					CursorBuilding = UnitTypeByIdent(EditorStartUnit);
				}
				break;
		}
	}

	//
	// Draw map cursor
	//
	if (UI.MouseViewport && !CursorBuilding) {
		x = UI.MouseViewport->Viewport2MapX(CursorX);
		y = UI.MouseViewport->Viewport2MapY(CursorY);
		x = UI.MouseViewport->Map2ViewportX(x);
		y = UI.MouseViewport->Map2ViewportY(y);
		if (EditorState == EditorEditTile) {
			int i;
			int j;

			PushClipping();
			SetClipping(UI.MouseViewport->X, UI.MouseViewport->Y,
				UI.MouseViewport->EndX, UI.MouseViewport->EndY);
			for (j = 0; j < TileCursorSize; ++j) {
				int ty;

				ty = y + j * TileSizeY;
				if (ty >= UI.MouseViewport->EndY) {
					break;
				}
				for (i = 0; i < TileCursorSize; ++i) {
					int tx;

					tx = x + i * TileSizeX;
					if (tx >= UI.MouseViewport->EndX) {
						break;
					}
					Map.TileGraphic->DrawFrameClip(
						Map.Tileset.Table[0x10 + TileCursor * 16], tx, ty);
				}
			}
			Video.DrawRectangleClip(ColorWhite, x, y, TileSizeX * TileCursorSize,
				TileSizeY * TileCursorSize);
			PopClipping();
		} else {
			//
			// If there is an unit under the cursor, it's selection thing
			//  is drawn somewhere else (Check DrawUnitSelection.)
			//
			if (!UnitUnderCursor) {
				PushClipping();
				SetClipping(UI.MouseViewport->X, UI.MouseViewport->Y,
					UI.MouseViewport->EndX, UI.MouseViewport->EndY);
				Video.DrawRectangleClip(ColorWhite, x, y, TileSizeX, TileSizeY);
				PopClipping();
			}
		}
	}
}

/**
**  Draw the start locations of all active players on the map
*/
static void DrawStartLocations(void)
{
	int i;
	int x, y;
	CUnitType *type;

	type = NULL;
	if (EditorStartUnit) {
		type = UnitTypeByIdent(EditorStartUnit);
	}

	for (i = 0; i < PlayerMax; i++) {
		if (Map.Info.PlayerType[i] != PlayerNobody && Map.Info.PlayerType[i] != PlayerNeutral) {
			x = CurrentViewport->Map2ViewportX(Players[i].StartX);
			y = CurrentViewport->Map2ViewportY(Players[i].StartY);
			if (type) {
				DrawUnitType(type, type->Sprite, i, 0, x, y);
			} else {
				PushClipping();
				Video.DrawLineClip(PlayerColors[i][0], x, y, x + TileSizeX, y + TileSizeY);
				Video.DrawLineClip(PlayerColors[i][0], x, y + TileSizeY, x + TileSizeX, y);
				PopClipping();
			}
		}
	}
}

/**
**  Draw editor info.
**
**  If cursor is on map or minimap show information about the current tile.
*/
static void DrawEditorInfo(void)
{
#if 0
	int tile;
	int i;
	int x;
	int y;
	unsigned flags;
	char buf[256];

	x = y = 0;
	if (UI.MouseViewport) {
		x = Viewport2MapX(UI.MouseViewport, CursorX);
		y = Viewport2MapY(UI.MouseViewport, CursorY);
	}

	sprintf(buf, "Editor (%d %d)", x, y);
	VideoDrawText(UI.ResourceX + 2, UI.ResourceY + 2, GameFont, buf);

	//
	// Flags info
	//
	flags = Map.Fields[x + y * Map.Info.MapWidth].Flags;
	sprintf(buf, "%02X|%04X|%c%c%c%c%c%c%c%c%c%c%c%c%c",
		Map.Fields[x + y * Map.Info.MapWidth].Value, flags,
		flags & MapFieldUnpassable   ? 'u' : '-',
		flags & MapFieldNoBuilding   ? 'n' : '-',
		flags & MapFieldHuman        ? 'h' : '-',
		flags & MapFieldWall         ? 'w' : '-',
		flags & MapFieldRocks        ? 'r' : '-',
		flags & MapFieldForest       ? 'f' : '-',
		flags & MapFieldLandAllowed  ? 'L' : '-',
		flags & MapFieldCoastAllowed ? 'C' : '-',
		flags & MapFieldWaterAllowed ? 'W' : '-',
		flags & MapFieldLandUnit     ? 'l' : '-',
		flags & MapFieldAirUnit      ? 'a' : '-',
		flags & MapFieldSeaUnit      ? 's' : '-',
		flags & MapFieldBuilding     ? 'b' : '-');
	VideoDrawText(UI.ResourceX + 118, UI.ResourceY + 2, GameFont, buf);

	//
	// Tile info
	//
	tile = Map.Fields[x + y * Map.Info.MapWidth].Tile;

	for (i = 0; i < Map.Tileset.NumTiles; ++i) {
		if (tile == Map.Tileset.Table[i]) {
			break;
		}
	}

	Assert(i != Map.Tileset.NumTiles);

	sprintf(buf, "%d %s %s", tile,
		Map.Tileset.SolidTerrainTypes[Map.Tileset.Tiles[i].BaseTerrain].TerrainName,
		Map.Tileset.Tiles[i].MixTerrain
			? Map.Tileset.SolidTerrainTypes[Map.Tileset.Tiles[i].MixTerrain].TerrainName
			: "");

	VideoDrawText(UI.ResourceX + 252, UI.ResourceY + 2, GameFont, buf);
#endif
}

/**
**  Show info about unit.
**
**  @param unit  Unit pointer.
*/
static void ShowUnitInfo(const CUnit *unit)
{
	char buf[256];
	int i;

	i = sprintf(buf, "#%d '%s' Player:#%d %s", UnitNumber(unit),
		unit->Type->Name, unit->Player->Index,
		unit->Active ? "active" : "passive");
	if (unit->Type->GivesResource) {
		sprintf(buf + i," Amount %d", unit->ResourcesHeld);
	}
	UI.StatusLine.Set(buf);
}

/**
**  Update editor display.
*/
void EditorUpdateDisplay(void)
{
	int i;

	DrawMapArea(); // draw the map area

	DrawStartLocations();

	//
	// Fillers
	//
	for (i = 0; i < (int)UI.Fillers.size(); ++i) {
		UI.Fillers[i].G->DrawSub(0, 0,
			UI.Fillers[i].G->Width,
			UI.Fillers[i].G->Height,
			UI.Fillers[i].X, UI.Fillers[i].Y);
	}

	if (CursorOn == CursorOnMap) {
		DrawMapCursor(); // cursor on map
	}

	//
	// Menu button
	//
	DrawMenuButton(UI.MenuButton.Style,
		(ButtonAreaUnderCursor == ButtonAreaMenu
			&& ButtonUnderCursor == ButtonUnderMenu ? MI_FLAGS_ACTIVE : 0) |
		(GameMenuButtonClicked ? MI_FLAGS_CLICKED : 0),
		UI.MenuButton.X,UI.MenuButton.Y,
		UI.MenuButton.Text);

	//
	// Minimap
	//
	if (UI.SelectedViewport) {
		UI.Minimap.Draw(UI.SelectedViewport->MapX, UI.SelectedViewport->MapY);
		UI.Minimap.DrawCursor(UI.SelectedViewport->MapX,
			UI.SelectedViewport->MapY);
	}
	//
	// Info panel
	//
	if (UI.InfoPanel.G) {
		UI.InfoPanel.G->DrawSub(0, 0,
			UI.InfoPanel.G->Width, UI.InfoPanel.G->Height,
			UI.InfoPanel.X, UI.InfoPanel.Y);
	}
	//
	// Button panel
	//
	if (UI.ButtonPanel.G) {
		UI.ButtonPanel.G->DrawSub(0, 0,
			UI.ButtonPanel.G->Width,
			UI.ButtonPanel.G->Height, UI.ButtonPanel.X,
			UI.ButtonPanel.Y);
	}
	DrawEditorPanel();

	if (CursorOn == CursorOnMap) {
		DrawEditorInfo();
	}

	//
	// Status line
	//
	UI.StatusLine.Draw();

	DrawCursor();

	// FIXME: For now update everything each frame

	// refresh entire screen, so no further invalidate needed
	Invalidate();
	RealizeVideoMemory();
}

/*----------------------------------------------------------------------------
--  Input / Keyboard / Mouse
----------------------------------------------------------------------------*/

/**
**  Callback for input.
*/
static void EditorCallbackButtonUp(unsigned button)
{
	if (GameCursor == UI.Scroll.Cursor) {
		// Move map.
		GameCursor = UI.Point.Cursor; // Reset
		return;
	}

	if ((1 << button) == LeftButton && GameMenuButtonClicked == 1) {
		GameMenuButtonClicked = 0;
		if (ButtonUnderCursor == ButtonUnderMenu) {
			ProcessMenu("menu-editor", 1);
		}
	}
	if ((1 << button) == LeftButton) {
		UnitPlacedThisPress = 0;
	}
}

/**
**  Called if mouse button pressed down.
**
**  @param button  Mouse button number (0 left, 1 middle, 2 right)
*/
static void EditorCallbackButtonDown(unsigned button)
{
	//
	// Click on menu button
	//
	if (CursorOn == CursorOnButton && ButtonAreaUnderCursor == ButtonAreaMenu &&
			(MouseButtons & LeftButton) && !GameMenuButtonClicked) {
		PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
		GameMenuButtonClicked = 1;
		return;
	}
	//
	// Click on minimap
	//
	if (CursorOn == CursorOnMinimap) {
		if (MouseButtons & LeftButton) { // enter move mini-mode
			UI.SelectedViewport->Set(
				UI.Minimap.Screen2MapX(CursorX) -
					UI.SelectedViewport->MapWidth / 2,
				UI.Minimap.Screen2MapY(CursorY) -
					UI.SelectedViewport->MapHeight / 2, TileSizeX / 2, TileSizeY / 2);
		}
		return;
	}
	//
	// Click on mode area
	//
	if (CursorOn == CursorOnButton) {
		CursorBuilding = NULL;
		switch (ButtonUnderCursor) {
			case SelectButton :
				EditorState = EditorSelecting;
				return;
			case UnitButton:
				EditorState = EditorEditUnit;
				return;
			case TileButton :
				if (EditorEditTile)
					EditorState = EditorEditTile;
				return;
			case StartButton:
				EditorState = EditorSetStartLocation;
				return;
			default:
				break;
		}
	}
	//
	// Click on tile area
	//
	if (CursorOn == CursorOnButton && ButtonUnderCursor >= 100 &&
			EditorState == EditorEditTile) {
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
		if (Map.Tileset.Tiles[16 + (ButtonUnderCursor - 100) * 16].BaseTerrain) {
			TileCursor = ButtonUnderCursor - 100;
		}
		return;
	}
	
	// Click on player area
	if (EditorState == EditorEditUnit || EditorState == EditorSetStartLocation) {
		// Cursor on player icons
		if (CursorPlayer != -1) {
			if (Map.Info.PlayerType[CursorPlayer] != PlayerNobody) {
				SelectedPlayer = CursorPlayer;
				ThisPlayer = Players + SelectedPlayer;
			}
			return;
		}
	}

	//
	// Click on unit area
	//
	if (EditorState == EditorEditUnit) {
		int percent;
		int j;
		int count;

		percent = UnitIndex * 100 / (MaxShownUnits ? MaxShownUnits : 1);
		j = (percent * (176 - 8 - 54)) / 100;
		count = CalculateUnitIcons();

		// Unit icons scroll left area
		if (UI.ButtonPanel.X + 4 < CursorX &&
				CursorX < UI.ButtonPanel.X + 4 + 18 + j &&
				UI.ButtonPanel.Y + 4 < CursorY &&
				CursorY < UI.ButtonPanel.Y + 24) {
			if (UnitIndex - count >= 0) {
				UnitIndex -= count;
			} else {
				UnitIndex = 0;
			}
			return;
		}
		// Unit icons scroll right area
		if (UI.ButtonPanel.X + 4 + 18 + j + 18 < CursorX &&
				CursorX < UI.ButtonPanel.X + 176 - 4 &&
				UI.ButtonPanel.Y + 4 < CursorY &&
				CursorY < UI.ButtonPanel.Y + 24) {
			if (UnitIndex + count <= MaxShownUnits) {
				UnitIndex += count;
			}
			return;
		}
		// Cursor on unit icons
		if (CursorUnitIndex != -1) {
			SelectedUnitIndex = CursorUnitIndex;
			CursorBuilding = UnitTypeByIdent(ShownUnitTypes[CursorUnitIndex]);
			return;
		}
		// Cursor on unit selection icons
		if (UI.InfoPanel.X + 10 + 28 * 0 < CursorX &&
				CursorX < UI.InfoPanel.X + 10 + 28 * 1 &&
				UI.InfoPanel.Y + 140 < CursorY &&
				CursorY < UI.InfoPanel.Y + 140 + 28) {
			ShowUnitsToSelect ^= 1;
			RecalculateShownUnits();
			return;
		}
		if (UI.InfoPanel.X + 10 + 28 * 1 < CursorX &&
				CursorX < UI.InfoPanel.X + 10 + 28 * 2 &&
				UI.InfoPanel.Y + 140 < CursorY &&
				CursorY < UI.InfoPanel.Y + 140 + 28) {
			ShowBuildingsToSelect ^= 1;
			RecalculateShownUnits();
			return;
		}
#if 0
		if (UI.InfoPanel.X + 10 + 28 * 2 < CursorX &&
				CursorX < UI.InfoPanel.X + 10 + 28 * 3 &&
				UI.InfoPanel.Y + 140 < CursorY &&
				CursorY < UI.InfoPanel.Y + 140 + 28) {
			ShowHeroesToSelect ^= 1;
			RecalculateShownUnits();
			return;
		}
#endif
		if (UI.InfoPanel.X + 10 + 28 * 3 < CursorX &&
				CursorX < UI.InfoPanel.X + 10 + 28 * 4 &&
				UI.InfoPanel.Y + 140 < CursorY &&
				CursorY < UI.InfoPanel.Y + 140 + 28) {
			ShowLandToSelect ^= 1;
			RecalculateShownUnits();
			return;
		}
		if (UI.InfoPanel.X + 10 + 28 * 4 < CursorX &&
				CursorX < UI.InfoPanel.X + 10 + 28 * 5 &&
				UI.InfoPanel.Y + 140 < CursorY &&
				CursorY < UI.InfoPanel.Y + 140 + 28) {
			ShowWaterToSelect ^= 1;
			RecalculateShownUnits();
			return;
		}
		if (UI.InfoPanel.X + 10 + 28 * 5 < CursorX &&
				CursorX < UI.InfoPanel.X + 10 + 28 * 6 &&
				UI.InfoPanel.Y + 140 < CursorY &&
				CursorY < UI.InfoPanel.Y + 140 + 28) {
			ShowAirToSelect ^= 1;
			RecalculateShownUnits();
			return;
		}
	}

	//
	// Right click on a resource
	//
	if (EditorState == EditorSelecting) {
		if ((MouseButtons & RightButton && UnitUnderCursor)) {
			if (UnitUnderCursor->Type->GivesResource) {
				EditorEditResource();
			} else {
				EditorEditAiProperties();
			}
			return;
		}
	}

	//
	// Click on map area
	//
	if (CursorOn == CursorOnMap) {
		CViewport *vp;

		vp = GetViewport(CursorX, CursorY);
		Assert(vp);
		if ((MouseButtons & LeftButton) && UI.SelectedViewport != vp) {
			// viewport changed
			UI.SelectedViewport = vp;
		}

		if (MouseButtons & LeftButton) {
			if (EditorState == EditorEditTile) {
				EditTiles(UI.MouseViewport->Viewport2MapX(CursorX),
					UI.MouseViewport->Viewport2MapY(CursorY), TileCursor,
					TileCursorSize);
			}
			if (!UnitPlacedThisPress) {
				if (EditorState == EditorEditUnit && CursorBuilding) {
					if (CanBuildUnitType(NULL, CursorBuilding,
							UI.MouseViewport->Viewport2MapX(CursorX),
							UI.MouseViewport->Viewport2MapY(CursorY), 1)) {
						PlayGameSound(GameSounds.PlacementSuccess.Sound,
							MaxSampleVolume);
						EditUnit(UI.MouseViewport->Viewport2MapX(CursorX),
							UI.MouseViewport->Viewport2MapY(CursorY),
							CursorBuilding, Players + SelectedPlayer);
						UnitPlacedThisPress = 1;
						UI.StatusLine.Clear();
					} else {
						UI.StatusLine.Set("Unit can't be placed here.");
						PlayGameSound(GameSounds.PlacementError.Sound,
							MaxSampleVolume);
					}
				}
			}
			if (EditorState == EditorSetStartLocation) {
				Players[SelectedPlayer].StartX = UI.MouseViewport->Viewport2MapX(CursorX);
				Players[SelectedPlayer].StartY = UI.MouseViewport->Viewport2MapY(CursorY);
			}
		} else if (MouseButtons & MiddleButton) {
			// enter move map mode
			CursorStartX = CursorX;
			CursorStartY = CursorY;
			GameCursor = UI.Scroll.Cursor;
		}
	}
}

/**
**  Handle key down.
**
**  @param key      Key scancode.
**  @param keychar  Character code.
*/
static void EditorCallbackKeyDown(unsigned key, unsigned keychar)
{
	if (HandleKeyModifiersDown(key, keychar)) {
		return;
	}

	switch (key) {
		case 'f': // ALT+F, CTRL+F toggle fullscreen
			if (!(KeyModifiers & (ModifierAlt | ModifierControl))) {
				break;
			}
			ToggleFullScreen();
			break;

		case 's': // ALT+S, CTRL+S, F11 save map menu
		case SDLK_F11:
			if (key == 's' && !(KeyModifiers & (ModifierAlt | ModifierControl))) {
				break;
			}
			if (EditorSaveMenu() != -1) {
				UI.StatusLine.Set("Map saved");
			}
			InterfaceState = IfaceStateNormal;
			break;

		case SDLK_F12:
			EditorLoadMenu();
			InterfaceState = IfaceStateNormal;
			break;

		case 'v': // 'v' Viewport
			if (KeyModifiers & ModifierControl) {
				CycleViewportMode(-1);
			} else {
				CycleViewportMode(1);
			}
			break;

		case 'r': // CTRL+R Randomize map
			if (KeyModifiers & ModifierControl) {
				EditorCreateRandomMap();
			}
			break;

		case 'm': // CTRL+M Mirror edit
			if (KeyModifiers & ModifierControl)  {
				++MirrorEdit;
				if (MirrorEdit == 3) {
					MirrorEdit = 0;
				}
				switch (MirrorEdit) {
					case 1:
						UI.StatusLine.Set("Mirror editing enabled: 2-side");
						break;
					case 2:
						UI.StatusLine.Set("Mirror editing enabled: 4-side");
						break;
					default:
						UI.StatusLine.Set("Mirror editing disabled");
						break;
				  }
			}
			break;

		case 'x': // ALT+X, CTRL+X: Exit editor
			if (!(KeyModifiers & (ModifierAlt | ModifierControl))) {
				break;
			}
			Exit(0);

		case SDLK_DELETE: // Delete
			if (UnitUnderCursor) {
				CUnit *unit = UnitUnderCursor;

				unit->Remove(NULL);
				UnitLost(unit);
				UnitClearOrders(unit);
				unit->Release();
				UI.StatusLine.Set("Unit deleted");
			}
			break;

		case SDLK_F10:
			ProcessMenu("menu-editor", 1);
			break;

		case SDLK_UP: // Keyboard scrolling
		case SDLK_KP8:
			KeyScrollState |= ScrollUp;
			break;
		case SDLK_DOWN:
		case SDLK_KP2:
			KeyScrollState |= ScrollDown;
			break;
		case SDLK_LEFT:
		case SDLK_KP4:
			KeyScrollState |= ScrollLeft;
			break;
		case SDLK_RIGHT:
		case SDLK_KP6:
			KeyScrollState |= ScrollRight;
			break;

		default:
			return;
	}
	return;
}

/**
**  Handle key up.
**
**  @param key      Key scancode.
**  @param keychar  Character code.
*/
static void EditorCallbackKeyUp(unsigned key, unsigned keychar)
{
	if (HandleKeyModifiersUp(key, keychar)) {
		return;
	}

	switch (key) {
		case SDLK_UP: // Keyboard scrolling
		case SDLK_KP8:
			KeyScrollState &= ~ScrollUp;
			break;
		case SDLK_DOWN:
		case SDLK_KP2:
			KeyScrollState &= ~ScrollDown;
			break;
		case SDLK_LEFT:
		case SDLK_KP4:
			KeyScrollState &= ~ScrollLeft;
			break;
		case SDLK_RIGHT:
		case SDLK_KP6:
			KeyScrollState &= ~ScrollRight;
			break;
		default:
			break;
	}
}

/**
**  Callback for input.
*/
static void EditorCallbackKey3(unsigned dummy1, unsigned dummy2)
{
}

/**
**  Callback for input movement of the cursor.
**
**  @param x  Screen X position.
**  @param y  Screen Y position.
*/
static void EditorCallbackMouse(int x, int y)
{
	int i;
	int bx;
	int by;
	static int LastMapX;
	static int LastMapY;
	enum _cursor_on_ OldCursorOn;
	char buf[256];

	HandleCursorMove(&x, &y); // Reduce to screen

	//
	// Move map.
	//
	if (GameCursor == UI.Scroll.Cursor) {
		int xo;
		int yo;

		// FIXME: Support with CTRL for faster scrolling.
		// FIXME: code duplication, see ../ui/mouse.c
		xo = UI.MouseViewport->MapX;
		yo = UI.MouseViewport->MapY;
		if (UI.MouseScrollSpeedDefault < 0) {
			if (x < CursorStartX) {
				xo++;
			} else if (x > CursorStartX) {
				xo--;
			}
			if (y < CursorStartY) {
				yo++;
			} else if (y > CursorStartY) {
				yo--;
			}
		} else {
			if (x < CursorStartX) {
				xo--;
			} else if (x > CursorStartX) {
				xo++;
			}
			if (y < CursorStartY) {
				yo--;
			} else if (y > CursorStartY) {
				yo++;
			}
		}
		UI.MouseWarpX = CursorStartX;
		UI.MouseWarpY = CursorStartY;
		UI.MouseViewport->Set(xo, yo, TileSizeX / 2, TileSizeY / 2);
		return;
	}

	// Automatically unpress when map tile has changed
	if (LastMapX != UI.SelectedViewport->Viewport2MapX(CursorX) ||
		LastMapY != UI.SelectedViewport->Viewport2MapY(CursorY)) {
		LastMapX = UI.SelectedViewport->Viewport2MapX(CursorX);
		LastMapY = UI.SelectedViewport->Viewport2MapY(CursorY);
		UnitPlacedThisPress = 0;
	}
	//
	// Drawing tiles on map.
	//
	if (CursorOn == CursorOnMap && (MouseButtons & LeftButton) &&
			(EditorState == EditorEditTile || EditorState == EditorEditUnit)) {

		//
		// Scroll the map
		//
		if (!(FrameCounter % SpeedMouseScroll)) {
			if (CursorX <= UI.SelectedViewport->X) {
				UI.SelectedViewport->Set(
					UI.SelectedViewport->MapX - 1,
					UI.SelectedViewport->MapY, TileSizeX / 2, TileSizeY / 2);
			} else if (CursorX >= UI.SelectedViewport->EndX) {
				UI.SelectedViewport->Set(
					UI.SelectedViewport->MapX + 1,
					UI.SelectedViewport->MapY, TileSizeX / 2, TileSizeY / 2);
			}

			if (CursorY <= UI.SelectedViewport->Y) {
				UI.SelectedViewport->Set(
					UI.SelectedViewport->MapX,
					UI.SelectedViewport->MapY - 1, TileSizeX / 2, TileSizeY / 2);
			} else if (CursorY >= UI.SelectedViewport->EndY) {
				UI.SelectedViewport->Set(
					UI.SelectedViewport->MapX,
					UI.SelectedViewport->MapY + 1, TileSizeX / 2, TileSizeY / 2);

			}
		}

		//
		// Scroll the map, if cursor moves outside the viewport.
		//
		RestrictCursorToViewport();

		if (EditorState == EditorEditTile) {
			EditTiles(UI.SelectedViewport->Viewport2MapX(CursorX),
				UI.SelectedViewport->Viewport2MapY(CursorY), TileCursor,
				TileCursorSize);
		} else if (EditorState == EditorEditUnit && CursorBuilding) {
			if (!UnitPlacedThisPress) {
				if (CanBuildUnitType(NULL, CursorBuilding,
					UI.SelectedViewport->Viewport2MapX(CursorX),
					UI.SelectedViewport->Viewport2MapY(CursorY), 1)) {
					EditUnit(UI.SelectedViewport->Viewport2MapX(CursorX),
						UI.SelectedViewport->Viewport2MapY(CursorY),
						CursorBuilding, Players + SelectedPlayer);
					UnitPlacedThisPress = 1;
					UI.StatusLine.Clear();
				}
			}
		}

		return;
	}

	//
	// Minimap move viewpoint
	//
	if (CursorOn == CursorOnMinimap && (MouseButtons & LeftButton)) {
		RestrictCursorToMinimap();
		UI.SelectedViewport->Set(
			UI.Minimap.Screen2MapX(CursorX)
				- UI.SelectedViewport->MapWidth / 2,
			UI.Minimap.Screen2MapY(CursorY)
				- UI.SelectedViewport->MapHeight / 2, 0, 0);
		return;
	}

	OldCursorOn = CursorOn;

	MouseScrollState = ScrollNone;
	GameCursor = UI.Point.Cursor;
	CursorOn = CursorOnUnknown;
	CursorPlayer = -1;
	CursorUnitIndex = -1;
	ButtonUnderCursor = -1;

	//
	// Minimap
	//
	if (x >= UI.Minimap.X && x < UI.Minimap.X + UI.Minimap.W &&
			y >= UI.Minimap.Y && y < UI.Minimap.Y + UI.Minimap.H) {
		CursorOn = CursorOnMinimap;
	}

	//
	// Handle edit unit area
	//
	if (EditorState == EditorEditUnit || EditorState == EditorSetStartLocation) {
		// Scrollbar
		if (UI.ButtonPanel.X + 4 < CursorX
				&& CursorX < UI.ButtonPanel.X + 176 - 4
				&& UI.ButtonPanel.Y + 4 < CursorY
				&& CursorY < UI.ButtonPanel.Y + 24) {
			return;
		}
		bx = UI.InfoPanel.X + 8;
		by = UI.InfoPanel.Y + 4 + IconHeight + 10;
		for (i = 0; i < PlayerMax; ++i) {
			if (i == PlayerMax / 2) {
				bx = UI.InfoPanel.X + 8;
				by += 20;
			}
			if (bx < x && x < bx + 20 && by < y && y < by + 20) {
				if (Map.Info.PlayerType[i] != PlayerNobody) {
					sprintf(buf,"Select player #%d",i);
					UI.StatusLine.Set(buf);
				} else {
					UI.StatusLine.Clear();
				}
				CursorPlayer = i;
#if 0
				ButtonUnderCursor = i + 100;
				CursorOn = CursorOnButton;
#endif
				return;
			}
			bx += 20;
		}

		i = UnitIndex;
		by = UI.ButtonPanel.Y + 24;
		while (by < UI.ButtonPanel.Y +
				UI.ButtonPanel.G->Height - IconHeight) {
			if (i >= MaxShownUnits || !ShownUnitTypes[i]) {
				break;
			}
			bx = UI.ButtonPanel.X + 10;
			while (bx < UI.ButtonPanel.X + 146) {
				if (i >= MaxShownUnits || !ShownUnitTypes[i]) {
					break;
				}
				if (bx < x && x < bx + IconWidth &&
						by < y && y < by + IconHeight) {
					sprintf(buf,"%s \"%s\"",
						UnitTypeByIdent(ShownUnitTypes[i])->Ident,
						UnitTypeByIdent(ShownUnitTypes[i])->Name);
					UI.StatusLine.Set(buf);
					CursorUnitIndex = i;
#if 0
					ButtonUnderCursor = i + 100;
					CursorOn = CursorOnButton;
#endif
					return;
				}
				bx += IconWidth + 8;
				i++;
			}
			by += IconHeight + 2;
		}
	}

	//
	// Handle tile area
	//
	if (EditorState == EditorEditTile) {
		i = 0;
		bx = UI.InfoPanel.X + 4;
		by = UI.InfoPanel.Y + 4 + IconHeight + 10;

		while (i < 6) {
			if (bx < x && x < bx + 100 && by < y && y < by + 18) {
				ButtonUnderCursor = i + 300;
				CursorOn = CursorOnButton;
				return;
			}
			++i;
			by += 20;
		}

		i = 0;
		by = UI.ButtonPanel.Y + 4;
		while (by < UI.ButtonPanel.Y + 100) {
			bx = UI.ButtonPanel.X + 4;
			while (bx < UI.ButtonPanel.X + 144) {
				if (!Map.Tileset.Tiles[0x10 + i * 16].BaseTerrain) {
					by = UI.ButtonPanel.Y + 100;
					break;
				}
				if (bx < x && x < bx + TileSizeX &&
						by < y && y < by + TileSizeY) {
					int j;

					// FIXME: i is wrong, must find the solid type
					j = Map.Tileset.Tiles[i * 16 + 16].BaseTerrain;
					//MAPTODO UI.StatusLine.Set(Map.Tileset.SolidTerrainTypes[j].TerrainName);
					ButtonUnderCursor = i + 100;
					CursorOn = CursorOnButton;
					return;
				}
				bx += TileSizeX+2;
				++i;
			}
			by += TileSizeY+2;
		}
	}

	//
	// Handle buttons
	//
	if (UI.InfoPanel.X + 4 < CursorX &&
			CursorX < UI.InfoPanel.X + 4 + IconWidth + 7 &&
			UI.InfoPanel.Y + 4 < CursorY &&
			CursorY < UI.InfoPanel.Y + 4 + IconHeight + 7) {
		// FIXME: what is this button?
		ButtonAreaUnderCursor = -1;
		ButtonUnderCursor = SelectButton;
		CursorOn = CursorOnButton;
		UI.StatusLine.Set("Select mode");
		return;
	}
	if (UI.InfoPanel.X + 4 + UNIT_ICON_X < CursorX &&
			CursorX < UI.InfoPanel.X + 4 + UNIT_ICON_X + IconWidth + 7 &&
			UI.InfoPanel.Y + 4 + UNIT_ICON_Y < CursorY &&
			CursorY < UI.InfoPanel.Y + 4 + UNIT_ICON_Y + IconHeight + 7) {
		ButtonAreaUnderCursor = -1;
		ButtonUnderCursor = UnitButton;
		CursorOn = CursorOnButton;
		UI.StatusLine.Set("Unit mode");
		return;
	}
	if (UI.InfoPanel.X + 4 + TILE_ICON_X < CursorX &&
			CursorX < UI.InfoPanel.X + 4 + TILE_ICON_X + TileSizeX + 7 &&
			UI.InfoPanel.Y + 4 + TILE_ICON_Y < CursorY &&
			CursorY < UI.InfoPanel.Y + 4 + TILE_ICON_Y + TileSizeY + 7) {
		ButtonAreaUnderCursor = -1;
		ButtonUnderCursor = TileButton;
		CursorOn = CursorOnButton;
		UI.StatusLine.Set("Tile mode");
		return;
	}
	if (UI.InfoPanel.X + 4 + START_ICON_X < CursorX &&
			CursorX < UI.InfoPanel.X + 4 + START_ICON_X + TileSizeX + 7 &&
			UI.InfoPanel.Y + 4 + START_ICON_Y < CursorY &&
			CursorY < UI.InfoPanel.Y + 4 + START_ICON_Y + TileSizeY + 7) {
		ButtonAreaUnderCursor = -1;
		ButtonUnderCursor = StartButton;
		CursorOn = CursorOnButton;
		UI.StatusLine.Set("Set start location mode");
		return;
	}
	if (UI.MenuButton.X != -1) {
		if (x >= UI.MenuButton.X &&
				x <= UI.MenuButton.X + UI.MenuButton.Style->Width &&
				y > UI.MenuButton.Y &&
				y <= UI.MenuButton.Y + UI.MenuButton.Style->Height) {
			ButtonAreaUnderCursor = ButtonAreaMenu;
			ButtonUnderCursor = ButtonUnderMenu;
			CursorOn = CursorOnButton;
			return;
		}
	}

	//
	// Minimap
	//
	if (x >= UI.Minimap.X && x < UI.Minimap.X + UI.Minimap.W &&
			y >= UI.Minimap.Y && y < UI.Minimap.Y + UI.Minimap.H) {
		CursorOn = CursorOnMinimap;
		return;
	}

	//
	// Map
	//
	UnitUnderCursor = NULL;
	if (x >= UI.MapArea.X && x <= UI.MapArea.EndX &&
			y >= UI.MapArea.Y && y <= UI.MapArea.EndY) {
		CViewport *vp;

		vp = GetViewport(x, y);
		Assert(vp);
		if (UI.MouseViewport != vp) { // viewport changed
			UI.MouseViewport = vp;
			DebugPrint("active viewport changed to %d.\n" _C_
				UI.Viewports - vp);
		}
		CursorOn = CursorOnMap;

		//
		// Look if there is an unit under the cursor.
		// FIXME: use Viewport2MapX Viewport2MapY
		//
		UnitUnderCursor = UnitOnScreen(NULL,
			CursorX - UI.MouseViewport->X +
				UI.MouseViewport->MapX * TileSizeX + UI.MouseViewport->OffsetX,
			CursorY - UI.MouseViewport->Y +
				UI.MouseViewport->MapY * TileSizeY + UI.MouseViewport->OffsetY);
		if (UnitUnderCursor) {
			ShowUnitInfo(UnitUnderCursor);
			return;
		}
	}
	//
	// Scrolling Region Handling
	//
	if (HandleMouseScrollArea(x, y)) {
		return;
	}

	// Not reached if cursor is inside the scroll area

	UI.StatusLine.Clear();
}

/**
**  Callback for exit.
*/
static void EditorCallbackExit(void)
{
}

/**
**  Create editor.
*/
static void CreateEditor(void)
{
	int i;
	char *file;
	char buf[PATH_MAX];
	CFile clf;

	//
	// Load and evaluate the editor configuration file
	// FIXME: the CLopen is very slow and repeats the work of LibraryFileName.
	//
	file = LibraryFileName(EditorStartFile, buf);
	if (clf.open(file, CL_OPEN_READ) != -1) {
		clf.close();
		ShowLoadProgress("Script %s", file);
		LuaLoadFile(file);
		CclGarbageCollect(0); // Cleanup memory after load
	}

	ThisPlayer = &Players[0];

	FlagRevealMap = 1; // editor without fog and all visible
	Map.NoFogOfWar = true;

	if (!*CurrentMapPath) { // new map!
		InitUnitTypes(1);
		//
		// Inititialize Map / Players.
		//
		InitPlayers();
		for (i = 0; i < PlayerMax; ++i) {
			if (i == PlayerNumNeutral) {
				CreatePlayer(PlayerNeutral);
				Map.Info.PlayerType[i] = PlayerNeutral;
				Map.Info.PlayerSide[i] = Players[i].Race = 0;
			} else {
				CreatePlayer(PlayerNobody);
				Map.Info.PlayerType[i] = PlayerNobody;
			}
		}

		Map.Fields = new CMapField[Map.Info.MapWidth * Map.Info.MapHeight];
		Map.Visible[0] = new unsigned[Map.Info.MapWidth * Map.Info.MapHeight / 2];
		memset(Map.Visible[0], 0, Map.Info.MapWidth * Map.Info.MapHeight / 2 * sizeof(unsigned));
		InitUnitCache();

		for (i = 0; i < Map.Info.MapWidth * Map.Info.MapHeight; ++i) {
			Map.Fields[i].Tile = Map.Fields[i].SeenTile = 0;
			Map.Fields[i].Tile = Map.Fields[i].SeenTile =
				Map.Tileset.Table[0x50];
			Map.Fields[i].Flags = Map.Tileset.FlagsTable[0x50];
		}
		GameSettings.Resources = SettingsResourcesMapDefault;
		CreateGame(NULL, &Map);
	} else {
		CreateGame(CurrentMapPath, &Map);
	}

	ReplayRevealMap = 1;
	FlagRevealMap = 0;
	SelectedPlayer = PlayerNumNeutral;

	//
	// Place the start points, which the loader discarded.
	//
	for (i = 0; i < PlayerMax; ++i) {
		if (Map.Info.PlayerType[i] != PlayerNobody) {
			// Set SelectedPlayer to a valid player
			if (SelectedPlayer == PlayerNumNeutral) {
				SelectedPlayer = i;
			}
		} else if (Players[i].StartX | Players[i].StartY) {
			DebugPrint("Player nobody has a start position\n");
		}
	}

	if (!EditorUnitTypes) {
		// Build empty editor unit-type tables.
		EditorUnitTypes = new char *[2];
		MaxUnitIndex = 0;
	}

	CalculateMaxIconSize();
	ShowUnitsToSelect = 1; // Show all units as default
	ShowBuildingsToSelect = 1;
#if 0
	ShowHeroesToSelect = 1;
#endif
	ShowAirToSelect = 1;
	ShowLandToSelect = 1;
	ShowWaterToSelect = 1;

	RecalculateShownUnits();
	UI.Minimap.Update();

	ProcessMenu("menu-editor-tips", 1);
	InterfaceState = IfaceStateNormal;
}

/**
**  Save a map from editor.
**
**  @param file  Save the level to this file.
**
**  @return      0 for success, -1 for error
**
**  @todo  FIXME: Check if the map is valid, contains no failures.
**         At least two players, one human slot, every player a startpoint
**         ...
*/
int EditorSaveMap(const char *file)
{
	if (SaveStratagusMap(file, &Map, TerrainEditable) == -1) {
		ErrorMenu("Cannot save map");
		InterfaceState = IfaceStateNormal;
		EditorUpdateDisplay();
		InterfaceState = IfaceStateMenu;
		return -1;
	}

	return 0;
}

/*----------------------------------------------------------------------------
--  Editor main loop
----------------------------------------------------------------------------*/

/**
**  Editor main event loop.
*/
void EditorMainLoop(void)
{
	EventCallback callbacks;
	int OldCommandLogDisabled;

	OldCommandLogDisabled = CommandLogDisabled;
	CommandLogDisabled = 1;

	while (1) {
		EditorMapLoaded = 0;
		EditorRunning = EditorEditing;

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

		GameCursor = UI.Point.Cursor;
		InterfaceState = IfaceStateNormal;
		EditorState = EditorSelecting;
		UI.SelectedViewport = UI.Viewports;
		TileCursorSize = 1;

		while (EditorRunning) {
			PlayListAdvance();

			UI.Minimap.Update();

			EditorUpdateDisplay();

			//
			// Map scrolling
			//
			if (UI.MouseScroll && !(FrameCounter % SpeedMouseScroll)) {
				DoScrollArea(MouseScrollState, 0);
			}
			if (UI.KeyScroll && !(FrameCounter % SpeedKeyScroll)) {
				DoScrollArea(KeyScrollState, KeyModifiers & ModifierControl);
				if (CursorOn == CursorOnMap && (MouseButtons & LeftButton) &&
						(EditorState == EditorEditTile ||
							EditorState == EditorEditUnit)) {
					EditorCallbackButtonDown(0);
				}
			}

			WaitEventsOneFrame(&callbacks);
		}

		if (!EditorMapLoaded) {
			break;
		}

		CleanModules();

		LoadCcl(); // Reload the main config file

		PreMenuSetup();

		InterfaceState = IfaceStateMenu;
		GameCursor = UI.Point.Cursor;

		Video.ClearScreen();
		Invalidate();
	}

	CommandLogDisabled = OldCommandLogDisabled;
}

//@}
