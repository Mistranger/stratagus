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
/**@name sound_server.c		-	The sound server
**                                      (hardware layer and so on) */
//
//	(c) Copyright 1998-2002 by Lutz Sammer and Fabrice Rossi
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

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include "freecraft.h"

#ifdef WITH_SOUND	// {

#if !defined(USE_LIBMODPLUG) && !defined(noUSE_LIBMODPLUG)
#define USE_LIBMODPLUG			/// Include lib modplug support
#endif
#define _USE_LIBMODPLUG32		/// Test with 32bit resolution

#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif
#include <limits.h>

#ifdef USE_SDLA
#ifdef USE_THREAD
#error "not USE_SDLA and USE_THREAD"
#endif
#include <SDL_audio.h>
#else
#ifdef __linux__
#   include <sys/ioctl.h>
#   include <linux/soundcard.h>
#else
#   include <sys/ioctl.h>
#   include <machine/soundcard.h>
#endif
#endif

#include "video.h"
#include "sound_id.h"
#include "unitsound.h"
#include "tileset.h"
#include "map.h"
#include "iolib.h"

#include "sound_server.h"
#include "sound.h"
#include "ccl.h"

#ifdef USE_LIBMODPLUG
#include "../libmodplug/modplug.h"
#endif

#ifdef USE_LIBCDA
#include "libcda.h"
#endif

#ifdef USE_GLIB
#include <glib.h>
#else
#include "etlib/hash.h"
#endif

#ifdef USE_SDLCD
#include <SDL.h>
#endif

/*----------------------------------------------------------------------------
--	Defines
----------------------------------------------------------------------------*/

//#define SoundSampleSize	8		/// sample size of dsp in bit
#define SoundSampleSize	16		// sample size of dsp in bit
#define SoundFrequency	44100		// sample rate of dsp
//#define SoundFrequency	22050		// sample rate of dsp
//#define SoundFrequency	11025		/// sample rate of dsp
#define SoundDeviceName	"/dev/dsp"	/// dsp device

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

global int SoundFildes = -1;		/// audio file descriptor
global int PlayingMusic;		/// flag true if playing music
global int CallbackMusic;		/// flag true callback ccl if stops

#ifdef DEBUG
global unsigned AllocatedSoundMemory;	/// memory used by sound
global unsigned CompressedSoundMemory;	/// compressed memory used by sound
#endif

global int GlobalVolume=128;		/// global sound volume
global int MusicVolume=128;		/// music volume

global int DistanceSilent;		/// silent distance

// the sound FIFO
global SoundRequest SoundRequests[MAX_SOUND_REQUESTS];
global int NextSoundRequestIn;
global int NextSoundRequestOut;

#ifdef USE_THREAD
global sem_t SoundThreadChannelSemaphore;/// FIXME: docu
local pthread_t SoundThread;		/// FIXME: docu
global int WithSoundThread;		/// FIXME: docu
#endif

global int SoundThreadRunning;		/// FIXME: docu

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

#if defined(USE_OGG) || defined(USE_FLAC) || defined(USE_MAD)
extern Sample* MusicSample;		/// Music samples
extern int MusicIndex;			/// Music sample index
#endif

#if defined(USE_OGG) || defined(USE_FLAC) || defined(USE_MAD) || defined(USE_LIBMODPLUG)

extern ModPlugFile* ModFile;		/// Mod file loaded into memory

/**
**	Mix music to stereo 32 bit.
**
**	@param buffer	Buffer for mixed samples.
**	@param size	Number of samples that fits into buffer.
*/
local void MixMusicToStereo32(int* buffer, int size)
{
    int i;
    int n;

    if (PlayingMusic) {
	n = 0;
#ifdef USE_LIBMODPLUG
	if (ModFile) {
#ifdef USE_LIBMODPLUG32
	    long *buf;
#else
	    short *buf;
#endif
	    buf = alloca(size * sizeof(*buf));

	    n = ModPlug_Read(ModFile, buf, size * sizeof(*buf));

	    for (i = 0; i < n / sizeof(*buf); ++i) {	// Add to our samples
#ifdef USE_LIBMODPLUG32
		buffer[i] += ((buf[i] >> 16) * MusicVolume) / 256;
#else
		buffer[i] += (buf[i] * MusicVolume) / 256;
#endif
	    }

	    n = n != size * sizeof(*buf);	// End reached?
	}
#endif
#if defined(USE_OGG) || defined(USE_FLAC) || defined(USE_MAD)
	if (MusicSample) {
	    short *buf;

	    buf = (short *)(MusicSample->Data + MusicIndex);
	    if (size * 2 > MusicSample->Length - MusicIndex) {
		n = MusicSample->Length - MusicIndex;
	    } else {
		n = size * 2;
	    }
	    n >>= 1;
	    for (i = 0; i < n; ++i) {	// Add to our samples
		buffer[i] += (buf[i] * MusicVolume) / 256;
	    }
	    MusicIndex += i * 2;

	    n = MusicIndex == MusicSample->Length;
	}
#endif

	if (n) {			// End reached
	    SCM cb;

	    PlayingMusic = 0;
#ifdef USE_LIBMODPLUG
	    if (ModFile) {
		ModPlug_Unload(ModFile);
		ModFile=NULL;
	    }
#endif
#if defined(USE_OGG) || defined(USE_FLAC) || defined(USE_MAD)
	    if (MusicSample) {
		free(MusicSample);
		MusicSample=NULL;
	    }
#endif
	    // FIXME: we are inside the SDL callback!
	    if (CallbackMusic) {
		cb = gh_symbol2scm("music-stopped");
		if (!gh_null_p(symbol_boundp(cb, NIL))) {
		    SCM value;

		    value = symbol_value(cb, NIL);
		    if (!gh_null_p(value)) {
			gh_apply(value, NIL);
		    }
		}
	    }
	}
    }
}

#else

global void PlayMusic(const char* name __attribute__((unused)))
{
}

global void StopMusic(void)
{
}

#define MixMusicToStereo32(buffer,size)

#endif

/**
**	Check cdrom.
**
**	Perodic called from the main loop.
*/
global void CDRomCheck(void)
{
#ifdef USE_SDLCD
    if (strcmp(CDMode, ":off") && strcmp(CDMode, ":stopped") && SDL_CDStatus(CDRom) == 1) {
	if (!strcmp(CDMode, ":all")) {
	    PlayMusic(":all");
	} else if (!strcmp(CDMode, ":random")) {
	    PlayMusic(":random");
	}
    }
#endif

#ifdef USE_LIBCDA
    if (strcmp(CDMode, ":off") && strcmp(CDMode, ":stopped") && !cd_current_track()) {
	DebugLevel0Fn("Playing new track\n");
	if (!strcmp(CDMode, ":all")) {
	    PlayMusic(":all");
	} else if (!strcmp(CDMode, ":random")) {
	    PlayMusic(":random");
	}
    } else if (strcmp(CDMode, ":off") && strcmp(CDMode, ":stopped")) {
	DebugLevel0Fn("get track\n");
        CDTrack = cd_current_track() + 1;
	if (CDTrack > NumCDTracks)
	    CDTrack = 1;
    }
#endif
}

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Mix sample to buffer.
**
**	The input samples are adjusted by the local volume and resampled
**	to the output frequence.
**
**	@param sample	Input sample
**	@param index	Position into input sample
**	@param volume	Volume of the input sample
**	@param stereo	Stereo (left/right) position of sample
**	@param buffer	Output buffer
**	@param size	Size of the output buffer to be filled
**	
**	@return		the number of bytes used to fill buffer
**
**	@todo		Can mix faster if signed 8 bit buffers are used.
**	@todo	FIXME:	Remove float for stereo!
*/
local int MixSampleToStereo32(Sample* sample,int index,unsigned char volume,
			      float stereo,int* buffer,int size)
{
    int ri;				// read index
    int wi;				// write index
    int length;
    int v;
    int local_volume;
    float left,right;

    local_volume=((int)volume+1)*2*GlobalVolume/MaxVolume;
    length=sample->Length-index;
    if( stereo < 0.0 ) {
	left=1.0;
	right=1.0+stereo;
    }
    else {
	left=1.0-stereo;
	right=1.0;
    }
    DebugLevel3("Length %d\n",length);

    // FIXME: support other formats, stereo or 32 bit ...

    if( sample->SampleSize==8 ) {
	unsigned char* rp;

	rp=sample->Data+index;
	if( (size*sample->Frequency)/SoundFrequency/2>length ) {
	    size=(length*SoundFrequency/sample->Frequency)*2;
	}

	// mix mono, 8 bit sound to stereo, 32 bit
	for( ri=wi=0; wi<size; ) {
	    ri=(wi*sample->Frequency)/SoundFrequency;
	    ri/=2;			// adjust for mono

	    // FIXME: must interpolate samples!
	    v=(rp[ri]-127)*local_volume;

	    buffer[wi++]+=v*left;
	    buffer[wi++]+=v*right;	// left+right channel
	}
	ri=(wi*sample->Frequency)/SoundFrequency;
	ri/=2;				// adjust for mono
	DebugLevel3("Mixed %d bytes to %d\n",ri,wi);
    } else {
	short* rp;

	DebugCheck( index&1 );

	rp=(short*)(sample->Data+index);
	if( (size*sample->Frequency)/SoundFrequency>length ) {
	    size=(length*SoundFrequency/sample->Frequency);
	}

	// mix mono, 16 bit sound to stereo, 32 bit
	for( ri=wi=0; wi<size; ) {
	    ri=(wi*sample->Frequency)/SoundFrequency;
	    ri/=2;			// adjust for mono

	    // FIXME: must interpolate samples!
	    v=rp[ri]*local_volume/256;

	    buffer[wi++]+=v*left;
	    buffer[wi++]+=v*right;
	}
	ri=(wi*sample->Frequency)/SoundFrequency;
    }

    return ri;
}

/*----------------------------------------------------------------------------
--	Channels and other internal variables
----------------------------------------------------------------------------*/

#define MaxChannels	16

typedef struct _sound_channel_ {
    unsigned char	Command;	// channel command
    int			Point;		// point into sample
    Sample*		Sample;		// sample to play
    Origin              Source;         // unit playing
    unsigned char       Volume;         // Volume of this channel
    SoundId             Sound;          // The sound currently played
    float               Stereo;         // stereo value (-1.0 to 1.0)
} SoundChannel;

#define ChannelFree	0		// channel is free
#define ChannelPlay	3		// channel is playing

/*
**	All possible sound channels.
*/
global SoundChannel Channels[MaxChannels];
global int NextFreeChannel;

/*
** Selection handling
*/
typedef struct _selection_handling_ {
    Origin Source;		// origin of the sound
    ServerSoundId Sound;	// last sound played by this unit
    unsigned char HowMany;	// number of sound played in this group
} SelectionHandling;

SelectionHandling SelectionHandler={{NULL,0},NULL,0};

/*
** Source registration
*/
/// hash table used to store unit to channel mapping
#ifdef USE_GLIB
local GHashTable* UnitToChannel;
#else
local hashtable(int,61) UnitToChannel;
#endif


/**
** Distance to Volume Mapping
*/
local int ViewPointOffset;

/**
** Number of free channels
*/
local int HowManyFree(void) {
  int channel;
  int nb;
  nb=0;
  channel=NextFreeChannel;
  while(channel<MaxChannels) {
      nb++;
      channel=Channels[channel].Point;
  }
  return nb;
}

/*
** Check whether to discard or not a sound request
*/
local int KeepRequest(SoundRequest* sr) {
    //FIXME: take fight flag into account
    int channel;

    if (sr->Sound==NO_SOUND) {
	return 0;
    }
#ifdef USE_GLIB
    if ( (channel=(int)(long)g_hash_table_lookup(UnitToChannel,
				      (gpointer)(sr->Source.Base))) ) {
	channel--;
	if (Channels[channel].Source.Id==sr->Source.Id) {
	    //FIXME: decision should take into account the sound
	    return 0;
	}
	return 1;
    }
#else
    {
	const SoundChannel *theChannel;

	// slow but working solution: we look for the source in the channels
	theChannel=Channels;
	for(channel=0;channel<MaxChannels;channel++) {
	    if ((*theChannel).Command == ChannelPlay
		    && (*theChannel).Source.Base==sr->Source.Base
		    && (*theChannel).Source.Id==sr->Source.Id) {
		//FIXME: decision should take into account the sound
		return 0;
	    }
	    theChannel++;
	}
    }
#endif
    return 1;
}

/*
** Register the source of a request in the selection handling hash table.
*/
local void RegisterSource(SoundRequest* sr,int channel)
{
    // always keep the last registered channel
    // use channel+1 to avoid inserting null values
    //FIXME: we should use here the unique identifier of the source, not only
    // the pointer
#ifdef USE_GLIB
    g_hash_table_insert(UnitToChannel,(gpointer)(long)(sr->Source.Base),
			(gpointer)(channel+1));
#else
    int i;
    void* p;

    i=channel;
    p=sr;
    DebugLevel3Fn("FIXME: must write %p -> %d\n",sr,channel);
#endif
    DebugLevel3("Registering %p (channel %d)\n",sr->Source.Base,channel);
}

/*
** Remove the source of a channel from the selection handling hash table.
*/
local void UnRegisterSource(int channel) {
#ifdef USE_GLIB
    if ( (int)(long)g_hash_table_lookup(UnitToChannel,
				  (gpointer)(Channels[channel].Source.Base))
	 ==channel+1) {
	g_hash_table_remove(UnitToChannel,
			    (gpointer)(long)(Channels[channel].Source.Base));
    }
#else
    int i;

    i=channel;
    DebugLevel3Fn("FIXME: must write %d\n", channel);
#endif
    DebugLevel3("Removing %p (channel %d)\n",Channels[channel].Source.Base,
		channel);
}

/**
**	Compute a suitable volume for something taking place at a given
**	distance from the current view point.
**
**	@param d	distance
**	@param range	range
**
**	@return		volume for given distance (0..??)
*/
local unsigned char VolumeForDistance(unsigned short d,unsigned char range) {
    int d_tmp;
    int range_tmp;
    //FIXME: THIS IS SLOW!!!!!!!
    if (d <= ViewPointOffset || range==INFINITE_SOUND_RANGE)
	return MaxVolume;
    else {
	if (range) {
	    d-=ViewPointOffset;
	    d_tmp=d*MAX_SOUND_RANGE;
	    range_tmp=DistanceSilent*range;
	    DebugLevel3("Distance: %d, Range: %d\n",d_tmp,range_tmp);
	    if (d_tmp > range_tmp )
		return 0;
	    else
		return (unsigned char)((range_tmp-d_tmp)*MAX_SOUND_RANGE/range_tmp);
	} else {
	    return 0;
	}
    }
}

/*
** Compute the volume associated with a request, either by clipping the Range
** parameter of this request, or by mapping this range to a volume.
*/
local unsigned char ComputeVolume(SoundRequest* sr) {
    if (sr->IsVolume) {
	if (sr->Power>MaxVolume) {
	    return MaxVolume;
	} else {
	    return (unsigned char)sr->Power;
	}
    } else {
	// map distance to volume
	return VolumeForDistance(sr->Power,((ServerSoundId)(sr->Sound))->Range);
    }
}

/*
** "Randomly" choose a sample from a sound group.
*/
local Sample* SimpleChooseSample(ServerSoundId sound) {
    if (sound->Number==ONE_SOUND) {
	return sound->Sound.OneSound;
    } else {
	//FIXME: check for errors
	//FIXME: valid only in shared memory context (FrameCounter)
	return sound->Sound.OneGroup[FrameCounter%sound->Number];
    }
}

/*
** Choose a sample from a SoundRequest. Take into account selection and sound
** groups.
*/
local Sample* ChooseSample(SoundRequest* sr)
{
    ServerSoundId TheSound;
    Sample* Result=NO_SOUND;

    if (sr->Sound!=NO_SOUND) {
	TheSound=(ServerSoundId)(sr->Sound);
	if (TheSound->Number==TWO_GROUPS) {
	    // handle a special sound (selection)
	    if (SelectionHandler.Source.Base==sr->Source.Base
		&& SelectionHandler.Source.Id==sr->Source.Id) {
		if (SelectionHandler.Sound==TheSound->Sound.TwoGroups->First) {
		    DebugLevel3("First group\n");
		    Result=SimpleChooseSample(SelectionHandler.Sound);
		    SelectionHandler.HowMany++;
		    if (SelectionHandler.HowMany>=3) {
			SelectionHandler.HowMany=0;
			SelectionHandler.Sound=(ServerSoundId)TheSound->Sound.TwoGroups->Second;
			DebugLevel3("Switching to second group\n");
		    }
		} else {
		    //FIXME: checks for error
		    DebugLevel3("Second group\n");
		    // check wether the second group is really a group
		    if (SelectionHandler.Sound->Number>1) {
			Result=SelectionHandler.Sound->Sound.OneGroup[SelectionHandler.HowMany];
			SelectionHandler.HowMany++;
			if(SelectionHandler.HowMany>=SelectionHandler.Sound->Number) {
			    SelectionHandler.HowMany=0;
			    SelectionHandler.Sound=(ServerSoundId)TheSound->Sound.TwoGroups->First;
			    DebugLevel3("Switching to first group\n");
			}
		    } else {
			Result=SelectionHandler.Sound->Sound.OneSound;
			SelectionHandler.HowMany=0;
			SelectionHandler.Sound=(ServerSoundId)TheSound->Sound.TwoGroups->First;
			DebugLevel3("Switching to first group\n");
		    }
		}
	    } else {
		SelectionHandler.Source=sr->Source;
		SelectionHandler.Sound=TheSound->Sound.TwoGroups->First;
		Result=SimpleChooseSample(SelectionHandler.Sound);
		SelectionHandler.HowMany=1;
	    }
	} else {
	    // normal sound/sound group handling
	    Result=SimpleChooseSample(TheSound);
	    if (sr->Selection) {
		SelectionHandler.Source=sr->Source;
	    }
	}
    }
    return Result;
}

/*
** Free a channel and unregister its source
*/
local void FreeOneChannel(int channel)
{
    Channels[channel].Command=ChannelFree;
    Channels[channel].Point=NextFreeChannel;
    NextFreeChannel=channel;
    if (Channels[channel].Source.Base) {
	// unregister only valid sources
	UnRegisterSource(channel);
    }
}

/*
** Put a sound request in the next free channel. While doing this, the
** function computes the volume of the source and chooses a sample.
*/
local int FillOneChannel(SoundRequest* sr)
{
    int next_free;
    int old_free;

    old_free=NextFreeChannel;
    if (NextFreeChannel<MaxChannels) {
	next_free=Channels[NextFreeChannel].Point;
	Channels[NextFreeChannel].Source=sr->Source;
	Channels[NextFreeChannel].Point=0;
	Channels[NextFreeChannel].Volume=ComputeVolume(sr);
	Channels[NextFreeChannel].Command=ChannelPlay;
	Channels[NextFreeChannel].Sample=ChooseSample(sr);
	Channels[NextFreeChannel].Stereo=sr->Stereo;
	NextFreeChannel=next_free;
    } else {
	// should not happen
	DebugLevel0("***** NO FREE CHANNEL *****\n");
    }
    return old_free;
}

/*
** get orders from the fifo and put them into channels. This function takes
** care of registering sound sources.
//FIXME: is this the correct place to do this?
*/
local void FillChannels(int free_channels,int* discarded,int* started)
{
    int channel;
    SoundRequest* sr;

    sr=SoundRequests+NextSoundRequestOut;
    *discarded=0;
    *started=0;
    while(free_channels && sr->Used) {
	if(KeepRequest(sr)) {
	    DebugLevel3("Source [%p]: start playing request %p at slot %d\n",
			sr->Source.Base,sr->Sound,NextSoundRequestOut);
	    channel=FillOneChannel(sr);
	    if (sr->Source.Base) {
		//Register only sound with a valid origin
		RegisterSource(sr,channel);
	    }
	    free_channels--;
	    DebugLevel3("Free channels: %d\n",free_channels);
	    sr->Used=0;
	    NextSoundRequestOut++;
	    (*started)++;
	} else {
	  // Discarding request (for whatever reason)
	  DebugLevel3("Discarding resquest %p from %p at slot %d\n",
		      sr->Sound,sr->Source.Base,NextSoundRequestOut);
	  sr->Used=0;
	  NextSoundRequestOut++;
	  (*discarded)++;
	}
	if(NextSoundRequestOut>=MAX_SOUND_REQUESTS) {
	    NextSoundRequestOut=0;
	}
	sr=SoundRequests+NextSoundRequestOut;
    }
}

/**
**	Mix channels to stereo 32 bit.
**
**	@param buffer	Buffer for mixed samples.
**	@param size	Number of samples that fits into buffer.
**
**	@return		How many channels become free after mixing them.
*/
local int MixChannelsToStereo32(int* buffer,int size)
{
    int channel;
    int i;
    int new_free_channels;

    new_free_channels=0;
    for( channel=0; channel<MaxChannels; ++channel ) {
	if( Channels[channel].Command==ChannelPlay
		&& Channels[channel].Sample) {
	    i=MixSampleToStereo32(
		    Channels[channel].Sample,Channels[channel].Point,
		    Channels[channel].Volume,Channels[channel].Stereo,
		    buffer,size);
	    Channels[channel].Point+=i;

	    if( Channels[channel].Point>=Channels[channel].Sample->Length ) {
		// free channel as soon as possible (before playing)
		// useful in multithreading
		DebugLevel3("End playing request from %p\n",
			    Channels[channel].Source.Base);
		FreeOneChannel(channel);
		new_free_channels++;
	    }
	}
    }

    return new_free_channels;
}

/**
**	Clip mix to output stereo 8 unsigned bit.
**
**	@param mix	signed 32 bit input.
**	@param size	number of samples in input.
**	@param output	clipped 8 unsigned bit output buffer.
*/
local void ClipMixToStereo8(const int* mix,int size,unsigned char* output)
{
    int s;

    while( size-- ) {
	s=(*mix++)/256;
	if( s>127 ) {
	    *output++=255;
	} else if( s<-127 ) {
	    *output++=0;
	} else {
	    *output++=s+127;
	}
    }
}

/**
**	Clip mix to output stereo 16 signed bit.
**
**	@param mix	signed 32 bit input.
**	@param size	number of samples in input.
**	@param output	clipped 16 signed bit output buffer.
*/
local void ClipMixToStereo16(const int* mix,int size,short* output)
{
    int s;
    const int* end;

    end=mix+size;
    while( mix<end ) {
	s=(*mix++);
	if( s>SHRT_MAX ) {
	    *output++=SHRT_MAX;
	} else if( s<SHRT_MIN ) {
	    *output++=SHRT_MIN;
	} else {
	    *output++=s;
	}
    }
}

/*----------------------------------------------------------------------------
--	Other
----------------------------------------------------------------------------*/

/**
**	Load one sample
**
**	@param name	File name of sample (short version).
**
**	@return		Wav sample loaded from file.
*/
local Sample* LoadSample(const char* name)
{
    Sample* sample;
    char* buf;

    buf = strdcat3(FreeCraftLibPath, "/sounds/", name);
#ifdef USE_OGG
    if( (sample=LoadOgg(buf)) ) {
	return sample;
    }
#endif
#ifdef USE_FLAC
    if( (sample=LoadFlac(buf)) ) {
	return sample;
    }
#endif
#ifdef USE_MAD
    if( (sample=LoadMp3(buf)) ) {
	return sample;
    }
#endif
    if( !(sample=LoadWav(buf)) ) {
	fprintf(stderr,"Can't load the sound `%s'\n",name);
    }
    // FIXME: should support more sample formats, mp3.
    free(buf);
    return sample;
}

/**
**	Ask the sound server to register a sound (and currently to load it)
**	and to return an unique identifier for it. The unique identifier is
**	memory pointer of the server.
**
**	@param files	An array of wav files.
**	@param number	Number of files belonging together.
**
**	@return		the sound unique identifier
**
**	@todo	FIXME: Must handle the errors better.
**		FIXME: Support for more sample files (ogg/flac/mp3).
*/
global SoundId RegisterSound(char *files[], unsigned number)
{
    unsigned i;
    ServerSoundId id;

    id = malloc(sizeof(*id));
    if (number > 1) {			// load a sound group
	id->Sound.OneGroup = malloc(sizeof(Sample *) * number);
	for (i = 0; i < number; i++) {
	    DebugLevel3("Registering `%s'\n", files[i]);
	    id->Sound.OneGroup[i] = LoadSample(files[i]);
	    if (!id->Sound.OneGroup[i]) {
		free(id->Sound.OneGroup);
		free(id);
		return NO_SOUND;
	    }
	}
	id->Number = number;
    } else {				// load an unique sound
	DebugLevel3("Registering `%s'\n", files[0]);
	id->Sound.OneSound = LoadSample(files[0]);
	if (!id->Sound.OneSound) {
	    free(id);
	    return NO_SOUND;
	}
	id->Number = ONE_SOUND;
    }
    id->Range = MAX_SOUND_RANGE;
    return (SoundId) id;
}

/**
** 	Ask the sound server to put together two sounds to form a special sound.
**
**	@param first	first part of the group
**	@param second	second part of the group
**
**	@return		the special sound unique identifier
*/
global SoundId RegisterTwoGroups(SoundId first, SoundId second)
{
    ServerSoundId id;

    if (first == NO_SOUND || second == NO_SOUND) {
	return NO_SOUND;
    }
    id = (ServerSoundId) malloc(sizeof(*id));
    id->Number = TWO_GROUPS;
    id->Sound.TwoGroups = (TwoGroups *) malloc(sizeof(TwoGroups));
    id->Sound.TwoGroups->First = first;
    id->Sound.TwoGroups->Second = second;
    id->Range = MAX_SOUND_RANGE;
    return (SoundId) id;
}

/*
** Modify the range of a given sound.
*/
global void SetSoundRange(SoundId sound,unsigned char range) {
    if (sound!=NO_SOUND) {
	((ServerSoundId)sound)->Range=range;
	DebugLevel3("Setting sound <%p> to range %u\n",sound,range);
    }
}

/**
**	Mix into buffer.
**
**	@param buffer	Buffer to be filled with samples. Buffer must be big
**			enough.
**	@param samples	Number of samples.
*/
global void MixIntoBuffer(void* buffer,int samples)
{
    int* mixer_buffer;
    int free_channels;
    int dummy1;
    int dummy2;

    free_channels=HowManyFree();
    FillChannels(free_channels,&dummy1,&dummy2);

    // Create empty mixer buffer
    mixer_buffer=alloca(samples*sizeof(*mixer_buffer));
    // FIXME: can save the memset here, if first channel sets the values
    memset(mixer_buffer,0,samples*sizeof(*mixer_buffer));

    // Add channels to mixer buffer
    MixChannelsToStereo32(mixer_buffer,samples);
    // Add music to mixer buffer
    MixMusicToStereo32(mixer_buffer,samples);

#if SoundSampleSize==8
    ClipMixToStereo8(mixer_buffer,samples,buffer);
#endif
#if SoundSampleSize==16
    ClipMixToStereo16(mixer_buffer,samples,buffer);
#endif
}

#ifndef USE_SDLA
/**
**	Write buffer to sound card.
*/
global void WriteSound(void)
{
#if SoundSampleSize==8
    char buffer[1024];
#endif
#if SoundSampleSize==16
    short buffer[1024];
#endif

    // ARI: If DSP open had failed: No soundcard, other user, etc..
    if (SoundFildes == -1) {
	DebugLevel0Fn("Shouldn't be reached\n");
	return;
    }

    DebugLevel3Fn("\n");

    if( 0 ) {
	audio_buf_info info;

	ioctl(SoundFildes,SNDCTL_DSP_GETOSPACE,&info);
	DebugLevel0("%lu Free bytes %d\n",GameCycle,info.bytes);
	if( info.bytes<sizeof(buffer) ) {
	    return;
	}
    }

    MixIntoBuffer(buffer,sizeof(buffer)/sizeof(*buffer));

    while( write(SoundFildes,buffer,sizeof(buffer))==-1 ) {
	switch( errno ) {
	    case EAGAIN:
	    case EINTR:
		continue;
	}
	perror("write");
	break;
    }
}
#endif

#ifdef USE_SDLA	// {

global void WriteSound(void)
{
}

/**
**	Fill buffer for the sound card.
**
**	@see SDL_OpenAudio
**
**	@param udata	the pointer stored in userdata field of SDL_AudioSpec.
**	@param stream	pointer to buffer you want to fill with information.
**	@param len	is length of audio buffer in bytes.
*/
global void FillAudio(void* udata __attribute__((unused)),Uint8* stream,int len)
{
#if SoundSampleSize==16
    len>>=1;
#endif
    DebugLevel3Fn("%d\n",len);

    MixIntoBuffer(stream,len);
}
#endif	// } USE_SDLA

#ifdef USE_THREAD	// {

/**
**	FIXME: docu
*/
global void WriteSoundThreaded(void)
{
    int mixer_buffer[1024];
#if SoundSampleSize==8
    char buffer[1024];
#endif
#if SoundSampleSize==16
    short buffer[1024];
#endif
    int new_free_channels;
    int free_channels;
    int how_many_playing;
    int discarded_request;
    int started_request;

    DebugLevel3Fn("\n");

    free_channels = MaxChannels;
    how_many_playing = 0;
    // wait for the first sound to come
    sem_wait(&SoundThreadChannelSemaphore);
    for (;;) {
	if (0) {
	    audio_buf_info info;

	    ioctl(SoundFildes, SNDCTL_DSP_GETOSPACE, &info);
	    DebugLevel0("Free bytes %d\n", info.bytes);
	}
	FillChannels(free_channels, &discarded_request, &started_request);
	how_many_playing += started_request;
	new_free_channels = 0;
	if (how_many_playing || PlayingMusic) {

	    memset(mixer_buffer, 0, sizeof(mixer_buffer));

	    if (how_many_playing) {
		new_free_channels = MixChannelsToStereo32(mixer_buffer,
			sizeof(mixer_buffer) / sizeof(int));
	    }
	    MixMusicToStereo32(mixer_buffer,
		sizeof(mixer_buffer) / sizeof(int));

#if SoundSampleSize==8
	    ClipMixToStereo8(mixer_buffer, sizeof(mixer_buffer) / sizeof(int),
		buffer);
#endif
#if SoundSampleSize==16
	    ClipMixToStereo16(mixer_buffer, sizeof(mixer_buffer) / sizeof(int),
		buffer);
#endif

	    while (write(SoundFildes, buffer, sizeof(buffer)) == -1) {
		switch (errno) {
		    case EAGAIN:
		    case EINTR:
			continue;
		}
		perror("write");
		break;
	    }
	    how_many_playing -= new_free_channels;
	}
	free_channels = MaxChannels - how_many_playing;
	DebugLevel3("Channels: %d %d %d\n", free_channels, how_many_playing,
	    new_free_channels);
	new_free_channels += discarded_request;
	// decrement semaphore by the number of stopped channels
	for (; new_free_channels > 0; new_free_channels--) {
	    //        sem_getvalue(&SoundThreadChannelSemaphore,&tmp);
	    //        DebugLevel3("SoundSemaphore: %d\n",tmp);
	    sem_wait(&SoundThreadChannelSemaphore);
	    //        sem_getvalue(&SoundThreadChannelSemaphore,&tmp);
	    //        DebugLevel3("SoundSemaphore: %d\n",tmp);
	}
    }
}

#endif	// } USE_THREAD

/**
**	Initialize sound card.
*/
global int InitSound(void)
{
    int dummy;

#ifdef USE_SDLA
    //
    //	Open sound device, 8bit samples, stereo.
    //
    if( InitSdlSound(SoundDeviceName,SoundFrequency,SoundSampleSize,
	    WaitForSoundDevice) ) {
	return 1;
    }
#else
    //
    //	Open dsp device, 8bit samples, stereo.
    //
    if( InitOssSound(SoundDeviceName,SoundFrequency,SoundSampleSize,
	    WaitForSoundDevice) ) {
	return 1;
    }
#endif	// USE_SDLA

    //	ARI:	The following must be done here to allow sound to work in
    //		pre-start menus!
    // initialize channels
    for(dummy=0;dummy<MaxChannels; ++dummy) {
	Channels[dummy].Point=dummy+1;
    }

    // initialize unit to channel hash table
    // WARNING: creation is only valid for a hash table using pointers as key
#ifdef USE_GLIB
    UnitToChannel=g_hash_table_new(g_direct_hash,NULL);
#else
    DebugLevel0Fn("FIXME: must write non GLIB hash functions\n");
#endif

    PlayMusic(TitleMusic);

    return 0;
}

/**
**	Initialize sound server structures (and thread)
*/
global int InitSoundServer(void)
{
    //FIXME: Valid only in shared memory context!
    DistanceSilent=3*max(MapWidth,MapHeight);
    DebugLevel2("Distance Silent: %d\n",DistanceSilent);
    ViewPointOffset=max(MapWidth/2,MapHeight/2);
    DebugLevel2("ViewPointOffset: %d\n",ViewPointOffset);

#ifdef USE_THREAD
    if (WithSoundThread) {
      //prepare for the sound thread
      if( sem_init(&SoundThreadChannelSemaphore,0,0) ) {
	//FIXME: better error handling
	perror(__FUNCTION__);
	close(SoundFildes);
	SoundFildes=-1;
	return 1;
      }
      if( pthread_create(&SoundThread,NULL,(void *)&WriteSoundThreaded,NULL) ) {
	//FIXME: better error handling
	perror(__FUNCTION__);
	close(SoundFildes);
	SoundFildes=-1;
	return 1;
      }
      SoundThreadRunning=1;
    }
#endif

    return 0;
}

/**
**	Cleanup sound server.
*/
global void QuitSound(void)
{
#ifdef USE_SDLA
    SDL_CloseAudio();
    SoundFildes=-1;
#else
    if (SoundFildes != -1) {
	close(SoundFildes);
	SoundFildes=-1;
    }
#endif
    QuitCD();
}

global void QuitCD(void)
{
#ifdef USE_SDLCD
    if (strcmp(CDMode,":off") && strcmp(CDMode,":stopped"))
	SDL_CDStop(CDRom);
	CDMode = ":stopped";
    if (strcmp(CDMode,":off")) {
        SDL_CDClose(CDRom);
	CDMode = ":off";
    }
#endif

#ifdef USE_LIBCDA
    if (strcmp(CDMode,":off") && strcmp(CDMode,":stopped"))
        cd_stop();
	CDMode = ":stopped";
    if (strcmp(CDMode,":off")) {
        cd_close();
        cd_exit();
	CDMode = ":off";
    }
#endif
}

#endif	// } WITH_SOUND

global int WaitForSoundDevice;		/// Block until sound device available

//@}
