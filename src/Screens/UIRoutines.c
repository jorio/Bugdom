// UI ROUTINES.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/*********************/
/*    VARIABLES      */
/*********************/

int32_t				gHoveredPick = -1;

static TQ3Point3D	gBonusCameraFrom = {0, 0, 250 };

/**********************  *************************/

void MoveBonusBackground(ObjNode *theNode)
{
	float	fps = gFramesPerSecondFrac;

	theNode->Rot.y += fps * .03f;
	UpdateObjectTransforms(theNode);
}

ObjNode* MakeBonusBackground(void)
{
	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
	gNewObjectDefinition.type 		= BONUS_MObjType_Background;
	gNewObjectDefinition.coord		= gBonusCameraFrom;
	gNewObjectDefinition.slot 		= 999;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG | STATUS_BIT_NULLSHADER;
	gNewObjectDefinition.moveCall 	= MoveBonusBackground;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 30.0;
	return MakeNewDisplayGroupObject(&gNewObjectDefinition);
}

void SetupUIStuff(int backgroundType)
{
	FSSpec					spec;
	QD3DSetupInputType		viewDef;
	TQ3Point3D				cameraTo = {0, 0, 0 };
	TQ3ColorRGB				lightColor = { 1.0, 1.0, .9 };
	TQ3Vector3D				fillDirection1 = { 1, -.4, -.8 };			// key
	TQ3Vector3D				fillDirection2 = { -.7, -.2, -.9 };			// fill


	/* RESET PICKABLE ITEMS */
	DisposePickableObjects();

	/*************/
	/* MAKE VIEW */
	/*************/

	QD3D_NewViewDef(&viewDef);

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

	if (backgroundType == kUIBackground_White)
		viewDef.view.clearColor = TQ3ColorRGBA_FromInt(0xFFFFFFFF);
	else
		viewDef.view.clearColor = TQ3ColorRGBA_FromInt(0x000000FF);


	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);


	QD3D_InitShards();

	/************/
	/* LOAD ART */
	/************/

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:BonusScreen.3dmf", &spec);
	LoadGrouped3DMF(&spec,MODEL_GROUP_BONUS);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Lawn_Models2.3dmf", &spec);
	LoadGrouped3DMF(&spec,MODEL_GROUP_LEVELSPECIFIC2);

	LoadASkeleton(SKELETON_TYPE_SPIDER);  // for back button

	/* LOAD SOUNDS */

	LoadSoundBank(SOUNDBANK_MAIN);
	LoadSoundBank(SOUNDBANK_BONUS);

	/*******************/
	/* MAKE BACKGROUND */
	/*******************/

	if (backgroundType == kUIBackground_Cyclorama)
		MakeBonusBackground();

	/*******************/
	/* START FADING IN */
	/*******************/

	MakeFadeEvent(true);

	InitAnalogCursor();
}

void CleanupUIStuff()
{
	ShutdownAnalogCursor();
	GammaFadeOut(false);
	DisposePickableObjects();
	TextMesh_Shutdown();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DeleteAll3DMFGroups();
	QD3D_DisposeShards();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);
	DisposeSoundBank(SOUNDBANK_BONUS);
	DisposeSoundBank(SOUNDBANK_MAIN);
	GameScreenToBlack();
	Pomme_FlushPtrTracking(true);
}

static void MoveSpider(ObjNode* objNode)
{
	long* pickID	= &objNode->SpecialL[4];
	long* isWalking	= &objNode->SpecialL[5];
	bool isHovered = gHoveredPick == *pickID;

	if (*isWalking && !isHovered)
	{
		*isWalking = false;
		MorphToSkeletonAnim(objNode->Skeleton, 0 /*SPIDER_ANIM_WAIT*/, 10);
	}
	else if (!*isWalking && isHovered)
	{
		*isWalking = true;
		MorphToSkeletonAnim(objNode->Skeleton, 2 /*SPIDER_ANIM_WALK*/, 10);
		objNode->Skeleton->AnimSpeed = 1.25f;
	}
}

void MakeSpiderButton(
		TQ3Point3D coord,
		const char* caption,
		int pickID)
{
	float gs = 0.95f;

	// Create pickable quad
	NewPickableQuad(coord, 45*gs, 45*gs, pickID);

	// Create spider button
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SKELETON_TYPE_SPIDER;
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	gNewObjectDefinition.coord		= (TQ3Point3D){coord.x, coord.y-1, coord.z-1};
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= MoveSpider;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .2f * gs;
	ObjNode* spider = MakeNewSkeletonObject(&gNewObjectDefinition);
	spider->Rot.y = 1.25f * PI / 2.0f;
	UpdateObjectTransforms(spider);

	spider->SpecialL[4] = pickID;	// remember pickID for move call
	spider->SpecialL[5] = 0;		// is walking

	// Create caption text
	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);
	tmd.slot  = SLOT_OF_DUMB;
	tmd.coord = (TQ3Point3D){coord.x, coord.y-16, coord.z};
	tmd.align = TEXTMESH_ALIGN_CENTER;
	tmd.scale = 0.3f * gs;
	tmd.color = TQ3ColorRGBA_FromInt(0xbee000ff);
	TextMesh_Create(&tmd, caption);
}

int UpdateHoveredPickID(void)
{
	gHoveredPick = -1;

	int mouseX, mouseY;
	mouseX = 0;
	mouseY = 0;
	MoveAnalogCursor(&mouseX, &mouseY);

	int32_t pickID;
	if (PickObject(mouseX, mouseY, &pickID))
	{
		gHoveredPick = pickID;
	}

	return gHoveredPick;
}

void NukeObjectsInSlot(u_short objNodeSlotID)
{
	ObjNode* node = gFirstNodePtr;
	while (node)
	{
		ObjNode* nextNode = node->NextNode;
		if (node->Slot == objNodeSlotID)
		{
			//QD3D_ExplodeGeometry(node, 200.0f, SHARD_MODE_UPTHRUST | SHARD_MODE_NULLSHADER, 1, 0.5f);
			DeleteObject(node);
		}
		node = nextNode;
	}
}
