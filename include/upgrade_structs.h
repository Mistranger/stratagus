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
/**@name upgrade_structs.h	-	The upgrade/allow headerfile. */
//
//	(c) Copyright 1999-2003 by Vladi Belperchinov-Shabanski and
//	                           Jimmy Salmon
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

#ifndef __UPGRADE_STRUCTS_H__
#define __UPGRADE_STRUCTS_H__

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include "icons.h"

/*----------------------------------------------------------------------------
--	Defines
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/


/**
**	Indices into costs/resource/income array. (A litte future here :)
*/
enum _costs_ {
    TimeCost,				/// time in game cycles

// standard
    GoldCost,				/// gold  resource
    WoodCost,				/// wood  resource
    OilCost,				/// oil   resource
// extensions
    OreCost,				/// ore   resource
    StoneCost,				/// stone resource
    CoalCost,				/// coal  resource

    MaxCosts				/// how many different costs
};

#define FoodCost MaxCosts 
#define ScoreCost MaxCosts+1 

/**
**	Speed factor for harvesting resources
*/
extern int SpeedResourcesHarvest[MaxCosts];

/**
**	Speed factor for returning resources
*/
extern int SpeedResourcesReturn[MaxCosts];

/**
**	Default resources for a new player.
*/
extern int DefaultResources[MaxCosts];

/**
**	Default resources for a new player with low resources.
*/
extern int DefaultResourcesLow[MaxCosts];

/**
**	Default resources for a new player with mid resources.
*/
extern int DefaultResourcesMedium[MaxCosts];

/**
**	Default resources for a new player with high resources.
*/
extern int DefaultResourcesHigh[MaxCosts];

/**
**	Default incomes for a new player.
*/
extern int DefaultIncomes[MaxCosts];

/**
**	Default action for the resources.
*/
extern char* DefaultActions[MaxCosts];

/**
**	Default names for the resources.
*/
extern char* DefaultResourceNames[MaxCosts];

/**
**	This are the current stats of an unit. Upgraded or downgraded.
*/
typedef struct _unit_stats_ {
    int		AttackRange;		/// how far can the unit attack
    int		SightRange;		/// how far can the unit see
    int		Armor;			/// armor strength
    int		BasicDamage;		/// weapon basic damage
    int		PiercingDamage;		/// weapon piercing damage
    int		Speed;			/// movement speed
    int		HitPoints;		/// hit points
    int		Costs[MaxCosts];	/// current costs of the unit
    int		Level;			/// unit level (upgrades)
} UnitStats;

/**
**	The main useable upgrades.
*/
typedef struct _upgrade_ {
    const void*	OType;			/// object type (future extensions)
    char*	Ident;			/// identifier
    int		Costs[MaxCosts];	/// costs for the upgrade
	// FIXME: not used by buttons
    IconConfig	Icon;			/// icon to display to the user
} Upgrade;

/*----------------------------------------------------------------------------
--	upgrades and modifiers
----------------------------------------------------------------------------*/

/**
**	Modifiers of the unit stats.
**	All the following are modifiers not values!
**	@see UnitStats
*/
typedef struct _modifiers_ {
    int	AttackRange;			/// attack range modifier
    int	SightRange;			/// sight range modifier
    int	BasicDamage;			/// basic damage modifier
    int	PiercingDamage;			/// piercing damage modifier
    int	Armor;				/// armor modifier
    int	Speed;				/// speed modifier (FIXME: not working)
    int	HitPoints;			/// hit points modifier

    int	Costs[MaxCosts];		/// costs modifier
} Modifiers;

/**
**	This is the modifier of an upgrade.
**	This do the real action of an upgrade, an upgrade can have multiple
**	modifiers.
*/
typedef struct _upgrade_modifier_ {

  int		UpgradeId;		/// used to filter required modifier

  Modifiers	Modifier;		/// modifier of unit stats

  // allow/forbid bitmaps -- used as chars for example:
  // `?' -- leave as is, `F' -- forbid, `A' -- allow
  // FIXME: see below allow more semantics?
  // FIXME: pointers or ids would be faster and less memory use
  char	ChangeUnits[UnitTypeMax];	/// allow/forbid units
  char	ChangeUpgrades[UpgradeMax];	/// allow/forbid upgrades
  char	ApplyTo[UnitTypeMax];		/// which unit types are affected

  // FIXME: UnitType*
  void*		ConvertTo;		/// convert to this unit-type.

} UpgradeModifier;

/**
**	Allow what a player can do. Every #Player has an own allow struct.
**
**	This could allow/disallow units, actions or upgrades.
**
**	Values are:
**		@li `A' -- allowed,
**		@li `F' -- forbidden,
**		@li `R' -- acquired, perhaps other values
**		@li `Q' -- acquired but forbidden (does it make sense?:))
**		@li `E' -- enabled, allowed by level but currently forbidden
***		@li `X' -- fixed, acquired can't be disabled
*/
typedef struct _allow_ {
    char	Units[UnitTypeMax];	/// units allowed/disallowed
    char	Upgrades[UpgradeMax];	/// upgrades allowed/disallowed
} Allow;

/**
**	Upgrade timer used in the player structure.
**	Every player has an own UpgradeTimers struct.
*/
typedef struct _upgrade_timers_ {

    /**
    **	all 0 at the beginning, all upgrade actions do increment values in
    **	this struct.
    */
    int	Upgrades[UpgradeMax];		/// counter for each upgrade

} UpgradeTimers;

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern const char UpgradeType[];	/// upgrade type
extern Upgrade Upgrades[UpgradeMax];	/// the main user useable upgrades

//@}

#endif // !__UPGRADE_STRUCTS_H__
