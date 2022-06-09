/****************************/
/*    BUGDOM - MAIN 		*/
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include <string.h>


/****************************/
/*    PROTOTYPES            */
/****************************/

static void InitArea(void);
static void CleanupLevel(void);
static void PlayArea(void);
static void DoDeathReset(void);
static void PlayGame(void);
static void CheckForCheats(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	KILL_DELAY	4

typedef struct
{
	Byte	levelType;
	Byte	areaNum;
	Boolean	isBossLevel;
}LevelType;


/****************************/
/*    VARIABLES             */
/****************************/

static LevelType	gLevelTable[NUM_LEVELS] =
{
	{ LEVEL_TYPE_LAWN,		0,	false },			// 0: training
	{ LEVEL_TYPE_LAWN,		1,	false },			// 1: lawn
	{ LEVEL_TYPE_POND,		0,	false },			// 2: pond
	{ LEVEL_TYPE_FOREST,	0,	false },			// 3: beach
	{ LEVEL_TYPE_FOREST,	1,	true },				// 4: dragonfly attack
	{ LEVEL_TYPE_HIVE,		0,	false },			// 5: bee hive
	{ LEVEL_TYPE_HIVE,		1,	true },				// 6: queen bee
	{ LEVEL_TYPE_NIGHT,		0,	false },			// 7: night
	{ LEVEL_TYPE_ANTHILL,	0,	false },			// 8: ant hill
	{ LEVEL_TYPE_ANTHILL,	1,	true },				// 9: ant king
};

u_short		gRealLevel = 0;
u_short		gLevelType = 0;
u_short		gAreaNum = 0;
u_short		gLevelTypeMask = 0;


Boolean		gShowDebugStats = false;
Boolean		gShowDebug = false;
Boolean		gLiquidCheat = false;
Boolean		gUseCyclorama;
float		gCurrentYon;

u_long		gAutoFadeStatusBits;
short		gMainAppRezFile;
Boolean		gGameOverFlag,gAreaCompleted;
Boolean		gPlayerGotKilledFlag,gWonGameFlag,gRestoringSavedGame = false;

QD3DSetupOutputType		*gGameViewInfoPtr = nil;

PrefsType	gGamePrefs;

FSSpec		gDataSpec;


TQ3Vector3D		gLightDirection1 = { .4, -.35, 1 };		// also serves as lense flare vector
TQ3Vector3D		gLightDirection2;

TQ3ColorRGB		gAmbientColor = { 1.0, 1.0, .9 };
TQ3ColorRGB		gFillColor1 = { 1.0, 1.0, .6 };
TQ3ColorRGB		gFillColor2 = { 1.0, 1.0, 1 };


		/* LEVEL SETUP PARAMETER TABLES */

static const bool	gLevelHasCyc[NUM_LEVEL_TYPES] =
{
	true,						// garden
	false,						// boat
	true,						// dragonfly
	false,						// hive
	true,						// night
	false						// anthill
};

static const bool	gLevelHasCeiling[NUM_LEVEL_TYPES] =
{
	false,						// garden
	false,						// boat
	false,						// dragonfly
	true,						// hive
	false,						// night
	true						// anthill
};

static const Byte	gLevelSuperTileActiveRange[NUM_LEVEL_TYPES] =
{
	5,						// garden
	4,						// boat
	5,						// dragonfly
	4,						// hive
	4,						// night
	4						// anthill
};

static const float	gLevelFogStart[NUM_LEVEL_TYPES] =
{
	.5,						// garden
	.4,						// boat
	.6,						// forest
	.85,					// hive
	.4,						// night
	.65,					// anthill
};

static const float	gLevelFogEnd[NUM_LEVEL_TYPES] =
{
	.9,						// garden
	1,						// boat
	.85,					// dragonfly
	1.0,					// hive
	.9,						// night
	1,						// anthill
};


static const float	gLevelAutoFadeStart[NUM_LEVEL_TYPES] =
{
	YON_DISTANCE+400,		// garden
	0,						// boat
	0,						// dragonfly
	0,						// hive
	YON_DISTANCE-250,		// night
	0,						// anthill
};


static const bool	gLevelHasLensFlare[NUM_LEVEL_TYPES] =
{
	true,						// garden
	true,						// boat
	true,						// dragonfly
	false,						// hive
	true,						// night
	false						// anthill
};

static const TQ3Vector3D	gLensFlareVector[NUM_LEVEL_TYPES] =
{
	{ 0.4f, -0.35f, 1.0f },				// garden
	{ 0.4f, -0.45f, 1.0f },				// boat
	{ 0.4f, -0.15f, 1.0f },				// dragonfly
	{ 0.4f, -0.35f, 1.0f },				// hive
	{ 0.4f, -0.35f, 1.0f },				// night
	{ 0.4f, -0.35f, 1.0f },				// anthill
};

static const TQ3ColorRGB	gLevelLightColors[NUM_LEVEL_TYPES][3] =		// 0 = ambient, 1 = fill0, 2 = fill1
{
	{ {1.0f, 1.0f, 0.9f}, {1.0f, 1.0f, 0.6f}, {1.0f, 1.0f, 1.0f} }, // garden
	{ {1.0f, 1.0f, 0.9f}, {1.0f, 1.0f, 0.6f}, {1.0f, 1.0f, 1.0f} }, // boat
	{ {1.0f, 0.6f, 0.3f}, {1.0f, 0.8f, 0.3f}, {1.0f, 0.9f, 0.3f} }, // dragonfly
	{ {1.0f, 1.0f, 0.8f}, {1.0f, 1.0f, 0.7f}, {1.0f, 1.0f, 0.9f} }, // hive
	{ {0.5f, 0.5f, 0.5f}, {0.8f, 1.0f, 0.8f}, {0.6f, 0.8f, 0.7f} }, // night
	{ {0.5f, 0.5f, 0.6f}, {0.7f, 0.7f, 0.8f}, {1.0f, 1.0f, 1.0f} }, // anthill
};

static const TQ3ColorRGBA	gLevelFogColor[NUM_LEVEL_TYPES] =
{
	{ 0.050f, 0.250f, 0.050f, 1.000f },				// garden
	{ 0.900f, 0.900f, 0.850f, 1.000f },				// boat
	{ 1.000f, 0.290f, 0.063f, 1.000f },				// dragonfly
//	{ 0.900f, 0.700f, 0.100f, 1.000f },				// hive
	{ 0.700f, 0.600f, 0.400f, 1.000f },				// hive
	{ 0.020f, 0.020f, 0.080f, 1.000f },				// night
	{ 0.150f, 0.070f, 0.150f, 1.000f },				// anthill
};

// Source port addition: on rare occasions you get to see the void "above" the cyclorama.
// To camouflage this, we make the clear color roughly match the color at the top of the cyc.
// This is not necessarily the same color as the fog!
// NOTE: If there's no cyc in a level, this value is ignored and the fog color is used instead.
static const TQ3ColorRGBA	gLevelClearColorWithCyc[NUM_LEVEL_TYPES] =
{
	{ 0.352f, 0.380f, 1.000f, 1.000f },				// garden		(DIFFERENT FROM FOG)
	{ 0.900f, 0.900f, 0.850f, 1.000f },				// boat			(same)
	{ 1.000f, 0.290f, 0.063f, 1.000f },				// dragonfly	(same)
	{ 0.700f, 0.600f, 0.400f, 1.000f },				// hive			(same)
	{ 0.020f, 0.020f, 0.080f, 1.000f },				// night		(same)
	{ 0.150f, 0.070f, 0.150f, 1.000f },				// anthill		(same)
};



//======================================================================================
//======================================================================================
//======================================================================================


/*****************/
/* TOOLBOX INIT  */
/*****************/

void ToolBoxInit(void)
{
	gMainAppRezFile = CurResFile();

		/* FIRST VERIFY SYSTEM BEFORE GOING TOO FAR */
				
	VerifySystem();

	InitPrefs();
}

/******************** INIT PREFS ************************/

void InitPrefs(void)
{
	memset(&gGamePrefs, 0, sizeof(PrefsType));

	gGamePrefs.easyMode				= false;	
	gGamePrefs.playerRelativeKeys	= false;	
	gGamePrefs.fullscreen			= true;
	gGamePrefs.lowDetail			= false;
	gGamePrefs.mouseSensitivityLevel= DEFAULT_MOUSE_SENSITIVITY_LEVEL;
	gGamePrefs.showBottomBar		= true;
	gGamePrefs.force4x3AspectRatio	= false;
	gGamePrefs.antialiasingLevel	= 0;

	LoadPrefs(&gGamePrefs);							// attempt to read from prefs file		
}

#pragma mark -

/******************** PLAY GAME ************************/

static void PlayGame(void)
{

			/***********************/
			/* GAME INITIALIZATION */
			/***********************/
			
	InitInventoryForGame();
	gGameOverFlag = false;


			/* CHEAT: LET USER SELECT STARTING LEVEL & AREA */

	UpdateInput();
	
	if (GetKeyState_SDL(SDL_SCANCODE_F10))				// see if do level cheat
	{
		if (!DoLevelSelect())
			return;
	}
	else
	if (!gRestoringSavedGame)							// otherwise start @ 0 if not restoring
		gRealLevel = 0;

			/**********************/
			/* GO THRU EACH LEVEL */
			/**********************/
			//
			// Note: gRealLevel is already set if restoring a saved game
			//
					
	for (; gRealLevel < NUM_LEVELS; gRealLevel++)
	{
			/* GET LEVEL TYPE & AREA FOR THIS LEVEL */

		gLevelType = gLevelTable[gRealLevel].levelType;
		gAreaNum = gLevelTable[gRealLevel].areaNum;
		

			/* PLAY THIS AREA */
		
		ShowLevelIntroScreen();
		InitArea();

		gRestoringSavedGame = false;				// we dont need this anymore
		
		PlayArea();


			/* CLEANUP LEVEL */

		GammaFadeOut(true);
		CleanupLevel();
		GameScreenToBlack();		
		
		if (gGameOverFlag)
			goto game_over;
			
		/* DO END-LEVEL BONUS SCREEN */
			
		DoBonusScreen();
	}
	
			/*************/
			/* GAME OVER */
			/*************/
game_over:
			/* PLAY WIN MOVIE */
	
	if (gWonGameFlag)
		DoWinScreen();
	
			/* PLAY LOSE MOVIE */
	else
		DoLoseScreen();

	ShowHighScoresScreen(gScore);
}



/**************** PLAY AREA ************************/

static void PlayArea(void)
{
float killDelay = KILL_DELAY;						// time to wait after I'm dead before fading out
float fps;

	CaptureMouse(true);
	
	UpdateInput();
	QD3D_CalcFramesPerSecond();						// prime this
	QD3D_CalcFramesPerSecond();

		/* PRIME 1ST FRAME & MADE FADE EVENT */

	gGammaFadeFactor = 0.0f;
	QD3D_DrawScene(gGameViewInfoPtr,DrawTerrain);
	MakeFadeEvent(true);

	ResetInputState();

		/******************/
		/* MAIN GAME LOOP */
		/******************/

	while(true)
	{
		fps = gFramesPerSecondFrac;
		UpdateInput();

				/* SPECIFIC MAINTENANCE */

		CheckPlayerMorph();				
		UpdateLiquidAnimation();
		UpdateHoneyTubeTextureAnimation();
		UpdateRootSwings();
		
	
				/* MOVE OBJECTS */
				
		MoveObjects();
		MoveSplineObjects();
		QD3D_MoveParticles();
		MoveParticleGroups();
		UpdateCamera();
	
			/* DRAW OBJECTS & TERRAIN */
					
		UpdateInfobar();

		DoMyTerrainUpdate();
		QD3D_DrawScene(gGameViewInfoPtr,DrawTerrain);

		QD3D_CalcFramesPerSecond();
		DoSDLMaintenance();
		gDisableHiccupTimer = false;

			/* SEE IF PAUSE GAME */

		if (GetNewKeyState(kKey_Pause))				// see if pause/abort
		{
			CaptureMouse(false);
			DoPaused();
			CaptureMouse(true);
		}

			/* SEE IF GAME ENDED */				
		
		if (gGameOverFlag)
			break;

		if (gAreaCompleted)
		{
			if (gRealLevel == LEVEL_NUM_ANTKING)		// if completed Ant King, then I won!
				gWonGameFlag = true;
			break;
		}

			/* CHECK FOR CHEATS */
			
		CheckForCheats();
					
			
			/* SEE IF GOT KILLED */
				
		if (gPlayerGotKilledFlag)				// if got killed, then hang around for a few seconds before resetting player
		{
			killDelay -= fps;					
			if (killDelay < 0.0f)				// see if time to reset player
			{
				killDelay = KILL_DELAY;			// reset kill timer for next death
				DoDeathReset();
				if (gGameOverFlag)				// see if that's all folks
					break;
			}
			ResetInputState();
		}	
	}
	
	CaptureMouse(false);
}


/***************** INIT AREA ************************/

static void InitArea(void)
{
QD3DSetupInputType	viewDef;


			/* INIT SOME PRELIM STUFF */

	gLevelTypeMask 			= 1 << gLevelType;						// create bit mask
	gAreaCompleted 			= false;
	gPlayerMode 			= PLAYER_MODE_BUG;						// init this here so infobar looks correct
	gPlayerObj 				= nil;

	gUseCyclorama			= !gGamePrefs.lowDetail && gLevelHasCyc[gLevelType];
	gAutoFadeStartDist		= gUseCyclorama ? gLevelAutoFadeStart[gLevelType] : 0;
	gDoAutoFade				= gAutoFadeStartDist > 0.0f;
	gDrawLensFlare			= !gGamePrefs.lowDetail && gLevelHasLensFlare[gLevelType];

	gDoCeiling				= gLevelHasCeiling[gLevelType];
	gSuperTileActiveRange	= gLevelSuperTileActiveRange[gLevelType];
	
		
	gAmbientColor 			= gLevelLightColors[gLevelType][0];
	gFillColor1 			= gLevelLightColors[gLevelType][1];
	gFillColor2 			= gLevelLightColors[gLevelType][2];
	gLightDirection1 		= gLensFlareVector[gLevelType];		// get direction of light 0 for lense flare
	Q3Vector3D_Normalize(&gLightDirection1,&gLightDirection1);
	
	gBestCheckPoint			= -1;								// no checkpoint yet

		
	if (gSuperTileActiveRange == 5)								// set yon clipping value
	{
		gCurrentYon = YON_DISTANCE + 1700;
		gCycScale = 81;
	}
	else
	{
		gCurrentYon = YON_DISTANCE;
		gCycScale = 50;
	}



	if (gDoAutoFade)
		gAutoFadeStatusBits = STATUS_BIT_AUTOFADE|STATUS_BIT_NOTRICACHE;
	else
		gAutoFadeStatusBits = 0;


			/*************/
			/* MAKE VIEW */
			/*************/

//	GameScreenToBlack();
	

			/* SETUP VIEW DEF */
			
	QD3D_NewViewDef(&viewDef);
	
	viewDef.camera.hither 			= HITHER_DISTANCE;
	viewDef.camera.yon 				= gCurrentYon;
	
	viewDef.camera.fov 				= 1.1;
	
	viewDef.view.paneClip.top		=	62;
	viewDef.view.paneClip.bottom	=	gGamePrefs.showBottomBar ? 60 : 0;
	viewDef.view.paneClip.left		=	0;
	viewDef.view.paneClip.right		=	0;

	viewDef.view.clearColor 	= gLevelFogColor[gLevelType];	// set clear & fog color

	
			/* SET LIGHTS */
			

	if (gDoCeiling)
	{
		viewDef.lights.numFillLights 	= 2;
		gLightDirection2.x = -.8;
		gLightDirection2.y = 1.0;
		gLightDirection2.z = -.2;
		viewDef.lights.fillBrightness[1] = .8;
	}
	else
	{
		viewDef.lights.numFillLights 	= 2;	
		gLightDirection2.x = -.2;
		gLightDirection2.y = -.7;
		gLightDirection2.z = -.1;
		viewDef.lights.fillBrightness[1] 	= .5;
	}

	Q3Vector3D_Normalize(&gLightDirection1,&gLightDirection1);
	Q3Vector3D_Normalize(&gLightDirection2,&gLightDirection2);

	viewDef.lights.ambientBrightness 	= 0.2;
	viewDef.lights.fillDirection[0] 	= gLightDirection1;
	viewDef.lights.fillDirection[1] 	= gLightDirection2;
	viewDef.lights.fillBrightness[0] 	= 1.1;
	viewDef.lights.fillBrightness[1] 	= .5;
	
	viewDef.lights.ambientColor 	= gAmbientColor;
	viewDef.lights.fillColor[0] 	= gFillColor1;
	viewDef.lights.fillColor[1] 	= gFillColor2;
	
	
			/* SET FOG */
			
	viewDef.lights.useFog 		= true;
	viewDef.lights.fogColor		= gLevelFogColor[gLevelType];
	viewDef.lights.fogStart 	= gLevelFogStart[gLevelType];
	viewDef.lights.fogEnd	 	= gLevelFogEnd[gLevelType];
	viewDef.lights.fogDensity 	= 1.0;	
	viewDef.lights.fogMode		= kQ3FogModePlaneBasedLinear;  // Source port note: plane-based linear fog accurately reproduces fog rendering on real Macs

	// Source port addition: camouflage seam in sky with custom clear color that roughly matches top of cyc
	if (gUseCyclorama)
	{
		viewDef.lights.useCustomFogColor = true;	// need this so fog color will be different from clear color
		viewDef.view.clearColor = gLevelClearColorWithCyc[gLevelType];
	}
	
//	if (gUseCyclorama && (gLevelType != LEVEL_TYPE_FOREST) && (gLevelType != LEVEL_TYPE_NIGHT))
//		viewDef.view.dontClear		= true;

		
	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);

	
			/**********************/
			/* LOAD ART & TERRAIN */
			/**********************/
			
	LoadLevelArt();			


				/* INIT FLAGS */

	gGameOverFlag = false;
	gPlayerGotKilledFlag = false;
	gWonGameFlag = false;

		/* DRAW INITIAL INFOBAR */
				
	InitInventoryForArea();					// must call after terrain is loaded!!
	InitInfobar();			
	

			/* INIT OTHER MANAGERS */

	CreateSuperTileMemoryList();	
	
	QD3D_InitParticles();	
	InitParticleSystem();
	InitItemsManager();

		
		/* INIT THE PLAYER */
			
	InitPlayerAtStartOfLevel();
	InitEnemyManager();	
						
			/* INIT CAMERA */
			
	InitCamera();
	
			/* PREP THE TERRAIN */
			
	PrimeInitialTerrain(false);

			/* INIT BACKGROUND */
			
	if (gUseCyclorama)
		CreateCyclorama();
 }


/**************** CLEANUP LEVEL **********************/

static void CleanupLevel(void)
{
	StopAllEffectChannels();
 	EmptySplineObjectList();
	DeleteAllObjects();
	gCyclorama = nil;
	FreeAllSkeletonFiles(-1);
	DisposeSuperTileMemoryList();
	DisposeTerrain();
	DeleteAllParticleGroups();
	DisposeFences();
	DisposeLensFlares();
	DisposeLiquids();
	DeleteAll3DMFGroups();
	DisposeInfobarTexture();
	QD3D_DisposeParticles();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);
	DisposeAllSoundBanks();
	Pomme_FlushPtrTracking(true);

			/* CLEAR ANY RESIDUAL REFERENCES TO LEVEL OBJECTS */

	gTheQueen = nil;
	gHiveObj = nil;
	gAntKingObj = nil;
}




/************************* DO DEATH RESET ******************************/
//
// Called from above when player is killed.
//

static void DoDeathReset(void)
{

			/* SEE IF THAT WAS THE LAST LIFE */
			
	gNumLives--;
	gInfobarUpdateBits |= UPDATE_LIVES;
	if (gNumLives <= 0)
	{
		gGameOverFlag = true;
		return;		// don't fade out -- PlayGame() does a final fade out as it cleans up after PlayArea returns
	}

			/* FADE OUT IF WE HAVE MORE LIVES */

	GammaFadeOut(false);
	

			/* RESET THE PLAYER & CAMERA INFO */
				
	ResetPlayer();
	InitCamera();
	
	
		/* RESET TERRAIN SCROLL AT LAST CHECKPOINT */
		
	InitCurrentScrollSettings();
	PrimeInitialTerrain(true);
	
	MakeFadeEvent(true);
}



/******************** CHECK FOR CHEATS ************************/

static void CheckForCheats(void)
{
#if !(_DEBUG)	// in debug builds, expose cheats without needing command/control key
	if (GetKeyState_SDL(SDL_SCANCODE_GRAVE))	// must hold down the help key
#endif
	{
		if (GetNewKeyState_SDL(SDL_SCANCODE_F1))	// win the level!
			gAreaCompleted = true;

		if (GetNewKeyState_SDL(SDL_SCANCODE_F2))	// get shield
			gShieldTimer = SHIELD_TIME;

		if (GetKeyState_SDL(SDL_SCANCODE_F3))	// get full health
			GetHealth(1.0);							
			
		if (GetKeyState_SDL(SDL_SCANCODE_F4))	// get full ball-time
		{
			gBallTimer = 1.0f;
			gInfobarUpdateBits |= UPDATE_TIMER;	
		}	
		
		if (GetKeyState_SDL(SDL_SCANCODE_F5))	// get full inventory
		{
			GetMoney();
			GetKey(0);
			GetKey(1);
			GetKey(2);
			GetKey(3);
			GetKey(4);
		}

		if (GetNewKeyState_SDL(SDL_SCANCODE_F6))	// see if liquid invincible
			gLiquidCheat = !gLiquidCheat;

		if (GetKeyState_SDL(SDL_SCANCODE_F7))		// hurt player
			PlayerGotHurt(NULL, 1/60.0f, 1.0f, false, true, 1/60.0f);

		if (GetNewKeyState_SDL(SDL_SCANCODE_F8))
		{
			gShowDebugStats = !gShowDebugStats;
			QD3D_UpdateDebugTextMesh(NULL);
		}

		if (GetNewKeyState_SDL(SDL_SCANCODE_F9))
			gShowDebug = !gShowDebug;
	}
}

#pragma mark -

/********************* DEBUG SHORTCUT KEYS ON BOOT ***********************/

static void CheckDebugShortcutKeysOnBoot(void)
{
	static const int levelKeys[10] =
	{
		SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4, SDL_SCANCODE_5,
		SDL_SCANCODE_6, SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9, SDL_SCANCODE_0,
	};

	if (GetKeyState_SDL(SDL_SCANCODE_F1))
	{
		DoModelDebug();
		return;
	}

	if (GetKeyState_SDL(SDL_SCANCODE_F2))
	{
		DoFileSelectScreen(FILE_SELECT_SCREEN_TYPE_SAVE);
		DoFileSelectScreen(FILE_SELECT_SCREEN_TYPE_LOAD);
		return;
	}

	if (GetKeyState_SDL(SDL_SCANCODE_F3))
	{
		DoLoseScreen();
		DoWinScreen();
		return;
	}

	if (GetKeyState_SDL(SDL_SCANCODE_F4))
	{
		DoSettingsScreen();
		return;
	}

	for (int i = 0; i < 10; i++)
	{
		if (GetKeyState_SDL(levelKeys[i]))
		{
			InitInventoryForGame();
			gRealLevel = i;
			gRestoringSavedGame = true;
			PlayGame();
			return;
		}
	}
}



#pragma mark -

/************************************************************/
/******************** PROGRAM MAIN ENTRY  *******************/
/************************************************************/


int GameMain(void)
{
unsigned long	someLong;

				/**************/
				/* BOOT STUFF */
				/**************/

	TryOpenController(true);

	ToolBoxInit();
	SDL_ShowCursor(0);

			/* INIT SOME OF MY STUFF */

	Render_CreateContext();
	InitWindowStuff();
	InitTerrainManager();
	InitSkeletonManager();
	InitSoundTools();
	Init3DMFManager();	
	InitFenceManager();


			/* INIT MORE MY STUFF */
					
	InitObjectManager();
	LoadInfobarArt();
	
	GetDateTime ((unsigned long *)(&someLong));		// init random seed
	SetMyRandomSeed(someLong);

	SDL_WarpMouseInWindow(gSDLWindow, gWindowWidth/2, gWindowHeight/2);		// prime cursor position


			/* DO INTRO */

	Pomme_FlushPtrTracking(false);

	DoPangeaLogo();
	CheckDebugShortcutKeysOnBoot();


		/* MAIN LOOP */
			
	while(true)
	{
		DoTitleScreen();
		if (DoMainMenu())
			continue;
		
		PlayGame();
	}
	
	
	return(0);
}



