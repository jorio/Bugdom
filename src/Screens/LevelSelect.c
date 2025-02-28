// LEVEL SELECT.C
// (C) 2021 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include <stdio.h>


/****************************/
/*    PROTOTYPES            */
/****************************/

static void LevelSelectDrawStuff(const QD3DSetupOutputType *setupInfo);
static void MakeLevelSelectObjects(void);
static void MoveText(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

ObjNode* gLevelScreenshotNode = NULL;


/********************** DO LEVEL SELECT SCREEN *************************/

bool DoLevelSelect(void)
{
	bool proceed = false;

	/*********/
	/* SETUP */
	/*********/

	SetupUIStuff(kUIBackground_White);
	QD3D_CalcFramesPerSecond();

    gHoveredPick = 0;

	GLuint levelScreenshots[NUM_LEVELS];
	for (int i = 0; i < NUM_LEVELS; i++)
	{
		levelScreenshots[i] = QD3D_LoadTextureFile(
				3510+i, kRendererTextureFlags_ClampBoth | kRendererTextureFlags_SolidBlackIsAlpha);
	}


	/**************/
	/* PROCESS IT */
	/**************/

	MakeFadeEvent(true);

	MakeLevelSelectObjects();

	gLevelScreenshotNode->MeshList[0]->glTextureName = levelScreenshots[0];

	FlushMouseButtonPress();

	while (1)
	{
		UpdateInput();
		MoveObjects();
		QD3D_MoveShards();
		QD3D_DrawScene(gGameViewInfoPtr, LevelSelectDrawStuff);
		QD3D_CalcFramesPerSecond();
		DoSDLMaintenance();

		bool button = IsAnalogCursorClicked() || GetNewKeyState(kKey_UI_PadConfirm);

		if (!button && GetSkipScreenInput())
		{
			proceed = false;
			break;
		}

        if (GetNewKeyState(kKey_Forward) && (gHoveredPick > 0)) // up
            gHoveredPick -= 1;
        else if (GetNewKeyState(kKey_Backward) && (gHoveredPick < NUM_LEVELS-1)) // down
            gHoveredPick += 1;

		//UpdateHoveredPickID();
		if (gHoveredPick >= 0)
		{
			GAME_ASSERT(gHoveredPick < NUM_LEVELS);
			gLevelScreenshotNode->MeshList[0]->glTextureName = levelScreenshots[gHoveredPick];

			if (button)
			{
				gRealLevel = gHoveredPick;
				proceed = true;
				break;
			}
		}
	}

	/* CLEANUP */

	CleanupUIStuff();

	glDeleteTextures(NUM_LEVELS, levelScreenshots);

	return proceed;
}

/****************** SETUP LEVEL SELECT SCREEN **************************/

static void MakeLevelSelectObjects(void)
{
	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);
	tmd.align = TEXTMESH_ALIGN_CENTER;
	tmd.slot = 0;
	tmd.coord.y += 110;
	tmd.withShadow = false;
	tmd.color = TQ3ColorRGBA_FromInt(0x000000FF);

	const float LH = 18;

	TextMesh_Create(&tmd, "Level Select");

	tmd.align = TEXTMESH_ALIGN_LEFT;
	tmd.scale = .25f;
	tmd.coord.y -= 20;
	tmd.coord.x = -140;
	tmd.color = TQ3ColorRGBA_FromInt(0x0599f8ff);
	for (int i = 0; i < NUM_LEVELS; i++)
	{
		char caption[128];
		snprintf(caption, sizeof(caption), "Level %d: %s", i+1, kLevelNames[i]);

		tmd.coord.y -= LH;
		ObjNode* levelText = TextMesh_Create(&tmd, caption);
        levelText->SpecialL[0] = i;
        levelText->MoveCall = MoveText;

		TQ3Point3D quadCenter = tmd.coord;
		quadCenter.x += 50;
		NewPickableQuad(quadCenter, 100, LH*.8f, i);
	}

	gNewObjectDefinition.genre		= DISPLAY_GROUP_GENRE;
	gNewObjectDefinition.group 		= MODEL_GROUP_ILLEGAL;
	gNewObjectDefinition.coord.x = 75;
	gNewObjectDefinition.coord.y = 20;
	gNewObjectDefinition.coord.z = -5;
	gNewObjectDefinition.flags 	= STATUS_BIT_NULLSHADER|STATUS_BIT_NOZWRITE;
	gNewObjectDefinition.slot 	= 0;
	gNewObjectDefinition.moveCall = nil;
	gNewObjectDefinition.rot 	= 0;
	gNewObjectDefinition.scale = 175.0f;
	gLevelScreenshotNode = MakeNewObject(&gNewObjectDefinition);

	TQ3TriMeshData* mesh = MakeQuadMesh(1, 1.0f, 1.0f);
	mesh->texturingMode = kQ3TexturingModeOpaque;
	AttachGeometryToDisplayGroupObject(gLevelScreenshotNode, 1, &mesh,
			kAttachGeometry_TransferMeshOwnership);

	UpdateObjectTransforms(gLevelScreenshotNode);
}


/******************* DRAW BONUS STUFF ********************/

static void LevelSelectDrawStuff(const QD3DSetupOutputType *setupInfo)
{
	DrawObjects(setupInfo);
	QD3D_DrawShards(setupInfo);
}

/******************* Selection motion ********************/

static float UpdateTextState(ObjNode* theNode, long currentStateID)
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

static void MoveText_Wave(ObjNode* theNode)
{
	const float t = UpdateTextState(theNode, 'WAVE');
	const float D = 0.5f;

	TweenTowardsTQ3Point3D(t, D, &theNode->Coord, (TQ3Point3D){
		theNode->InitCoord.x,
		theNode->InitCoord.y,
		theNode->InitCoord.z + (1 + cosf(5*t)) * 7
	});
}

static void MoveText_Neutral(ObjNode* theNode)
{
	const float t = UpdateTextState(theNode, 'NTRL');
	const float D = 0.5f;
	TweenTowardsTQ3Point3D(t, D, &theNode->Coord, theNode->InitCoord);
}


static void MoveText(ObjNode *theNode)
{
    if (gHoveredPick == theNode->SpecialL[0]) {
        MoveText_Wave(theNode);
    } else {
        MoveText_Neutral(theNode);
    }

	UpdateObjectTransforms(theNode);
}
