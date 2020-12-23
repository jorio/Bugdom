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
static const TQ3ColorRGB gValueTextColor	= { 190/255.0f,224/255.0f,0 };

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
	bool useClovers;
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
		0, false, CHOICES_NO_YES,
		0,
	},
	{
		&gGamePrefs.mouseSensitivityLevel,
		"MOUSE SENSITIVITY",
		0, NULL,
		NULL,
		NUM_MOUSE_SENSITIVITY_LEVELS, true, NULL,
		0,
	},
	{
		&gGamePrefs.fullscreen,
		"FULLSCREEN",
		0, NULL,
		SetFullscreenMode,
		0, false, CHOICES_NO_YES,
		0,
	},
	{
		&gGamePrefs.vsync,
		"V-SYNC",
		0, NULL,
		NULL,
		0, false, CHOICES_NO_YES,
		0,
	},
	{
		&gGamePrefs.antiAliasing,
		"ANTI-ALIASING",
		0, NULL,
		NULL,
		0, false, {"NO","YES",NULL},
		0,
	},
	{
		&gGamePrefs.useCyclorama,
		"CYCLORAMA",
		0, "THE ''ATI RAGE II'' LOOK",
		NULL,
		0, false, CHOICES_NO_YES,
		0,
	},
	{
		&gGamePrefs.useAutoFade,
		"FADE FARAWAY OBJS",
		1, "EXPERIMENTAL. MAY DEGRADE PERFORMANCE.",
		NULL,
		0, false, CHOICES_NO_YES,
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
		0, false, NULL,
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
			//QD3D_ExplodeGeometry(node, 200.0f, PARTICLE_MODE_UPTHRUST | PARTICLE_MODE_NULLSHADER, 1, 0.5f);
			DeleteObject(node);
		}
		node = nextNode;
	}
}

static void MakeSettingEntryObjects(int settingID, bool firstTime)
{
	static const float XSPREAD = 120;
	static const float LH = 28;

	const float x = 0;
	const float y = 80 - LH * settingID;
	const float z = 0;

	SettingEntry* entry = &gSettingEntries[settingID];

	// If it's not the first time we're setting up the screen, nuke old value objects
	if (!firstTime)
	{
		NukeObjectsInSlot(20000 + settingID);
	}

	bool hasSubtitle = entry->subtitle && *entry->ptr == entry->subtitleShownForValue;

	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);

	tmd.coord = (TQ3Point3D) {x,y,z};
	tmd.align = TEXTMESH_ALIGN_LEFT;
	tmd.scale = 0.3f;
	tmd.slot = 10000 + settingID;

	if (firstTime)
	{
		// Create pickable quad
		PickableQuads_NewQuad(
				(TQ3Point3D) {x+XSPREAD/2, y, z},
				XSPREAD*1.0f, LH*0.75f,
				settingID);

		// Create grass blade
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC2;
		gNewObjectDefinition.type 		= LAWN2_MObjType_Grass2;
		gNewObjectDefinition.coord		= (TQ3Point3D){x+XSPREAD, y-1, z-1};
		gNewObjectDefinition.slot 		= 10000 + settingID;
		gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;//PI+PI/2;
		gNewObjectDefinition.scale 		= .02;
		ObjNode* grassBlade = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		grassBlade->Rot.z = PI/2.0f;
		grassBlade->Scale.z = 0.001f;
		UpdateObjectTransforms(grassBlade);

		// Create caption text
		tmd.coord.x = -XSPREAD;
		tmd.align = TEXTMESH_ALIGN_LEFT;
		TextMesh_Create(&tmd, entry->label);
	}

	// Create value text
	tmd.color = gValueTextColor;
	tmd.coord.x = XSPREAD/2.0f;
	if (hasSubtitle)
		tmd.coord.y += LH * 0.15f;
	if (entry->useClovers)
	{
		for (unsigned int i = 0; i < entry->nChoices; i++)
		{
			TQ3Point3D coord = {x+XSPREAD/2, y, z};
			coord.x += 12.0f * (i - (entry->nChoices-1)/2.0f);
			gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
			gNewObjectDefinition.type 		= BONUS_MObjType_GoldClover;
			gNewObjectDefinition.coord		= coord;
			gNewObjectDefinition.slot 		= 20000 + settingID;
			gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER;
			gNewObjectDefinition.moveCall 	= nil;
			gNewObjectDefinition.rot 		= PI+PI/2;
			gNewObjectDefinition.scale 		= .03;
			ObjNode* clover = MakeNewDisplayGroupObject(&gNewObjectDefinition);
			if (i > *entry->ptr)
				MakeObjectTransparent(clover, 0.25f);
		}
	}
	else
	{
		tmd.slot = 20000 + settingID;
		tmd.align = TEXTMESH_ALIGN_CENTER;
		TextMesh_Create(&tmd, entry->choices[*entry->ptr]);
	}

	// Create subtitle text
	tmd.coord.y -= LH * 0.15f * 2.0f;
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

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:Lawn_Models2.3dmf", &spec);
	LoadGrouped3DMF(&spec,MODEL_GROUP_LEVELSPECIFIC2);

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
