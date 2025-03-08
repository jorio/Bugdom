#pragma once

#include "Pomme.h"
#include <QD3D.h>
#include <QD3DMath.h>
#include <SDL3/SDL.h>

#if _MSC_VER
#define _Static_assert static_assert
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include "version.h"
#include "pool.h"
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
#include "structformats.h"

extern	Boolean						gAreaCompleted;
extern	Boolean						gBatExists;
extern	Boolean						gDetonatorBlown[];
extern	Boolean						gDisableAnimSounds;
extern	Boolean						gDisableHiccupTimer;
extern	Boolean						gDoAutoFade;
extern	Boolean						gDoCeiling;
extern	Boolean						gDrawLensFlare;
extern	Boolean						gEnteringName;
extern	Boolean						gGameOverFlag;
extern	Boolean						gIsInGame;
extern	Boolean						gIsGamePaused;
extern	Boolean						gLiquidCheat;
extern	Boolean						gMuteMusicFlag;
extern	Boolean						gPlayerCanMove;
extern	Boolean						gPlayerGotKilledFlag;
extern	Boolean						gPlayerKnockOnButt;
extern	Boolean						gPlayerUsingKeyControl;
extern	Boolean						gRestoringSavedGame;
extern	Boolean						gSongPlayingFlag;
extern	Boolean						gSuperTileMemoryListExists;
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
extern	ObjNode						*gCyclorama;
extern	ObjNode						*gFirstNodePtr;
extern	ObjNode						*gHiveObj;
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
extern	RenderStats					gRenderStats;
extern	SDL_Gamepad					*gSDLGamepad;
extern	SDL_Window					*gSDLWindow;
extern	SplineDefType				**gSplineList;
extern	TQ3BoundingBox				gObjectGroupBBoxList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TQ3BoundingSphere			gObjectGroupRadiusList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
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
extern	char						gTypedAsciiKey;
extern	const char					*kLevelNames[NUM_LEVELS];
extern	const RenderModifiers		kDefaultRenderMods_UI;
extern	const RenderModifiers		kDefaultRenderMods_DebugUI;
extern	const RenderModifiers		kDefaultRenderMods_Pillarbox;
extern	const TQ3Point3D			gBatMouthOff;
extern	const TQ3Point3D			gPondFishMouthOff;
extern	const TQ3Point3D			kQ3Point3D_Zero;
extern	const float					gLiquidCollisionTopOffset[NUM_LIQUID_TYPES];
extern	float						gAutoFadeStartDist;
extern	float						gBallTimer;
extern	float						gCheckPointRot;
extern	float						gCycScale;
extern	float						gFramesPerSecond;
extern	float						gFramesPerSecondFrac;
extern	float						gGammaFadeFactor;
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
extern	int							gCurrentAntialiasingLevel;
extern	int							gDebugMode;
extern	int							gMaxItemsAllocatedInAPass;
extern	int							gNumObjNodes;
extern	int							gWindowHeight;
extern	int							gWindowWidth;
extern	int 						gCurrentSaveSlot;
extern	int32_t						gCurrentGasParticleGroup;
extern	int32_t						gHoveredPick;
extern	int32_t						gNitroParticleGroup;
extern	int32_t						gTerrainScrollBuffer[MAX_SUPERTILES_DEEP][MAX_SUPERTILES_WIDE];
extern	long						gCurrentSuperTileCol;
extern	long						gCurrentSuperTileRow;
extern	long						gEatMouse;
extern	long						gMyStartX;
extern	long						gMyStartZ;
extern	long						gNumFences;
extern	long						gNumFreeSupertiles;
extern	long						gNumSplines;
extern	long						gNumSuperTilesDeep;
extern	long						gNumSuperTilesWide;
extern	long						gNumTerrainTextureTiles;
extern	long						gPrefsFolderDirID;
extern	long						gSupertileBudget;
extern	long						gTerrainTileDepth;
extern	long						gTerrainTileWidth;
extern	long						gTerrainUnitDepth;
extern	long						gTerrainUnitWidth;
extern	short						gBestCheckPoint;
extern	short						gCurrentRopeJoint;
extern	short						gMoney;
extern	short						gNumBlueClovers;
extern	short						gNumCollisions;
extern	short						gNumEnemies;
extern	short						gNumEnemyOfKind[];
extern	short						gNumGoldClovers;
extern	short						gNumGreenClovers;
extern	short						gNumLadyBugsThisArea;
extern	short						gNumLives;
extern	short						gNumObjectsInGroupList[MAX_3DMF_GROUPS];
extern	short						gNumTerrainItems;
extern	short						gPrefsFolderVRefNum;
extern	u_long						gAutoFadeStatusBits;
extern	u_long						gInfobarUpdateBits;
extern	u_short						**gCeilingMap;
extern	u_short						**gFloorMap;
extern	u_short						**gTileDataHandle;
extern	u_short						**gVertexColors[2];
extern	u_short						gAreaNum;
extern	u_short						gLevelType;
extern	u_short						gLevelTypeMask;
extern	u_short						gRealLevel;
extern	unsigned long 				gScore;

#ifdef __cplusplus
};
#endif
