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
/**@name cdaudio.h		-	cd audio header file. */
//
//	(c) Copyright 2003 by Nehal Mistry
//
//	Stratagus is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation; only version 2 of the License.
//
//	Stratagus is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	$Id$

#ifndef __CDAUDIO_H__
#define __CDAUDIO_H__

//@{

#if defined(USE_SDLCD) || defined(USE_LIBCDA) || defined(USE_CDDA)

#ifndef WITH_SOUND
#error WITH_SOUND must be defined to use cd audio
#endif

#ifndef USE_CDAUDIO
#define USE_CDAUDIO
#endif

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#if defined(USE_SDLCD) 
#include "SDL.h" 
#elif defined(USE_LIBCDA) 
#include "libcda.h" 
#elif defined(USE_CDDA) 
#include <linux/cdrom.h> 
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include "iocompat.h"
#endif 

#include "sound_server.h"

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

typedef enum _cd_modes_ {
    CDModeStopped = -1,                 /// Stopped
    CDModeOff,                          /// Off
    CDModeAll,                          /// All
    CDModeRandom,                       /// Random
    CDModeDefined,                      /// Defined
} CDModes;

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern CDModes CDMode;                  /// CD mode
extern int CDTrack;			/// Current track
extern int NumCDTracks;			/// Number of tracks on CD

#ifdef USE_CDDA
extern int CDDrive;				/// CDRom device
extern struct cdrom_tocentry CDtocentry[64];	/// TOC track header struct
#endif

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

#ifdef USE_CDDA
    /// Load a cd track
extern Sample* LoadCD(const char* name,int flags);      
#endif 

    /// Play CDMode 'name'
extern int PlayCDRom(int name);
    /// Play 'track'
extern int PlayCDTrack(int track);
    /// Is 'track' an audio track?
extern int IsAudioTrack(int track);
    /// Get cd volume (0-255)
extern int GetCDVolume(void);
    /// Set cd volume (0-255)
extern void SetCDVolume(int vol);
    /// Resume CD
extern void ResumeCD(void);
    /// Pause CD
extern void PauseCD(void);
    /// Close CD
extern void QuitCD(void);
    /// Check the cdrom status
extern int CDRomCheck(void *);

#else

#define QuitCD()                        /// Dummy macro for without cd

#endif

//@}

#endif	// !__CDAUDIO_H__
