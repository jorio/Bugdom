/****************************/
/*     SOUND ROUTINES       */
/* (c)1994-99 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

extern	short		gMainAppRezFile,gTextureRezfile;
extern	TQ3Point3D	gMyCoord;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	Boolean		gEnteringName;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SongCompletionProc(SndChannelPtr chan);
static short FindSilentChannel(void);
static void Calc3DEffectVolume(short effectNum, TQ3Point3D *where, float volAdjust, u_long *leftVolOut, u_long *rightVolOut);
#if 0
static pascal void CallBackFn (SndChannelPtr chan, SndCommand *cmd) ;
#endif


/****************************/
/*    CONSTANTS             */
/****************************/

#define FloatToFixed16(a)      ((Fixed)((float)(a) * 0x000100L))		// convert float to 16bit fixed pt

#define		MAX_CHANNELS			14

#define		MAX_SOUND_BANKS			5
#define		MAX_EFFECTS				30


typedef struct
{
	Byte	bank,sound;
	long	refDistance;
}EffectType;



#define	STREAM_BUFFER_SIZE	100000

#define	VOLUME_DISTANCE_FACTOR	.004f		// bigger == sound decays FASTER with dist, smaller = louder far away

/**********************/
/*     VARIABLES      */
/**********************/

float						gGlobalVolume = .4;
		
TQ3Point3D					gEarCoords;				// coord of camera plus a bit to get pt in front of camera
static	TQ3Vector3D			gEyeVector;


static	SndListHandle		gSndHandles[MAX_SOUND_BANKS][MAX_EFFECTS];		// handles to ALL sounds
static  long				gSndOffsets[MAX_SOUND_BANKS][MAX_EFFECTS];

static	SndChannelPtr		gSndChannel[MAX_CHANNELS];
static	ChannelInfoType		gChannelInfo[MAX_CHANNELS];

static short				gMaxChannels = 0;


static short				gNumSndsInBank[MAX_SOUND_BANKS] = {0,0,0,0,0};


Boolean						gSongPlayingFlag = false;
Boolean						gResetSong = false;
Boolean						gLoopSongFlag = true;

long						gOriginalSystemVolume,gCurrentSystemVolume;

static	SndChannelPtr		gMusicChannel=nil;
static short				gMusicFileRefNum = 0x0ded;
Boolean						gMuteMusicFlag = false;
static Ptr					gMusicBuffer = nil;					// buffers to use for streaming play
static short				gCurrentSong = -1;

int							gNumLoopingEffects;


		/*****************/
		/* EFFECTS TABLE */
		/*****************/
		
static EffectType	gEffectsTable[] =
{
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_SELECT,800,			// EFFECT_SELECT
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_JUMP,1400,				// EFFECT_JUMP
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_THROWSPEAR,1300,		// EFFECT_THROWSPEAR
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_HITDIRT,900,			// EFFECT_HITDIRT
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_POP,800,				// EFFECT_POP
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_GETPOW,500,			// EFFECT_GETPOW
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_BUZZ,800,				// EFFECT_BUZZ
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_OUCH,900,				// EFFECT_OUCH
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_KICK,700,				// EFFECT_KICK
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_POUND,900,				// EFFECT_POUND
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_SPEEDBOOST,800,		// EFFECT_SPEEDBOOST
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_MORPH,600,				// EFFECT_MORPH
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_FIRECRACKER,2500,		// EFFECT_FIRECRACKER
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_SHIELD,2000,			// EFFECT_SHIELD
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_SPLASH,900,			// EFFECT_SPLASH
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_BUDDYLAUNCH,300,		// EFFECT_BUDDYLAUNCH
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_RESCUE,500,			// EFFECT_RESCUE
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_CHECKPOINT,1000,		// EFFECT_CHECKPOINT
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_KABLAM,2000,			// EFFECT_KABLAM
	
	SOUND_BANK_LEVELSPECIFIC,SOUND_WATER_BOATENGINE,2400,	// EFFECT_BOATENGINE	
	SOUND_BANK_LEVELSPECIFIC,SOUND_WATER_WATERBUG,400,		// EFFECT_WATERBUG	

	SOUND_BANK_LEVELSPECIFIC,SOUND_FOREST_FOOTSTEP,4000,	// EFFECT_FOOTSTEP
	SOUND_BANK_LEVELSPECIFIC,SOUND_FOREST_HELICOPTER,800,	// EFFECT_HELICOPTER
	SOUND_BANK_LEVELSPECIFIC,SOUND_FOREST_PLASMABURST,3500,		// EFFECT_PLASMABURST
	SOUND_BANK_LEVELSPECIFIC,SOUND_FOREST_PLASMAEXPLODE,4500,	// EFFECT_PLASMAEXPLODE
	SOUND_BANK_LEVELSPECIFIC,SOUND_FOREST_FIRECRACKLE,8000,	// EFFECT_FIRECRACKLE
	
	SOUND_BANK_LEVELSPECIFIC,SOUND_WATER_SLURP,500,			// EFFECT_SLURP	
	SOUND_BANK_LEVELSPECIFIC,SOUND_NIGHT_ROCKSLAM,1000,		// EFFECT_ROCKSLAM
	SOUND_BANK_LEVELSPECIFIC,SOUND_ANT_VALVEOPEN,1000,		// EFFECT_VALVEOPEN
	SOUND_BANK_LEVELSPECIFIC,SOUND_ANT_WATERLEAK,1000,		// EFFECT_WATERLEAK
	SOUND_BANK_LEVELSPECIFIC,SOUND_ANT_SHOOT,7000,			// EFFECT_KINGSHOOT
	SOUND_BANK_LEVELSPECIFIC,SOUND_ANT_EXPLODE,8000,		// EFFECT_KINGEXPLODE
	SOUND_BANK_LEVELSPECIFIC,SOUND_ANT_KINGCRACKLE,2000,	// EFFECT_KINGCRACKLE
	SOUND_BANK_LEVELSPECIFIC,SOUND_ANT_SIZZLE,2000,			// EFFECT_SIZZLE
	SOUND_BANK_LEVELSPECIFIC,SOUND_ANT_LAUGH,2000,			// EFFECT_KINGLAUGH
	SOUND_BANK_LEVELSPECIFIC,SOUND_ANT_CLANG,2000,			// EFFECT_PIPECLANG

	SOUND_BANK_LEVELSPECIFIC,SOUND_LAWN_OPENDOOR,1500,		// EFFECT_OPENLAWNDOOR
	SOUND_BANK_LEVELSPECIFIC,SOUND_NIGHT_OPENDOOR,1500,		// EFFECT_OPENNIGHTDOOR

	SOUND_BANK_LEVELSPECIFIC,SOUND_HIVE_STINGERSHOOT,1300,	// EFFECT_STINGERSHOOT
	SOUND_BANK_LEVELSPECIFIC,SOUND_HIVE_PUMP,600,			// EFFECT_PUMP
	SOUND_BANK_LEVELSPECIFIC,SOUND_HIVE_PLUNGER,600,		// EFFECT_PLUNGER
	

	SOUND_BANK_BONUS,SOUND_BONUS_BELL,1000,					// EFFECT_BONUSBELL
	SOUND_BANK_BONUS,SOUND_BONUS_CLICK,1000,				// EFFECT_BONUSCLICK
	
};


/********************* INIT SOUND TOOLS ********************/

void InitSoundTools(void)
{
OSErr		iErr;
short		i;

			/* SET SYSTEM VOLUME INFO */
			
	GetDefaultOutputVolume(&gOriginalSystemVolume);		
	gOriginalSystemVolume &= 0xffff;	
	gCurrentSystemVolume = gOriginalSystemVolume;


	gMaxChannels = 0;

			/* INIT BANK INFO */
			
	for (i = 0; i < MAX_SOUND_BANKS; i++)
		gNumSndsInBank[i] = 0;

			/******************/
			/* ALLOC CHANNELS */
			/******************/

			/* MAKE MUSIC CHANNEL */
			
	SndNewChannel(&gMusicChannel,sampledSynth,initStereo,nil);


			/* ALL OTHER CHANNELS */
				
	for (gMaxChannels = 0; gMaxChannels < MAX_CHANNELS; gMaxChannels++)
	{
			/* NEW SOUND CHANNEL */
			
		SndCommand 		mySndCmd;
			
		iErr = SndNewChannel(&gSndChannel[gMaxChannels],sampledSynth,initMono+initNoInterp,/*NewSndCallBackUPP(CallBackFn)*/nil);
		if (iErr)												// if err, stop allocating channels
			break;

		gChannelInfo[gMaxChannels].isLooping = false;
					
#if 0  // Source port removal
			/* FOR POST- SM 3.6.5 DO THIS! */
	
		mySndCmd.cmd = soundCmd;	
		mySndCmd.param1 = 0;
		mySndCmd.param2 = (long)&sndHdr;
		if ((iErr = SndDoImmediate(gSndChannel[gMaxChannels], &mySndCmd)) != noErr)
		{
			DoAlert("InitSoundTools: SndDoImmediate failed!");
			ShowSystemErr_NonFatal(iErr);
		}
		
		
		mySndCmd.cmd = reInitCmd;	
		mySndCmd.param1 = 0;
		mySndCmd.param2 = initNoInterp|initStereo;
		if ((iErr = SndDoImmediate(gSndChannel[gMaxChannels], &mySndCmd)) != noErr)
		{
			DoAlert("InitSoundTools: SndDoImmediate failed 2!");
			ShowSystemErr_NonFatal(iErr);
		}
#endif
			
	}
	

		/* INIT MUSIC STREAMING BUFFER */
		
	if (gMusicBuffer == nil)
	{
		gMusicBuffer = AllocPtr(STREAM_BUFFER_SIZE);
		if (gMusicBuffer == nil)
			DoFatalAlert("InitSoundTools: gMusicBuffer == nil");
	}
}


/***************************** CALLBACKFN ***************************/
//  
// Called by the Sound Manager at interrupt time to let us know that
// the sound is done playing.
//

#if 0
static pascal void CallBackFn (SndChannelPtr chan, SndCommand *cmd) 
{
SndCommand      theCmd;

    // Play the sound again (loop it)

    theCmd.cmd = bufferCmd;
    theCmd.param1 = 0;
    theCmd.param2 = cmd->param2;
	SndDoCommand (chan, &theCmd, true);

    // Just reuse the callBackCmd that got us here in the first place
    SndDoCommand (chan, cmd, true);
}
#endif




/******************* LOAD SOUND BANK ************************/

void LoadSoundBank(FSSpec *spec, long bankNum)
{
short			srcFile1,numSoundsInBank,i;
OSErr			iErr;

	StopAllEffectChannels();

	if (bankNum >= MAX_SOUND_BANKS)
		DoFatalAlert("LoadSoundBank: bankNum >= MAX_SOUND_BANKS");

			/* DISPOSE OF EXISTING BANK */
			
	DisposeSoundBank(bankNum);


			/* OPEN APPROPRIATE REZ FILE */
			
	srcFile1 = FSpOpenResFile(spec, fsRdPerm);
	if (srcFile1 == -1)
		DoFatalAlert("LoadSoundBank: OpenResFile failed!");

			/****************************/
			/* LOAD ALL EFFECTS IN BANK */
			/****************************/

	UseResFile( srcFile1 );												// open sound resource fork
	numSoundsInBank = Count1Resources('snd ');							// count # snd's in this bank
	if (numSoundsInBank > MAX_EFFECTS)
		DoFatalAlert("LoadSoundBank: numSoundsInBank > MAX_EFFECTS");

	for (i=0; i < numSoundsInBank; i++)
	{
				/* LOAD SND REZ */
				
		gSndHandles[bankNum][i] = (SndListResource **)GetResource('snd ',BASE_EFFECT_RESOURCE+i);
		if (gSndHandles[bankNum][i] == nil) 
		{
			iErr = ResError();
			DoAlert("LoadSoundBank: GetResource failed!");
			if (iErr == memFullErr)
				DoFatalAlert("LoadSoundBank: Out of Memory");
			else
				ShowSystemErr(iErr);
		}
		DetachResource((Handle)gSndHandles[bankNum][i]);				// detach resource from rez file & make a normal Handle
			
		HNoPurge((Handle)gSndHandles[bankNum][i]);						// make non-purgeable
		HLockHi((Handle)gSndHandles[bankNum][i]);
		
				/* GET OFFSET INTO IT */
				
		GetSoundHeaderOffset(gSndHandles[bankNum][i], &gSndOffsets[bankNum][i]);		

				/* PRE-DECOMPRESS IT (Source port addition) */

		Pomme_DecompressSoundResource(&gSndHandles[bankNum][i], &gSndOffsets[bankNum][i]);
	}

	UseResFile(gMainAppRezFile );								// go back to normal res file
	UseResFile(gTextureRezfile);
	CloseResFile(srcFile1);

	gNumSndsInBank[bankNum] = numSoundsInBank;					// remember how many sounds we've got
}


/******************** DISPOSE SOUND BANK **************************/

void DisposeSoundBank(short bankNum)
{
short	i; 

	if (bankNum > MAX_SOUND_BANKS)
		return;

	StopAllEffectChannels();									// make sure all sounds are stopped before nuking any banks

			/* FREE ALL SAMPLES */
			
	for (i=0; i < gNumSndsInBank[bankNum]; i++)
		DisposeHandle((Handle)gSndHandles[bankNum][i]);


	gNumSndsInBank[bankNum] = 0;
}


/******************* DISPOSE ALL SOUND BANKS *****************/

void DisposeAllSoundBanks(void)
{
short	i;

	for (i = 0; i < MAX_SOUND_BANKS; i++)
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
	if (theStatus.scChannelBusy)					// if channel busy, then stop it
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
	
	if (gChannelInfo[c].isLooping)
	{
		gNumLoopingEffects--;
		gChannelInfo[c].isLooping = false;
	}
	
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


/****************** WAIT EFFECTS SILENT *********************/

void WaitEffectsSilent(void)
{
short	i;
Boolean	isBusy;
SCStatus				theStatus;

	do
	{
		isBusy = 0;
		for (i=0; i < gMaxChannels; i++)
		{
			SndChannelStatus(gSndChannel[i],sizeof(SCStatus),&theStatus);	// get channel info
			isBusy |= theStatus.scChannelBusy;
		}
	}while(isBusy);
}


/******************** PLAY SONG ***********************/
//
// if songNum == -1, then play existing open song
//
// INPUT: loopFlag = true if want song to loop
//

void PlaySong(short songNum, Boolean loopFlag)
{
Str255	errStr = "PlaySong: Couldnt Open Music AIFF File.";
static	SndCommand 		mySndCmd;
int		volume;
OSErr	iErr;

	if (songNum == gCurrentSong)					// see if this is already playing
		return;

		/* SEE IF JUST RESTART CURRENT STREAM */
		
	if (songNum == -1)	
		goto stream_again;

		/* ZAP ANY EXISTING SONG */
		
	gCurrentSong 	= songNum;
	gResetSong 		= false;
	gLoopSongFlag 	= loopFlag;
	KillSong();
	DoSoundMaintenance();

			/******************************/
			/* OPEN APPROPRIATE AIFF FILE */
			/******************************/
			
	switch(songNum)
	{
		case	SONG_MENU:
				OpenGameFile(":audio:MenuSong.aiff",&gMusicFileRefNum,errStr);
				volume = FULL_CHANNEL_VOLUME;
				break;
	
		case	SONG_GARDEN:
				OpenGameFile(":audio:LawnSong.aiff",&gMusicFileRefNum,errStr);
				volume = FULL_CHANNEL_VOLUME;
				break;

		case	SONG_PANGEA:
				OpenGameFile(":audio:song_pangea",&gMusicFileRefNum,errStr);
				volume = FULL_CHANNEL_VOLUME;
				break;
			
		case	SONG_HIGHSCORES:
				OpenGameFile(":audio:HighScores.aiff",&gMusicFileRefNum,errStr);
				volume = FULL_CHANNEL_VOLUME;
				break;

		case	SONG_NIGHT:
				OpenGameFile(":audio:Night.aiff",&gMusicFileRefNum,errStr);
				volume = FULL_CHANNEL_VOLUME;
				break;

		case	SONG_FOREST:
				OpenGameFile(":audio:Forest.aiff",&gMusicFileRefNum,errStr);
				volume = FULL_CHANNEL_VOLUME;
				break;

		case	SONG_POND:
				OpenGameFile(":audio:PondSong.aiff",&gMusicFileRefNum,errStr);
				volume = FULL_CHANNEL_VOLUME;
				break;

		case	SONG_ANTHILL:
				OpenGameFile(":audio:AntHillSong.aiff",&gMusicFileRefNum,errStr);
				volume = FULL_CHANNEL_VOLUME;
				break;
				
		case	SONG_HIVE:
				OpenGameFile(":audio:HiveLevel.aiff",&gMusicFileRefNum,errStr);
				volume = FULL_CHANNEL_VOLUME;
				break;
				
		case	SONG_WIN:
				OpenGameFile(":audio:WinSong.aiff",&gMusicFileRefNum,errStr);
				volume = FULL_CHANNEL_VOLUME;
				break;

		case	SONG_LOSE:
				OpenGameFile(":audio:LoseSong.aiff",&gMusicFileRefNum,errStr);
				volume = FULL_CHANNEL_VOLUME;
				break;

		case	SONG_BONUS:
				OpenGameFile(":audio:BonusSong.aiff",&gMusicFileRefNum,errStr);
				volume = FULL_CHANNEL_VOLUME;
				break;

		default:
				DoFatalAlert("PlaySong: unknown song #");
	}

	gCurrentSong = songNum;
	
	
				/*******************/
				/* START STREAMING */
				/*******************/
		
		/* RESET CHANNEL TO DEFAULT VALUES */

	mySndCmd.cmd = volumeCmd;										// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (volume<<16) | volume;
	SndDoImmediate(gMusicChannel, &mySndCmd);
					
	mySndCmd.cmd 		= rateMultiplierCmd;						// modify the rate to change the frequency 
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= NORMAL_CHANNEL_RATE;	
	SndDoImmediate(gMusicChannel, &mySndCmd);
					
			
			
			/* START PLAYING FROM FILE */

stream_again:					
	iErr = SndStartFilePlay(gMusicChannel, gMusicFileRefNum, 0, STREAM_BUFFER_SIZE, gMusicBuffer,
							nil, SongCompletionProc, true);

	if (iErr)
	{
		FSClose(gMusicFileRefNum);								// close the file
		gMusicFileRefNum = 0x0ded;
		DoAlert("PlaySong: SndStartFilePlay failed!");
		ShowSystemErr(iErr);
	}
	gSongPlayingFlag = true;


			/* SET LOOP FLAG ON STREAM (SOURCE PORT ADDITION) */
			/* So we don't need to re-read the file over and over. */

	mySndCmd.cmd = pommeSetLoopCmd;
	mySndCmd.param1 = loopFlag ? 1 : 0;
	mySndCmd.param2 = 0;
	iErr = SndDoImmediate(gMusicChannel, &mySndCmd);
	if (iErr)
		DoFatalAlert("PlaySong: SndDoImmediate (pomme loop extension) failed!");


			/* SEE IF WANT TO MUTE THE MUSIC */
			
//	if (gMuteMusicFlag)													
//		SndPauseFilePlay(gMusicChannel);						// pause it	
}

/***************** SONG COMPLETION PROC *********************/

static void SongCompletionProc(SndChannelPtr chan)
{
	if (gSongPlayingFlag)
		gResetSong = true;
}


/*********************** KILL SONG *********************/

void KillSong(void)
{
OSErr	iErr;

	gCurrentSong = -1;

	if (!gSongPlayingFlag)
		return;
		
	gSongPlayingFlag = false;											// tell callback to do nothing

//	SndStopFilePlay(gMusicChannel, true);								// stop it
	
	if (gMusicFileRefNum == 0x0ded)
		DoAlert("KillSong: gMusicFileRefNum == 0x0ded");
	else
	{
		iErr = FSClose(gMusicFileRefNum);								// close the file
		if (iErr)
		{
			DoAlert("KillSong: FSClose failed!");
			ShowSystemErr_NonFatal(iErr);
		}
	}
		
	gMusicFileRefNum = 0x0ded;
}

#pragma mark -


/***************************** PLAY EFFECT 3D ***************************/
//
// NO SSP
//
// OUTPUT: channel # used to play sound
//

short PlayEffect3D(short effectNum, TQ3Point3D *where)
{
short					theChan;
Byte					bankNum,soundNum;
u_long					leftVol, rightVol;

			/* GET BANK & SOUND #'S FROM TABLE */
			
	bankNum 	= gEffectsTable[effectNum].bank;
	soundNum 	= gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoAlert("Illegal sound number!");
		ShowSystemErr(effectNum);	
	}

				/* CALC VOLUME */

	Calc3DEffectVolume(effectNum, where, 1.0, &leftVol, &rightVol);
	if ((leftVol+rightVol) == 0)
		return(-1);


	theChan = PlayEffect_Parms(effectNum, leftVol, rightVol, NORMAL_CHANNEL_RATE);

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

short PlayEffect_Parms3D(short effectNum, TQ3Point3D *where, u_long rateMultiplier, float volumeAdjust)
{
short			theChan;
Byte			bankNum,soundNum;
u_long			leftVol, rightVol;

			/* GET BANK & SOUND #'S FROM TABLE */
			
	bankNum 	= gEffectsTable[effectNum].bank;
	soundNum 	= gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoAlert("Illegal sound number!");
		ShowSystemErr(effectNum);	
	}

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

Boolean Update3DSoundChannel(short effectNum, short *channel, TQ3Point3D *where)
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
			
	if (!gChannelInfo[c].isLooping)										// loopers wont complete, duh.
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

short PlayEffect(short effectNum)
{
	return(PlayEffect_Parms(effectNum,FULL_CHANNEL_VOLUME,FULL_CHANNEL_VOLUME,NORMAL_CHANNEL_RATE));

}


/***************************** PLAY EFFECT PARMS ***************************/
//
// Plays an effect with parameters
//
// OUTPUT: channel # used to play sound
//

short  PlayEffect_Parms(short effectNum, u_long leftVolume, u_long rightVolume, unsigned long rateMultiplier)
{
SndCommand 		mySndCmd;
SndChannelPtr	chanPtr;
short			theChan;
Byte			bankNum,soundNum;
OSErr			myErr;
u_long			lv2,rv2;
static UInt32          loopStart, loopEnd;
#if 1
SOURCE_PORT_MINOR_PLACEHOLDER();
#else
SoundHeaderPtr   sndPtr;
#endif


	
			/* GET BANK & SOUND #'S FROM TABLE */
			
	bankNum = gEffectsTable[effectNum].bank;
	soundNum = gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoAlert("Illegal sound number!");
		ShowSystemErr(effectNum);	
	}

			/* LOOK FOR FREE CHANNEL */
			
	theChan = FindSilentChannel();
	if (theChan == -1)
	{
		return(-1);
	}

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

	mySndCmd.cmd = volumeCmd;										// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (rv2<<16) | lv2;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);


	mySndCmd.cmd = bufferCmd;										// make it play
	mySndCmd.param1 = 0;
	mySndCmd.param2 = ((long)*gSndHandles[bankNum][soundNum])+gSndOffsets[bankNum][soundNum];	// pointer to SoundHeader
	SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd 		= rateMultiplierCmd;						// modify the rate to change the frequency 
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= rateMultiplier;	
	SndDoImmediate(chanPtr, &mySndCmd);

    
    
    		/* SEE IF THIS IS A LOOPING EFFECT */    		
    
#if 1
	SOURCE_PORT_MINOR_PLACEHOLDER();
#else
	
    sndPtr = (SoundHeaderPtr)(((long)*gSndHandles[bankNum][soundNum])+gSndOffsets[bankNum][soundNum]);
    loopStart = sndPtr->loopStart;
    loopEnd = sndPtr->loopEnd;
    if ((loopStart + 1) < loopEnd)
    {
    	mySndCmd.cmd = callBackCmd;										// let us know when the buffer is done playing
    	mySndCmd.param1 = 0;
    	mySndCmd.param2 = ((long)*gSndHandles[bankNum][soundNum])+gSndOffsets[bankNum][soundNum];	// pointer to SoundHeader
    	SndDoCommand(chanPtr, &mySndCmd, true);
    	
    	gChannelInfo[theChan].isLooping = true;
    	gNumLoopingEffects++;
	}
	else
#endif
		gChannelInfo[theChan].isLooping = false;
		

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
//	SndPauseFilePlay(gMusicChannel);			// pause it
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

		if (GetNewKeyState(KEY_M))
			ToggleMusic();			
			
		
				/* SEE IF CHANGE VOLUME */

		if (GetNewKeyState(KEY_PERIOD))
		{
			if (gCurrentSystemVolume < 0x100)
			{
				if (gCurrentSystemVolume < 10)					// finer control @ low volumes
					gCurrentSystemVolume += 1;
				else
					gCurrentSystemVolume += 15;
			}
			if (gCurrentSystemVolume > 0x100)
				gCurrentSystemVolume = 0x100;
			SetDefaultOutputVolume((gCurrentSystemVolume<<16)|gCurrentSystemVolume); // set system volume
		}
		else
		if (GetNewKeyState(KEY_COMMA))
		{
			if (gCurrentSystemVolume > 0x000)
			{
				if (gCurrentSystemVolume < 10)					// finer control @ low volumes
					gCurrentSystemVolume -= 1;
				else
					gCurrentSystemVolume -= 15;
			}
			if (gCurrentSystemVolume < 0x000)
				gCurrentSystemVolume = 0;
			
			SetDefaultOutputVolume((gCurrentSystemVolume<<16)|gCurrentSystemVolume); // set system volume
		}
	}
				/* SEE IF STREAMED MUSIC STOPPED - SO RESET */
				
	if (gResetSong)
	{
		gResetSong = false;
		if (gLoopSongFlag)							// see if stop song now or loop it
			PlaySong(-1,true);
		else
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

	dist 	= Q3Point3D_Distance(where, &gEarCoords);		// calc dist to sound for pane 0
		
			/* DO VOLUME CALCS */
			
	refDist = gEffectsTable[effectNum].refDistance;			// get ref dist

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






