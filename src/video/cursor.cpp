//       _________ __                 __                               
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/ 
//  ______________________			     ______________________
//			  T H E	  W A R	  B E G I N S
//	   Stratagus - A free fantasy real time strategy game engine
//
/**@name cursor.c	-	The cursors. */
//
//	(c) Copyright 1998,2000-2002 by Lutz Sammer and Nehal Mistry
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
#include "video.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "cursor.h"
#include "tileset.h"
#include "map.h"
#include "interface.h"
#include "ui.h"

#include "intern_video.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/**
**	Number of bytes needed for current video-mode
*/
local int MemSize;

/**
**	Cursor-type type definition
*/
global const char CursorTypeType[] = "cursor-type";

/**
**	Define cursor-types.
**
**	@todo FIXME: Should this be move to ui part?
*/
global CursorType* Cursors;

global CursorStates CursorState;/// current cursor state (point,...)
global int CursorAction;	/// action for selection
global int CursorValue;		/// value for CursorAction (spell type f.e.)

	//Event changed mouse position, can alter at any moment
global int CursorX;		/// cursor position on screen X
global int CursorY;		/// cursor position on screen Y

global int CursorStartX;	/// rectangle started on screen X
global int CursorStartY;	/// rectangle started on screen Y

global int SubScrollX;          /// pixels the mouse moved while scrolling
global int SubScrollY;          /// pixels the mouse moved while scrolling

    /// X position of starting point of selection rectangle, in screen pixels.
global int CursorStartScrMapX;
    /// Y position of starting point of selection rectangle, in screen pixels.
global int CursorStartScrMapY;


/*--- DRAW BUILDING  CURSOR ------------------------------------------------*/
local int BuildingCursor;		/// Flag (0/1): last cursor was building

    /// area of tiles covered by building cursor (SX,SY;EX,EY)
local int BuildingCursorSX;
    /// area of tiles covered by building cursor (SX,SY;EX,EY)
local int BuildingCursorSY;
    /// area of tiles covered by building cursor (SX,SY;EX,EY)
local int BuildingCursorEX;
    /// area of tiles covered by building cursor (SX,SY;EX,EY)
local int BuildingCursorEY;

global UnitType* CursorBuilding;	/// building cursor


/*--- DRAW SPRITE CURSOR ---------------------------------------------------*/
	// Saved area after draw cursor, needed later to hide it again
	// (OldCursorW!=0 denotes it's defined)
local int OldCursorInvalidate;	/// flag (0/1): if cursor need invalidate
local int OldCursorX;		/// saved cursor position on screen X
local int OldCursorY;		/// saved cursor position on screen Y
local int OldCursorW;		/// saved cursor width in pixel
local int OldCursorH;		/// saved cursor height in pixel
global CursorType* GameCursor;	/// current shown cursor-type

	// Area which is already hidden, but needed for invalidate
	// (HiddenCursorW!=0 denotes it's defined)
local int HiddenCursorX;	/// saved cursor position on screen X
local int HiddenCursorY;	/// saved cursor position on screen Y
local int HiddenCursorW;	/// saved cursor width in pixel
local int HiddenCursorH;	/// saved cursor height in pixel

        /// Memory re-use, so can be defined although no save present!
local int OldCursorSize;	/// size of saved cursor image
#ifdef USE_SDL_SURFACE
local SDL_Surface* OldCursorImage;		/// background saved behind cursor
#else
local void* OldCursorImage;		/// background saved behind cursor
#endif

    /**
    **	Function pointer: Save 2D image behind sprite cursor
    **
    **	@param x	Screen X pixels coordinate for left-top corner.
    **	@param y	Screen Y pixels coordinate for left-top corner.
    **	@param w	Width in pixels for image starting at left-top.
    **	@param h	Height in pixels for image starting at left-top.
    **
    **	@note the complete image should be in TheScreen (no clipping) and
    **	non-empty
    **     (x >= 0, y >= 0, w > 0, h > 0, (x + w - 1) <= VideoWidth, (y + h - 1) <= VideoHeight)
    */
#ifdef USE_SDL_SURFACE
local void SaveCursorBackground(int x, int y, int w, int h);
    /// Function pointer: Load background behind cursor
local void LoadCursorBackground(int x, int y, int w, int h);
#else
local void (*SaveCursorBackground)(int x, int y, int w, int h);
    /// Function pointer: Load background behind cursor
local void (*LoadCursorBackground)(int x, int y, int w, int h);
#endif

/*--- DRAW RECTANGLE CURSOR ------------------------------------------------*/
	// Saved area after draw rectangle, needed later to hide it again
	// (OldCursorRectangleW != 0 denotes it's defined)
local int OldCursorRectangleInvalidate;	/// flag (0/1): ..need invalidate
local int OldCursorRectangleX;		/// saved cursor position on screen X
local int OldCursorRectangleY;		/// saved cursor position on screen Y
local int OldCursorRectangleW;		/// saved cursor width in pixel
local int OldCursorRectangleH;		/// saved cursor height in pixel
#ifdef USE_SDL_SURFACE
local SDL_Surface* OldCursorRectangle;		/// background saved behind rectangle
#else
local void* OldCursorRectangle;		/// background saved behind rectangle
#endif

	// Area which is already hidden, but needed for invalidate
	// (HiddenCursorRectangleW != 0 denotes it's defined)
local int HiddenCursorRectangleX;	/// saved cursor position on screen X
local int HiddenCursorRectangleY;	/// saved cursor position on screen Y
local int HiddenCursorRectangleW;	/// saved cursor width in pixel
local int HiddenCursorRectangleH;	/// saved cursor height in pixel

/**
**	Function pointer: Save rectangle behind cursor
**
**	@param buffer	Buffer in which the graphic is stored.
**	@param x	Screen X pixels coordinate for left-top corner.
**	@param y	Screen Y pixels coordinate for left-top corner.
**	@param w	Width in pixels for rectangle starting at left-top.
**	@param h	Height in pixels for rectangle starting at left-top.
**
**	@note the complete rectangle should be in TheScreen (no clipping) and
**	non-empty
**     (x >= 0, y >= 0, w > 0, h > 0, (x + w - 1) <= VideoWidth, (y + h - 1) <= VideoHeight)
*/
#ifdef USE_SDL_SURFACE
global void SaveCursorRectangle(void* buffer, int x, int y, int w, int h);
#else
global void (*SaveCursorRectangle)(void* buffer, int x, int y, int w, int h);
#endif

/**
**	Function pointer: Load rectangle behind cursor
**
**	@param buffer	Buffer in which the graphic is stored.
**	@param x	Screen X pixels coordinate.
**	@param y	Screen Y pixels coordinate.
**	@param w	Width in pixels.
**	@param h	Height in pixels.
**
**	@note rectangle previously saved with SaveCursorRectangle(x,y,w,h)
*/
#ifdef USE_SDL_SURFACE
global void LoadCursorRectangle(void* buffer, int x, int y, int w, int h);
#else
global void (*LoadCursorRectangle)(void* buffer, int x, int y, int w, int h);
#endif

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Load all cursor sprites.
**
**	@param race	Cursor graphics of this race to load.
*/
global void LoadCursors(const char* race)
{
    int i;
    const char* file;

    //
    //	Free old cursor sprites.
    //
    for (i = 0; Cursors[i].OType; ++i) {
	VideoSaveFree(Cursors[i].Sprite);
	Cursors[i].Sprite = NULL;
    }

    //
    //	Load the graphics
    //
    for (i = 0; Cursors[i].OType; ++i) {
	//
	//	Only load cursors of this race or universal cursors.
	//
	if (Cursors[i].Race && strcmp(Cursors[i].Race, race)) {
	    continue;
	}

	file = Cursors[i].File;
	if (file) {
	    char* buf;

	    buf = alloca(strlen(file) + 9 + 1);
	    file = strcat(strcpy(buf,"graphics/"), file);
	    ShowLoadProgress("Cursor %s", file);
#ifdef USE_SDL_SURFACE
	    Cursors[i].Sprite = LoadSprite(file,
		Cursors[i].Width, Cursors[i].Height);
#else
	    Cursors[i].Sprite = LoadSprite(file,
		Cursors[i].Width, Cursors[i].Height);
#endif
	}
    }
}

/**
**	Find the cursor-type of with this identifier.
**
**	@param ident	Identifier for the cursor (from config files).
**
**	@return		Returns the matching cursor-type.
**
**	@note If we have more cursors, we should add hash to find them faster.
*/
global CursorType* CursorTypeByIdent(const char* ident)
{
    CursorType* cursortype;

    for (cursortype = Cursors; cursortype->OType; ++cursortype) {
	if (strcmp(cursortype->Ident, ident)) {
	    continue;
	}
#ifdef USE_SDL_SURFACE
	if (!cursortype->Race || cursortype->Sprite) {
#else
	if (!cursortype->Race || cursortype->Sprite) {
#endif
	    return cursortype;
	}
    }
    DebugLevel0Fn("Cursor `%s' not found, please check your code.\n" _C_ ident);
    return NULL;
}

/*----------------------------------------------------------------------------
--	DRAW RECTANGLE CURSOR
----------------------------------------------------------------------------*/

#ifdef USE_SDL_SURFACE
global void LoadCursorRectangle(void* buffer, int x, int y, int w, int h)
{
    // FIXME: todo
    int i;
    int bpp;

    return;

    bpp = TheScreen->format->BytesPerPixel;
    SDL_LockSurface(TheScreen);
    for (i = 0; i < h; ++i) {
	memcpy(&((char*)TheScreen->pixels)[x * bpp + (y + i) * TheScreen->pitch], buffer, w * bpp);
    }
    SDL_UnlockSurface(TheScreen);
}

global void SaveCursorRectangle(void* buffer, int x, int y, int w, int h)
{
    int i;
    int bpp;

    return;

    bpp = TheScreen->format->BytesPerPixel;
    SDL_LockSurface(TheScreen);
    for (i = 0; i < h; ++i) {
	memcpy(buffer, &((char*)TheScreen->pixels)[x * bpp + (y + i) * TheScreen->pitch], w * bpp);
    }
    SDL_UnlockSurface(TheScreen);
    // FIXME: todo
}
#else
/**
**	Puts stored 'image' from SAVECURSORRECTANGLE back on the screen.
**      Note w and h are both > 0
**
**	FIXME: this kind of macros are hard to single step with gdb.
**	FIXME: inline functions should have the same speed and are debugable.
*/
#define LOADCURSORRECTANGLE(buffer, video, memtype, x, y, w, h)  { \
    const memtype* sp; \
    memtype* dp; \
    sp = (memtype*)buffer; \
    dp = video + y * VideoWidth + x; \
    memcpy(dp, sp, w * sizeof(memtype)); \
    if (--h) { \
	sp += w; \
	dp += VideoWidth; \
	while (--h) { \
	    *dp = *sp++; \
	    dp[w - 1] = *sp++; \
	    dp += VideoWidth; \
	} \
	memcpy(dp, sp, w * sizeof(memtype)); \
    } \
}

/**
**	Saves 'image' of screen overlapped by rectangle cursor, to be able to
**      restore it later using LOADCURSORRECTANGLE.
**      Note w and h > 0
**
**	FIXME: this kind of macros are hard to single step with gdb.
**	FIXME: inline functions should have the same speed and are debugable.
*/
#define SAVECURSORRECTANGLE(buffer, video, memtype, x, y, w, h)  { \
    const memtype* sp; \
    memtype* dp; \
    sp = video + y * VideoWidth + x; \
    dp = (memtype*)buffer; \
    memcpy(dp, sp, w * sizeof(memtype)); \
    if (--h) { \
	dp += w; \
	sp += VideoWidth; \
	while (--h) { \
	    *dp++ = *sp; \
	    *dp++ = sp[w - 1]; \
	    sp += VideoWidth; \
	} \
	memcpy(dp, sp, w * sizeof(memtype)); \
    } \
}


/**
**	Restore cursor rectangle for 8bpp frame buffer.
**	 (See description function pointer LoadCursorRectangle)
**	@see LoadCursorRectangle
*/
global void LoadCursorRectangle8(void* buffer, int x, int y, int w, int h)
{
    LOADCURSORRECTANGLE(buffer, VideoMemory8, VMemType8, x, y, w, h);
}

/**
**	Restore cursor rectangle for 16bpp frame buffer.
**	 (See description function pointer LoadCursorRectangle)
**	@see LoadCursorRectangle
*/
global void LoadCursorRectangle16(void* buffer, int x, int y, int w, int h)
{
    LOADCURSORRECTANGLE(buffer, VideoMemory16, VMemType16, x, y, w, h);
}

/**
**	Restore cursor rectangle for 24bpp frame buffer.
**	 (See description function pointer LoadCursorRectangle)
**	@see LoadCursorRectangle
*/
global void LoadCursorRectangle24(void* buffer, int x, int y, int w, int h)
{
    LOADCURSORRECTANGLE(buffer,VideoMemory24, VMemType24, x, y, w, h);
}

/**
**	Restore cursor rectangle for 32bpp frame buffer.
**	 (See description function pointer LoadCursorRectangle)
**	@see LoadCursorRectangle
*/
global void LoadCursorRectangle32(void* buffer, int x, int y, int w, int h)
{
    LOADCURSORRECTANGLE(buffer, VideoMemory32, VMemType32, x, y, w, h);
}

/**
**	Save cursor rectangle for 8bpp frame buffer.
**	 (See description function pointer SaveCursorRectangle)
**	@see SaveCursorRectangle
*/
global void SaveCursorRectangle8(void* buffer, int x, int y, int w, int h)
{
    SAVECURSORRECTANGLE(buffer, VideoMemory8, VMemType8, x, y, w, h);
}

/**
**	Save cursor rectangle for 16bpp frame buffer.
**	 (See description function pointer SaveCursorRectangle)
**	@see SaveCursorRectangle
*/
global void SaveCursorRectangle16(void* buffer, int x, int y, int w, int h)
{
    SAVECURSORRECTANGLE(buffer, VideoMemory16, VMemType16, x, y, w, h);
}

/**
**	Save cursor rectangle for 24bpp frame buffer.
**	 (See description function pointer SaveCursorRectangle)
**	@see SaveCursorRectangle
*/
global void SaveCursorRectangle24(void* buffer, int x, int y, int w, int h)
{
    SAVECURSORRECTANGLE(buffer, VideoMemory24, VMemType24, x, y, w, h);
}

/**
**	Save cursor rectangle for 32bpp frame buffer.
**	 (See description function pointer SaveCursorRectangle)
**	@see SaveCursorRectangle
*/
global void SaveCursorRectangle32(void* buffer, int x, int y, int w, int h)
{
    SAVECURSORRECTANGLE(buffer, VideoMemory32, VMemType32, x, y, w, h);
}
#endif

/**
**	Draw rectangle cursor when visible, defined by
**      OldCursorRectangleW (!=0),..
**      Pre: for this to work OldCursorRectangleW should be 0 upfront
**
**	@param x	Screen x start position of rectangle
**	@param y	Screen y start position of rectangle
**	@param x1	Screen x end position of rectangle
**	@param y1	Screen y end position of rectangle
*/
local void DrawVisibleRectangleCursor(int x, int y, int x1, int y1)
{
    int w;
    int h;
    const Viewport* vp;

    //
    //	Clip to map window.
    //	FIXME: should re-use CLIP_RECTANGLE in some way from linedraw.c ?
    //
    vp = TheUI.SelectedViewport;
    if (x1 < vp->X) {
	x1 = vp->X;
    } else if (x1 > vp->EndX) {
	x1 = vp->EndX;
    }
    if (y1 < vp->Y) {
	y1 = vp->Y;
    } else if (y1 > vp->EndY) {
	y1 = vp->EndY;
    }

    if (x > x1) {
	x = x1;
	w = CursorStartX - x + 1;
    } else {
	w = x1 - x + 1;
    }
    if (y > y1) {
	y = y1;
	h = CursorStartY - y + 1;
    } else {
	h = y1 - y + 1;
    }

    if (w && h) {
	SaveCursorRectangle(OldCursorRectangle,
	    OldCursorRectangleX = x,OldCursorRectangleY = y,
	    OldCursorRectangleW = w,OldCursorRectangleH = h);
	VideoDrawRectangleClip(ColorGreen, x, y, w, h);
	OldCursorRectangleInvalidate = 1;
    }
}

/*----------------------------------------------------------------------------
--	DRAW SPRITE CURSOR
----------------------------------------------------------------------------*/
#ifdef USE_SDL_SURFACE
local void LoadCursorBackground(int x, int y, int w, int h)
{
    SDL_Rect drect;
    SDL_Rect srect;

    srect.x = 0;
    srect.y = 0;
    srect.w = w;
    srect.h = h;

    drect.x = x;
    drect.y = y;

    SDL_BlitSurface(OldCursorImage, &srect, TheScreen, &drect);
}

local void SaveCursorBackground(int x, int y, int w, int h)
{
    SDL_Rect srect;

    srect.x = x;
    srect.y = y;
    srect.w = w;
    srect.h = h;

    SDL_BlitSurface(TheScreen, &srect, OldCursorImage, NULL);
}
#else
/**
**	Restore cursor background for 8bpp frame buffer.
**
**	@param x	Screen X pixels coordinate.
**	@param y	Screen Y pixels coordinate.
**	@param w	Width in pixels.
**	@param h	Height in pixels.
*/
local void LoadCursorBackground8(int x, int y, int w, int h)
{
    const VMemType8* sp;
    VMemType8* dp;

    sp = OldCursorImage;
    dp = VideoMemory8 + y * VideoWidth + x;
    while (h--) {
	memcpy(dp, sp, w * sizeof(VMemType8));
	sp += w;
	dp += VideoWidth;
    }
}

/**
**	Restore cursor background for 16bpp frame buffer.
**
**	@param x	Screen X pixels coordinate.
**	@param y	Screen Y pixels coordinate.
**	@param w	Width in pixels.
**	@param h	Height in pixels.
*/
local void LoadCursorBackground16(int x, int y, int w, int h)
{
    const VMemType16* sp;
    VMemType16* dp;

    sp = OldCursorImage;
    dp = VideoMemory16 + y * VideoWidth + x;
    while (h--) {
	memcpy(dp, sp, w * sizeof(VMemType16));
	sp += w;
	dp += VideoWidth;
    }
}

/**
**	Restore cursor background for 24bpp frame buffer.
**
**	@param x	Screen X pixels coordinate.
**	@param y	Screen Y pixels coordinate.
**	@param w	Width in pixels.
**	@param h	Height in pixels.
*/
local void LoadCursorBackground24(int x, int y, int w, int h)
{
    const VMemType24* sp;
    VMemType24* dp;

    sp = OldCursorImage;
    dp = VideoMemory24 + y * VideoWidth+x;
    while (h--) {
	memcpy(dp, sp, w * sizeof(VMemType24));
	sp += w;
	dp += VideoWidth;
    }
}

/**
**	Restore cursor background for 32bpp frame buffer.
**
**	@param x	Screen X pixels coordinate.
**	@param y	Screen Y pixels coordinate.
**	@param w	Width in pixels.
**	@param h	Height in pixels.
*/
local void LoadCursorBackground32(int x, int y, int w, int h)
{
    const VMemType32* sp;
    VMemType32* dp;

    sp = OldCursorImage;
    dp = VideoMemory32 + y * VideoWidth + x;
    while (h--) {
	memcpy(dp, sp, w * sizeof(VMemType32));
	sp += w;
	dp += VideoWidth;
    }
}

/**
**	Save cursor background for 8bpp frame buffer.
**
**	@param x	Screen X pixels coordinate.
**	@param y	Screen Y pixels coordinate.
**	@param w	Width in pixels.
**	@param h	Height in pixels.
*/
local void SaveCursorBackground8(int x, int y, int w, int h)
{
    VMemType8* dp;
    VMemType8* sp;

    dp = OldCursorImage;
    sp = VideoMemory8 + y * VideoWidth + x;
    while (h--) {
	memcpy(dp, sp, w * sizeof(VMemType8));
	dp += w;
	sp += VideoWidth;
    }
}

/**
**	Save cursor background for 16bpp frame buffer.
**
**	@param x	Screen X pixels coordinate.
**	@param y	Screen Y pixels coordinate.
**	@param w	Width in pixels.
**	@param h	Height in pixels.
*/
local void SaveCursorBackground16(int x, int y, int w, int h)
{
    VMemType16* dp;
    const VMemType16* sp;

    dp = OldCursorImage;
    sp = VideoMemory16 + y * VideoWidth + x;
    while (h--) {
	memcpy(dp, sp, w * sizeof(VMemType16));
	dp += w;
	sp += VideoWidth;
    }
}

/**
**	Save cursor background for 24bpp frame buffer.
**
**	@param x	Screen X pixels coordinate.
**	@param y	Screen Y pixels coordinate.
**	@param w	Width in pixels.
**	@param h	Height in pixels.
*/
local void SaveCursorBackground24(int x, int y, int w, int h)
{
    VMemType24* dp;
    const VMemType24* sp;

    dp = OldCursorImage;
    sp = VideoMemory24 + y * VideoWidth + x;
    while (h--) {
	memcpy(dp, sp, w * sizeof(VMemType24));
	dp += w;
	sp += VideoWidth;
    }
}

/**
**	Save cursor background for 32bpp frame buffer.
**
**	@param x	Screen X pixels coordinate.
**	@param y	Screen Y pixels coordinate.
**	@param w	Width in pixels.
**	@param h	Height in pixels.
*/
local void SaveCursorBackground32(int x, int y, int w, int h)
{
    VMemType32* dp;
    const VMemType32* sp;

    dp = OldCursorImage;
    sp = VideoMemory32 + y * VideoWidth + x;
    while (h--) {
	memcpy(dp, sp, w * sizeof(VMemType32));
	dp += w;
	sp += VideoWidth;
    }
}
#endif

/**
**	Destroy image behind cursor.
*/
global void DestroyCursorBackground(void)
{
    if (OldCursorImage) {
	free(OldCursorImage);
	OldCursorImage = NULL;
    }
    OldCursorSize = 0;
}

/**
**	Draw (sprite) cursor when visible, defined by
**      OldCursorW (!=0),..
**      Pre: for this to work OldCursorW should be 0 upfront
**
**	@param type	Cursor-type of the cursor to draw.
**	@param x	Screen x pixel position.
**	@param y	Screen y pixel position.
**	@param frame	Animation frame # of the cursor.
*/
local void DrawCursor(const CursorType* type, int x, int y, int frame)
{
    int size;
    int w;
    int h;
    int spritex;
    int spritey;

    DebugCheck(!type);
    //
    //	Save cursor position and size, for faster cursor redraw.
    //
    spritex = (x -= type->HotX);
    spritey = (y -= type->HotY);
#ifdef USE_SDL_SURFACE
    w = VideoGraphicWidth(type->Sprite);
    h = VideoGraphicHeight(type->Sprite);
#else
    w = VideoGraphicWidth(type->Sprite);
    h = VideoGraphicHeight(type->Sprite);
#endif

    //Reserve enough memory for background of sprite (also for future calls)
#ifdef USE_SDL_SURFACE
    size = w * h;
    if (OldCursorSize < size) {
	OldCursorImage = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, BMASK, GMASK, RMASK, AMASK);
	OldCursorSize = size;
    }
#else
    size = w * h * MemSize;
    if (OldCursorSize < size) {
	if (OldCursorImage) {
	    OldCursorImage = realloc(OldCursorImage, size);
	} else {
	    OldCursorImage = malloc(size);
	}
	OldCursorSize = size;
    }
#endif
    //Save (seen) area behind sprite
    CLIP_RECTANGLE(x, y, w, h);
    SaveCursorBackground(OldCursorX = x, OldCursorY = y,
	OldCursorW = w, OldCursorH = h);

    //Draw sprite (using its own clipping)  FIXME: prevent clipping twice
#ifdef USE_SDL_SURFACE
    VideoDrawClip(type->Sprite, frame, spritex, spritey);
#else
    VideoDrawClip(type->Sprite, frame, spritex, spritey);
#endif
    OldCursorInvalidate = 1;
}

/*----------------------------------------------------------------------------
--	DRAW BUILDING CURSOR
----------------------------------------------------------------------------*/
/**
**	Draw cursor for selecting building position.
*/
local void DrawBuildingCursor(void)
{
    int x;
    int y;
    int x1;
    int y1;
    int mx;
    int my;
#ifdef USE_SDL_SURFACE
    SDL_Color color;
#else
    VMemType color;
#endif
    int f;
    int w;
    int w0;
    int h;
    int mask;
    const Viewport* vp;
    int frame;

    // Align to grid
    vp = TheUI.MouseViewport;
    x = CursorX - (CursorX - vp->X) % TileSizeX;
    y = CursorY - (CursorY - vp->Y) % TileSizeY;
    BuildingCursorSX = mx = Viewport2MapX(vp, x);
    BuildingCursorSY = my = Viewport2MapY(vp, y);

    //
    //	Draw building
    //
    frame = CursorBuilding->Animations->Still[0].Frame +
	(CursorBuilding->Building ? 0 : CursorBuilding->NumDirections / 2 + 1 - 1);
    PushClipping();
    SetClipping(vp->X, vp->Y, vp->EndX, vp->EndY);
    DrawShadow(NULL, CursorBuilding, frame, x, y);
    GraphicPlayerPixels(ThisPlayer, CursorBuilding->Sprite);
    DrawUnitType(CursorBuilding, CursorBuilding->Sprite, frame, x, y);
    PopClipping();

    //
    //	Draw the allow overlay
    //
    f = CanBuildHere(CursorBuilding, mx, my);

    mask = CursorBuilding->MovementMask;
    h=CursorBuilding->TileHeight;
    BuildingCursorEY = my + h - 1;
    if (my + h > vp->MapY + vp->MapHeight) {	// reduce to view limits
	h = vp->MapY + vp->MapHeight - my;
    }
    w0 = CursorBuilding->TileWidth;	// reduce to view limits
    BuildingCursorEX = mx + w0 - 1;
    if (mx + w0 > vp->MapX + vp->MapWidth) {
	w0 = vp->MapX + vp->MapWidth - mx;
    }
    while (h--) {
	w = w0;
	while (w--) {
	    int basex;
	    int basey;
	    int i;
	    int j;

	    // FIXME: The field is covered by fog of war!
	    if (f && (CursorBuilding->MustBuildOnTop ||
		    CanBuildOn(mx + w, my + h, mask & ((NumSelected &&
			!CursorBuilding->BuilderOutside &&
			Selected[0]->X == mx + w && Selected[0]->Y == my + h) ?
			    ~(MapFieldLandUnit | MapFieldSeaUnit) : -1)))
		  && IsMapFieldExplored(ThisPlayer, mx + w, my + h))  {
		color = ColorGreen;
	    } else {
		color = ColorRed;
	    }
	    // FIXME: I could do this faster+better
	    /* latimerius: I'm not sure what you have in mind but I can
	     * at least move invariants out of the loops. */
	    basex = x + w * TileSizeX;
	    basey = y + h * TileSizeY;
	    for (y1 = 0; y1 < TileSizeY; ++y1) {
		j = basey + y1;
		for (x1 = y1 & 1; x1 < TileSizeX; x1 += 2) {
		    i = basex + x1;
		    if (i > vp->EndX) {
			break;
		    }
		    VideoDrawPixel(color, i, j);
		}
		if (j > vp->EndY) {
		    break;
		}
	    }
	}
    }
}


/*----------------------------------------------------------------------------
--	DRAW/HIDE CURSOR (interface for the outside world)
----------------------------------------------------------------------------*/
/**
**	Draw the cursor and prepare tobe restored by HideAnyCursor again.
**      Note: This function can be called, without calling HideAnyCursor first,
**            which means that this function should re-use/free memory of the
**            last call.
**            When calling multiple times, the old cursor is expected to be
**            overdrawn by something else (else HideAnyCursor is needed!)
**            Also the cursors are not invalidated (refresh on real screen)
**            here, but this is done by InvalidateCursorAreas.
**
**  FIXME: event handler should be temporary stopped while copying
**         CursorX, CursorY,.. because between two copy commands another
**         event can occure, which let invalid mouse position be delivered.
*/
global void DrawAnyCursor(void)
{
    // Disable any previous drawn cursor
    OldCursorInvalidate = OldCursorW = OldCursorRectangleInvalidate =
	OldCursorRectangleW = BuildingCursor = 0;

    //
    //	First, Selecting rectangle
    //
    if (CursorState == CursorStateRectangle &&
	    (CursorStartX != CursorX || CursorStartY != CursorY)) {
	DrawVisibleRectangleCursor(CursorStartX, CursorStartY, CursorX, CursorY);
    } else if (CursorBuilding && CursorOn == CursorOnMap) {
	//
	//	Or Selecting position for building
	//
	DrawBuildingCursor();
	BuildingCursor = 1;
    }

    //
    //	Last, Normal cursor.
    //  This will also save (part of) drawn rectangle cursor, but that's ok.
    //  Cursor May not Exist if we are loading a game or something. Only
    //	draw it if it exists
    //
    if (GameCursor) {
	DrawCursor(GameCursor, CursorX, CursorY, GameCursor->SpriteFrame);
    }
}

/**
**	Remove old cursor from display.
**      (in the opposite direction of DrawAnyCursor)
**      Note: this function can be called, without calling DrawAnyCursor first,
**            which means that cursors shouldn't be restored twice.
**            As cursors are, like DrawAnyCursor,  not invalidated here, it
**            still needs to be done by InvalidateCursorAreas.
*/
global void HideAnyCursor(void)
{
    //
    //	First, Normal cursor (might restore part of rectangle cursor also).
    //
    if (OldCursorW && OldCursorImage) {
        // restore area of visible cursor
        LoadCursorBackground(OldCursorX, OldCursorY, OldCursorW, OldCursorH);

        // save hidden area to be invalidated
        HiddenCursorX = OldCursorX;
        HiddenCursorY = OldCursorY;
        HiddenCursorW = OldCursorW;
        HiddenCursorH = OldCursorH;

        // Denote cursor no longer visible
        OldCursorW = 0;
    }

    //
    //	Last, Selecting rectangle
    //
    if (OldCursorRectangleW) {
        //  restore area of visible cursor
	LoadCursorRectangle(OldCursorRectangle,
	    OldCursorRectangleX, OldCursorRectangleY,
	    OldCursorRectangleW, OldCursorRectangleH);

        // save hidden area to be invalidated
        HiddenCursorRectangleX = OldCursorRectangleX;
        HiddenCursorRectangleY = OldCursorRectangleY;
        HiddenCursorRectangleW = OldCursorRectangleW;
        HiddenCursorRectangleH = OldCursorRectangleH;

        // Denote cursor no longer visible
        OldCursorRectangleW = 0;
    } else if (BuildingCursor) {
	//
	//	Or Selecting position for building
	//
	//NOTE: this will restore tiles themselves later in next video update
	MarkDrawAreaMap(BuildingCursorSX, BuildingCursorSY,
	    BuildingCursorEX, BuildingCursorEY);
        BuildingCursor = 0;
    }
}

/**
**	Animate the cursor.
**
**	@param ticks	Current tick
*/
global void CursorAnimate(unsigned ticks)
{
    static unsigned last = 0;

    if (!GameCursor) {
	return;
    }
    if (ticks > last + GameCursor->FrameRate) {
	last = ticks + GameCursor->FrameRate;
	GameCursor->SpriteFrame++;
	if ((GameCursor->SpriteFrame & 127) >=
#ifdef USE_SDL_SURFACE
		VideoGraphicFrames(GameCursor->Sprite)) {
#else
		VideoGraphicFrames(GameCursor->Sprite)) {
#endif
	    GameCursor->SpriteFrame = 0;
	}
	MustRedraw |= RedrawCursor;
    }
}

/**
**      Let an area be invalidated, but remembering if cursor is automaticly
**      invalidated with this area.
**      Note: building-cursor is already invalidated by redraw-map
**
**	@param x	left-top x-position of area on screen
**	@param y	left-top y-position of area on screen
**	@param w	width of area on screen
**	@param h	height of area on screen
*/
global void InvalidateAreaAndCheckCursor(int x, int y, int w, int h)
{
    int dx;
    int dy;

    //Invalidate area
    InvalidateArea(x, y, w, h);

    //Now check if cursor sprite is inside it, then no need for invalidate
    if (OldCursorInvalidate) {
	dx = OldCursorX - x;
	dy = OldCursorY - y;
	if (dx >= 0 && dy >= 0 && (w - dx) >= OldCursorW &&
		(h - dy) >= OldCursorH) {
	    OldCursorInvalidate = 0;
	}
    }

    //Now check if previously hidden cursor sprite is inside it..
    if (HiddenCursorW) {
	dx = HiddenCursorX - x;
	dy = HiddenCursorY - y;
	if (dx >= 0 && dy >= 0 && (w - dx) >= HiddenCursorW &&
	    (h - dy) >= HiddenCursorH) {
	    HiddenCursorW = 0;
	}
    }

    //Now check if cursor rectangle is inside it..
    if (OldCursorRectangleInvalidate) {
	dx = OldCursorRectangleX - x;
	dy = OldCursorRectangleY - y;
	if (dx >= 0 && dy >= 0 && (w - dx) >= OldCursorRectangleW &&
		(h - dy) >= OldCursorRectangleH) {
	    OldCursorRectangleInvalidate = 0;
	}
    }

    //Now check if previously hidden cursor rectangle is inside it..
    if (HiddenCursorRectangleW) {
	dx = HiddenCursorRectangleX - x;
	dy = HiddenCursorRectangleY - y;
	if (dx >= 0 && dy >= 0 && (w - dx) >= HiddenCursorRectangleW &&
		(h - dy) >= HiddenCursorRectangleH) {
	    HiddenCursorRectangleW = 0;
	}
    }
}

/**
**      Invalidate only the sides of a given rectangle (not its contents)
**
**	@param x	left-top x-position of rectangle on screen
**	@param y	left-top y-position of rectangle on screen
**	@param w	width of rectangle on screen
**	@param h	height of rectangle on screen
*/
local void InvalidateRectangle(int x, int y, int w, int h)
{
    InvalidateArea(x, y, w, 1); // top side
    if (--h > 0) {
	InvalidateArea(x, y + h, w, 1); // bottom side
	if (--h > 0) {
	    InvalidateArea(x, ++y, 1, h); // left side
	    if (--w > 0) {
		InvalidateArea(x + w, y, 1, h); // right side
	    }
	}
    }
}

/**
**      Let the (remaining) areas taken by the cursors, as determined by
**      DrawAnyCursor and InvalidateAreaAndCheckcursor,  be invalidated.
**      Note: building-cursor is already invalidated by redraw-map
*/
global void InvalidateCursorAreas(void)
{
    //Invalidate cursor sprite
    if (OldCursorInvalidate) {
	InvalidateArea(OldCursorX, OldCursorY, OldCursorW, OldCursorH);
	OldCursorInvalidate = 0;
    }

    //Invalidate hidden cursor sprite
    if (HiddenCursorW) {
	InvalidateArea(HiddenCursorX, HiddenCursorY, HiddenCursorW, HiddenCursorH);
	HiddenCursorW = 0;
    }

    //Invalidate cursor rectangle
    if (OldCursorRectangleInvalidate) {
	InvalidateRectangle(OldCursorRectangleX, OldCursorRectangleY,
	    OldCursorRectangleW, OldCursorRectangleH);
	OldCursorRectangleInvalidate = 0;
    }

    //Invalidate hidden cursor rectangle
    if (HiddenCursorRectangleW) {
	InvalidateRectangle(HiddenCursorRectangleX, HiddenCursorRectangleY,
	    HiddenCursorRectangleW, HiddenCursorRectangleH);
	HiddenCursorRectangleW = 0;
    }
}

#ifdef USE_OPENGL
local void LoadCursorRectangleOpenGL(void* buffer __attribute__((unused)),
    int x __attribute__((unused)), int y __attribute__((unused)),
    int w __attribute__((unused)), int h __attribute__((unused)))
{
}
local void SaveCursorRectangleOpenGL(void* buffer __attribute__((unused)),
    int x __attribute__((unused)), int y __attribute__((unused)),
    int w __attribute__((unused)), int h __attribute__((unused)))
{
}
local void LoadCursorBackgroundOpenGL(int x __attribute__((unused)),
    int y __attribute__((unused)),
    int w __attribute__((unused)), int h __attribute__((unused)))
{
}
local void SaveCursorBackgroundOpenGL(int x __attribute__((unused)),
    int y __attribute__((unused)),
    int w __attribute__((unused)), int h __attribute__((unused)))
{
}
#endif

/**
**	Setup the cursor part.
**
**	@todo	FIXME: Now max possible memory for OldCursorRectangle,
**		to be limited to Map?
*/
global void InitVideoCursors(void)
{
#ifdef USE_OPENGL
    SaveCursorBackground = SaveCursorBackgroundOpenGL;
    LoadCursorBackground = LoadCursorBackgroundOpenGL;
    MemSize = 1;
    SaveCursorRectangle = SaveCursorRectangleOpenGL;
    LoadCursorRectangle = LoadCursorRectangleOpenGL;
#else
    if (OldCursorRectangle) {	// memory of possible previous video-setting?
	free(OldCursorRectangle);
	OldCursorRectangle = 0;
    }

#ifdef USE_SDL_SURFACE
//    OldCursorImage = SDL_CreateRGBSurface(SDL_SWSURFACE, 40, 40, 8, 0, 0, 0, 0);
//    OldCursorRectangle = SDL_CreateRGBSurface(SDL_SWSURFACE, 40, 40, 8, 0, 0, 0, 0);
#else
    switch (VideoBpp) {
	case 8:
	    SaveCursorBackground = SaveCursorBackground8;
	    LoadCursorBackground = LoadCursorBackground8;
	    MemSize = sizeof(VMemType8);
	    SaveCursorRectangle = SaveCursorRectangle8;
	    LoadCursorRectangle = LoadCursorRectangle8;
	    break;
	case 15:
	case 16:
	    SaveCursorBackground = SaveCursorBackground16;
	    LoadCursorBackground = LoadCursorBackground16;
	    MemSize = sizeof(VMemType16);
	    SaveCursorRectangle = SaveCursorRectangle16;
	    LoadCursorRectangle = LoadCursorRectangle16;
	    break;
	case 24:
	    SaveCursorBackground = SaveCursorBackground24;
	    LoadCursorBackground = LoadCursorBackground24;
	    MemSize = sizeof(VMemType24);
	    SaveCursorRectangle = SaveCursorRectangle24;
	    LoadCursorRectangle = LoadCursorRectangle24;
	    break;
	case 32:
	    SaveCursorBackground = SaveCursorBackground32;
	    LoadCursorBackground = LoadCursorBackground32;
	    MemSize = sizeof(VMemType32);
	    SaveCursorRectangle = SaveCursorRectangle32;
	    LoadCursorRectangle = LoadCursorRectangle32;
	    break;
	default:
	    DebugLevel0Fn("unsupported %d bpp\n" _C_ VideoBpp);
	    abort();
    }
    OldCursorRectangle = malloc((2 * VideoWidth + 2 * (VideoHeight - 2)) * MemSize);
#endif // ifdef USE_SDL_SURFACE
#endif

    CursorX = VideoWidth / 2;
    CursorY = VideoHeight / 2;
}

/**
**	Save cursor state.
*/
global void SaveCursors(CLFile* file)
{
    int i;

    CLprintf(file, "\n;;; -----------------------------------------\n");
    CLprintf(file, ";;; MODULE: cursors $Id$\n\n");

    for (i = 0; Cursors[i].OType; ++i) {
	CLprintf(file, "(define-cursor '%s '%s\n",
	    Cursors[i].Ident, Cursors[i].Race ? Cursors[i].Race : "any");
	CLprintf(file, "  'image \"%s\"\n", Cursors[i].File);
	CLprintf(file, "  'hot-spot '(%d %d) ", Cursors[i].HotX, Cursors[i].HotY);
	CLprintf(file, "'size '(%d %d) ", Cursors[i].Width, Cursors[i].Height);
	CLprintf(file, ")\n\n");
    }

    // Not ready:
    CLprintf(file, ";;(set-game-cursor! '%s)\n", GameCursor->Ident);
    // FIXME: what about the other variables???
    switch (CursorState) {
	case CursorStatePoint:
	    CLprintf(file, ";;(cursor-state 'point)\n");
	    break;
	case CursorStateSelect:
	    CLprintf(file, ";;(cursor-state 'select)\n");
	    break;
	case CursorStateRectangle:
	    CLprintf(file, ";;(cursor-state 'rectangle)\n");
	    break;
    }
    CLprintf(file, ";;(cursor-action %d)\n", CursorAction);
    CLprintf(file, ";;(cursor-value %d)\n", CursorValue);
    CLprintf(file, ";;(cursor-building '%s)\n",
	CursorBuilding ? CursorBuilding->Ident : "()");
    CLprintf(file, ";;(cursor-position '(%d %d)\n", CursorX, CursorY);
    CLprintf(file, ";;(cursor-start '(%d %d)\n", CursorStartX, CursorStartY);
    CLprintf(file, ";;(unit-under-cursor %s\n",
	UnitUnderCursor ? UnitReference(UnitUnderCursor) : "()");
}

/**
**	Cleanup cursor module
*/
global void CleanCursors(void)
{
    int i;

    for (i = 0; Cursors[i].OType; ++i) {
	free(Cursors[i].Ident);
	free(Cursors[i].Race);
	free(Cursors[i].File);
	VideoSaveFree(Cursors[i].Sprite);
    }
    free(Cursors);
    Cursors = NULL;

    free(OldCursorRectangle);
    OldCursorRectangle = 0;

    DestroyCursorBackground();

    CursorBuilding = 0;
    GameCursor = 0;
    UnitUnderCursor = NoUnitP;
}

//@}
