/****************************/
/*     SOUND ROUTINES       */
/* By Brian Greenstone      */
/* (c)1994-99 Pangea Software  */
/* (c)2021 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
#include <stdio.h>


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SongCompletionProc(SndChannelPtr chan);
static short FindSilentChannel(void);
static void Calc3DEffectVolume(short effectNum, TQ3Point3D *where, float volAdjust, u_long *leftVolOut, u_long *rightVolOut);


/****************************/
/*    CONSTANTS             */
/****************************/

#define		MAX_CHANNELS			14


typedef struct
{
	int				bank;
	const char*		filename;
	float			refDistance;
	int				flags;
} EffectDef;

typedef struct
{
	SndListHandle	sndHandle;
	long			sndOffset;
	short			lastPlayedOnChannel;
	u_long			lastLoudness;
} LoadedEffect;


#define	VOLUME_DISTANCE_FACTOR	.004f		// bigger == sound decays FASTER with dist, smaller = louder far away


enum
{
	// At most one instance of the effect may be played back at once.
	// If the effect is started again, the previous instance is stopped.
	kSoundFlag_Unique = 1 << 0,

	// When combined with kSoundFlag_Unique, any new instances of the effect
	// will be ignored As long as an instance of the effect is still playing.
	kSoundFlag_DontInterrupt = 1 << 1,

	// Turn off linear interpolation for this effect.
	kSoundFlag_NoInterp = 1 << 2,
};

/**********************/
/*     VARIABLES      */
/**********************/

float						gGlobalVolume = .4;

float						gSongVolume = 1;		// multiplied by gGlobalVolume

TQ3Point3D					gEarCoords;				// coord of camera plus a bit to get pt in front of camera
static	TQ3Vector3D			gEyeVector;

static	LoadedEffect		gLoadedEffects[NUM_EFFECTS];

static	SndChannelPtr		gSndChannel[MAX_CHANNELS];
static	ChannelInfoType		gChannelInfo[MAX_CHANNELS];

static short				gMaxChannels = 0;

Boolean						gSongPlayingFlag = false;
static Boolean				gResetSong = false;
static Boolean				gLoopSongFlag = true;

static	SndChannelPtr		gMusicChannel=nil;
Boolean						gMuteMusicFlag = false;
static short				gCurrentSong = -1;


		/*****************/
		/* EFFECTS TABLE */
		/*****************/

static const char* kSoundBankNames[NUM_SOUNDBANKS] =
{
	[SOUNDBANK_MAIN]	= "main",
	[SOUNDBANK_LAWN]	= "lawn",
	[SOUNDBANK_POND]	= "pond",
	[SOUNDBANK_HIVE]	= "hive",
	[SOUNDBANK_NIGHT]	= "night",
	[SOUNDBANK_FOREST]	= "forest",
	[SOUNDBANK_ANTHILL]	= "anthill",
	[SOUNDBANK_BONUS]	= "bonus",
};

static const EffectDef	kEffectsTable[] =
{
	[EFFECT_SELECT]			= {SOUNDBANK_MAIN,		"select",        800, kSoundFlag_Unique },
	[EFFECT_JUMP]			= {SOUNDBANK_MAIN,		"jump",         1400, 0	},
	[EFFECT_THROWSPEAR]		= {SOUNDBANK_MAIN,		"throwspear",   1300, 0	},
	[EFFECT_HITDIRT]		= {SOUNDBANK_MAIN,		"hitdirt",       900, 0	},
	[EFFECT_POP]			= {SOUNDBANK_MAIN,		"pop",           800, 0	},
	[EFFECT_GETPOW]			= {SOUNDBANK_MAIN,		"getpow",        500, 0	},
	[EFFECT_BUZZ]			= {SOUNDBANK_MAIN,		"flybuzz",        50, 0	},
	[EFFECT_OUCH]			= {SOUNDBANK_MAIN,		"gethit",        900, 0	},
	[EFFECT_KICK]			= {SOUNDBANK_MAIN,		"kick",          700, 0	},
	[EFFECT_POUND]			= {SOUNDBANK_MAIN,		"pound",         900, kSoundFlag_Unique },
	[EFFECT_SPEEDBOOST]		= {SOUNDBANK_MAIN,		"speedboost",    800, kSoundFlag_Unique },
	[EFFECT_MORPH]			= {SOUNDBANK_MAIN,		"morph",         600, kSoundFlag_Unique },
	[EFFECT_FIRECRACKER]	= {SOUNDBANK_MAIN,		"firecracker",  2500, kSoundFlag_Unique | kSoundFlag_NoInterp },
	[EFFECT_SHIELD]			= {SOUNDBANK_MAIN,		"shield",       2000, 0	},
	[EFFECT_SPLASH]			= {SOUNDBANK_MAIN,		"splash",        900, 0	},
	[EFFECT_BUDDYLAUNCH]	= {SOUNDBANK_MAIN,		"buddylaunch",   300, kSoundFlag_NoInterp },
	[EFFECT_RESCUE]			= {SOUNDBANK_MAIN,		"ladybugrescue", 500, kSoundFlag_Unique },
	[EFFECT_CHECKPOINT]		= {SOUNDBANK_MAIN,		"checkpoint",   1000, 0	},
	[EFFECT_KABLAM]			= {SOUNDBANK_MAIN,		"kablam",       2000, 0	},
	[EFFECT_BOATENGINE]		= {SOUNDBANK_POND,		"boatengine",   2400, 0	},
	[EFFECT_WATERBUG]		= {SOUNDBANK_POND,		"waterbug",      400, 0	},
	[EFFECT_FOOTSTEP]		= {SOUNDBANK_FOREST,	"footstep",     4000, 0 },
	[EFFECT_HELICOPTER]		= {SOUNDBANK_FOREST,	"helicopter",    800, 0	},
	[EFFECT_PLASMABURST]	= {SOUNDBANK_FOREST,	"plasmaburst",  3500, 0	},
	[EFFECT_PLASMAEXPLODE]	= {SOUNDBANK_FOREST,	"explosion",    4500, 0	},
	[EFFECT_FIRECRACKLE]	= {SOUNDBANK_FOREST,	"firecrackle",  8000, 0	},
	[EFFECT_SLURP]			= {SOUNDBANK_POND,		"slurp",         500, 0	},
	[EFFECT_ROCKSLAM]		= {SOUNDBANK_NIGHT,		"rockslam",     1000, 0	},
	[EFFECT_VALVEOPEN]		= {SOUNDBANK_ANTHILL,	"valveopen",    1000, 0	},
	[EFFECT_WATERLEAK]		= {SOUNDBANK_ANTHILL,	"waterleak",    1000, 0	},
	[EFFECT_KINGSHOOT]		= {SOUNDBANK_ANTHILL,	"shoot",        7000, 0	},
	[EFFECT_KINGEXPLODE]	= {SOUNDBANK_ANTHILL,	"explosion",    8000, 0	},
	[EFFECT_KINGCRACKLE]	= {SOUNDBANK_ANTHILL,	"firecrackle",  2000, 0	},
	[EFFECT_SIZZLE]			= {SOUNDBANK_ANTHILL,	"sizzle",       2000, 0	},
	[EFFECT_KINGLAUGH]		= {SOUNDBANK_ANTHILL,	"laugh",        2000, kSoundFlag_Unique | kSoundFlag_DontInterrupt },
	[EFFECT_PIPECLANG]		= {SOUNDBANK_ANTHILL,	"pipeclang",    2000, kSoundFlag_Unique | kSoundFlag_DontInterrupt },
	[EFFECT_OPENLAWNDOOR]	= {SOUNDBANK_LAWN,		"dooropen",     1500, 0	},
	[EFFECT_OPENNIGHTDOOR]	= {SOUNDBANK_NIGHT,		"dooropen",     1500, 0	},
	[EFFECT_STINGERSHOOT]	= {SOUNDBANK_HIVE,		"stingershoot", 1300, kSoundFlag_Unique },
	[EFFECT_PUMP]			= {SOUNDBANK_HIVE,		"pump",          600, 0	},
	[EFFECT_PLUNGER]		= {SOUNDBANK_HIVE,		"plunger",       600, 0	},
	[EFFECT_BONUSBELL]		= {SOUNDBANK_BONUS,		"bell",         1000, 0	},
	[EFFECT_BONUSCLICK]		= {SOUNDBANK_BONUS,		"click",        1000, kSoundFlag_Unique },
};


/********************* INIT SOUND TOOLS ********************/

void InitSoundTools(void)
{
OSErr		iErr;

	gMaxChannels = 0;

			/* INIT BANK INFO */

	memset(gLoadedEffects, 0, sizeof(gLoadedEffects));

			/******************/
			/* ALLOC CHANNELS */
			/******************/

			/* MAKE MUSIC CHANNEL */
			
	SndNewChannel(&gMusicChannel,sampledSynth,initStereo,nil);


			/* ALL OTHER CHANNELS */
				
	for (gMaxChannels = 0; gMaxChannels < MAX_CHANNELS; gMaxChannels++)
	{
			/* NEW SOUND CHANNEL */
			
		iErr = SndNewChannel(&gSndChannel[gMaxChannels],sampledSynth,0,/*NewSndCallBackUPP(CallBackFn)*/nil);
		if (iErr)												// if err, stop allocating channels
			break;
	}
}

/******************* LOAD A SOUND EFFECT ************************/

void LoadSoundEffect(int effectNum)
{
char path[256];
FSSpec spec;
short refNum;
OSErr err;

	LoadedEffect* loadedSound = &gLoadedEffects[effectNum];
	const EffectDef* effectDef = &kEffectsTable[effectNum];

	if (loadedSound->sndHandle)
	{
		// already loaded
		return;
	}

	snprintf(path, sizeof(path), ":audio:%s.sounds:%s.aiff", kSoundBankNames[effectDef->bank], effectDef->filename);

	err = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &spec);
	if (err != noErr)
	{
		DoAlert(path);
		return;
	}

	err = FSpOpenDF(&spec, fsRdPerm, &refNum);
	GAME_ASSERT_MESSAGE(err == noErr, path);

	loadedSound->sndHandle = Pomme_SndLoadFileAsResource(refNum);
	GAME_ASSERT_MESSAGE(loadedSound->sndHandle, path);

	FSClose(refNum);

			/* GET OFFSET INTO IT */

	GetSoundHeaderOffset(loadedSound->sndHandle, &loadedSound->sndOffset);

			/* PRE-DECOMPRESS IT */

	Pomme_DecompressSoundResource(&loadedSound->sndHandle, &loadedSound->sndOffset);
}

/******************* DISPOSE OF A SOUND EFFECT ************************/

void DisposeSoundEffect(int effectNum)
{
	LoadedEffect* loadedSound = &gLoadedEffects[effectNum];

	if (loadedSound->sndHandle)
	{
		DisposeHandle((Handle) loadedSound->sndHandle);
		memset(loadedSound, 0, sizeof(LoadedEffect));
	}
}

/******************* LOAD SOUND BANK ************************/

void LoadSoundBank(int bankNum)
{
	StopAllEffectChannels();

			/****************************/
			/* LOAD ALL EFFECTS IN BANK */
			/****************************/

	for (int i = 0; i < NUM_EFFECTS; i++)
	{
		if (kEffectsTable[i].bank == bankNum)
		{
			LoadSoundEffect(i);
		}
	}
}

/******************** DISPOSE SOUND BANK **************************/

void DisposeSoundBank(int bankNum)
{
	StopAllEffectChannels();									// make sure all sounds are stopped before nuking any banks

			/* FREE ALL SAMPLES */

	for (int i = 0; i < NUM_EFFECTS; i++)
	{
		if (kEffectsTable[i].bank == bankNum)
		{
			DisposeSoundEffect(i);
		}
	}
}


/******************* DISPOSE ALL SOUND BANKS *****************/

void DisposeAllSoundBanks(void)
{
	for (int i = 0; i < NUM_SOUNDBANKS; i++)
	{
		DisposeSoundBank(i);
	}
}

/********************* STOP A CHANNEL **********************/
//
// Stops the indicated sound channel from playing.
//

void StopAChannel(short *channelNum)
{
SndCommand 	mySndCmd;
OSErr 		myErr;
SCStatus	theStatus;
short		c = *channelNum;

	if ((c < 0) || (c >= gMaxChannels))		// make sure its a legal #
		return;

	myErr = SndChannelStatus(gSndChannel[c],sizeof(SCStatus),&theStatus);	// get channel info
	if (myErr == noErr && theStatus.scChannelBusy)					// if channel busy, then stop it
	{
		mySndCmd.cmd = flushCmd;	
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);

		mySndCmd.cmd = quietCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);
	}
	
	*channelNum = -1;
	
	gChannelInfo[c].effectNum = -1;	
	
}


/********************* STOP ALL EFFECT CHANNELS **********************/

void StopAllEffectChannels(void)
{
short		i;

	for (i=0; i < gMaxChannels; i++)
	{
		short	c;
		
		c = i;
		StopAChannel(&c);
	}
}


/*************** PAUSE ALL SOUND CHANNELS **************/

void PauseAllChannels(Boolean pause)
{
	SndCommand cmd = { .cmd = pause ? pommePausePlaybackCmd : pommeResumePlaybackCmd };

	for (int c = 0; c < gMaxChannels; c++)
	{
		SndDoImmediate(gSndChannel[c], &cmd);
	}

	if (!gMuteMusicFlag)
	{
		SndDoImmediate(gMusicChannel, &cmd);
	}
}



/****************** UPDATE GLOBAL VOLUME ************************/
//
// Call this whenever gEffectsVolume is changed.  This will update
// all of the sounds with the correct volume.
//

void FadeGlobalVolume(float fadeVolume)
{
	float globalVolumeBackup = gGlobalVolume;

	gGlobalVolume = gGlobalVolume * fadeVolume;

			/* ADJUST VOLUMES OF ALL CHANNELS REGARDLESS IF THEY ARE PLAYING OR NOT */

	for (int c = 0; c < gMaxChannels; c++)
	{
		ChangeChannelVolume(c, gChannelInfo[c].leftVolume, gChannelInfo[c].rightVolume);
	}


			/* UPDATE SONG VOLUME */

	// First, resume song playback if it was paused --
	// e.g. when we're adjusting the volume via pause menu
	SndCommand cmd1 = { .cmd = pommeResumePlaybackCmd };
	SndDoImmediate(gMusicChannel, &cmd1);

	// Now update song channel volume
	uint32_t lv2 = kFullVolume * gSongVolume * fadeVolume;
	uint32_t rv2 = kFullVolume * gSongVolume * fadeVolume;
	SndCommand cmd2 =
	{
		.cmd = volumeCmd,
		.param1 = 0,
		.param2 = (rv2 << 16) | lv2,
	};
	SndDoImmediate(gMusicChannel, &cmd2);


	gGlobalVolume = globalVolumeBackup;
}


/******************** PLAY SONG ***********************/
//
// INPUT: loopFlag = true if want song to loop
//

void PlaySong(short songNum, Boolean loopFlag)
{
static	SndCommand 		mySndCmd;
int		volume;
OSErr	iErr;

	if (songNum == gCurrentSong)					// see if this is already playing
		return;

		/* ZAP ANY EXISTING SONG */
		
	gCurrentSong 	= songNum;
	gLoopSongFlag 	= loopFlag;
	KillSong();
	DoSoundMaintenance();

			/******************************/
			/* OPEN APPROPRIATE AIFF FILE */
			/******************************/

	const char* path = NULL;
	switch(songNum)
	{
		case	SONG_MENU:			path = ":audio:menusong.aiff";		break;
		case	SONG_GARDEN:		path = ":audio:lawnsong.aiff";		break;
		case	SONG_GARDEN_OLD:	path = ":audio:lawnsongold.aiff";	break;
		case	SONG_PANGEA:		path = ":audio:song_pangea.aiff";	break;
		case	SONG_HIGHSCORES:	path = ":audio:highscores.aiff";	break;
		case	SONG_NIGHT:			path = ":audio:night.aiff";			break;
		case	SONG_FOREST:		path = ":audio:forest.aiff";		break;
		case	SONG_POND:			path = ":audio:pondsong.aiff";		break;
		case	SONG_ANTHILL:		path = ":audio:anthillsong.aiff";	break;
		case	SONG_HIVE:			path = ":audio:hivelevel.aiff";		break;
		case	SONG_WIN:			path = ":audio:winsong.aiff";		break;
		case	SONG_LOSE:			path = ":audio:losesong.aiff";		break;
		case	SONG_BONUS:			path = ":audio:bonussong.aiff";		break;
		default:
			DoAlert("PlaySong: unknown song #");
			return;
	}

	short musicFileRefNum = OpenGameFile(path);
	volume = FULL_CHANNEL_VOLUME * gSongVolume;


	gCurrentSong = songNum;
	
	
				/*******************/
				/* START STREAMING */
				/*******************/
		
		/* RESET CHANNEL TO DEFAULT VALUES */

	mySndCmd.cmd = volumeCmd;										// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (volume<<16) | volume;
	SndDoImmediate(gMusicChannel, &mySndCmd);
					
	mySndCmd.cmd 		= freqCmd;									// modify the rate to change the frequency 
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= kMiddleC;
	SndDoImmediate(gMusicChannel, &mySndCmd);
					
			
			
			/* START PLAYING FROM FILE */

	iErr = SndStartFilePlay(
			gMusicChannel,
			musicFileRefNum,
			0,
			0, //STREAM_BUFFER_SIZE,
			nil, // gMusicBuffer
			nil,
			SongCompletionProc,
			true);

	if (iErr)
	{
		FSClose(musicFileRefNum);								// close the file
		DoAlert("PlaySong: SndStartFilePlay failed! (%d)", iErr);
	}
	gSongPlayingFlag = true;


			/* SET LOOP FLAG ON STREAM (SOURCE PORT ADDITION) */
			/* So we don't need to re-read the file over and over. */

	if (loopFlag)
	{
		mySndCmd.cmd = pommeSetLoopCmd;
		mySndCmd.param1 = loopFlag ? 1 : 0;
		mySndCmd.param2 = 0;
		iErr = SndDoImmediate(gMusicChannel, &mySndCmd);
		if (iErr)
			DoFatalAlert("PlaySong: SndDoImmediate (pomme loop extension) failed!");
	}


			/* SEE IF WANT TO MUTE THE MUSIC */
			
//	if (gMuteMusicFlag)													
//		SndPauseFilePlay(gMusicChannel);						// pause it	
}

/***************** SONG COMPLETION PROC *********************/

static void SongCompletionProc(SndChannelPtr chan)
{
	(void) chan;

	if (gSongPlayingFlag && !gLoopSongFlag)
	{
		gResetSong = true;
	}
}


/*********************** KILL SONG *********************/

void KillSong(void)
{
	gCurrentSong = -1;

	if (!gSongPlayingFlag)
		return;

	gSongPlayingFlag = false;											// tell callback to do nothing

	SndStopFilePlay(gMusicChannel, true);								// stop it
}

#pragma mark -


/***************************** PLAY EFFECT 3D ***************************/
//
// NO SSP
//
// OUTPUT: channel # used to play sound
//

short PlayEffect3D(int effectNum, TQ3Point3D *where)
{
short					theChan;
u_long					leftVol, rightVol;

				/* CALC VOLUME */

	Calc3DEffectVolume(effectNum, where, 1.0, &leftVol, &rightVol);
	if ((leftVol+rightVol) == 0)
		return(-1);


	theChan = PlayEffect_Parms(effectNum, leftVol, rightVol, kMiddleC);

	if (theChan != -1)
		gChannelInfo[theChan].volumeAdjust = 1.0;			// full volume adjust
						
	return(theChan);									// return channel #	
}



/***************************** PLAY EFFECT PARMS 3D ***************************/
//
// Plays an effect with parameters in 3D
//
// OUTPUT: channel # used to play sound
//

short PlayEffect_Parms3D(int effectNum, TQ3Point3D *where, u_long rateMultiplier, float volumeAdjust)
{
short			theChan;
u_long			leftVol, rightVol;

				/* CALC VOLUME */

	Calc3DEffectVolume(effectNum, where, volumeAdjust, &leftVol, &rightVol);
	if ((leftVol+rightVol) == 0)
		return(-1);


				/* PLAY EFFECT */
				
	theChan = PlayEffect_Parms(effectNum, leftVol, rightVol, rateMultiplier);
	
	if (theChan != -1)
		gChannelInfo[theChan].volumeAdjust = volumeAdjust;	// remember volume adjuster

	return(theChan);									// return channel #	
}



/************************* UPDATE 3D SOUND CHANNEL ***********************/
//
// Returns TRUE if effectNum was a mismatch or something went wrong
//

Boolean Update3DSoundChannel(int effectNum, short *channel, TQ3Point3D *where)
{
SCStatus		theStatus;
u_long			leftVol,rightVol;
short			c;

	c = *channel;

	if (c == -1)
		return(true);

			/* MAKE SURE THE SAME SOUND IS STILL ON THIS CHANNEL */
			
	if (effectNum != gChannelInfo[c].effectNum)
	{
		*channel = -1;
		return(true);
	}
	

			/* SEE IF SOUND HAS COMPLETED */
			
#if 0	// Source port removal
	if (!gChannelInfo[c].isLooping)										// loopers wont complete, duh.
#endif
	{
		SndChannelStatus(gSndChannel[c],sizeof(SCStatus),&theStatus);	// get channel info
		if (!theStatus.scChannelBusy)									// see if channel not busy
		{
			StopAChannel(channel);							// make sure it's really stopped (OS X sound manager bug)
			return(true);
		}
	}

			/* UPDATE THE THING */

	if (where)
	{
		Calc3DEffectVolume(gChannelInfo[c].effectNum, where, gChannelInfo[c].volumeAdjust, &leftVol, &rightVol);
		if ((leftVol+rightVol) == 0)										// if volume goes to 0, then kill channel
		{
			StopAChannel(channel);
			return(false);
		}

		ChangeChannelVolume(c, leftVol, rightVol);
	}	
	return(false);
}



#pragma mark -

/******************* UPDATE LISTENER LOCATION ******************/

void UpdateListenerLocation(void)
{
TQ3Vector3D	v;

	v.x = gGameViewInfoPtr->currentCameraLookAt.x - gGameViewInfoPtr->currentCameraCoords.x;
	v.y = gGameViewInfoPtr->currentCameraLookAt.y - gGameViewInfoPtr->currentCameraCoords.y;
	v.z = gGameViewInfoPtr->currentCameraLookAt.z - gGameViewInfoPtr->currentCameraCoords.z;

	gEyeVector = v;

	FastNormalizeVector(v.x, v.y, v.z, &v);

	gEarCoords.x = gGameViewInfoPtr->currentCameraCoords.x + (v.x * 300.0f);
	gEarCoords.y = gGameViewInfoPtr->currentCameraCoords.y + (v.y * 300.0f);
	gEarCoords.z = gGameViewInfoPtr->currentCameraCoords.z + (v.z * 300.0f);
}


/***************************** PLAY EFFECT ***************************/
//  
// OUTPUT: channel # used to play sound
//

short PlayEffect(int effectNum)
{
	return(PlayEffect_Parms(effectNum,FULL_CHANNEL_VOLUME,FULL_CHANNEL_VOLUME,kMiddleC));

}


/***************************** PLAY EFFECT PARMS ***************************/
//
// Plays an effect with parameters
//
// OUTPUT: channel # used to play sound
//

short  PlayEffect_Parms(int effectNum, u_long leftVolume, u_long rightVolume, unsigned long rateMultiplier)
{
SndCommand 		mySndCmd;
SndChannelPtr	chanPtr;
short			theChan;
OSErr			myErr;
u_long			lv2,rv2;

			/* GET BANK & SOUND #'S FROM TABLE */

	LoadedEffect* sound = &gLoadedEffects[effectNum];

	GAME_ASSERT_MESSAGE(effectNum >= 0 && effectNum < NUM_EFFECTS, "illegal effect number");
	GAME_ASSERT_MESSAGE(sound->sndHandle, "effect wasn't loaded!");


			/* DON'T PLAY EFFECT MULTIPLE TIMES AT ONCE IF EFFECTS TABLE PREVENTS IT */
			// (Source port addition)

	theChan = sound->lastPlayedOnChannel;
	Byte flags = kEffectsTable[effectNum].flags;

	if (theChan >= 0
		&& (kSoundFlag_Unique & flags)
		&& gChannelInfo[theChan].effectNum == effectNum)
	{
		SCStatus theStatus;
		myErr = SndChannelStatus(gSndChannel[theChan], sizeof(SCStatus), &theStatus);
		if (myErr == noErr && theStatus.scChannelBusy)
		{
			if (kSoundFlag_DontInterrupt & flags)					// don't interrupt if this flag is set
				return -1;
//			else if ((kSoundFlag_DontInterruptLouder & flags) && sound->lastLoudness >= leftVolume + rightVolume)	// don't interrupt louder effect
//				return -1;
			else												// otherwise interrupt current effect, force replay
				StopAChannel(&theChan);
		}
	}

			/* LOOK FOR FREE CHANNEL */

	theChan = FindSilentChannel();
	if (theChan == -1)
	{
		return(-1);
	}

	// Remember channel # on which we played this effect
	sound->lastPlayedOnChannel = theChan;
	sound->lastLoudness = leftVolume + rightVolume;

	lv2 = (float)leftVolume * gGlobalVolume;							// amplify by global volume
	rv2 = (float)rightVolume * gGlobalVolume;					


					/* GET IT GOING */

	chanPtr = gSndChannel[theChan];						
	
	mySndCmd.cmd = flushCmd;	
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd = quietCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd = reInitCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = initStereo | ((flags & kSoundFlag_NoInterp) ? initNoInterp : 0);
	SndDoImmediate(chanPtr, &mySndCmd);

	mySndCmd.cmd = bufferCmd;										// make it play
	mySndCmd.param1 = 0;
	mySndCmd.ptr = ((Ptr)*sound->sndHandle) + sound->sndOffset;		// pointer to SoundHeader
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);
	
	mySndCmd.cmd = volumeCmd;										// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (rv2<<16) | lv2;
	SndDoImmediate(chanPtr, &mySndCmd);
	
	mySndCmd.cmd 		= freqCmd;									// set frequency 
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= rateMultiplier;	
	SndDoImmediate(chanPtr, &mySndCmd);


			/* SET MY INFO */
			
	gChannelInfo[theChan].effectNum 	= effectNum;		// remember what effect is playing on this channel
	gChannelInfo[theChan].leftVolume 	= leftVolume;		// remember requested volume (not the adjusted volume!)
	gChannelInfo[theChan].rightVolume 	= rightVolume;	
	return(theChan);										// return channel #	
}


#pragma mark -



/*************** CHANGE CHANNEL VOLUME **************/
//
// Modifies the volume of a currently playing channel
//

void ChangeChannelVolume(short channel, float leftVol, float rightVol)
{
SndCommand 		mySndCmd;
SndChannelPtr	chanPtr;
u_long			lv2,rv2;

	if (channel < 0)									// make sure it's valid
		return;

	lv2 = leftVol * gGlobalVolume;				// amplify by global volume
	rv2 = rightVol * gGlobalVolume;			

	chanPtr = gSndChannel[channel];						// get the actual channel ptr				

	mySndCmd.cmd = volumeCmd;							// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (rv2<<16) | lv2;			// set volume left & right
	SndDoImmediate(chanPtr, &mySndCmd);

	gChannelInfo[channel].leftVolume = leftVol;				// remember requested volume (not the adjusted volume!)
	gChannelInfo[channel].rightVolume = rightVol;
}



/******************** TOGGLE MUSIC *********************/

void ToggleMusic(void)
{
	gMuteMusicFlag = !gMuteMusicFlag;
	SndPauseFilePlay(gMusicChannel);			// pause it
}


/******************** DO SOUND MAINTENANCE *************/
//
// 		UpdateInput() must have already been called
//

void DoSoundMaintenance(void)
{
	if (!gEnteringName)
	{	
					/* SEE IF TOGGLE MUSIC */

		if (GetNewKeyState(kKey_ToggleMusic))
			ToggleMusic();			
	}

				/* SEE IF STREAMED MUSIC STOPPED - SO RESET */

	if (gResetSong)
	{
		gResetSong = false;
		if (!gLoopSongFlag)
			KillSong();
	}
}



/******************** FIND SILENT CHANNEL *************************/

static short FindSilentChannel(void)
{
short		theChan;
OSErr		myErr;
SCStatus	theStatus;

	for (theChan=0; theChan < gMaxChannels; theChan++)
	{
		myErr = SndChannelStatus(gSndChannel[theChan],sizeof(SCStatus),&theStatus);	// get channel info
		if (myErr)
			continue;
		if (!theStatus.scChannelBusy)					// see if channel not busy
		{
			return(theChan);
		}
	}
	
			/* NO FREE CHANNELS */
	
	return(-1);										
}


/********************** IS EFFECT CHANNEL PLAYING ********************/

Boolean IsEffectChannelPlaying(short chanNum)
{
SCStatus	theStatus;

	SndChannelStatus(gSndChannel[chanNum],sizeof(SCStatus),&theStatus);	// get channel info
	return (theStatus.scChannelBusy);
}



/******************** CALC 3D EFFECT VOLUME *********************/

static void Calc3DEffectVolume(short effectNum, TQ3Point3D *where, float volAdjust, u_long *leftVolOut, u_long *rightVolOut)
{
float	dist;
float	refDist,volumeFactor;
u_long	volume,left,right;
u_long	maxLeft,maxRight;

			/* TONE DOWN ANNOYING BUZZ EFFECT */

	if (effectNum == EFFECT_BUZZ)
	{
		volAdjust *= .5f;
	}

	dist 	= Q3Point3D_Distance(where, &gEarCoords);		// calc dist to sound for pane 0
		
			/* DO VOLUME CALCS */
			
	refDist = kEffectsTable[effectNum].refDistance;			// get ref dist

	dist -= refDist;
	if (dist <= EPS)
		volumeFactor = 1.0f;
	else
	{
		volumeFactor = 1.0f / (dist * VOLUME_DISTANCE_FACTOR);
		if (volumeFactor > 1.0f)
			volumeFactor = 1.0f;
	}
	
	volume = (float)FULL_CHANNEL_VOLUME * volumeFactor * volAdjust;	
	
				
	if (volume < 6)							// if really quiet, then just turn it off
	{
		*leftVolOut = *rightVolOut = 0;
		return;
	}

			/************************/
			/* DO STEREO SEPARATION */
			/************************/
	
	else		
	{
		float		volF = (float)volume;
		
		if (volF > 256.0f)
		{
//			printf("Sfx %d volume %f clipped\n", effectNum, volF);
			volF = 256.0f;
		}
		
		TQ3Vector2D	earToSound,lookVec;
		float		dot,cross;
		
		maxLeft = maxRight = 0;
		
			/* CALC VECTOR TO SOUND */
			
		earToSound.x = where->x - gEarCoords.x;
		earToSound.y = where->z - gEarCoords.z;
		FastNormalizeVector2D(earToSound.x, earToSound.y, &earToSound);
		
		
			/* CALC EYE LOOK VECTOR */
			
		FastNormalizeVector2D(gEyeVector.x, gEyeVector.z, &lookVec);
			

			/* DOT PRODUCT  TELLS US HOW MUCH STEREO SHIFT */
			
		dot = 1.0f - fabs(Q3Vector2D_Dot(&earToSound,  &lookVec));
		if (dot < 0.0f)
			dot = 0.0f;
		else
		if (dot > 1.0f)
			dot = 1.0f;


			/* CROSS PRODUCT TELLS US WHICH SIDE */
			
		cross = Q3Vector2D_Cross(&earToSound,  &lookVec);
		
		
				/* DO LEFT/RIGHT CALC */
				
		if (cross > 0.0f)
		{
			left 	= volF + (volF * dot);
			right 	= volF - (volF * dot);
		}
		else
		{
			right 	= volF + (volF * dot);
			left 	= volF - (volF * dot);
		}
		
		
				/* KEEP MAX */
				
		if (left > maxLeft)
			maxLeft = left;
		if (right > maxRight)
			maxRight = right;
				
	}

	*leftVolOut = maxLeft;
	*rightVolOut = maxRight;		
}

