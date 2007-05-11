//
// selfrundemo.h
//

#define	END_DEMO_MARK		0x7fff
#define	MAX_DEMO_SIZE		50000L

#define	DEMO_FPS			25

enum
{
	DEMO_MODE_OFF,
	DEMO_MODE_PLAYBACK,
	DEMO_MODE_RECORD

};

typedef struct
{
	long		count;
	KeyMap		keyMap;
}DemoCacheKeyType;


//========================================================

extern	void StartRecordingDemo(void);
extern	void SaveDemoData(void);
extern	void InitDemoPlayback(void);
extern	void StopDemo(void);


