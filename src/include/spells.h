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
/**@name spells.h - The Spells. */
//
//      (c) Copyright 1999-2005 by Vladi Belperchinov-Shabanski and Joris DAUPHIN
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

#ifndef __SPELLS_H__
#define __SPELLS_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "unitsound.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUnit;
struct _unit_type_;
struct _player_;
struct lua_State;
class SpellType;

/*----------------------------------------------------------------------------
--  Definitons
----------------------------------------------------------------------------*/

/**
**  Different targets.
*/
typedef enum {
	TargetSelf,
	TargetPosition,
	TargetUnit
}  TargetType;

/**
**  Different targets.
*/
typedef enum {
	LocBaseCaster,
	LocBaseTarget
}  LocBaseType;

/**
**  This struct is used for defining a missile start/stop location.
**
**  It's evaluated like this, and should be more or less flexible.:
**  base coordinates(caster or target) + (AddX,AddY) + (rand()%AddRandX,rand()%AddRandY)
*/
class SpellActionMissileLocation {
public:
	SpellActionMissileLocation(LocBaseType base) : Base(base), AddX(0), AddY(0),
								   AddRandX(0), AddRandY(0) {} ;

	LocBaseType Base;   /// The base for the location (caster/target)
	int AddX;           /// Add to the X coordinate
	int AddY;           /// Add to the X coordinate
	int AddRandX;       /// Random add to the X coordinate
	int AddRandY;       /// Random add to the X coordinate
};

class SpellActionTypeAdjustVariable {
public:
	SpellActionTypeAdjustVariable() : Enable(0), Value(0), Max(0), Increase(0),
									ModifEnable(0), ModifValue(0),
									ModifMax(0), ModifIncrease(0),
									InvertEnable(0), AddValue(0),
									AddMax(0), AddIncrease(0), IncreaseTime(0),
									TargetIsCaster(0) {};

	int Enable;                 /// Value to affect to this field.
	int Value;                  /// Value to affect to this field.
	int Max;                    /// Value to affect to this field.
	int Increase;               /// Value to affect to this field.

	char ModifEnable;           /// true if we modify this field.
	char ModifValue;            /// true if we modify this field.
	char ModifMax;              /// true if we modify this field.
	char ModifIncrease;         /// true if we modify this field.

	char InvertEnable;          /// true if we invert this field.
	int AddValue;               /// Add this value to this field.
	int AddMax;                 /// Add this value to this field.
	int AddIncrease;            /// Add this value to this field.
	int IncreaseTime;           /// How many time increase the Value field.
	char TargetIsCaster;        /// true if the target is the caster.
};


/**
**  Generic spell action virtual class.
**  Spells are sub class of this one
*/
class SpellActionType {
protected:
	typedef int SpellFunc(CUnit *caster, const SpellType* spell,
		CUnit *target, int x, int y);

public:
	SpellActionType(int mod = 0) : ModifyManaCaster(mod) {};
	virtual ~SpellActionType() {};
	virtual SpellFunc Cast = 0;
	const int ModifyManaCaster;
};

//
//  Specific spells.
//

class AreaAdjustVitals : public SpellActionType
{
public:
	AreaAdjustVitals() : HP(0), Mana(0) {};
	virtual SpellFunc Cast;

	int HP;         /// Target HP gain.(can be negative)
	int Mana;       /// Target Mana gain.(can be negative)
} ;

class SpawnMissile : public SpellActionType {
public:
	SpawnMissile() : Damage(0), TTL(-1), Delay(0),
					StartPoint(LocBaseCaster), EndPoint(LocBaseTarget), Missile(0) {};
	virtual SpellFunc Cast;

	int Damage;                             /// Missile damage.
	int TTL;                                /// Missile TTL.
	int Delay;                              /// Missile original delay.
	SpellActionMissileLocation StartPoint;  /// Start point description.
	SpellActionMissileLocation EndPoint;    /// Start point description.
	MissileType* Missile;                   /// Missile fired on cast
};

class Demolish : public SpellActionType {
public:
	Demolish() : Damage(0), Range(0) {};
	virtual SpellFunc Cast;

	int Damage; /// Damage for every unit in range.
	int Range;  /// Range of the explosion.
};

class AreaBombardment : public SpellActionType {
public:
	AreaBombardment() : Fields(0), Shards(0), Damage(0),
						StartOffsetX(0), StartOffsetY(0), Missile(NULL) {};
	virtual SpellFunc Cast;

	int Fields;             /// The size of the affected square.
	int Shards;             /// Number of shards thrown.
	int Damage;             /// Damage for every shard.
	int StartOffsetX;       /// The offset of the missile start point to the hit location.
	int StartOffsetY;       /// The offset of the missile start point to the hit location.
	MissileType* Missile;   /// Missile fired on cast
};

class SpawnPortal : public SpellActionType {
public:
	SpawnPortal() : PortalType(0) {};
	virtual SpellFunc Cast;

	struct _unit_type_* PortalType;   /// The unit type spawned
};

class AdjustVariable : public SpellActionType {
public:
	AdjustVariable() : Var (NULL){};
	~AdjustVariable() {delete [] (this->Var);};
	virtual SpellFunc Cast;

	SpellActionTypeAdjustVariable *Var;
};

class AdjustVitals : public SpellActionType {
public:
	AdjustVitals() : SpellActionType(1), HP(0), Mana(0), MaxMultiCast(0) {};
	virtual SpellFunc Cast;

	int HP;         /// Target HP gain.(can be negative)
	int Mana;       /// Target Mana gain.(can be negative)
	/// This spell is designed to be used wit very small amounts. The spell
	/// can scale up to MaxMultiCast times. Use 0 for infinite.
	int MaxMultiCast;
};

class Polymorph : public SpellActionType {
public:
	Polymorph() : SpellActionType(1), NewForm(NULL), PlayerNeutral(0) {};
	virtual SpellFunc Cast;

	struct _unit_type_* NewForm;/// The new form
	int PlayerNeutral;          /// Convert the unit to the neutral player.
	// TODO: temporary polymorphs would be awesome, but hard to implement
};

class Summon : public SpellActionType {
public:
	Summon() : SpellActionType(1), UnitType(NULL), TTL(0), RequireCorpse(0) {} ;
	virtual SpellFunc Cast;

	struct _unit_type_* UnitType;/// Type of unit to be summoned.
	int TTL;                /// Time to live for summoned unit. 0 means infinite
	int RequireCorpse;      /// Corpse consumed while summoning.
};

class Capture : public SpellActionType {
public:
	Capture() : SacrificeEnable(0), Damage(0), DamagePercent(0) {};
	virtual SpellFunc Cast;

	char SacrificeEnable; /// true if the caster dies after casting.
	int Damage;           /// damage the spell does if unable to caputre
	int DamagePercent;    /// percent the target must be damaged for a
						  /// capture to suceed.
};

/*
** *******************
** Target definition.
** *******************
*/

typedef struct {
	TargetType which_sort_of_target;  /// for identify what sort of target.
	int X;                            /// x coord.
	int Y;                            /// y coord.
	CUnit *unit;                      /// Unit target.
} Target;

/*
** *******************
** Conditions definition.
** *******************
*/

class ConditionInfoVariable {
public:
	ConditionInfoVariable() : Enable(0), MinValue(0), MaxValue(0),
							MinMax(0), MinValuePercent(0), MaxValuePercent(0),
							ConditionApplyOnCaster(0) {};

	char Enable;                /// Target is 'user defined variable'.

	int MinValue;               /// Target must have more Value than that.
	int MaxValue;               /// Target must have less Value than that.
	int MinMax;                 /// Target must have more Max than that.
	int MinValuePercent;        /// Target must have more (100 * Value / Max) than that.
	int MaxValuePercent;        /// Target must have less (100 * Value / Max) than that.

	char ConditionApplyOnCaster; /// true if these condition are for caster.
	// FIXME : More (increase, MaxMax) ?
};

/**
**  Conditions for a spell.
**
**  @todo  Move more parameters into this structure.
*/
class ConditionInfo {
public:
	ConditionInfo() : Alliance(0), Opponent(0), TargetSelf(0),
					BoolFlag(NULL), Variable(NULL) {};
	~ConditionInfo() { delete [] BoolFlag; delete [] Variable; };
	//
	//  Conditions that check specific flags. Possible values are the defines below.
	//
#define CONDITION_FALSE 1
#define CONDITION_TRUE  0
#define CONDITION_ONLY  2
	char Alliance;          /// Target is allied. (neutral is neither allied, nor opponent)
	char Opponent;          /// Target is opponent. (neutral is neither allied, nor opponent)
	char TargetSelf;        /// Target is the same as the caster.
	char* BoolFlag;         /// User defined boolean flag.

	ConditionInfoVariable* Variable;
	//
	//  @todo more? feel free to add, here and to
	//  @todo PassCondition, CclSpellParseCondition, SaveSpells
	//
};

/**
**  Informations about the autocasting mode.
*/
class AutoCastInfo {
public:
	AutoCastInfo() : Range(0), Condition(0), Combat(0) {};
	~AutoCastInfo() {delete Condition;};
	/// @todo this below is SQUARE!!!
	int Range;                   /// Max range of the target.

	ConditionInfo* Condition;    /// Conditions to cast the spell.

	/// Detalied generic conditions (not per-target, where Condition is evaluated.)
	/// Combat mode is when there are hostile non-coward units around
	int Combat;                  /// If it should be casted in combat

	/// @todo Add stuff here for target preference.
	/// @todo Heal units with the lowest hit points first.
};

/**
**  Base structure of a spell type.
*/
class SpellType {
public:
	SpellType(int slot, const char *identname);
	~SpellType();

	// Identification stuff
	char* Ident;    /// Spell unique identifier (spell-holy-vision)
	char* Name;     /// Spell name shown by the engine
	int Slot;       /// Spell numeric identifier

	// Spell Specifications
	TargetType Target;          /// Targetting information. See TargetType.
	std::vector<SpellActionType *> Action; /// More arguments for spell (damage, delay, additional sounds...).

	int Range;                  /// Max range of the target.
#define INFINITE_RANGE 0xFFFFFFF
	int ManaCost;               /// Required mana for each cast.
	int RepeatCast;             /// If the spell will be cast again until out of targets.

	int DependencyId;           /// Id of upgrade, -1 if no upgrade needed for cast the spell.
	ConditionInfo *Condition;   /// Conditions to cast the spell. (generic (no test for each target))

	// Autocast informations. No AICast means the AI use AutoCast.
	AutoCastInfo* AutoCast;     /// AutoCast information for your own units
	AutoCastInfo* AICast;       /// AutoCast information for ai. More detalied.

	// Graphics and sounds. Add something else here?
	SoundConfig SoundWhenCast;  /// Sound played if cast
};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/**
**  Define the names and effects of all available spells.
*/
extern std::vector<SpellType*> SpellTypeTable;


/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

	/// register fonction.
extern void SpellCclRegister(void);

	/// init spell tables
extern void InitSpells(void);

	/// done spell tables
extern void CleanSpells(void);

	/// return 1 if spell is availible, 0 if not (must upgrade)
extern int SpellIsAvailable(const struct _player_* player, int SpellId);

	/// returns != 0 if spell can be casted (enough mana, valid target)
extern int CanCastSpell(const CUnit *caster, const SpellType* spell,
	const CUnit *target, int x, int y);

	/// cast spell on target unit or place at x,y
extern int SpellCast(CUnit *caster, const SpellType* spell,
	CUnit *target, int x, int y);

	/// auto cast the spell if possible
extern int AutoCastSpell(CUnit *caster, const SpellType* spell);

	/// return spell type by ident string
extern SpellType* SpellTypeByIdent(const char* Ident);

	/// return 0, 1, 2 for true, only, false.
extern char Ccl2Condition(struct lua_State* l, const char* value);

//@}

#endif // !__SPELLS_H__
