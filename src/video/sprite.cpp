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
/**@name sprite.c	-	The general sprite functions. */
//
//	(c) Copyright 2000-2002 by Lutz Sammer, Stephan Rasenberg
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
#include <string.h>

#include "freecraft.h"
#include "video.h"

#include "intern_video.h"

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

local GraphicType GraphicSprite8Type;	/// sprite type 8bit palette
local GraphicType GraphicSprite16Type;	/// sprite type 16bit palette

global void (*VideoDrawRawClip)( VMemType *pixels,
                                 const unsigned char *data,
                                 unsigned int x, unsigned int y,
                                 unsigned int w, unsigned int h );

/*----------------------------------------------------------------------------
--	Local functions
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	RLE Sprites
----------------------------------------------------------------------------*/

//
//	The current implementation uses RLE encoded sprites.
//	If you know something better, write it.
//
//	The encoding translates the sprite data to a stream of segments
//	of the form:
//
//	<skip> <run> <data>
//
//	where	<skip> is the number of transparent pixels to skip,
//		<run>  is the number of opaque pixels to blit,
//	 and	<data> are the pixels themselves.
//
//	<skip> and <run> are unsigned 8 bit integers.
//	If more than 255 transparent pixels are needed 255 0 <n> <run> is
//	used. <run> is always stored, even 0 at the end of line.
//	This makes the pixel data aligned at all times.
//	Segments never wrap around from one scan line to the next.
//

/**
**	Draw a RLE encoded graphic object unclipped into framebuffer.
**
**	@NOTE: This macro looks nice, but is absolutly no debugable.
**	@TODO: Make this an inline function.
**
**	@param bpp	Bit depth of target framebuffer
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
#define RLE_BLIT(bpp,sprite,frame,x,y)	\
    do {							\
	const unsigned char* sp;				\
	unsigned w;						\
	VMemType##bpp* dp;					\
	const VMemType##bpp* lp;				\
	const VMemType##bpp* ep;				\
	const VMemType##bpp* pixels;				\
	const VMemType##bpp* pp;				\
	unsigned da;						\
								\
	pixels=(VMemType##bpp*)sprite->Pixels;		        \
	sp=((unsigned char**)sprite->Frames)[frame];		\
	w=sprite->Width;					\
	da=VideoWidth-w;					\
	dp=VideoMemory##bpp+x+y*VideoWidth;			\
	ep=dp+VideoWidth*sprite->Height;			\
								\
	do {							\
	    lp=dp+w;						\
	    do {			/* 1 line */		\
		dp+=*sp++;		/* transparent # */	\
		pp=dp-1+*sp++;		/* opaque # */		\
		while( dp<pp ) {	/* unrolled */		\
		    *dp++=pixels[*sp++];			\
		    *dp++=pixels[*sp++];			\
		}						\
		if( dp<=pp ) {					\
		    *dp++=pixels[*sp++];			\
		}						\
	    } while( dp<lp );					\
	    dp+=da;						\
	} while( dp<ep );		/* all lines */		\
    } while( 0 )

/**
**	Draw 8bit graphic object unclipped into 8 bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to8(const Graphic* sprite,unsigned frame,int x,int y)
{
    RLE_BLIT(8,sprite,frame,x,y);
}

/**
**	Draw 8bit graphic object unclipped into 16 bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to16(const Graphic* sprite,unsigned frame,int x,int y)
{
    RLE_BLIT(16,sprite,frame,x,y);
}

/**
**	Draw 8bit graphic object unclipped into 24 bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to24(const Graphic* sprite,unsigned frame,int x,int y)
{
    RLE_BLIT(24,sprite,frame,x,y);
}

/**
**	Draw 8bit graphic object unclipped into 32 bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to32(const Graphic* sprite,unsigned frame,int x,int y)
{
    RLE_BLIT(32,sprite,frame,x,y);
}

/**
**	Draw 8bit graphic object unclipped and flipped in X direction
**	into 8 bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to8X(const Graphic* sprite,unsigned frame,int x,int y)
{
    const unsigned char* sp;
    unsigned w;
    VMemType8* dp;
    const VMemType8* lp;
    const VMemType8* ep;
    const VMemType8* pp;
    const VMemType8* pixels;
    unsigned da;

    pixels=(VMemType8*)sprite->Pixels;
    sp=((unsigned char**)sprite->Frames)[frame];
    w=sprite->Width;
    dp=VideoMemory8+x+y*VideoWidth+w-1;
    da=VideoWidth+w;
    ep=dp+VideoWidth*sprite->Height;
    do {
	lp=dp-w;
	do {				// 1 line
	    dp-=*sp++;			// transparent
	    pp=dp+1-*sp++;		// opaque
	    while( dp>pp ) {		// unrolled
		*dp--=pixels[*sp++];
		*dp--=pixels[*sp++];
	    }
	    if( dp>=pp ) {
		*dp--=pixels[*sp++];
	    }
	} while( dp>lp );
	dp+=da;
    } while( dp<ep );			// all lines
}

/**
**	Draw 8bit graphic object unclipped and flipped in X direction
**	into 16 bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to16X(const Graphic* sprite,unsigned frame,int x,int y)
{
    const unsigned char* sp;
    unsigned w;
    VMemType16* dp;
    const VMemType16* lp;
    const VMemType16* ep;
    const VMemType16* pp;
    const VMemType16* pixels;
    unsigned da;

    pixels=(VMemType16*)sprite->Pixels;
    sp=((unsigned char**)sprite->Frames)[frame];
    w=sprite->Width;
    dp=VideoMemory16+x+y*VideoWidth+w-1;
    da=VideoWidth+w;
    ep=dp+VideoWidth*sprite->Height;

    do {
	lp=dp-w;
	do {				// 1 line
	    dp-=*sp++;			// transparent
	    pp=dp+1-*sp++;		// opaque
	    while( dp>pp ) {		// unrolled
		*dp--=pixels[*sp++];
		*dp--=pixels[*sp++];
	    }
	    if( dp>=pp ) {
		*dp--=pixels[*sp++];
	    }
	} while( dp>lp );
	dp+=da;
    } while( dp<ep );			// all lines
}

/**
**	Draw 8bit graphic object unclipped and flipped in X direction
**	into 24 bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to24X(const Graphic* sprite,unsigned frame,int x,int y)
{
    const unsigned char* sp;
    unsigned w;
    VMemType24* dp;
    const VMemType24* lp;
    const VMemType24* ep;
    const VMemType24* pp;
    const VMemType24* pixels;
    unsigned da;

    pixels=(VMemType24*)sprite->Pixels;
    sp=((unsigned char**)sprite->Frames)[frame];
    w=sprite->Width;
    dp=VideoMemory24+x+y*VideoWidth+w-1;
    da=VideoWidth+w;
    ep=dp+VideoWidth*sprite->Height;

    do {
	lp=dp-w;
	do {				// 1 line
	    dp-=*sp++;			// transparent
	    pp=dp+1-*sp++;		// opaque
	    while( dp>pp ) {		// unrolled
		*dp--=pixels[*sp++];
		*dp--=pixels[*sp++];
	    }
	    if( dp>=pp ) {
		*dp--=pixels[*sp++];
	    }
	} while( dp>lp );
	dp+=da;
    } while( dp<ep );			// all lines
}

/**
**	Draw 8bit graphic object unclipped and flipped in X direction
**	into 32 bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to32X(const Graphic* sprite,unsigned frame,int x,int y)
{
    const unsigned char* sp;
    unsigned w;
    VMemType32* dp;
    const VMemType32* lp;
    const VMemType32* ep;
    const VMemType32* pp;
    const VMemType32* pixels;
    unsigned da;

    pixels=(VMemType32*)sprite->Pixels;
    sp=((unsigned char**)sprite->Frames)[frame];
    w=sprite->Width;
    dp=VideoMemory32+x+y*VideoWidth+w-1;
    da=VideoWidth+w;
    ep=dp+VideoWidth*sprite->Height;

    do {
	lp=dp-w;
	do {				// 1 line
	    dp-=*sp++;			// transparent
	    pp=dp+1-*sp++;		// opaque
	    while( dp>pp ) {		// unrolled
		*dp--=pixels[*sp++];
		*dp--=pixels[*sp++];
	    }
	    if( dp>=pp ) {
		*dp--=pixels[*sp++];
	    }
	} while( dp>lp );
	dp+=da;
    } while( dp<ep );			// all lines
}

/**
**	Draw 8bit graphic object clipped into 8 bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to8Clip(const Graphic* sprite,unsigned frame,int x,int y)
{
    int ox;
    int ex;
    int oy;
    int w;
    int h;
    const unsigned char* sp;
    unsigned sw;
    VMemType8* dp;
    const VMemType8* lp;
    const VMemType8* ep;
    VMemType8* pp;
    const VMemType8* pixels;
    unsigned da;


    //
    // reduce to visible range
    //
    sw=w=sprite->Width;
    h=sprite->Height;
    CLIP_RECTANGLE_OFS(x,y,w,h,ox,oy,ex);

    //
    //	Draw the clipped sprite
    //
    pixels=(VMemType8*)sprite->Pixels;
    sp=((unsigned char**)sprite->Frames)[frame];

    //
    //	Skip top lines, if needed.
    //
    while( oy-- ) {
	da=0;
	do {
	    da+=*sp++;			// transparent
	    da+=*sp;			// opaque
	    sp+=*sp+1;
	} while( da<sw );
    }

    da=VideoWidth-sw;
    dp=VideoMemory8+x+y*VideoWidth;
    ep=dp+VideoWidth*h;

    if( w==sw ) {			// Unclipped horizontal

	do {
	    lp=dp+sw;
	    do {			// 1 line
		dp+=*sp++;		// transparent
		pp=dp-1+*sp++;		// opaque
		while( dp<pp ) {	// unroll
		    *dp++=pixels[*sp++];
		    *dp++=pixels[*sp++];
		}
		if( dp<=pp ) {
		    *dp++=pixels[*sp++];
		}
	    } while( dp<lp );
	    dp+=da;
	} while( dp<ep );		// all lines

    } else {				// Clip horizontal

	da+=ox;
	do {
	    lp=dp+w;
	    //
	    //	Clip left
	    //
	    pp=dp-ox;
	    for( ;; ) {
		pp+=*sp++;		// transparent
		if( pp>=dp ) {
		    dp=pp;
		    goto middle_trans;
		}
		pp+=*sp;		// opaque
		if( pp>=dp ) {
		    sp+=*sp-(pp-dp)+1;
		    goto middle_pixel;
		}
		sp+=*sp+1;
	    }

	    //
	    //	Draw middle
	    //
	    for( ;; ) {
		dp+=*sp++;		// transparent
middle_trans:
		if( dp>=lp ) {
		    lp+=sw-w-ox;
		    goto right_trans;
		}
		pp=dp+*sp++;		// opaque
middle_pixel:
		if( pp<lp ) {
		    while( dp<pp ) {
			*dp++=pixels[*sp++];
		    }
		    continue;
		}
		while( dp<lp ) {
		    *dp++=pixels[*sp++];
		}
		sp+=pp-dp;
		dp=pp;
		break;
	    }

	    //
	    //	Clip right
	    //
	    lp+=sw-w-ox;
	    while( dp<lp ) {
		dp+=*sp++;		// transparent
right_trans:
		dp+=*sp;		// opaque
		sp+=*sp+1;
	    }
	    dp+=da;
	} while( dp<ep );		// all lines

    }
}

/**
**	Draw 8bit graphic object clipped into 16 bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to16Clip(const Graphic* sprite,unsigned frame,int x,int y)
{
    int ox;
    int ex;
    int oy;
    int w;
    int h;
    const unsigned char* sp;
    unsigned sw;
    VMemType16* dp;
    const VMemType16* lp;
    const VMemType16* ep;
    VMemType16* pp;
    const VMemType16* pixels;
    unsigned da;

    //
    // reduce to visible range
    //
    sw=w=sprite->Width;
    h=sprite->Height;
    CLIP_RECTANGLE_OFS(x,y,w,h,ox,oy,ex);

    //
    //	Draw the clipped sprite
    //
    pixels=(VMemType16*)sprite->Pixels;
    sp=((unsigned char**)sprite->Frames)[frame];

    //
    //	Skip top lines, if needed.
    //
    while( oy-- ) {
	da=0;
	do {
	    da+=*sp++;			// transparent
	    da+=*sp;			// opaque
	    sp+=*sp+1;
	} while( da<sw );
    }

    da=VideoWidth-sw;
    dp=VideoMemory16+x+y*VideoWidth;
    ep=dp+VideoWidth*h;

    if( w==sw ) {			// Unclipped horizontal

	do {
	    lp=dp+sw;
	    do {			// 1 line
		dp+=*sp++;		// transparent
		pp=dp-1+*sp++;		// opaque
		while( dp<pp ) {	// unroll
		    *dp++=pixels[*sp++];
		    *dp++=pixels[*sp++];
		}
		if( dp<=pp ) {
		    *dp++=pixels[*sp++];
		}
	    } while( dp<lp );
	    dp+=da;
	} while( dp<ep );		// all lines

    } else {				// Clip horizontal

	da+=ox;
	do {
	    lp=dp+w;
	    //
	    //	Clip left
	    //
	    pp=dp-ox;
	    for( ;; ) {
		pp+=*sp++;		// transparent
		if( pp>=dp ) {
		    dp=pp;
		    goto middle_trans;
		}
		pp+=*sp;		// opaque
		if( pp>=dp ) {
		    sp+=*sp-(pp-dp)+1;
		    goto middle_pixel;
		}
		sp+=*sp+1;
	    }

	    //
	    //	Draw middle
	    //
	    for( ;; ) {
		dp+=*sp++;		// transparent
middle_trans:
		if( dp>=lp ) {
		    lp+=sw-w-ox;
		    goto right_trans;
		}
		pp=dp+*sp++;		// opaque
middle_pixel:
		if( pp<lp ) {
		    while( dp<pp ) {
			*dp++=pixels[*sp++];
		    }
		    continue;
		}
		while( dp<lp ) {
		    *dp++=pixels[*sp++];
		}
		sp+=pp-dp;
		dp=pp;
		break;
	    }

	    //
	    //	Clip right
	    //
	    lp+=sw-w-ox;
	    while( dp<lp ) {
		dp+=*sp++;		// transparent
right_trans:
		dp+=*sp;		// opaque
		sp+=*sp+1;
	    }
	    dp+=da;
	} while( dp<ep );		// all lines

    }
}

/**
**	Draw 8bit graphic object clipped into 24 bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to24Clip(const Graphic* sprite,unsigned frame,int x,int y)
{
    int ox;
    int ex;
    int oy;
    int w;
    int h;
    const unsigned char* sp;
    unsigned sw;
    VMemType24* dp;
    const VMemType24* lp;
    const VMemType24* ep;
    VMemType24* pp;
    const VMemType24* pixels;
    unsigned da;

    //
    // reduce to visible range
    //
    sw=w=sprite->Width;
    h=sprite->Height;
    CLIP_RECTANGLE_OFS(x,y,w,h,ox,oy,ex);

    //
    //	Draw the clipped sprite
    //
    pixels=(VMemType24*)sprite->Pixels;
    sp=((unsigned char**)sprite->Frames)[frame];

    //
    //	Skip top lines, if needed.
    //
    while( oy-- ) {
	da=0;
	do {
	    da+=*sp++;			// transparent
	    da+=*sp;			// opaque
	    sp+=*sp+1;
	} while( da<sw );
    }

    da=VideoWidth-sw;
    dp=VideoMemory24+x+y*VideoWidth;
    ep=dp+VideoWidth*h;

    if( w==sw ) {			// Unclipped horizontal

	do {
	    lp=dp+sw;
	    do {			// 1 line
		dp+=*sp++;		// transparent
		pp=dp-1+*sp++;		// opaque
		while( dp<pp ) {	// unroll
		    *dp++=pixels[*sp++];
		    *dp++=pixels[*sp++];
		}
		if( dp<=pp ) {
		    *dp++=pixels[*sp++];
		}
	    } while( dp<lp );
	    dp+=da;
	} while( dp<ep );		// all lines

    } else {				// Clip horizontal

	da+=ox;
	do {
	    lp=dp+w;
	    //
	    //	Clip left
	    //
	    pp=dp-ox;
	    for( ;; ) {
		pp+=*sp++;		// transparent
		if( pp>=dp ) {
		    dp=pp;
		    goto middle_trans;
		}
		pp+=*sp;		// opaque
		if( pp>=dp ) {
		    sp+=*sp-(pp-dp)+1;
		    goto middle_pixel;
		}
		sp+=*sp+1;
	    }

	    //
	    //	Draw middle
	    //
	    for( ;; ) {
		dp+=*sp++;		// transparent
middle_trans:
		if( dp>=lp ) {
		    lp+=sw-w-ox;
		    goto right_trans;
		}
		pp=dp+*sp++;		// opaque
middle_pixel:
		if( pp<lp ) {
		    while( dp<pp ) {
			*dp++=pixels[*sp++];
		    }
		    continue;
		}
		while( dp<lp ) {
		    *dp++=pixels[*sp++];
		}
		sp+=pp-dp;
		dp=pp;
		break;
	    }

	    //
	    //	Clip right
	    //
	    lp+=sw-w-ox;
	    while( dp<lp ) {
		dp+=*sp++;		// transparent
right_trans:
		dp+=*sp;		// opaque
		sp+=*sp+1;
	    }
	    dp+=da;
	} while( dp<ep );		// all lines

    }
}

/**
**	Draw 8bit graphic object clipped into 32 bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to32Clip(const Graphic* sprite,unsigned frame,int x,int y)
{
    int ox;
    int ex;
    int oy;
    int w;
    int h;
    const unsigned char* sp;
    unsigned sw;
    VMemType32* dp;
    const VMemType32* lp;
    const VMemType32* ep;
    VMemType32* pp;
    const VMemType32* pixels;
    unsigned da;

    //
    // reduce to visible range
    //
    sw=w=sprite->Width;
    h=sprite->Height;
    CLIP_RECTANGLE_OFS(x,y,w,h,ox,oy,ex);

    //
    //	Draw the clipped sprite
    //
    pixels=(VMemType32*)sprite->Pixels;
    sp=((unsigned char**)sprite->Frames)[frame];

    //
    //	Skip top lines, if needed.
    //
    while( oy-- ) {
	da=0;
	do {
	    da+=*sp++;			// transparent
	    da+=*sp;			// opaque
	    sp+=*sp+1;
	} while( da<sw );
    }

    da=VideoWidth-sw;
    dp=VideoMemory32+x+y*VideoWidth;
    ep=dp+VideoWidth*h;

    if( w==sw ) {			// Unclipped horizontal

	do {
	    lp=dp+sw;
	    do {			// 1 line
		dp+=*sp++;		// transparent
		pp=dp-1+*sp++;		// opaque
		while( dp<pp ) {	// unroll
		    *dp++=pixels[*sp++];
		    *dp++=pixels[*sp++];
		}
		if( dp<=pp ) {
		    *dp++=pixels[*sp++];
		}
	    } while( dp<lp );
	    dp+=da;
	} while( dp<ep );		// all lines

    } else {				// Clip horizontal

	da+=ox;
	do {
	    lp=dp+w;
	    //
	    //	Clip left
	    //
	    pp=dp-ox;
	    for( ;; ) {
		pp+=*sp++;		// transparent
		if( pp>=dp ) {
		    dp=pp;
		    goto middle_trans;
		}
		pp+=*sp;		// opaque
		if( pp>=dp ) {
		    sp+=*sp-(pp-dp)+1;
		    goto middle_pixel;
		}
		sp+=*sp+1;
	    }

	    //
	    //	Draw middle
	    //
	    for( ;; ) {
		dp+=*sp++;		// transparent
middle_trans:
		if( dp>=lp ) {
		    lp+=sw-w-ox;
		    goto right_trans;
		}
		pp=dp+*sp++;		// opaque
middle_pixel:
		if( pp<lp ) {
		    while( dp<pp ) {
			*dp++=pixels[*sp++];
		    }
		    continue;
		}
		while( dp<lp ) {
		    *dp++=pixels[*sp++];
		}
		sp+=pp-dp;
		dp=pp;
		break;
	    }

	    //
	    //	Clip right
	    //
	    lp+=sw-w-ox;
	    while( dp<lp ) {
		dp+=*sp++;		// transparent
right_trans:
		dp+=*sp;		// opaque
		sp+=*sp+1;
	    }
	    dp+=da;
	} while( dp<ep );		// all lines

    }
}

/**
**	Draw 8bit graphic object clipped and flipped in X direction
**	into 8 bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to8ClipX(const Graphic* sprite,unsigned frame,int x,int y)
{
    int ox;
    int ex;
    int oy;
    int w;
    int h;
    const unsigned char* sp;
    unsigned sw;
    VMemType8* dp;
    const VMemType8* lp;
    const VMemType8* ep;
    VMemType8* pp;
    const VMemType8* pixels;
    unsigned da;

    //
    // reduce to visible range
    //
    sw=w=sprite->Width;
    h=sprite->Height;
    CLIP_RECTANGLE_OFS(x,y,w,h,ox,oy,ex);

    //
    //	Draw the clipped sprite
    //
    pixels=(VMemType8*)sprite->Pixels;
    sp=((unsigned char**)sprite->Frames)[frame];

    //
    // Skip top lines
    //
    while( oy-- ) {
	da=0;
	do {
	    da+=*sp++;			// transparent
	    da+=*sp;			// opaque
	    sp+=*sp+1;
	} while( da<sw );
    }

    da=VideoWidth+sw;
    dp=VideoMemory8+x+y*VideoWidth+w;
    ep=dp+VideoWidth*h;

    if( w==sw ) {			// Unclipped horizontal

	while( dp<ep ) {		// all lines
	    lp=dp-w;
	    do {			// 1 line
		dp-=*sp++;		// transparent
		pp=dp+1-*sp++;		// opaque
		while( dp>pp ) {
		    *dp--=pixels[*sp++];
		    *dp--=pixels[*sp++];
		}
		if( dp>=pp ) {
		    *dp--=pixels[*sp++];
		}
	    } while( dp>lp );
	    dp+=da;
	}

    } else {				// Clip horizontal

	da-=sw-w-ox;
	while( dp<ep ) {		// all lines
	    lp=dp-w;
	    //
	    //	Clip right side
	    //
	    pp=dp+sw-w-ox;
	    for( ;; ) {
		pp-=*sp++;		// transparent
		if( pp<=dp ) {
		    dp=pp;
		    goto middle_trans;
		}
		pp-=*sp;		// opaque
		if( pp<=dp ) {
		    sp+=*sp-(dp-pp)+1;
		    goto middle_pixel;
		}
		sp+=*sp+1;
	    }

	    //
	    //	Draw middle
	    //
	    for( ;; ) {
		dp-=*sp++;		// transparent
middle_trans:
		if( dp<=lp ) {
		    lp-=ox;
		    goto right_trans;
		}
		pp=dp-*sp++;		// opaque
middle_pixel:
		if( pp>lp ) {
		    while( dp>pp ) {
			*dp--=pixels[*sp++];
		    }
		    continue;
		}
		while( dp>lp ) {
		    *dp--=pixels[*sp++];
		}
		sp+=dp-pp;
		dp=pp;
		break;
	    }

	    //
	    //	Clip left side
	    //
	    lp-=ox;
	    while( dp>lp ) {
		dp-=*sp++;		// transparent
right_trans:
		dp-=*sp;		// opaque
		sp+=*sp+1;
	    }
	    dp+=da;

	}
    }
}

/**
**	Draw 8bit graphic object clipped and flipped in X direction
**	into 16 bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to16ClipX(const Graphic* sprite,unsigned frame
	,int x,int y)
{
    int ox;
    int ex;
    int oy;
    int w;
    int h;
    const unsigned char* sp;
    unsigned sw;
    VMemType16* dp;
    const VMemType16* lp;
    const VMemType16* ep;
    VMemType16* pp;
    const VMemType16* pixels;
    unsigned da;


    //
    // reduce to visible range
    //
    sw=w=sprite->Width;
    h=sprite->Height;
    CLIP_RECTANGLE_OFS(x,y,w,h,ox,oy,ex);

    //
    //	Draw the clipped sprite
    //
    pixels=(VMemType16*)sprite->Pixels;
    sp=((unsigned char**)sprite->Frames)[frame];

    //
    // Skip top lines
    //
    while( oy-- ) {
	da=0;
	do {
	    da+=*sp++;			// transparent
	    da+=*sp;			// opaque
	    sp+=*sp+1;
	} while( da<sw );
    }

    da=VideoWidth+sw;
    dp=VideoMemory16+x+y*VideoWidth+w;
    ep=dp+VideoWidth*h;

    if( w==sw ) {			// Unclipped horizontal

	while( dp<ep ) {		// all lines
	    lp=dp-w;
	    do {			// 1 line
		dp-=*sp++;		// transparent
		pp=dp+1-*sp++;		// opaque
		while( dp>pp ) {
		    *dp--=pixels[*sp++];
		    *dp--=pixels[*sp++];
		}
		if( dp>=pp ) {
		    *dp--=pixels[*sp++];
		}
	    } while( dp>lp );
	    dp+=da;
	}

    } else {				// Clip horizontal

	da-=sw-w-ox;
	while( dp<ep ) {		// all lines
	    lp=dp-w;
	    //
	    //	Clip right side
	    //
	    pp=dp+sw-w-ox;
	    for( ;; ) {
		pp-=*sp++;		// transparent
		if( pp<=dp ) {
		    dp=pp;
		    goto middle_trans;
		}
		pp-=*sp;		// opaque
		if( pp<=dp ) {
		    sp+=*sp-(dp-pp)+1;
		    goto middle_pixel;
		}
		sp+=*sp+1;
	    }

	    //
	    //	Draw middle
	    //
	    for( ;; ) {
		dp-=*sp++;		// transparent
middle_trans:
		if( dp<=lp ) {
		    lp-=ox;
		    goto right_trans;
		}
		pp=dp-*sp++;		// opaque
middle_pixel:
		if( pp>lp ) {
		    while( dp>pp ) {
			*dp--=pixels[*sp++];
		    }
		    continue;
		}
		while( dp>lp ) {
		    *dp--=pixels[*sp++];
		}
		sp+=dp-pp;
		dp=pp;
		break;
	    }

	    //
	    //	Clip left side
	    //
	    lp-=ox;
	    while( dp>lp ) {
		dp-=*sp++;		// transparent
right_trans:
		dp-=*sp;		// opaque
		sp+=*sp+1;
	    }
	    dp+=da;

	}
    }
}

/**
**	Draw 8bit graphic object clipped and flipped in X direction
**	into 24bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to24ClipX(const Graphic* sprite,unsigned frame
	,int x,int y)
{
    int ox;
    int ex;
    int oy;
    int w;
    int h;
    const unsigned char* sp;
    unsigned sw;
    VMemType24* dp;
    const VMemType24* lp;
    const VMemType24* ep;
    VMemType24* pp;
    const VMemType24* pixels;
    unsigned da;

    //
    // reduce to visible range
    //
    sw=w=sprite->Width;
    h=sprite->Height;
    CLIP_RECTANGLE_OFS(x,y,w,h,ox,oy,ex);

    //
    //	Draw the clipped sprite
    //
    pixels=(VMemType24*)sprite->Pixels;
    sp=((unsigned char**)sprite->Frames)[frame];

    //
    // Skip top lines
    //
    while( oy-- ) {
	da=0;
	do {
	    da+=*sp++;			// transparent
	    da+=*sp;			// opaque
	    sp+=*sp+1;
	} while( da<sw );
    }

    da=VideoWidth+sw;
    dp=VideoMemory24+x+y*VideoWidth+w;
    ep=dp+VideoWidth*h;

    if( w==sw ) {			// Unclipped horizontal

	while( dp<ep ) {		// all lines
	    lp=dp-w;
	    do {			// 1 line
		dp-=*sp++;		// transparent
		pp=dp+1-*sp++;		// opaque
		while( dp>pp ) {
		    *dp--=pixels[*sp++];
		    *dp--=pixels[*sp++];
		}
		if( dp>=pp ) {
		    *dp--=pixels[*sp++];
		}
	    } while( dp>lp );
	    dp+=da;
	}

    } else {				// Clip horizontal

	da-=sw-w-ox;
	while( dp<ep ) {		// all lines
	    lp=dp-w;
	    //
	    //	Clip right side
	    //
	    pp=dp+sw-w-ox;
	    for( ;; ) {
		pp-=*sp++;		// transparent
		if( pp<=dp ) {
		    dp=pp;
		    goto middle_trans;
		}
		pp-=*sp;		// opaque
		if( pp<=dp ) {
		    sp+=*sp-(dp-pp)+1;
		    goto middle_pixel;
		}
		sp+=*sp+1;
	    }

	    //
	    //	Draw middle
	    //
	    for( ;; ) {
		dp-=*sp++;		// transparent
middle_trans:
		if( dp<=lp ) {
		    lp-=ox;
		    goto right_trans;
		}
		pp=dp-*sp++;		// opaque
middle_pixel:
		if( pp>lp ) {
		    while( dp>pp ) {
			*dp--=pixels[*sp++];
		    }
		    continue;
		}
		while( dp>lp ) {
		    *dp--=pixels[*sp++];
		}
		sp+=dp-pp;
		dp=pp;
		break;
	    }

	    //
	    //	Clip left side
	    //
	    lp-=ox;
	    while( dp>lp ) {
		dp-=*sp++;		// transparent
right_trans:
		dp-=*sp;		// opaque
		sp+=*sp+1;
	    }
	    dp+=da;

	}
    }
}

/**
**	Draw 8bit graphic object clipped and flipped in X direction
**	into 32bit framebuffer.
**
**	@param sprite	pointer to object
**	@param frame	number of frame (object index)
**	@param x	x coordinate on the screen
**	@param y	y coordinate on the screen
*/
global void VideoDraw8to32ClipX(const Graphic* sprite,unsigned frame
	,int x,int y)
{
    int ex;
    int ox;
    int oy;
    int w;
    int h;
    const unsigned char* sp;
    unsigned sw;
    VMemType32* dp;
    const VMemType32* lp;
    const VMemType32* ep;
    VMemType32* pp;
    const VMemType32* pixels;
    unsigned da;

    //
    // reduce to visible range
    //
    sw=w=sprite->Width;
    h=sprite->Height;
    CLIP_RECTANGLE_OFS(x,y,w,h,ox,oy,ex);

    //
    //	Draw the clipped sprite
    //
    pixels=(VMemType32*)sprite->Pixels;
    sp=((unsigned char**)sprite->Frames)[frame];

    //
    // Skip top lines
    //
    while( oy-- ) {
	da=0;
	do {
	    da+=*sp++;			// transparent
	    da+=*sp;			// opaque
	    sp+=*sp+1;
	} while( da<sw );
    }

    da=VideoWidth+sw;
    dp=VideoMemory32+x+y*VideoWidth+w;
    ep=dp+VideoWidth*h;

    if( w==sw ) {			// Unclipped horizontal

	while( dp<ep ) {		// all lines
	    lp=dp-w;
	    do {			// 1 line
		dp-=*sp++;		// transparent
		pp=dp+1-*sp++;		// opaque
		while( dp>pp ) {
		    *dp--=pixels[*sp++];
		    *dp--=pixels[*sp++];
		}
		if( dp>=pp ) {
		    *dp--=pixels[*sp++];
		}
	    } while( dp>lp );
	    dp+=da;
	}

    } else {				// Clip horizontal

	da-=ex;
	while( dp<ep ) {		// all lines
	    lp=dp-w;
	    //
	    //	Clip right side
	    //
	    pp=dp+ex;
	    for( ;; ) {
		pp-=*sp++;		// transparent
		if( pp<=dp ) {
		    dp=pp;
		    goto middle_trans;
		}
		pp-=*sp;		// opaque
		if( pp<=dp ) {
		    sp+=*sp-(dp-pp)+1;
		    goto middle_pixel;
		}
		sp+=*sp+1;
	    }

	    //
	    //	Draw middle
	    //
	    for( ;; ) {
		dp-=*sp++;		// transparent
middle_trans:
		if( dp<=lp ) {
		    lp-=ox;
		    goto right_trans;
		}
		pp=dp-*sp++;		// opaque
middle_pixel:
		if( pp>lp ) {
		    while( dp>pp ) {
			*dp--=pixels[*sp++];
		    }
		    continue;
		}
		while( dp>lp ) {
		    *dp--=pixels[*sp++];
		}
		sp+=dp-pp;
		dp=pp;
		break;
	    }

	    //
	    //	Clip left side
	    //
	    lp-=ox;
	    while( dp>lp ) {
		dp-=*sp++;		// transparent
right_trans:
		dp-=*sp;		// opaque
		sp+=*sp+1;
	    }
	    dp+=da;

	}
    }
}

/**
**	Draw 8bit raw graphic data clipped, using given pixel pallette
**	of a given color-depth in bytes: 8=1, 16=2, 24=3, 32=4
**
**	@param pixels	VMemType 256 color palette to translate given data
**	@param data	raw graphic data in 8bit color indexes of above palette
**	@param x	left-top corner x coordinate in pixels on the screen
**	@param y	left-top corner y coordinate in pixels on the screen
**	@param w	width of above graphic data in pixels
**	@param h	height of above graphic data in pixels
**	@param bytes	color-depth of given palette
**
**	FIXME: make this faster..
*/
global void VideoDrawRawXXClip( char *pixels,
                                const unsigned char *data,
                                int x, int y,
                                unsigned int w, unsigned int h,
                                char bytes )
{
  char *dest;
  unsigned int ofsx, ofsy, skipx, nextline;

// Clip given rectangle area, keeping track of start- and end-offsets
  nextline=w;
  CLIP_RECTANGLE_OFS(x,y,w,h,ofsx,ofsy,skipx);
  data+=(ofsy*nextline)+ofsx;
  skipx+=ofsx;

// Draw the raw data, through the given palette
  dest     = (char *)VideoMemory + (y * VideoWidth + x) * bytes;
  nextline = (VideoWidth - w) * bytes;

  do
  {
    int w2 = w;

    do
    {
      memcpy( dest, pixels + *data++ * bytes, bytes );
      dest+=bytes;
    }
    while ( --w2 > 0 );

    data += skipx;
    dest += nextline;
  }
  while ( --h > 0 );
}

/**
**	Draw 8bit raw graphic data clipped, using given pixel pallette
**	into 8bit framebuffer.
**
**	@param pixels	VMemType8 256 color palette to translate given data
**	@param data	raw graphic data in 8bit color indexes of above palette
**	@param x	left-top corner x coordinate in pixels on the screen
**	@param y	left-top corner y coordinate in pixels on the screen
**	@param w	width of above graphic data in pixels
**	@param h	height of above graphic data in pixels
*/
local void VideoDrawRaw8Clip( VMemType *pixels,
                              const unsigned char *data,
                              unsigned int x, unsigned int y,
                              unsigned int w, unsigned int h )
{
  VideoDrawRawXXClip( (char *)pixels, data, x, y, w, h, sizeof(VMemType8) );
}

/**
**	Draw 8bit raw graphic data clipped, using given pixel pallette
**	into 16bit framebuffer.
**
**	@param pixels	VMemType16 256 color palette to translate given data
**	@param data	raw graphic data in 8bit color indexes of above palette
**	@param x	left-top corner x coordinate in pixels on the screen
**	@param y	left-top corner y coordinate in pixels on the screen
**	@param w	width of above graphic data in pixels
**	@param h	height of above graphic data in pixels
*/
local void VideoDrawRaw16Clip( VMemType *pixels,
                               const unsigned char *data,
                               unsigned int x, unsigned int y,
                               unsigned int w, unsigned int h )
{
  VideoDrawRawXXClip( (char *)pixels, data, x, y, w, h, sizeof(VMemType16) );
}

/**
**	Draw 8bit raw graphic data clipped, using given pixel pallette
**	into 24bit framebuffer.
**
**	@param pixels	VMemType24 256 color palette to translate given data
**	@param data	raw graphic data in 8bit color indexes of above palette
**	@param x	left-top corner x coordinate in pixels on the screen
**	@param y	left-top corner y coordinate in pixels on the screen
**	@param w	width of above graphic data in pixels
**	@param h	height of above graphic data in pixels
*/
local void VideoDrawRaw24Clip( VMemType *pixels,
                               const unsigned char *data,
                               unsigned int x, unsigned int y,
                               unsigned int w, unsigned int h )
{
  VideoDrawRawXXClip( (char *)pixels, data, x, y, w, h, sizeof(VMemType24) );
}

/**
**	Draw 8bit raw graphic data clipped, using given pixel pallette
**	into 32bit framebuffer.
**
**	@param pixels	VMemType32 256 color palette to translate given data
**	@param data	raw graphic data in 8bit color indexes of above palette
**	@param x	left-top corner x coordinate in pixels on the screen
**	@param y	left-top corner y coordinate in pixels on the screen
**	@param w	width of above graphic data in pixels
**	@param h	height of above graphic data in pixels
*/
local void VideoDrawRaw32Clip( VMemType *pixels,
                               const unsigned char *data,
                               unsigned int x, unsigned int y,
                               unsigned int w, unsigned int h )
{
  VideoDrawRawXXClip( (char *)pixels, data, x, y, w, h, sizeof(VMemType32) );
}

/**
**	Free graphic object.
*/
local void FreeSprite8(Graphic* graphic)
{
    IfDebug( AllocatedGraphicMemory-=graphic->Size );
    IfDebug( AllocatedGraphicMemory-=sizeof(Graphic) );

    VideoFreeSharedPalette(graphic->Pixels);
    free(graphic->Frames);
    free(graphic);
}

// FIXME: need 16 bit palette version
// FIXME: need alpha blending version
// FIXME: need zooming version

/*----------------------------------------------------------------------------
--	Global functions
----------------------------------------------------------------------------*/

/**
**	Load sprite from file.
**
**	Compress the file as RLE (run-length-encoded) sprite.
**
**	@param	name	File name of sprite to load.
**	@param	width	Width of a single frame.
**	@param	height	Height of a single frame.
**
**	@return		A graphic object for the loaded sprite.
**
**	@see	LoadGraphic
*/
global Graphic* LoadSprite(const char* name,unsigned width,unsigned height)
{
    Graphic* sprite;
    Graphic* graphic;
    unsigned char* data;
    const unsigned char* sp;
    unsigned char* dp;
    unsigned char* cp;
    int depth;
    int fl;
    int n;
    int counter;
    int i;
    int h;
    int w;

    graphic=LoadGraphic(name);

    DebugCheck( !width || !height );
    DebugCheck( width>graphic->Width || height>graphic->Height );

    depth=8;

    if( ((graphic->Width/width)*width!=graphic->Width)
	    || ((graphic->Height/height)*height!=graphic->Height) ) {
	fprintf(stderr,"Invalid graphic (width, height) %s\n",name);
	fprintf(stderr,"Expected: (%d,%d)  Found: (%d,%d)",
		width,height,graphic->Width,graphic->Height);
    }
       
    // Check if width and height fits.
    DebugCheck( ((graphic->Width/width)*width!=graphic->Width)
	    || ((graphic->Height/height)*height!=graphic->Height) );

    n=(graphic->Width/width)*(graphic->Height/height);
    DebugLevel3Fn("%dx%d in %dx%d = %d frames.\n"
	    ,width,height,graphic->Width,graphic->Height,n);

    //
    //	Allocate structure
    //
    sprite=malloc(sizeof(Graphic));
    IfDebug( AllocatedGraphicMemory+=sizeof(Graphic) );

    if( !sprite ) {
	fprintf(stderr,"Out of memory\n");
	FatalExit(-1);
    }
    if( depth==8 ) {
	sprite->Type=&GraphicSprite8Type;
    } else if( depth==16 ) {
	sprite->Type=&GraphicSprite16Type;
    } else {
	fprintf(stderr,"Unsported image depth\n");
	FatalExit(-1);
    }
    sprite->Width=width;
    sprite->Height=height;

    sprite->NumFrames=n;

    sprite->Palette=graphic->Palette;
    sprite->Pixels=graphic->Pixels;	// WARNING: if not shared freed below!

    sprite->Size=0;
    sprite->Frames=NULL;


    // Worst case is alternating opaque and transparent pixels
    data=malloc(n*sizeof(unsigned char*)
	    +(graphic->Width/2+1)*3*graphic->Height);
    // Pixel area
    dp=(unsigned char *)data+n*sizeof(unsigned char*);

    //
    //	Compress all frames of the sprite.
    //
    fl=graphic->Width/width;
    for( i=0; i<n; ++i ) {		// each frame
	((unsigned char**)data)[i]=dp;
	for( h=0; h<height; ++h ) {	// each line
	    sp=(const unsigned char*)graphic->Frames+(i%fl)*width+((i/fl)*height+h)*graphic->Width;

	    for (counter=w=0; w<width; ++w ) {
		if( *sp==255 || *sp==0) {			// start transparency
		    ++sp;
		    if( ++counter==256 ) {
			*dp++=255;
			*dp++=0;
			counter=1;
		    }
		    continue;
		}
		*dp++=counter;

		cp=dp++;
		counter=0;
		for( ; w<width; ++w ) {			// opaque
		    *dp++=*sp++;
		    if( ++counter==255 ) {
			*cp=255;
			*dp++=0;
			cp=dp++;
			counter=0;
		    }
		    // ARI: FIXME: wrong position
		    if( w+1!=width && (*sp==255 || *sp==0) ) {	// end transparency
			break;
		    }
		}
		*cp=counter;
		counter=0;
	    }
	    if( counter ) {
		*dp++=counter;
		*dp++=0;		// 1 byte more, 1 check less! (phantom end transparency)
	    }
	}
    }

    DebugLevel3Fn("\t%d => %d RLE compressed\n"
	    ,graphic->Width*graphic->Height,dp-data);

    //
    //	Update to real length
    //
    sprite->Frames=data;
    i=n*sizeof(unsigned char*)+dp-data;
    sprite->Size=i;
    dp=realloc(data,i);
    if( dp!=data ) {			// shrink only - happens rarely
	for( h=0; h<n; ++h ) {		// convert address
	    ((unsigned char**)dp)[h]+=dp-data;
	}
	sprite->Frames=dp;
    }

    IfDebug( CompressedGraphicMemory+=i; );

    graphic->Pixels=NULL;		// We own now the shared pixels
    VideoFree(graphic);

    return sprite;
}

/**
**	Init sprite
*/
global void InitSprite(void)
{
    switch( VideoBpp ) {
	case 8:
	    GraphicSprite8Type.Draw=VideoDraw8to8;
	    GraphicSprite8Type.DrawClip=VideoDraw8to8Clip;
	    GraphicSprite8Type.DrawX=VideoDraw8to8X;
	    GraphicSprite8Type.DrawClipX=VideoDraw8to8ClipX;
            VideoDrawRawClip=VideoDrawRaw8Clip;
	    break;

	case 15:
	case 16:
	    GraphicSprite8Type.Draw=VideoDraw8to16;
	    GraphicSprite8Type.DrawClip=VideoDraw8to16Clip;
	    GraphicSprite8Type.DrawX=VideoDraw8to16X;
	    GraphicSprite8Type.DrawClipX=VideoDraw8to16ClipX;
            VideoDrawRawClip=VideoDrawRaw16Clip;
	    break;

	case 24:
	    GraphicSprite8Type.Draw=VideoDraw8to24;
	    GraphicSprite8Type.DrawClip=VideoDraw8to24Clip;
	    GraphicSprite8Type.DrawX=VideoDraw8to24X;
	    GraphicSprite8Type.DrawClipX=VideoDraw8to24ClipX;
            VideoDrawRawClip=VideoDrawRaw24Clip;
	    break;

	case 32:
	    GraphicSprite8Type.Draw=VideoDraw8to32;
	    GraphicSprite8Type.DrawClip=VideoDraw8to32Clip;
	    GraphicSprite8Type.DrawX=VideoDraw8to32X;
	    GraphicSprite8Type.DrawClipX=VideoDraw8to32ClipX;
            VideoDrawRawClip=VideoDrawRaw32Clip;
	    break;

	default:
	    DebugLevel0Fn("Unsupported %d bpp\n",VideoBpp);
	    abort();
    }

    GraphicSprite8Type.Free=FreeSprite8;
}

//@}
