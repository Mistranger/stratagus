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
/**@name video.c	-	The universal video functions. */
//
//	(c) Copyright 1999-2002 by Lutz Sammer and Nehal Mistry
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

/**
**	  @page VideoModule Module - Video
**
**				There are lots of video functions available, therefore this
**				page tries to summarize these separately.
**
**				@note care must be taken what to use, how to use it and where
**				put new source-code. So please read the following sections
**				first.
**
**
**	  @section VideoMain Video main initialization
**
**			  The general setup of platform dependent video and basic video
**				functionalities is done with function @see InitVideo
**
**				We support (depending on the platform) resolutions:
**				640x480, 800x600, 1024x768, 1600x1200
**				with colors 8,15,16,24,32 bit
**
**				@see video.h @see video.c
**
**
**	  @section Deco Decorations
**
**				A mechanism beteen the Stratagus engine and draw routines
**				to make a screen refresh/update faster and accurate.
**				It will 'know' about overlapping screen decorations and draw
**				them all (partly) when one is to be updated.
**
**				See page @ref Deco for a detailed description.
**
**			  @see deco.h @see deco.c
**
**
**	  @section VideoModuleHigh High Level - video dependent functions
**
**				These are the video platforms that are supported, any platform
**				dependent settings/functionailty are located within each
**				separate files:
**
**				SDL				: Simple Direct Media for Linux,
**								  Win32 (Windows 95/98/2000), BeOs, MacOS
**								  (visit http://www.libsdl.org)
**
**				@see sdl.c
**
**
**	  @section VideoModuleLow  Low Level - draw functions
**
**				All direct drawing functions
**
**				@note you might need to use Decorations (see above), to prevent
**				drawing directly to screen in conflict with the video update.
**
**			  @see linedraw.c
**			  @see sprite.c
*/

/*----------------------------------------------------------------------------
--		Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stratagus.h"
#include "video.h"
#include "map.h"
#include "ui.h"
#include "cursor.h"
#include "iolib.h"

#include "intern_video.h"

#ifdef USE_SDL
#include "SDL.h"
#endif

/*----------------------------------------------------------------------------
--		Declarations
----------------------------------------------------------------------------*/

/**
**		Structure of pushed clippings.
*/
typedef struct _clip_ {
	struct _clip_*		Next;				/// next pushed clipping.
	int						X1;				/// pushed clipping top left
	int						Y1;				/// pushed clipping top left
	int						X2;				/// pushed clipping bottom right
	int						Y2;				/// pushed clipping bottom right
} Clip;

/*----------------------------------------------------------------------------
--		Externals
----------------------------------------------------------------------------*/

extern void InitVideoSdl(void);				/// Init SDL video hardware driver

extern void SdlLockScreen(void);		/// Do SDL hardware lock
extern void SdlUnlockScreen(void);		/// Do SDL hardware unlock

/*----------------------------------------------------------------------------
--		Variables
----------------------------------------------------------------------------*/

global char VideoFullScreen;				/// true fullscreen wanted

global int ColorCycleAll;				/// Flag Color Cycle with all palettes

global int ClipX1;						/// current clipping top left
global int ClipY1;						/// current clipping top left
global int ClipX2;						/// current clipping bottom right
global int ClipY2;						/// current clipping bottom right

local Clip* Clips;						/// stack of all clips
local Clip* ClipsGarbage;				/// garbage-list of available clips

#ifdef DEBUG
global unsigned AllocatedGraphicMemory; /// Allocated memory for objects
global unsigned CompressedGraphicMemory;/// memory for compressed objects
#endif

	/**
	**		Architecture-dependant video depth. Set by InitVideoXXX, if 0.
	**		(8,15,16,24,32)
	**		@see InitVideo @see InitVideoSdl
	**		@see main
	*/
global int VideoDepth;

	/**
	**		Architecture-dependant video bpp (bits pro pixel).
	**		Set by InitVideoXXX. (8,16,24,32)
	**		@see InitVideo @see InitVideoSdl
	**		@see main
	*/
global int VideoBpp;

	/**
	**  Architecture-dependant video memory-size (byte pro pixel).
	**  Set by InitVideo. (1,2,3,4 equals VideoBpp/8)
	**  @see InitVideo
	*/
global int VideoTypeSize;

	/**
	**		Architecture-dependant videomemory. Set by InitVideoXXX.
	**		FIXME: need a new function to set it, see #ifdef SDL code
	**		@see InitVideo @see InitVideoSdl
	**		@see VMemType
	*/
#ifdef USE_SDL_SURFACE
global SDL_Surface* TheScreen;
#else
global VMemType* VideoMemory;
#endif

global int VideoSyncSpeed = 100;		/// 0 disable interrupts
global volatile int VideoInterrupts;		/// be happy, were are quicker

	/**
	** FIXME: this docu is added only to the first variable!
	**
	**  As a video 8bpp pixel color doesn't have RGB encoded, but denote some
	**  index (a value from contents Pixels) in a system pallete, the
	**  following precalculated arrays deliver a shortcut.
	**  NOTE: all array pointers are NULL in a non 8bpp mode
	**
	**  lookup25trans8:
	**  Array to get from two system colors as unsigned int (color1<<8)|color2
	**  to a new system color which is aproximately 75% color1 and 25% color2.
	**  lookup50trans8:
	**  The same for 50% color1 and 50% color2.
	*/
#ifdef USE_SDL_SURFACE
	// FIXME: docu
global SDL_Color* lookup25trans8;
	// FIXME: docu
global SDL_Color* lookup50trans8;
#else
	// FIXME: docu
global VMemType8* lookup25trans8;
	// FIXME: docu
global VMemType8* lookup50trans8;
#endif

global int ColorWaterCycleStart;
global int ColorWaterCycleEnd;
global int ColorIconCycleStart;
global int ColorIconCycleEnd;
global int ColorBuildingCycleStart;
global int ColorBuildingCycleEnd;

	/// Does ColorCycling..
#ifdef USE_SDL_SURFACE
global void ColorCycle(void);

Uint32 ColorBlack;
Uint32 ColorDarkGreen;
Uint32 ColorBlue;
Uint32 ColorOrange;
Uint32 ColorWhite;
Uint32 ColorGray;
Uint32 ColorRed;
Uint32 ColorGreen;
Uint32 ColorYellow;
#else
global void (*ColorCycle)(void);

VMemType ColorBlack;
VMemType ColorDarkGreen;
VMemType ColorBlue;
VMemType ColorOrange;
VMemType ColorWhite;
VMemType ColorNPC;
VMemType ColorGray;
VMemType ColorRed;
VMemType ColorGreen;
VMemType ColorYellow;
#endif


/*----------------------------------------------------------------------------
--		Functions
----------------------------------------------------------------------------*/

/**
**		Clip Rectangle to another rectangle
**
**		@param left		Left X original rectangle coordinate.
**		@param top		Top Y original rectangle coordinate.
**		@param right		Right X original rectangle coordinate.
**		@param bottom		Bottom Y original rectangle coordinate.
**		@param x1		Left X bounding rectangle coordinate.
**		@param y1		Top Y bounding rectangle coordinate.
**		@param x2		Right X bounding rectangle coordinate.
**		@param y2		Bottom Y bounding rectangle coordinate.
**/
global void ClipRectToRect(int* left, int* top, int* right,int* bottom,
		int x1, int y1, int x2, int y2)
{
	// Swap the coordinates, if the order is wrong
	if (*left > *right) {
		*left ^= *right;
		*right ^= *left;
		*left ^= *right;
	}
	if (*top > *bottom) {
		*top ^= *bottom;
		*bottom ^= *top;
		*top ^= *bottom;
	}

	// Limit to bounding rectangle
	if (*left < x1) {
		*left = x1;
	} else if (*left >= x2) {
		*left = x2 - 1;
	}
	if (*top < y1) {
		*top = y1;
	} else if (*top >= y2) {
		*top = y2 - 1;
	}
	if (*right < x1) {
		*right = x1;
	} else if (*right >= x2) {
		*right = x2 - 1;
	}
	if (*bottom < y1) {
		*bottom = y1;
	} else if (*bottom >= y2) {
		*bottom = y2 - 1;
	}
}

/**
**		Set clipping for graphic routines.
**
**		@param left		Left X screen coordinate.
**		@param top		Top Y screen coordinate.
**		@param right		Right X screen coordinate.
**		@param bottom		Bottom Y screen coordinate.
*/
global void SetClipping(int left, int top, int right, int bottom)
{
#ifdef DEBUG
	if (left > right || top > bottom || left < 0 || left >= VideoWidth ||
			top < 0 || top >= VideoHeight || right < 0 ||
			right >= VideoWidth || bottom < 0 || bottom >= VideoHeight) {
		DebugLevel0Fn("Wrong clipping %d->%d %d->%d, write cleaner code.\n" _C_
				left _C_ right _C_ top _C_ bottom);
//		DebugCheck(1);
	}
#endif
	ClipRectToRect(&left, &top, &right, &bottom, 0, 0, VideoWidth, VideoHeight);

	ClipX1 = left;
	ClipY1 = top;
	ClipX2 = right;
	ClipY2 = bottom;
}

/**
**		Set clipping for graphic routines. This clips against the current clipping.
**
**		@param left		Left X screen coordinate.
**		@param top		Top Y screen coordinate.
**		@param right		Right X screen coordinate.
**		@param bottom		Bottom Y screen coordinate.
*/
global void SetClipToClip(int left, int top, int right, int bottom)
{
	//  No warnings... exceeding is expected.
	ClipRectToRect(&left, &top, &right, &bottom, ClipX1, ClipY1, ClipX2, ClipY2);

	ClipX1 = left;
	ClipY1 = top;
	ClipX2 = right;
	ClipY2 = bottom;
}

/**
**		Push current clipping.
*/
global void PushClipping(void)
{
	Clip* clip;

	if ((clip = ClipsGarbage)) {
		ClipsGarbage = ClipsGarbage->Next;
	} else {
		clip = malloc(sizeof(Clip));
	}

	clip->Next = Clips;
	clip->X1 = ClipX1;
	clip->Y1 = ClipY1;
	clip->X2 = ClipX2;
	clip->Y2 = ClipY2;
	Clips = clip;
}

/**
**		Pop current clipping.
*/
global void PopClipping(void)
{
	Clip* clip;

	clip = Clips;
	if (clip) {
		Clips = clip->Next;
		ClipX1 = clip->X1;
		ClipY1 = clip->Y1;
		ClipX2 = clip->X2;
		ClipY2 = clip->Y2;

		clip->Next = ClipsGarbage;
		ClipsGarbage = clip;
	} else {
		ClipX1 = 0;
		ClipY1 = 0;
		ClipX2 = VideoWidth;
		ClipY2 = VideoHeight;
	}
}

/**
**		Creates a checksum used to compare palettes.
**
**		@param palette		Palette source.
**
**		@return				Calculated hash/checksum.
*/
#ifndef USE_SDL_SURFACE
local long GetPaletteChecksum(const Palette* palette)
{
	long retVal;
	int i;

	for (retVal = i = 0; i < 256; ++i){
		//This is designed to return different values if
		// the pixels are in a different order.
		retVal = ((palette[i].r + i) & 0xff) + retVal;
		retVal = ((palette[i].g + i) & 0xff) + retVal;
		retVal = ((palette[i].b + i) & 0xff) + retVal;
	}
	return retVal;
}
#endif

/**
**  FIXME: docu
*/
#ifdef USE_SDL_SURFACE
global void VideoPaletteListAdd(SDL_Surface* surface)
{
	PaletteLink* curlink;

	curlink = malloc(sizeof(PaletteLink));

	curlink->Surface = surface;
	curlink->Next = PaletteList;

	PaletteList = curlink;
}

/**
**  FIXME: docu
*/
global void VideoPaletteListRemove(SDL_Surface* surface)
{
	PaletteLink** curlink;
	PaletteLink* tmp;

	curlink = &PaletteList;
	while (*curlink) {
		if ((*curlink)->Surface == surface) {
			break;
		}
		curlink = &((*curlink)->Next);
	}
	DebugCheck(!*curlink);
	if (*curlink == PaletteList) {
		tmp = PaletteList->Next;
		free(PaletteList);
		PaletteList = tmp;
	} else {
		tmp = *curlink;
		*curlink = tmp->Next;
		free(tmp);
	}
}
#else
global VMemType* VideoCreateSharedPalette(const Palette* palette)
{
	PaletteLink* current_link;
	PaletteLink** prev_link;
	VMemType* pixels;
	long checksum;

	pixels = VideoCreateNewPalette(palette);
	checksum = GetPaletteChecksum(palette);
	prev_link = &PaletteList;
	while ((current_link = *prev_link)) {
		if (current_link->Checksum == checksum &&
				!memcmp(current_link->Palette, pixels,
					256 * ((VideoDepth + 7) / 8))) {
			break;
		}
		prev_link = &current_link->Next;
	}

	if (current_link) {						// Palette found (reuse)
		free(pixels);
		pixels = current_link->Palette;
		++current_link->RefCount;
		DebugLevel3("uses palette %p %d\n" _C_ pixels _C_ current_link->RefCount);
	} else {								// Palette NOT found
		DebugLevel3("loading new palette %p\n" _C_ pixels);

		current_link = *prev_link = malloc(sizeof(PaletteLink));
		current_link->Checksum = checksum;
		current_link->Next = NULL;
		current_link->Palette = pixels;
		current_link->RefCount = 1;
	}

	return pixels;
}

/**
**		Free a shared hardware palette.
**
**		@param pixels		palette in hardware dependend format
*/
global void VideoFreeSharedPalette(VMemType* pixels)
{
	PaletteLink* current_link;
	PaletteLink** prev_link;

//	if (pixels == Pixels) {
//		DebugLevel3Fn("Freeing global palette\n");
//	}

	//
	//  Find and free palette.
	//
	prev_link = &PaletteList;
	while ((current_link = *prev_link)) {
		if (current_link->Palette == pixels) {
			break;
		}
		prev_link = &current_link->Next;
	}
	if (current_link) {
		if (!--current_link->RefCount) {
			DebugLevel3Fn("Free palette %p\n" _C_ pixels);
			free(current_link->Palette);
			*prev_link = current_link->Next;
			free(current_link);
		}
	}
}
#endif

/**
**		Load a picture and display it on the screen (full screen),
**		changing the colormap and so on..
**
**		@param name		Name of the picture (file) to display.
*/
global void DisplayPicture(const char* name)
{
	Graphic* picture;

	picture = LoadGraphic(name);
	ResizeGraphic(picture, VideoWidth, VideoHeight);
#ifdef USE_OPENGL
	MakeTexture(picture, picture->Width, picture->Height);
#endif

#ifdef USE_SDL_SURFACE
	// Unset the alpha color key, not needed
	SDL_SetColorKey(picture->Surface, 0, 0);
	SDL_BlitSurface(picture->Surface, NULL, TheScreen, NULL);
#else
	VideoLockScreen();
	VideoDrawSubClip(picture, 0, 0, picture->Width, picture->Height,
		(VideoWidth - picture->Width) / 2, (VideoHeight - picture->Height) / 2);
	VideoUnlockScreen();
#endif

	VideoFree(picture);
}

// FIXME: this isn't 100% correct
// Color cycling info - forest:
// 3		flash red/green (attacked building on minimap)
// 38-47		cycle				(water)
// 48-56		cycle				(water-coast boundary)
// 202		pulsates red		(Circle of Power)
// 240-244		cycle				(water around ships, Runestone, Dark Portal)
// Color cycling info - swamp:
// 3		flash red/green (attacked building on minimap)
// 4		pulsates red		(Circle of Power)
// 5-9		cycle				(Runestone, Dark Portal)
// 38-47		cycle				(water)
// 88-95		cycle				(waterholes in coast and ground)
// 240-244		cycle				(water around ships)
// Color cycling info - wasteland:
// 3		flash red/green (attacked building on minimap)
// 38-47		cycle				(water)
// 64-70		cycle				(coast)
// 202		pulsates red		(Circle of Power)
// 240-244		cycle				(water around ships, Runestone, Dark Portal)
// Color cycling info - winter:
// 3		flash red/green (attacked building on minimap)
// 40-47		cycle				(water)
// 48-54		cycle				(half-sunken ice-floe)
// 202		pulsates red		(Circle of Power)
// 205-207		cycle				(lights on christmas tree)
// 240-244		cycle				(water around ships, Runestone, Dark Portal)

#ifdef USE_SDL_SURFACE
/**
**  Color cycle.
**
**  FIXME: Also icons and some units use color cycling.
*/
// FIXME: cpu intensive to go through the whole PaletteList
global void ColorCycle(void)
{
	SDL_Color* palcolors;
	SDL_Color colors[256];
	int waterlen;
	int iconlen;
	int buildinglen;

	waterlen = (ColorWaterCycleEnd - ColorWaterCycleStart) * sizeof(SDL_Color);
	iconlen = (ColorIconCycleEnd - ColorIconCycleStart) * sizeof(SDL_Color);
	buildinglen = (ColorBuildingCycleEnd - ColorBuildingCycleStart) * sizeof(SDL_Color);

	if (ColorCycleAll) {
		PaletteLink* curlink;

		curlink = PaletteList;
		while (curlink != NULL) {
			palcolors = curlink->Surface->format->palette->colors;

			memcpy(colors, palcolors, sizeof(colors));

			memcpy(colors + ColorWaterCycleStart,
				palcolors + ColorWaterCycleStart + 1, waterlen);
			colors[ColorWaterCycleEnd] = palcolors[ColorWaterCycleStart];

			memcpy(colors + ColorIconCycleStart,
				palcolors + ColorIconCycleStart + 1, iconlen);
			colors[ColorIconCycleEnd] = palcolors[ColorIconCycleStart];

			memcpy(colors + ColorBuildingCycleStart,
				palcolors + ColorBuildingCycleStart + 1, buildinglen);
			colors[ColorBuildingCycleEnd] = palcolors[ColorBuildingCycleStart];

			SDL_SetPalette(curlink->Surface, SDL_LOGPAL | SDL_PHYSPAL,
				colors, 0, 256);
			curlink = curlink->Next;
		}
	} else {
		//
		//  Color cycle tileset palette
		//
		palcolors = TheMap.TileGraphic->Surface->format->palette->colors;

		memcpy(colors, palcolors, sizeof(colors));

		memcpy(colors + ColorWaterCycleStart,
			palcolors + ColorWaterCycleStart + 1, waterlen);
		colors[ColorWaterCycleEnd] = palcolors[ColorWaterCycleStart];

		memcpy(colors + ColorIconCycleStart,
			palcolors + ColorIconCycleStart + 1, iconlen);
		colors[ColorIconCycleEnd] = palcolors[ColorIconCycleStart];

		memcpy(colors + ColorBuildingCycleStart,
			palcolors + ColorBuildingCycleStart + 1, buildinglen);
		colors[ColorBuildingCycleEnd] = palcolors[ColorBuildingCycleStart];

		SDL_SetPalette(TheMap.TileGraphic->Surface, SDL_LOGPAL | SDL_PHYSPAL,
			colors, 0, 256);
	}

	MustRedraw |= RedrawColorCycle;
}
#else
local void ColorCycle8(void)
{
	int i;
	int x;
	VMemType8* pixels;

	if (ColorCycleAll) {
		PaletteLink* current_link;

		current_link = PaletteList;
		while (current_link != NULL) {
			x = ((VMemType8*)(current_link->Palette))[ColorWaterCycleStart];
			for (i = ColorWaterCycleStart; i < ColorWaterCycleEnd; ++i) {		// tileset color cycle
				((VMemType8*)(current_link->Palette))[i] =
					((VMemType8*)(current_link->Palette))[i + 1];
			}
			((VMemType8*)(current_link->Palette))[ColorWaterCycleEnd] = x;

			x = ((VMemType8*)(current_link->Palette))[ColorIconCycleStart];
			for (i = ColorIconCycleStart; i < ColorIconCycleEnd; ++i) {		// units/icons color cycle
				((VMemType8*)(current_link->Palette))[i] =
					((VMemType8*)(current_link->Palette))[i + 1];
			}
			((VMemType8*) (current_link->Palette))[ColorIconCycleEnd] = x;

			x = ((VMemType8*)(current_link->Palette))[ColorBuildingCycleStart];
			for (i = ColorBuildingCycleStart; i < ColorBuildingCycleEnd; ++i) {		// buildings color cycle
				((VMemType8*)(current_link->Palette))[i] =
					((VMemType8*)(current_link->Palette))[i + 1];
			}
			((VMemType8*)(current_link->Palette))[ColorBuildingCycleEnd] = x;

			current_link = current_link->Next;
		}
	} else {

		//
		//		Color cycle tileset palette
		//
		pixels = TheMap.TileData->Pixels;
		x = pixels[ColorWaterCycleStart];
		for (i = ColorWaterCycleStart; i < ColorWaterCycleEnd; ++i) {
			pixels[i] = pixels[i + 1];
		}
		pixels[ColorWaterCycleEnd] = x;
	}

	MapColorCycle();				// FIXME: could be little more informative
	MustRedraw |= RedrawColorCycle;
}

/**
**		Color cycle for 16 bpp video mode.
**
**		FIXME: Also icons and some units use color cycling.
**		FIXME: must be configured by the tileset or global.
*/
local void ColorCycle16(void)
{
	int i;
	int x;
	VMemType16* pixels;

	if (ColorCycleAll) {
		PaletteLink* current_link;

		current_link = PaletteList;
		while (current_link != NULL) {
			x = ((VMemType16*)(current_link->Palette))[ColorWaterCycleStart];
			for (i = ColorWaterCycleStart; i < ColorWaterCycleEnd; ++i) {		// tileset color cycle
				((VMemType16*)(current_link->Palette))[i] =
					((VMemType16*)(current_link->Palette))[i + 1];
			}
			((VMemType16*)(current_link->Palette))[ColorWaterCycleEnd] = x;

			x = ((VMemType16*)(current_link->Palette))[ColorIconCycleStart];
			for (i = ColorIconCycleStart; i < ColorIconCycleEnd; ++i) {		// units/icons color cycle
				((VMemType16*)(current_link->Palette))[i] =
					((VMemType16*)(current_link->Palette))[i + 1];
			}
			((VMemType16*)(current_link->Palette))[ColorIconCycleEnd] = x;

			x = ((VMemType16*)(current_link->Palette))[ColorBuildingCycleStart];
			for (i = ColorBuildingCycleStart; i < ColorBuildingCycleEnd; ++i) {		// Buildings color cycle
				((VMemType16*)(current_link->Palette))[i] =
					((VMemType16*)(current_link->Palette))[i + 1];
			}
			((VMemType16*)(current_link->Palette))[ColorBuildingCycleEnd] = x;

			current_link = current_link->Next;
		}
	} else {

		//
		//		Color cycle tileset palette
		//
		pixels = TheMap.TileData->Pixels;
		x = pixels[ColorWaterCycleStart];
		for (i = ColorWaterCycleStart; i < ColorWaterCycleEnd; ++i) {
			pixels[i] = pixels[i + 1];
		}
		pixels[ColorWaterCycleEnd] = x;
	}

	MapColorCycle();				// FIXME: could be little more informative
	MustRedraw |= RedrawColorCycle;
}

/**
**		Color cycle for 24 bpp video mode.
**
**		FIXME: Also icons and some units use color cycling.
**		FIXME: must be configured by the tileset or global.
*/
local void ColorCycle24(void)
{
	int i;
	VMemType24 x;
	VMemType24* pixels;

	if (ColorCycleAll) {
		PaletteLink* current_link;

		current_link = PaletteList;
		while (current_link != NULL) {
			x = ((VMemType24*)(current_link->Palette))[ColorWaterCycleStart];
			for (i = ColorWaterCycleStart; i < ColorWaterCycleEnd; ++i) {		// tileset color cycle
				((VMemType24*)(current_link->Palette))[i] =
					((VMemType24*)(current_link->Palette))[i + 1];
			}
			((VMemType24*)(current_link->Palette))[ColorWaterCycleEnd] = x;

			x = ((VMemType24*)(current_link->Palette))[ColorIconCycleStart];
			for (i = ColorIconCycleStart; i < ColorIconCycleEnd; ++i) {		// units/icons color cycle
				((VMemType24*)(current_link->Palette))[i] =
					((VMemType24*)(current_link->Palette))[i + 1];
			}
			((VMemType24*)(current_link->Palette))[ColorIconCycleEnd] = x;

			x = ((VMemType24*)(current_link->Palette))[ColorBuildingCycleStart];
			for (i = ColorBuildingCycleStart; i < ColorBuildingCycleEnd; ++i) {		// Buildings color cycle
				((VMemType24*)(current_link->Palette))[i] =
					((VMemType24*)(current_link->Palette))[i + 1];
			}
			((VMemType24*)(current_link->Palette))[ColorBuildingCycleEnd] = x;

			current_link = current_link->Next;
		}
	} else {

		//
		//		Color cycle tileset palette
		//
		pixels = TheMap.TileData->Pixels;
		x = pixels[ColorWaterCycleStart];
		for (i = ColorWaterCycleStart; i < ColorWaterCycleEnd; ++i) {
			pixels[i] = pixels[i + 1];
		}
		pixels[ColorWaterCycleEnd] = x;
	}

	MapColorCycle();				// FIXME: could be little more informative
	MustRedraw |= RedrawColorCycle;
}

/**
**		Color cycle for 32 bpp video mode.
**
**		FIXME: Also icons and some units use color cycling.
**		FIXME: must be configured by the tileset or global.
*/
local void ColorCycle32(void)
{
	int i;
	int x;
	VMemType32* pixels;

	if (ColorCycleAll) {
		PaletteLink* current_link;

		current_link = PaletteList;
		while (current_link != NULL) {
			x = ((VMemType32*)(current_link->Palette))[ColorWaterCycleStart];
			for (i = ColorWaterCycleStart; i < ColorWaterCycleEnd; ++i) { // tileset color cycle
				((VMemType32*)(current_link->Palette))[i] =
					((VMemType32*)(current_link->Palette))[i + 1];
			}
			((VMemType32*)(current_link->Palette))[ColorWaterCycleEnd] = x;

			x = ((VMemType32*)(current_link->Palette))[ColorIconCycleStart];
			for (i = ColorIconCycleStart; i < ColorIconCycleEnd; ++i) {		// units/icons color cycle
				((VMemType32*)(current_link->Palette))[i] =
					((VMemType32*)(current_link->Palette))[i + 1];
			}
			((VMemType32*)(current_link->Palette))[ColorIconCycleEnd] = x;

			x = ((VMemType32*)(current_link->Palette))[ColorBuildingCycleStart];
			for (i = ColorBuildingCycleStart; i < ColorBuildingCycleEnd; ++i) {		// Buildings color cycle
				((VMemType32*)(current_link->Palette))[i] =
					((VMemType32*)(current_link->Palette))[i + 1];
			}
			((VMemType32*)(current_link->Palette))[ColorBuildingCycleEnd] = x;

			current_link = current_link->Next;
		}
	} else {

		//
		//		  Color cycle tileset palette
		//
		pixels = TheMap.TileData->Pixels;
		x = pixels[ColorWaterCycleStart];
		for (i = ColorWaterCycleStart; i < ColorWaterCycleEnd; ++i) {
			pixels[i] = pixels[i + 1];
		}
		pixels[ColorWaterCycleEnd] = x;
	}

	MapColorCycle();				// FIXME: could be little more informative
	MustRedraw |= RedrawColorCycle;
}
#endif

/*----------------------------------------------------------------------------
--		Functions
----------------------------------------------------------------------------*/

/**
**		Lock the screen for write access.
*/
global void VideoLockScreen(void)
{
#ifdef USE_SDL
	SdlLockScreen();
#endif
}

/**
**		Unlock the screen for write access.
*/
global void VideoUnlockScreen(void)
{
#ifdef USE_SDL
	SdlUnlockScreen();
#endif
}

/**
**		Clear the video screen.
*/
global void VideoClearScreen(void)
{
	VideoFillRectangle(ColorBlack, 0, 0, VideoWidth, VideoHeight);
}

/**
**		Return ticks in ms since start.
*/
global unsigned long GetTicks(void)
{
#ifdef USE_SDL
	return SDL_GetTicks();
#endif
}

/**
**		Video initialize.
*/
global void InitVideo(void)
{
#ifdef USE_SDL
	InitVideoSdl();
#endif

	//
	//		General (video) modules and settings
	//
#ifdef USE_SDL_SURFACE
#else
	switch (VideoBpp) {
		case  8: ColorCycle = ColorCycle8; break;
		case 15:
		case 16: ColorCycle = ColorCycle16; break;
		case 24: ColorCycle = ColorCycle24; break;
		case 32: ColorCycle = ColorCycle32; break;
		default: DebugLevel0Fn("Video %d bpp unsupported\n" _C_ VideoBpp);
	}
	VideoTypeSize = VideoBpp / 8;
#endif

	//
	//		Init video sub modules
	//
	InitGraphic();
	InitLineDraw();
	InitSprite();

#ifdef NEW_DECODRAW
// Use the decoration mechanism to only redraw what is needed on screen update
	DecorationInit();
#endif
}

//@}
