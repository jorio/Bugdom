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

static void SetupSettingsScreen(void);
static void SettingsScreenMainLoop(void);


/****************************/
/*    CONSTANTS             */
/****************************/

static const TQ3ColorRGBA gValueTextColor	= TQ3ColorRGBA_FromInt(0xbee000ff);

enum
{
	PICKID_SETTING_MASK		=	0x10000000,
	PICKID_SPIDER			=	0x20000000,
	SLOTID_CAPTION_MASK		=	0x1000,
	SLOTID_VALUE_MASK		=	0x2000,
};

/*********************/
/*    VARIABLES      */
/*********************/




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
		"Kiddie mode",
		1, "Player takes less damage",
		NULL,
		0, false, CHOICES_NO_YES,
		0,
	},
	{
		&gGamePrefs.mouseSensitivityLevel,
		"Mouse sensitivity",
		0, NULL,
		NULL,
		NUM_MOUSE_SENSITIVITY_LEVELS, true, { NULL },
		0,
	},
	{
		&gGamePrefs.fullscreen,
		"Fullscreen",
		0, NULL,
		SetFullscreenMode,
		0, false, CHOICES_NO_YES,
		0,
	},
#if ALLOW_MSAA
	{
		&gGamePrefs.antiAliasing,
		"ANTI-ALIASING",
		0, NULL,
		NULL,
		0, false, {"NO","YES",NULL},
		0,
	},
#endif
#if _DEBUG
	{
		&gGamePrefs.vsync,
		"V-Sync",
		0, NULL,
		NULL,
		0, false, CHOICES_NO_YES,
		0,
	},
	{
		&gGamePrefs.useCyclorama,
		"Cyclorama",
		0, "The \"ATI Rage II\" look",
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
	{
		&gGamePrefs.terrainTextureDetail,
		"TERRAIN QUALITY",
		"3 LODs, 128x128 MAX",
		NULL,
		0,
		{ "LOW (VANILLA)", "MEDIUM", "HIGH" },
	},
	{
		&gGamePrefs.hideBottomBarInNonBossLevels,
		"BOTTOM BAR",
		0, NULL,
		NULL,
		0, false, {"ALWAYS SHOWN","ONLY WHEN NEEDED",NULL},
		0,
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
#endif
	// End sentinel
	{
		NULL,
		NULL,
		0, NULL,
		NULL,
		0, false, {NULL},
		0,
	}
};




/********************** DO BONUS SCREEN *************************/

void DoSettingsScreen(void)
{
	/*********/
	/* SETUP */
	/*********/

	SetupUIStuff();
	SetupSettingsScreen();

	QD3D_CalcFramesPerSecond();

	/**************/
	/* PROCESS IT */
	/**************/

	SettingsScreenMainLoop();

	SavePrefs(&gGamePrefs);

	/* CLEANUP */

	CleanupUIStuff();
}

/****************** SETUP FILE SCREEN **************************/

static void MakeSettingEntryObjects(int settingID, bool firstTime)
{
	static const float XSPREAD = 120;
	static const float LH = 28;

	const float x = 0;
	const float y = 60 - LH * settingID;
	const float z = 0;

	SettingEntry* entry = &gSettingEntries[settingID];

	// If it's not the first time we're setting up the screen, nuke old value objects
	if (!firstTime)
	{
		NukeObjectsInSlot(SLOTID_VALUE_MASK | settingID);
	}

	bool hasSubtitle = entry->subtitle && *entry->ptr == entry->subtitleShownForValue;

	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);

	tmd.coord = (TQ3Point3D) {x,y,z};
	tmd.align = TEXTMESH_ALIGN_LEFT;
	tmd.scale = 0.3f;
	tmd.slot = SLOTID_CAPTION_MASK | settingID;

	if (firstTime)
	{
		// Create pickable quad
		NewPickableQuad(
				(TQ3Point3D) {x+XSPREAD/2, y, z},
				XSPREAD*1.0f, LH*0.75f,
				PICKID_SETTING_MASK | settingID);

		// Create grass blade
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC2;
		gNewObjectDefinition.type 		= LAWN2_MObjType_Grass2;
		gNewObjectDefinition.coord		= (TQ3Point3D){x+XSPREAD, y-1, z-1};
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
		tmd.coord.x = -XSPREAD;
		tmd.align = TEXTMESH_ALIGN_LEFT;
		TextMesh_Create(&tmd, entry->label);
	}

	// Create value text
	tmd.color = gValueTextColor;
	tmd.coord.x = XSPREAD/2.0f;
	tmd.slot = SLOTID_VALUE_MASK | settingID;
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
			gNewObjectDefinition.slot 		= SLOTID_VALUE_MASK | settingID;
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
		tmd.slot = SLOTID_VALUE_MASK | settingID;
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

static void SetupSettingsScreen(void)
{
	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);
	tmd.coord.y += 110;
	tmd.color = gValueTextColor;
	TextMesh_Create(&tmd, "Bugdom Settings");

	for (int i = 0; gSettingEntries[i].ptr; i++)
	{
		MakeSettingEntryObjects(i, true);
	}

	MakeSpiderButton((TQ3Point3D) {-100, -100, 0}, "DONE", PICKID_SPIDER);
}


/******************* DRAW BONUS STUFF ********************/

static void SettingsScreenDrawStuff(const QD3DSetupOutputType *setupInfo)
{
	DrawObjects(setupInfo);
	QD3D_DrawParticles(setupInfo);
}

static void SettingsScreenMainLoop()
{
	while (1)
	{
		UpdateInput();
		MoveObjects();
		QD3D_MoveParticles();
		QD3D_DrawScene(gGameViewInfoPtr, SettingsScreenDrawStuff);
		QD3D_CalcFramesPerSecond();
		DoSDLMaintenance();

		if (GetNewKeyState(kKey_UI_Cancel))
			return;

		// See if user hovered/clicked on a pickable item
		int pickID = UpdateHoveredPickID();
		if (pickID >= 0 && FlushMouseButtonPress())
		{
			if (pickID & PICKID_SETTING_MASK)
			{
				int settingID = pickID & 0xFFFF;
				SettingEntry_Cycle(&gSettingEntries[settingID], 1);
				MakeSettingEntryObjects(settingID, false);

				PlayEffect(EFFECT_BONUSCLICK);
			}
			else if (pickID == PICKID_SPIDER)
			{
				PlayEffect_Parms(EFFECT_BONUSCLICK, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, kMiddleC-12);
				return;
			}
		}
	}
}
