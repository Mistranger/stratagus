//   ___________		     _________		      _____  __
//   \_	  _____/______   ____   ____ \_   ___ \____________ _/ ____\/  |_
//    |    __) \_  __ \_/ __ \_/ __ \/    \  \/\_  __ \__  \\   __\\   __\ 
//    |     \   |  | \/\  ___/\  ___/\     \____|  | \// __ \|  |   |  |
//    \___  /   |__|    \___  >\___  >\______  /|__|  (____  /__|   |__|
//        \/		    \/	   \/	     \/		   \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//	   FreeCraft - A free fantasy real time strategy game engine
//
/**@name spells.h	-	The Spells. */
//
//	(c) Copyright 1999-2001 by Vladi Belperchinov-Shabanski
//
//	$Id$

#ifndef __SPELLS_H__
#define __SPELLS_H__

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>

#include "freecraft.h"
#include "sound_id.h"
#include "sound.h"
#include "unittype.h"
#include "unit.h"
#include "missile.h"

/*----------------------------------------------------------------------------
--	Definitons
----------------------------------------------------------------------------*/

/**
**	Different spell actions, defines the behavior of the spells.
*/
typedef enum _spell_action_type_ {
    SpellActionNone,			/// FIXME: Comments
//	---human paladins---
    SpellActionHolyVision,		/// FIXME: Comments
    SpellActionHealing,			/// FIXME: Comments
    SpellActionExorcism,		/// FIXME: Comments
//	---human mages---
    SpellActionFireball,		/// FIXME: Comments
    SpellActionSlow,			/// FIXME: Comments
    SpellActionFlameShield,		/// FIXME: Comments
    SpellActionInvisibility,		/// FIXME: Comments
    SpellActionPolymorph,		/// FIXME: Comments
    SpellActionBlizzard,		/// FIXME: Comments
//	---orc ogres---
    SpellActionEyeOfKilrogg,		/// FIXME: Comments
    SpellActionBloodlust,		/// FIXME: Comments
    SpellActionRunes,			/// FIXME: Comments
//	---orc death knights---
    SpellActionDeathCoil,		/// FIXME: Comments
    SpellActionHaste,			/// FIXME: Comments
    SpellActionRaiseDead,		/// FIXME: Comments
    SpellActionWhirlwind,		/// FIXME: Comments
    SpellActionUnholyArmor,		/// FIXME: Comments
    SpellActionDeathAndDecay,		/// FIXME: Comments

    // Here you can new spell actions

} SpellActionType;

/**
**	Base structure of a spell type
**
**	@todo	Move more parameters into this structure.
*/
typedef struct _spell_type_ {

    char* Ident;		/// spell identifier
    char* Name;			/// spell name shown by the engine

    int  Range;			/// spell range
    int  ManaCost;		/// required mana for each cast
    int  TTL;			/// time to live (ticks)

    SpellActionType  Action;	/// SpellAction*

    SoundConfig Casted;		/// sound played if casted
    MissileConfig Missile;	/// missile fired on cast
} SpellType;

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

    /// init spell tables
extern void InitSpells(void);

    /// done spell tables
extern void DoneSpells(void);

    /// return spell id by ident string
extern int SpellIdByIdent( const char* Ident );

    /// return spell type by ident string
extern const SpellType* SpellTypeByIdent( const char* Ident );

    /// return spell type by spell id
extern SpellType* SpellTypeById( int Id );

    /// returns != 0 if spell can be casted by this unit, enough mana?
extern int CanCastSpell( const Unit*, int, const Unit*, const Unit*,int, int );

    /// fire spell on target unit or place at x,y
extern int SpellCast( const SpellType* SpellId, Unit* unit, Unit* target,
	int x, int y );

//@}

#endif	// !__BUTTON_H__
