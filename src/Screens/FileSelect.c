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

static void SetupFileScreen(int type);
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

static int gHoveredPick = -1;

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
};


static ObjNode* floppies[3];




/*********************/
/*    VARIABLES      */
/*********************/


static TQ3Point3D	gBonusCameraFrom = {0, 0, 250 };


/********************** DO BONUS SCREEN *************************/

int DoFileSelectScreen(int type)
{
	/*********/
	/* SETUP */
	/*********/

	InitCursor();

	SetupFileScreen(type);

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

static void MoveFloppy(ObjNode *theNode)
{
	float	fps = gFramesPerSecondFrac;

	int fileNumber = theNode->SpecialL[0];

	if (gHoveredPick == (kPickBits_Floppy | fileNumber))
	{
		theNode->SpecialF[0] += fps * 5.0f;
		theNode->Rot.y = cosf(0.5f * theNode->SpecialF[0]) * 0.53f;
		theNode->Rot.z = 0;
		theNode->Coord.y = theNode->InitCoord.y + fabsf(cosf(1.0f*theNode->SpecialF[0])) * 7.0f;
	}
	else if (gHoveredPick == (kPickBits_Delete | fileNumber) && fileInfos[fileNumber].isSaveDataValid)
	{
		theNode->SpecialF[0] += fps * 9.0f;
		float t = theNode->SpecialF[0];
		theNode->Rot.y = 0;
		theNode->Rot.z = cosf(1.2f * t) * 0.1f * sinf(3.0f * t);
	}
	else
	{
		theNode->Rot.y = 0;
		theNode->Rot.z = 0;
		theNode->SpecialF[0] = 0;
		theNode->Coord.y = theNode->InitCoord.y;
	}

	UpdateObjectTransforms(theNode);
}

static void MakeFileObjects(const int fileNumber, bool createPickables)
{
	GAME_ASSERT(fileNumber < NUM_SAVE_FILES);

	SaveGameType saveData;
	OSErr saveDataErr = GetSaveGameData(fileNumber, &saveData);
	bool saveDataValid = saveDataErr == noErr;

	float x = gFloppyPositions[fileNumber].x;
	float y = gFloppyPositions[fileNumber].y;

	fileInfos[fileNumber].isSaveDataValid = saveDataValid;

	short objNodeSlotID = 10000 + fileNumber;				// all nodes related to this file slot will have this

	const float gs = 0.8f;

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

	tmd.coord.x	= x;
	tmd.coord.y	= y+80*gs;
	tmd.scale	= 0.6f * gs;
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

		tmd.color		= gDeleteColor;
		tmd.coord.x	= x+30*gs;
		tmd.coord.y	= y+90*gs;
		tmd.scale	= .3f * gs;
		TextMesh_Create(&tmd, "delete");
	}



	if (createPickables)
	{
		TQ3Point3D quadCenter = {x, y, 0};
		PickableQuads_NewQuad(quadCenter, 85, 85, kPickBits_Floppy | fileNumber);			// Floppy

		quadCenter.x += 30;
		quadCenter.y += 90*gs;
		PickableQuads_NewQuad(quadCenter, 20, 5, kPickBits_Delete | fileNumber);			// Delete
	}
}


/**************** MOVE BONUS BACKGROUND *********************/

static void MoveBonusBackground(ObjNode *theNode)
{
	float	fps = gFramesPerSecondFrac;

	theNode->Rot.y += fps * .03f;
	UpdateObjectTransforms(theNode);
}




static void SetupFileScreen(int type)
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

	float y = 110.0f;

	if (type == FILE_SELECT_SCREEN_TYPE_LOAD)
	{
//		MakeTextWithShadow("Pick a Saved Game", 0.0f, y, 1.0f, &gTitleTextColor);

//		MakeTextWithShadow("BACK", 50.0f, -100, 0.5f, &gBackColor);
	}
	else
	{
//		MakeTextWithShadow("Save where?", 0.0f, y, 1.0f, &gTitleTextColor);

		snprintf(textBuffer, sizeof(textBuffer), "You are entering Level %d", gRealLevel+2, true);
//		MakeTextWithShadow(textBuffer, 0, y - 25.0f, 0.5f, &gTitleTextColor);
	}

	for (int i = 0; i < NUM_SAVE_FILES; i++)
	{
		MakeFileObjects(i, true);
	}


	y -= 50.0f;
	//MakeFloppy(floppyx, y-16.0f);
//	MakeText("Continue without saving", x, y, 0.5f); y -= 16.0f;

	//---------------------------------------------------------------

	MakeFadeEvent(true);
}



static void DeleteFile(int fileNumber)
{
	if (!fileInfos[fileNumber].isSaveDataValid)
		return;


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

	DeleteSavedGame(fileNumber);

	MakeFileObjects(fileNumber, false);
}




/******************* DRAW BONUS STUFF ********************/

static void FileScreenDrawStuff(const QD3DSetupOutputType *setupInfo)
{
	DrawObjects(setupInfo);
	QD3D_DrawParticles(setupInfo);
}

static int FileScreenMainLoop()
{
	while (1)
	{
		UpdateInput();
		MoveObjects();
		QD3D_MoveParticles();
		QD3D_DrawScene(gGameViewInfoPtr, FileScreenDrawStuff);
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
					if (gHoveredPick & kPickBits_Floppy)
					{
						return gHoveredPick & kPickBits_FileNumberMask;
					}
					else if (gHoveredPick & kPickBits_Delete)
					{
						DeleteFile(gHoveredPick & kPickBits_FileNumberMask);
					}
					else
					{
						ShowSystemErr_NonFatal(gHoveredPick);
					}
				}
			}
		}

		if (GetNewKeyState(kVK_Escape))
		{
			return -1;
		}
	}
}
