//
// Sound2.h
//

typedef struct
{
	short	effectNum;
	float	volumeAdjust;
	float	leftVolume, rightVolume;
	Boolean	isLooping;
}ChannelInfoType;

#define		USE_SOUNDSPROCKET		0


#define		BASE_EFFECT_RESOURCE	10000

#define		FULL_CHANNEL_VOLUME		kFullVolume
#define		NORMAL_CHANNEL_RATE		0x10000


enum
{
	SONG_MENU,
	SONG_GARDEN,
	SONG_PANGEA,
	SONG_HIGHSCORES,
	SONG_NIGHT,
	SONG_FOREST,
	SONG_POND,
	SONG_ANTHILL,
	SONG_HIVE,
	SONG_WIN,
	SONG_LOSE,
	SONG_BONUS
};

enum
{
	SOUND_BANK_DEFAULT 			= 0,
	SOUND_BANK_MENU				= 1,
	SOUND_BANK_LEVELSPECIFIC	= 2,
	SOUND_BANK_BONUS			= 2
};



/***************** EFFECTS *************************/
// This table must match gEffectsTable
//

enum
{
	EFFECT_SELECT,
	EFFECT_JUMP,
	EFFECT_THROWSPEAR,
	EFFECT_HITDIRT,
	EFFECT_POP,
	EFFECT_GETPOW,
	EFFECT_BUZZ,
	EFFECT_OUCH,
	EFFECT_KICK,
	EFFECT_POUND,
	EFFECT_SPEEDBOOST,
	EFFECT_MORPH,
	EFFECT_FIRECRACKER,
	EFFECT_SHIELD,
	EFFECT_SPLASH,
	EFFECT_BUDDYLAUNCH,
	EFFECT_RESCUE,
	EFFECT_CHECKPOINT,
	EFFECT_KABLAM,

	EFFECT_BOATENGINE,
	EFFECT_WATERBUG,
	
	EFFECT_FOOTSTEP,
	EFFECT_HELICOPTER,
	EFFECT_PLASMABURST,
	EFFECT_PLASMAEXPLODE,
	EFFECT_FIRECRACKLE,
	EFFECT_SLURP,
	EFFECT_ROCKSLAM,
	EFFECT_VALVEOPEN,
	EFFECT_WATERLEAK,
	EFFECT_KINGSHOOT,
	EFFECT_KINGEXPLODE,
	EFFECT_KINGCRACKLE,
	EFFECT_SIZZLE,
	EFFECT_KINGLAUGH,
	EFFECT_PIPECLANG,
	
	EFFECT_OPENLAWNDOOR,
	EFFECT_OPENNIGHTDOOR,
	EFFECT_STINGERSHOOT,
	EFFECT_PUMP,
	EFFECT_PLUNGER,
	
	EFFECT_BONUSBELL,
	EFFECT_BONUSCLICK
};



/**********************/
/* SOUND BANK TABLES  */
/**********************/
//
// These are simple enums for equating the sound effect #'s in each sound
// bank's rez file.
//

/******** SoundBank_Default *********/

enum
{
	SOUND_DEFAULT_SELECT,
	SOUND_DEFAULT_JUMP,
	SOUND_DEFAULT_THROWSPEAR,
	SOUND_DEFAULT_HITDIRT,
	SOUND_DEFAULT_POP,
	SOUND_DEFAULT_GETPOW,
	SOUND_DEFAULT_BUZZ,
	SOUND_DEFAULT_OUCH,
	SOUND_DEFAULT_KICK,
	SOUND_DEFAULT_POUND,
	SOUND_DEFAULT_SPEEDBOOST,
	SOUND_DEFAULT_MORPH,
	SOUND_DEFAULT_FIRECRACKER,
	SOUND_DEFAULT_SHIELD,
	SOUND_DEFAULT_SPLASH,
	SOUND_DEFAULT_BUDDYLAUNCH,
	SOUND_DEFAULT_RESCUE,
	SOUND_DEFAULT_CHECKPOINT,
	SOUND_DEFAULT_KABLAM
};

/******** SoundBank_Lawn *********/

enum
{
	SOUND_LAWN_OPENDOOR
};

/******** SoundBank_Water *********/

enum
{
	SOUND_WATER_BOATENGINE,
	SOUND_WATER_SLURP,
	SOUND_WATER_WATERBUG
};


/******** SoundBank_Hive *********/

enum
{
	SOUND_HIVE_STINGERSHOOT,
	SOUND_HIVE_PUMP,
	SOUND_HIVE_PLUNGER
};

/******** SoundBank_Night *********/

enum
{
	SOUND_NIGHT_ROCKSLAM,
	SOUND_NIGHT_OPENDOOR
};


/******** SoundBank_Forest *********/

enum
{
	SOUND_FOREST_FOOTSTEP,
	SOUND_FOREST_HELICOPTER,
	SOUND_FOREST_PLASMABURST,
	SOUND_FOREST_PLASMAEXPLODE,
	SOUND_FOREST_FIRECRACKLE
};


/******** SoundBank_AntHill *********/

enum
{
	SOUND_ANT_VALVEOPEN,
	SOUND_ANT_WATERLEAK,
	SOUND_ANT_SHOOT,
	SOUND_ANT_EXPLODE,
	SOUND_ANT_KINGCRACKLE,
	SOUND_ANT_SIZZLE,
	SOUND_ANT_LAUGH,
	SOUND_ANT_CLANG
};


/******** SoundBank_Bonus *********/

enum
{
	SOUND_BONUS_BELL,
	SOUND_BONUS_CLICK
};

//===================== PROTOTYPES ===================================


extern void	InitSoundTools(void);
void StopAChannel(short *channelNum);
extern	void StopAllEffectChannels(void);
void PlaySong(short songNum, Boolean loopFlag);
extern void	KillSong(void);
extern	short PlayEffect(short effectNum);
short PlayEffect_Parms3D(short effectNum, TQ3Point3D *where, u_long freq, float volumeAdjust);
extern void	ToggleMusic(void);
extern void	DoSoundMaintenance(void);
extern	void LoadSoundBank(FSSpec *spec, long bankNum);
extern	void WaitEffectsSilent(void);
extern	void DisposeSoundBank(short bankNum);
extern	void DisposeAllSoundBanks(void);
short  PlayEffect_Parms(short effectNum, u_long leftVolume, u_long rightVolume, unsigned long rateMultiplier);
void ChangeChannelVolume(short channel, float leftVol, float rightVol);
short PlayEffect3D(short effectNum, TQ3Point3D *where);
Boolean Update3DSoundChannel(short effectNum, short *channel, TQ3Point3D *where);
void DoSSPConfig(void);
Boolean IsEffectChannelPlaying(short chanNum);
void UpdateListenerLocation(void);









