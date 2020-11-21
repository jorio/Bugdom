//
// triggers.h
//

#define	TRIGGER_SLOT	4					// needs to be early in the collision list
#define	TriggerSides	SpecialL[5]


		/* TRIGGER TYPES */
enum
{
	TRIGTYPE_NUT,
	TRIGTYPE_WATERBUG,
	TRIGTYPE_DRAGONFLY,
	TRIGTYPE_HONEYCOMBPLATFORM,
	TRIGTYPE_DETONATOR,
	TRIGTYPE_CHECKPOINT,
	TRIGTYPE_TWIGDOOR,
	TRIGTYPE_WEBBULLET,
	TRIGTYPE_EXITLOG,
	TRIGTYPE_POW,
	TRIGTYPE_WATERVALVE,
	TRIGTYPE_KINGPIPE,
	TRIGTYPE_CAGE
};


enum
{
	WATERBUG_MODE_WAITING,
	WATERBUG_MODE_COAST,
	WATERBUG_MODE_RIDE
};

enum
{
	WATERBUG_ANIM_WAIT,
	WATERBUG_ANIM_LEFT,
	WATERBUG_ANIM_RIGHT,
	WATERBUG_ANIM_CENTER,
	WATERBUG_ANIM_OUTOFSERVICE
};



#define	DRAGONFLY_JOINT_TAIL	3
#define MAX_DRAGONFLY_FLIGHT_HEIGHT		5000.0f
#define MAX_DRAGONFLY_FLIGHT_HEIGHT2	4000.0f

#define	HONEYCOMB_PLATFORM_SCALE	2.0f
#define	HONEYCOMB_PLATFORM_SCALE2	1.3f

#define	SHIELD_TIME			10							// n seconds of shield per powerup


//===============================================================================

extern	Boolean HandleTrigger(ObjNode *triggerNode, ObjNode *whoNode, Byte side);
extern	Boolean AddNut(TerrainItemEntryType *itemPtr, long  x, long z);
void KickNut(ObjNode *kickerObj, ObjNode *nutObj);

Boolean AddWaterBug(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_WaterBug(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
void DriveWaterBug(ObjNode *bug, ObjNode *player);

Boolean AddDragonFly(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_DragonFly(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
void DriveDragonFly(ObjNode *bug, ObjNode *player);
void PlayerOffDragonfly(void);


Boolean AddHoneycombPlatform(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddDetonator(TerrainItemEntryType *itemPtr, long  x, long z);
void InitDetonators(void);

Boolean AddCheckpoint(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_Checkpoint(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

Boolean AddLawnDoor(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean DoTrig_WebBullet(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

Boolean AddExitLog(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_ExitLog(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

Boolean AddWaterValve(TerrainItemEntryType *itemPtr, long  x, long z);
void InitWaterValves(void);

Boolean AddKingWaterPipe(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_KingWaterPipe(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
void KickKingWaterPipe(ObjNode *theNode);

Boolean AddLadyBugBonus(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean KickLadyBugBox(ObjNode *cage);
Boolean DoTrig_Cage(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);







