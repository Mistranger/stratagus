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
/**@name unittype.h	-	The unit-types headerfile. */
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

#ifndef __UNITTYPE_H__
#define __UNITTYPE_H__

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include "video.h"
#include "icons.h"
#include "sound_id.h"
#include "unitsound.h"
#include "upgrade_structs.h"
#include "construct.h"

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

/**
**	Defines the animation for different actions.
*/
typedef struct _animation_ {
    char	Flags;			/// Flags for actions
    char	Pixel;			/// Change the position in pixels
    char	Sleep;			/// Wait for next animation
    char	Frame;			/// Sprite-frame to display
} Animation;

#define AnimationRestart	1	/// restart animation
#define AnimationReset		2	/// animation could here be aborted
#define AnimationSound		4	/// play sound
#define AnimationMissile	8	/// fire projectil
#define AnimationEnd		0x80	/// animation end in memory

/**
**	Define all animations scripts of an unittype.
*/
typedef struct __animations__ {
    Animation*	Still;			/// Standing still
    Animation*	Move;			/// Unit moving
    Animation*	Attack;			/// Unit attacking/working
    Animation*	Die;			/// Unit dieing
    Animation**	Extend;			/// For future extensions
} Animations;

#ifndef __STRUCT_MISSILETYPE__
#define __STRUCT_MISSILETYPE__
typedef struct _missile_type_ MissileType;         /// missile-type typedef
#endif

/**
**      Missile type definition (used in config tables)
**
**	@todo Shouldn't I move this into missle.h?
*/
typedef struct _missile_config_ {
    char*	Name;			/// config missile name
    MissileType*Missile;		/// identifier to use to run time
} MissileConfig;

/**
**	Typedef of base structure of unit-type
*/
typedef struct _unit_type_ UnitType;

/**
**	Base structure of unit-type
**
**	Contains all informations for a special unit-type.
**	Appearance, features...
*/
struct _unit_type_ {
    const void*	OType;			/// Object type (future extensions)

    char*	Ident;			/// identifier
    char*	Name;			/// unit name shown from the engine
    char*	SameSprite;		/// unit-type shared sprites
    char*	File[4/*TilesetMax*/];	/// sprite files

    int		Width;			/// sprite width
    int		Height;			/// sprite height

    Animations*	Animations;		/// animation scripts

    IconConfig	Icon;			/// icon to display for this unit
    MissileConfig Missile;		/// missile weapon

    char*	CorpseName;		/// corpse type name
    UnitType*	CorpseType;		/// corpse unit-type
    int		CorpseScript;		/// corpse script start

    int		_Speed;			/// movement speed

// this is taken from the UDTA section
    Construction*Construction;		/// what is shown in construction phase
    int		_SightRange;		/// sight range
    unsigned	_HitPoints;		/// maximum hit points
    // FIXME: only flag
    int		Magic;			/// Unit can cast spells

    int		_Costs[MaxCosts];	/// how many resources needed

    int		TileWidth;		/// tile size on map width
    int		TileHeight;		/// tile size on map height
    int		BoxWidth;		/// selected box size width
    int		BoxHeight;		/// selected box size height
    int		MinAttackRange;		/// minimal attack range
    int		_AttackRange;		/// how far can the unit attack
    int		ReactRangeComputer;	/// reacts on enemy for computer
    int		ReactRangePerson;	/// reacts on enemy for person player
    int		_Armor;			/// amount of armor this unit has
    int		Priority;		/// Priority value / AI Treatment
    int		_BasicDamage;		/// Basic damage dealt
    int		_PiercingDamage;	/// Piercing damage dealt
    int		WeaponsUpgradable;	/// Weapons could be upgraded
    int		ArmorUpgradable;	/// Armor could be upgraded
    //int	MissileWeapon;		/// Missile type used when it attacks
    int		UnitType;		/// land / fly / naval
    // FIXME: original only visual effect, we do more with this!
#define UnitTypeLand	0			/// Unit lives on land
#define UnitTypeFly	1			/// Unit lives in air
#define UnitTypeNaval	2			/// Unit lives on water
    int		DecayRate;		/// Decay rate in 1/6 seconds
    // FIXME: not used
    int		AnnoyComputerFactor;	/// How much this annoys the computer
    int		MouseAction;		/// Right click action
#define MouseActionNone		0		/// Nothing
#define MouseActionAttack	1		/// Attack
#define MouseActionMove		2		/// Move
#define MouseActionHarvest	3		/// Harvest or mine gold
#define MouseActionHaulOil	4		/// Haul oil
#define MouseActionDemolish	5		/// Demolish
#define MouseActionSail		6		/// Sail
    int		Points;			/// How many points you get for unit
    int		CanTarget;		/// which units can it attack
#define CanTargetLand	1			/// can attack land units
#define CanTargetSea	2			/// can attack sea units
#define CanTargetAir	4			/// can attack air units

    unsigned LandUnit : 1;		/// Land animated
    unsigned AirUnit : 1;		/// Air animated
    unsigned SeaUnit : 1;		/// Sea animated
    unsigned ExplodeWhenKilled : 1;	/// Death explosion animated
    unsigned Critter : 1;		/// Unit is controlled by nobody
    unsigned Building : 1;		/// Building
    unsigned Submarine : 1;		/// Is only visible by CanSeeSubmarine
    unsigned CanSeeSubmarine : 1;	/// Only this units can see Submarine
    unsigned CowerWorker : 1;		/// Is a worker, runs away if attcked
    unsigned Tanker : 1;		/// FIXME: used? Can transport oil
    unsigned Transporter : 1;		/// can transport units
    unsigned GivesOil : 1;		/// We get here oil
    unsigned StoresGold : 1;		/// We can store oil/gold/wood here
    unsigned Vanishes : 1;		/// Corpes & destroyed places
    unsigned GroundAttack : 1;		/// Can do command ground attack
    unsigned IsUndead : 1;		/// FIXME: docu
    unsigned ShoreBuilding : 1;		/// FIXME: docu
    unsigned CanCastSpell : 1;		/// FIXME: docu
    unsigned StoresWood : 1;		/// We can store wood here
    unsigned CanAttack : 1;		/// FIXME: docu
    unsigned Tower : 1;			/// FIXME: docu
    unsigned OilPatch : 1;		/// FIXME: docu
    unsigned GoldMine : 1;		/// FIXME: docu
    unsigned Hero : 1;			/// FIXME: docu
    unsigned StoresOil : 1;		/// We can store oil here
    unsigned Volatile : 1;		/// invisiblity/unholy armor kills unit
    unsigned CowerMage : 1;		/// FIXME: docu
    unsigned Organic : 1;		/// organic

    unsigned SelectableByRectangle : 1;	/// selectable with mouse rectangle

    UnitSound Sound;			/// sounds for events
    // FIXME: temporary solution
    WeaponSound Weapon;                 /// currently sound for weapon

// --- FILLED UP ---

    unsigned	Supply;			/// Food supply
    unsigned	Demand;			/// Food demand

	// FIXME: This stats should? be moved into the player struct
    UnitStats Stats[PlayerMax];		/// Unit status for each player

	// FIXME: Should us a general name f.e. Slot here?
    unsigned	Type;			/// Type as number

    void*	Property;		/// CCL property storage

    Graphic*	Sprite;			/// sprite images
};

    // FIXME: ARI: should be dynamic (ccl..)
    /// How many unit-types are currently supported
#define UnitTypeMax	0xFF

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern const char UnitTypeType[];	/// unit-type type
extern UnitType* UnitTypes;		/// all unit-types
extern int NumUnitTypes;		/// number of unit-types made

extern UnitType*UnitTypeGoldMine;	/// Gold-mine unit-type pointer
extern UnitType*UnitTypeHumanTanker;	/// orc tanker unit-type pointer
extern UnitType*UnitTypeOrcTanker;	/// human tanker unit-type pointer
extern UnitType*UnitTypeHumanTankerFull;/// orc tanker full unit-type pointer
extern UnitType*UnitTypeOrcTankerFull;	/// human tanker full unit-type pointer
extern UnitType*UnitTypeHumanWorker;	/// Human worker
extern UnitType*UnitTypeOrcWorker;	/// Orc worker
extern UnitType*UnitTypeHumanWorkerWithGold;	/// Human worker with gold
extern UnitType*UnitTypeOrcWorkerWithGold;	/// Orc worker with gold
extern UnitType*UnitTypeHumanWorkerWithWood;	/// Human worker with wood
extern UnitType*UnitTypeOrcWorkerWithWood;	/// Orc worker with wood
extern UnitType*UnitTypeHumanWall;	/// Human wall
extern UnitType*UnitTypeOrcWall;	/// Orc wall
extern UnitType*UnitTypeCritter;	/// Critter unit-type pointer
extern UnitType*UnitTypeBerserker;	/// Berserker for berserker regeneration

extern char** UnitTypeWcNames;		/// Mapping wc-number 2 symbol

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

extern void UnitTypeCclRegister(void);	/// register ccl features

extern void PrintUnitTypeTable(void);	/// generate c-table
extern void UpdateStats(void);		/// update unit stats
extern void ParsePudUDTA(const char*,int); /// parse pud udta table
extern UnitType* UnitTypeByIdent(const char*);	/// get unit-type by ident
extern UnitType* UnitTypeByWcNum(unsigned);	/// get unit-type by wc number

extern void SaveUnitTypes(FILE* file);	/// save the unit-type table
extern UnitType* NewUnitTypeSlot(char*);/// allocate an empty unit-type slot
    /// Draw the sprite frame of unit-type
extern void DrawUnitType(const UnitType* type,unsigned frame,int x,int y);

extern void InitUnitTypes(void);	/// init unit-type table
extern void LoadUnitTypes(void);	/// load the unit-type data
extern void CleanUnitTypes(void);	/// cleanup unit-type module

//@}

#endif	// !__UNITTYPE_H__
