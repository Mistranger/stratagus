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
/**@name ccl_sound.h	-	The Ccl sound header file. */
//
//	(c) Copyright 1999-2001 by Lutz Sammer and Fabrice Rossi
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

#ifndef __CCL_SOUND_H__
#define __CCL_SOUND_H__

//@{

/*----------------------------------------------------------------------------
--		Includes
----------------------------------------------------------------------------*/

#ifdef WITH_SOUND		// {

#include "ccl.h"

/*----------------------------------------------------------------------------
--		Functions
----------------------------------------------------------------------------*/

#if defined(USE_GUILE) || defined(USE_SIOD)
extern int ccl_sound_p(SCM sound);		/// is it a ccl sound?

extern SoundId ccl_sound_id(SCM sound);		/// scheme -> sound id
#elif defined(USE_LUA)
#endif

extern void SoundCclRegister(void);		/// register ccl features

#else		// }{ defined(WITH_SOUND)

//-----------------------------------------------------------------------------

extern void SoundCclRegister(void);		/// register ccl features

#endif		// } !defined(WITH_SOUND)

//@}

#endif		// !__CCL_SOUND_H__
