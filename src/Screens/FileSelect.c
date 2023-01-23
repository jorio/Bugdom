// FILE SELECT.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom


/*
 * The original game used to pop up a standard file open/save dialog
 * to manage save games. For portability's sake, the source port
 * implements a rudimentary in-game save file picker instead.
 */


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include <time.h>
#include <ctype.h>
#include <stdio.h>


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupFileScreen(void);
static int FileScreenMainLoop(void);


/****************************/
/*    CONSTANTS             */
/****************************/

const char* kLevelNames[NUM_LEVELS] =
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


static const TQ3ColorRGBA gTextShadowColor	= {0.0f, 0.0f, 0.3f, 1.0f};
static const TQ3ColorRGBA gTextColor		= {1.0f, 0.9f, 0.0f, 1.0f};
static const TQ3ColorRGBA gTitleTextColor	= {1.0f, 0.9f, 0.0f, 1.0f};
static const TQ3ColorRGBA gDeleteColor		= {0.9f, 0.3f, 0.1f, 1.0f};


static char textBuffer[512];


enum
{
	kPickBits_FileNumberMask	= 0x0000FFFF,
	kPickBits_Floppy			= 0x00010000,
	kPickBits_Delete			= 0x00020000,
	kPickBits_DontSave			= 0x00040000,
	kPickBits_Spider			= 0x00080000,
};


static ObjNode* floppies[3];




/*********************/
/*    VARIABLES      */
/*********************/


static int 			gCurrentFileScreenType = FILE_SELECT_SCREEN_TYPE_LOAD;

static bool			gFileScreenAwaitingInput = true;


/********************** DO BONUS SCREEN *************************/

int DoFileSelectScreen(int type)
{
	gCurrentFileScreenType = type;

	/*********/
	/* SETUP */
	/*********/

	SetupUIStuff(kUIBackground_Cyclorama);
	SetupFileScreen();

	QD3D_CalcFramesPerSecond();

	/**************/
	/* PROCESS IT */
	/**************/

	int picked = FileScreenMainLoop();

	/* CLEANUP */

	CleanupUIStuff();

	return picked;
}


/****************** SETUP FILE SCREEN **************************/

static float UpdateFloppyState(ObjNode* theNode, long currentStateID)
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
			EaseOutExpo, t, D,
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
	const float D = .5f;

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
	const float y = gFloppyPositions[fileNumber].y +
			(gCurrentFileScreenType == FILE_SELECT_SCREEN_TYPE_LOAD? 15: 0);

	const float gs = .9f;			// global scale of objects created in this function
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

	// Set floppy label texture
	GLuint labelTexture = QD3D_LoadTextureFile(3510 + (saveDataValid? saveData.realLevel: 0), 0);
	newFloppy->MeshList[1]->glTextureName = labelTexture;
	newFloppy->OwnsMeshTexture[1] = true;

	snprintf(textBuffer, sizeof(textBuffer), "File %c", 'A' + fileNumber);


	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);
	tmd.align			= TEXTMESH_ALIGN_CENTER;
	tmd.color			= gTextColor;
	tmd.shadowColor		= gTextShadowColor;
	tmd.slot			= objNodeSlotID;
	tmd.coord.x			= x;
	tmd.coord.y			= y+90 * gs;
	tmd.scale			= 0.6f * gs;
	TextMesh_Create(&tmd, textBuffer);


	if (saveDataValid)
	{
		snprintf(textBuffer, sizeof(textBuffer), "Level %d: %s", 1 + saveData.realLevel, kLevelNames[saveData.realLevel]);

		tmd.coord.y	= y+70*gs;
		tmd.scale	= .2f * gs;
		TextMesh_Create(&tmd, textBuffer);

		time_t timestamp = saveData.timestamp;
		struct tm tm;
		tm = *localtime(&timestamp);
		strftime(textBuffer, sizeof(textBuffer), "%e %b %Y  %H:%M", &tm);
		tmd.coord.y	-= 10*gs;
		TextMesh_Create(&tmd, textBuffer);

		if (canDelete)
		{
			tmd.color		= gDeleteColor;
			tmd.coord.x		= x+30 * gs;
			tmd.coord.y		= y-65 * gs;
			tmd.scale		= deleteScale * gs;
			TextMesh_Create(&tmd, "delete");
		}
	}


	if (createPickables)
	{
		// Floppy
		{
			TQ3Point3D quadCenter = {x, y, 0};
			NewPickableQuad(quadCenter, 90, 90, kPickBits_Floppy | fileNumber);
		}

		// Delete
		if (canDelete)
		{
			TQ3Point3D quadCenter = { x+30*gs, y-65*gs, 0 };
			NewPickableQuad(quadCenter, .3f*100*gs, deleteScale*50*gs, kPickBits_Delete | fileNumber);
		}
	}
}


/**************** MOVE BONUS BACKGROUND *********************/

static void SetupFileScreen(void)
{
	/* TEXT */

	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);
	tmd.align		= TEXTMESH_ALIGN_CENTER;
	tmd.coord.y		= 110.0f;
	tmd.color		= gTitleTextColor;
	tmd.shadowColor	= gTextShadowColor;

	if (gCurrentFileScreenType == FILE_SELECT_SCREEN_TYPE_LOAD)
	{
		MakeSpiderButton((TQ3Point3D) {-140, -100, 0}, "BACK", kPickBits_Spider);
	}
	else
	{
		snprintf(textBuffer, sizeof(textBuffer), "Entering level %d. Save where?", gRealLevel+2);
		tmd.scale	= .33f;
		tmd.color = gTitleTextColor;
		TextMesh_Create(&tmd, textBuffer);

		tmd.coord.x = 140;
		tmd.coord.y = -120;
		tmd.scale = .33f;
		tmd.color = gDeleteColor;
		tmd.align = TEXTMESH_ALIGN_RIGHT;
		TextMesh_Create(&tmd, "Don\222t save");

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
		NewPickableQuad(dontSaveQuadCenter, 100, 25, kPickBits_DontSave);
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
	QD3D_ExplodeGeometry(floppies[fileNumber], 200.0f, SHARD_MODE_UPTHRUST | SHARD_MODE_NULLSHADER, 1, 0.5f);
	PlayEffect_Parms3D(EFFECT_POP, &floppies[fileNumber]->Coord, kMiddleC - 6, 3.0);
	floppies[fileNumber]->StatusBits |= STATUS_BIT_HIDDEN;

	NukeObjectsInSlot(10000 + fileNumber);
}


static void DeleteFileInSlot(int fileNumber)
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
	QD3D_DrawShards(setupInfo);
}

static int FileScreenMainLoop()
{
	gFileScreenAwaitingInput = true;
	int finalPick = -1;
	float transitionAwayTime = 0;

	while (1)
	{
		UpdateInput();
		MoveObjects();
		QD3D_MoveShards();
		QD3D_DrawScene(gGameViewInfoPtr, FileScreenDrawStuff);
		QD3D_CalcFramesPerSecond();
		DoSDLMaintenance();

		// See if user hovered/clicked on a pickable item

		if (gFileScreenAwaitingInput)
		{
			if (GetNewKeyState(kKey_UI_Cancel)
				|| GetNewKeyState(kKey_UI_PadBack)
				|| GetNewKeyState(kKey_UI_PadCancel))
			{
				return -1;
			}

			int pickID = UpdateHoveredPickID();
			if (pickID >= 0 && IsAnalogCursorClicked())
			{
				switch (pickID & ~kPickBits_FileNumberMask)
				{
					case kPickBits_Floppy:
						finalPick = pickID & kPickBits_FileNumberMask;
						gFileScreenAwaitingInput = false;
						transitionAwayTime = 1.0f;

						if (gCurrentFileScreenType == FILE_SELECT_SCREEN_TYPE_SAVE)
						{
							PopFloppy(finalPick);
						}
						break;

					case kPickBits_Delete:
						DeleteFileInSlot(pickID & kPickBits_FileNumberMask);
						break;

					case kPickBits_DontSave:
					case kPickBits_Spider:
						return -1;

					default:
						DoAlert("FileSelect: Unsupported pickID 0x%08x", pickID);
				}
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
