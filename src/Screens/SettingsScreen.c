/****************************/
/* SETTINGS SCREEN          */
/* Source port addition     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

extern	float						gFramesPerSecondFrac,gFramesPerSecond;
extern	WindowPtr					gCoverWindow;
extern	NewObjectDefinitionType		gNewObjectDefinition;
extern	QD3DSetupOutputType			*gGameViewInfoPtr;
extern	FSSpec						gDataSpec;
extern	PrefsType					gGamePrefs;
extern 	ObjNode 					*gFirstNodePtr;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupSettingsScreen(void);
static int SettingsScreenMainLoop(void);


/****************************/
/*    CONSTANTS             */
/****************************/

static int gHoveredPick = -1;

static const TQ3ColorRGB gTextShadowColor	= { 0.0f, 0.0f, 0.3f };
static const TQ3ColorRGB gTextColor			= { 1.0f, 0.9f, 0.0f };
static const TQ3ColorRGB gTitleTextColor	= { 1.0f, 0.9f, 0.0f };
static const TQ3ColorRGB gDeleteColor		= { 0.9f, 0.3f, 0.1f };
static const TQ3ColorRGB gBackColor			= { 1,1,1 };

static char textBuffer[512];

/*********************/
/*    VARIABLES      */
/*********************/

static TQ3Point3D	gBonusCameraFrom = {0, 0, 250 };




typedef struct
{
	Byte* ptr;
	const char* label;
	Byte subtitleShownForValue;
	const char* subtitle;
	void (*callback)(void);
	unsigned int nChoices;
	const char* choices[8];
	float y;

} SettingEntry;

static unsigned int PositiveModulo(int value, unsigned int m)
{
	int mod = value % (int) m;
	if (mod < 0)
	{
		mod += m;
	}
	return mod;
}

static void SettingEntry_Cycle(SettingEntry* entry, int delta)
{
	if (entry->nChoices == 0 && entry->choices)
	{
		// compute nChoices
		for (entry->nChoices = 0; entry->choices[entry->nChoices]; entry->nChoices++);
	}


	unsigned int value = (unsigned int) *entry->ptr;


	value = PositiveModulo(value + delta, entry->nChoices);
	*entry->ptr = value;
	if (entry->callback)
	{
		entry->callback();
	}
}


#define CHOICES_NO_YES {"NO","YES",NULL}

static SettingEntry gSettingEntries[] =
{
	{
		&gGamePrefs.easyMode,
		"KIDDIE MODE",
		1, "PLAYER TAKES LESS DAMAGE",
		NULL,
		0,
		CHOICES_NO_YES,
		0,
	},
	{
		&gGamePrefs.mouseSensitivityLevel,
		"MOUSE SENSITIVITY",
		0, NULL,
		NULL,
		0,
		{"VERY LOW", "LOW", "NORMAL", "HIGH", "VERY HIGH", NULL},
		0,
	},
	{
		&gGamePrefs.fullscreen,
		"FULLSCREEN",
		0, NULL,
		SetFullscreenMode,
		0,
		CHOICES_NO_YES,
		0,
	},
	{
		&gGamePrefs.vsync,
		"V-SYNC",
		0, NULL,
		NULL,
		0,
		CHOICES_NO_YES,
		0,
	},
	{
		&gGamePrefs.antiAliasing,
		"ANTI-ALIASING",
		0, NULL,
		NULL,
		0,
		{"NO","YES",NULL},
		0,
	},
	{
		&gGamePrefs.useCyclorama,
		"CYCLORAMA",
		0, "THE ''ATI RAGE II'' LOOK",
		NULL,
		0,
		CHOICES_NO_YES,
		0,
	},
	{
		&gGamePrefs.useAutoFade,
		"FADE FARAWAY OBJS",
		1, "EXPERIMENTAL. MAY DEGRADE PERFORMANCE.",
		NULL,
		0,
		CHOICES_NO_YES,
		0,
	},
	/*
	{
		&gGamePrefs.textureFiltering,
		"TEXTURE FILTERING",
		0, NULL,
		NULL,
		0,
		CHOICES_NO_YES,
		0,
	},
	*/
	/*
	{
		&gGamePrefs.terrainTextureDetail,
		"TERRAIN QUALITY",
		"3 LODs, 128x128 MAX",
		NULL,
		0,
		{ "LOW (VANILLA)", "MEDIUM", "HIGH" },
	},
	 */
	/*
	{
		&gGamePrefs.hideBottomBarInNonBossLevels,
		"BOTTOM BAR",
		NULL,
		NULL,
		0,
		{"ALWAYS SHOWN","ONLY WHEN NEEDED",NULL},
	},
	{
		&gGamePrefs.playerRelativeKeys,
		"PLAYER-RELATIVE KEYS",
		NULL,
		NULL,
		0,
		CHOICES_NO_YES
	},
	 */
	{
		NULL,
		NULL,
		NULL,
		NULL,
		0,
		NULL,
		0,
	}
};




/********************** DO BONUS SCREEN *************************/

void DoSettingsScreen(void)
{
	/*********/
	/* SETUP */
	/*********/

	InitCursor();

	SetupSettingsScreen();

	QD3D_CalcFramesPerSecond();

	/**************/
	/* PROCESS IT */
	/**************/

	SettingsScreenMainLoop();

	SavePrefs(&gGamePrefs);

	/* CLEANUP */

	PickableQuads_DisposeAll();

	GammaFadeOut();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DeleteAll3DMFGroups();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);
	DisposeSoundBank(SOUND_BANK_DEFAULT);
	GameScreenToBlack();
}

/****************** SETUP FILE SCREEN **************************/

/**************** MOVE BONUS BACKGROUND *********************/

static void MoveBonusBackground(ObjNode *theNode)
{
	float	fps = gFramesPerSecondFrac;

	theNode->Rot.y += fps * .03f;
	UpdateObjectTransforms(theNode);
}

static void NukeObjectsInSlot(int objNodeSlotID)
{
	ObjNode* node = gFirstNodePtr;
	while (node)
	{
		ObjNode* nextNode = node->NextNode;
		if (node->Slot == objNodeSlotID)
		{
			DeleteObject(node);
		}
		node = nextNode;
	}
}

static void MakeSettingEntryObjects(int settingID, bool firstTime)
{
	static const float XSPREAD = 100;
	static const float LH = 16;

	SettingEntry* entry = &gSettingEntries[settingID];

	// If it's not the first time we're setting up the screen, nuke old value objects
	if (!firstTime)
	{
		NukeObjectsInSlot(20000 + settingID);
	}

	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);

	tmd.coord.x = 0;
	tmd.coord.y += 80 - LH * settingID;
	tmd.align = TEXTMESH_ALIGN_LEFT;
	tmd.scale = 0.3f;
	tmd.slot = 10000 + settingID;

	// Create pickable quad
	if (firstTime)
	{
		PickableQuads_NewQuad(tmd.coord, XSPREAD*2.0f, LH*0.9f, settingID);

		// Create caption text
		tmd.coord.x = -XSPREAD;
		tmd.align = TEXTMESH_ALIGN_LEFT;
		TextMesh_Create(&tmd, entry->label);
	}

	// Create value text
	tmd.slot = 20000 + settingID;
	tmd.coord.x = XSPREAD/2.0f;
	tmd.align = TEXTMESH_ALIGN_CENTER;
	TextMesh_Create(&tmd, entry->choices[*entry->ptr]);

	// Create subtitle text
	tmd.coord.y -= LH / 2.0f;
	if (entry->subtitle && *entry->ptr == entry->subtitleShownForValue)
	{
		float scaleBackup = tmd.scale;
		tmd.scale *= 0.5f;
		TextMesh_Create(&tmd, entry->subtitle);
		tmd.scale = scaleBackup;
	}
}

static void MakeSettingsObjects(void)
{
	/* BACKGROUND */

	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
	gNewObjectDefinition.type 		= BONUS_MObjType_Background;
	gNewObjectDefinition.coord		= gBonusCameraFrom;
	gNewObjectDefinition.slot 		= 999;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_NULLSHADER /*| STATUS_BIT_HIDDEN*/;
	gNewObjectDefinition.moveCall 	= MoveBonusBackground;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 30.0;
	MakeNewDisplayGroupObject(&gNewObjectDefinition);

	/* TEXT */

	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);
	tmd.coord.y += 110;
	TextMesh_Create(&tmd, "Bugdom Settings");

	for (int i = 0; gSettingEntries[i].ptr; i++)
	{
		MakeSettingEntryObjects(i, true);
	}
}

static void SetupSettingsScreen(void)
{
	FSSpec					spec;
	QD3DSetupInputType		viewDef;
	TQ3Point3D				cameraTo = {0, 0, 0 };
	TQ3ColorRGB				lightColor = { 1.0, 1.0, .9 };
	TQ3Vector3D				fillDirection1 = { 1, -.4, -.8 };			// key
	TQ3Vector3D				fillDirection2 = { -.7, -.2, -.9 };			// fill


	/* RESET PICKABLE ITEMS */
	PickableQuads_DisposeAll();

	/*************/
	/* MAKE VIEW */
	/*************/

	QD3D_NewViewDef(&viewDef, gCoverWindow);

	viewDef.camera.hither 			= 40;
	viewDef.camera.yon 				= 2000;
	viewDef.camera.fov 				= 1.0;
	viewDef.styles.usePhong 		= false;
	viewDef.camera.from				= gBonusCameraFrom;
	viewDef.camera.to	 			= cameraTo;

	viewDef.lights.numFillLights 	= 2;
	viewDef.lights.ambientBrightness = 0.2;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[0] 	= lightColor;
	viewDef.lights.fillColor[1] 	= lightColor;
	viewDef.lights.fillBrightness[0] = 1.0;
	viewDef.lights.fillBrightness[1] = .2;

	viewDef.view.dontClear = false;
	viewDef.view.clearColor.r =
	viewDef.view.clearColor.g =
	viewDef.view.clearColor.b = 0;


	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);

	/************/
	/* LOAD ART */
	/************/

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:BonusScreen.3dmf", &spec);
	LoadGrouped3DMF(&spec,MODEL_GROUP_BONUS);

	TextMesh_Load();

	/* LOAD SOUNDS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Main.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_DEFAULT);

	/*******************/
	/* MAKE BACKGROUND */
	/*******************/

	MakeSettingsObjects();

	//---------------------------------------------------------------

	MakeFadeEvent(true);
}


/******************* DRAW BONUS STUFF ********************/

static void SettingsScreenDrawStuff(const QD3DSetupOutputType *setupInfo)
{
	DrawObjects(setupInfo);
	QD3D_DrawParticles(setupInfo);
#if _DEBUG
	PickableQuads_Draw(setupInfo->viewObject);
#endif
}

static int SettingsScreenMainLoop()
{
	while (1)
	{
		UpdateInput();
		MoveObjects();
		QD3D_MoveParticles();
		QD3D_DrawScene(gGameViewInfoPtr, SettingsScreenDrawStuff);
		QD3D_CalcFramesPerSecond();
		DoSDLMaintenance();

		// See if user hovered/clicked on a pickable item
		gHoveredPick = -1;
		{
			Point		mouse;
			TQ3Point2D	pt;

			SetPort(GetWindowPort(gCoverWindow));
			GetMouse(&mouse);
			pt.x = mouse.h;
			pt.y = mouse.v;

			TQ3Int32 pickID;
			if (PickableQuads_GetPick(gGameViewInfoPtr->viewObject, pt, &pickID))
			{
				gHoveredPick = pickID;

				if (FlushMouseButtonPress())
				{
					SettingEntry_Cycle(&gSettingEntries[pickID], 1);
					MakeSettingEntryObjects(pickID, false);
				}
			}
		}

		if (GetNewKeyState(kVK_Escape))
		{
			return -1;
		}
	}
}
