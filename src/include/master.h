//       _________ __                 __                               
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ \ 
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/ 
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   Stratagus - A free fantasy real time strategy game engine
//
/**@name master.h	-	The master server headerfile. */
//
//	(c) Copyright 2003 by Tom Zickel and Jimmy Salmon
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

#ifndef __MASTER_H__
#define __MASTER_H__

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Defines
----------------------------------------------------------------------------*/

#define MASTER_HOST "stratagus.dyndns.org"
#define MASTER_PORT 8123

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

    /// FIXME: docu
extern char MasterTempString[50];
    /// FIXME: docu
extern int PublicMasterAnnounce;
    /// FIXME: docu
extern unsigned long LastTimeAnnounced;
    /// FIXME: docu
extern int MasterPort;
    /// FIXME: docu
extern unsigned long MasterHost;
    /// FIXME: docu
extern char *MasterHostString;

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

    /// FIXME: docu
extern int MasterInit(void);
    /// FIXME: docu
extern void MasterLoop(unsigned long ticks);
    /// FIXME: docu
extern void MasterSendAnnounce(void);
    /// FIXME: docu
extern void MasterProcessGetServerData(const char* msg, size_t length, unsigned long host, int port);

//@}

#endif	// !__MASTER_H__
