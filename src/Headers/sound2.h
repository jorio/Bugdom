//
// Sound2.h
//

#pragma once

typedef struct
{
	short	effectNum;
	float	volumeAdjust;
	float	leftVolume, rightVolume;
}ChannelInfoType;

#define		FULL_CHANNEL_VOLUME		kFullVolume


enum
{
	SONG_MENU,
	SONG_GARDEN,
	SONG_GARDEN_OLD,
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
	SOUNDBANK_MAIN,
	SOUNDBANK_LAWN,
	SOUNDBANK_POND,
	SOUNDBANK_HIVE,
	SOUNDBANK_NIGHT,
	SOUNDBANK_FOREST,
	SOUNDBANK_ANTHILL,
	SOUNDBANK_BONUS,
	NUM_SOUNDBANKS
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
	EFFECT_BONUSCLICK,

	NUM_EFFECTS
};


//===================== PROTOTYPES ===================================


extern void	InitSoundTools(void);
void StopAChannel(short *channelNum);
extern	void StopAllEffectChannels(void);
void PlaySong(short songNum, Boolean loopFlag);
extern void	KillSong(void);
short PlayEffect(int effectNum);
short PlayEffect_Parms3D(int effectNum, TQ3Point3D *where, u_long freq, float volumeAdjust);
extern void	ToggleMusic(void);
extern void	DoSoundMaintenance(void);
void LoadSoundEffect(int effectNum);
void DisposeSoundEffect(int effectNum);
void LoadSoundBank(int bankNum);
void DisposeSoundBank(int bankNum);
void DisposeAllSoundBanks(void);
void PauseAllChannels(Boolean pause);
short PlayEffect_Parms(int effectNum, u_long leftVolume, u_long rightVolume, unsigned long rateMultiplier);
void ChangeChannelVolume(short channel, float leftVol, float rightVol);
short PlayEffect3D(int effectNum, TQ3Point3D *where);
Boolean Update3DSoundChannel(int effectNum, short *channel, TQ3Point3D *where);
Boolean IsEffectChannelPlaying(short chanNum);
void UpdateListenerLocation(void);

