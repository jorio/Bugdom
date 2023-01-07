// SETTINGS SCREEN.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static const char* GenerateKiddieModeSubtitle(void);
static const char* GenerateDetailSubtitle(void);

#if !__APPLE__
static const char* GenerateMSAASubtitle(void);
#endif

#if OSXPPC
static const char* GeneratePPCFullscreenModeSubtitle(void);
static const char* GeneratePPCDisplayModeSubtitle(void);
#endif

/****************************/
/*    CONSTANTS             */
/****************************/

static const TQ3ColorRGBA kValueTextColor = {0.75f, 0.88f, 0.00f, 1.00f};

enum
{
	PICKID_SETTING_CYCLER	=	0x10000000,
	PICKID_SETTING_BUTTON	=	0x20000000,
	PICKID_SPIDER			=	0x40000000,
	SLOTID_CAPTION_MASK		=	0x1000,
	SLOTID_VALUE_MASK		=	0x2000,
};

enum
{
	kEND_SENTINEL,
	kCycler,
	kCloverRange,
	kButton,
};

#define MAX_CHOICES 32

_Static_assert(NUM_MOUSE_SENSITIVITY_LEVELS <= MAX_CHOICES, "too many mouse sensitivity levels");

/*********************/
/*    VARIABLES      */
/*********************/

typedef struct
{
	uint32_t id;
	Byte kind;
	Byte* ptr;
	const char* label;
	const char* (*subtitle)(void);
	void (*callback)(void);
	unsigned int nChoices;
	const char* choices[MAX_CHOICES];
	float y;
} SettingEntry;

static const SettingEntry* gCurrentMenu = NULL;

static const SettingEntry gSettingsMenu[] =
{
	{
		.kind = kCycler,
		.ptr = &gGamePrefs.easyMode,
		.label = "Kiddie mode",
		.subtitle = GenerateKiddieModeSubtitle,
		.nChoices = 2,
		.choices = {"No", "Yes"},
	},

#if !OSXPPC
	{
		.kind = kCloverRange,
		.ptr = &gGamePrefs.mouseSensitivityLevel,
		.label = "Mouse sensitivity",
		.nChoices = NUM_MOUSE_SENSITIVITY_LEVELS,
		.choices = {"1","2","3","4","5","6","7","8"},
	},
#endif

	{
		.kind = kCycler,
		.ptr = &gGamePrefs.dragonflyControl,
		.label = "Dragonfly steering",
		.nChoices = 4,
		.choices = {"Normal", "Invert Y axis", "Invert X axis", "Invert X & Y"},
	},

#if _DEBUG
	{
		.kind = kCycler,
		.ptr = &gGamePrefs.playerRelativeKeys,
		.label = "Tank controls",
		.nChoices = 2,
		.choices = {"No", "Yes"},
	},
#endif

	{
		.kind = kButton,
		.ptr = NULL,
		.label = "Video settings",
		.id = 'vide',
	},

	// End sentinel
	{
		.kind = kEND_SENTINEL,
		.ptr = NULL,
		.label = NULL,
		.callback = NULL,
		.nChoices = 0,
	}
};

static SettingEntry gVideoMenu[] =
{
	{
		.kind = kCycler,
		.ptr = &gGamePrefs.fullscreen,
		.label = "Fullscreen",
		.callback = SetFullscreenMode,
		.nChoices = 2,
		.choices = {"No", "Yes"},
#if OSXPPC
		.subtitle = GeneratePPCFullscreenModeSubtitle,
#endif
	},

#if OSXPPC
	{
		.kind = kCycler,
		.ptr = &gGamePrefs.curatedDisplayModeID,
		.label = "Fullscreen mode",
		.nChoices = 1,
		.choices = {"0x0"},
		.subtitle = GeneratePPCDisplayModeSubtitle,
	},
#endif

	{
		.kind = kCycler,
		.ptr = &gGamePrefs.showBottomBar,
		.label = "Bottom bar",
		.nChoices = 2,
		.choices = {"Hidden", "Visible"},
	},

	{
		.kind = kCycler,
		.ptr = &gGamePrefs.lowDetail,
		.label = "Level of detail",
		.subtitle = GenerateDetailSubtitle,
		.nChoices = 2,
		.choices = {"High", "Low"},
	},

	{
		.kind = kCycler,
		.ptr = &gGamePrefs.force4x3AspectRatio,
		.label = "Aspect ratio",
		.nChoices = 2,
		.choices = {"Fit to screen", "4:3"},
	},

#if !(__APPLE__)
	{
		.kind = kCycler,
		.ptr = &gGamePrefs.antialiasingLevel,
		.label = "Antialiasing",
		.nChoices = 4,
		.choices = {"None", "MSAA 2x", "MSAA 4x", "MSAA 8x"},
		.subtitle = GenerateMSAASubtitle,
	},
#endif

	// End sentinel
	{
		.kind = kEND_SENTINEL,
		.ptr = NULL,
		.label = NULL,
		.callback = NULL,
		.nChoices = 0,
	}
};


/***********************************************/

static void SettingEntry_Cycle(const SettingEntry* entry, int delta)
{
	unsigned int value = (unsigned int)*entry->ptr;

	value = PositiveModulo(value + delta, entry->nChoices);
	*entry->ptr = value;
	if (entry->callback)
	{
		entry->callback();
	}
}

static const char* GenerateKiddieModeSubtitle(void)
{
	return gGamePrefs.easyMode ? "Player takes less damage" : NULL;
}

static const char* GenerateDetailSubtitle(void)
{
	return gGamePrefs.lowDetail ? "The \"ATI Rage II\" look" : NULL;
}

#if !(__APPLE__)
static const char* GenerateMSAASubtitle(void)
{
	return gGamePrefs.antialiasingLevel != gAntialiasingLevelAppliedOnBoot
		? "Will apply when you restart the game"
		: NULL;
}
#endif

#if OSXPPC
static const char* GeneratePPCFullscreenModeSubtitle(void)
{
	return gGamePrefs.fullscreen != gFullscreenModeAppliedOnBoot
		? "Will apply when you restart the game"
		: NULL;
}

static const char* GeneratePPCDisplayModeSubtitle(void)
{
	static bool initialized = false;
	static Byte currentDisplayMode = 0;

	if (!initialized)
	{
		initialized = true;
		currentDisplayMode = gGamePrefs.curatedDisplayModeID;
	}

	if (gGamePrefs.curatedDisplayModeID != currentDisplayMode)
	{
		return "Will apply when you restart the game";
	}
	else
	{
		return NULL;
	}
}
#endif

/****************** SETUP SETTINGS SCREEN **************************/

static void MakeSettingEntryObjects(int settingID, bool firstTime)
{
	static const float XSPREAD = 120;
	static const float LH = 28;

	const float x = 0;
	const float y = 60 - LH * settingID;
	const float z = 0;

	const SettingEntry* entry = &gCurrentMenu[settingID];

	// If it's not the first time we're setting up the screen, nuke old value objects
	if (!firstTime)
	{
		NukeObjectsInSlot(SLOTID_VALUE_MASK | settingID);
	}

	const char* subtitleGenerator = entry->subtitle ? entry->subtitle() : NULL;

	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);

	tmd.coord = (TQ3Point3D) {x,y,z};
	tmd.align = TEXTMESH_ALIGN_LEFT;
	tmd.scale = 0.25f;
	tmd.slot = SLOTID_CAPTION_MASK | settingID;

	float pickableX = x;
	int pickableMask;

	if (entry->kind != kButton)
	{
		pickableX += XSPREAD / 2;
		pickableMask = PICKID_SETTING_CYCLER | settingID;
	}
	else
	{
		pickableMask = PICKID_SETTING_BUTTON | settingID;
	}

	if (firstTime)
	{
		// Create pickable quad
		NewPickableQuad(
				(TQ3Point3D) {pickableX, y, z},
				XSPREAD*1.0f, LH*0.75f,
				pickableMask);

		// Create grass blade
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC2;
		gNewObjectDefinition.type 		= LAWN2_MObjType_Grass2;
		gNewObjectDefinition.coord		= (TQ3Point3D){pickableX+XSPREAD/2+5, y-1, z-1};
		gNewObjectDefinition.slot 		= SLOTID_CAPTION_MASK | settingID;
		gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;//PI+PI/2;
		gNewObjectDefinition.scale 		= .02;
		ObjNode* grassBlade = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		grassBlade->Rot.z = PI/2.0f;
		grassBlade->Scale.z = 0.001f;
		UpdateObjectTransforms(grassBlade);

		// Create caption text
		if (entry->kind != kButton)
		{
			tmd.coord.x = -XSPREAD;
			tmd.align = TEXTMESH_ALIGN_LEFT;
			TextMesh_Create(&tmd, entry->label);
		}
	}

	// Create value text
	tmd.color = kValueTextColor;
	tmd.coord.x = pickableX;
	tmd.slot = SLOTID_VALUE_MASK | settingID;
	if (subtitleGenerator)
		tmd.coord.y += LH * 0.15f;

	switch (entry->kind)
	{
		case kCloverRange:
			for (unsigned int i = 0; i < entry->nChoices; i++)
			{
				TQ3Point3D coord = {x+XSPREAD/2, y, z};
				coord.x += 12.0f * (i - (entry->nChoices-1)/2.0f);
				gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
				gNewObjectDefinition.type 		= BONUS_MObjType_GoldClover;
				gNewObjectDefinition.coord		= coord;
				gNewObjectDefinition.slot 		= SLOTID_VALUE_MASK | settingID;
				gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER;
				gNewObjectDefinition.moveCall 	= nil;
				gNewObjectDefinition.rot 		= PI+PI/2;
				gNewObjectDefinition.scale 		= .03;
				ObjNode* clover = MakeNewDisplayGroupObject(&gNewObjectDefinition);
				if (i > *entry->ptr)
					MakeObjectTransparent(clover, 0.25f);
			}
			break;

		case kCycler:
			tmd.slot = SLOTID_VALUE_MASK | settingID;
			tmd.align = TEXTMESH_ALIGN_CENTER;
			TextMesh_Create(&tmd, entry->choices[*entry->ptr]);
			break;

		case kButton:
			tmd.slot = SLOTID_VALUE_MASK | settingID;
			tmd.align = TEXTMESH_ALIGN_CENTER;
			TextMesh_Create(&tmd, entry->label);
			break;
	}

	// Create subtitle text
	tmd.coord.y -= LH * 0.15f * 2.0f;
	if (subtitleGenerator)
	{
		float scaleBackup = tmd.scale;
		tmd.scale *= 0.5f;
		TextMesh_Create(&tmd, subtitleGenerator);
		tmd.scale = scaleBackup;
	}
}

static void SetupSettingsScreen(const char* title, const SettingEntry* menu)
{
	SetupUIStuff(kUIBackground_Cyclorama);

	gCurrentMenu = menu;

	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);
	tmd.align = TEXTMESH_ALIGN_CENTER;
	tmd.coord.y += 110;
	tmd.color = kValueTextColor;
	TextMesh_Create(&tmd, title);

	for (int i = 0; gCurrentMenu[i].label; i++)
	{
		MakeSettingEntryObjects(i, true);
	}

	MakeSpiderButton((TQ3Point3D) {-100, -100, 0}, "DONE", PICKID_SPIDER);
}

static void MakeMSAAWarning(void)
{
	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);
	tmd.align = TEXTMESH_ALIGN_RIGHT;
	tmd.coord = (TQ3Point3D){ 220, -220, 0 };
	tmd.color = kValueTextColor;
	tmd.scale = .2;

	TextMesh_Create(&tmd, "The new antialiasing level will take effect when you restart the game.");
}

/******************* DO SETTINGS SCREEN ********************/

static void SettingsScreenDrawStuff(const QD3DSetupOutputType *setupInfo)
{
	DrawObjects(setupInfo);
	QD3D_DrawParticles(setupInfo);
}

#if OSXPPC
static void FillCuratedDisplayModeOptions(void)
{
	static SettingEntry* displayModeEntry = &gVideoMenu[1];
	static char modeNames[24][MAX_CHOICES];

	Byte* curatedModeIDs = NULL;
	int numCuratedModes = CurateDisplayModes(0, &curatedModeIDs);
	if (numCuratedModes > MAX_CHOICES)
		numCuratedModes = MAX_CHOICES;

	displayModeEntry->nChoices = numCuratedModes;
	for (int i = 0; i < numCuratedModes; i++)
	{
		SDL_DisplayMode mode = {0};
		SDL_GetDisplayMode(0, curatedModeIDs[i], &mode);

		snprintf(modeNames[i], sizeof(modeNames[i]), "%dx%d, %dHz", mode.w, mode.h, mode.refresh_rate);
		displayModeEntry->choices[i] = modeNames[i];
	}
}
#endif

void DoSettingsScreen(void)
{
#if OSXPPC
	FillCuratedDisplayModeOptions();
#endif

	SetupSettingsScreen("Settings", gSettingsMenu);
	bool done = false;

	while (!done)
	{
		UpdateInput();
		MoveObjects();
		QD3D_MoveParticles();
		QD3D_DrawScene(gGameViewInfoPtr, SettingsScreenDrawStuff);
		QD3D_CalcFramesPerSecond();
		DoSDLMaintenance();

		if (GetNewKeyState(kKey_UI_Cancel))
		{
			done = true;
			continue;
		}

		// See if user hovered/clicked on a pickable item
		int pickID = UpdateHoveredPickID();
		if (pickID >= 0 && IsAnalogCursorClicked())
		{
			int settingID = pickID & 0xFFFF;

			if (pickID & PICKID_SETTING_CYCLER)
			{
				SettingEntry_Cycle(&gCurrentMenu[settingID], 1);
				MakeSettingEntryObjects(settingID, false);

				PlayEffect(EFFECT_BONUSCLICK);
			}
			else if (pickID & PICKID_SETTING_BUTTON)
			{
				PlayEffect_Parms(EFFECT_BONUSCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, kMiddleC - 12);

				switch (gCurrentMenu[settingID].id)
				{
				case 'vide':
					CleanupUIStuff();
					SetupSettingsScreen("Video settings", gVideoMenu);
					MakeMSAAWarning();
				}
			}
			else if (pickID == PICKID_SPIDER)
			{
				PlayEffect_Parms(EFFECT_BONUSCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, kMiddleC-12);
				done = true;
				continue;
			}
		}
	}

	CleanupUIStuff();

	SavePrefs(&gGamePrefs);
}
