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
/**@name scm.c		-	The scm. */
//
//	(c) Copyright 2002 by Jimmy Salmon
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
#include "map.h"
#include "player.h"
#include "settings.h"
#include "mpq.h"
#include "iocompat.h"
#include "myendian.h"
#include "pud.h"


/*----------------------------------------------------------------------------
--	Definitions
----------------------------------------------------------------------------*/

#define SC_IsUnitMineral(x) ((x)==176 || (x)==177 || (x)==178)	/// sc unit mineral
#define SC_UnitGeyser	    188	    /// sc unit geyser
#define SC_TerranSCV	    7	    /// terran scv
#define SC_ZergDrone	    41	    /// zerg drone
#define SC_ProtossProbe	    64	    /// protoss probe
#define SC_TerranCommandCenter	106 /// terran command center
#define SC_ZergHatchery	    131	    /// zerg hatchery
#define SC_ProtossNexus	    154	    /// protoss nexus

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

local char *scm_ptr;
local char *scm_endptr;

local int MapOffsetX;			/// Offset X for combined maps
local int MapOffsetY;			/// Offset Y for combined maps

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Convert scm's MTXM section into internal format.
**
**	@param mtxm	Section data
**	@param width	Section width
**	@param height	Section height
**	@param map	Map to store into
*/
local void ScmConvertMTXM(const unsigned short * mtxm,int width,int height,WorldMap* map)
{
    const unsigned short* ctab;
    int h;
    int w;

    if( map->Terrain<TilesetMax ) {
	// FIXME: should use terrain name!!
	ctab=Tilesets[map->Terrain]->Table;
	DebugLevel0Fn("FIXME: %s <-> %s\n" _C_ Tilesets[map->Terrain]->Class _C_
		map->TerrainName);
    } else {
	DebugLevel1("Unknown terrain!\n");
	// FIXME: don't use TilesetSummer
	ctab=Tilesets[TilesetSummer]->Table;
    }

    for( h=0; h<height; ++h ) {
	for( w=0; w<width; ++w ) {
	    int v;

	    v=ConvertLE16(mtxm[h*width+w]);
	    map->Fields[MapOffsetX+w+(MapOffsetY+h)*TheMap.Width].Tile=ctab[v];
	    map->Fields[MapOffsetX+w+(MapOffsetY+h)*TheMap.Width].Value=0;
	}
    }
}

/**
**	Read header
*/
local int ScmReadHeader(char* header,long* length)
{
    long len;

    if( scm_ptr >= scm_endptr) {
	return 0;
    }
    memcpy(header, scm_ptr, 4);
    scm_ptr += 4;
    memcpy(&len, scm_ptr, 4);
    scm_ptr += 4;
    *length = ConvertLE32(len);
    return 1;
}

/**
**	Read dword
*/
local int ScmReadDWord(void)
{
    unsigned int temp_int;

    memcpy(&temp_int, scm_ptr, 4);
    scm_ptr += 4;
    return ConvertLE32(temp_int);
}

/**
**	Read word
*/
local int ScmReadWord(void)
{
    unsigned short temp_short;

    memcpy(&temp_short, scm_ptr, 2);
    scm_ptr += 2;
    return ConvertLE16(temp_short);
}

/**
**	Read byte
*/
local int ScmReadByte(void)
{
    unsigned char temp_char;

    temp_char = *scm_ptr;
    scm_ptr += 1;
    return temp_char;
}

/**
**	Extract/uncompress entry.
**
**	@param mpqfd	The mpq file
**	@param name	Name of the entry to extract
**	@param entry	Returns the entry, NULL if not found
**	@param size	Returns the size of the entry
*/
local void ExtractMap(FILE *mpqfd,unsigned char** entry,int* size)
{
    int i;
    int max;
    int maxi;

    *entry=NULL;
    *size=0;
    max=0;
    maxi=0;
    // Assumes the largest file is the .chk map
    for( i=0; i<MpqFileCount; ++i ) {
	if( MpqBlockTable[i*4+2]>max ) {
	    maxi=i;
	    max=MpqBlockTable[i*4+2];
	}
    }
    if( max!=0 ) {
	*size=MpqBlockTable[maxi*4+2];
	*entry=malloc(*size+1);
	MpqExtractTo(*entry,maxi,mpqfd);
    }
}

/**
**	Get the info for a scm level.
*/
global MapInfo* GetScmInfo(const char* scm)
{
    unsigned char *scmdata;
    long length;
    char header[5];
    char buf[1024];
    MapInfo* info;
    FILE *fpMpq;
    int entry_size;

    if( !(fpMpq=fopen(scm, "rb")) ) {
	fprintf(stderr,"Try ./path/name\n");
	sprintf(buf, "scm: fopen(%s)", scm);
	perror(buf);
	ExitFatal(-1);
    }

    if( MpqReadInfo(fpMpq) ) {
	fprintf(stderr,"MpqReadInfo failed\n");
	ExitFatal(-1);
    }

    ExtractMap(fpMpq, &scmdata, &entry_size);

    fclose(fpMpq);
    if( !scmdata ) {
	fprintf(stderr,"Could not extract map in %s\n",scm);
	ExitFatal(-1);
    }

    info=calloc(1, sizeof(MapInfo));	// clears with 0

    scm_ptr = scmdata;
    scm_endptr = scm_ptr + entry_size;
    header[4] = '\0';

    while( ScmReadHeader(header,&length) ) {

	//
	//	SCM version
	//
	if( !memcmp(header, "VER ",4) ) {
	    if( length==2 ) {
		int v;
		v = ScmReadWord();
		// 57 - beta57
		// 59 - 1.00
		// 63 - 1.04
		// 205 - brood
		continue;
	    }
	    DebugLevel1("Wrong VER  length\n");
	}

	//
	//	SCM version additional information
	//
	if( !memcmp(header, "IVER",4) ) {
	    if( length==2 ) {
		int v;
		v = ScmReadWord();
		// 9 - obsolete, beta
		// 10 - current
		continue;
	    }
	    DebugLevel1("Wrong IVER length\n");
	}

	//
	//	Verification code
	//
	if( !memcmp(header, "VCOD",4) ) {
	    if( length==1040 ) {
		scm_ptr += 1040;
		continue;
	    }
	    DebugLevel1("Wrong VCOD length\n");
	}

	//
	//	Specifies the owner of the player
	//
	if( !memcmp(header, "IOWN",4) ) {
	    if( length==12 ) {
		scm_ptr += 12;
		continue;
	    }
	    DebugLevel1("Wrong IOWN length\n");
	}

	//
	//	Specifies the owner of the player, same as IOWN but with 0 added
	//
	if( !memcmp(header, "OWNR",4) ) {
	    if( length==12 ) {
		int i;
		int p;

		for( i=0; i<12; ++i ) {
		    p=ScmReadByte();
		    if( p==0 ) {
			info->PlayerType[i]=PlayerNobody;
		    }
		    else if( p==3 ) {
			// FIXME: should this be passive?
			info->PlayerType[i]=PlayerRescuePassive;
		    }
		    else if( p==5 ) {
			info->PlayerType[i]=PlayerComputer;
		    }
		    else if( p==6 ) {
			info->PlayerType[i]=PlayerPerson;
		    }
		    else if( p==7 ) {
			info->PlayerType[i]=PlayerNeutral;
		    }
		    else {
			DebugLevel1("Wrong OWNR type: %d\n" _C_ p);
			info->PlayerType[i]=PlayerNobody;
		    }
		}
		continue;
	    }
	    DebugLevel1("Wrong OWNR length\n");
	}

	//
	//	Terrain type
	//
	if( !memcmp(header, "ERA ",4) ) {
	    if( length==2 ) {
		int i;
		int t;
		t = ScmReadWord();
		//
		//	Look if we have this as tileset.
		//
		for( i=0; i<t && TilesetWcNames[i]; ++i ) {
		}
		if( !TilesetWcNames[i] ) {
		    t=0;
		}
		// 00 - Badlands
		// 01 - Space Platform
		// 02 - Installation
		// 03 - Ashworld
		// 04 - Jungle World
		// 05 - Desert World
		// 06 - Arctic World
		// 07 - Twilight World
		info->MapTerrainName=strdup(TilesetWcNames[t]);
		info->MapTerrain=t;
		continue;
	    }
	    DebugLevel1("Wrong ERA  length\n");
	}

	//
	//	Dimension
	//
	if( !memcmp(header, "DIM ",4) ) {
	    if( length==4 ) {
		info->MapWidth=ScmReadWord();
		info->MapHeight=ScmReadWord();
		continue;
	    }
	    DebugLevel1("Wrong DIM  length\n");
	}

	//
	//	Identifies race of each player
	//
	if( !memcmp(header, "SIDE",4) ) {
	    if( length==12 ) {
		int i;
		int v;

		// 00 - Zerg
		// 01 - Terran
		// 02 - Protoss
		// 03 - Independent
		// 04 - Neutral
		// 05 - User Select
		// 07 - Inactive
		// 10 - Human

		for( i=0; i<12; ++i ) {
		    v=ScmReadByte();
		    info->PlayerSide[i]=v;
		}
		continue;
	    }
	    DebugLevel1("Wrong SIDE length\n");
	}

	//
	//	Graphical tile map
	//
	if( !memcmp(header, "MTXM",4) ) {
	    scm_ptr += length;
	    continue;
	}

	//
	//	Player unit restrictions
	//
	if( !memcmp(header, "PUNI",4) ) {
	    if( length==228*12 + 228 + 228*12 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong PUNI length\n");
	}

	//
	//	Player upgrade restrictions
	//
	if( !memcmp(header, "UPGR",4) ) {
	    if( length==46*12 + 46*12 + 46 + 46 + 46*12 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong UPGR length\n");
	}

	//
	//	Player technology restrictions
	//
	if( !memcmp(header, "PTEC",4) ) {
	    if( length==24*12 + 24*12 + 24 + 24 + 24*12 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong PTEC length\n");
	}

	//
	//	Units
	//
	if( !memcmp(header, "UNIT",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong UNIT length\n");
	}

	//
	//	Isometric tile mapping
	//
	if( !memcmp(header, "ISOM",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong ISOM length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "TILE",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong TILE length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "DD2 ",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong DD2  length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "THG2",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong THG2 length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "MASK",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong MASK length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "STR ",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		info->Description=strdup(scm_ptr+2051);
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong STR  length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "UPRP",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong UPRP length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "UPUS",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong UPUS length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "MRGN",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong MRGN length\n");
	}

	//
	//	Triggers
	//
	if( !memcmp(header, "TRIG",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong TRIG length\n");
	}

	//
	//	Mission briefing
	//
	if( !memcmp(header, "MBRF",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong MBRF length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "SPRP",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong SPRP length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "FORC",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong FORC length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "WAV ",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong WAV  length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "UNIS",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong UNIS length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "UPGS",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong UPGS length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "TECS",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong TECS length\n");
	}

	DebugLevel2("Unsupported Section: %4.4s\n" _C_ header);
	scm_ptr += length;
    }

    free(scmdata);
    CleanMpq();

    return info;
}

/**
**	Load scm
**
**	@param scm	Name of the scm file
**	@param map	The map
*/
global void LoadScm(const char* scm,WorldMap* map)
{
    unsigned char *scmdata;
    long length;
    char header[5];
    char buf[1024];
    int width;
    int height;
    int aiopps;
    FILE *fpMpq;
    int entry_size;

    if (!map->Info) {
	map->Info = GetScmInfo(scm);
    }

    if( !(fpMpq=fopen(scm, "rb")) ) {
	fprintf(stderr,"Try ./path/name\n");
	sprintf(buf, "scm: fopen(%s)", scm);
	perror(buf);
	ExitFatal(-1);
    }

    if( MpqReadInfo(fpMpq) ) {
	fprintf(stderr,"MpqReadInfo failed\n");
	ExitFatal(-1);
    }

    ExtractMap(fpMpq, &scmdata, &entry_size);

    fclose(fpMpq);
    if( !scmdata ) {
	fprintf(stderr,"Could not extract map in %s\n",scm);
	ExitFatal(-1);
    }

    scm_ptr = scmdata;
    scm_endptr = scm_ptr + entry_size;
    header[4] = '\0';
    aiopps=width=height=0;

    while( ScmReadHeader(header,&length) ) {

	//
	//	SCM version
	//
	if( !memcmp(header, "VER ",4) ) {
	    if( length==2 ) {
		int v;
		v = ScmReadWord();
		continue;
	    }
	    DebugLevel1("Wrong VER  length\n");
	}

	//
	//	SCM version additional information
	//
	if( !memcmp(header, "IVER",4) ) {
	    if( length==2 ) {
		int v;
		v = ScmReadWord();
		continue;
	    }
	    DebugLevel1("Wrong IVER length\n");
	}

	//
	//	Verification code
	//
	if( !memcmp(header, "VCOD",4) ) {
	    if( length==1040 ) {
		scm_ptr += 1040;
		continue;
	    }
	    DebugLevel1("Wrong VCOD length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "IOWN",4) ) {
	    if( length==12 ) {
		scm_ptr += 12;
		continue;
	    }
	    DebugLevel1("Wrong IOWN length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "OWNR",4) ) {
	    if( length==12 ) {
		int i;
		int p;

		for( i=0; i<12; ++i ) {
		    p=ScmReadByte();
		    if( p==0 ) {
			p=PlayerNobody;
		    }
		    else if( p==3 ) {
			p=PlayerRescuePassive;
		    }
		    else if( p==5 ) {
			p=PlayerComputer;
		    }
		    else if( p==6 ) {
			p=PlayerPerson;
		    }
		    else if( p==7 ) {
			p=PlayerNeutral;
		    }
		    else {
			DebugLevel1("Wrong OWNR type: %d\n" _C_ p);
			p=PlayerNobody;
		    }

		    // Single player games only:
		    // ARI: FIXME: convert to a preset array to share with network game code
		    if (GameSettings.Opponents != SettingsPresetMapDefault) {
			if (p == PlayerComputer) {
			    if (aiopps < GameSettings.Opponents) {
				aiopps++;
			    } else {
				p = PlayerNobody;
			    }
			}
		    }
		    // Network games only:
		    if (GameSettings.Presets[i].Type != SettingsPresetMapDefault) {
			p = GameSettings.Presets[i].Type;
		    }
		    if( i==11 ) {
			p = PlayerNeutral;
		    }
		    CreatePlayer(p);
		    PlayerSetAiNum(&Players[i], 0);
		}
		for( i=12; i<PlayerMax; ++i ) {
		    p=PlayerNobody;
		    CreatePlayer(p);
		    PlayerSetAiNum(&Players[i], 0);
		}
		continue;
	    }
	    DebugLevel1("Wrong OWNR length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "ERA ",4) ) {
	    if( length==2 ) {
		int t;
		int i;

		t = ScmReadWord();
		if (GameSettings.Terrain != SettingsPresetMapDefault) {
		    t = GameSettings.Terrain;
		}
		if( map->TerrainName ) {
		    free(map->TerrainName);
		}
		//
		//	Look if we have this as tileset.
		//
		for( i=0; i<t && TilesetWcNames[i]; ++i ) {
		}
		if( !TilesetWcNames[i] ) {
		    t=0;
		}
		map->TerrainName=strdup(TilesetWcNames[t]);
		map->Terrain=t;
		continue;
	    }
	    DebugLevel1("Wrong ERA  length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "DIM ",4) ) {
	    if( length==4 ) {
		width=ScmReadWord();
		height=ScmReadWord();

		DebugLevel2("\tMap %d x %d\n" _C_ width _C_ height);

		if( !map->Fields ) {
		    map->Width=width;
		    map->Height=height;

		    map->Fields=calloc(width*height,sizeof(*map->Fields));
		    if( !map->Fields ) {
			perror("calloc()");
			ExitFatal(-1);
		    }
		    TheMap.Visible[0]=calloc(TheMap.Width*TheMap.Height/8,1);
		    if( !TheMap.Visible[0] ) {
			perror("calloc()");
			ExitFatal(-1);
		    }
		    InitUnitCache();
		    // FIXME: this should be CreateMap or InitMap?
		} else {			// FIXME: should do some checks here!
		    DebugLevel0Fn("Warning: Fields already allocated\n");
		}
		continue;
	    }
	    DebugLevel1("Wrong DIM  length\n");
	}

	//
	//	Identifies race of each player
	//
	if( !memcmp(header, "SIDE",4) ) {
	    if( length==12 ) {
		int i;
		int v;

		for( i=0; i<12; ++i ) {
		    v=ScmReadByte();
		    switch( v ) {
			case 0: // Zerg
			case 1: // Terran
			    break;
			case 2:	// Protoss
			    // FIXME: Add more races
			    v=0;
			    break;
			case 4: // Neutral
			    v=PlayerRaceNeutral;
			    break;
			case 5: // User select
			    v=0;
			    break;
			case 7: // Inactive
			    v=0;
			    break;
			default:
			    DebugLevel1("Unknown race %d\n" _C_ v);
			    v=PlayerRaceNeutral;
			    break;
		    }
		    if (GameSettings.Presets[i].Race == SettingsPresetMapDefault) {
			PlayerSetSide(&Players[i],v);
		    } else {
			PlayerSetSide(&Players[i],GameSettings.Presets[i].Race);
		    }
		}
		continue;
	    }
	    DebugLevel1("Wrong SIDE length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "MTXM",4) ) {
	    if( length==width*height*2 ) {
		ScmConvertMTXM((unsigned short*)scm_ptr,width,height,map);
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong MTXM length\n");
	}

	//
	//	Player unit restrictions
	//
	if( !memcmp(header, "PUNI",4) ) {
	    if( length==228*12 + 228 + 228*12 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong PUNI length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "UPGR",4) ) {
	    // 1748
//	    if( length==45*12 + 45*12 + 45 + 45 + 45*12 ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong UPGR length\n");
	}

	//
	//	Player technology restrictions
	//
	if( !memcmp(header, "PTEC",4) ) {
	    if( length==24*12 + 24*12 + 24 + 24 + 24*12 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong PTEC length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "UNIT",4) ) {
	    int i;

//	    if( length== ) {
	    if( length%36==0 ) {
		int x;
		int y;
		int t;
		int f1;
		int f2;
		int o;
		int hpp;
		int shp;
		int ep;
		int res;
		int nh;
		int st;
		int s;
		int v;
		Unit* unit;
		UnitType* type;

		while( length>0 ) {
		    scm_ptr += 4;	// unknown
		    x = ScmReadWord();	// x coordinate
		    y = ScmReadWord();	// y coordinate
		    t = ScmReadWord();	// unit type
		    scm_ptr += 2;	// unknown
		    f1 = ScmReadWord();	// special properties flag
		    f2 = ScmReadWord();	// valid elements
		    o = ScmReadByte();	// owner
		    hpp = ScmReadByte();// hit point %
		    shp = ScmReadByte();// shield point %
		    ep = ScmReadByte();	// energy point %
		    res = ScmReadDWord(); // resource amount
		    nh = ScmReadWord();	// num units in hanger
		    st = ScmReadWord();	// state flags
		    scm_ptr += 8;	// unknown

		    length -= 36;

		    // FIXME: remove
		    type = UnitTypeByWcNum(t);
		    x = (x - 32*type->TileWidth/2) / 32;
		    y = (y - 32*type->TileHeight/2) / 32;

		    if( t==SC_StartLocation ) {
			Players[o].StartX=MapOffsetX+x;
			Players[o].StartY=MapOffsetY+y;
			if (GameSettings.NumUnits == SettingsNumUnits1
				&& Players[o].Type != PlayerNobody) {
			    if (t == SC_StartLocation) {
				if( Players[o].Race==0 ) {
				    t=SC_ZergDrone;
				} else if( Players[o].Race==1 ) {
				    t=SC_TerranSCV;
				} else {
				    t=SC_ProtossProbe;
				}
			    }
			    v = 1;
			    goto pawn;
			}
		    } else {
			if ( GameSettings.NumUnits == SettingsNumUnitsMapDefault ||
			    SC_IsUnitMineral(t) || t == SC_UnitGeyser ) {
pawn:
			    if( !SC_IsUnitMineral(t) && t != SC_UnitGeyser ) {
				if( (s = GameSettings.Presets[o].Race) != SettingsPresetMapDefault ) {
#if 0
				    // FIXME: change races
				    if( s == PlayerRaceHuman && (t & 1) == 1 ) {
					t--;
				    }
				    if( s == PlayerRaceOrc && (t & 1) == 0 ) {
					t++;
				    }
				    // FIXME: ARI: This is hard-coded WAR2 ... also: support more races?
#endif
				}
			    }
			    if( Players[o].Type != PlayerNobody ) {
				unit=MakeUnitAndPlace(MapOffsetX+x,MapOffsetY+y
					,UnitTypeByWcNum(t),&Players[o]);
				if( unit->Type->GoldMine || unit->Type->GivesOil
					|| unit->Type->OilPatch ) {
#if 0
				    DebugCheck( !v );
				    unit->Value=v;
#endif
				} else {	
				    // active/inactive AI units!!
				    // Johns: it is better to have active buildings
				    if( !unit->Type->Building ) {
#if 0
					unit->Active=!v;
#endif
				    }
				}
				UpdateForNewUnit(unit,0);
			    }
			}
		    }
		}

		for( i=0; i<12; ++i ) {
		    if( Players[i].Type==PlayerPerson || Players[i].Type==PlayerComputer ) {
			if( Players[i].TotalUnits == 0 ) {
			    // If the player has no units use 4 peasants and a town hall as default
			    int j;
			    int t1;
			    Unit* unit;
			    UnitType* type;
			    if( Players[i].Race==0 ) {
				t1=SC_ZergDrone;
				type=UnitTypeByWcNum(SC_ZergHatchery);
			    } else if( Players[i].Race==1 ) {
				t1=SC_TerranSCV;
				type=UnitTypeByWcNum(SC_TerranCommandCenter);
			    } else {
				t1=SC_ProtossProbe;
				type=UnitTypeByWcNum(SC_ProtossNexus);
			    }
			    unit=MakeUnitAndPlace(Players[i].StartX,Players[i].StartY,
				type,&Players[i]);
			    UpdateForNewUnit(unit,0);
			    for( j=0; j<4; ++j ) {
				unit=MakeUnit(UnitTypeByWcNum(t1),&Players[i]);
				unit->X=Players[i].StartX;
				unit->Y=Players[i].StartY;
				DropOutOnSide(unit,LookingS,type->TileWidth,type->TileHeight);
				UpdateForNewUnit(unit,0);
			    }
			}
		    }
		}

		continue;
	    }
	    DebugLevel1("Wrong UNIT length\n");
	}

	//
	//	Isometric tile mapping
	//
	if( !memcmp(header, "ISOM",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong ISOM length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "TILE",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong TILE length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "DD2 ",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong DD2  length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "THG2",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong THG2 length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "MASK",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong MASK length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "STR ",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		strcpy(map->Description, scm_ptr+2051);
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong STR  length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "UPRP",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong UPRP length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "UPUS",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong UPUS length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "MRGN",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong MRGN length\n");
	}

	//
	//	Triggers
	//
	if( !memcmp(header, "TRIG",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong TRIG length\n");
	}

	//
	//	Mission briefing
	//
	if( !memcmp(header, "MBRF",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong MBRF length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "SPRP",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong SPRP length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "FORC",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong FORC length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "WAV ",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong WAV  length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "UNIS",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong UNIS length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "UPGS",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong UPGS length\n");
	}

	//
	//	
	//
	if( !memcmp(header, "TECS",4) ) {
//	    if( length== ) {
	    if( 1 ) {
		scm_ptr += length;
		continue;
	    }
	    DebugLevel1("Wrong TECS length\n");
	}

	DebugLevel2("Unsupported Section: %4.4s\n" _C_ header);
	scm_ptr += length;
    }

    MapOffsetX+=width;
    if( MapOffsetX>=map->Width ) {
	MapOffsetX=0;
	MapOffsetY+=height;
    }

    CleanMpq();
}

/**
**	Clean scm module.
*/
global void CleanScm(void)
{
    MapOffsetX=MapOffsetY=0;
}
