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
/**@name cursor.h	-	The cursors header file. */
//
//	(c) Copyright 1998-2001 by Lutz Sammer
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

#ifndef __CURSOR_H__
#define __CURSOR_H__

//@{

/*----------------------------------------------------------------------------
--	Documentation
----------------------------------------------------------------------------*/

/**
**	@struct _cursor_type_ cursor.h
**
**	\#include "cursor.h"
**
**	typedef struct _cursor_type_ CursorType;
**
**	This structure contains all informations about a cursor.
**	The cursor changes depending of the current user input state.
**	A cursor can have transparent areas and color cycle animated.
**
**	In the future it is planned to support animated cursors.
**
**	The cursor-type structure members:
**
**	CursorType::OType
**
**		Object type (future extensions).
**
**	CursorType::Ident
**
**		Unique identifier of the cursor, used to reference it in config
**		files and during startup. Don't use this in game, use instead
**		the pointer to this structure.
**
**	CursorType::Race
**
**		Owning Race of this cursor ("human", "orc", "alliance",
**		"mythical", ...). If NULL, this cursor could be used by any
**		race.
**
**	CursorType::File
**
**		File containing the image graphics of the cursor.
**
**	CursorType::HotX CursorType::HotY
**
**		Hot spot of the cursor in pixels. Relative to the sprite origin
**		(0,0). The hot spot of a cursor is the point to which Stratagus
**		refers in tracking the cursor's position.
**
**	CursorType::Width CursorType::Height
**
**		Size of the cursor in pixels.
**
**	CursorType::SpriteFrame
**
**		Current displayed cursor frame.
**		From 0 to CursorType::Graphic::NumFrames.
**
**	CursorType::FrameRate
**
**		Rate of changing the frames. The "rate" tells the engine how
**		many milliseconds to hold each frame of the animation.
**
**	@note	This is the first time that for timing ms are used! I would
**		change it to display frames.
**
**	CursorType::Graphic
**
**		Contains the sprite of the cursor, loaded from CursorType::File.
**		This can be a multicolor image with alpha or transparency.
*/

/**
**	@struct _cursor_config_ cursor.h
**
**	\#include "cursor.h"
**
**	typedef struct _cursor_config_ CursorConfig;
**
**	This structure contains all informations to reference/use a cursor.
**	It is normally used in other config structures.
**
**	CursorConfig::Name
**
**		Name to reference this cursor-type. Used while initialization.
**		(See CursorType::Ident)
**
**	CursorConfig::Cursor
**
**		Pointer to this cursor-type. Used while runtime.
*/

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include "player.h"
#include "video.h"

/*----------------------------------------------------------------------------
--	Definitions
----------------------------------------------------------------------------*/

    ///	Cursor-type typedef
typedef struct _cursor_type_ CursorType;

    ///	Private type which specifies the cursor-type
struct _cursor_type_ {
    const void*	OType;			/// Object type (future extensions)

    char*	Ident;			/// Identifier to reference it
    char*	Race;			/// Race name

    char*	File;			/// Graphic file of the cursor

    int		HotX;			/// Hot point x
    int		HotY;			/// Hot point y
    int		Width;			/// Width of cursor
    int		Height;			/// Height of cursor

    int		SpriteFrame;		/// Current displayed cursor frame
    int		FrameRate;		/// Rate of changing the frames

// --- FILLED UP ---

#ifdef USE_SDL_SURFACE
    Graphic*	Sprite;			/// Cursor sprite image
#else
    Graphic*	Sprite;			/// Cursor sprite image
#endif
};

    /// Cursor config reference
typedef struct _cursor_config_ {
    char*	Name;			/// Config cursor-type name
    CursorType*	Cursor;			/// Cursor-type pointer
} CursorConfig;

    /// Cursor state
typedef enum _cursor_states_ {
    CursorStatePoint,			/// Normal cursor
    CursorStateSelect,			/// Select position
    CursorStateRectangle,		/// Rectangle selecting
} CursorStates;

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern const char CursorTypeType[];	/// cursor-type type
extern CursorType* Cursors;		/// cursor-types description

extern CursorStates CursorState;	/// current cursor state (point,...)
extern int CursorAction;		/// action for selection
extern int CursorValue;			/// value for action (spell type f.e.)
extern UnitType* CursorBuilding;	/// building cursor

extern CursorType* GameCursor;		/// cursor-type
extern int CursorX;			/// cursor position on screen X
extern int CursorY;			/// cursor position on screen Y
extern int CursorStartX;		/// rectangle started on screen X
extern int CursorStartY;		/// rectangle started on screen Y
extern int CursorStartScrMapX;	/// the same in screen map coordinate system
extern int CursorStartScrMapY;	/// the same in screen map coordinate system
extern int SubScrollX;          /// pixels the mouse moved while scrolling
extern int SubScrollY;          /// pixels the mouse moved while scrolling

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

    /// Load all cursors
extern void LoadCursors(const char* racename);

    /// Cursor-type by identifier
extern CursorType* CursorTypeByIdent(const char* ident);

    /// Draw any cursor
extern void DrawAnyCursor(void);
    /// Hide any cursor
extern void HideAnyCursor(void);
    /// Animate the cursor
extern void CursorAnimate(unsigned ticks);

    /// Save/load rectangle region from/to screen
    /// Note: this is made extern for minimap only
#ifdef USE_SDL_SURFACE
extern void SaveCursorRectangle(void *buffer,int x,int y,int w,int h);
extern void LoadCursorRectangle(void *buffer,int x,int y,int w,int h);
#else
extern void (*SaveCursorRectangle)(void *buffer,int x,int y,int w,int h);
extern void (*LoadCursorRectangle)(void *buffer,int x,int y,int w,int h);
#endif

    /// Invalidate given area and check if cursor won't need any
extern void InvalidateAreaAndCheckCursor( int x, int y, int w, int h );
    /// Invalidate (remaining) cursor areas 
extern void InvalidateCursorAreas(void);

    /// Initialize the cursor module
extern void InitVideoCursors(void);
    /// Save the cursor definitions
extern void SaveCursors(CLFile*);
    /// Cleanup the cursor module
extern void CleanCursors(void);
    /// Destroy image behind cursor.
extern void DestroyCursorBackground(void);

//@}

#endif	// !__CURSOR_H__
