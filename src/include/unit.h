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
/**@name unit.h		-	The unit headerfile. */
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

#ifndef __UNIT_H__
#define __UNIT_H__

//@{

/*----------------------------------------------------------------------------
--	Documentation
----------------------------------------------------------------------------*/

/**
**	@struct _unit_ unit.h
**
**	\#include "unit.h"
**
**	typedef struct _unit_ Unit;
**
**	Everything belonging to an unit. FIXME: rearrange for less memory.
**
**	This structure contains all informations about an unit in game.
**	An unit could be everything, a man, a vehicle, a ship or a building.
**	Currently only a tile, an unit or a missile could be placed on the map.
**
**	The unit structure members:
**
**	Unit::Refs
**
**		The reference counter of the unit. If the pointer to the unit
**		is stored the counter must be incremented and if this reference
**		is destroyed the counter must be decremented. Alternative it
**		would be possible to implement a garbage collector for this.
**
**	Unit::Slot
**
**		This is the unique slot number. It is not possible that two
**		units have the same slot number at the same time. The slot
**		numbers are reused.
**		This field could be accessed by the macro UnitNumber(Unit*).
**		Maximal 65535 (=#MAX_UNIT_SLOTS) simultaneous units are
**		supported.
**
**	Unit::UnitSlot
**
**		This is the pointer into #Units[], where the unit pointer is
**		stored.  #Units[] is a table of all units currently active in
**		game. This pointer is only needed to speed up, the remove of
**		the unit pointer from #Units[], it didn't must be searched in
**		the table.
**
**	Unit::PlayerSlot
**
**		A pointer into Player::Units[], where the unit pointer is
**		stored. Player::Units[] is a table of all units currently
**		belonging to a player.	This pointer is only needed to speed
**		up, the remove of the unit pointer from Player::Units[].
**
**	Unit::Next
**
**		A generic link pointer. This member is currently used, if an
**		unit is on the map, to link all units on the same map field
**		together.  This member is currently unused, if the unit is
**		removed (see Unit::Removed). F.E.: A worker is removed, if he
**		is in a mine or depot. Or an unit is on board a transporter.
**
**	Unit::Name
**
**		Name of the unit.
**
**	Unit::X Unit::Y
**
**		The tile map coordinates of the unit. 0,0 is the upper left on
**		the map. To convert the map coordinates into pixels, they
**		must be multiplicated with the #TileSizeX and #TileSizeY.
**		To get the pixel coordinates of an unit, calculate 
**		Unit::X*#TileSize+Unit::IX , Unit::Y*#TileSizeY+Unit::IY.
**		
**
**	Unit::Type
**
**		Pointer to the unit-type (::UnitType). The unit-type contains
**		all informations that all units of the same type shares.
**		(Animations, Name, Stats, ...)
**
**	Unit::Player
**
**		Pointer to the owner of this unit (::Player). An unit could
**		only be owned by one player.
**
**	Unit::Stats
**
**		Pointer to the current status (::UnitStats) of an unit. The
**		units of the same player and the same type could share the same
**		stats. The status contains all values which could be different
**		for each player. This f.e. the upgradeable abilities of an
**		unit.  (Unit::Stats::SightRange, Unit::Stats::Armor,
**		Unit::Stats::HitPoints, ...)
**
**	Unit::Colors
**
**		Player colors of the unit. Contains the hardware dependent
**		pixel values for the player colors (palette index #208-#211).
**		Setup from the global palette.
**		@note Index #208-#211 are various SHADES of the team color
**		(#208 is brightest shade, #211 is darkest shade) .... these
**		numbers are NOT red=#208, blue=#209, etc
**
**	Unit::IX Unit::IY
**
**		Coordinate displacement in pixels or coordinates inside a tile.
**		Currently only !=0, if the unit is moving from one tile to
**		another (0-32 and for ships/flyers 0-64).
**
**	Unit::Frame
**
**		Current graphic image of the animation sequence. The high bit
**		(128) is used to flip this image horizontal (x direction).
**		This also limits the number of different frames/image to 126.
**
**	Unit::SeenFrame
**
**		Graphic image (see Unit::Frame) what the player on this
**		computer has last seen. If -1 (0xFF) the player haven't seen
**		this unit yet.
**
**	Unit::Direction
**
**		Contains the binary angle (0-255) in which the direction the
**		unit looks. 0, 32, 64, 128, 160, 192, 224, 256 corresponds to
**		0�, 45�, 90�, 135�, 180�, 225�, 270�, 315�, 360� or north,
**		north-east, east, south-east, south, south-west, west,
**		north-west, north. Currently only 8 directions are used, this
**		is more for the future.
**
**	Unit::Attacked
**
**		If Attacked is non-zero, the unit is attacked. This member is
**		counted down.
**
**	Unit::Burning
**
**		If Burning is non-zero, the unit is burning.
**
**	Unit::Visible
**
**		Used for submarines. It is a bit field for all players. If
**		Unit::Visible&(1<<player-nr) is non-zero, the unit could be
**		seen on the map.
**
**	Unit::Destroyed
**
**	FIXME: @todo
**		If you need more informations, please send me an email or
**		write it self.
**
**	Unit::Removed
**
**
**	Unit::Selected
**
**
**	Unit::Constructed
**
**
**	Unit::Mana
**
**
**	Unit::HP
**
**
**	Unit::XP
**
**
**	Unit::Kills
**
**
**	Unit::Bloodlust
**
**
**	Unit::Haste
**
**
**	Unit::Slow
**
**
**	Unit::Invisible
**
**
**	Unit::FlameShield
**
**
**	Unit::UnholyArmor
**
**
**	Unit::GroupId
**
**		Number of the group to that the unit belongs. This is the main
**		group showed on map, an unit can belong to many groups.
**
**	Unit::LastGroup
**
**		Automatic group number, to reselect the same units. When the
**		user selects more than one unit all units is given the next
**		same number. (Used for ALT-CLICK)
**
**	Unit::Value
**
**		This values hold the amount of gold in a goldmine, amount of
**		oil in an oil patch or oil platform.
**		FIXME: continue documentation
**
**	Unit::SubAction
**
**		This is an action private variable, it is zero on the first
**		entry of an action. Must be set to zero, if an action finishes.
**		It should only be used inside of actions.
**
**	Unit::Wait
**
**		FIXME: continue documentation
**
**	Unit::State
**
**		Animation state, currently position in the animation script.
**		0 if an animation has just started, it should only be changed
**		inside of actions.
**
**
**	Unit::Reset
**
**		FIXME: continue documentation
**
**	Unit::Blink
**
**
**	Unit::Moving
**
**
**	Unit::Rs
**
**
**	Unit::Revealer
**
**		`revealer' is unit that has to keep the fog of war  revealed
**		for some time, this unit cannot be used in usual manner.
**		JOHNS: Should be moved into unit-type!
**
**	Unit::OnBoard[::MAX_UNITS_ONBOARD]
**
**		A table of units on board. This can be units in a transporter
**		or units in a tower.
**
**	Unit::OrderCount
**
**		The number of the orders unit to process. An unit has atleast
**		one order. Unit::OrderCount should be a number between 1 and
**		::MAX_ORDERS. The orders are in Unit::Orders[].
**
**	Unit::OrderFlush
**
**		A flag, which tells the unit to stop with the current order
**		and immediately start with the next order.
**
**	Unit::Orders
**
**		Contains all orders of the unit. Slot 0 is always used.
**		Up to ::MAX_ORDERS can be stored.
**
**	Unit::SavedOrder
**
**		This order is executed, if the current order is finished.
**		This is used for attacking units, to return to the old
**		place or for patrolling units to return to patrol after
**		killing some enemies. Any new order given to the unit,
**		clears this saved order.
**
**	Unit::NewOrder
**
**		This field is only used by buildings and this order is
**		assigned to any by this building new trained unit.
**		This is can be used to set the exit or gathering point of a
**		building.
**
**	Unit::Data
**
**		FIXME: continue documentation
**
**	Unit::Goal
**
**		Unit pointer of the goal of the current command, only used for
**		debugging the new orders, will be removed in release 1.18.
**		The pathfinder must be trigger, if the goal changes.
**
**	Unit::GoalX Unit::GoalY
**
**		Map tile position of the goal of the current command. only used
**		for debugging the new orders, will be removed in release 1.18.
**		The pathfinder must be trigger, if the goal changes.
**
**	Unit::Retreating
**
**		FIXME: continue documentation
**
*/

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#if !defined(__STRUCT_PLAYER__) && !defined(DOXYGEN)
#define __STRUCT_PLAYER__
typedef struct _player_ Player;		// recursive includes :(
#endif

#include "video.h"
#include "unittype.h"
#include "upgrade_structs.h"
#include "upgrade.h"

#ifdef NEW_DECODRAW
#include "deco.h"
#endif

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

typedef struct _unit_ Unit;		/// unit itself
typedef enum _unit_action_ UnitAction;	/// all possible unit actions

/**
**	Unit references over network, or for memory saving.
*/
typedef unsigned short UnitRef;

/**
**	All possible unit actions.
**
**	@note	Always change the table ::HandleActionTable
**
**	@see HandleActionTable
*/
enum _unit_action_ {
    UnitActionNone,			/// No valid action

    UnitActionStill,			/// unit stand still, does nothing
    UnitActionStandGround,		/// unit stands ground
    UnitActionFollow,			/// unit follows units
    UnitActionMove,			/// unit moves to position/unit
    UnitActionAttack,			/// unit attacks position/unit
    UnitActionAttackGround,		/// unit attacks ground
    UnitActionDie,			/// unit dies

    UnitActionSpellCast,		/// unit casts spell

    UnitActionTrain,			/// building is training
    UnitActionUpgradeTo,		/// building is upgrading itself
    UnitActionResearch,			/// building is researching spell
    UnitActionBuilded,			/// building is under construction

// Compound actions
    UnitActionBoard,			/// unit entering transporter
    UnitActionUnload,			/// unit leaving transporter
    UnitActionPatrol,			/// unit paroling area
    UnitActionBuild,			/// unit builds building

    UnitActionRepair,			/// unit repairing
    UnitActionHarvest,			/// unit harvest lumber
    UnitActionMineGold,			/// unit mines gold
    UnitActionMineOre,			/// unit mines ore FIXME: not written
    UnitActionMineCoal,			/// unit mines coal FIXME: not written
    UnitActionQuarryStone,		/// unit quarrying stone FIXME: not written
    UnitActionHaulOil,			/// unit hauling oil
    UnitActionReturnGoods,		/// unit returning any resource

    UnitActionDemolish,			/// unit demolish at position/unit
};

/**
**	Unit order structure.
*/
typedef struct _order_ {
    UnitAction		Action : 8;	/// global action
    unsigned char	Flags;		/// Order flags (unused)
    unsigned char	RangeX;		/// How near in X direction
    unsigned char	RangeY;		/// How near in Y direction

    Unit*		Goal;		/// goal of the order (if any)
    int			X;		/// or X tile coordinate of destination
    int			Y;		/// or Y tile coordinate of destination
    UnitType*		Type;		/// Unit-type argument

    void*		Arg1;		/// Extra command argument
} Order;

/**
**	Voice groups for an unit
*/
typedef enum _unit_voice_group_ {
    VoiceSelected,			/// If selected
    VoiceAcknowledging,			/// Acknowledge command
    VoiceAttacking,			/// FIXME: Should be removed?
    VoiceReady,				/// Command completed
    VoiceHelpMe,			/// If attacked
    VoiceDying,				/// If killed
    VoiceWorkCompleted,			/// only worker, work completed
    VoiceBuilding,			/// only for building under construction
    VoiceDocking,			/// only for transport reaching coast
    VoiceTreeChopping,			/// only worker tree chopping
    VoiceRepair,			/// only worker repairing
} UnitVoiceGroup;

/**
**	Unit/Missile headings.
**		N
**	NW		NE
**	W		 E
**	SW		SE
**		S
*/
enum _directions_ {
    LookingN	=0*32,			/// Unit looking north
    LookingNE	=1*32,			/// Unit looking north east
    LookingE	=2*32,			/// Unit looking east
    LookingSE	=3*32,			/// Unit looking south east
    LookingS	=4*32,			/// Unit looking south
    LookingSW	=5*32,			/// Unit looking south west
    LookingW	=6*32,			/// Unit looking west
    LookingNW	=7*32,			/// Unit looking north west
};

#define NextDirection	32		/// Next direction N->NE->E...

    /// The big unit structure
struct _unit_ {
    // NOTE: int is faster than shorts
    unsigned	Refs;			/// Reference counter
    unsigned	Slot;			/// Assignd slot number
    Unit**	UnitSlot;		/// Slot pointer of Units
    Unit**	PlayerSlot;		/// Slot pointer of Player->Units
    Unit*	Next;			/// Generic link pointer (on map)

    char*	Name;			/// Unit own name

    int		X;			/// Map position X
    int		Y;			/// Map position Y

    UnitType*	Type;			/// Pointer to unit-type (peon,...)
    Player*     Player;			/// Owner of this unit
    UnitStats*	Stats;			/// Current unit stats

//	DISPLAY:
    UnitColors	Colors;			/// Player colors
    char	IX;			/// X image displacement to map position
    char	IY;			/// Y image displacement to map position
    unsigned	Frame : 8;		/// Image frame: high bit used for flip
    unsigned	SeenFrame : 8;		/// last seen frame/stage of buildings

    unsigned	Direction : 8;		/// angle (0-255) unit looking

    unsigned	Attacked : 4;		/// unit is attacked

    unsigned	Burning : 1;		/// unit is burning
    unsigned	Destroyed : 1;		/// unit is destroyed pending reference
    unsigned	Removed : 1;		/// unit is removed (not on map)
    unsigned	Selected : 1;		/// unit is selected

    unsigned	Visible : 16;		/// Unit is visible (submarine)
    unsigned	Constructed : 1;	/// Unit is in construction
    unsigned	Active : 1;		/// Unit is active for AI
    unsigned	Rescued : 1;		/// Unit is rescued

#define MaxMana	255			/// maximal mana for units
    unsigned	Mana : 8;		/// mana points
    unsigned	HP;			/// hit points
    unsigned	XP;			/// experience points
    unsigned	Kills;			/// how many unit has this unit killed

    unsigned	TTL;			/// time to life
    unsigned	Bloodlust;		/// ticks bloodlust
    unsigned	Haste;			/// ticks haste (disables slow)
    unsigned	Slow;			/// ticks slow (disables haste)
    unsigned	Invisible;		/// ticks invisible
    unsigned	FlameShield;		/// ticks flame shield
    unsigned	UnholyArmor;		/// ticks unholy armor

    unsigned	GroupId;		/// unit belongs to this group id
    unsigned	LastGroup;		/// unit belongs to this last group

    unsigned	Value;			/// value used for much

    unsigned	SubAction : 8;		/// sub-action of unit
    unsigned	Wait : 8;		/// action counter
#define MAX_UNIT_WAIT	255		/// biggest number in action counter
    unsigned	State : 8;		/// action state
#define MAX_UNIT_STATE	255		/// biggest state for action
    unsigned	Reset : 1;		/// can process new command
    unsigned	Blink : 3;		/// Let selection rectangle blink
    unsigned	Moving : 1;		/// The unit is moving
					/** set to random 1..100 when MakeUnit()
					** ,used for fancy buildings
					*/
    unsigned	Rs : 8;
    unsigned	Revealer : 1;		/// reveal the fog of war

#define MAX_UNITS_ONBOARD 6		/// max number of units in transporter
    // FIXME: use the new next pointer
    Unit*	OnBoard[MAX_UNITS_ONBOARD];	/// Units in transporter

#define MAX_ORDERS 16			/// How many outstanding orders?
    char	OrderCount;		/// how many orders in queue
    char	OrderFlush;		/// cancel current order, take next
    Order	Orders[MAX_ORDERS];	/// orders to process
    Order	SavedOrder;		/// order to continue after current
    Order	NewOrder;		/// order for new trained units

    union _order_data_ {
    struct _order_move_ {
	char	Fast;			/// Flag fast move (one step)
	char	Length;			/// stored path length
#define MAX_PATH_LENGTH	14		/// max length of precalculated path
	char	Path[MAX_PATH_LENGTH];	/// directions of stored path
    }		Move;			/// ActionMove,...
    struct _order_builded_ {
	Unit*	Worker;			/// Worker building this unit
	int	Sum;			/// HP sum for building
	int	Add;			/// + to HP and sum
	int	Val;			/// Counter (fixed point trick)
	int	Sub;			/// - to counter
	int	Cancel;			/// Cancel construction
    }		Builded;		/// ActionBuilded,...
    struct _order_resource_ {
	int	Active;			/// how much units are in the resource
    }		Resource;		/// Resource still
    struct _order_research_ {
	Upgrade* Upgrade;		/// Upgrade researched
    }		Research;		/// Research action
    struct _order_upgradeto_ {
	unsigned	Ticks;		/// Ticks to complete
    } UpgradeTo;			/// Upgrade to action
    struct _order_train_ {
	unsigned	Ticks;		/// Ticks to complete
	unsigned	Count;		/// Units in training queue
	// FIXME: vladi: later we should train more units or automatic
#define MAX_UNIT_TRAIN	6		/// max number of units in queue
	UnitType*	What[MAX_UNIT_TRAIN];	/// Unit trained
    } Train;				/// Train units action
    }		Data;			/// Storage room for different commands

    const Unit*	Goal;			/// Goal for pathfinder
    int		GoalX;			/// Destination X of pathfinder
    int		GoalY;			/// Destination Y of pathfinder

#ifdef HIERARCHIC_PATHFINDER
#define UNIT_CLEAN		0	/// FIXME: comment missing
#define UNIT_RETREATING		1	/// FIXME: comment missing
#define UNIT_WINNING		2	/// FIXME: comment missing
    unsigned	Retreating:2;		/// FIXME: comment what is this?
	void *PfHierData;
#endif	// HIERARCHIC_PATHFINDER

#ifdef NEW_DECODRAW
    Deco*  deco;           /// Decoration when visible on screen
#endif
};

#define NoUnitP		(Unit*)0	/// return value: for no unit found
#define InfiniteDistance INT_MAX	/// the distance is unreachable

#define FlushCommands	1		/// Flush commands in queue

#define MAX_UNIT_SLOTS	65535		/// Maximal number of used slots

/**
**	Returns true, if unit is unusable. (for attacking,...)
**	FIXME: look if correct used (UnitActionBuilded is no problem if
**	attacked)?
*/
#define UnitUnusable(unit) \
    ( (unit)->Removed || (unit)->Orders[0].Action==UnitActionDie || \
      (unit)->Orders[0].Action==UnitActionBuilded)

/**
**	Returns unit number (unique to this unit)
*/
#define UnitNumber(unit)	((unit)->Slot)

/**
**	How many units could be selected
*/
#define MaxSelectable	9

// FIXME: hardcoded...
/**
**	How many units could be in a group
*/
#define NUM_UNITS_PER_GROUP 9

/**
**	How many groups supported
*/
#define NUM_GROUPS 10

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern Unit* UnitSlots[MAX_UNIT_SLOTS];	/// All possible units
extern Unit** UnitSlotFree;		/// First free unit slot

extern Unit* Units[MAX_UNIT_SLOTS];	/// Units used
extern int NumUnits;			/// Number of units used

//	in unit_draw.c (FIXME: could be moved into the user interface?)
extern int ShowHealthBar;		/// Flag: show health bar
extern int ShowHealthDot;		/// Flag: show health dot
extern int ShowManaBar;			/// Flag: show mana bar
extern int ShowManaDot;			/// Flag: show mana dot
extern int ShowHealthHorizontal;	/// Flag: show health bar horizontal
extern int ShowManaHorizontal;		/// Flag: show mana bar horizontal
extern int ShowNoFull;			/// Flag: show no full health or mana
extern int ShowEnergySelectedOnly;	/// Flag: show energy only for selected
extern int DecorationOnTop;		/// Flag: show health and mana on top
extern int ShowSightRange;		/// Flag: show right range
extern int ShowReactionRange;		/// Flag: show reaction range
extern int ShowAttackRange;		/// Flag: show attack range
extern int ShowOrders;			/// Flag: show orders of unit on map
extern int ShowOrdersCount;		/// Show orders for some time
extern int HitPointRegeneration;	/// Enable hit point regeneration for all units
extern int EnableTrainingQueue;		/// Config: training queues enabled
extern int EnableBuildingCapture;	/// Config: building capture enabled
   /// Draw the selection
extern void (*DrawSelection)(const Unit*,const UnitType*,int,int);

//	in selection.c
extern Unit* Selected[MaxSelectable];	/// currently selected units
extern int NumSelected;			/// how many units selected

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

    /// Prepare unit memory allocator
extern void InitUnitsMemory(void);
    /// Free memory used by unit
//extern void FreeUnitMemory(Unit* unit);
    /// Release an unit
extern void ReleaseUnit(Unit* unit);
    ///	Create a new unit
extern Unit* MakeUnit(UnitType* type,Player* player);
    /// Place an unit on map
extern void PlaceUnit(Unit* unit,int x,int y);
    ///	Create a new unit and place on map
extern Unit* MakeUnitAndPlace(int x,int y,UnitType* type,Player* player);
    /// Remove unit from map/groups/...
extern void RemoveUnit(Unit* unit);
    /// Handle the loose of an unit (food,...)
extern void UnitLost(Unit* unit);
    /// FIXME: more docu
extern void UpdateForNewUnit(const Unit* unit,int upgrade);
    /// FIXME: more docu
extern void NearestOfUnit(const Unit* unit,int tx,int ty,int *dx,int *dy);
    /// Mark submarine as seen
extern void MarkSubmarineSeen(const Player* player,int x,int y,int range);
    /// Returns true, if unit is visible on the map
extern int UnitVisibleOnMap(const Unit* unit);
    /// Returns true, if unit is known on the map
extern int UnitKnownOnMap(const Unit* unit);
    /// Returns true, if unit is visible on current map view
extern int UnitVisibleOnScreen(const Unit* unit);
    /// FIXME: more docu
extern int CheckUnitToBeDrawn(const Unit* unit);
    /// FIXME: more docu
extern void GetUnitMapArea( const Unit* unit,
                            int *sx, int *sy, int *ex, int *ey );
#ifdef HIERARCHIC_PATHFINDER
extern int UnitGetNextPathSegment (Unit * , int * , int * );
#endif
    /// Increment mana of all magic units each second
extern void UnitIncrementMana(void);
    /// Increment health of all regenerating units each second
extern void UnitIncrementHealth(void);
    /// Check for rescue each second
extern void RescueUnits(void);
    /// Change owner of unit
extern void ChangeUnitOwner(Unit* unit,Player* old,Player* new);

    /// Convert direction (dx,dy) to heading (0-255)
extern int DirectionToHeading(int,int);
    /// Update frame from heading
extern void UnitUpdateHeading(Unit* unit);
    /// Heading and frame from delta direction x,y
extern void UnitHeadingFromDeltaXY(Unit* unit,int x,int y);

    /// FIXME: more docu
extern void DropOutOnSide(Unit* unit,int heading,int addx,int addy);
    /// FIXME: more docu
extern void DropOutNearest(Unit* unit,int x,int y,int addx,int addy);
    /// Drop out all units in the unit
extern void DropOutAll(const Unit* unit);

    /// FIXME: more docu
extern int CanBuildHere(const UnitType* type,unsigned x,unsigned y);
    /// FIXME: more docu
extern int CanBuildOn(int x,int y,int mask);
    /// FIXME: more docu
extern int CanBuildUnitType(const Unit* unit,const UnitType* type,int x,int y);

    /// Find nearest gold mine
extern Unit* FindGoldMine(const Unit*,int,int);
    /// Find nearest gold deposit
extern Unit* FindGoldDeposit(const Unit*,int,int);
    /// Find nearest wood deposit
extern Unit* FindWoodDeposit(const Player* player,int x,int y);
    /// Find nearest oil deposit
extern Unit* FindOilDeposit(const Unit*,int x,int y);
    /// Find the next idle worker
extern Unit* FindIdleWorker(const Player* player,const Unit* last);

    /// Find the nearest piece of wood in sight range
extern int FindWoodInSight(const Unit* unit,int* x,int* y);
    /// FIXME: more docu
extern Unit* FindOilPlatform(const Player* player,int x,int y);

    /// FIXME: more docu
extern Unit* UnitOnScreen(Unit* unit,unsigned x,unsigned y);

    /// Let an unit die
extern void LetUnitDie(Unit* unit);
    /// Destory all units inside another unit
extern void DestroyAllInside(Unit* source);
    /// Hit unit with damage, if destroyed give attacker the points
extern void HitUnit(Unit* attacker,Unit* target,int damage);

    /// Returns the map distance between two points
extern int MapDistance(int x1,int y1,int x2,int y2);
    ///	Returns the map distance between two points with unit-type
extern int MapDistanceToType(int x1,int y1,const UnitType* type,int x2,int y2);
    ///	Returns the map distance to unit
extern int MapDistanceToUnit(int x,int y,const Unit* dest);
    ///	Returns the map distance between two units
extern int MapDistanceBetweenUnits(const Unit* src,const Unit* dst);

    /// Calculate the distance from current view point to coordinate
extern int ViewPointDistance(int x,int y);
    /// Calculate the distance from current view point to unit
extern int ViewPointDistanceToUnit(const Unit* dest);

    /// Return true, if unit is an enemy of the player
extern int IsEnemy(const Player* player,const Unit* dest);
    /// Return true, if unit is allied with the player
extern int IsAllied(const Player* player,const Unit* dest);
    /// Can this unit-type attack the other (destination)
extern int CanTarget(const UnitType* type,const UnitType* dest);

    /// Generate a unit reference, a printable unique string for unit
extern char* UnitReference(const Unit*);
    /// save unit-structure
extern void SaveUnit(const Unit* unit,FILE* file);
    /// save all units
extern void SaveUnits(FILE* file);

    /// Initialize unit module
extern void InitUnits(void);
    /// Clean unit module
extern void CleanUnits(void);

//	in unitcache.c
    /// Insert new unit into cache
extern void UnitCacheInsert(Unit* unit);
    /// Remove unit from cache
extern void UnitCacheRemove(Unit* unit);
    /// Change unit position in cache
extern void UnitCacheChange(Unit* unit);
    /// Select units in range
extern int UnitCacheSelect(int x1,int y1,int x2,int y2,Unit** table);
    /// Select units on tile
extern int UnitCacheOnTile(int x,int y,Unit** table);
    /// Select unit on X,Y of type naval,fly,land
extern Unit* UnitCacheOnXY(int x,int y,int type);
    /// Print unit-cache statistic
extern void UnitCacheStatistic(void);
    /// Initialize unit-cache
extern void InitUnitCache(void);

//	in map.c	belongs to map or unit??
    /// FIXME: more docu
extern int UnitMovement(const Unit* unit);
    /// FIXME: more docu
extern unsigned UnitFieldFlags(const Unit* unit);
    /// FIXME: more docu
extern int TypeMovementMask(const UnitType* type);
    /// FIXME: more docu
extern int UnitMovementMask(const Unit* unit);

//	in bottom_panel.c
//--------------------
    /// FIXME: more docu
extern void UpgradeButtons(int upgrade);

//	in unit_draw.c
//--------------------
    /// Draw nothing around unit
extern void DrawSelectionNone(const Unit* unit,const UnitType* type
	,int x,int y);
    /// Draw circle around unit
extern void DrawSelectionCircle(const Unit* unit,const UnitType* type
	,int x,int y);
    /// Draw circle filled with alpha around unit
extern void DrawSelectionCircleWithTrans(const Unit* unit,const UnitType* type
	,int x,int y);
    /// Draw rectangle around unit
extern void DrawSelectionRectangle(const Unit* unit,const UnitType* type
	,int x,int y);
    /// Draw rectangle filled with alpha around unit
extern void DrawSelectionRectangleWithTrans(const Unit* unit
	,const UnitType* type,int x,int y);
    /// Draw corners around unit
extern void DrawSelectionCorners(const Unit* unit,const UnitType* type
	,int x,int y);

    /// Load the decorations (health,mana) of units
extern void LoadDecorations(void);
    /// Save the decorations (health,mana) of units
extern void SaveDecorations(FILE* file);
    /// Clean the decorations (health,mana) of units
extern void CleanDecorations(void);
    /// Draw all units visible on map
extern void DrawUnits(void);

//	in unit_find.c
    /// Select units in rectangle range
extern int SelectUnits(int x1,int y1,int x2,int y2,Unit** table);
    /// Select units on map tile
extern int SelectUnitsOnTile(int x,int y,Unit** table);

    /// Find all units of this type
extern int FindUnitsByType(const UnitType* type,Unit** table);
    /// Find all units of this type of the player
extern int FindPlayerUnitsByType(const Player*,const UnitType*,Unit**);
    /// Return any unit on that map tile
extern Unit* UnitOnMapTile(unsigned tx,unsigned ty);
    /// Return repairable unit on that map tile
extern Unit* RepairableOnMapTile(unsigned tx,unsigned ty);
    /// Return possible attack target on that map tile
extern Unit* TargetOnMapTile(const Unit* unit,unsigned tx,unsigned ty);
    /// Return transporter unit on that map tile
extern Unit* TransporterOnMapTile(unsigned tx,unsigned ty);
    /// Return any unit on that screen map position
extern Unit* UnitOnScreenMapPosition (unsigned , unsigned );
    /// Return repairable unit on that screen map position
extern Unit* RepairableOnScreenMapPosition (unsigned , unsigned );
    /// Return possible attack target on that screen map position
extern Unit* TargetOnScreenMapPosition (const Unit* unit,unsigned , unsigned );
    /// Return transporter unit on that screen map position
extern Unit* TransporterOnScreenMapPosition (unsigned , unsigned );

    /// Return gold mine, if on map tile
extern Unit* GoldMineOnMap(int tx,int ty);
    /// Return gold deposit, if on map tile
extern Unit* GoldDepositOnMap(int tx,int ty);
    /// Return oil patch, if on map tile
extern Unit* OilPatchOnMap(int tx,int ty);
    /// Return oil platform, if on map tile
extern Unit* PlatformOnMap(int tx,int ty);
    /// Return oil deposit, if on map tile
extern Unit* OilDepositOnMap(int tx,int ty);
    /// Return wood deposit, if on map tile
extern Unit* WoodDepositOnMap(int tx,int ty);

    /// Find any enemy in numeric range
extern Unit* EnemyInRage(const Unit* unit,unsigned range);
    /// Find best enemy in numeric range to attack
extern Unit* AttackUnitsInDistance(const Unit* unit,unsigned range);
    /// Find best enemy in attack range to attack
extern Unit* AttackUnitsInRange(const Unit* unit);
    /// Find best enemy in reaction range to attack
extern Unit* AttackUnitsInReactRange(const Unit* unit);

//      in groups.c

    /// Initialize data structures for groups
extern void InitGroups(void);
    /// Save groups
extern void SaveGroups(FILE* file);
    /// Cleanup groups
extern void CleanGroups(void);

    // 2 functions to conseal the groups internal data structures...
    /// Get the number of units in a particular group
extern int GetNumberUnitsOfGroup(int num);
    /// Get the array of units of a particular group
extern Unit** GetUnitsOfGroup(int num);

    /// Remove all units from a group
extern void ClearGroup(int num);
    /// Add the array of units to the group
extern void AddToGroup(Unit **units,int nunits,int num);
    /// Set the contents of a particular group with an array of units
extern void SetGroup(Unit **units,int nunits,int num);
    /// Remove a unit from a group
extern void RemoveUnitFromGroups(Unit *unit);
    /// Register CCL group features
extern void GroupCclRegister(void);

//	in selection.c

    /// Check if unit is the currently only selected
#define IsOnlySelected(unit)	(NumSelected==1 && Selected[0]==(unit))

    /// Clear current selection
extern void UnSelectAll(void);
    /// Select group as selection
extern void ChangeSelectedUnits(Unit** units,int num_units);
    /// Add a unit to selection
extern int SelectUnit(Unit* unit);
    /// Select one unit as selection
extern void SelectSingleUnit(Unit* unit);
    /// Remove a unit from selection
extern void UnSelectUnit(Unit* unit);
    /// Add a unit to selected if not already selected, remove it otherwise
extern int ToggleSelectUnit(Unit* unit);
    /// Select units from the same type (if selectable by rectangle)
extern int SelectUnitsByType(Unit* base);
    /// Toggle units from the same type (if selectable by rectangle)
extern int ToggleUnitsByType(Unit* base);
    /// Select the units belonging to a particular group
extern int SelectGroup(int group_number);
    /// Select the unit from the same group as the one in parameter
extern int SelectGroupFromUnit(Unit *unit);
    /// Select the units in the selection rectangle
extern int SelectUnitsInRectangle(int tx,int ty,int w,int h);
    /// Select ground units in the selection rectangle
extern int SelectGroundUnitsInRectangle(int tx,int ty,int w,int h);
    /// Select flying units in the selection rectangle
extern int SelectAirUnitsInRectangle(int tx,int ty,int w,int h);
    /// Add the units in the selection rectangle to the current selection
extern int AddSelectedUnitsInRectangle(int tx,int ty,int w,int h);
    /// Add ground units in the selection rectangle to the current selection
extern int AddSelectedGroundUnitsInRectangle(int tx,int ty,int w,int h);
    /// Add flying units in the selection rectangle to the current selection
extern int AddSelectedAirUnitsInRectangle(int tx,int ty,int w,int h);

    /// Init selections
extern void InitSelections(void);
    /// Save current selection state
extern void SaveSelections(FILE* file);
    /// Clean up selections
extern void CleanSelections(void);
    /// Register CCL selection features
extern void SelectionCclRegister(void);

//	in ccl_unit.c

    /// register CCL units features
extern void UnitCclRegister(void);

//@}

#endif	// !__UNIT_H__
