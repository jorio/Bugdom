/****************************/
/* FILE SELECT              */
/* Source port addition     */
/****************************/

/*
 * The original game used to pop up a standard file open/save dialog
 * to manage save games. For portability's sake, the source port
 * implements a rudimentary in-game save file picker instead.
 */


/****************************/
/*    EXTERNALS             */
/****************************/

#include <time.h>

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	WindowPtr			gCoverWindow;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	FSSpec		gDataSpec;
extern	PrefsType	gGamePrefs;
extern	u_short		gLevelType,gRealLevel;
extern 	ObjNode 	*gFirstNodePtr;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupFileScreen(void);
static int FileScreenMainLoop(void);


/****************************/
/*    CONSTANTS             */
/****************************/

static const char* gLevelNames[NUM_LEVELS] =
{
	"Training",
	"Lawn",
	"Pond",
	"Forest",
	"Hive Attack",
	"Bee Hive",
	"Queen Bee",
	"Night Attack",
	"Ant Hill",
	"Ant King",
};

static const TQ3Point2D gFloppyPositions[NUM_SAVE_FILES] =
{
	{ -120.0f, -10.0f },
	{    0.0f, -10.0f },
	{  120.0f, -10.0f },
};

struct
{
	bool		isSaveDataValid;
} fileInfos[NUM_SAVE_FILES];


static const TQ3ColorRGB gTextShadowColor	= { 0.0f, 0.0f, 0.3f };
static const TQ3ColorRGB gTextColor			= { 1.0f, 0.9f, 0.0f };
static const TQ3ColorRGB gTitleTextColor	= { 1.0f, 0.9f, 0.0f };
static const TQ3ColorRGB gDeleteColor		= { 0.9f, 0.3f, 0.1f };
static const TQ3ColorRGB gBackColor			= { 1,1,1 };


static char textBuffer[512];


enum
{
	kPickBits_FileNumberMask	= 0x0000FFFF,
	kPickBits_Floppy			= 0x00010000,
	kPickBits_Delete			= 0x00020000,
	kPickBits_DontSave			= 0x00040000,
};


static ObjNode* floppies[3];




/*********************/
/*    VARIABLES      */
/*********************/


static int 			gCurrentFileScreenType = FILE_SELECT_SCREEN_TYPE_LOAD;

static bool			gFileScreenAwaitingInput = true;

static int			gHoveredPick = -1;

static TQ3Point3D	gBonusCameraFrom = {0, 0, 250 };


/********************** DO BONUS SCREEN *************************/

int DoFileSelectScreen(int type)
{
	gCurrentFileScreenType = type;

	/*********/
	/* SETUP */
	/*********/

	InitCursor();

	SetupFileScreen();

	QD3D_CalcFramesPerSecond();

	/**************/
	/* PROCESS IT */
	/**************/

	int picked = FileScreenMainLoop();

	/* CLEANUP */

	PickableQuads_DisposeAll();

	GammaFadeOut();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DeleteAll3DMFGroups();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);
	DisposeSoundBank(SOUND_BANK_DEFAULT);
	GameScreenToBlack();

	return picked;
}


/****************** SETUP FILE SCREEN **************************/

static float UpdateFloppyState(ObjNode* theNode, uint32_t currentStateID)
{
	long*	stateID		= &theNode->SpecialL[5];
	float*	stateTimer	= &theNode->SpecialF[5];

	if (*stateID != currentStateID)
	{
		*stateID = currentStateID;
		*stateTimer = 0;
	}
	else
	{
		*stateTimer += gFramesPerSecondFrac;
	}

	return *stateTimer;
}

static void MoveFloppy_Bounce(ObjNode* theNode)
{
	const float t = UpdateFloppyState(theNode, 'BNCE');
	const float D = 0.5f;

	TweenTowardsTQ3Vector3D(t, D, &theNode->Rot, (TQ3Vector3D){
		0,
		cosf(2.5f*t) * 0.53f,
		0
	});

	TweenTowardsTQ3Point3D(t, D, &theNode->Coord, (TQ3Point3D){
		theNode->InitCoord.x,
		theNode->InitCoord.y + fabsf(cosf(5*t)) * 7,
		theNode->InitCoord.z
	});
}

static void MoveFloppy_Shiver(ObjNode* theNode)
{
	const float t = UpdateFloppyState(theNode, 'SHVR');
	const float D = 0.5f;
	TweenTowardsTQ3Vector3D(t, D, &theNode->Rot, (TQ3Vector3D){
		0,
		0,
		cosf(1*9*t) * 0.1f * sinf(3*9*t)
	});
}

static void MoveFloppy_Neutral(ObjNode* theNode)
{
	const float t = UpdateFloppyState(theNode, 'NTRL');
	const float D = 0.5f;
	TweenTowardsTQ3Vector3D(t, D, &theNode->Rot, (TQ3Vector3D){0,0,0});
	TweenTowardsTQ3Point3D(t, D, &theNode->Coord, theNode->InitCoord);
}

static void MoveFloppy_Appear(ObjNode* theNode)
{
	const float t = UpdateFloppyState(theNode, 'APPR');
	const float D = 0.25f;
	theNode->Coord = TweenTQ3Point3D(
			EaseOutQuad, t, D,
			(TQ3Point3D){
				theNode->InitCoord.x,
				theNode->InitCoord.y,
				theNode->InitCoord.z - 500
				},
			theNode->InitCoord);
}


static void MoveFloppy_Loading(ObjNode* theNode)
{
	const float t = UpdateFloppyState(theNode, 'LDNG');
	const float D = 1.0f;

	TweenTowardsTQ3Vector3D(t, D/2.0f, &theNode->Rot, (TQ3Vector3D){0,0,0});

	theNode->Coord = TweenTQ3Point3D(
			EaseOutExpo, t, D,
			theNode->InitCoord,
			(TQ3Point3D){
					0,
					-16,
					200
			});
}


static void MoveFloppy(ObjNode *theNode)
{
	int fileNumber = theNode->SpecialL[0];

	float* age = &theNode->SpecialF[0];

	bool isPickingMe = !(gHoveredPick & kPickBits_DontSave)
					   && (gHoveredPick & kPickBits_FileNumberMask) == fileNumber;

	if (!gFileScreenAwaitingInput
		&& gCurrentFileScreenType == FILE_SELECT_SCREEN_TYPE_LOAD
		&& isPickingMe)
	{
		MoveFloppy_Loading(theNode);
	}
	else if (*age < 0.5f)
	{
		MoveFloppy_Appear(theNode);
	}
	else if (isPickingMe)
	{
		if (gCurrentFileScreenType == FILE_SELECT_SCREEN_TYPE_LOAD)
		{
			if (gHoveredPick & kPickBits_Floppy)
				MoveFloppy_Bounce(theNode);
			else if (gHoveredPick & kPickBits_Delete && fileInfos[fileNumber].isSaveDataValid)
				MoveFloppy_Shiver(theNode);
			else
				MoveFloppy_Neutral(theNode);
		}
		else
		{
			MoveFloppy_Shiver(theNode);
		}
	}
	else
	{
		MoveFloppy_Neutral(theNode);
	}

	*age += gFramesPerSecondFrac;

	UpdateObjectTransforms(theNode);
}

static void MakeFileObjects(const int fileNumber, bool createPickables)
{
	bool canDelete = gCurrentFileScreenType == FILE_SELECT_SCREEN_TYPE_LOAD;

	GAME_ASSERT(fileNumber >= 0 && fileNumber < NUM_SAVE_FILES);

	SaveGameType saveData;
	OSErr saveDataErr = GetSaveGameData(fileNumber, &saveData);
	bool saveDataValid = saveDataErr == noErr;

	const float x = gFloppyPositions[fileNumber].x;
	const float y = gFloppyPositions[fileNumber].y;

	const float gs = 1.0f;			// global scale of objects created in this function
	const float deleteScale = 0.33f;

	fileInfos[fileNumber].isSaveDataValid = saveDataValid;

	short objNodeSlotID = 10000 + fileNumber;				// all nodes related to this file slot will have this

	// ----------------------
	// Make floppy
	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
	gNewObjectDefinition.type 		= BONUS_MObjType_SaveIcon;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.slot 		= objNodeSlotID;
	gNewObjectDefinition.flags 		= STATUS_BIT_CLONE;
	gNewObjectDefinition.moveCall 	= MoveFloppy;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.0f * gs;
	ObjNode* newFloppy = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newFloppy->SpecialL[0] = fileNumber;

	floppies[fileNumber] = newFloppy;

	// Get path to floppy label image
	if (saveDataValid)
		snprintf(textBuffer, sizeof(textBuffer), ":Floppy:%d.tga", saveData.realLevel);
	else
		snprintf(textBuffer, sizeof(textBuffer), ":Floppy:NewGame.tga");

	// Set floppy label texture
	FSSpec floppyLabelPictSpec;
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, textBuffer, &floppyLabelPictSpec);
	TQ3ShaderObject shaderObject = QD3D_TGAToTexture(&floppyLabelPictSpec);
	GAME_ASSERT(shaderObject);
	QD3D_ReplaceGeometryTexture(newFloppy->BaseGroup, shaderObject);

//	gPickables[gNumPickables++] = newFloppy->BaseGroup;

	snprintf(textBuffer, sizeof(textBuffer), "File %c", 'A' + fileNumber);


	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);
	tmd.color			= gTextColor;
	tmd.shadowColor		= gTextShadowColor;
	tmd.slot			= objNodeSlotID;
	tmd.coord.x			= x;
	tmd.coord.y			= y+80 * gs;
	tmd.scale			= 0.6f * gs;
	TextMesh_Create(&tmd, textBuffer);

	if (saveDataValid)
	{
		snprintf(textBuffer, sizeof(textBuffer), "Level %d", 1 + saveData.realLevel);

		tmd.coord.y	= y-70*gs;
		tmd.scale	= .35f * gs;
		TextMesh_Create(&tmd, textBuffer);

		tmd.coord.y	-= 10*gs;
		tmd.scale	= .25f * gs;
		TextMesh_Create(&tmd, gLevelNames[saveData.realLevel]);

		time_t timestamp = saveData.timestamp;
		struct tm tm;
		tm = *localtime(&timestamp);
		strftime(textBuffer, sizeof(textBuffer), "%-e %b %Y   %-l%:%M%p", &tm);
		for (char *c = textBuffer; *c; c++)
			*c = tolower(*c);
		tmd.coord.y	= y+65*gs;
		TextMesh_Create(&tmd, textBuffer);

		if (canDelete)
		{
			tmd.color		= gDeleteColor;
			tmd.coord.x		= x+30 * gs;
			tmd.coord.y		= y+100 * gs;
			tmd.scale		= deleteScale * gs;
			TextMesh_Create(&tmd, "delete");
		}
	}


	if (createPickables)
	{
		{
			TQ3Point3D quadCenter = {x, y, 0};
			PickableQuads_NewQuad(quadCenter, 90, 90, kPickBits_Floppy | fileNumber);            // Floppy
		}

		if (canDelete)
		{
			TQ3Point3D quadCenter = { x+30*gs, y+100*gs, 0 };
			PickableQuads_NewQuad(quadCenter, .3f*100*gs, deleteScale*50*gs, kPickBits_Delete | fileNumber);		// Delete
		}
	}
}


/**************** MOVE BONUS BACKGROUND *********************/

static void MoveBonusBackground(ObjNode *theNode)
{
	float	fps = gFramesPerSecondFrac;

	theNode->Rot.y += fps * .03f;
	UpdateObjectTransforms(theNode);
}




static void SetupFileScreen(void)
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

	QD3D_InitParticles();

	/*******************/
	/* MAKE BACKGROUND */
	/*******************/

	/* BACKGROUND */

	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
	gNewObjectDefinition.type 		= BONUS_MObjType_Background;
	gNewObjectDefinition.coord		= gBonusCameraFrom;
	gNewObjectDefinition.slot 		= 999;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_NULLSHADER;
	gNewObjectDefinition.moveCall 	= MoveBonusBackground;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 30.0;
	MakeNewDisplayGroupObject(&gNewObjectDefinition);

	/* TEXT */

	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);
	tmd.coord.y		= 110.0f;
	tmd.color		= gTitleTextColor;
	tmd.shadowColor	= gTextShadowColor;

	if (gCurrentFileScreenType == FILE_SELECT_SCREEN_TYPE_LOAD)
	{
//		MakeTextWithShadow("Pick a Saved Game", 0.0f, y, 1.0f, &gTitleTextColor);

//		MakeTextWithShadow("BACK", 50.0f, -100, 0.5f, &gBackColor);
	}
	else
	{
		//TextMesh_Create(&tmd, "Save where?");

		snprintf(textBuffer, sizeof(textBuffer), "ENTERING LEVEL %d. SAVE WHERE?", gRealLevel+2);
		tmd.scale	= .33f;
		tmd.color = gTitleTextColor;
		TextMesh_Create(&tmd, textBuffer);

		tmd.coord.x = 140;
		tmd.coord.y = -120;
		tmd.scale = .33f;
		tmd.color = gDeleteColor;
		tmd.align = TEXTMESH_ALIGN_RIGHT;
		TextMesh_Create(&tmd, "DON'T SAVE");

		// Make floppy
		gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
		gNewObjectDefinition.type 		= BONUS_MObjType_DontSaveIcon;
		gNewObjectDefinition.coord.x 	= tmd.coord.x + 20;
		gNewObjectDefinition.coord.y 	= tmd.coord.y;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.flags 		= STATUS_BIT_CLONE;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= 0.3f;
		MakeNewDisplayGroupObject(&gNewObjectDefinition);

		// Make pickable quad
		TQ3Point3D dontSaveQuadCenter = tmd.coord;
		dontSaveQuadCenter.x -= 15;
		PickableQuads_NewQuad(dontSaveQuadCenter, 100, 25, kPickBits_DontSave);
	}

	for (int i = 0; i < NUM_SAVE_FILES; i++)
	{
		MakeFileObjects(i, true);
	}

	//---------------------------------------------------------------

	MakeFadeEvent(true);
}


static void PopFloppy(int fileNumber)
{
	QD3D_ExplodeGeometry(floppies[fileNumber], 200.0f, PARTICLE_MODE_UPTHRUST | PARTICLE_MODE_NULLSHADER, 1, 0.5f);
	PlayEffect_Parms3D(EFFECT_POP, &floppies[fileNumber]->Coord, kMiddleC - 6, 3.0);
	floppies[fileNumber]->StatusBits |= STATUS_BIT_HIDDEN;

	ObjNode* node = gFirstNodePtr;
	while (node)
	{
		ObjNode* nextNode = node->NextNode;
		if (node->Slot == 10000 + fileNumber)
		{
			DeleteObject(node);
		}
		node = nextNode;
	}
}


static void DeleteFile(int fileNumber)
{
	GAME_ASSERT(fileNumber >= 0 && fileNumber < NUM_SAVE_FILES);

	if (!fileInfos[fileNumber].isSaveDataValid)
		return;

	PopFloppy(fileNumber);

	DeleteSavedGame(fileNumber);

	MakeFileObjects(fileNumber, false);
}




/******************* DRAW BONUS STUFF ********************/

static void FileScreenDrawStuff(const QD3DSetupOutputType *setupInfo)
{
	DrawObjects(setupInfo);
	QD3D_DrawParticles(setupInfo);
#if _DEBUG
	PickableQuads_Draw(setupInfo->viewObject);
#endif
}

enum
{
	FILE_SCREEN_STATE_AWAITING_INPUT,
	FILE_SCREEN_STATE_TRANSITIONING_AWAY
};

static int FileScreenMainLoop()
{
	gFileScreenAwaitingInput = true;
	int finalPick = -1;
	float transitionAwayTime = 0;

	while (1)
	{
		UpdateInput();
		MoveObjects();
		QD3D_MoveParticles();
		QD3D_DrawScene(gGameViewInfoPtr, FileScreenDrawStuff);
		QD3D_CalcFramesPerSecond();
		DoSDLMaintenance();

		// See if user hovered/clicked on a pickable item

		if (gFileScreenAwaitingInput)
		{
			gHoveredPick = -1;

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
					switch (gHoveredPick & ~kPickBits_FileNumberMask)
					{
						case kPickBits_Floppy:
							finalPick = gHoveredPick & kPickBits_FileNumberMask;
							gFileScreenAwaitingInput = false;
							transitionAwayTime = 1.0f;

							if (gCurrentFileScreenType == FILE_SELECT_SCREEN_TYPE_SAVE)
							{
								PopFloppy(finalPick);
							}
							break;

						case kPickBits_Delete:
							DeleteFile(gHoveredPick & kPickBits_FileNumberMask);
							break;

						case kPickBits_DontSave:
							return -1;

						default:
							ShowSystemErr_NonFatal(gHoveredPick);
					}
				}
			}

			if (GetNewKeyState(kVK_Escape))
			{
				return -1;
			}
		}
		else
		{
			transitionAwayTime -= gFramesPerSecondFrac;
			if (transitionAwayTime <= 0)
				return finalPick;
		}
	}
}
