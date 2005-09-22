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
/**@name unittype.cpp - The unit types. */
//
//      (c) Copyright 1998-2005 by Lutz Sammer and Jimmy Salmon
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
#include <string.h>
#include <ctype.h>

#include "stratagus.h"

#include <string>
#include <map>

#include "video.h"
#include "tileset.h"
#include "map.h"
#include "sound_id.h"
#include "unitsound.h"
#include "construct.h"
#include "unittype.h"
#include "animation.h"
#include "player.h"
#include "missile.h"
#include "script.h"
#include "spells.h"

#include "util.h"

#include "myendian.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CUnitType *> UnitTypes;   /// unit-types definition
std::map<std::string, CUnitType *> UnitTypeMap;

/**
**  Next unit type are used hardcoded in the source.
**
**  @todo find a way to make it configurable!
*/
CUnitType* UnitTypeHumanWall;       /// Human wall
CUnitType* UnitTypeOrcWall;         /// Orc wall

/**
**  Default resources for a new player.
*/
int DefaultResources[MaxCosts];

/**
**  Default resources for a new player with low resources.
*/
int DefaultResourcesLow[MaxCosts];

/**
**  Default resources for a new player with mid resources.
*/
int DefaultResourcesMedium[MaxCosts];

/**
**  Default resources for a new player with high resources.
*/
int DefaultResourcesHigh[MaxCosts];

/**
**  Default incomes for a new player.
*/
int DefaultIncomes[MaxCosts];

/**
**  Default action for the resources.
*/
char* DefaultActions[MaxCosts];

/**
**  Default names for the resources.
*/
char* DefaultResourceNames[MaxCosts];

/**
**  Default amounts for the resources.
*/
int DefaultResourceAmounts[MaxCosts];

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Update the player stats for changed unit types.
**  @param reset indicates wether default value should be set to each stat (level, upgrades)
*/
void UpdateStats(int reset)
{
	CUnitType* type;
	UnitStats* stats;
	int player;
	unsigned i;

	//
	//  Update players stats
	//
	for (std::vector<CUnitType *>::size_type j = 0; j < UnitTypes.size(); ++j) {
		type = UnitTypes[j];
		if (reset) {
			// LUDO : FIXME : reset loading of player stats !
			for (player = 0; player < PlayerMax; ++player) {
				stats = &type->Stats[player];
				for (i = 0; i < MaxCosts; ++i) {
					stats->Costs[i] = type->_Costs[i];
				}
				if (!stats->Variables) {
					stats->Variables = (VariableType*)calloc(UnitTypeVar.NumberVariable, sizeof(*stats->Variables));
				}
				for (i = 0; (int) i < UnitTypeVar.NumberVariable; i++) {
					stats->Variables[i] = type->Variable[i];
				}
			}
		}

		//
		//  As side effect we calculate the movement flags/mask here.
		//
		switch (type->UnitType) {
			case UnitTypeLand:                              // on land
				type->MovementMask =
					MapFieldLandUnit |
					MapFieldSeaUnit |
					MapFieldBuilding | // already occuppied
					MapFieldCoastAllowed |
					MapFieldWaterAllowed | // can't move on this
					MapFieldUnpassable;
				break;
			case UnitTypeFly:                               // in air
				type->MovementMask =
					MapFieldAirUnit; // already occuppied
				break;
			case UnitTypeNaval:                             // on water
				if (type->CanTransport) {
					type->MovementMask =
						MapFieldLandUnit |
						MapFieldSeaUnit |
						MapFieldBuilding | // already occuppied
						MapFieldLandAllowed; // can't move on this
					// Johns: MapFieldUnpassable only for land units?
				} else {
					type->MovementMask =
						MapFieldLandUnit |
						MapFieldSeaUnit |
						MapFieldBuilding | // already occuppied
						MapFieldCoastAllowed |
						MapFieldLandAllowed | // can't move on this
						MapFieldUnpassable;
				}
				break;
			default:
				DebugPrint("Where moves this unit?\n");
				type->MovementMask = 0;
				break;
		}
		if (type->Building || type->ShoreBuilding) {
			// Shore building is something special.
			if (type->ShoreBuilding) {
				type->MovementMask =
					MapFieldLandUnit |
					MapFieldSeaUnit |
					MapFieldBuilding | // already occuppied
					MapFieldLandAllowed; // can't build on this
			}
			type->MovementMask |= MapFieldNoBuilding;
			//
			// A little chaos, buildings without HP can be entered.
			// The oil-patch is a very special case.
			//
			if (type->Variable[HP_INDEX].Max) {
				type->FieldFlags = MapFieldBuilding;
			} else {
				type->FieldFlags = MapFieldNoBuilding;
			}
		} else switch (type->UnitType) {
			case UnitTypeLand: // on land
				type->FieldFlags = MapFieldLandUnit;
				break;
			case UnitTypeFly: // in air
				type->FieldFlags = MapFieldAirUnit;
				break;
			case UnitTypeNaval: // on water
				type->FieldFlags = MapFieldSeaUnit;
				break;
			default:
				DebugPrint("Where moves this unit?\n");
				type->FieldFlags = 0;
				break;
		}
	}
}

/**
**  Get the animations structure by ident.
**
**  @param ident  Identifier for the animation.
**
**  @return  Pointer to the animation structure.
*/
Animations *AnimationsByIdent(const char *ident)
{
	return AnimationMap[ident];
}

/**
**  Save state of an unit-stats to file.
**
**  @param stats  Unit-stats to save.
**  @param ident  Unit-type ident.
**  @param plynr  Player number.
**  @param file   Output file.
*/
static void SaveUnitStats(const UnitStats* stats, const char* ident, int plynr,
	CFile* file)
{
	int j;

	Assert(plynr < PlayerMax);
	file->printf("DefineUnitStats(\"%s\", %d,\n  ", ident, plynr);
	for (j = 0; j < UnitTypeVar.NumberVariable; ++j) {
		file->printf("\"%s\", {Value = %d, Max = %d, Increase = %d%s},\n  ",
			UnitTypeVar.VariableName[j], stats->Variables[j].Value,
			stats->Variables[j].Max, stats->Variables[j].Increase,
			stats->Variables[j].Enable ? ", Enable = true" : "");
	}
	file->printf("\"costs\", {");
	for (j = 0; j < MaxCosts; ++j) {
		if (j) {
			file->printf(" ");
		}
		file->printf("\"%s\", %d,", DefaultResourceNames[j], stats->Costs[j]);
	}

	file->printf("})\n");
}

/**
**  Save state of the unit-type table to file.
**
**  @param file  Output file.
*/
void SaveUnitTypes(CFile* file)
{
	int j;

	file->printf("\n--- -----------------------------------------\n");
	file->printf("--- MODULE: unittypes $Id$\n\n");

	// Save all stats
	for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
		file->printf("\n");
		for (j = 0; j < PlayerMax; ++j) {
			if (Players[j].Type != PlayerNobody) {
				SaveUnitStats(&UnitTypes[i]->Stats[j], UnitTypes[i]->Ident, j, file);
			}
		}
	}
}

/**
**  Find unit-type by identifier.
**
**  @param ident  The unit-type identifier.
**
**  @return       Unit-type pointer.
*/
CUnitType *UnitTypeByIdent(const char *ident)
{
	return UnitTypeMap[ident];
}

/**
**  Allocate an empty unit-type slot.
**
**  @param ident  Identifier to identify the slot (malloced by caller!).
**
**  @return       New allocated (zeroed) unit-type pointer.
*/
CUnitType *NewUnitTypeSlot(char *ident)
{
	CUnitType *type;

	type = new CUnitType;
	if (!type) {
		fprintf(stderr, "Out of memory\n");
		ExitFatal(-1);
	}
	memset(type, 0, sizeof(*type));
	type->Slot = UnitTypes.size();
	type->Ident = ident;
	type->BoolFlag = (unsigned char*)calloc(UnitTypeVar.NumberBoolFlag, sizeof(*type->BoolFlag));
	type->CanTargetFlag = (unsigned char*)calloc(UnitTypeVar.NumberBoolFlag, sizeof(*type->CanTargetFlag));
	type->Variable = (VariableType*)calloc(UnitTypeVar.NumberVariable, sizeof(*type->Variable));
	memcpy(type->Variable, UnitTypeVar.Variable,
		UnitTypeVar.NumberVariable * sizeof(*type->Variable));

	UnitTypes.push_back(type);
	UnitTypeMap[type->Ident] = type;
	return type;
}

/**
**  Draw unit-type on map.
**
**  @param type    Unit-type pointer.
**  @param sprite  Sprite to use for drawing
**  @param player  Player number for color substitution.
**  @param frame   Animation frame of unit-type.
**  @param x       Screen X pixel postion to draw unit-type.
**  @param y       Screen Y pixel postion to draw unit-type.
**
**  @todo  Do screen position caculation in high level.
**         Better way to handle in x mirrored sprites.
*/
void DrawUnitType(const CUnitType* type, Graphic* sprite, int player, int frame,
	int x, int y)
{
	// FIXME: move this calculation to high level.
	x -= (type->Width - type->TileWidth * TileSizeX) / 2;
	y -= (type->Height - type->TileHeight * TileSizeY) / 2;
	x += type->OffsetX;
	y += type->OffsetY;

	if (type->Flip) {
		if (frame < 0) {
			sprite->DrawPlayerColorFrameClipX(player, -frame - 1, x, y);
		} else {
			sprite->DrawPlayerColorFrameClip(player, frame, x, y);
		}
	} else {
		int row;

		row = type->NumDirections / 2 + 1;
		if (frame < 0) {
			frame = ((-frame - 1) / row) * type->NumDirections + type->NumDirections - (-frame - 1) % row;
		} else {
			frame = (frame / row) * type->NumDirections + frame % row;
		}
		sprite->DrawPlayerColorFrameClip(player, frame, x, y);
	}
}

/**
**  Get the still animation frame
*/
static int GetStillFrame(CUnitType* type)
{
	Animation* anim;

	anim = type->Animations->Still;
	while (anim) {
		if (anim->Type == AnimationFrame) {
			// Use the frame facing down
			return anim->D.Frame.Frame + type->NumDirections / 2;
		} else if (anim->Type == AnimationExactFrame) {
			return anim->D.Frame.Frame;
		}
		anim = anim->Next;
	}
	return type->NumDirections / 2;
}

/**
**  Init unit types.
*/
void InitUnitTypes(int reset_player_stats)
{
	for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
		Assert(UnitTypes[i]->Slot == (int)i);

		//  Add idents to hash.
		UnitTypeMap[UnitTypes[i]->Ident] = UnitTypes[i];

		// Determine still frame
		UnitTypes[i]->StillFrame = GetStillFrame(UnitTypes[i]);
	}

	// LUDO : called after game is loaded -> don't reset stats !
	UpdateStats(reset_player_stats); // Calculate the stats

	//
	// Setup hardcoded unit types. FIXME: should be moved to some configs.
	//
	UnitTypeHumanWall = UnitTypeByIdent("unit-human-wall");
	UnitTypeOrcWall = UnitTypeByIdent("unit-orc-wall");
}

/**
**  Loads the Sprite for a unit type
**
**  @param type  type of unit to load
*/
void LoadUnitTypeSprite(CUnitType* type)
{
	ResourceInfo* resinfo;
	int i;

	if (type->ShadowFile) {
		type->ShadowSprite = ForceNewGraphic(type->ShadowFile, type->ShadowWidth,
			type->ShadowHeight);
		type->ShadowSprite->Load();
		if (type->Flip) {
			type->ShadowSprite->Flip();
		}
		type->ShadowSprite->MakeShadow();
	}

	if (type->Harvester) {
		for (i = 0; i < MaxCosts; ++i) {
			if ((resinfo = type->ResInfo[i])) {
				if (resinfo->FileWhenLoaded) {
					resinfo->SpriteWhenLoaded = NewGraphic(resinfo->FileWhenLoaded,
						type->Width, type->Height);
					resinfo->SpriteWhenLoaded->Load();
					if (type->Flip) {
						resinfo->SpriteWhenLoaded->Flip();
					}
				}
				if (resinfo->FileWhenEmpty) {
					resinfo->SpriteWhenEmpty = NewGraphic(resinfo->FileWhenEmpty,
						type->Width, type->Height);
					resinfo->SpriteWhenEmpty->Load();
					if (type->Flip) {
						resinfo->SpriteWhenEmpty->Flip();
					}
				}
			}
		}
	}

	if (type->File) {
		type->Sprite = NewGraphic(type->File, type->Width, type->Height);
		type->Sprite->Load();
		if (type->Flip) {
			type->Sprite->Flip();
		}
	}

#ifdef USE_MNG
	if (type->Portrait.Num) {
		for (i = 0; i < type->Portrait.Num; ++i) {
			type->Portrait.Mngs[i] = new Mng;
			type->Portrait.Mngs[i]->Load(type->Portrait.Files[i]);
		}
		// FIXME: should be configurable
		type->Portrait.CurrMng = 0;
		type->Portrait.NumIterations = SyncRand() % 16 + 1;
	}
#endif
}

/**
** Load the graphics for the unit-types.
*/
void LoadUnitTypes(void)
{
	CUnitType* type;

	for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
		type = UnitTypes[i];

		//
		// Lookup icons.
		//
		type->Icon.Icon = IconByIdent(type->Icon.Name);
		if (!type->Icon.Icon) {
			printf("Can't find icon %s\n", type->Icon.Name);
			ExitFatal(-1);
		}
		//
		// Lookup missiles.
		//
		type->Missile.Missile = MissileTypeByIdent(type->Missile.Name);
		if (type->Explosion.Name) {
			type->Explosion.Missile = MissileTypeByIdent(type->Explosion.Name);
		}
		//
		// Lookup corpse.
		//
		if (type->CorpseName) {
			type->CorpseType = UnitTypeByIdent(type->CorpseName);
		}

		//
		// Lookup BuildingTypes
		if (type->BuildingRules) {
			int x;
			BuildRestriction* b;

			x = 0;
			while (type->BuildingRules[x] != NULL) {
				b = type->BuildingRules[x];
				while (b != NULL) {
					if (b->RestrictType == RestrictAddOn) {
						b->Data.AddOn.Parent = UnitTypeByIdent(b->Data.AddOn.ParentName);
					} else if (b->RestrictType == RestrictOnTop) {
						b->Data.OnTop.Parent = UnitTypeByIdent(b->Data.OnTop.ParentName);
					} else if (b->RestrictType == RestrictDistance) {
						b->Data.Distance.RestrictType = UnitTypeByIdent(b->Data.Distance.RestrictTypeName);
					}
					b = b->Next;
				}
				++x;
			}
		}

		//
		// Load Sprite
		//
#ifndef DYNAMIC_LOAD
		if (!type->Sprite) {
			ShowLoadProgress("Unit \"%s\"", type->Name);
			LoadUnitTypeSprite(type);
		}
#endif
		// FIXME: should i copy the animations of same graphics?
	}
}

/**
**  Clean animation
*/
static void CleanAnimation(Animation* anim)
{
	int i;
	Animation* ptr;

	ptr = anim;
	while (ptr->Type) {
		if (ptr->Type == AnimationSound) {
			free(ptr->D.Sound.Name);
		} else if (ptr->Type == AnimationRandomSound) {
			for (i = 0; i < ptr->D.RandomSound.NumSounds; ++i) {
				free(ptr->D.RandomSound.Name[i]);
			}
			free(ptr->D.RandomSound.Name);
			free(ptr->D.RandomSound.Sound);
		}
		++ptr;
	}
	free(anim);
}

/**
**  Cleanup the unit-type module.
*/
void CleanUnitTypes(void)
{
	CUnitType *type;
	int j;
	int res;

	DebugPrint("FIXME: icon, sounds not freed.\n");

	// FIXME: scheme contains references on this structure.
	// Clean all animations.
	for (j = 0; j < NumAnimations; ++j) {
		CleanAnimation(AnimationsArray[j]);
	}
	NumAnimations = 0;

	// Clean all unit-types

	for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
		type = UnitTypes[i];

		Assert(type->Ident);
		free(type->Ident);
		Assert(type->Name);
		free(type->Name);

		free(type->Variable);
		free(type->BoolFlag);
		free(type->CanTargetFlag);
		free(type->CanTransport);

		for (j = 0; j < PlayerMax; j++) {
			free(type->Stats[j].Variables);
		}

		// Free Building Restrictions if there are any
		if (type->BuildingRules) {
			int x;
			BuildRestriction* b;
			BuildRestriction* f;

			x = 0;
			while (type->BuildingRules[x] != NULL) {
				b = type->BuildingRules[x];
				while (b != NULL) {
					f = b;
					b = b->Next;
					if (f->RestrictType == RestrictAddOn) {
						free(f->Data.AddOn.ParentName);
					} else if (f->RestrictType == RestrictOnTop) {
						free(f->Data.OnTop.ParentName);
					} else if (f->RestrictType == RestrictDistance) {
						free(f->Data.Distance.RestrictTypeName);
					}
					free(f);
				}
				++x;
			}
			free(type->BuildingRules);
		}
		if (type->File) {
			free(type->File);
		}
		if (type->ShadowFile) {
			free(type->ShadowFile);
		}
		if (type->Icon.Name) {
			free(type->Icon.Name);
		}
		if (type->Missile.Name) {
			free(type->Missile.Name);
		}
		if (type->Explosion.Name) {
			free(type->Explosion.Name);
		}
		if (type->CorpseName) {
			free(type->CorpseName);
		}
		if (type->CanCastSpell) {
			free(type->CanCastSpell);
		}
		free(type->AutoCastActive);

		for (res = 0; res < MaxCosts; ++res) {
			if (type->ResInfo[res]) {
				if (type->ResInfo[res]->SpriteWhenLoaded) {
					FreeGraphic(type->ResInfo[res]->SpriteWhenLoaded);
				}
				if (type->ResInfo[res]->SpriteWhenEmpty) {
					FreeGraphic(type->ResInfo[res]->SpriteWhenEmpty);
				}
				if (type->ResInfo[res]->FileWhenEmpty) {
					free(type->ResInfo[res]->FileWhenEmpty);
				}
				if (type->ResInfo[res]->FileWhenLoaded) {
					free(type->ResInfo[res]->FileWhenLoaded);
				}
				free(type->ResInfo[res]);
			}
		}

		//
		// FIXME: Sounds can't be freed, they still stuck in sound hash.
		//
		if (type->Sound.Selected.Name) {
			free(type->Sound.Selected.Name);
		}
		if (type->Sound.Acknowledgement.Name) {
			free(type->Sound.Acknowledgement.Name);
		}
		if (type->Sound.Ready.Name) {
			free(type->Sound.Ready.Name);
		}
		if (type->Sound.Repair.Name) {
			free(type->Sound.Repair.Name);
		}
		for (j = 0; j < MaxCosts; ++j) {
			if (type->Sound.Harvest[j].Name) {
				free(type->Sound.Harvest[j].Name);
			}
		}
		if (type->Sound.Help.Name) {
			free(type->Sound.Help.Name);
		}
		if (type->Sound.Dead.Name) {
			free(type->Sound.Dead.Name);
		}

		FreeGraphic(type->Sprite);
#ifdef USE_MNG
		if (type->Portrait.Num) {
			for (j = 0; j < type->Portrait.Num; ++j) {
				delete type->Portrait.Mngs[j];
				free(type->Portrait.Files[j]);
			}
			free(type->Portrait.Mngs);
			free(type->Portrait.Files);
		}
#endif

		delete UnitTypes[i];
	}
	UnitTypes.clear();
	UnitTypeMap.clear();

	for (j = 0; j < UnitTypeVar.NumberBoolFlag; ++j) { // User defined flags
		free(UnitTypeVar.BoolFlagName[j]);
	}
	for (j = 0; j < UnitTypeVar.NumberVariable; ++j) { // User defined variables
		free(UnitTypeVar.VariableName[j]);
	}
	free(UnitTypeVar.BoolFlagName);
	free(UnitTypeVar.VariableName);
	free(UnitTypeVar.Variable);
	free(UnitTypeVar.DecoVar);
	memset(&UnitTypeVar, 0, sizeof (UnitTypeVar));

	//
	// Clean hardcoded unit types.
	//
	UnitTypeHumanWall = NULL;
	UnitTypeOrcWall = NULL;
}

//@}
