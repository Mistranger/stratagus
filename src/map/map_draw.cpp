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
/**@name map_draw.cpp - The map drawing. */
//
//      (c) Copyright 1999-2005 by Lutz Sammer and Jimmy Salmon
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

#include "stratagus.h"
#include "unit.h"
#include "tileset.h"
#include "video.h"
#include "map.h"
#include "player.h"
#include "pathfinder.h"
#include "ui.h"
#include "missile.h"
#include "unittype.h"
#include "font.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Global functions
----------------------------------------------------------------------------*/

/**
**  Check if any part of an area is visible in a viewport.
**
**  @param sx  X map tile position of area in map to be checked.
**  @param sy  Y map tile position of area in map to be checked.
**  @param ex  X map tile position of area in map to be checked.
**  @param ey  Y map tile position of area in map to be checked.
**
**  @return    True if any part of area is visible, false otherwise
*/
bool CViewport::AnyMapAreaVisibleInViewport(int sx, int sy, int ex, int ey) const
{
	if (ex < this->MapX || ey < this->MapY ||
			sx >= this->MapX + this->MapWidth || sy >= this->MapY + this->MapHeight) {
		return false;
	}
	return true;
}

bool CViewport::IsInsideMapArea(int x, int y) const
{
	int tilex;
	int tiley;

	tilex = x - this->X + this->MapX * TileSizeX + this->OffsetX;
	if (tilex < 0) {
		tilex = (tilex - TileSizeX + 1) / TileSizeX;
	} else {
		tilex /= TileSizeX;
	}

	tiley = y - this->Y + this->MapY * TileSizeY + this->OffsetY;
	if (tiley < 0) {
		tiley = (tiley - TileSizeY + 1) / TileSizeY;
	} else {
		tiley /= TileSizeY;
	}

	return (tilex >= 0 && tiley >= 0 && tilex < Map.Info.MapWidth && tiley < Map.Info.MapHeight);
}

/**
**  Convert viewport x coordinate to map tile x coordinate.
**
**  @param x   X coordinate into this viewport (in pixels, relative
**             to origin of Stratagus's window - not the viewport
**             itself!).
**
**  @return    X map tile coordinate.
*/
int CViewport::Viewport2MapX(int x) const
{
	int r = (x - this->X + this->MapX * TileSizeX + this->OffsetX) / TileSizeX;
	return std::min<int>(r, Map.Info.MapWidth - 1);
}

/**
**  Convert viewport y coordinate to map tile y coordinate.
**
**  @param y   Y coordinate into this viewport (in pixels, relative
**             to origin of Stratagus's window - not the viewport
**             itself!).
**
**  @return    Y map tile coordinate.
*/
int CViewport::Viewport2MapY(int y) const
{
	int r = (y - this->Y + this->MapY * TileSizeY + this->OffsetY) / TileSizeY;
	return std::min<int>(r, Map.Info.MapHeight - 1);
}

/**
**  Convert a map tile X coordinate into a viewport x pixel coordinate.
**
**  @param x   The map tile's X coordinate.
**
**  @return    X screen coordinate in pixels (relative
**             to origin of Stratagus's window).
*/
int CViewport::Map2ViewportX(int x) const
{
	return this->X + (x - this->MapX) * TileSizeX - this->OffsetX;
}

/**
**  Convert a map tile Y coordinate into a viewport y pixel coordinate.
**
**  @param y   The map tile's Y coordinate.
**
**  @return    Y screen coordinate in pixels (relative
**             to origin of Stratagus's window).
*/
int CViewport::Map2ViewportY(int y) const
{
	return this->Y + (y - this->MapY) * TileSizeY - this->OffsetY;
}

/**
**  Convert map pixel coordinates into viewport coordinates.
*/
void CViewport::MapPixel2Viewport(int &x, int &y) const
{
	x = x + this->X - (this->MapX * TileSizeX + this->OffsetX);
	y = y + this->Y - (this->MapY * TileSizeY + this->OffsetY);
}

/**
**  Change viewpoint of map viewport v to x,y.
**
**  @param x        X map tile position.
**  @param y        Y map tile position.
**  @param offsetx  X offset in tile.
**  @param offsety  Y offset in tile.
*/
void CViewport::Set(int x, int y, int offsetx, int offsety)
{
	x = x * TileSizeX + offsetx;
	y = y * TileSizeY + offsety;

	if (x < -UI.MapArea.ScrollPaddingLeft) {
		x = -UI.MapArea.ScrollPaddingLeft;
	}
	if (y < -UI.MapArea.ScrollPaddingTop) {
		y = -UI.MapArea.ScrollPaddingTop;
	}
	if (x > Map.Info.MapWidth * TileSizeX - (this->EndX - this->X) - 1 + UI.MapArea.ScrollPaddingRight) {
		x = Map.Info.MapWidth * TileSizeX - (this->EndX - this->X) - 1 + UI.MapArea.ScrollPaddingRight;
	}
	if (y > Map.Info.MapHeight * TileSizeY - (this->EndY - this->Y) - 1 + UI.MapArea.ScrollPaddingBottom) {
		y = Map.Info.MapHeight * TileSizeY - (this->EndY - this->Y) - 1 + UI.MapArea.ScrollPaddingBottom;
	}

	this->MapX = x / TileSizeX;
	if (x < 0 && x % TileSizeX) {
		this->MapX--;
	}
	this->MapY = y / TileSizeY;
	if (y < 0 && y % TileSizeY) {
		this->MapY--;
	}
	this->OffsetX = x % TileSizeX;
	if (this->OffsetX < 0) {
		this->OffsetX += TileSizeX;
	}
	this->OffsetY = y % TileSizeY;
	if (this->OffsetY < 0) {
		this->OffsetY += TileSizeY;
	}
	this->MapWidth = ((this->EndX - this->X) + this->OffsetX - 1) / TileSizeX + 1;
	this->MapHeight = ((this->EndY - this->Y) + this->OffsetY - 1) / TileSizeY + 1;
}

/**
**  Center map viewport v on map tile (x,y).
**
**  @param x   X map tile position.
**  @param y   Y map tile position.
**  @param offsetx  X offset in tile.
**  @param offsety  Y offset in tile.
*/
void CViewport::Center(int x, int y, int offsetx, int offsety)
{
	x = x * TileSizeX + offsetx - (this->EndX - this->X) / 2;
	y = y * TileSizeY + offsety - (this->EndY - this->Y) / 2;
	this->Set(x / TileSizeX, y / TileSizeY, x % TileSizeX, y % TileSizeY);
}

/**
**  Draw the map backgrounds.
**
** StephanR: variables explained below for screen:<PRE>
** *---------------------------------------*
** |                                       |
** |        *-----------------------*      |<-TheUi.MapY,dy (in pixels)
** |        |   |   |   |   |   |   |      |        |
** |        |   |   |   |   |   |   |      |        |
** |        |---+---+---+---+---+---|      |        |
** |        |   |   |   |   |   |   |      |        |MapHeight (in tiles)
** |        |   |   |   |   |   |   |      |        |
** |        |---+---+---+---+---+---|      |        |
** |        |   |   |   |   |   |   |      |        |
** |        |   |   |   |   |   |   |      |        |
** |        *-----------------------*      |<-ey,UI.MapEndY (in pixels)
** |                                       |
** |                                       |
** *---------------------------------------*
**          ^                       ^
**        dx|-----------------------|ex,UI.MapEndX (in pixels)
**            UI.MapX MapWidth (in tiles)
** (in pixels)
** </PRE>
*/
void CViewport::DrawMapBackgroundInViewport() const
{
	int ex = this->EndX;
	int sy = this->MapY;
	int dy = this->Y - this->OffsetY;
	int ey = this->EndY;
	const int map_max = Map.Info.MapWidth * Map.Info.MapHeight;
	unsigned short int tile;

	while (sy  < 0) {
		sy++;
		dy += TileSizeY;
	}
	sy *=  Map.Info.MapWidth;

	while (dy <= ey && sy  < map_max) {

	/*
		if (sy / Map.Info.MapWidth < 0) {
			sy += Map.Info.MapWidth;
			dy += TileSizeY;
			continue;
		}
*/
		int sx = this->MapX + sy;
		int dx = this->X - this->OffsetX;
		while (dx <= ex && (sx - sy < Map.Info.MapWidth)) {
			if (sx - sy < 0) {
				++sx;
				dx += TileSizeX;
				continue;
			}

			if (ReplayRevealMap) {
				tile = Map.Fields[sx].Tile;
			} else {
				tile = Map.Fields[sx].SeenTile;
			}
			Map.TileGraphic->DrawFrameClip(tile, dx, dy);

#ifdef DEBUG
			int my_mask = 0;
			unsigned int color = 0;
			if (Map.CheckMask(sx, MapFieldUnpassable))
			{
				my_mask = 1;
			}
			if (Map.CheckMask(sx, MapFieldNoBuilding))
			{
				my_mask |= 2;
			}
			switch(my_mask) {
				case 1://tile only Unpassable
					color = 0xFF0000;
				break;
				case 2://tile only NoBuilding
					color = 0x00FF00;
				break;
				case 3://tile Unpassable and NoBuilding
					color = 0xFF;
				break;
				default:
				break;
			}

			Video.DrawHLineClip(color, dx, dy, TileSizeX);
			Video.DrawVLineClip(color, dx, dy, TileSizeY);
			if ( 0 && my_mask ) {
				CLabel label(SmallFont);
				label.Draw(dx + 2, dy +2, tile);
				label.Draw(dx + 2, dy + SmallFont->Height() + 4,
					 Map.Fields[sx].TilesetTile );

			}
#endif
			++sx;
			dx += TileSizeX;
		}
		sy += Map.Info.MapWidth;
		dy += TileSizeY;
	}
}

struct CDrawProxy {

	CDrawProxy(): nunits(0), nmissiles(0) {}

	CMutex lock;
	CUnitDrawProxy unittable[UnitMax];
	MissileDrawProxy missiletable[MAX_MISSILES * 9];
	int nunits;
	int nmissiles;

	void Update(const CViewport *const vp)
	{
		//
		// We find and sort units after draw level.
		//
		if (lock.TryLock()) {
			nunits = FindAndSortUnits(vp, unittable);
			nmissiles = FindAndSortMissiles(vp, missiletable, MAX_MISSILES * 9);
			lock.UnLock();
		}
	}

	void Draw(const CViewport *const vp)
	{
		int i = 0, j = 0;
		lock.Lock ();
		while (i < nunits && j < nmissiles) {
			if (unittable[i].Type->DrawLevel <= missiletable[j].Type->DrawLevel) {
				unittable[i].Draw(vp);
				++i;
			} else {
				missiletable[j].DrawMissile(vp);
				++j;
			}
		}
		for (; i < nunits; ++i) {
			unittable[i].Draw(vp);
		}
		for (; j < nmissiles; ++j) {
			missiletable[j].DrawMissile(vp);
		}
		lock.UnLock();
	}


};

void CViewport::UpdateUnits()
{
	if (!Proxy) {
		Proxy = new CDrawProxy();
	}
	Proxy->Update(this);
}

/**
**  Draw a map viewport.
*/
void CViewport::Draw() const
{

	PushClipping();
	SetClipping(this->X, this->Y, this->EndX, this->EndY);

	/* this may take while */
	this->DrawMapBackgroundInViewport();

	CurrentViewport = this;
	if (Proxy) {
		Proxy->Draw(this);
	} else 	{
		CUnit *unittable[UnitMax];
		Missile* missiletable[MAX_MISSILES * 9];

		//
		// We find and sort units after draw level.
		//
		int nunits = FindAndSortUnits(this, unittable);
		int nmissiles = FindAndSortMissiles(this, missiletable, MAX_MISSILES * 9);
		int i = 0;
		int j = 0;

		while (i < nunits && j < nmissiles) {
			if (unittable[i]->Type->DrawLevel <= missiletable[j]->Type->DrawLevel) {
				unittable[i]->Draw(this);
				++i;
			} else {
				missiletable[j]->DrawMissile(this);
				++j;
			}
		}
		for (; i < nunits; ++i) {
			unittable[i]->Draw(this);
		}
		for (; j < nmissiles; ++j) {
			missiletable[j]->DrawMissile(this);
		}
	}

	this->DrawMapFogOfWar();

	//
	// Draw orders of selected units.
	// Drawn here so that they are shown even when the unit is out of the screen.
	//
	//FIXME: This is still unsecure during parallel
	if (Preference.ShowOrders < 0 ||
		(ShowOrdersCount >= GameCycle) || (KeyModifiers & ModifierShift)) {
		for (int i = 0; i < NumSelected; ++i) {
			ShowOrder(*Selected[i]);
		}
	}

	DrawBorder();

	PopClipping();
}

/**
**  Draw border around the viewport
*/
void CViewport::DrawBorder() const
{
	// if we a single viewport, no need to denote the "selected" one
	if (UI.NumViewports == 1) {
		return;
	}

	Uint32 color = ColorBlack;
	if (this == UI.SelectedViewport) {
		color = ColorOrange;
	}

	Video.DrawRectangle(color, this->X, this->Y, this->EndX - this->X + 1,
		this->EndY - this->Y + 1);
}

CViewport::~CViewport() {
	if (Proxy) {
		delete Proxy;
	}
}

/**
**  Initialize the fog of war.
**  Build tables, setup functions.
*/
void CMap::Init()
{
	InitFogOfWar();
}

//@}
