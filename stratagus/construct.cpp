//   ___________		     _________		      _____  __
//   \_	  _____/______   ____   ____ \_   ___ \____________ _/ ____\/  |_
//    |    __) \_  __ \_/ __ \_/ __ \/    \  \/\_  __ \__  \\   __\\   __|
//    |     \   |  | \/\  ___/\  ___/\     \____|  | \// __ \|  |   |  |
//    \___  /   |__|    \___  >\___  >\______  /|__|  (____  /__|   |__|
//	  \/		    \/	   \/	     \/		   \/
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   FreeCraft - A free fantasy real time strategy game engine
//
/**@name construct.c	-	The constructions. */
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
#include <string.h>

#include "freecraft.h"
#include "video.h"
#include "tileset.h"
#include "map.h"
#include "construct.h"
#include "ccl.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/**
**	Construction type definition
*/
global const char ConstructionType[] = "construction";

/**
**	Constructions.
**
**	@todo support more constructions and don't use a fixed size table.
*/
local Construction Constructions[16];

/**
**	Number of constuctions.
*/
local int NumConstructions=16;

/**
**	Table mapping the original construction numbers in puds to
**	our internal string.
*/
global char** ConstructionWcNames;

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Initialize  the constructions.
*/
global void InitConstructions(void)
{
}

/**
**	Load the graphics for the constructions.
**
**	HELPME:	who make this better terrain depended and extendable
**	HELPME: filename constuction.
*/
global void LoadConstructions(void)
{
    int i;
    const char* file;

    for( i=0; i<NumConstructions; ++i ) {
	if( !Constructions[i].Ident ) {
	    continue;
	}
	file=Constructions[i].File[TheMap.Terrain];
	if( !file ) {			// default one
	    file=Constructions[i].File[0];
	}
	if( *file ) {
	    char* buf;

	    buf=alloca(strlen(file)+9+1);
	    file=strcat(strcpy(buf,"graphics/"),file);
	    ShowLoadProgress("\tConstruction %s\n",file);
	    Constructions[i].Sprite=LoadSprite(file
		    ,Constructions[i].Width,Constructions[i].Height);
	}
	if( (file=Constructions[i].ShadowFile) ) {
	    char *buf;

	    buf=alloca(strlen(file)+9+1);
	    file=strcat(strcpy(buf,"graphics/"),file);
	    ShowLoadProgress("\tConstruction %s\n",file);
	    Constructions[i].ShadowSprite=LoadSprite(file
		    ,Constructions[i].ShadowWidth,Constructions[i].ShadowHeight);
	}
    }
}

/**
**	Save state of constructions to file.
**
**	@param file	Output file.
*/
global void SaveConstructions(FILE* file)
{
    int i;
    int j;
    char** cp;

    fprintf(file,"\n;;; -----------------------------------------\n");
    fprintf(file,";;; MODULE: constructions $Id$\n\n");

    //
    //	Dump table wc2 construction numbers -> internal symbol.
    //
    if( (cp=ConstructionWcNames) ) {
	fprintf(file,"(define-construction-wc-names");

	i=90;
	while( *cp ) {
	    if( i+strlen(*cp)>79 ) {
		i=fprintf(file,"\n ");
	    }
	    i+=fprintf(file," '%s",*cp++);
	}
	fprintf(file,")\n\n");
    }

    //
    //	Dump table of all constructions
    //
    for( i=0; i<NumConstructions; ++i ) {
	if( !Constructions[i].Ident ) {
	    continue;
	}
	fprintf(file,"(define-construction '%s\n",Constructions[i].Ident);
	fprintf(file,"  'files '(");
	for( j=0; j<TilesetMax; ++j ) {
	    if( Constructions[i].File[j] ) {
		if( j ) {
		    fputs("\n    ",file);
		}
		fprintf(file,"%s \"%s\"",Tilesets[j]->Class,
			Constructions[i].File[j]);
	    }
	}
	fprintf(file,")\n");
	fprintf(file,"  'size '(%d %d)", Constructions[i].Width,
		Constructions[i].Height);
	if( Constructions[i].ShadowFile ) {
	    fprintf(file,"\n  'shadow '(file \"%s\" width %d height %d)",
		    Constructions[i].ShadowFile, Constructions[i].ShadowWidth,
		    Constructions[i].ShadowHeight);
	}
	fprintf(file,")\n\n");
    }
}

/**
**	Cleanup the constructions.
*/
global void CleanConstructions(void)
{
    char** cp;
    int i;
    int j;

    //
    //	Mapping original construction numbers in puds to our internal strings
    //
    if( (cp=ConstructionWcNames) ) {	// Free all old names
	while( *cp ) {
	    free(*cp++);
	}
	free(ConstructionWcNames);
	ConstructionWcNames=NULL;
    }

    //
    //	Free the construction table.
    //
    for( i=0; i<NumConstructions; ++i ) {
	if( Constructions[i].Ident ) {
	    free(Constructions[i].Ident);
	}
	for( j=0; j<TilesetMax; ++j ) {
	    if( Constructions[i].File[j] ) {
		free(Constructions[i].File[j]);
	    }
	}
	VideoSaveFree(Constructions[i].Sprite);
	if( Constructions[i].ShadowFile ) {
	    free(Constructions[i].ShadowFile);
	}
	VideoSaveFree(Constructions[i].ShadowSprite);
    }
    memset(Constructions,0,sizeof(Constructions));
}

/**
**	Draw construction.
**
**	@param construction	Construction pointer.
**	@param frame	Frame number to draw.
**	@param x	X position.
**	@param y	Y position.
*/
global void DrawConstruction(const Construction* construction,int frame,
    int x,int y)
{
    // FIXME: This should be moved to caller/init...
    x-=construction->Width/2;
    y-=construction->Height/2;

    VideoDrawClip(construction->Sprite,frame,x,y);
}

/**
**	Get construction by identifier.
**
**	@param ident	Identfier of the construction
**
**	@return		Construction structure pointer
*/
global Construction* ConstructionByIdent(const char* ident)
{
    int i;

    for( i=0; i<NumConstructions; ++i ) {
	if( Constructions[i].Ident && !strcmp(ident,Constructions[i].Ident) ) {
	    return &Constructions[i];
	}
    }
    DebugLevel0Fn("Construction `%s' not found.\n" _C_ ident);
    return NULL;
}

/**
**	Get construction by original wc number.
**
**	@param num	Original number used in puds.
*/
global Construction* ConstructionByWcNum(int num)
{
    return ConstructionByIdent(ConstructionWcNames[num]);
}

// ----------------------------------------------------------------------------

/**
**	Define construction mapping from original number to internal symbol
**
**	@param list	List of all names.
*/
local SCM CclDefineConstructionWcNames(SCM list)
{
    int i;
    char** cp;

    if( (cp=ConstructionWcNames) ) {		// Free all old names
	while( *cp ) {
	    free(*cp++);
	}
	free(ConstructionWcNames);
    }

    //
    //	Get new table.
    //
    i=gh_length(list);
    ConstructionWcNames=cp=malloc((i+1)*sizeof(char*));
    while( i-- ) {
	*cp++=gh_scm2newstr(gh_car(list),NULL);
	list=gh_cdr(list);
    }
    *cp=NULL;

    return SCM_UNSPECIFIED;
}

/**
**	Parse the construction.
**
**	@param list	List describing the construction.
**
**	@note make this more flexible
*/
local SCM CclDefineConstruction(SCM list)
{
    SCM value;
    SCM sublist;
    char* str;
    Construction* construction;
    int i;

    //	Slot identifier

    str=gh_scm2newstr(gh_car(list),NULL);
    list=gh_cdr(list);

    for( i=0; ConstructionWcNames[i]; ++i ) {
	if( !strcmp(ConstructionWcNames[i],str) ) {
	    break;
	}
    }
    if( !ConstructionWcNames[i] ) {
	DebugLevel0Fn("Construction not found.\n");
	free(str);
	return SCM_UNSPECIFIED;
    }
    construction=&Constructions[i];
    construction->OType=ConstructionType;
    construction->Nr=i;			// FIXME: remove
    construction->Ident=str;

    //
    //	Parse the arguments, in tagged format.
    //
    while( !gh_null_p(list) ) {
	value=gh_car(list);
	list=gh_cdr(list);
	if( gh_eq_p(value,gh_symbol2scm("files")) ) {
	    sublist=gh_car(list);

	    //
	    //	Parse the tilesets
	    //
	    while( !gh_null_p(sublist) ) {
		str=gh_scm2newstr(gh_car(sublist),NULL);

		// FIXME: use a general get tileset function here!
		i=0; 
		if( strcmp(str,"default") ) {
		    for( ; i<NumTilesets; ++i ) {
			if( !strcmp(str,Tilesets[i]->Ident) ) {
			    break;
			}
			if( !strcmp(str,Tilesets[i]->Class) ) {
			    break;
			}
		    }
		    if( i==NumTilesets ) {
			fprintf(stderr,"Tileset `%s' not available\n",str);
			errl("tileset not available",gh_car(sublist));
		    }
		}
		sublist=gh_cdr(sublist);
		free(str);
		free(construction->File[i]);
		construction->File[i]=gh_scm2newstr(gh_car(sublist),NULL);
		sublist=gh_cdr(sublist);
	    }

	} else if( gh_eq_p(value,gh_symbol2scm("size")) ) {
	    value=gh_car(list);
	    construction->Width=gh_scm2int(gh_car(value));
	    value=gh_cdr(value);
	    construction->Height=gh_scm2int(gh_car(value));

	} else if( gh_eq_p(value,gh_symbol2scm("shadow")) ) {
	    sublist=gh_car(list);
	    while( !gh_null_p(sublist) ) {
		value=gh_car(sublist);
		sublist=gh_cdr(sublist);

		if( gh_eq_p(value,gh_symbol2scm("file")) ) {
		    construction->ShadowFile=gh_scm2newstr(gh_car(sublist),NULL);
		} else if( gh_eq_p(value,gh_symbol2scm("width")) ) {
		    construction->ShadowWidth=gh_scm2int(gh_car(sublist));
		} else if( gh_eq_p(value,gh_symbol2scm("height")) ) {
		    construction->ShadowHeight=gh_scm2int(gh_car(sublist));
		} else {
		    errl("Unsupported shadow tag",value);
		}
		sublist=gh_cdr(sublist);
	    }
	} else {
	    // FIXME: this leaves a half initialized construction
	    errl("Unsupported tag",value);
	}
	list=gh_cdr(list);
    }

    return SCM_UNSPECIFIED;
}

// ----------------------------------------------------------------------------

/**
**	Register CCL features for construction.
*/
global void ConstructionCclRegister(void)
{
    gh_new_procedureN("define-construction-wc-names",
	    CclDefineConstructionWcNames);
    gh_new_procedureN("define-construction",CclDefineConstruction);

}
//@}
