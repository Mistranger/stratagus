//       _________ __                 __                               
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ \ 
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/ 
//  ______________________			     ______________________
//			  T H E	  W A R	  B E G I N S
//	   Stratagus - A free fantasy real time strategy game engine
//
/**@name font.h		-	The font headerfile. */
//
//	(c) Copyright 1998-2003 by Lutz Sammer and Jimmy Salmon
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

#ifndef __FONT_H__
#define __FONT_H__

//@{

/*----------------------------------------------------------------------------
--	Documentation
----------------------------------------------------------------------------*/

/**
**	@struct _color_font_ font.h
**
**	\#include "font.h"
**
**	typedef struct _color_font_ ColorFont;
**
**		Defines the fonts used in the Stratagus engine. We support
**		proportional multicolor fonts of 7 colors. The eighth color is
**		transparent. (Currently the fonts aren't packed)
**
**	ColorFont::File
**
**		File containing the graphics for the font.
**
**	ColorFont::Width
**
**		Maximal width of a character in pixels.
**
**	ColorFont::Height
**
**		Height of all characters in pixels.
**
**	ColorFont::CharWidth[]
**
**		The width of each font glyph in pixels. The index 0 is the
**		width of the SPACE (' ', 0x20).
**
**	ColorFont::Graphic
**
**		Contains the graphics of the font, loaded from ColorFont::File.
**		Only 7 colors are supported.
**		@li #0	 is background color
**		@li #1	 is the light font color
**		@li #2	 is the middle = main font color
**		@li #3	 is the dark font color
**		@li #4	 is the font/shadow antialias color
**		@li #5	 is the dark shadow color
**		@li #6	 is the light shadow color
**		@li #255 is transparent
*/

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include "ccl.h"

/*----------------------------------------------------------------------------
--	Definitions
----------------------------------------------------------------------------*/

    /// Color font definition
typedef struct _color_font_ {
    char*	File;			/// File containing font data

    int		Width;			/// Max width of characters in file
    int		Height;			/// Max height of characters in file

    char	CharWidth[208];		/// Real font width (starting with ' ')

// --- FILLED UP ---

    Graphic*	Graphic;		/// Graphic object used to draw
} ColorFont;

/**
**	Font selector for the font functions.
*/
enum _game_font_ {
    SmallFont,				/// Small font used in stats
    GameFont,				/// Normal font used in game
    LargeFont,				/// Large font used in menus
    SmallTitleFont,			/// Small font used in episoden titles
    LargeTitleFont,			/// Large font used in episoden titles
    User1Font,				/// User font 1
    User2Font,				/// User font 2
    User3Font,				/// User font 3
    User4Font,				/// User font 4
    User5Font,				/// User font 5
    // ... more to come or not
    MaxFonts,				/// Number of fonts supported
};

/**
**	Color selector for the font functions.
*/
#define FontRed "red"
#define FontGreen "green"
#define FontYellow "yellow"
#define FontWhite "white"
#define FontGrey "grey"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/**
**	Font names
**	FIXME: should use the names of the real fonts.
*/
extern char *FontNames[];

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

    /// Set the default text colors for normal and reverse text
extern void SetDefaultTextColors(char* normal,char* reverse);
    /// Get the default text colors for normal and reverse text
extern void GetDefaultTextColors(char** normalp,char** reversep);
    /// Returns the pixel length of a text
extern int VideoTextLength(unsigned font,const unsigned char* text);
    /// Returns the height of the font
extern int VideoTextHeight(unsigned font);
    /// Draw text unclipped
extern int VideoDrawText(int x,int y,unsigned font,const unsigned char* text);
    /// Draw text unclipped
extern int VideoDrawTextClip(int x,int y,unsigned font,const unsigned char* text);
    /// Draw reverse text unclipped
extern int VideoDrawReverseText(int x,int y,unsigned font,const unsigned char* text);
    /// Draw text centered and unclipped
extern int VideoDrawTextCentered(int x,int y,unsigned font,const unsigned char* text);
    /// Draw number unclipped
extern int VideoDrawNumber(int x,int y,unsigned font,int number);
    /// Draw reverse number unclipped
extern int VideoDrawReverseNumber(int x,int y,unsigned font,int number);
    /// Draw number clipped
extern int VideoDrawNumberClip(int x,int y,unsigned font,int number);
    /// Draw reverse number clipped
extern int VideoDrawReverseNumberClip(int x,int y,unsigned font,int number);

    /// Load and initialize the fonts
extern void LoadFonts(void);
    /// Register ccl features
extern void FontsCclRegister(void);
    /// Cleanup the font module
extern void CleanFonts(void);
    /// Check if font is loaded
extern int IsFontLoaded(unsigned font);
    /// Font symbol to id
extern int CclFontByIdentifier(SCM type);

//@}

#endif	// !__FONT_H__
