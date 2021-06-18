#pragma once

#include "Pomme.h"
#include "QD3D.h"
#include "QD3DMath.h"
#include <SDL.h>

// bogus types to ease noquesa transition
// TODO NOQUESA: Nuke that when renderer rewrite is complete
typedef void* TQ3StyleObject;
typedef void* TQ3GroupObject;
typedef void* TQ3CameraObject;
typedef void* TQ3DrawContextObject;
typedef void* TQ3AttributeSet;
typedef void* TQ3Mipmap;
typedef void* TQ3PickObject;
typedef void* TQ3RendererObject;
typedef void* TQ3DisplayGroupObject;

#ifdef __cplusplus
extern "C"
{
#endif

#include "globals.h"
#include "renderer.h"
#include "structs.h"
#include "mobjtypes.h"
#include "objects.h"
#include "window.h"
#include "main.h"
#include "misc.h"
#include "bones.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include "skeletonjoints.h"
#include "camera.h"
#include "player_control.h"
#include "sound2.h"
#include "3dmf.h"
#include "file.h"
#include "input.h"
#include "terrain.h"
#include "myguy.h"
#include "enemy.h"
#include "3dmath.h"
#include "items.h"
#include "highscores.h"
#include "qd3d_geometry.h"
#include "environmentmap.h"
#include "infobar.h"
#include "title.h"
#include "effects.h"
#include "miscscreens.h"
#include "fences.h"
#include "mainmenu.h"
#include "bonusscreen.h"
#include "liquids.h"
#include "collision.h"
#include "mytraps.h"
#include "triggers.h"
#include "pick.h"
#include "fileselect.h"
#include "textmesh.h"
#include "tga.h"
#include "tween.h"
#include "mousesmoothing.h"
#include "frustumculling.h"

extern	Boolean						gAbortedFlag;
extern	Boolean						gAreaCompleted;
extern	Boolean						gBatExists;
extern	Boolean						gDetonatorBlown[];
extern	Boolean						gDisableAnimSounds;
extern	Boolean						gDisableHiccupTimer;
extern	Boolean						gDoCeiling;
extern	Boolean						gDrawLensFlare;
extern	Boolean						gEnteringName;
extern	Boolean						gGameOverFlag;
extern	Boolean						gLiquidCheat;
extern	Boolean						gMuteMusicFlag;
extern	Boolean						gPlayerCanMove;
extern	Boolean						gPlayerGotKilledFlag;
extern	Boolean						gPlayerKnockOnButt;
extern	Boolean						gPlayerUsingKeyControl;
extern	Boolean						gQD3DInitialized;
extern	Boolean						gResetSong;
extern	Boolean						gRestoringSavedGame;
extern	Boolean						gShowBottomBar;
extern	Boolean						gShowDebug;
extern	Boolean						gSongPlayingFlag;
extern	Boolean						gTorchPlayer;
extern	Boolean						gValveIsOpen[];
extern	Byte						gCurrentLiquidType;
extern	Byte						gMyStartAim;
extern	Byte						gPlayerMode;
extern	Byte						gTotalSides;
extern	CollisionRec				gCollisionList[];
extern	FSSpec						gDataSpec;
extern	FenceDefType				*gFenceList;
extern	NewObjectDefinitionType		gNewObjectDefinition;
extern	ObjNode						*gAntKingObj;
extern	ObjNode						*gCurrentCarryingFireFly;
extern	ObjNode						*gCurrentChasingFireFly;
extern	ObjNode						*gCurrentDragonFly;
extern	ObjNode						*gCurrentEatingBat;
extern	ObjNode						*gCurrentEatingFish;
extern	ObjNode						*gCurrentNode;
extern	ObjNode						*gCurrentRope;
extern	ObjNode						*gCurrentWaterBug;
extern	ObjNode						*gFirstNodePtr;
extern	ObjNode						*gHiveObj;
extern	ObjNode						*gMenuIcons[NUM_MENU_ICONS];
extern	ObjNode						*gMostRecentlyAddedNode;
extern	ObjNode						*gMyBuddy;
extern	ObjNode						*gNextNode;
extern	ObjNode						*gPlayerObj;
extern	ObjNode						*gPrevRope;
extern	ObjNode						*gSaveNo;
extern	ObjNode						*gSaveYes;
extern	ObjNode						*gTheQueen;
extern	PrefsType					gGamePrefs;
extern	QD3DSetupOutputType			*gGameViewInfoPtr;
extern	RenderModifiers				gPauseQuadRenderMods;
extern	RenderStats					gRenderStats;
extern	SDL_GameController*			gSDLController;
extern	SDL_Window					*gSDLWindow;
extern	SplineDefType				**gSplineList;
extern	TQ3BoundingBox				gObjectGroupBBoxList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TQ3BoundingSphere			gObjectGroupRadiusList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TQ3Int32					gHoveredPick;
extern	TQ3Matrix4x4				gCameraWorldToFrustumMatrix;
extern	TQ3Matrix4x4				gCameraWorldToViewMatrix;
extern	TQ3Matrix4x4				gWindowToFrustum;
extern	TQ3Matrix4x4				gWindowToFrustumCorrectAspect;
extern	TQ3Param2D					gEnvMapUVs[];
extern	TQ3Point3D					gCoord;
extern	TQ3Point3D					gMostRecentCheckPointCoord;
extern	TQ3Point3D					gMyCoord;
extern	TQ3TriMeshData				*gPauseQuad;
extern	TQ3TriMeshFlatGroup			gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TQ3Vector2D					gCameraControlDelta;
extern	TQ3Vector3D					gDelta;
extern	TQ3Vector3D					gLightDirection1;
extern	TQ3Vector3D					gPlayerKnockOnButtDelta;
extern	TQ3Vector3D					gRecentTerrainNormal[2];
extern	TerrainInfoMatrixType		**gMapInfoMatrix;
extern	TerrainItemEntryType		**gTerrainItemLookupTableX;
extern	TerrainItemEntryType 		**gMasterItemList;
extern	TerrainYCoordType			**gMapYCoords;
extern	WindowPtr					gCoverWindow;
extern	char						gTypedAsciiKey;
extern	const TQ3Float32			gTextureAlphaThreshold;
extern	const TQ3Point3D			gBatMouthOff;
extern	const TQ3Point3D			gPondFishMouthOff;
extern	const float					gLiquidCollisionTopOffset[NUM_LIQUID_TYPES];
extern	float						gAutoFadeStartDist;
extern	float						gBallTimer;
extern	float						gCheckPointRot;
extern	float						gCycScale;
extern	float						gFramesPerSecond;
extern	float						gFramesPerSecondFrac;
extern	float						gMyDistToFloor;
extern	float						gMyHealth;
extern	float						gPlayerCurrentWaterY;
extern	float						gPlayerMaxSpeed;
extern	float						gPlayerToCameraAngle;
extern	float						gShieldTimer;
extern	float						gTerrainItemDeleteWindow_Far;
extern	float						gTerrainItemDeleteWindow_Left;
extern	float						gTerrainItemDeleteWindow_Near;
extern	float						gTerrainItemDeleteWindow_Right;
extern	int							gMaxItemsAllocatedInAPass;
extern	int							gNitroParticleGroup;
extern	int							gWindowHeight;
extern	int							gWindowWidth;
extern	int 						gCurrentSaveSlot;
extern	int8_t						gNumEnemyOfKind[];
extern	long						gCurrentSuperTileCol;
extern	long						gCurrentSuperTileRow;
extern	long						gCurrentSystemVolume;
extern	long						gEatMouse;
extern	long						gMyStartX;
extern	long						gMyStartZ;
extern	long						gNumFences;
extern	long						gNumSplines;
extern	long						gNumSuperTilesDeep;
extern	long						gNumSuperTilesWide;
extern	long						gNumTerrainTextureTiles;
extern	long						gOriginalSystemVolume;
extern	long						gPrefsFolderDirID;
extern	long						gTerrainTileDepth;
extern	long						gTerrainTileWidth;
extern	long						gTerrainUnitDepth;
extern	long						gTerrainUnitWidth;
extern	short						gBestCheckPoint;
extern	short						gCurrentGasParticleGroup;
extern	short						gCurrentRopeJoint;
extern	short						gMainAppRezFile;
extern	short						gMoney;
extern	short						gNumBlueClovers;
extern	short						gNumCollisions;
extern	short						gNumEnemies;
extern	short						gNumGoldClovers;
extern	short						gNumGreenClovers;
extern	short						gNumLadyBugsThisArea;
extern	short						gNumLives;
extern	short						gNumObjectsInGroupList[MAX_3DMF_GROUPS];
extern	short						gNumTerrainItems;
extern	short						gPrefsFolderVRefNum;
extern	u_long						gAutoFadeStatusBits;
extern	u_long						gCurrentGasParticleMagicNum;
extern	u_long						gInfobarUpdateBits;
extern	u_short						**gCeilingMap;
extern	u_short						**gFloorMap;
extern	u_short						**gTileDataHandle;
extern	u_short						**gVertexColors[2];
extern	u_short						gAreaNum;
extern	u_short						gLevelType;
extern	u_short						gLevelTypeMask;
extern	u_short						gRealLevel;
extern	uint8_t						gTerrainScrollBuffer[MAX_SUPERTILES_DEEP][MAX_SUPERTILES_WIDE];
extern	unsigned long 				gScore;

#ifdef __cplusplus
};
#endif

// C++
#include "pickablequads.h"
#include "gamepatches.h"
#include "structformats.h"

#define GAME_ASSERT(condition) do { if (!(condition)) DoAssert(#condition, __func__, __LINE__); } while(0)
#define GAME_ASSERT_MESSAGE(condition, message) do { if (!(condition)) DoAssert(message, __func__, __LINE__); } while(0)
