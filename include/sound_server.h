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
/**@name sound_server.h	-	The sound server header file. */
//
//	(c) Copyright 1998-2003 by Lutz Sammer and Fabrice Rossi
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

#ifndef __SOUND_SERVER_H__
#define __SOUND_SERVER_H__

//@{

#ifdef WITH_SOUND	// {

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#if defined(USE_THREAD) && !defined(USE_SDLA)
#include <pthread.h>
#include <semaphore.h>
extern sem_t SoundThreadChannelSemaphore;
#endif

#include "sound_id.h"

/*----------------------------------------------------------------------------
--	Definitons
----------------------------------------------------------------------------*/

    /**
    **	Maximal volume value
    */
#define MaxVolume 255

/**
**	General sample object typedef. (forward)
*/
typedef struct _sample_ Sample;

/**
**	General sound object type.
**
**	@todo	FIXME: This is the planned new sample handling,
**		which supports streaming, decompressing on demand, caching.
*/
typedef struct _sample_type_ {
	/**
	**	Read next samples from object.
	**
	**	@param o	pointer to object.
	**	@param buf	buffer to fill.
	**	@param len	length of buffer.
	**
	**	@return		Number of bytes filled.
	*/
    int (*Read)		(Sample* o, void* buf, int len);
	/**
	**	Free the sample object.
	**
	**	@param o	pointer to object.
	*/
    void (*Free)	(Sample* o);
} SampleType;

/**
**	RAW samples.
*/
struct _sample_ {
    const SampleType*	Type;		/// Object type dependend
    void*		User;		/// Object user data

    unsigned char	Channels;	/// mono or stereo
    unsigned char	SampleSize;	/// sample size in bits
    unsigned int	Frequency;	/// frequency in hz
    int			Length;		/// sample length
    char		Data[1];	/// sample bytes
};

    /// Free a sample object.
#define SoundFree(o)            ((o)->Type->Free)((o))
    /// Save (NULL) free a sample object.
#define SoundSaveFree(o) \
    do { if( (o) ) ((o)->Type->Free)((o)); } while( 0 )

/**
**	Sound double group: a sound that groups two sounds, used to implement
**	the annoyed/selected sound system of WC
*/
typedef struct _two_groups_ {
    struct _sound_ *First;		/// first group: selected sound
    struct _sound_ *Second;		/// second group: annoyed sound
} TwoGroups;

/**
 ** A possible value for Number in the Sound struct: means a simple sound
 */
#define ONE_SOUND	0
/**
 ** A possible value for Number in the Sound struct: means a double group (for
 ** selection/annoyed sounds)
 */
#define TWO_GROUPS	1

/**
 ** the range value that makes a sound volume distance independent
 */
#define INFINITE_SOUND_RANGE 255
/**
 ** the maximum range value
 */
#define MAX_SOUND_RANGE 254

/**
**	Sound definition.
*/
typedef struct _sound_ {
	/**
	**	Range is a multiplier for ::DistanceSilent.
	**	255 means infinite range of the sound.
	*/
    unsigned char Range;	/// Range is a multiplier for DistanceSilent
    unsigned char Number;	/// single, group, or table of sounds.
    union {
	Sample*     OneSound;	/// if it's only a simple sound
	Sample**    OneGroup;	/// when it's a simple group
	TwoGroups*  TwoGroups;	/// when it's a double group
    } Sound;
} Sound;

/**
**	Sound unique identifier
*/
typedef Sound* ServerSoundId;

/**
**	Origin of a sound
*/
typedef struct _origin_ {
    const void* Base;	/// pointer on a Unit
    unsigned Id;	/// unique identifier (if the pointer has been shared)
} Origin;

/**
**	sound request FIFO
*/
typedef struct _sound_request {
    Origin Source;			/// origin of sound
    unsigned short Power;		/// Volume or Distance
    SoundId Sound;			/// which sound
    unsigned Used : 1;			/// flag for used/unused
    unsigned Fight : 1;			/// is it a fight sound?
    unsigned Selection : 1;		/// is it a selection sound?
    unsigned IsVolume : 1;		/// how to interpret power (as a
                                        ///volume or as a distance?)
    char Stereo;			/// stereo location of sound (
                                        ///-128 left, 0 center, 127 right)
} SoundRequest;

#define MAX_SOUND_REQUESTS 32		/// maximal number of sound requests

#define MaxChannels     16              /// How many channels are supported

    /// Channels for sound effects and unit speach
typedef struct _sound_channel_
{
    unsigned char       Command;        /// channel command
    int                 Point;          /// point into sample
    Sample*             Sample;         /// sample to play
    Origin              Source;         /// unit playing
    unsigned char       Volume;         /// Volume of this channel
    SoundId             Sound;          /// The sound currently played
        /// stereo location of sound (-128 left, 0 center, 127 right)
    signed char         Stereo;
} SoundChannel;
		
/**
**      Play audio flags.
*/
enum _play_audio_flags_ {
    PlayAudioStream = 1,            	/// Stream the file from medium
    PlayAudioPreLoad = 2,		/// Load compressed in memory
    PlayAudioLoadInMemory = 4,		/// Preload file into memory
    PlayAudioLoadOnDemand = 8,		/// Load only if needed.
};

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

    /// sound file descriptor, if -1 no sound available
extern int SoundFildes;

    /// sound volume (from 0 to MaxVolume, acts as a multiplier)
extern int GlobalVolume;
    /// music volume (from 0 to MaxVolume, acts as a multiplier)
extern int MusicVolume;

    /// global range control (max cut off distance for sound)
extern int DistanceSilent;

    /// FIFO for sound requests
extern SoundRequest SoundRequests[MAX_SOUND_REQUESTS];
    /// FIFO index in
extern int NextSoundRequestIn;
    /// FIFO index out
extern int NextSoundRequestOut;

#define ChannelFree     0               /// channel is free 
#define ChannelPlay     3               /// channel is playing 

    /// All possible sound channels
extern SoundChannel Channels[MaxChannels];
    /// Next free channel
extern int NextFreeChannel;

#ifdef USE_THREAD
    /// are we using a sound thread? (default is zero -> no thread)
extern int WithSoundThread;
#endif

    /// FIXME: docu
extern int SoundThreadRunning;

#ifdef DEBUG
    /// allocated memory for sound samples
extern unsigned AllocatedSoundMemory;
#endif

extern Sample* MusicSample;		/// Music samples

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

extern Sample* LoadFlac(const char* name,int flags);	/// Load a flac file
extern Sample* LoadWav(const char* name,int flags);	/// Load a wav file
extern Sample* LoadOgg(const char* name,int flags);	/// Load an ogg file
extern Sample* LoadMp3(const char* name,int flags);	/// Load a mp3 file

extern int ConvertToStereo32(const char* in, char* out, int frequency,
    int bitrate, int channels, int bytes);

    ///	Register a sound (can be a simple sound or a group)
extern SoundId RegisterSound(char* file[],unsigned number);

    /**
    **	Ask the sound server to put together two sounds to form a special sound.
    **	@param first	first part of the group
    **	@param second	second part of the group
    **	@return		the special sound unique identifier
    **  @brief Create a special sound group with two sounds
    */
extern SoundId RegisterTwoGroups(const SoundId first,const SoundId second);

    /// Modify the range of a given sound.
extern void SetSoundRange(SoundId sound,unsigned char range);

    /// Free a channel and unregister its source
extern void FreeOneChannel(int channel);

    /// Initialize the sound card.
extern int InitSound(void);
    /// Initialize the oss compatible sound card.
extern int InitOssSound(const char* dev,int freq,int size,int wait);
    /// Initialize the sound card with SDL support.
extern int InitSdlSound(const char* dev,int freq,int size,int wait);
    /// Initialize connection to arts sound daemon.
extern int InitArtsSound(int freq,int size);
    /// Close the arts sound daemon connection.
extern void ExitArtsSound(void);
    /// Write out sound data to arts daemon.
extern int WriteArtsSound(void* data,int len);
    /// Query available sample buffer space from arts daemon.
extern int ArtsGetSpace(void);

    /// Initialize the sound server.
extern int InitSoundServer(void);

/** Ask the sound layer to write the content of its buffer to the sound
    device. To be used only in the unthreaded version.
 */
extern void WriteSound(void);

    ///	Cleanup sound.
extern void QuitSound(void);

#else	// }{ WITH_SOUND

/*----------------------------------------------------------------------------
--	Definitons
----------------------------------------------------------------------------*/

#define SoundFildes	-1		/// Dummy macro for without sound
#define SoundThreadRunning	0	/// Dummy macro for without sound

#define InitSound()	0		/// Dummy macro for without sound
#define WriteSound	NULL		/// Dummy macro for without sound
#define QuitSound()			/// Dummy macro for without sound

#endif	// } WITH_SOUND

extern int WaitForSoundDevice;		/// Block until sound device available

//@}

#endif	// !__SOUND_SERVER_H__
