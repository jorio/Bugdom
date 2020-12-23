/****************************/
/*    BUGDOM - MAIN 		*/
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


extern	Boolean			gAbortDemoFlag,gGameIsDemoFlag,gSongPlayingFlag,gDisableHiccupTimer;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float			gFramesPerSecond,gFramesPerSecondFrac,gAutoFadeStartDist;
extern	Byte		gDemoMode,gPlayerMode;
extern	WindowPtr	gCoverWindow;
extern	TQ3Point3D	gCoord;
extern	long				gMyStartX,gMyStartZ;
extern	Boolean				gDoCeiling,gDrawLensFlare;
extern	unsigned long 		gScore;
extern	ObjNode				*gPlayerObj,*gFirstNodePtr;
extern	TQ3ShaderObject		gWaterShader;
extern	short			gNumLives;
extern	short		gBestCheckPoint,gNumEnemies;
extern	u_long 		gInfobarUpdateBits;
extern	float		gCycScale,gBallTimer;
extern	signed char	gNumEnemyOfKind[];
extern	TQ3Point3D	gMyCoord;
extern	int			gMaxItemsAllocatedInAPass;
extern	GWorldPtr	gInfoBarTop;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void InitArea(void);
static void CleanupLevel(void);
static void PlayArea(void);
static void DoDeathReset(void);
static void PlayGame(void);
static void CheckForCheats(void);
static void ShowDebug(void);


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


Boolean		gShowDebug = false;
Boolean		gLiquidCheat = false;
Boolean		gUseCyclorama;
Boolean		gShowBottomBar;
float		gCurrentYon;

u_long		gAutoFadeStatusBits;
short		gMainAppRezFile,gTextureRezfile;
Boolean		gGameOverFlag,gAbortedFlag,gAreaCompleted;
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
			
static const Boolean	gLevelHasCyc[NUM_LEVELS] =
{
	true,						// garden
	false,						// boat
	true,						// dragonfly
	false,						// hive
	true,						// night
	false						// anthill
};

static const Boolean	gLevelHasCeiling[NUM_LEVELS] =
{
	false,						// garden
	false,						// boat
	false,						// dragonfly
	true,						// hive
	false,						// night
	true						// anthill
};

static const Byte	gLevelSuperTileActiveRange[NUM_LEVELS] =
{
	5,						// garden
	4,						// boat
	5,						// dragonfly
	4,						// hive
	4,						// night
	4						// anthill
};

static const float	gLevelFogStart[NUM_LEVELS] =
{
	.5,						// garden
	.4,						// boat
	.6,						// forest
	.85,					// hive
	.4,						// night
	.65,					// anthill
};

static const float	gLevelFogEnd[NUM_LEVELS] =
{
	.9,						// garden
	1,						// boat
	.85,					// dragonfly
	1.0,					// hive
	.9,						// night
	1,						// anthill
};


static const float	gLevelAutoFadeStart[NUM_LEVELS] =
{
	YON_DISTANCE+400,		// garden
	0,						// boat
	0,						// dragonfly
	0,						// hive
	YON_DISTANCE-250,		// night
	0,						// anthill
};


static const Boolean	gLevelHasLenseFlare[NUM_LEVELS] =
{
	true,						// garden
	true,						// boat
	true,						// dragonfly
	false,						// hive
	true,						// night
	false						// anthill
};

static const TQ3Vector3D	gLensFlareVector[NUM_LEVELS] =
{
	{ 0.4f, -0.35f, 1.0f },				// garden
	{ 0.4f, -0.45f, 1.0f },				// boat
	{ 0.4f, -0.15f, 1.0f },				// dragonfly
	{ 0.4f, -0.35f, 1.0f },				// hive
	{ 0.4f, -0.35f, 1.0f },				// night
	{ 0.4f, -0.35f, 1.0f },				// anthill
};

static const TQ3ColorRGB	gLevelLightColors[NUM_LEVELS][3] =		// 0 = ambient, 1 = fill0, 2 = fill1
{
	{ {1.0f, 1.0f, 0.9f}, {1.0f, 1.0f, 0.6f}, {1.0f, 1.0f, 1.0f} }, // garden
	{ {1.0f, 1.0f, 0.9f}, {1.0f, 1.0f, 0.6f}, {1.0f, 1.0f, 1.0f} }, // boat
	{ {1.0f, 0.6f, 0.3f}, {1.0f, 0.8f, 0.3f}, {1.0f, 0.9f, 0.3f} }, // dragonfly
	{ {1.0f, 1.0f, 0.8f}, {1.0f, 1.0f, 0.7f}, {1.0f, 1.0f, 0.9f} }, // hive
	{ {0.5f, 0.5f, 0.5f}, {0.8f, 1.0f, 0.8f}, {0.6f, 0.8f, 0.7f} }, // night
	{ {0.5f, 0.5f, 0.6f}, {0.7f, 0.7f, 0.8f}, {1.0f, 1.0f, 1.0f} }, // anthill
};

static const TQ3ColorARGB	gLevelFogColor[NUM_LEVELS] =
{
	{ 1.000f, 0.050f, 0.250f, 0.050f },				// garden
	{ 1.000f, 0.900f, 0.900f, 0.850f },				// boat
	{ 1.000f, 1.000f, 0.290f, 0.063f },				// dragonfly
//	{ 1.000f, 0.900f, 0.700f, 0.100f },				// hive
	{ 1.000f, 0.700f, 0.600f, 0.400f },				// hive
	{ 1.000f, 0.020f, 0.020f, 0.080f },				// night
	{ 1.000f, 0.150f, 0.070f, 0.150f },				// anthill
};

// Source port addition: on rare occasions you get to see the void "above" the cyclorama.
// To camouflage this, we make the clear color roughly match the color at the top of the cyc.
// This is not necessarily the same color as the fog!
// NOTE: If there's no cyc in a level, this value is ignored and the fog color is used instead.
static const TQ3ColorARGB	gLevelClearColorWithCyc[NUM_LEVELS] =
{
	{ 1.000f, 0.352f, 0.380f, 1.000f },				// garden		(DIFFERENT FROM FOG)
	{ 1.000f, 0.900f, 0.900f, 0.850f },				// boat			(same)
	{ 1.000f, 0.482f, 0.188f, 0.129f },				// dragonfly	(DIFFERENT FROM FOG)
	{ 1.000f, 0.700f, 0.600f, 0.400f },				// hive			(same)
	{ 1.000f, 0.000f, 0.000f, 0.000f },				// night		(DIFFERENT FROM FOG)
	{ 1.000f, 0.150f, 0.070f, 0.150f },				// anthill		(same)
};



//======================================================================================
//======================================================================================
//======================================================================================


/*****************/
/* TOOLBOX INIT  */
/*****************/

void ToolBoxInit(void)
{
FSSpec		spec;
	
	gMainAppRezFile = CurResFile();

		/* FIRST VERIFY SYSTEM BEFORE GOING TOO FAR */
				
	VerifySystem();


			/* BOOT QD3D */
			
	QD3D_Boot();



		/* OPEN TEXTURES RESOURCE FILE */
				
	if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:Textures", &spec) != noErr)
		DoFatalAlert("ToolBoxInit: cannot find Sprites:Textures file");
	gTextureRezfile = FSpOpenResFile(&spec, fsRdPerm);
	if (gTextureRezfile == -1)
		DoFatalAlert("ToolBoxInit: Cannot locate Textures resource file!");
	UseResFile(gTextureRezfile);




			/* INIT PREFERENCES */
			
	memset(&gGamePrefs, 0, sizeof(PrefsType));
	gGamePrefs.easyMode				= false;	
	gGamePrefs.playerRelativeKeys	= false;	
	gGamePrefs.fullscreen			= true;
	gGamePrefs.vsync				= true;
	gGamePrefs.antiAliasing			= false;
	gGamePrefs.textureFiltering		= true;
	gGamePrefs.mouseSensitivityLevel= DEFAULT_MOUSE_SENSITIVITY_LEVEL;
	gGamePrefs.hideBottomBarInNonBossLevels = true;
	gGamePrefs.useCyclorama			= true;
	gGamePrefs.useAutoFade			= true;
	gGamePrefs.terrainTextureDetail = TERRAIN_TEXTURE_PREF_1_LOD_160;
				
	LoadPrefs(&gGamePrefs);							// attempt to read from prefs file		
	
	SetFullscreenMode();
	
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);

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
	
	if (GetKeyState(kVK_F10))						// see if do level cheat
		DoLevelCheatDialog();
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

		GammaFadeOut();
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
	
	QD3D_DrawScene(gGameViewInfoPtr,DrawTerrain);
	MakeFadeEvent(true);


		/******************/
		/* MAIN GAME LOOP */
		/******************/

	while(true)
	{
		fps = gFramesPerSecondFrac;
		UpdateInput();

				/* SEE IF DEMO ENDED */				
		
		if (gAbortDemoFlag)
			break;
	
	
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
		SubmitInfobarOverlay();

		QD3D_CalcFramesPerSecond();
		DoSDLMaintenance();
		gDisableHiccupTimer = false;


		/* SHOW DEBUG */
		
		if (gShowDebug)
			ShowDebug();
		

			/* SEE IF PAUSE GAME */
				
		if (gDemoMode != DEMO_MODE_RECORD)
		{
			if (GetNewKeyState(kKey_Pause))				// see if pause/abort
			{
				CaptureMouse(false);
				DoPaused();
				CaptureMouse(true);
			}
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

	gUseCyclorama			= gGamePrefs.useCyclorama && gLevelHasCyc[gLevelType];
	gAutoFadeStartDist		= gGamePrefs.useAutoFade ? gLevelAutoFadeStart[gLevelType] : 0;
	gDrawLensFlare			= gLevelHasLenseFlare[gLevelType];
		
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



	if (gAutoFadeStartDist != 0.0f)
		gAutoFadeStatusBits = STATUS_BIT_AUTOFADE|STATUS_BIT_NOTRICACHE;
	else
		gAutoFadeStatusBits = 0;


	// Source port addition: let user hide bottom bar when it's not needed (i.e. outside boss levels)
	gShowBottomBar = !gGamePrefs.hideBottomBarInNonBossLevels || gLevelTable[gRealLevel].isBossLevel;
		

			/*************/
			/* MAKE VIEW */
			/*************/

//	GameScreenToBlack();
	

			/* SETUP VIEW DEF */
			
	QD3D_NewViewDef(&viewDef, gCoverWindow);
	
	viewDef.camera.hither 			= HITHER_DISTANCE;
	viewDef.camera.yon 				= gCurrentYon;
	
	viewDef.camera.fov 				= 1.1;
	
	viewDef.view.paneClip.top		=	62;
	viewDef.view.paneClip.bottom	=	gShowBottomBar? 60: 0;
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
				
	gAbortDemoFlag = gGameOverFlag = false;
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
		
	HideCursor();								// do this again to be sure!
 }


/**************** CLEANUP LEVEL **********************/

static void CleanupLevel(void)
{
	StopAllEffectChannels();
	StopDemo();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);
 	EmptySplineObjectList();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeSuperTileMemoryList();
	DisposeTerrain();
	DeleteAllParticleGroups();
	DisposeFenceShaders();
	DisposeLensFlares();
	DisposeLiquids();
	DeleteAll3DMFGroups();
	
	if (gInfoBarTop)								// dispose of infobar cached image
	{
		DisposeGWorld(gInfoBarTop);
		gInfoBarTop = nil;
	}
	
	// Flush any Quesa errors so we don't prevent loading the next screen (Error -28411 comes up often...TODO?)
	FlushQuesaErrors();
}




/************************* DO DEATH RESET ******************************/
//
// Called from above when player is killed.
//

static void DoDeathReset(void)
{
	GammaFadeOut();												// fade out

			/* SEE IF THAT WAS THE LAST LIFE */
			
	gNumLives--;
	gInfobarUpdateBits |= UPDATE_LIVES;
	if (gNumLives <= 0)
	{
		gGameOverFlag = true;
		return;
	}
	

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
#if _DEBUG
											// in debug builds, expose cheats without needing command/control key
#elif __APPLE__
	if (GetKeyState(kVK_Command))			// must hold down the help key
#else
	if (GetKeyState(kVK_Control))			// must hold down the help key
#endif
	{
		if (GetNewKeyState(kVK_F1))			// win the level!
			gAreaCompleted = true;
			
		if (GetNewKeyState(kVK_F2))			// win the game!
		{
			gGameOverFlag = true;
			gWonGameFlag = true;
		}
		
		if (GetNewKeyState(kVK_F3))			// get full health
			GetHealth(1.0);							
			
		if (GetNewKeyState(kVK_F4))			// get full ball-time
		{
			gBallTimer = 1.0f;
			gInfobarUpdateBits |= UPDATE_TIMER;	
		}	
		
		if (GetNewKeyState(kVK_F5))			// get full inventory
		{
			GetMoney();
			GetKey(0);
			GetKey(1);
			GetKey(2);
			GetKey(3);
			GetKey(4);
		}

		if (GetNewKeyState(kVK_F6))			// see if liquid invincible
			gLiquidCheat = !gLiquidCheat;

		if (GetNewKeyState(kVK_F9))
			gShowDebug = !gShowDebug;

		if (GetNewKeyState(kVK_F10))
			CaptureMouse(false);
	}
}

#pragma mark -

/********************* SHOW DEBUG ***********************/

static void ShowDebug(void)
{
}

/********************* DEBUG SHORTCUT KEYS ON BOOT ***********************/

static void CheckDebugShortcutKeysOnBoot(void)
{
	static const int levelKeys[10] =
	{
		kVK_ANSI_1, kVK_ANSI_2, kVK_ANSI_3, kVK_ANSI_4, kVK_ANSI_5,
		kVK_ANSI_6, kVK_ANSI_7, kVK_ANSI_8, kVK_ANSI_9, kVK_ANSI_0,
	};

	if (GetKeyState(kVK_F1))
	{
		DoModelDebug();
		return;
	}

	if (GetKeyState(kVK_F2))
	{
		DoFileSelectScreen(FILE_SELECT_SCREEN_TYPE_SAVE);
		DoFileSelectScreen(FILE_SELECT_SCREEN_TYPE_LOAD);
		return;
	}

	if (GetKeyState(kVK_F3))
	{
		DoLoseScreen();
		DoWinScreen();
		return;
	}

	if (GetKeyState(kVK_F4))
	{
		DoSettingsScreen();
		return;
	}

	for (int i = 0; i < 10; i++)
	{
		if (GetKeyState(levelKeys[i]))
		{
			printf("Starting level %i\n", i);
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
				
	ToolBoxInit();
 	


			/* INIT SOME OF MY STUFF */

	InitWindowStuff();
	InitTerrainManager();
	InitSkeletonManager();
	InitSoundTools();
	Init3DMFManager();	
	InitFenceManager();
	InitParticleSystem();


			/* INIT MORE MY STUFF */
					
	InitObjectManager();
	LoadInfobarArt();
	
	GetDateTime ((unsigned long *)(&someLong));		// init random seed
	SetMyRandomSeed(someLong);


			/* DO INTRO */


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



