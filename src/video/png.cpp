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
/**@name png.c		-	The png graphic file loader. */
//
//	(c) Copyright 1998-2002 by Lutz Sammer
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
#include <png.h>

#include "freecraft.h"
#include "video.h"
#include "iolib.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	png read callback for CL-IO.
**
**	@param png_ptr	png struct pointer.
**	@param data	byte address to read to.
**	@param length	number of bytes to read.
*/
local void CL_png_readfn(png_structp png_ptr,png_bytep data,png_size_t length)
{
   png_size_t check;

   check = (png_size_t)CLread((CLFile *)png_get_io_ptr(png_ptr), data,
	(size_t)length);
   if (check != length) {
      png_error(png_ptr, "Read Error");
   }
}

/**
**	Load a png graphic file.
**
**	@param name	png filename to load.
**
**	@return		graphic object with loaded graphic, or NULL if failure.
**
**	@todo	FIXME: must support other formats than 8bit indexed
*/
global Graphic* LoadGraphicPNG(const char* name)
{
    Graphic* graphic;
    Palette *palette;
    CLFile* fp;
    png_structp png_ptr;
    png_infop info_ptr;
    unsigned char** lines;
    unsigned char* data;
    int h;
    int w;
    int i;

    //
    //	open + prepare
    //
    if( !(fp=CLopen(name)) ) {
	perror("Can't open file");
	return NULL;
    }

    png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
    if( !png_ptr ) {
	CLclose(fp);
	return NULL;
    }
    info_ptr=png_create_info_struct(png_ptr);
    if( !info_ptr ) {
	png_destroy_read_struct(&png_ptr,NULL,NULL);
	CLclose(fp);
	return NULL;
    }
    if( setjmp(png_ptr->jmpbuf) ) {
	png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
	CLclose(fp);
	return NULL;
    }
    png_set_read_fn(png_ptr,fp,CL_png_readfn);

    //
    //	Setup ready, read header info.
    //
    png_read_info(png_ptr,info_ptr);

    DebugLevel3("%s: width %ld height %ld = %ld bytes\n"
	    ,name,info_ptr->width,info_ptr->height
	    ,info_ptr->width*info_ptr->height);
    DebugLevel3("%s: %s",name
	,png_get_valid(png_ptr,info_ptr,PNG_INFO_PLTE) ? "palette" : "");
    DebugLevel3(" %s"
	,png_get_valid(png_ptr,info_ptr,PNG_INFO_tRNS) ? "transparent" : "");
    DebugLevel3(" depth %d\n",info_ptr->bit_depth);

    //	Setup translators:

    palette= (Palette *)calloc(256,sizeof(Palette));

    if( info_ptr->color_type==PNG_COLOR_TYPE_PALETTE ) {
	DebugLevel3("Color palette\n");
	if( info_ptr->valid&PNG_INFO_PLTE ) {
	    DebugLevel3Fn(" palette %d\n",info_ptr->num_palette);
	    if( info_ptr->num_palette>256 ) {
		abort();
	    }
	    for( i=0; i<info_ptr->num_palette; ++i ) {
		palette[i].r=info_ptr->palette[i].red;
		palette[i].g=info_ptr->palette[i].green;
		palette[i].b=info_ptr->palette[i].blue;
	    }
	    for( ; i<256; ++i ) {
		palette[i].r=palette[i].g=palette[i].b=0;
	    }
	}
    }

    if( info_ptr->bit_depth==16 ) {
	png_set_strip_16(png_ptr);
    }
    if( info_ptr->bit_depth<8 ) {
	png_set_packing(png_ptr);
    }

#if 0
    //	Want 8 bit palette with transparent!
    if( info_ptr->color_type==PNG_COLOR_TYPE_PALETTE &&
	    info_ptr->bit_depth<8 ) {
	png_set_expand(png_ptr);
    }

    if( 0 ) {
	extern unsigned char GlobalPalette[];

	png_set_dither(png_ptr,GlobalPalette,256,256,NULL,1);
    }
#endif

    png_read_update_info(png_ptr,info_ptr);

    //	Allocate and reserve memory.
    w=info_ptr->width;
    h=info_ptr->height;
    if( info_ptr->width!=info_ptr->rowbytes ) {
	DebugLevel0("width(%ld)!=rowbytes(%ld) in file:%s\n"
	    ,info_ptr->width,info_ptr->rowbytes,name);
	abort();
    }

    lines=alloca(h*sizeof(*lines));
    if( !lines ) {
	png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
	CLclose(fp);
	return NULL;
    }
    data=malloc(h*w);
    if( !data ) {
	png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
	CLclose(fp);
	return NULL;
    }
    IfDebug( AllocatedGraphicMemory+=h*w; );

    for( i=0; i<h; ++i ) {		// start of lines
	lines[i]=data+i*w;
    }

    //	Final read the image.

    png_read_image(png_ptr,lines);
    png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
    CLclose(fp);

    graphic=MakeGraphic(8,w,h,data,w*h);	// data freed by make graphic
    graphic->Palette=palette;  //FIXME: should this be part of MakeGraphic

    return graphic;
}

//@}
