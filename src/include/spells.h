//       _________ __                 __                               
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/ 
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//	   Stratagus - A free fantasy real time strategy game engine
//
/**@name spells.h	-	The Spells. */
//
//	(c) Copyright 1999-2003 by Vladi Belperchinov-Shabanski and Joris DAUPHIN
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

#ifndef __SPELLS_H__
#define __SPELLS_H__

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>

#include "stratagus.h"
#include "sound_id.h"
#include "sound.h"
#include "unittype.h"
#include "unit.h"
#include "missile.h"
#include "ccl.h"

/*----------------------------------------------------------------------------
--	Definitons
----------------------------------------------------------------------------*/

//	TODO : Remove, configure in ccl.
#define FIREBALL_DAMAGE		20	/// Damage of center fireball
#define WHIRLWIND_DAMAGE1	 4	/// the center of the whirlwind
#define WHIRLWIND_DAMAGE2	 1	/// the periphery of the whirlwind
#define RUNE_DAMAGE		50	/// Rune damage

enum {
    flag_slow,
    flag_haste,
    flag_bloodlust,
    flag_invisibility,
    flag_unholyarmor,
    flag_flameshield,
    flag_HP,
    flag_Mana,
    flag_HP_percent,
    flag_Mana_percent,
    flag_coward,
    flag_organic,
    flag_isundead,
    flag_canattack,
    flag_building
};

/**
**	Different targets.
*/
typedef enum {
	TargetSelf,
	TargetNone,
	TargetPosition,
	TargetUnit
#if 0
		,
	TargetUnits
#endif
}	TargetType;




typedef union
{
// FIXME rename structure more properly.
// TTL's below are in ticks: approx: 500=13sec, 1000=25sec, 2000=50sec
// FIXME use TTL, as in TICKS to live
    struct {
	int Fields;		/// The size of the affected square
	int Shards;		/// Number of shards thrown.
	int Damage;		/// Damage for every shard.
	/// The offset of the missile start point to the hit location.
	int StartOffsetX;
	int StartOffsetY;
    } AreaBombardment;
    
    struct {
	UnitType *PortalType;	/// The unit type spawned
    } SpawnPortal;
    
    struct {
	int TTL;		/// time to live (ticks)
	int Damage;		/// Damage.
    } Fireball;
    
    struct {
	int TTL;		/// time to live (ticks)
    } FlameShield;

    struct {
	int HasteTicks;		/// Number of ticks to set Haste to.
	int SlowTicks;		/// Number of ticks to set Slow to.
	int BloodlustTicks;	/// Number of ticks to set Bloodlust to.
	int InvisibilityTicks;	/// Number of ticks to set Invisibility to.
	int InvincibilityTicks;	/// Number of ticks to set UnholyArmor to.
#define BUFF_NOT_AFFECTED 0xC0FF33 /// Don't like the value? The value doesn't like you!
    } AdjustBuffs;
    
    struct {
	int HP;			/// Target HP gain.(can be negative)
	int Mana;		/// Target Mana gain.(can be negative)
	/// This spell is designed to be used wit very small amounts. The spell
	/// can scale up to MaxMultiCast times. Use 0 for infinite.
	int MaxMultiCast; 
    } AdjustVitals;
    
    struct {
	UnitType *revealer;	/// Type of unit to be summoned: (unit-revealer).
    } HolyVision;
    
    struct {
	UnitType *NewForm;	/// The new form
	//  TODO: temporary polymorphs would be awesome, but hard to implement
    } Polymorph;
    
    struct {
	UnitType *UnitType;	/// Type of unit to be summoned.
	int TTL;		/// Time to live for summoned unit. 0 means infinite
    } Summon;
    
    struct {
	UnitType *UnitRaised;	/// The unit to spawn from corpses
	int TTL;		/// Time to live for summon. 0 means infinite.
    } RaiseDead;
    //  What about a resurection spell?

    struct {
	int TTL;		/// time to live (ticks)
	int Damage;		/// Damage.
    } Runes;
    
    struct {
	int  TTL;		/// time to live (ticks)
	// FIXME: more configurations
    } Whirlwind;
} SpellActionType;

/*
** *******************
** Target definition.
** *******************
*/

typedef struct {
    TargetType	which_sort_of_target;	/// for identify what sort of target.
    int X;			/// x coord.
    int Y;			/// y coord.
    Unit	*unit;	/// Unit target.
} Target;

/*
** *******************
** Conditions definition.
** *******************
*/

/**
**	Informations about the conditions of autocasting mode and simple cast.
**
**	@todo	Move more parameters into this structure.
*/
struct s_Conditions;

/*
**	 Specific conditions.
*/
typedef	int f_specific_condition(const struct s_Conditions *condition,
	const Unit* caster,const Unit* target,int x,int y);

typedef	int f_generic_condition(const struct s_Conditions *condition,const Unit* caster);


/**
**	Informations about the condition of autocasting mode.
**
**	@todo	Move more parameters into this structure.
*/
typedef struct s_Conditions {
    int expectvalue;				/// Value expected (function expected result, true or false)
    union {
	f_specific_condition *specific;		/// Evaluation Function for the condition.
	f_generic_condition  *generic;		/// Evaluation Function for the condition.
    } f;
    union {
	int range;	/// range
//	struct {
//	    t_SpecificConditions c1;
//	    t_SpecificConditions c2;
//	} or;		//
	unsigned int flag;	///< flag
	struct {
	    unsigned int	flag;
	    unsigned int	ttl;
	} durationeffect;	///< durationeffect
    } u;
    struct s_Conditions	*next;	///< for list.
} t_Conditions;


/**
**	Informations about the autocasting mode.
**
*/
typedef struct {
	t_Conditions	*Condition_generic;		///< Conditions to cast the spell. (generic (no test for each target))
	t_Conditions	*Condition_specific;	///< Conditions to cast the spell. (target specific (ex:Hp full))
// Something to have a position target
	int				Range;							/// Max range of the target.
#if 0	// When sort supported
	t_f_order			*f;							/// Sort functions for the best target.
#endif
} t_AutoCast;

struct _spell_type_;

/*
**	Pointer on function that cast the spell.
*/
typedef int SpellFunc(Unit* caster, const struct _spell_type_* spell, Unit* target,
	int x, int y);

/**
**	Base structure of a spell type.
*/
typedef struct _spell_type_ {
    //  Identification stuff
    int Ident;				/// Spell numeric identifier
    char *IdentName;			/// Spell unique identifier (spell-holy-vision)
    char *Name;				/// Spell name shown by the engine

    //	Spell Specifications
    TargetType	Target;			/// Targetting information. See TargetType.
    SpellFunc *CastFunction;		/// function to cast the spell.
    SpellActionType *SpellAction;	/// More arguments for spell (damage, delay, additional sounds...).
    int Range;				/// Max range of the target.
    int ManaCost;			/// required mana for each cast

    int DependencyId;			/// Id of upgrade, -1 if no upgrade needed for cast the spell.
    t_Conditions *Condition_generic;	/// Conditions to cast the spell. (generic (no test for each target))
    t_Conditions *Condition_specific;	/// Conditions to cast the spell. (target specific (ex:Hp full))
//	Autocast	// FIXME : can use different for AI ? Use it in this structure ?
    t_AutoCast	*AutoCast;					/// AutoCast information

//	Uses for graphics and sounds
    SoundConfig SoundWhenCast;		/// sound played if cast
    MissileType	*Missile;		/// missile fired on cast
} SpellType;

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/**
**	Define the names and effects of all available spells.
*/
extern SpellType *SpellTypeTable;

/// How many spell-types are available
extern int SpellTypeCount;


/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

///	register fonction.
extern void SpellCclRegister(void);

/// init spell tables
extern void InitSpells(void);

/// save spell tables
extern void SaveSpells(CLFile * file);

/// done spell tables
extern void DoneSpells(void);

/// return 1 if spell is availible, 0 if not (must upgrade)
extern int SpellIsAvailable(const Player* player, int SpellId);

/// returns != 0 if spell can be casted (enough mana, valid target)
extern int CanCastSpell(const Unit* caster, const SpellType*,
						const Unit* target, int x, int y);

/// cast spell on target unit or place at x,y
extern int SpellCast(Unit* caster, const SpellType*,
					Unit* target, int x, int y);

/// auto cast the spell if possible
extern int AutoCastSpell(Unit* caster, const SpellType* spell);

/// returns != 0 if spell can be auto cast
extern int CanAutoCastSpell(const SpellType* spell);

/// return spell id by ident string
extern int SpellIdByIdent(const char* Ident);

/// return spell type by ident string
extern SpellType* SpellTypeByIdent(const char* Ident);

/// return spell type by spell id
extern SpellType* SpellTypeById(int Id);

extern unsigned CclGetSpellByIdent(SCM value);

/*
**	Spelltype to cast.
*/

SpellFunc CastHolyVision;
SpellFunc CastAdjustVitals;
SpellFunc CastAdjustBuffs;
SpellFunc CastFireball;
SpellFunc CastFlameShield;
SpellFunc CastPolymorph;
SpellFunc CastAreaBombardment;
SpellFunc CastSummon;
SpellFunc CastRunes;
SpellFunc CastDeathCoil;
SpellFunc CastRaiseDead;
SpellFunc CastWhirlwind;
SpellFunc CastSpawnPortal;


/*
**	generic condition.
*/
f_generic_condition	CheckEnemyPresence;

/*
**	Specific condition.
*/
f_specific_condition	CheckUnitTypeFlag;
f_specific_condition	CheckAllied;
f_specific_condition	CheckUnitDurationEffect;

//@}

#endif	// !__SPELLS_H__
