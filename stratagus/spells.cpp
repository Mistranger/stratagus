//       _________ __                 __                               
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/ 
//  ______________________                           ______________________
//			  T H E	  W A R	  B E G I N S
//	   Stratagus - A free fantasy real time strategy game engine
//
/**@name spells.c	-	The spell cast action. */
//
//	(c) Copyright 1998-2003 by Vladi Belperchinov-Shabanski, Lutz Sammer,
//	                           Jimmy Salmon and Joris DAUPHIN
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

/*
**	And when we cast our final spell
**	And we meet in our dreams
**	A place that no one else can go
**	Don't ever let your love die
**	Don't ever go breaking this spell
*/

//@{

/*----------------------------------------------------------------------------
--	Notes
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"

#include "spells.h"
#include "sound.h"
#include "sound_id.h"
#include "missile.h"
#include "map.h"
#include "ui.h"
#include "actions.h"

/*----------------------------------------------------------------------------
--	Definitons
----------------------------------------------------------------------------*/

// TODO Move this in missile.c and remove Hardcoded string.
MissileType *MissileTypeRune; // MissileTypeByIdent("missile-rune");

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/**
**	Define the names and effects of all im play available spells.
*/
global SpellType *SpellTypeTable;


/// How many spell-types are available
global int SpellTypeCount;

/*----------------------------------------------------------------------------
--	Functions (Spells Controllers/Callbacks)
----------------------------------------------------------------------------*/

// ****************************************************************************
// Action of the missile of spells
// ****************************************************************************

/*
** Missile controllers
**
** To cancel a missile set it's TTL to 0, it will be handled right after
** the controller call and missile will be down.
**
*/

// FIXME Move this codes into missile.c

/**
**	Fireball controller
**
**	@param missile	Controlled missile
**
**	@todo	Move this code into the missile code
*/
local void SpellFireballController(Missile *missile)
{
    Unit *table[UnitMax];
    int i;
    int n;
    int x;
    int y;

    //NOTE: vladi: TTL is used as counter for explosions
    // explosions start at target and continue (10 tiles) beyond
    // explosions are on each tile on the way

    // approx
    if (missile->TTL <= missile->State && missile->TTL % 2 == 0) {
	//+TileSize/2 to align gfx to baseline
	x = missile->X + TileSizeX / 2;
	y = missile->Y + TileSizeY / 2;

	MakeMissile(MissileTypeExplosion, x, y, x, y);

	x = x / TileSizeX;
	y = y / TileSizeY;

	// Effect of the explosion on units
	// NOTE: vladi: this is slightly different than original
	//      now it hits all units in range 1
	n = SelectUnits(x - 1, y - 1, x + 1, y + 1, table);
	for (i = 0; i < n; ++i)	{
	    if (table[i]->HP) {
		HitUnit(missile->SourceUnit, table[i], FIREBALL_DAMAGE); // Should be missile->damage
	    }
	}
    }
}

/**
**	Death-Coil controller
**
**	@param missile	Controlled missile
**
**	@todo	Move this code into the missile code
*/
local void SpellDeathCoilController(Missile *missile)
{
    Unit *table[UnitMax];
    int	i;
    int	n;
    Unit *source;

    //
    //  missile has not reached target unit/spot
    //
    if (!(missile->X == missile->DX && missile->Y == missile->DY)) {
	return ;
    }
    source = missile->SourceUnit;
    if (source->Destroyed) {
	return ;
    }
    // source unit still exists
    //
    //	Target unit still exists and casted on a special target
    //
    if (missile->TargetUnit && !missile->TargetUnit->Destroyed
	    && missile->TargetUnit->HP)  {
	if (missile->TargetUnit->HP <= 50) {// 50 should be parametrable
	    source->Player->Score += missile->TargetUnit->Type->Points;
	    if( missile->TargetUnit->Type->Building) {
		source->Player->TotalRazings++;
	    } else {
		source->Player->TotalKills++;
	    }
#ifdef USE_HP_FOR_XP
	    source->XP += missile->TargetUnit->HP;
#else
	    source->XP += missile->TargetUnit->Type->Points;
#endif
	    ++source->Kills;
	    missile->TargetUnit->HP = 0;
	    LetUnitDie(missile->TargetUnit);
	} else {
#ifdef USE_HP_FOR_XP
	    source->XP += 50;
#endif
	    missile->TargetUnit->HP -= 50;
	}
	if (source->Orders[0].Action != UnitActionDie) {
	    source->HP += 50;
	    if (source->HP > source->Stats->HitPoints) {
		source->HP = source->Stats->HitPoints;
	    }
	}
    } else {
	//
	//  No target unit -- try enemies in range 5x5 // Must be parametrable
	//
	int ec;		// enemy count
	int x;
	int y;

	ec = 0;
	x = missile->DX / TileSizeX;
	y = missile->DY / TileSizeY;

	n = SelectUnits(x - 2, y - 2, x + 2, y + 2, table);
	if (n == 0) {
	    return ;
	}
	// calculate organic enemy count
	for (i = 0; i < n; ++i) {
	    ec += (IsEnemy(source->Player, table[i])
	    && table[i]->Type->Organic != 0);
	}
	if (ec > 0)  {
	    // yes organic enemies found
	    for (i = 0; i < n; ++i) {
		if (IsEnemy(source->Player, table[i]) && table[i]->Type->Organic != 0) {
		    // disperse damage between them
		    //NOTE: 1 is the minimal damage
		    if (table[i]->HP <= 50 / ec ) {
			source->Player->Score += table[i]->Type->Points;
			if( table[i]->Type->Building ) {
			    source->Player->TotalRazings++;
			} else {
			    source->Player->TotalKills++;
			}
#ifdef USE_HP_FOR_XP
			source->XP += table[i]->HP;
#else
			source->XP += table[i]->Type->Points;
#endif
			++source->Kills;
			table[i]->HP = 0;
			LetUnitDie(table[i]); // too much damage
		    } else {
#ifdef USE_HP_FOR_XP
			source->XP += 50/ec;
#endif
			table[i]->HP -= 50 / ec;
		    }
		}
	    }
	    if (source->Orders[0].Action!=UnitActionDie) {
		source->HP += 50;
		if (source->HP > source->Stats->HitPoints) {
		    source->HP = source->Stats->HitPoints;
		}
	    }
	}
    }
}

/**
**	Whirlwind controller
**
**	@param missile	Controlled missile
**
**	@todo	Move this code into the missile code
*/
local void SpellWhirlwindController(Missile *missile)
{
    Unit *table[UnitMax];
    int i;
    int n;
    int x;
    int y;

    //
    //	Center of the tornado
    //
    x = (missile->X+TileSizeX/2+missile->Type->Width/2) / TileSizeX;
    y = (missile->Y+TileSizeY+missile->Type->Height/2) / TileSizeY;
    //
    //	Every 4 cycles 4 points damage in tornado center
    //
    if (!(missile->TTL % 4)) {
	n = SelectUnitsOnTile(x, y, table);
	for (i = 0; i < n; ++i)	{
	    if (table[i]->HP) {
		HitUnit(missile->SourceUnit,table[i], WHIRLWIND_DAMAGE1);// should be missile damage ?
	    }
	}
    }
    //
    //	Every 1/10s 1 points damage on tornado periphery
    //
    if (!(missile->TTL % (CYCLES_PER_SECOND/10))) {
    	// we should parameter this
	n = SelectUnits(x - 1, y - 1, x + 1, y + 1, table);
	DebugLevel3Fn("Damage on %d,%d-%d,%d = %d\n" _C_ x-1 _C_ y-1 _C_ x+1 _C_ y+1 _C_ n);
	for (i = 0; i < n; ++i) {
	    if( (table[i]->X != x || table[i]->Y != y) && table[i]->HP) {
		HitUnit(missile->SourceUnit,table[i], WHIRLWIND_DAMAGE2); // should be in missile
	    }
	}
    }
    DebugLevel3Fn( "Whirlwind: %d, %d, TTL: %d\n" _C_
	    missile->X _C_ missile->Y _C_ missile->TTL );

    //
    //	Changes direction every 3 seconds (approx.)
    //
    if (!(missile->TTL % 100)) { // missile has reached target unit/spot
	int nx;
	int ny;

	do {
	    // find new destination in the map
	    nx = x + SyncRand() % 5 - 2;
	    ny = y + SyncRand() % 5 - 2;
	} while (nx < 0 && ny < 0 && nx >= TheMap.Width && ny >= TheMap.Height);
	missile->DX = nx * TileSizeX + TileSizeX / 2;
	missile->DY = ny * TileSizeY + TileSizeY / 2;
	missile->State=0;
	DebugLevel3Fn( "Whirlwind new direction: %d, %d, TTL: %d\n" _C_
		missile->X _C_ missile->Y _C_ missile->TTL );
    }
}

/**
**	Runes controller
**
**	@param missile	Controlled missile
**
**	@todo	Move this code into the missile code
*/
local void SpellRunesController(Missile *missile)
{
    Unit *table[UnitMax];
    int i;
    int n;
    int x;
    int y;

    x = missile->X / TileSizeX;
    y = missile->Y / TileSizeY;

    n = SelectUnitsOnTile(x, y, table);
    for (i = 0; i < n; ++i) {
	if (table[i]->Type->UnitType != UnitTypeFly && table[i]->HP) {
	    // FIXME: don't use ident!!!
	    PlayMissileSound(missile, SoundIdForName("explosion"));
	    MakeMissile(MissileTypeExplosion, missile->X, missile->Y,
					missile->X, missile->Y);
	    HitUnit(missile->SourceUnit, table[i], RUNE_DAMAGE);
	    missile->TTL=0;		// Rune can only hit once
	}
    }
    // show rune every 4 seconds (approx.)
    if (missile->TTL % 100 == 0) {
	MakeMissile(MissileTypeRune, missile->X, missile->Y,missile->X, missile->Y);
    }
}

/**
**	FlameShield controller
**
**	@param missile	Controlled missile
**
**	@todo	Move this code into the missile code
*/
local void SpellFlameShieldController(Missile *missile)
{
    static int fs_dc[] = {
	0, 32, 5, 31, 10, 30, 16, 27, 20, 24, 24, 20, 27, 15, 30, 10, 31,
	5, 32, 0, 31, -5, 30, -10, 27, -16, 24, -20, 20, -24, 15, -27, 10,
	-30, 5, -31, 0, -32, -5, -31, -10, -30, -16, -27, -20, -24, -24, -20,
	-27, -15, -30, -10, -31, -5, -32, 0, -31, 5, -30, 10, -27, 16, -24,
	20, -20, 24, -15, 27, -10, 30, -5, 31, 0, 32};
    Unit *table[UnitMax];
    int n;
    int i;
    int dx;
    int dy;
    int ux;
    int uy;
    int ix;
    int iy;
    int uw;
    int uh;

    i = missile->TTL % 36;		// 36 positions on the circle
    dx = fs_dc[i * 2];
    dy = fs_dc[i * 2 + 1];
    ux = missile->TargetUnit->X;
    uy = missile->TargetUnit->Y;
    ix = missile->TargetUnit->IX;
    iy = missile->TargetUnit->IY;
    uw = missile->TargetUnit->Type->Width;
    uh = missile->TargetUnit->Type->Height;
    missile->X = ux * TileSizeX + ix + uw / 2 + dx - 32;
    missile->Y = uy * TileSizeY + iy + uh / 2 + dy - 32 - 16;
    if (missile->TargetUnit->Orders[0].Action == UnitActionDie) {
	missile->TTL = i;
    }
    if (missile->TTL == 0) {
	missile->TargetUnit->FlameShield = 0;
    }
    //vladi: still no have clear idea what is this about :)
    CheckMissileToBeDrawn(missile);

    // Only hit 1 out of 8 frames
    if (missile->TTL & 7) {
	return;
    }
    n = SelectUnits(ux - 1, uy - 1, ux + 1 + 1, uy + 1 + 1, table);
    for (i = 0; i < n; ++i) {
	if (table[i] == missile->TargetUnit) {
	    // cannot hit target unit
	    continue;
	}
	if (table[i]->HP) {
	    HitUnit(missile->SourceUnit, table[i], 1);
	}
    }
}

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

// ****************************************************************************
// Cast the Spell
// ****************************************************************************

/**
**	Cast circle of power.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastSpawnPortal(Unit *caster, const SpellType *spell __attribute__((unused)),
    Unit *target __attribute__((unused)), int x, int y)
{
    // FIXME: vladi: cop should be placed only on explored land
    Unit *portal;
    UnitType *ptype;
    
    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    DebugCheck(!spell->Action->Data.SpawnPortal.PortalType);

    ptype = spell->Action->Data.SpawnPortal.PortalType;

    portal = caster->Goal;
    if (portal) {
	// FIXME: if cop is already defined --> move it, but it doesn't work?
	RemoveUnit(portal, NULL);
	PlaceUnit(portal, x, y);
    } else {
	portal = MakeUnitAndPlace(x, y, ptype, &Players[PlayerMax - 1]);
    }
    MakeMissile(spell->Missile,
	    x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2,
	    x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2);
    //  Goal is used to link to destination circle of power
    caster->Goal = portal;
    RefsDebugCheck(!portal->Refs || portal->Destroyed);
    portal->Refs++;
    //FIXME: setting destination circle of power should use mana
    return 0;
}

//	AreaBombardment
//   NOTE: vladi: blizzard differs than original in this way:
//   original: launches 50 shards at 5 random spots x 10 for 25 mana.

/**
**	Cast area bombardment.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastAreaBombardment(Unit *caster, const SpellType *spell,
    Unit *target __attribute__((unused)), int x, int y)
{
    int fields;
    int shards;
    int damage;
    Missile *mis;
    int offsetx;
    int offsety;
    int dx;
    int dy;
    int i;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    //assert(x in range, y in range);

    mis = NULL;

    fields = spell->Action->Data.AreaBombardment.Fields;
    shards = spell->Action->Data.AreaBombardment.Shards;
    damage = spell->Action->Data.AreaBombardment.Damage;
    offsetx = spell->Action->Data.AreaBombardment.StartOffsetX;
    offsety = spell->Action->Data.AreaBombardment.StartOffsetY;
    while (fields--) {
    	// FIXME : radius configurable...
	do {
	    // find new destination in the map
	    dx = x + SyncRand() % 5 - 2;
	    dy = y + SyncRand() % 5 - 2;
	} while (dx < 0 && dy < 0 && dx >= TheMap.Width && dy >= TheMap.Height);
	for (i = 0; i < shards; ++i) {
	    mis = MakeMissile(spell->Missile,
		    dx * TileSizeX + TileSizeX / 2 + offsetx,
		    dy * TileSizeY + TileSizeY / 2 + offsety,
		    dx * TileSizeX + TileSizeX / 2,
		    dy * TileSizeY + TileSizeY / 2);
	    //  FIXME: This is just patched up, it works, but I have no idea why.
	    //  FIXME: What is the reasoning behind all this?
	    if (mis->Type->Speed) {
		mis->Delay = i * mis->Type->Sleep * 2 * TileSizeX / mis->Type->Speed;
	    } else {
		mis->Delay = i * mis->Type->Sleep * VideoGraphicFrames(mis->Type->Sprite);
	    }
	    mis->Damage = damage;
	    // FIXME: not correct -- blizzard should continue even if mage is
	    //       destroyed (though it will be quite short time...)
	    mis->SourceUnit = caster;
	    RefsDebugCheck(!caster->Refs || caster->Destroyed);
	    caster->Refs++;
	}
    }
    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    caster->Mana -= spell->ManaCost;
    return caster->Mana > spell->ManaCost;
}

/**
**	Cast death coil.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastDeathCoil(Unit *caster, const SpellType *spell, Unit *target,
    int x, int y)
{
    Missile *mis;
    int sx;
    int sy;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
// assert(target);
// assert(x in range, y in range);

    mis = NULL;
    sx = caster->X;
    sy = caster->Y;

    caster->Mana -= spell->ManaCost;

    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    mis = MakeMissile(spell->Missile,
	    sx * TileSizeX + TileSizeX / 2, sy * TileSizeY + TileSizeY / 2,
	    x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2);
    mis->SourceUnit = caster;
    RefsDebugCheck(!caster->Refs || caster->Destroyed);
    caster->Refs++;
    if (target) {
	mis->TargetUnit = target;
	RefsDebugCheck(!target->Refs || target->Destroyed);
	target->Refs++;
    }
    mis->Controller = SpellDeathCoilController;
    return 0;
}

/**
**	Cast fireball.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastFireball(Unit *caster, const SpellType *spell,
    Unit *target __attribute__((unused)), int x, int y)
{
    Missile *missile;
    int sx;
    int sy;
    int dist;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    DebugCheck(!spell->Missile);

    missile = NULL;

    // NOTE: fireball can be casted on spot
    sx = caster->X;
    sy = caster->Y;
    dist = MapDistance(sx, sy, x, y);
    DebugCheck(!dist);
    x += ((x - sx) * 10) / dist;
    y += ((y - sy) * 10) / dist;
    sx = sx * TileSizeX + TileSizeX / 2;
    sy = sy * TileSizeY + TileSizeY / 2;
    x = x * TileSizeX + TileSizeX / 2;
    y = y * TileSizeY + TileSizeY / 2;
    caster->Mana -= spell->ManaCost;
    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    missile = MakeMissile(spell->Missile, sx, sy, x, y);
    missile->State = spell->Action->Data.Fireball.TTL - (dist - 1) * 2;
    missile->TTL = spell->Action->Data.Fireball.TTL;
    missile->Controller = SpellFireballController;
    missile->SourceUnit = caster;
    RefsDebugCheck(!caster->Refs || caster->Destroyed);
    caster->Refs++;
    return 0;
}

/**
**	Cast flame shield.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastFlameShield(Unit* caster, const SpellType *spell, Unit *target,
    int x __attribute__((unused)), int y __attribute__((unused)))
{
    Missile *mis;
    int	i;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    DebugCheck(!target);
//  assert(x in range, y in range);
    DebugCheck(!spell->Missile);

    mis = NULL;

    // get mana cost
    caster->Mana -= spell->ManaCost;
    target->FlameShield = spell->Action->Data.FlameShield.TTL;
    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    for (i = 0; i < 5; i++) {
	mis = MakeMissile(spell->Missile, 0, 0, 0, 0);
	mis->TTL = spell->Action->Data.FlameShield.TTL + i * 7;
	mis->TargetUnit = target;
	mis->Controller = SpellFlameShieldController;
	RefsDebugCheck(!target->Refs || target->Destroyed);
	target->Refs++;
    }
    return 0;
}

/**
**	Cast haste.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastAdjustBuffs(Unit *caster, const SpellType *spell, Unit *target,
    int x, int y)
{
    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    DebugCheck(!target);

    // get mana cost
    caster->Mana -= spell->ManaCost;

    if (spell->Action->Data.AdjustBuffs.HasteTicks!=BUFF_NOT_AFFECTED) {
	target->Haste=spell->Action->Data.AdjustBuffs.HasteTicks;
    }
    if (spell->Action->Data.AdjustBuffs.SlowTicks!=BUFF_NOT_AFFECTED) {
	target->Slow=spell->Action->Data.AdjustBuffs.SlowTicks;
    }
    if (spell->Action->Data.AdjustBuffs.BloodlustTicks!=BUFF_NOT_AFFECTED) {
	target->Bloodlust=spell->Action->Data.AdjustBuffs.BloodlustTicks;
    }
    if (spell->Action->Data.AdjustBuffs.InvisibilityTicks!=BUFF_NOT_AFFECTED) {
	target->Invisible=spell->Action->Data.AdjustBuffs.InvisibilityTicks;
    }
    if (spell->Action->Data.AdjustBuffs.InvincibilityTicks!=BUFF_NOT_AFFECTED) {
	target->UnholyArmor=spell->Action->Data.AdjustBuffs.InvincibilityTicks;
    }
    CheckUnitToBeDrawn(target);
    PlayGameSound(spell->SoundWhenCast.Sound,MaxSampleVolume);
    MakeMissile(spell->Missile,
	x*TileSizeX+TileSizeX/2, y*TileSizeY+TileSizeY/2,
	x*TileSizeX+TileSizeX/2, y*TileSizeY+TileSizeY/2 );
    return 0;
}

/**
**	Cast healing. (or exorcism)
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastAdjustVitals(Unit *caster, const SpellType *spell, Unit *target,
    int x, int y)
{
    int castcount;
    int diffHP;
    int diffMana;
    int hp;
    int mana;
    int manacost;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    DebugCheck(!target);

    hp = spell->Action->Data.AdjustVitals.HP;
    mana = spell->Action->Data.AdjustVitals.Mana;
    manacost = spell->ManaCost;

    //  Healing and harming
    if (hp>0) {
	diffHP = target->Stats->HitPoints - target->HP;
    } else {
	diffHP = target->HP;
    }
    if (mana>0) {
	diffMana = target->Type->_MaxMana - target->Mana;
    } else {
	diffMana = target->Mana;
    }

    //  When harming cast again to send the hp to negative values.
    //  Carefull, a perfect 0 target hp kills too.
    //  Avoid div by 0 errors too!
    castcount=0;
    if (hp) {
	castcount=max(castcount,diffHP/abs(hp)+( ( (hp<0) && (diffHP%(-hp)>0) ) ? 1 : 0));
    }
    if (mana) {
	castcount=max(castcount,diffMana/abs(mana)+( ( (mana<0) && (diffMana%(-mana)>0) ) ? 1 : 0));
    }
    if (manacost) {
	castcount=min(castcount,caster->Mana/manacost);
    }
    if (spell->Action->Data.AdjustVitals.MaxMultiCast) {
	castcount=min(castcount,spell->Action->Data.AdjustVitals.MaxMultiCast);
    }

    DebugCheck(castcount<0);

    DebugLevel3Fn("Used to have %d hp and %d mana.\n" _C_ target->HP _C_ target->Mana);

    caster->Mana-=castcount*manacost;
    if (hp < 0) {
	HitUnit(caster,target,castcount*hp);
    } else {
	target->HP += castcount * hp;
	if (target->HP>target->Stats->HitPoints) {
	    target->HP=target->Stats->HitPoints;
	}
    }
    target->Mana+=castcount*mana;
    if (target->Mana<0) {
	target->Mana=0;
    }
    if (target->Mana>target->Type->_MaxMana) {
	target->Mana=target->Type->_MaxMana;
    }

    DebugLevel3Fn("Unit now has %d hp and %d mana.\n" _C_ target->HP _C_ target->Mana);
    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    MakeMissile(spell->Missile,
	    x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2,
	    x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2);
    return 0;
}

/**
**	Cast polymorph.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastPolymorph(Unit *caster, const SpellType *spell, Unit *target,
    int x, int y)
{
    UnitType *type;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    DebugCheck(!target);

    type = spell->Action->Data.Polymorph.NewForm;
    DebugCheck(!type);

    caster->Player->Score += target->Type->Points;
    if (target->Type->Building) {
	caster->Player->TotalRazings++;
    } else {
	caster->Player->TotalKills++;
    }
#ifdef USE_HP_FOR_XP
    caster->XP += target->HP;
#else
    caster->XP += target->Type->Points;
#endif
    caster->Kills++;
    // as said somewhere else -- no corpses :)
    RemoveUnit(target,NULL);
    UnitLost(target);
    UnitClearOrders(target);
    ReleaseUnit(target);
    if (UnitTypeCanMoveTo(x, y, type)) {
	MakeUnitAndPlace(x, y, type, Players + PlayerNumNeutral);
    }
    caster->Mana -= spell->ManaCost;
    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    MakeMissile(spell->Missile,
	    x*TileSizeX+TileSizeX/2, y*TileSizeY+TileSizeY/2,
	    x*TileSizeX+TileSizeX/2, y*TileSizeY+TileSizeY/2 );
    return 0;
}

/**
**	Cast raise dead.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastRaiseDead(Unit *caster, const SpellType *spell, Unit *target,
    int x, int y)
{
    Unit **corpses;
    Unit *tempcorpse;
    UnitType *skeleton;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
//  assert(x in range, y in range);

    skeleton = spell->Action->Data.RaiseDead.UnitRaised;
    DebugCheck(!skeleton);

    corpses = &CorpseList;

    while (*corpses) {
	// FIXME: this tries to raise all corps, ohje
	// FIXME: I can raise ships?
	if ((*corpses)->Orders[0].Action == UnitActionDie
		&& !(*corpses)->Type->Building
		&& (*corpses)->X >= x - 1 && (*corpses)->X <= x + 1
		&& (*corpses)->Y >= y - 1 && (*corpses)->Y <= y + 1) {
	    // FIXME: did they count on food?
	    // nobody: unlikely.
	    // Can there be more than 1 skeleton created on the same tile? yes
	    target = MakeUnit(skeleton, caster->Player);
	    target->X = (*corpses)->X;
	    target->Y = (*corpses)->Y;
	    DropOutOnSide(target,LookingW,0,0);
	    // set life span
	    target->TTL = GameCycle + target->Type->DecayRate * 6 * CYCLES_PER_SECOND;
	    CheckUnitToBeDrawn(target);
	    tempcorpse = *corpses;
	    corpses = &(*corpses)->Next;
	    ReleaseUnit(tempcorpse);
	    caster->Mana -= spell->ManaCost;
	    if (caster->Mana<spell->ManaCost) {
		break;
	    }
	} else {
	    corpses=&(*corpses)->Next;
	}
    }
    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    MakeMissile(spell->Missile,
	    x*TileSizeX+TileSizeX/2, y*TileSizeY+TileSizeY/2,
	    x*TileSizeX+TileSizeX/2, y*TileSizeY+TileSizeY/2 );
    return 0;
}

/**
**	Cast runes.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastRunes(Unit *caster, const SpellType *spell,
    Unit *target __attribute__((unused)), int x, int y)
{
    Missile *mis;
    const int xx[] = {-1,+1, 0, 0, 0};
    const int yy[] = { 0, 0, 0,-1,+1};
    int oldx;
    int oldy;
    int i;

    DebugCheck(!caster);
    DebugCheck(spell);
    DebugCheck(!spell->Action);
//  assert(x in range, y in range);

    mis = NULL;
    oldx = x;
    oldy = y;

    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    for (i = 0; i < 5; i++) {
	x = oldx + xx[i];
	y = oldy + yy[i];
	    
	if (IsMapFieldEmpty(x - 1, y + 0)) {
	    mis = MakeMissile(spell->Missile,
		    x * TileSizeX + TileSizeX / 2,
		    y * TileSizeY + TileSizeY / 2,
		    x * TileSizeX + TileSizeX / 2,
		    y * TileSizeY + TileSizeY / 2);
	    mis->TTL = spell->Action->Data.Runes.TTL;
	    mis->Controller = SpellRunesController;
	    caster->Mana -= spell->ManaCost / 5;
	}
    }
    return 0;
}

/**
**	Cast summon spell.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastSummon(Unit *caster, const SpellType *spell, Unit *target,
    int x, int y)
{
    int ttl;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    DebugCheck(!spell->Action->Data.Summon.UnitType);

    DebugLevel0("Summoning\n");
    ttl=spell->Action->Data.Summon.TTL;
    caster->Mana -= spell->ManaCost;
    // FIXME: johns: the unit is placed on the wrong position
    target = MakeUnit(spell->Action->Data.Summon.UnitType, caster->Player);
    target->X = x;
    target->Y = y;
    // set life span
    if (ttl) {
	target->TTL=GameCycle+ttl;
    } else {
	target->TTL=GameCycle+target->Type->DecayRate * 6 * CYCLES_PER_SECOND;
    }
    //
    //	Revealers are always removed, since they don't have graphics
    //
    if (target->Type->Revealer) {
	DebugLevel0Fn("new unit is a revealer, removed.\n");
	target->Removed=1;
	target->CurrentSightRange = target->Stats->SightRange;
	MapMarkUnitSight(target);
    } else {
	DropOutOnSide(target, LookingW, 0, 0);
	CheckUnitToBeDrawn(target);
    }

    PlayGameSound(spell->SoundWhenCast.Sound,MaxSampleVolume);
    MakeMissile(spell->Missile,
		x*TileSizeX+TileSizeX/2, y*TileSizeY+TileSizeY/2,
		x*TileSizeX+TileSizeX/2, y*TileSizeY+TileSizeY/2 );
    return 0;
}

/**
**	Cast whirlwind.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastWhirlwind(Unit *caster, const SpellType *spell,
    Unit *target __attribute__((unused)), int x, int y)
{
    Missile *mis;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
//  assert(x in range, y in range);

    mis = NULL;

    caster->Mana -= spell->ManaCost;
    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    mis = MakeMissile(spell->Missile,
	    x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2,
	    x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2);
    mis->TTL = spell->Action->Data.Whirlwind.TTL;
    mis->Controller = SpellWhirlwindController;
    return 0;
}

// ****************************************************************************
// Target constructor
// ****************************************************************************

/**
**	FIXME: docu
*/
local Target *NewTarget(TargetType t, const Unit *unit, int x, int y)
{
    Target *target;

    target = (Target *)malloc(sizeof(*target));

    DebugCheck(unit == NULL && t == TargetUnit);
    DebugCheck(!(0 <= x && x < TheMap.Width) && t == TargetPosition);
    DebugCheck(!(0 <= y && y < TheMap.Height) && t == TargetPosition);

    target->which_sort_of_target = t;
    target->unit = (Unit *)unit;
    target->X = x;
    target->Y = y;
    return target;
}

/**
**	FIXME: docu
*/
local Target *NewTargetNone(void)
{
    return NewTarget(TargetNone, NULL, 0, 0);
}

/**
**	FIXME: docu
*/
local Target *NewTargetUnit(const Unit *unit)
{
    DebugCheck(!unit);
    return NewTarget(TargetUnit, unit, 0, 0);
}

/**
**	FIXME: docu
*/
local Target *NewTargetPosition(int x, int y)
{
    DebugCheck(!(0 <= x && x < TheMap.Width));
    DebugCheck(!(0 <= y && y < TheMap.Height));

    return NewTarget(TargetPosition, NULL, x, y);
}

// ****************************************************************************
//	Main local functions
// ****************************************************************************

/*
**	Check the condition.
**
**	@param caster		Pointer to caster unit.
**	@param spell 		Pointer to the spell to cast.
**	@param target		Pointer to target unit, or 0 if it is a position spell.
**	@param x		X position, or -1 if it is an unit spell.
**	@param y		Y position, or -1 if it is an unit spell.
**	@param condition	Pointer to condition info.
**
**	@return 1 if passed, 0 otherwise.
*/
local int PassCondition(const Unit* caster,const SpellType* spell,const Unit* target,
	int x,int y,const ConditionInfo* condition)
{
    //
    //	Check caster mana. FIXME: move somewhere else?
    //
    if (caster->Mana<spell->ManaCost) {
	return 0;
    }
    //
    //	Casting an unit spell without a target. 
    //
    if (spell->Target==TargetUnit&&!target) {
	return 0;
    }
    if (!condition) {
	// no condition, pass.
	return 1;
    }
    //
    //	Now check conditions regarding the target unit.
    //
    if (target) {
	if (condition->Undead!=CONDITION_TRUE) {
	    if ((condition->Undead==CONDITION_ONLY)^(target->Type->IsUndead)) {
		return 0;
	    }
	}
	if (condition->Organic!=CONDITION_TRUE) {
	    if ((condition->Organic==CONDITION_ONLY)^(target->Type->Organic)) {
		return 0;
	    }
	}
	if (condition->Building!=CONDITION_TRUE) {
	    if ((condition->Building==CONDITION_ONLY)^(target->Type->Building)) {
		return 0;
	    }
	}
	if (condition->Hero!=CONDITION_TRUE) {
	    if ((condition->Hero==CONDITION_ONLY)^(target->Type->Hero)) {
		return 0;
	    }
	}
	if (condition->Coward!=CONDITION_TRUE) {
	    if ((condition->Coward==CONDITION_ONLY)^(target->Type->Coward)) {
		return 0;
	    }
	}
	if (condition->Alliance!=CONDITION_TRUE) {
	    if ((condition->Alliance==CONDITION_ONLY)^(IsAllied(caster->Player,target)||target->Player==caster->Player)) {
		return 0;
	    }
	}
	if (condition->TargetSelf!=CONDITION_TRUE) {
	    if ((condition->TargetSelf==CONDITION_ONLY)^(caster==target)) {
		return 0;
	    }
	}
	//
	//	Check vitals now.
	//
	if (condition->MinHpPercent*target->Stats->HitPoints/100>target->HP) {
	    return 0;
	}
	if (condition->MaxHpPercent*target->Stats->HitPoints/100<=target->HP) {
	    return 0;
	}
	if (target->Type->CanCastSpell) {
	    if (condition->MinManaPercent*target->Type->_MaxMana/100>target->Mana) {
		return 0;
	    }
	    if (condition->MaxManaPercent*target->Type->_MaxMana/100<target->Mana) {
		return 0;
	    }
	}
	//
	//	Check for slow/haste stuff
	//	This should be used mostly for ai, if you want to keep casting
	//	slow to no effect I can't see why should we stop you.
	//
	if (condition->MaxSlowTicks<target->Slow) {
	    return 0;
	}
	if (condition->MaxHasteTicks<target->Haste) {
	    return 0;
	}
	if (condition->MaxBloodlustTicks<target->Bloodlust) {
	    return 0;
	}
	if (condition->MaxInvisibilityTicks<target->Invisible) {
	    return 0;
	}
	if (condition->MaxInvincibilityTicks<target->UnholyArmor) {
	    return 0;
	}
    }
    return 1;
}

/**
**	Select the target for the autocast.
**
**	@param caster	Unit who would cast the spell.
**	@param spell	Spell-type pointer.
**	
**	@return Target*	choosen target or Null if spell can't be cast.
**
*/
// FIXME: should be global (for AI) ???
local Target *SelectTargetUnitsOfAutoCast(const Unit *caster, const SpellType *spell)
{
    Unit* table[UnitMax];
    int x;
    int y;
    int range;
    int nunits;
    int i;
    int j;
    int combat;
    AutoCastInfo* autocast;

    DebugCheck(!spell);
    DebugCheck(!spell->AutoCast);
    DebugCheck(!caster);

    //
    //	Ai cast should be a lot better. Use autocast if not found.
    //
    if (caster->Player->Ai&&spell->AICast) {
	DebugLevel3Fn("The borg uses AI autocast XP.\n");
	autocast=spell->AICast;
    } else {
	DebugLevel3Fn("You puny mortal, join the colective!\n");
	autocast=spell->AutoCast;
    }

    x=caster->X;
    y=caster->Y;
    range=spell->AutoCast->Range;

    //
    //	Select all units aroung the caster
    //
    nunits = SelectUnits(caster->X - range, caster->Y - range,
	    caster->X + range + caster->Type->TileWidth, caster->Y + range + caster->Type->TileHeight, table);
    //
    //  Check every unit if it is hostile
    // 
    combat=0;
    for (i = 0; i < nunits; i++) {
	if (IsEnemy(caster->Player,table[i]) && !table[i]->Type->Coward) {
	    combat=1;
	}
    }

    //
    //	Check generic conditions. FIXME: a better way to do this?
    //
    if (autocast->Combat!=CONDITION_TRUE) {
	if ((autocast->Combat==CONDITION_ONLY)^(combat)) {
	    return 0;
	}
    }

    switch (spell->Target) {
	case TargetNone :
	    // TargetNone?
	    return NewTargetNone();
	case TargetSelf :
	    if (PassCondition(caster, spell, caster, x, y, spell->Condition) &&
		    PassCondition(caster, spell, caster, x, y, autocast->Condition)) {
    	        return NewTargetUnit(caster);
    	    }
	    return 0;
	case TargetPosition:
	    return 0;
	    //  Autocast with a position? That's hard
	    //  Possibilities: cast reveal-map on a dark region
	    //  Cast raise dead on a bunch of corpses. That would rule.
	    //  Cast summon until out of mana in the heat of battle. Trivial?
	    //  Find a tight group of units and cast area-damage spells. HARD,
	    //  but it is a must-have for AI. What about area-heal?
	case TargetUnit:
	    //
	    //	The units are already selected.
	    //  Check every unit if it is a possible target
	    // 
	    for (i = 0, j = 0; i < nunits; i++) {
		//  FIXME: autocast conditions should include normal conditions.
		//  FIXME: no, really, they should.
		if (PassCondition(caster, spell, table[i], x, y, spell->Condition) &&
			PassCondition(caster, spell, table[i], x, y, autocast->Condition)) {
		    table[j++] = table[i];
		}
	    }
	    nunits = j;
	    //	
	    //	Now select the best unit to target.
	    //	FIXME: Some really smart way to do this. 
	    //	FIXME: Heal the unit with the lowest hit-points
	    //	FIXME: Bloodlust the unit with the highest hit-point
	    //	FIMXE: it will survive more
	    //
	    if (nunits != 0) {
#if 0
		// For the best target???
		sort(table, nb_units, spell->autocast->f_order);
		return NewTargetUnit(table[0]);
#else
		//	Best unit, random unit, oh well, same stuff.
		i = SyncRand() % nunits;
		return NewTargetUnit(table[i]);
#endif
	    }
	    break;
	default:
	    // Something is wrong
	    DebugLevel0Fn("Spell is screwed up, unknown target type\n");
	    DebugCheck(1);
	    return NULL;
	    break;
	}
    return NULL;	// Can't spell the auto-cast.
}

// ****************************************************************************
//	Public spell functions
// ****************************************************************************

// ****************************************************************************
// Constructor and destructor
// ****************************************************************************

/**
**	Spells constructor, inits spell id's and sounds
*/
global void InitSpells(void)
{
}

// ****************************************************************************
// Get Spell.
// ****************************************************************************

/**
**	Get the numeric spell id by string identifer.
**
**	@param IdentName	Spell identifier
**
**	@return		Spell id (index in spell-type table)
*/
global int SpellIdByIdent(const char *ident)
{
    int id;

    DebugCheck(!ident);

    for (id = 0; id < SpellTypeCount; ++id) {
	if (strcmp(SpellTypeTable[id].IdentName, ident) == 0) {
	    return id;
	}
    }
    return -1;
}

/**
**	Get spell-type struct pointer by string identifier.
**
**	@param IdentName	Spell identifier.
**
**	@return		spell-type struct pointer.
*/
global SpellType *SpellTypeByIdent(const char *ident)
{
    int id;

    DebugCheck(!ident);

    id = SpellIdByIdent(ident);
    return (id == -1 ? NULL : &SpellTypeTable[id]);
}

/**
**	FIXME: docu
*/
global unsigned CclGetSpellByIdent(SCM value)
{  
    int i;

    for (i = 0; i < SpellTypeCount; ++i) {
	if (gh_eq_p(value, gh_symbol2scm(SpellTypeTable[i].IdentName))) {
	    return i;
	}
    }
    return 0xABCDEF;
}

/**
**	Get spell-type struct ptr by id
**
**	@param id  Spell id (index in the spell-type table)
**
**	@return spell-type struct ptr
*/
global SpellType *SpellTypeById(int id)
{
    DebugCheck(!(0 <= id && id < SpellTypeCount));
    return &SpellTypeTable[id];
}

// ****************************************************************************
// CanAutoCastSpell, CanCastSpell, AutoCastSpell, CastSpell.
// ****************************************************************************

/**
**	Check if spell is research for player \p player.
**	@param	player : player for who we want to know if he knows the spell.
**	@param	id : 
*/
global int SpellIsAvailable(const Player *player, int spellid)
{
    int dependencyId;
    
    DebugCheck(!player);
    DebugCheck(!(0 <= spellid && spellid < SpellTypeCount));

    dependencyId = SpellTypeTable[spellid].DependencyId;

    return dependencyId == -1 || UpgradeIdAllowed(player, dependencyId) == 'R';
}


/**
**	Check if the spell can be auto cast.
**
**	@param spell	Spell-type pointer
**
**	@return		1 if spell can be cast, 0 if not
*/
global int CanAutoCastSpell(const SpellType *spell)
{
    DebugCheck(!spell);

    return spell->AutoCast ? 1 : 0;
}

/**
**	Check if unit can cast the spell.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should/can casted, 0 if not
*/
global int CanCastSpell(const Unit *caster, const SpellType *spell,
    const Unit *target, // FIXME : Use an unique struture t_Target ?
    int x, int y)
{
    DebugCheck(!caster);
    DebugCheck(!spell);

    // And caster must know the spell
    // FIXME spell->Ident < MaxSpell
    DebugCheck(!(caster->Type->CanCastSpell && caster->Type->CanCastSpell[spell->Ident]));

    if (!caster->Type->CanCastSpell
	    || !caster->Type->CanCastSpell[spell->Ident]
	    || (spell->Target == TargetUnit && target == NULL)) {
	return 0;
    }

    return PassCondition(caster,spell,target,x,y,spell->Condition);
}

/**
**	Check if the spell can be auto cast and cast it.
**
**	@param caster	Unit who can cast the spell.
**	@param spell	Spell-type pointer.
**
**	@return		1 if spell is casted, 0 if not.
*/
global int AutoCastSpell(Unit *caster, const SpellType *spell)
{
    Target *target;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!(0 <= spell->Ident && spell->Ident < SpellTypeCount));
    DebugCheck(!(caster->Type->CanCastSpell));
    DebugCheck(!(caster->Type->CanCastSpell[spell->Ident]));

    target = NULL;

    //  Check for mana, trivial optimization.
    if (caster->Mana<spell->ManaCost) {
	return 0;
    }
    target = SelectTargetUnitsOfAutoCast(caster, spell);
    if (target == NULL) {
	return 0;
    } else {
	//	Must move before ?
	//	FIXME SpellType* of CommandSpellCast must be const.
	CommandSpellCast(caster, target->X, target->Y, target->unit, (SpellType*) spell, FlushCommands);
	free(target);
    }
    return 1;
}

/**
**	Spell cast!
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		!=0 if spell should/can continue or 0 to stop
*/
global int SpellCast(Unit *caster, const SpellType *spell, Unit *target,
    int x, int y)
{
    DebugCheck(!spell);
    DebugCheck(!spell->Action->CastFunction);
    DebugCheck(!caster);
    DebugCheck(!SpellIsAvailable(caster->Player, spell->Ident));

    caster->Invisible = 0;// unit is invisible until attacks // FIXME Must be configurable
    if (target) {
	x = target->X;
	y = target->Y;
    } else {
	x += spell->Range;	// Why ??
	y += spell->Range;	// Why ??
    }
    DebugLevel3Fn("Spell cast: (%s), %s -> %s (%d,%d)\n" _C_ spell->IdentName _C_
	    unit->Type->Name _C_ target ? target->Type->Name : "none" _C_ x _C_ y);
    return CanCastSpell(caster, spell, target, x, y) && spell->Action->CastFunction(caster, spell, target, x, y);
}

/*
**	Cleanup the spell subsystem.
**	
**	@note: everything regarding spells is gone now.
**	FIXME: not complete
*/
void CleanSpells(void)
{
    SpellType* spell;

    DebugLevel0("Cleaning spells.\n");
    for (spell = SpellTypeTable; spell < SpellTypeTable + SpellTypeCount; ++spell) {
	free(spell->IdentName);
	free(spell->Name);
	free(spell->Action);
	if (spell->Condition) {
	    free(spell->Condition);
	}
	//
	//	Free Autocast.
	//
	if (spell->AutoCast) {
	    if (spell->AutoCast->Condition) {
		free(spell->AutoCast->Condition);
	    }
	    free(spell->AutoCast);
	}
	if (spell->AICast) {
	    if (spell->AICast->Condition) {
		free(spell->AICast->Condition);
	    }
	    free(spell->AICast);
	}
	// FIXME: missile free somewhere else, right?
    }
    free(SpellTypeTable);
    SpellTypeTable=0;
    SpellTypeCount=0;
}

#if 0

/*
**	 TODO :
**	- Modify missile.c for better configurable and clear the code.
** ccl info


// !!! Special deathcoil

// if (!target->Type->Building
	   && (target->Type->UnitType == UnitTypeLand || target->Type->UnitType == UnitTypeNaval)
	&& target->FlameShield < spell->TTL) // FlameShield

	= {

  NOTE: vladi:

  The point to have variable unsorted list of spell-types and
  dynamic id's and in the same time -- SpellAction id's is that
  spell actions are hardcoded and cannot be changed at all.
  On the other hand we can have different spell-types as with
  different range, cost and time to live (possibly and other
  parameters as extensions)

*/

#endif

//@}
