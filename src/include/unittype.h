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
/**@name unittype.h	-	The unit-types headerfile. */
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

#ifndef __UNITTYPE_H__
#define __UNITTYPE_H__

//@{

/*----------------------------------------------------------------------------
--	Documentation
----------------------------------------------------------------------------*/

/**
** 	@struct _unit_type_ unittype.h
**
**	\#include "unittype.h"
**
**	typedef struct _unit_type_ UnitType;
**
**	This structure contains the informations that are shared between all
**	units of the same type and determins if an unit is a building,
**	a person, ...
**
**	The unit-type structure members:
**
**	UnitType::Ident
**
**		Unique identifier of the unit-type, used to reference it in
**		config files and during startup. As convention they start with
**		"unit-" fe. "unit-farm".
**		@note Don't use this member in game, use instead the pointer
**		to this structure. See UnitTypeByIdent().
**
**	UnitType::Name
**
**		Pretty name shown by the engine. The name should be shorter
**		than 17 characters and no word can be longer than 8 characters. 
**
**	UnitType::SameSprite
**
**		Identifier of an unit-type with this are the sprites shared.
**
**	UnitType::File[::TilesetMax]
**
**		Path file name of sprite files for the different tilesets.
**		@note It is planned to change this to support more and 
**		better tilesets.
**
**	UnitType::SpriteFile
**
**		Path file name of shadow sprite file for the different tilesets.
**
**	UnitType::DrawLevel
**
**		The Level/Order to draw this type of unit in. 0-255 usually.
**
**	UnitType::Width UnitType::Height
**
**		Size of a sprite frame in pixels. All frames of a sprite have
**		the same size. Also all sprites (tilesets) must have the same
**		size.
**
**	UnitType::ShadowWidth UnitType::ShadowHeight
**
**		Size of a shadow sprite frame in pixels. All frames of a sprite
**		have the same size. Also all sprites (tilesets) must have the
**		same size.
**
**	UnitType::ShadowOffset
**
**		Vertical offset to draw the shadow in pixels.
**
**	UnitType::Animations
**
**		Animation scripts for the different actions. Currently the
**		animations still, move, attack and die are supported.
**		@see Animations @see _animations_
**		@see Animation @see _animation_
**
**	UnitType::Icon
**
**		Icon to display for this unit-type. Contains configuration and
**		run time variable.
**		@note This icon can be used for training, but isn't used.
**
**	UnitType::Missile
**
**		Configuration and run time variable of the missile weapon.
**		@note It is planned to support more than one weapons.
**		And the sound of the missile should be used as fire sound.
**
**	UnitType::CorpseName
**
**		Corpse unit-type name, should only be used during setup.
**
**	UnitType::CorpseType
**
**		Corpse unit-type pointer, only this should be used during run
**		time. Many unit-types can share the same corpse.
**
**	UnitType::CorpseScript
**
**		Index into corpse animation script. Used if unit-types share
**		the same corpse but have different animations.
**
**	UnitType::_Speed
**
**		Non upgraded movement speed.
**		@note Until now we didn't support speed upgrades.
**
**	FIXME: continue this documentation
**
**	UnitType::Construction
**
**		What is shown in construction phase.
**
**	UnitType::SightRange
**
**		Sight range
**
**	UnitType::_HitPoints
**
**		Maximum hit points
**
**	UnitType::_MaxMana
**
**		Maximum mana points
**
**	UnitType::Magic
**
**		Unit is a mage
**
**	UnitType::_Costs[::MaxCosts]
**
**		How many resources needed
**
**	UnitType::RepairHP
**
**		The HP given to a unit each cycle it's repaired.
**		If zero, unit cannot be repaired
**
**	UnitType::_RepairCosts[::MaxCosts]
**
**		Costs per repair cycle to fix a unit.
**
**	UnitType::TileWidth
**
**		Tile size on map width
**
**	UnitType::TileHeight
**
**		Tile size on map height
**
**	UnitType::BoxWidth
**
**		Selected box size width
**
**	UnitType::BoxHeight
**
**		Selected box size height
**
**	UnitType::NumDirections
**
**		Number of directions the unit can face
**
**	UnitType::MinAttackRange
**
**		Minimal attack range
**
**
**	UnitType::_AttackRange
**
**		How far can the unit attack
**
**	UnitType::ReactRangeComputer
**
**		Reacts on enemy for computer
**
**	UnitType::ReactRangePerson
**
**		Reacts on enemy for person player
**
**	UnitType::_Armor
**
**		Amount of armor this unit has
**
**	UnitType::Priority
**
**		Priority value / AI Treatment
**
**	UnitType::_BasicDamage
**
**		Basic damage dealt
**
**	UnitType::_PiercingDamage
**
**		Piercing damage dealt
**
**	UnitType::WeaponsUpgradable
**
**		Weapons could be upgraded
**
**	UnitType::ArmorUpgradable
**
**		Armor could be upgraded
**
**	UnitType::UnitType
**
**		Land / fly / naval
**
**		FIXME: original only visual effect, we do more with this!
**
**	UnitType::DecayRate
**
**		Decay rate in 1/6 seconds
**
**	UnitType::AnnoyComputerFactor
**
**		How much this annoys the computer
**
**		FIXME: not used
**
**	UnitType::MouseAction
**
**		Right click action
**
**	UnitType::Points
**
**		How many points you get for unit. Used in the final score table.
**
**	UnitType::DemolishRange
**
**		If non-zero This is the range for units that can demolish.
**		The demolished area is a square, sorry.
**
**	UnitType::DemolishDamage
**
**		Damage dealt to unit affected by demolition. This can be 0,
**		in this case only terrain will be affected. Units have to be in
**		DemolishRange to be affected.
**
**	UnitType::CanTarget
**
**		Which units can it attack
**
**	Unit::Revealer
**
**		A special unit used to reveal the map for a time. This unit
**		has active sight even when Removed. It's used for Reveal map
**		type of spells.
**
**	UnitType::LandUnit
**
**		Land animated
**
**	UnitType::AirUnit
**
**		Air animated
**
**	UnitType::SeaUnit
**
**		Sea animated
**
**	UnitType::ExplodeWhenKilled
**
**		Death explosion animated
**
**	UnitType::Critter
**
**		Unit is marked as critter. The effect of this is that when
**		idle the unit will move around randomly.
**
**	UnitType::RandomMovementProbability
**
**		When the unit is idle this is the probability that it will
**		take a step in a random direction, in percents.
**
**	UnitType::ClickToExplode
**
**		If this is non-zero, then after that many clicks the unit will
**		commit suicide. Doesn't work with resource workers/resources.
**
**	UnitType::Wall
**
**		This Unit is a wall, and should exihibit joining properties
**
**	UnitType::Building
**
**		Unit is a Building
**
**	UnitType::PermanentCloak
**
**		Unit is permanently cloaked.
**
**	UnitType::DetectCloak
**
**		These units can detect Cloaked units.
**
**	UnitType::Coward
**
**		Unit is a coward, and acts defensively. it will not attack
**		at will and auto-casters will not buff it(bloodlust).
**
**	UnitType::Transporter
**
**		Can transport units
**
**	UnitType::MaxOnBoard
**
**		Maximum units on board (for transporters)
**
**	UnitType::MaxWorkers
**
**		This limit the number of workers on a resource.
**
**	UnitType::GivesResource
**
**		This equals to the resource Id of the resource given
**		or 0 (TimeCost) for other buildings.
**
**	UnitType::CanHarvest
**
**		Resource can be harvested. It's false for things like
**		oil patches.
**		FIXME: crappy name.
**
**	UnitType::Harvester
**
**		Unit is a resource worker. Faster than examining ResInfo
**
**
**	UnitType::ResInfo[::MaxCosts]
**
**		Information about resource harvesting. If NULL, it can't
**		harvest it.
**
**	UnitType::MustBuildOnTop
**
**		Points to the type of building it must be build on or
**		NoUnitP otherwise. Buggy, works for oil platforms.
**
**	UnitType::CanStore[::MaxCosts]
**
**		What resource types we can store here.
**
**	UnitType::Vanishes
**
**		Corpes & destroyed places
**
**	UnitType::GroundAttack
**
**		Can do command ground attack
**
**	UnitType::IsUndead
**
**		Unit is already dead
**
**	UnitType::ShoreBuilding
**
**		Building must be build on coast
**
**	UnitType::CanCastSpell
**
**		Unit is able to use spells
**
**	UnitType::CanAttack
**
**		Unit is able to attack.
**
**	UnitType::RepairRange
**
**		Unit can repair buildings. It will use the actack animation.
**		It will heal 4 points for every repair cycle, and cost 1 of
**		each resource, alternatively(1 cycle wood, 1 cycle gold)
**		FIXME: The above should be more configurable.
**		If units have a repair range, they can repair, and this is the
**		distance.
**
**	UnitType::BuilderOutside
**
**		Only valid for buildings. When building the worker will
**		remain outside inside the building.
**
**		WARNING: Workers that can build buildings with the 
**		WARNING: BuilderOutside flag must have the CanRepair flag.
**
**	UnitType::BuilderLost
**
**		Only valid for buildings without the BuilderOutside flag.
**		The worker is lost when the building is completed.
**
**	UnitType::Tower
**
**		FIXME:  find a way to remove it.
**		Used for towers(attacknig buildings.)
**
**	UnitType::Hero
**
**		FIXME:	Unit is a hero. Where is this used?
**		FIXME:  I don't think w*rcr*ft 2 exp heroes have this flag.
**		In st*rcr*ft heroes seem to be imune to spawn broodlings,
**		maybe we could use it in the same way. Spawn broodlings is
**		an instant kill for many units.
**
**	UnitType::Volatile
**
**		Invisiblity/unholy armor kills unit
**
**	UnitType::Organic
**
**		Organic can be healed
**
**	UnitType::SelectableByRectangle
**
**		Selectable with mouse rectangle
**
**	UnitType::Teleporter
**
**		Can teleport other units.
**
**	UnitType::Sound
**
**		Sounds for events
**
**	UnitType::Weapon
**
**		Currently sound for weapon
**
**	FIXME: temporary solution
**
**	UnitType::Supply
**
**		How much food does this unit supply.
**
**	UnitType::Demand
**
**		Food demand
**
**	UnitType::ImproveIncomes[::MaxCosts]
**
**		Gives the player an improved income.
**
**	UnitType::FieldFlags
**
**		Flags that are set, if an unit enters a map field or cleared, if
**		an unit leaves a map field.
**
**	UnitType::MovementMask
**
**		Movement mask, this value is and'ed to the map field flags, to
**		see if an unit can enter or placed on the map field.
**
**	UnitType::Stats[::PlayerMax]
**
**		Unit status for each player
**		FIXME: This stats should? be moved into the player struct
**
**	UnitType::Type
**
**		Type as number
**		FIXME: Should us a general name f.e. Slot here?
**
**	UnitType::Property
**
**		CCL property storage
**		FIXME: how is this used at runtime?
**
**	UnitType::Sprite
**
**		Sprite images
**
**	UnitType::ShadowSprite
**
**		Shadow sprite images
**
**	UnitType::PlayerColorSprite
**
**		Sprite images of the player colors.  This image is drawn
**		over UnitType::Sprite.  Used with OpenGL only.
**
**
**
** 	@struct _resource_info_ unittype.h
**
** 	\#include "unittype.h"
**
** 	typedef struct _unit_type_ UnitType;
**
** 	This struct contains information about how a unit will harvest a
** 	resource.
**
**	ResourceInfo::FileWhenLoaded
**
**		The harvester's animation file will change when it's loaded.
**
**	ResourceInfo::FileWhenEmpty;
**	
**		The harvester's animation file will change when it's empty.
**		The standard animation is used only when building/repairing.
**
**
**	ResourceInfo::HarvestFromOutside
**
**		Unit will harvest from the outside. The unit will use it's
**		Attack animation (seems it turned into a generic Action anim.)
**
**      ResourceInfo::ResourceId
**
**		The resource this is for. Mostly redundant.
**
**	ResourceInfo::FinalResource
**
**		The resource is converted to this at the depot. Usefull for
**		a fisherman who harvests fish, but it all turns to food at the
**		depot.
**
**	ResourceInfo::WaitAtResource
**
**		Cycles the unit waits while inside a resource.
**
**	ResourceInfo::ResourceStep
**
**		The unit makes so-caled mining cycles. Each mining cycle
**		it does some sort of animation and gains ResourceStep
**		resources. You can stop after any number of steps.
**		when the quantity in the harvester reaches the maximum
**		(ResourceCapacity) it will return home. I this is 0 then
**		it's considered infinity, and ResourceCapacity will now
**		be the limit.
**
**	ResourceInfo::ResourceCapacity
**
**		Maximum amount of resources a harvester can carry. The
**		actual amount can be modified while unloading.
**
**	ResourceInfo::LoseResources
**
**		Special lossy behaviour for loaded harvesters. Harvesters
**		with loads other than 0 and ResourceCapacity will lose their
**		cargo on any new order.
**
**	ResourceInfo::WaitAtDepot
**
**		Cycles the unit waits while inside the depot to unload.
**
**	ResourceInfo::TerrainHarvester
**
**		The unit will harvest terrain. For now this only works
**		for wood. maybe it could be made to work for rocks, but
**		more than that requires a tileset rewrite.
**		FIXME: more configurable.
**
*/

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include "video.h"
#include "icons.h"
#include "sound_id.h"
#include "unitsound.h"
#include "upgrade_structs.h"
#include "construct.h"
#include "etlib/hash.h"
#ifdef NEW_UI
#include "ccl.h"
#endif

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

/**
**	Defines the animation for different actions.
*/
typedef struct _animation_ {
    unsigned char	Flags;		/// Flags for actions
    signed char		Pixel;		/// Change the position in pixels
    unsigned char	Sleep;		/// Wait for next animation
    int			Frame;		/// Sprite-frame to display
} Animation;

#define AnimationRestart	1	/// Restart animation
#define AnimationReset		2	/// Animation could here be aborted
#define AnimationSound		4	/// Play sound
#define AnimationMissile	8	/// Fire projectil
#define AnimationEnd		0x80	/// Animation end in memory

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

#define ANIMATIONS_MAXANIM 1024

    /// Hash table of all the animations
typedef hashtable(Animations*,ANIMATIONS_MAXANIM) _AnimationsHash;
extern _AnimationsHash AnimationsHash;

/**
**      Missile type definition (used in config tables)
**
**	@todo Shouldn't I move this into missle.h?
*/
typedef struct _missile_config_ {
    char*	Name;			/// Config missile name
    MissileType*Missile;		/// Identifier to use to run time
} MissileConfig;

typedef struct _resource_info_ {
    char * 	FileWhenLoaded;			/// Change the graphic when the unit is loaded.
    char * 	FileWhenEmpty;			/// Change the graphic when the unit is empty.
    unsigned 	HarvestFromOutside;		/// Unit harvests without entering the building.
    unsigned 	WaitAtResource;			/// Cycles the unit waits while mining.
    unsigned 	ResourceStep;			/// Resources the unit gains per mining cycle.
    unsigned 	ResourceCapacity;		/// Max amount of resources to carry.
    unsigned 	WaitAtDepot;			/// Cycles the unit waits while returning.
    unsigned	ResourceId;			/// Id of the resource harvested. Redundant.
    unsigned 	FinalResource;			/// Convert resource when delivered. 
    unsigned 	TerrainHarvester;		/// Unit will harvest terrain(wood only for now).
    unsigned	LoseResources;			/// The unit will lose it's resource when distracted.
    //  Runtime info:
    Graphic *   SpriteWhenLoaded;		/// The graphic corresponding to FileWhenLoaded.
    Graphic *   SpriteWhenEmpty;		/// The graphic corresponding to FileWhenEmpty
} ResourceInfo;

/**
**	Typedef of base structure of unit-type
*/
typedef struct _unit_type_ UnitType;

    /// Base structure of unit-type
struct _unit_type_ {
    char*	Ident;			/// Identifier
    char*	Name;			/// Pretty name shown from the engine
    char*	SameSprite;		/// Unit-type shared sprites
    char*	File[TilesetMax];	/// Sprite files
    char*	ShadowFile;		/// Shadow file

    int		Width;			/// Sprite width
    int		Height;			/// Sprite height
    int		DrawLevel;		/// Level to Draw Unittype at
    int		ShadowWidth;		/// Shadow sprite width
    int		ShadowHeight;		/// Shadow sprite height
    int		ShadowOffset;		/// Shadow vertical offset

    Animations*	Animations;		/// Animation scripts

    IconConfig	Icon;			/// Icon to display for this unit
    MissileConfig Missile;		/// Missile weapon

    char*	CorpseName;		/// Corpse type name
    UnitType*	CorpseType;		/// Corpse unit-type
    int		CorpseScript;		/// Corpse script start

    int		_Speed;			/// Movement speed

// this is taken from the UDTA section
    Construction*Construction;		/// What is shown in construction phase
    int		_SightRange;		/// Sight range
    int		_HitPoints;		/// Maximum hit points
    unsigned	_MaxMana : 8;		/// Maximum mana points
    // FIXME: only flag
    int		Magic;			/// Unit can cast spells

    int		_Costs[MaxCosts];	/// How many resources needed
    int		RepairHP;		/// Amount of HP per repair
    int		_RepairCosts[MaxCosts]; /// How much it costs to repair

    int		TileWidth;		/// Tile size on map width
    int		TileHeight;		/// Tile size on map height
    int		BoxWidth;		/// Selected box size width
    int		BoxHeight;		/// Selected box size height
    int		NumDirections;		/// Number of directions unit can face
    int		MinAttackRange;		/// Minimal attack range
    int		_AttackRange;		/// How far can the unit attack
    int		ReactRangeComputer;	/// Reacts on enemy for computer
    int		ReactRangePerson;	/// Reacts on enemy for person player
    int		_Armor;			/// Amount of armor this unit has
    int		Priority;		/// Priority value / AI Treatment
    int		_BasicDamage;		/// Basic damage dealt
    int		_PiercingDamage;	/// Piercing damage dealt
    int		WeaponsUpgradable;	/// Weapons could be upgraded
    int		ArmorUpgradable;	/// Armor could be upgraded
    // FIXME: original only visual effect, we do more with this!
    enum {
	UnitTypeLand,			/// Unit lives on land
	UnitTypeFly,			/// Unit lives in air
	UnitTypeNaval,			/// Unit lives on water
    }		UnitType;		/// Land / fly / naval
    int		DecayRate;		/// Decay rate in 1/6 seconds
    // FIXME: not used
    int		AnnoyComputerFactor;	/// How much this annoys the computer
    int		MouseAction;		/// Right click action
#define MouseActionNone		0		/// Nothing
#define MouseActionAttack	1		/// Attack
#define MouseActionMove		2		/// Move
#define MouseActionHarvest	3		/// Harvest resources
#define MouseActionDemolish	5		/// Demolish
#define MouseActionSail		6		/// Sail
    int		Points;			/// How many points you get for unit
    int		CanTarget;		/// Which units can it attack
#define CanTargetLand	1			/// Can attack land units
#define CanTargetSea	2			/// Can attack sea units
#define CanTargetAir	4			/// Can attack air units

    unsigned EquivType : 1;		/// 
    unsigned Revealer : 1;		/// reveal the fog of war
    unsigned LandUnit : 1;		/// Land animated
    unsigned AirUnit : 1;		/// Air animated
    unsigned SeaUnit : 1;		/// Sea animated
    unsigned ExplodeWhenKilled : 1;	/// Death explosion animated
    unsigned Critter : 1;		/// Unit is controlled by nobody
    unsigned int RandomMovementProbability;/// Probability to move randomly.
    unsigned int ClicksToExplode;	/// Number of consecutive clicks until unit suicides.
    unsigned Wall : 1;			/// Wall
    unsigned Building : 1;		/// Building
    unsigned PermanentCloak : 1;	/// Is only visible by CloakDetectors.
    unsigned DetectCloak : 1;		/// Can see Cloaked units.
    unsigned Coward : 1;		/// Unit will only attack if instructed.
    unsigned Transporter : 1;		/// Can transport units
    unsigned MaxOnBoard;		/// Number of Transporter slots.
    unsigned Vanishes : 1;		/// Corpes & destroyed places.
    unsigned GroundAttack : 1;		/// Can do command ground attack.
    unsigned IsUndead : 1;		/// Unit is already dead.
    unsigned ShoreBuilding : 1;		/// Building must be build on coast.
    unsigned CanCastSpell : 1;		/// Unit is able to use spells.
    unsigned CanAttack : 1;		/// Unit can attack.
    unsigned int DemolishRange;		/// Unit will Demolish around when dead.
    unsigned int DemolishDamage;	/// Damage dealt to unit affected by demolition.
    unsigned int RepairRange;		/// Units repair range.
    unsigned BuilderOutside : 1;	/// The builder stays outside during the build.
    unsigned BuilderLost : 1;		/// The builder is lost after the build.
    // FIXME: n0body: AutoBuildRate not implemented.
    unsigned AutoBuildRate;		/// The rate at which the building builds itself
    unsigned Tower : 1;			/// Unit can attack, but not move.
    unsigned Hero : 1;			/// Is hero only used for triggers .
    unsigned Volatile : 1;		/// Invisiblity/unholy armor kills unit.
    unsigned Organic : 1;		/// Organic can be healed.

    unsigned CanStore[MaxCosts];	/// Resources that we can store here.
    unsigned GivesResource;		/// The resource this unit gives.
    unsigned MaxWorkers;		/// Maximum number of workers.
    unsigned CanHarvest : 1;		/// Resource can be harvested.
    unsigned Harvester : 1;		/// unit is a resource harvester.
    ResourceInfo* ResInfo[MaxCosts];	/// Resource information.
    UnitType* MustBuildOnTop;		/// Must be built on top of something.

    unsigned SelectableByRectangle : 1;	/// Selectable with mouse rectangle.
    unsigned Teleporter : 1;		/// Can teleport other units.

    UnitSound Sound;			/// Sounds for events
    // FIXME: temporary solution
    WeaponSound Weapon;                 /// Currently sound for weapon

    int		Supply;			/// Food supply
    int		Demand;			/// Food demand

// --- FILLED UP ---

    int		ImproveIncomes[MaxCosts];/// Gives player an improved income

    unsigned	FieldFlags;		/// Unit map field flags
    unsigned	MovementMask;		/// Unit check this map flags for move

	// FIXME: This stats should? be moved into the player struct
    UnitStats Stats[PlayerMax];		/// Unit status for each player

	// FIXME: Should us a general name f.e. Slot here?
    int		Type;			/// Type as number

    void*	Property;		/// CCL property storage

    Graphic*	Sprite;			/// Sprite images
    Graphic*	ShadowSprite;		/// Shadow sprite image
#ifdef USE_OPENGL
    Graphic*	PlayerColorSprite[PlayerMax];	/// Sprites with player colors
#endif
#ifdef NEW_UI

    SCM		AddButtonsHook;		/// Script called to add UI buttons
#endif
};

    // FIXME: ARI: should be dynamic (ccl..), JOHNS: Pud only supports 255.
    /// How many unit-types are currently supported
#define UnitTypeMax	257

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern UnitType* UnitTypes[UnitTypeMax];	/// All unit-types
extern int NumUnitTypes;			/// Number of unit-types made

// FIXME: this hardcoded unit-types must be removed!!
extern UnitType*UnitTypeHumanWall;		/// Human wall
extern UnitType*UnitTypeOrcWall;		/// Orc wall
extern UnitType*UnitTypeCritter;		/// Critter unit-type pointer
extern UnitType*UnitTypeBerserker;		/// Berserker for berserker regeneration

extern char** UnitTypeWcNames;			/// Mapping wc-number 2 symbol

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

extern void UnitTypeCclRegister(void);		/// Register ccl features

extern void UpdateStats(int reset_to_default);		/// Update unit stats
extern void ParsePudUDTA(const char*,int); 		/// Parse pud udta table
extern UnitType* UnitTypeByIdent(const char*);		/// Get unit-type by ident
extern UnitType* UnitTypeByWcNum(unsigned);		/// Get unit-type by wc number

    /// Get the animations structure by ident
extern Animations* AnimationsByIdent(const char* ident);

extern void SaveUnitTypes(CLFile* file);		/// Save the unit-type table
extern UnitType* NewUnitTypeSlot(char*);		/// Allocate an empty unit-type slot
    /// Draw the sprite frame of unit-type
extern void DrawUnitType(const UnitType* type,int frame,int x,int y);

extern void InitUnitTypes(int reset_player_stats);	/// Init unit-type table
extern void LoadUnitTypes(void);			/// Load the unit-type data
extern void CleanUnitTypes(void);			/// Cleanup unit-type module

//@}

#endif	// !__UNITTYPE_H__
