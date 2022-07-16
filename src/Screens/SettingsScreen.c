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
static const char* GenerateMSAASubtitle(void);
static void MoveGrass(ObjNode *theNode);

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

#define MAX_CHOICES 8

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

    /*
	{
		.kind = kCloverRange,
		.ptr = &gGamePrefs.mouseSensitivityLevel,
		.label = "Mouse sensitivity",
		.nChoices = NUM_MOUSE_SENSITIVITY_LEVELS,
		.choices = {"1","2","3","4","5","6","7","8"},
	},
    */

//#if _DEBUG
	{
		.kind = kCycler,
		.ptr = &gGamePrefs.playerRelativeKeys,
		.label = "Tank controls (D-pad)",
		.nChoices = 2,
		.choices = {"No", "Yes"},
	},
//#endif

	{
		.kind = kCycler,
		.ptr = &gGamePrefs.flipCameraHorizontal,
		.label = "Camera controls (horizontal)",
		.nChoices = 2,
		.choices = {"Normal", "Reversed"},
	},
	{
		.kind = kCycler,
		.ptr = &gGamePrefs.flipFlightHorizontal,
		.label = "Dragonfly controls (horizontal)",
		.nChoices = 2,
		.choices = {"Normal", "Reversed"},
	},
	{
		.kind = kCycler,
		.ptr = &gGamePrefs.flipFlightVertical,
		.label = "Dragonfly controls (vertical)",
		.nChoices = 2,
		.choices = {"Normal", "Reversed"},
	},

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

static const SettingEntry gVideoMenu[] =
{
	/*
    {
		.kind = kCycler,
		.ptr = &gGamePrefs.fullscreen,
		.label = "Fullscreen",
		.callback = SetFullscreenMode,
		.nChoices = 2,
		.choices = {"No", "Yes"},
	},
    */

	{
		.kind = kCycler,
		.ptr = &gGamePrefs.showBottomBar,
		.label = "Bottom bar",
		.nChoices = 2,
		.choices = {"Hidden", "Visible"},
	},

	{
		.kind = kCycler,
		.ptr = &gGamePrefs.detailLevel,
		.label = "Level of detail",
		.subtitle = GenerateDetailSubtitle,
		.nChoices = 3,
		.choices = {"High", "Low", "High except terrain"},
	},

	{
		.kind = kCycler,
		.ptr = &gGamePrefs.force4x3AspectRatio,
		.label = "Aspect ratio",
		.nChoices = 2,
		.choices = {"Fit to screen", "4:3"},
	},

/*
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
*/

	// End sentinel
	{
		.kind = kEND_SENTINEL,
		.ptr = NULL,
		.label = NULL,
		.callback = NULL,
		.nChoices = 0,
	}
};

// array to hold the grass blade objects, used to sync motion
// 10 is more than the # of settings we have in any screen
ObjNode* grassBladeObjs[10];


/***********************************************/

static unsigned int PositiveModulo(int value, unsigned int m)
{
	int mod = value % (int)m;
	if (mod < 0)
	{
		mod += m;
	}
	return mod;
}

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
    if (gGamePrefs.detailLevel == DETAIL_LEVEL_LOW) 
        return "The \"ATI Rage II\" look";
    else if (gGamePrefs.detailLevel == DETAIL_LEVEL_TERRAIN_LOW)
        return "The middle ground";
    else
        return NULL;
}

static const char* GenerateMSAASubtitle(void)
{
	return gGamePrefs.antialiasingLevel != gAntialiasingLevelAppliedOnBoot
		? "Will apply when you restart the game"
		: NULL;
}

/****************** SETUP SETTINGS SCREEN **************************/

static void MakeSettingEntryObjects(int settingID, bool firstTime)
{
	static const float XSPREAD = 150;
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
		gNewObjectDefinition.moveCall 	= MoveGrass;
		gNewObjectDefinition.rot 		= 0;//PI+PI/2;
		gNewObjectDefinition.scale 		= .02;
		ObjNode* grassBlade = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		grassBlade->Rot.z = PI/2.0f;
		grassBlade->Scale.z = 0.001f;
		UpdateObjectTransforms(grassBlade);

        grassBlade->SpecialL[0] = settingID;

		// Create caption text
		if (entry->kind != kButton)
		{
			tmd.coord.x = -XSPREAD;
			tmd.align = TEXTMESH_ALIGN_LEFT;
            tmd.moveCall = MoveGrass;
			ObjNode* captionText = TextMesh_Create(&tmd, entry->label);
            captionText->SpecialL[0] = settingID;
            captionText->ShadowNode->MoveCall = MoveGrass;
            captionText->ShadowNode->SpecialL[0] = settingID;
            captionText->SpecialPtr[0] = grassBlade; // to sync timer
		}

        grassBladeObjs[settingID] = grassBlade;
	}

	// Create value text
	tmd.color = kValueTextColor;
	tmd.coord.x = pickableX;
	tmd.slot = SLOTID_VALUE_MASK | settingID;
    tmd.moveCall = MoveGrass;
	if (subtitleGenerator)
		tmd.coord.y += LH * 0.15f;

    ObjNode* grassText;
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
				gNewObjectDefinition.moveCall 	= MoveGrass;
				gNewObjectDefinition.rot 		= PI+PI/2;
				gNewObjectDefinition.scale 		= .03;
				ObjNode* clover = MakeNewDisplayGroupObject(&gNewObjectDefinition);
                clover->SpecialL[0] = settingID;
				if (i > *entry->ptr)
					MakeObjectTransparent(clover, 0.25f);
			}
			break;

		case kCycler:
			tmd.slot = SLOTID_VALUE_MASK | settingID;
			tmd.align = TEXTMESH_ALIGN_CENTER;
			grassText = TextMesh_Create(&tmd, entry->choices[*entry->ptr]);
            grassText->SpecialL[0] = settingID;
            grassText->ShadowNode->MoveCall = MoveGrass;
            grassText->ShadowNode->SpecialL[0] = settingID;
			break;

		case kButton:
			tmd.slot = SLOTID_VALUE_MASK | settingID;
			tmd.align = TEXTMESH_ALIGN_CENTER;
			grassText = TextMesh_Create(&tmd, entry->label);
            grassText->SpecialL[0] = settingID;
            grassText->ShadowNode->MoveCall = MoveGrass;
            grassText->ShadowNode->SpecialL[0] = settingID;
			break;
	}

	// Create subtitle text
	tmd.coord.y -= LH * 0.15f * 2.0f;
	if (subtitleGenerator)
	{
		float scaleBackup = tmd.scale;
		tmd.scale *= 0.5f;
		grassText = TextMesh_Create(&tmd, subtitleGenerator);
        grassText->SpecialL[0] = settingID;
        grassText->ShadowNode->MoveCall = MoveGrass;
        grassText->ShadowNode->SpecialL[0] = settingID;
		tmd.scale = scaleBackup;
	}
}

static void SetupSettingsScreen(const char* title, const SettingEntry* menu)
{
	SetupUIStuff(kUIBackground_Cyclorama);

    // set initial selection
    if ((menu[0].kind != kButton)) {
        gHoveredPick = PICKID_SETTING_CYCLER | 0;
    } else {
        gHoveredPick = PICKID_SETTING_BUTTON | 0;
    }

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

void DoSettingsScreen(void)
{
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

		if (GetNewKeyState(kKey_UI_Cancel) || GetNewKeyState(kKey_UI_Start))
		{
			done = true;
			continue;
		}


        // get current menu # of items
        int menuLength = 0;
        for (menuLength = 0; menuLength < 10; menuLength++) { // 10 is an arbitrary max of possible items
            if (gCurrentMenu[menuLength].kind == kEND_SENTINEL)
                break;
        }

        int curPick = 0;
        if (gHoveredPick == PICKID_SPIDER) {
            if (GetNewKeyState(kKey_Forward)) // up
            {
                curPick = menuLength - 1;
                if ((gCurrentMenu[curPick].kind != kButton)) {
                    gHoveredPick = PICKID_SETTING_CYCLER | curPick;
                } else {
                    gHoveredPick = PICKID_SETTING_BUTTON | curPick;
                }
            }
        } else {
            curPick = gHoveredPick & 0xFFFF;
            if (GetNewKeyState(kKey_Forward) && (curPick > 0)) // up
            {
                curPick -= 1;
                if ((gCurrentMenu[curPick].kind != kButton)) {
                    gHoveredPick = PICKID_SETTING_CYCLER | curPick;
                } else {
                    gHoveredPick = PICKID_SETTING_BUTTON | curPick;
                }
            }
            else if (GetNewKeyState(kKey_Backward)) // down
            {
                if  (curPick < menuLength - 1) {
                    curPick += 1;
                    if ((gCurrentMenu[curPick].kind != kButton)) {
                        gHoveredPick = PICKID_SETTING_CYCLER | curPick;
                    } else {
                        gHoveredPick = PICKID_SETTING_BUTTON | curPick;
                    }
                } else {
                    gHoveredPick = PICKID_SPIDER;
                }
            }
        }

		// See if user hovered/clicked on a pickable item
		int pickID = gHoveredPick;
		if (pickID >= 0 && GetNewKeyState(kKey_UI_PadConfirm))
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

/****************** SETTINGS ITEM MOTION **************************/

static float UpdateGrassState(ObjNode* theNode, long currentStateID)
{
	long*	stateID		= &theNode->SpecialL[5];

    ObjNode* grassBlade = grassBladeObjs[theNode->SpecialL[0]];
	float*	stateTimer;
    if (grassBlade)
        stateTimer = &grassBlade->SpecialF[5];
    else
    {
        stateTimer = &theNode->SpecialF[5];
    }

	if (*stateID != currentStateID)
	{
		*stateID = currentStateID;
        *stateTimer = 0;
	}
	else
	{
        if ( (theNode == grassBlade) || (!grassBlade) )
            *stateTimer += gFramesPerSecondFrac;
	}

	return *stateTimer;
}

static void MoveGrass_Wave(ObjNode* theNode)
{
	const float t = UpdateGrassState(theNode, 'WAVE');
	const float D = 0.5f;

	TweenTowardsTQ3Point3D(t, D, &theNode->Coord, (TQ3Point3D){
		theNode->InitCoord.x,
		theNode->InitCoord.y,
		theNode->InitCoord.z - (1 + cosf(5*t)) * 7
	});
}

static void MoveGrass_Neutral(ObjNode* theNode)
{
	const float t = UpdateGrassState(theNode, 'NTRL');
	const float D = 0.5f;
	TweenTowardsTQ3Point3D(t, D, &theNode->Coord, theNode->InitCoord);
}


static void MoveGrass(ObjNode *theNode)
{
    if (((gHoveredPick & 0xFFFF) == theNode->SpecialL[0]) && (gHoveredPick != PICKID_SPIDER)) {
        MoveGrass_Wave(theNode);
    } else {
        MoveGrass_Neutral(theNode);
    }

	UpdateObjectTransforms(theNode);
}
