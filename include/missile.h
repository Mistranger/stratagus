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
/**@name missile.h	-	The missile headerfile. */
//
//	(c) Copyright 1998-2002 by Lutz Sammer
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

#ifndef __MISSILE_H__
#define __MISSILE_H__

//@{

/*----------------------------------------------------------------------------
--	Documentation
----------------------------------------------------------------------------*/

/**
**	@struct _missile_type_ missile.h
**
**	\#include "missile.h"
**
**	typedef struct _missile_type_ MissileType;
**
**	This structure defines the base type information of all missiles. It
**	contains all information that all missiles of the same type shares.
**	The fields are filled from the configuration files (CCL). See
**	(define-missile-type).
**
**
**	The missile-type structure members:
**
**	MissileType::OType
**
**		Object type (future extensions).
**
**	MissileType::Ident
**
**		Unique identifier of the missile-type, used to reference it in
**		config files and during startup.
**		@note Don't use this member in game, use instead the pointer
**		to this structure. See MissileTypeByIdent().
**
**	MissileType::File
**
**		File containing the image (sprite) graphics of the missile.
**		The file can contain multiple sprite frames.  The sprite frames
**		for the different directions are placed in the row.
**		The different animations steps are placed in the column. But
**		the correct order depends on MissileType::Class (
**		::_missile_class_, ...). Missiles like fire have no directions,
**		missiles like arrows have a direction.
**		@note Note that only 8 directions are currently supported and
**		only 5 are stored in the image (N NW W SW S) and 4 are mirrored.
**
**	MissileType::DrawLevel
**
**		The Level/Order to draw the missile in, usually 0-255
**
**	MissileType::Width MissileType::Height
**
**		Size (width and height) of a frame in the image. All sprite
**		frames of the missile-type must have the same size.
**
**	MissileType::SpriteFrames
**
**		Total number of sprite frames in the graphic image.
**		@note If the image is smaller than the number of directions,
**		width/height and/or framecount suggest the engine crashes.
**		@note There is currently a limit of 127 sprite frames, which
**		can be lifted if needed.
**
**	MissileType::NumDirections
**
**		Number of directions missile can face.
**
**	MissileType::FiredSound
**
**		Sound of the missile, if fired. @note currently not used.
**
**	MissileType::ImpactSound
**
**		Impact sound for this missile.
**
**	MissileType::CanHitOwner
**
**		Can hit the unit that have fired the missile.
**		@note Currently no missile can hurt its owner.
**
**	MissileType::FriendlyFire
**
**		Can't hit the units of the same player, that has the
**		missile fired.
**
**	MissileType::Class
**
**		Class of the missile-type, defines the basic effects of the
**		missile. Look at the different class identifiers for more
**		informations (::_missile_class_, ::MissileClassNone, ...).
**		This isn't used if the missile is handled by
**		Missile::Controller.
**
**	MissileType::StartDelay
**
**		Delay in game cycles after the missile generation, until the
**		missile animation and effects starts. Delay denotes the number
**		of display cycles to skip before drawing the first sprite frame
**		and only happens once at start.
**
**	MissileType::Sleep
**
**		This are the number of game cycles to wait for the next
**		animation or the sleeping between the animation steps.
**		All animations steps use the same delay.  0 is the
**		fastest and 255 the slowest animation.
**		@note Perhaps we should later allow animation scripts for
**		more complex animations.
**
**	MissileType::Speed
**
**		The speed how fast the missile moves. 0 the missile didn't
**		move, 1 is the slowest speed and 32 s the fastest supported
**		speed. This is how many pixels the missiles moves with each
**		animation step.  The real use of this member depends on the
**		MisleType::Class or Missile::Controller.
**		@note This is currently only used by the point-to-point
**		missiles (::MissileClassPointToPoint, ...).  Perhaps we should
**		later allow animation scripts for more complex animations.
**
**	MissileType::Range
**
**		Determines the range that a projectile will deal its damage.
**		A range of 0 will mean that the damage will be limited to only
**		where the missile was directed towards.  So if you shot a
**		missile at a unit, it would only damage that unit.  A value of
**		1 only effects the field on that the missile is shot.  A value
**		of 2  would mean that for a range of 1 around the impact spot,
**		the damage for that particular missile would be dealt.
**		All fields that aren't the center get only 50% of the damage.
**		@note Can this value be higher? 3 (3x3 area with 25%),
**		4 (4x4 area with 12.5%)! Yes, but is currently not written.
**
**	MissileType::ImpactName
**
**		The name of the next (other) missile to generate, when this
**		missile reached its end point or its life time is over.  So it
**		can be used to generate a chain of missiles.
**		@note Used and should only be used during configuration and
**		startup.
**
**	MissileType::ImpactMissile
**
**		Pointer to the impact missile-type.  Used during run time.
**
**	MissileType::Sprite
**
**		Missile sprite image loaded from MissileType::File
*/

/**
**	@struct _missile_ missile.h
**
**	\#include "missile.h"
**
**	typedef struct _missile_ Missile;
**
**	This structure contains all informations about a missile in game.
**	A missile could have different effects, based on its missile-type.
**	Missiles could be saved and stored with CCL. See (missile).
**	Currently only a tile, an unit or a missile could be placed on the map.
**
**
**	The missile structure members:
**
**	Missile::X Missile::Y
**
**		Missile current map position in pixels.  To convert a map tile
**		position to pixel position use: (mapx*::TileSizeX+::TileSizeX/2)
**		and (mapy*::TileSizeY+::TileSizeY/2)
**		@note ::TileSizeX%2==0 && ::TileSizeY%2==0 and ::TileSizeX,
**		::TileSizeY are currently fixed 32 pixels.
**
**	Missile::DX Missile::DY
**
**		Missile destination on the map in pixels.  If
**		Missile::X==Missile::DX and Missile::Y==Missile::DY the missile
**		stays at its position.  But the movement also depends on
**		the Missile::Controller or MissileType::Class.
**
**	Missile::Type
**
**		::MissileType pointer of the missile, contains the shared
**		informations of all missiles of the same type.
**
**	Missile::SpriteFrame
**
**		Current sprite frame of the missile.  The rang is from 0
**		to MissileType::SpriteFrames-1.  The topmost bit (128) is
**		used as flag to mirror the sprites in X direction.
**		Animation scripts aren't currently supported for missiles,
**		everything is handled by the MissileType::Class or
**		Missile::Controller.
**		@note If wanted, we can add animation scripts support to the
**		engine.
**
**	Missile::State
**
**		Current state of the missile.  Used for a simple state machine.
**
**	Missile::Wait
**
**		Wait this number of game cycles until the next state or
**		animation of this missile is handled. This counts down from
**		MissileType::Sleep to 0.
**
**	Missile::Delay
**
**		Number of game cycles the missile isn't shown on the map.
**		This counts down from MissileType::StartDelay to 0, before this
**		happens the missile isn't shown and has no effects.
**		@note This can also be used by MissileType::Class or
**		Missile::Controller for temporary removement of the missile.
**
**	Missile::SourceUnit
**
**		The owner of the missile. Normally the one who fired the
**		missile.  Used to check units, to prevent hitting the owner
**		when field MissileType::CanHitOwner==1. Also used for kill
**		and experience points.
**
**	Missile::TargetUnit
**
**		The target of the missile.  Normally the unit which should be
**		hit by the missile.
**
**	Missile::Damage
**
**		Damage done by missile. See also MissileType::Range, which
**		denoted the 100% damage in center.
**
**	Missile::TTL
**
**		Time to live in game cycles of the missile, if it reaches zero
**		the missile is automatic removed from the map. If -1 the
**		missile lives for ever and the lifetime is handled by
**		Missile::Type:MissileType::Class or Missile::Controller.
**
**	Missile::Controller
**
**		A function pointer to the function which controls the missile.
**		Overwrites Missile::Type:MissileType::Class when !NULL.
**
**	Missile::D
**
**		Delta for Bresenham's line algorithm (point to point missiles).
**
**	Missile::Dx Missile::Dy
**
**		Delta x and y for Bresenham's line algorithm (point to point
**		missiles).
**
**	Missile::Xstep Missile::Ystep
**
**		X and y step for Bresenham's line algorithm (point to point
**		missiles).
**
**	Missile::Local
**
**		This is a local missile, which can be different on all
**		computer in play. Used for the user interface (fe the green
**		cross).
**
**	Missile::MissileSlot
**
**		Pointer to the slot of this missile. Used for faster freeing.
*/

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include "unitsound.h"
#include "unittype.h"
#include "upgrade_structs.h"
#include "player.h"
#include "video.h"

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Missile-type
----------------------------------------------------------------------------*/

#ifndef __STRUCT_MISSILETYPE__
#define __STRUCT_MISSILETYPE__		/// protect duplicate missile typedef

/**
**	Missile-type typedef
*/
typedef struct _missile_type_ MissileType;

#endif

#define MAX_MISSILES    1800            /// maximum number of missiles

/**
**	Missile-type-class typedef
*/
typedef int MissileClass;

/**
**      Missile-class this defines how a missile-type reacts.
**
**      @todo
**              Here is something double defined, the whirlwind is
**              ClassWhirlwind and also handled by controler.
**
**      FIXME:  We need no class or no controller.
*/
enum _missile_class_ {
        /**
        **      Missile does nothing
        */
	MissileClassNone,
        /**
         **      Missile flies from x,y to x1,y1
         */
	MissileClassPointToPoint,
	/**
	**      Missile flies from x,y to x1,y1 and stays there for a moment
	*/
	MissileClassPointToPointWithDelay,
	/**
	**      Missile don't move, than disappears
	*/
	MissileClassStayWithDelay,
	/**
	**      Missile flies from x,y to x1,y1 than bounces three times.
	*/
	MissileClassPointToPoint3Bounces,
	/**
	**      Missile flies from x,y to x1,y1 than changes into flame shield
	*/
	MissileClassFireball,
	/**
	**      Missile surround x,y
	*/
	MissileClassFlameShield,
	/**
	**      Missile appears at x,y, is blizzard
	*/
	MissileClassBlizzard,
	/**
	**      Missile appears at x,y, is death and decay
	*/
	MissileClassDeathDecay,
	/**
	**      Missile appears at x,y, is whirlwind
	*/
	MissileClassWhirlwind,
	/**
	**      Missile appears at x,y, than cycle through the frames up and
	**      down.
	*/
	MissileClassCycleOnce,
	/**
	**      Missile flies from x,y to x1,y1 than shows hit animation.
	*/
	MissileClassPointToPointWithHit,
	/**
	**      Missile don't move, than checks the source unit for HP.
	*/
	MissileClassFire,
	/**
	**      Missile is controlled completely by Controller() function.
	*/
	MissileClassCustom,
	/**
	**      Missile shows the hit points.
	*/
	MissileClassHit,
};

    ///		Base structure of missile-types
struct _missile_type_ {
    const void* OType;			/// object type (future extensions)

    char*	Ident;			/// missile name
    char*	File;			/// missile sprite file

    int		Width;			/// missile width in pixels
    int		Height;			/// missile height in pixels
    int		DrawLevel;		/// Level to draw missile at
    int		SpriteFrames;		/// number of sprite frames in graphic
    int		NumDirections;		/// number of directions missile can face

	// FIXME: FireSound defined but not used!
    SoundConfig FiredSound;		/// fired sound
    SoundConfig ImpactSound;		/// impact sound for this missile-type

    unsigned	CanHitOwner : 1;	/// missile can hit the owner
    unsigned	FriendlyFire : 1;	/// missile can't hit own units

    MissileClass	Class;		/// missile class
    int		StartDelay;		/// missile start delay
    int		Sleep;			/// missile sleep
    int		Speed;			/// missile speed

    int		Range;			/// missile damage range
    char*	ImpactName;		/// impact missile-type name
    MissileType*ImpactMissile;		/// missile produces an impact

// --- FILLED UP ---
    Graphic*	Sprite;			/// missile sprite image
};

/*----------------------------------------------------------------------------
--	Missile
----------------------------------------------------------------------------*/

/**
**	Missile typedef.
*/
typedef struct _missile_ Missile;

    /// Missile on the map
struct _missile_ {
    int		X;			/// missile pixel position
    int		Y;			/// missile pixel position
    int		DX;			/// missile pixel destination
    int		DY;			/// missile pixel destination
    MissileType*Type;			/// missile-type pointer
    int		SpriteFrame;		/// sprite frame counter
    int		State;			/// state
    int		Wait;			/// delay between frames
    int		Delay;			/// delay to showup

    Unit*	SourceUnit;		/// unit that fires (could be killed)
    Unit*	TargetUnit;		/// target unit, used for spells

    int		Damage;			/// direct damage that missile applies

    int		TTL;			/// time to live (ticks) used for spells
    void (*Controller)( Missile* );	/// used to controll spells

// Internal use:
    int		D;			/// for point to point missiles
    int		Dx;			/// delta x
    int		Dy;			/// delta y
    int		Xstep;			/// X step
    int		Ystep;			/// Y step

    unsigned	Local : 1;		/// missile is a local missile
    Missile**	MissileSlot;		/// pointer to missile slot
};

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern char** MissileTypeWcNames;	/// Mapping wc-number 2 symbol

extern MissileType* MissileTypes;		/// All missile-types
extern MissileType* MissileTypeSmallFire;	/// Small fire missile-type
extern MissileType* MissileTypeBigFire;		/// Big fire missile-type
extern MissileType* MissileTypeExplosion;	/// Explosion missile-type
extern MissileType* MissileTypeHit;		/// Hit missile-type

extern const char* MissileClassNames[];		/// Missile class names

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

//	In ccl_missile.c

    /// register ccl features
extern void MissileCclRegister(void);

//	In missile.c

    /// load the graphics for the missiles
extern void LoadMissileSprites(void);
    /// allocate an empty missile-type slot
extern MissileType* NewMissileTypeSlot(char*);
    /// Get missile-type by ident
extern MissileType* MissileTypeByIdent(const char*);
    /// create a missile
extern Missile* MakeMissile(MissileType*,int,int,int,int);
    /// create a local missile
extern Missile* MakeLocalMissile(MissileType*,int,int,int,int);
    /// fire a missile
extern void FireMissile(Unit*);
    /// check if missile should be drawn
extern int CheckMissileToBeDrawn(const Missile* missile);

    /// Draw all missiles
extern void DrawMissile(const MissileType* mtype,int frame,int x,int y);
extern int FindAndSortMissiles(const Viewport* vp, Missile** table);

    /// handle all missiles
extern void MissileActions(void);
    /// distance from view point to missile
extern int ViewPointDistanceToMissile(const Missile*);

    /// Save missile-types
extern void SaveMissileTypes(CLFile *);
    /// Save missiles
extern void SaveMissiles(CLFile*);

    /// Initialize missile-types
extern void InitMissileTypes(void);
    /// Clean missile-types
extern void CleanMissileTypes(void);
    /// Initialize missiles
extern void InitMissiles(void);
    /// Clean missiles
extern void CleanMissiles(void);

//@}

#endif	// !__MISSILE_H__
