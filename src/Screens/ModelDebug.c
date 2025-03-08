// MODEL DEBUG.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include "game.h"

static TQ3Point3D gModelDebug_modelCoord = { 0, 0, 0 };
static float gCameraLookAtAccel = 9;
static float gCameraFromAccelY = 1.5f;
static float gCameraFromAccel = 4.5f;
static float gCameraDistFromMe = 200.0f;
static float gCameraHeightFactor	= 0;
static float gCameraLookAtYOff		= 0;

static ObjNode*	gModel	= nil;
static int gDisplaySkeleton		= 1;
static int gGlowOn				= 0;
static int gCurrentSkeletonType = SKELETON_TYPE_KINGANT;
static int gCurrentModelFile	= 11;
static int gCurrentModelType	= 11;

#define gNumModelFiles 16

static const char* gModelFiles[gNumModelFiles] =
{
		":Models:AntHill_Models.3dmf",
		":Models:BeeHive_Models.3dmf",
		":Models:BonusScreen.3dmf",
		":Models:Forest_Models.3dmf",
		":Models:Global_Models1.3dmf",
		":Models:Global_Models2.3dmf",
		":Models:HighScores.3dmf",
		":Models:Lawn_Models1.3dmf",
		":Models:Lawn_Models2.3dmf",
		":Models:LevelIntro.3dmf",
		":Models:MainMenu.3dmf",
		":Models:Night_Models.3dmf",
		":Models:Pangea.3dmf",
		":Models:Pond_Models.3dmf",
		":Models:Title.3dmf",
		":Models:WinLose.3dmf",
};

#define	CAM_MINY			20.0f
#define	CAMERA_CLOSEST		150.0f
#define	CAMERA_FARTHEST		800.0f



static void MoveCamera_ModelDebug(void)
{
	TQ3Point3D	from,to,target;
	float		distX,distZ,distY,dist;
	TQ3Vector2D	pToC;
	float		myX,myY,myZ;
	float		fps = gFramesPerSecondFrac;


	myX = gModelDebug_modelCoord.x;
	myY = gModelDebug_modelCoord.y;// + gPlayerObj->BottomOff;
	myZ = gModelDebug_modelCoord.z;

	/**********************/
	/* CALC LOOK AT POINT */
	/**********************/

	target.x = myX;								// accelerate "to" toward target "to"
	target.y = myY + gCameraLookAtYOff;
	target.z = myZ;

	distX = target.x - gGameViewInfoPtr->currentCameraLookAt.x;
	distY = target.y - gGameViewInfoPtr->currentCameraLookAt.y;
	distZ = target.z - gGameViewInfoPtr->currentCameraLookAt.z;

	if (distX >  350.0f) distX =  350.0f;
	if (distX < -350.0f) distX = -350.0f;
	if (distZ >  350.0f) distZ =  350.0f;
	if (distZ < -350.0f) distZ = -350.0f;

	to.x = gGameViewInfoPtr->currentCameraLookAt.x+(distX * (fps * gCameraLookAtAccel));
	to.y = gGameViewInfoPtr->currentCameraLookAt.y+(distY * (fps * (gCameraLookAtAccel*.7)));
	to.z = gGameViewInfoPtr->currentCameraLookAt.z+(distZ * (fps * gCameraLookAtAccel));


	/*******************/
	/*  CALC FROM POINT */
	/*******************/

	pToC.x = gGameViewInfoPtr->currentCameraCoords.x - myX;				// calc player->camera vector
	pToC.y = gGameViewInfoPtr->currentCameraCoords.z - myZ;
	Q3Vector2D_Normalize(&pToC,&pToC);									// normalize it

	target.x = myX + (pToC.x * gCameraDistFromMe);						// target is appropriate dist based on cam's current coord
	target.z = myZ + (pToC.y * gCameraDistFromMe);


	/* MOVE CAMERA TOWARDS POINT */

	distX = target.x - gGameViewInfoPtr->currentCameraCoords.x;
	distZ = target.z - gGameViewInfoPtr->currentCameraCoords.z;

	// clamp accel factor
	if (distX >  500.0f) distX =  500.0f;
	if (distX < -500.0f) distX = -500.0f;
	if (distZ >  500.0f) distZ =  500.0f;
	if (distZ < -500.0f) distZ = -500.0f;

	distX = distX * (fps * gCameraFromAccel);
	distZ = distZ * (fps * gCameraFromAccel);


	from.x = gGameViewInfoPtr->currentCameraCoords.x+distX;
	from.y = gGameViewInfoPtr->currentCameraCoords.y;  // from.y mustn't be NaN for matrix transform in cam swivel below -- actual Y computed later
	from.z = gGameViewInfoPtr->currentCameraCoords.z+distZ;


	/*****************************/
	/* SEE IF USER ROTATE CAMERA */
	/*****************************/

	if (GetKeyState(kKey_SwivelCameraLeft))
	{
		TQ3Matrix4x4	m;
		float			r;

		r = fps * -10.0f;
		Q3Matrix4x4_SetRotateAboutPoint(&m, &to, 0, r, 0);
		Q3Point3D_Transform(&from, &m, &from);
	}
	else
	if (GetKeyState(kKey_SwivelCameraRight))
	{
		TQ3Matrix4x4	m;
		float			r;

		r = fps * 10.0f;
		Q3Matrix4x4_SetRotateAboutPoint(&m, &to, 0, r, 0);
		Q3Point3D_Transform(&from, &m, &from);
	}

	/***************/
	/* CALC FROM Y */
	/***************/

	dist = CalcQuickDistance(from.x, from.z, to.x, to.z) - CAMERA_CLOSEST;
	if (dist < 0.0f)
		dist = 0.0f;

	target.y = to.y + (dist*gCameraHeightFactor) + CAM_MINY;						// calc desired y based on dist and height factor

	dist = (target.y - gGameViewInfoPtr->currentCameraCoords.y)*gCameraFromAccelY;	// calc dist from current y to desired y
	from.y = gGameViewInfoPtr->currentCameraCoords.y+(dist*fps);

	if (from.y < (to.y+CAM_MINY))													// make sure camera never under the "to" point (insures against camera flipping over)
		from.y = (to.y+CAM_MINY);
	
	/**********************/
	/* UPDATE CAMERA INFO */
	/**********************/

	QD3D_UpdateCameraFromTo(gGameViewInfoPtr,&from,&to);
}


static int IncrementWrap(int input, int delta, unsigned int modulo)
{
	input += delta;
	while (input < 0)
		input += modulo;
	return input % modulo;
}



static ObjNode* MakeSkeleton(void)
{
	NewObjectDefinitionType def = gNewObjectDefinition;
	def.group		= MODEL_GROUP_LEVELSPECIFIC;
	def.type 		= gCurrentSkeletonType;
	def.animNum 	= 0;
	def.coord		= gModelDebug_modelCoord;
	def.flags 		= STATUS_BIT_NOFOG | (gGlowOn? STATUS_BIT_GLOW: 0);
	def.slot 		= 100;
	def.moveCall 	= nil;
	def.rot 		= PI;
	def.scale 		= 0.5f;
	return MakeNewSkeletonObject(&def);
}

static ObjNode* MakeStaticObject(void)
{
	NewObjectDefinitionType def = gNewObjectDefinition;
	def.group		= MODEL_GROUP_LEVELSPECIFIC;
	def.type		= gCurrentModelType;
	def.coord.x 	= 0;
	def.coord.y 	= 0;
	def.coord.z 	= 0;
	def.flags 		= STATUS_BIT_NOFOG | (gGlowOn? STATUS_BIT_GLOW: 0) | STATUS_BIT_NULLSHADER;
	def.slot 		= 50;
	def.moveCall 	= nil;
	def.rot 		= 0;
	def.scale 		= .33f;
	return MakeNewDisplayGroupObject(&def);
}

static void Regen(void)
{
	if (gModel)
	{
		DeleteObject(gModel);
	}

	if (gDisplaySkeleton)
		gModel = MakeSkeleton();
	else
		gModel = MakeStaticObject();
}



void DoModelDebug(void)
{
	TQ3Point3D			cameraFrom = { 0, 0, 70.0 };
	QD3DSetupInputType		viewDef;
	FSSpec			spec;
	TQ3ColorRGB				lightColor = { 1.0, 1.0, .9 };
	TQ3Vector3D				fillDirection1 = { 1, -.4, -.8 };			// key
	TQ3Vector3D				fillDirection2 = { -.7, -.2, -.9 };			// fill



	gDisableAnimSounds = true;
	gAutoFadeStartDist = 1.0f;


	/* MAKE VIEW */

	QD3D_NewViewDef(&viewDef);
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 2000;
	viewDef.camera.fov 				= .9;
	viewDef.camera.from				= cameraFrom;
	viewDef.view.clearColor.r 		= 0.125f;
	viewDef.view.clearColor.g 		= 0.25f;
	viewDef.view.clearColor.b		= 0.50f;
//	viewDef.view.paneClip.top		=	0;
//	viewDef.view.paneClip.bottom	=	32;
//	viewDef.view.paneClip.left		=	0;
//	viewDef.view.paneClip.right		=	0;
	viewDef.styles.usePhong 		= false;

	viewDef.lights.numFillLights 	= 2;
	viewDef.lights.ambientBrightness = 0.3;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[0] 	= lightColor;
	viewDef.lights.fillColor[1] 	= lightColor;
	viewDef.lights.fillBrightness[0] = 1.0;
	viewDef.lights.fillBrightness[1] = .2;

	viewDef.lights.fogStart 	= .5;
	viewDef.lights.fogEnd 		= 1.0;

	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);


	/* LOAD ART */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, gModelFiles[gCurrentModelFile], &spec);
	LoadGrouped3DMF(&spec,MODEL_GROUP_LEVELSPECIFIC);


	for (int i = 0; i < MAX_SKELETON_TYPES; i++)
		LoadASkeleton(i);

	
	/***************/
	/* MAKE MODELS */
	/***************/


	GammaOn();

	Regen();


	QD3D_CalcFramesPerSecond();


	Boolean changeWindowTitle = true;

	do
	{
		changeWindowTitle = false;
		
		QD3D_CalcFramesPerSecond();

		UpdateInput();

		MoveObjects();
		MoveCamera_ModelDebug();

//		CalcEnvironmentMappingCoords(&gGameViewInfoPtr->currentCameraCoords);

		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);

		DoSDLMaintenance();

		Boolean keyModelFile		= GetNewKeyState_SDL(SDL_SCANCODE_F);
		Boolean keyNextPrimary		= GetNewKeyState_SDL(SDL_SCANCODE_TAB);
		Boolean keyNextSecondary	= GetNewKeyState_SDL(SDL_SCANCODE_RETURN);
		Boolean keyShift			= GetKeyState_SDL(SDL_SCANCODE_LSHIFT) || GetKeyState_SDL(SDL_SCANCODE_RSHIFT);
		Boolean keyToggleCull		= GetNewKeyState_SDL(SDL_SCANCODE_SPACE);
		Boolean keyZoom				= GetNewKeyState_SDL(SDL_SCANCODE_Z);
		Boolean keyToggleGlow		= GetNewKeyState_SDL(SDL_SCANCODE_G);

		if (keyToggleCull)
		{
			gDisplaySkeleton = !gDisplaySkeleton;
			Regen();
			changeWindowTitle = true;
		}

		if (keyToggleGlow) {
			gGlowOn = !gGlowOn;
			Regen();
			changeWindowTitle = true;
		}

		if (keyModelFile && !gDisplaySkeleton)
		{

			changeWindowTitle = true;
		}

		if (keyNextPrimary)
		{
			if (gDisplaySkeleton)
			{
				gCurrentSkeletonType = IncrementWrap(gCurrentSkeletonType, keyShift?-1:1, MAX_SKELETON_TYPES);
			}
			else
			{
				if (gModel)
				{
					DeleteObject(gModel);
					gModel = nil;
				}
				Free3DMFGroup(MODEL_GROUP_LEVELSPECIFIC);
				gCurrentModelFile = IncrementWrap(gCurrentModelFile, keyShift?-1:1, gNumModelFiles);
				gCurrentModelType = 0;
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, gModelFiles[gCurrentModelFile], &spec);
				LoadGrouped3DMF(&spec, MODEL_GROUP_LEVELSPECIFIC);
			}
			Regen();
			changeWindowTitle = true;
		}

		if (keyNextSecondary)
		{
			if (gDisplaySkeleton)
			{
				int anim = IncrementWrap(gModel->Skeleton->AnimNum, keyShift?-1:1, gModel->Skeleton->skeletonDefinition->NumAnims);
				MorphToSkeletonAnim(gModel->Skeleton, anim, 10);
			}
			else
			{
				gCurrentModelType = IncrementWrap(gCurrentModelType, keyShift?-1:1, gNumObjectsInGroupList[MODEL_GROUP_LEVELSPECIFIC]);
				Regen();
			}
			changeWindowTitle = true;
		}

		if (keyZoom)
		{
			float s = gModel->Scale.x;
			s *= (keyShift? 2.0f: 0.5f);
			gModel->Scale.x =gModel->Scale.y =gModel->Scale.z = s;
			UpdateObject(gModel);
		}

		if (changeWindowTitle)
		{
			char titlebuf[512];
			if (gDisplaySkeleton)
			{
				SDL_snprintf(titlebuf, sizeof(titlebuf), "Skeleton %d/%d [TAB], Anim %d/%d [ENTER]. Press [SPACE] for DGOs.",
						1+gCurrentSkeletonType,
						MAX_SKELETON_TYPES,
						1+gModel->Skeleton->AnimNum,
						gModel->Skeleton->skeletonDefinition->NumAnims);
			}
			else
			{
				SDL_snprintf(titlebuf, sizeof(titlebuf), "3DMF Group %s (%d/%d) [TAB], DGO %d/%d [ENTER]. Press [SPACE] for skeletons.",
						gModelFiles[gCurrentModelFile],
						1+gCurrentModelFile,
						gNumModelFiles,
						1+gCurrentModelType,
						gNumObjectsInGroupList[MODEL_GROUP_LEVELSPECIFIC]);
			}
			SDL_SetWindowTitle(gSDLWindow, titlebuf);
		}
		
	} while (!GetKeyState(kKey_UI_Cancel));

	

	DeleteAllObjects();
	Free3DMFGroup(MODEL_GROUP_LEVELSPECIFIC);
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);
	GameScreenToBlack();
	Pomme_FlushPtrTracking(true);
}
