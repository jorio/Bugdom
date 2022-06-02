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

static const TQ3ColorRGBA kValueTextColor = {0.75f, 0.88f, 0.00f, 1.00f};

enum
{
	PICKID_SETTING_MASK		=	0x10000000,
	PICKID_SPIDER			=	0x20000000,
	SLOTID_CAPTION_MASK		=	0x1000,
	SLOTID_VALUE_MASK		=	0x2000,
};

#define MAX_CHOICES 8

_Static_assert(NUM_MOUSE_SENSITIVITY_LEVELS <= MAX_CHOICES, "too many mouse sensitivity levels");

/*********************/
/*    VARIABLES      */
/*********************/

typedef struct
{
	Byte* ptr;
	const char* label;
	const char* subtitle;
	Byte subtitleShownForValue;
	void (*callback)(void);
	bool useClovers;
	unsigned int nChoices;
	const char* choices[MAX_CHOICES];
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
	unsigned int value = (unsigned int) *entry->ptr;

	value = PositiveModulo(value + delta, entry->nChoices);
	*entry->ptr = value;
	if (entry->callback)
	{
		entry->callback();
	}
}

static SettingEntry gSettingEntries[] =
{
	{
		.ptr = &gGamePrefs.easyMode,
		.label = "Kiddie mode",
		.subtitle = "Player takes less damage",
		.subtitleShownForValue = 1,
		.nChoices = 2,
		.choices = {"No", "Yes"},
	},

	{
		.ptr = &gGamePrefs.mouseSensitivityLevel,
		.label = "Mouse sensitivity",
		.useClovers = true,
		.nChoices = NUM_MOUSE_SENSITIVITY_LEVELS,
		.choices = {"1","2","3","4","5","6","7","8"},
	},

	{
		.ptr = &gGamePrefs.fullscreen,
		.label = "Fullscreen",
		.callback = SetFullscreenMode,
		.nChoices = 2,
		.choices = {"No", "Yes"},
	},

	{
		.ptr = &gGamePrefs.showBottomBar,
		.label = "Show bottom bar",
		.nChoices = 2,
		.choices = {"No", "Yes"},
	},

	{
		.ptr = &gGamePrefs.lowDetail,
		.label = "Level of detail",
		.subtitle = "The \"ATI Rage II\" look",
		.subtitleShownForValue = 1,
		.nChoices = 2,
		.choices = {"High", "Low"},
	},

#if _DEBUG
	{
		.ptr = &gGamePrefs.playerRelativeKeys,
		.label = "Tank controls",
		.nChoices = 2,
		.choices = {"No", "Yes"},
	},
#endif

	// End sentinel
	{
		.ptr = NULL,
		.label = NULL,
		.callback = NULL,
		.nChoices = 0,
	}
};




/********************** DO BONUS SCREEN *************************/

void DoSettingsScreen(void)
{
	/*********/
	/* SETUP */
	/*********/

	SetupUIStuff(kUIBackground_Cyclorama);
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
	tmd.scale = 0.25f;
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
		gNewObjectDefinition.coord		= (TQ3Point3D){x+XSPREAD+5, y-1, z-1};
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
	tmd.color = kValueTextColor;
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
	tmd.align = TEXTMESH_ALIGN_CENTER;
	tmd.coord.y += 110;
	tmd.color = kValueTextColor;
	TextMesh_Create(&tmd, "Settings");

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
		if (pickID >= 0 && IsAnalogCursorClicked())
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
