/****************************/
/*   WIN/LOSE SCREEN.C	    */
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupWinScreen(void);
static void WinLoseDrawStuff(const QD3DSetupOutputType *setupInfo);
static Boolean WaitAndDraw(float duration, Boolean fire);
static void MoveWinLoseCamera(void);
static void SetupLoseScreen(void);
static void MoveLoseKing(ObjNode *theNode);
static void UpdateLoseFire(void);


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	WINLOSE_MObjType_ThroneRoom,
	WINLOSE_MObjType_WinRoom,
	WINLOSE_MObjType_Cage,
	WINLOSE_MObjType_Post
};

#define	CAGE_X	0
#define	CAGE_Z	630
#define	CAGE_Y	40

#define	KINGANT_HEAD_LIMB	1

#define LOSE_THRONE_LAVA_SUBMESH 5
#define WIN_THRONE_WATER_SUBMESH 23

/*********************/
/*    VARIABLES      */
/*********************/

static float gFireTimer = 0;
static int32_t	gFireGroup1,gFireGroup2;

static ObjNode	*gThrone;

#define	FireTimer	SpecialF[0]


/********************** DO WIN SCREEN *************************/

void DoWinScreen(void)
{

			/*********/
			/* SETUP */
			/*********/
			
	KillSong();
	SetupWinScreen();
	PlaySong(SONG_WIN,false);

	QD3D_CalcFramesPerSecond();
	QD3D_CalcFramesPerSecond();

		/**************/
		/* PROCESS IT */
		/**************/
				
	WaitAndDraw(29, false);	
	MakeFadeEvent(false);
	WaitAndDraw(1, false);	
	
			/* CLEANUP */

	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DeleteAll3DMFGroups();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);		
	Pomme_FlushPtrTracking(true);
}


/****************** SETUP WIN **************************/

static void SetupWinScreen(void)
{
float					scale;
ObjNode					*newObj;
FSSpec					spec;
QD3DSetupInputType		viewDef;
TQ3Point3D				cameraTo = {0, 240, 0 };
TQ3Point3D				cameraFrom = {0,250, 150};
TQ3ColorRGB				lightColor = { 1.0, 1.0, .9 };
TQ3Vector3D				fillDirection1 = { 1, -.4, -.8 };			// key
TQ3Vector3D				fillDirection2 = { -.7, -.2, -.9 };			// fill
int						i;
static const TQ3Point3D	lbCoord[] =
{
	{ -300, 95,  100 },		{ 300, 90,  100 },
	{ -300, 95,  300 },		{ 300, 95,  300 },
	{ -350, 95,  500 },		{ 350, 95,  500 },
	{ -350, 95,  700 },		{ 350, 95,  700 },
	{ -350, 95,  900 },		{ 350, 95,  900 },
	{ -120, 95, 1100 },		{ 120, 95, 1100 },
};

static const float	lbRot[] =
{
	-1.3,		1.3,
	-1.2,		1.2,
	-1.1,		1.1,
	-1.0,		1.0,
	-0.8,		0.8,
	-.03,		.03
};

static const TQ3Point2D po[4] =
{
	{-430,-430},	{-430, 430},
	{ 430, 430},	{ 430,-430}
};

			/*************/
			/* MAKE VIEW */
			/*************/

	QD3D_NewViewDef(&viewDef);
	
	viewDef.camera.hither 			= 20;
	viewDef.camera.yon 				= 3000;		// Source port mod (was 2000) so fog doesn't kick in
	viewDef.camera.fov 				= 1.1;
	viewDef.styles.usePhong 		= false;
	viewDef.camera.from				= cameraFrom;
	viewDef.camera.to	 			= cameraTo;
	
	viewDef.lights.numFillLights 	= 1;
	viewDef.lights.ambientBrightness = 0.4;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[0] 	= lightColor;
	viewDef.lights.fillColor[1] 	= lightColor;
	viewDef.lights.fillBrightness[0] = 1.0;
	viewDef.lights.fillBrightness[1] = .4;
	
	
	viewDef.view.clearColor.r = .9;
	viewDef.view.clearColor.g = .9;
	viewDef.view.clearColor.b = .8;
		
	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);

			/************/
			/* LOAD ART */
			/************/
			
	LoadASkeleton(SKELETON_TYPE_LADYBUG);
	LoadASkeleton(SKELETON_TYPE_ME);
	LoadASkeleton(SKELETON_TYPE_KINGANT);
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:WinLose.3dmf", &spec);
	LoadGrouped3DMF(&spec,MODEL_GROUP_MENU);	


			/*******************/
			/* MAKE BACKGROUND */
			/*******************/
			
			/* THRONE ROOM */
				
	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;	
	gNewObjectDefinition.type 		= WINLOSE_MObjType_WinRoom;	
	gNewObjectDefinition.coord.x	= 0;
	gNewObjectDefinition.coord.y	= 0;
	gNewObjectDefinition.coord.z	= 1000;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.1;
	gThrone = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* PLAYER BUG */
			
	gNewObjectDefinition.type 		= SKELETON_TYPE_ME;
	gNewObjectDefinition.animNum 	= PLAYER_ANIM_SITONTHRONE;							
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 135;
	gNewObjectDefinition.coord.z 	= -30;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= PI;
	gNewObjectDefinition.scale 		= 1.6;
	MakeNewSkeletonObject(&gNewObjectDefinition);			
	
	
			/* LADY BUGS */
	
	for (i = 0; i < 12; i++)
	{
		gNewObjectDefinition.type 		= SKELETON_TYPE_LADYBUG;
		gNewObjectDefinition.animNum 	= 3;							
		gNewObjectDefinition.coord		= lbCoord[i];
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= lbRot[i];
		gNewObjectDefinition.scale 		= .9;
		newObj = MakeNewSkeletonObject(&gNewObjectDefinition);			
		newObj->Skeleton->CurrentAnimTime = RandomFloat()*newObj->Skeleton->MaxAnimTime;	// random time index start
	}	
	
		/******************/
		/* KING IN CAGE */
		/******************/
	
			/* KING */
			
	gNewObjectDefinition.type 		= SKELETON_TYPE_KINGANT;
	gNewObjectDefinition.animNum 	= 0;	
	gNewObjectDefinition.coord.x 	= CAGE_X;
	gNewObjectDefinition.coord.y 	= CAGE_Y + 80;
	gNewObjectDefinition.coord.z 	= CAGE_Z;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= PI;
	gNewObjectDefinition.scale 		= .6;
	MakeNewSkeletonObject(&gNewObjectDefinition);			


			/* CAGE */

	scale = .2;
	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;	
	gNewObjectDefinition.type 		= WINLOSE_MObjType_Cage;	
	gNewObjectDefinition.coord.x 	= CAGE_X;
	gNewObjectDefinition.coord.y 	= CAGE_Y;
	gNewObjectDefinition.coord.z 	= CAGE_Z;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= scale;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* 4 POSTS */
				
	
	for (i = 0; i < 4; i++)
	{
		gNewObjectDefinition.type 		= WINLOSE_MObjType_Post;	
		gNewObjectDefinition.coord.x 	= CAGE_X + (po[i].x * scale);
		gNewObjectDefinition.coord.y 	= CAGE_Y;
		gNewObjectDefinition.coord.z 	= CAGE_Z + (po[i].y * scale);
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.rot 		= -PI/2 + (PI/2 * i);
		MakeNewDisplayGroupObject(&gNewObjectDefinition);
	}
	
	
	MakeFadeEvent(true);
}

#pragma mark -



/********************** DO LOSE SCREEN *************************/

void DoLoseScreen(void)
{

			/*********/
			/* SETUP */
			/*********/
			
	KillSong();
	SetupLoseScreen();
	PlaySong(SONG_LOSE,false);


	QD3D_CalcFramesPerSecond();
	QD3D_CalcFramesPerSecond();

		/**************/
		/* PROCESS IT */
		/**************/
				
	WaitAndDraw(29, true);	
	MakeFadeEvent(false);
	WaitAndDraw(1, true);	
				
	
			/* CLEANUP */

	DeleteAllObjects();
	DeleteAllParticleGroups();
	FreeAllSkeletonFiles(-1);
	DeleteAll3DMFGroups();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);		
	Pomme_FlushPtrTracking(true);
}


/****************** SETUP LOSE **************************/

static void SetupLoseScreen(void)
{
float					scale;
ObjNode					*newObj;
FSSpec					spec;
QD3DSetupInputType		viewDef;
TQ3Point3D				cameraTo = {0, 190, 0 };
TQ3Point3D				cameraFrom = {0,230, 150};
TQ3ColorRGB				lightColor = { 1.0, .9, .7 };
TQ3Vector3D				fillDirection1 = { 1, -.4, -.9 };			// key
TQ3Vector3D				fillDirection2 = { -.2, -1, -.2 };			// up
int						i;

static const TQ3Point3D	lbCoord[] =
{
	{ -300, 85,  100 },		{ 300, 80,  100 },
	{ -300, 85,  300 },		{ 300, 85,  300 },
	{ -350, 85,  500 },		{ 350, 85,  500 },
	{ -350, 85,  700 },		{ 350, 85,  700 },
	{ -350, 85,  900 },		{ 350, 85,  900 },
	{ -120, 85, 1100 },		{ 120, 85, 1100 },
};

static const float	lbRot[] =
{
	-1.4,		1.4,
	-1.4,		1.4,
	-1.4,		1.4,
	-1.4,		1.4,
	-1.4,		1.4,
	-.03,		.03
};

static const TQ3Point2D po[4] =
{
	{ -430,-430 },	{ -430, 430 },
	{  430, 430 },	{  430,-430 },
};
	

			/*************/
			/* MAKE VIEW */
			/*************/

	QD3D_NewViewDef(&viewDef);
	
	viewDef.camera.hither 			= 20;
	viewDef.camera.yon 				= 2000;
	viewDef.camera.fov 				= 1.1;
	viewDef.styles.usePhong 		= false;
	viewDef.camera.from				= cameraFrom;
	viewDef.camera.to	 			= cameraTo;

	viewDef.lights.fogStart			= .25f;				// source port note: these aren't original values,
	viewDef.lights.fogEnd			= 1.5f;				// but they mimic the ominous red fog present in the OS9 version

	viewDef.lights.numFillLights 	= 1;
	viewDef.lights.ambientBrightness = 0.1;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[0] 	= lightColor;
	viewDef.lights.fillColor[1] 	= lightColor;
	viewDef.lights.fillBrightness[0] = 1.0;
	viewDef.lights.fillBrightness[1] = 1;
	
	
	viewDef.view.clearColor.r = .8;
	viewDef.view.clearColor.g = 0;
	viewDef.view.clearColor.b = 0;
		
		
	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);

			/************/
			/* LOAD ART */
			/************/

	InitParticleSystem();
			
	LoadASkeleton(SKELETON_TYPE_FIREANT);
	LoadASkeleton(SKELETON_TYPE_ME);
	LoadASkeleton(SKELETON_TYPE_KINGANT);
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:WinLose.3dmf", &spec);
	LoadGrouped3DMF(&spec,MODEL_GROUP_MENU);	


			/*******************/
			/* MAKE BACKGROUND */
			/*******************/
			
			/* THRONE ROOM */
				
	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;	
	gNewObjectDefinition.type 		= WINLOSE_MObjType_ThroneRoom;	
	gNewObjectDefinition.coord.x	= 0;
	gNewObjectDefinition.coord.y	= 0;
	gNewObjectDefinition.coord.z	= 1000;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.1;
	gThrone = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* KING ON THRONE */
			
	gNewObjectDefinition.type 		= SKELETON_TYPE_KINGANT;
	gNewObjectDefinition.animNum 	= 5;							
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 230;
	gNewObjectDefinition.coord.z 	= -20;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveLoseKing;
	gNewObjectDefinition.rot 		= PI;
	gNewObjectDefinition.scale 		= .8;
	MakeNewSkeletonObject(&gNewObjectDefinition);			
	
	
			/* FIRE ANTS */
	
	for (i = 0; i < 12; i++)
	{
		gNewObjectDefinition.type 		= SKELETON_TYPE_FIREANT;
		gNewObjectDefinition.animNum 	= 0;
		gNewObjectDefinition.coord		= lbCoord[i];
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= lbRot[i];
		gNewObjectDefinition.scale 		= 1.0;
		newObj = MakeNewSkeletonObject(&gNewObjectDefinition);			
		newObj->Skeleton->CurrentAnimTime = RandomFloat()*newObj->Skeleton->MaxAnimTime;	// random time index start
	}	
	
		/******************/
		/* PLAYER IN CAGE */
		/******************/
	
			/* PLAYER */
			
	gNewObjectDefinition.type 		= SKELETON_TYPE_ME;
	gNewObjectDefinition.animNum 	= PLAYER_ANIM_CAPTURED;	
	gNewObjectDefinition.coord.x 	= CAGE_X;
	gNewObjectDefinition.coord.y 	= CAGE_Y;
	gNewObjectDefinition.coord.z 	= CAGE_Z;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= PI;
	gNewObjectDefinition.scale 		= 1.3;
	MakeNewSkeletonObject(&gNewObjectDefinition);			


			/* CAGE */

	scale = .2;
	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;	
	gNewObjectDefinition.type 		= WINLOSE_MObjType_Cage;	
	gNewObjectDefinition.coord.x 	= CAGE_X;
	gNewObjectDefinition.coord.y 	= CAGE_Y;
	gNewObjectDefinition.coord.z 	= CAGE_Z;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= scale;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* 4 POSTS */
				
	
	for (i = 0; i < 4; i++)
	{
		gNewObjectDefinition.type 		= WINLOSE_MObjType_Post;	
		gNewObjectDefinition.coord.x 	= CAGE_X + (po[i].x * scale);
		gNewObjectDefinition.coord.y 	= CAGE_Y;
		gNewObjectDefinition.coord.z 	= CAGE_Z + (po[i].y * scale);
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.rot 		= -PI/2 + (PI/2 * i);
		MakeNewDisplayGroupObject(&gNewObjectDefinition);
	}
	
	
		/* INIT FIRES */
		
	gFireGroup1 = -1;
	gFireGroup2 = -1;
	gFireTimer = 0;
	
	MakeFadeEvent(true);
}




#pragma mark -


/******************** WAIT AND DRAW *********************/

static Boolean WaitAndDraw(float duration, Boolean fire)
{
float	fps;

	do
	{
		UpdateInput();
		MoveObjects();
		MoveWinLoseCamera();
		MoveParticleGroups();
		QD3D_MoveShards();
		QD3D_DrawScene(gGameViewInfoPtr,WinLoseDrawStuff);
		QD3D_CalcFramesPerSecond();	
		DoSDLMaintenance();
		fps = gFramesPerSecondFrac;			
		duration -= fps;
		
		if (GetSkipScreenInput())
			return(true);		
		
		if (fire)
		{
 			UpdateLoseFire();

 			GAME_ASSERT_MESSAGE(gThrone->NumMeshes > LOSE_THRONE_LAVA_SUBMESH, "lava mesh ID not found in lose throne");
			QD3D_ScrollUVs(gThrone->MeshList[LOSE_THRONE_LAVA_SUBMESH], fps*.1f, -fps*.05f);
			gThrone->MeshList[LOSE_THRONE_LAVA_SUBMESH]->hasVertexNormals = false;  // make it pop - don't shade lava
		}
		else
		{
			GAME_ASSERT_MESSAGE(gThrone->NumMeshes > WIN_THRONE_WATER_SUBMESH, "water mesh ID not found in win throne");
			QD3D_ScrollUVs(gThrone->MeshList[WIN_THRONE_WATER_SUBMESH], fps*.1f, -fps*.05f);
		}
		
	}while(duration > 0.0f);

	return(false);
}

/***************** INTRO WINLOSE STUFF ******************/

static void WinLoseDrawStuff(const QD3DSetupOutputType *setupInfo)
{
	DrawObjects(setupInfo);
	QD3D_DrawShards(setupInfo);
	DrawParticleGroup(setupInfo);
}




/****************** MOVE WINLOSE CAMERA *********************/

static void MoveWinLoseCamera(void)
{
TQ3Point3D	from,to;
float		fps = gFramesPerSecondFrac;

	from = gGameViewInfoPtr->currentCameraCoords;
	to = gGameViewInfoPtr->currentCameraLookAt;


	from.z += 50.0f*fps;
	
	
	QD3D_UpdateCameraFromTo(gGameViewInfoPtr, &from, &to);	
}


/***************** MOVE LOSE KING ********************/

static void MoveLoseKing(ObjNode *theNode)
{
	theNode->FireTimer += gFramesPerSecondFrac;
	if (theNode->FireTimer > .02f)
	{
		theNode->FireTimer = 0;

		if (theNode->ParticleGroup == -1)
		{
new_group:		
			theNode->ParticleGroup = NewParticleGroup(
														PARTICLE_TYPE_FALLINGSPARKS,	// type
														0,							// flags
														-80,						// gravity
														8000,						// magnetism
														9,							// base scale
														.1,							// decay rate
														.5,							// fade rate
														PARTICLE_TEXTURE_FIRE);		// texture
		}

		if (theNode->ParticleGroup != -1)
		{
			TQ3Vector3D	delta;
			TQ3Point3D  pt;
			static const TQ3Point3D off = {0,100, -50};							// offset to top of head
		
			for (int i = 0; i < 4; i++)
			{
				FindCoordOnJoint(theNode, KINGANT_HEAD_LIMB, &off, &pt);			// get coord of head
				pt.x += (RandomFloat()-.5f) * 40.0f;
				pt.y += (RandomFloat()-.5f) * 20.0f;
				pt.z += (RandomFloat()-.5f) * 40.0f;
				
				delta.x = 0; //(RandomFloat()-.5f) * 60.0f;
				delta.y = RandomFloat() * 50.0f;
				delta.z = 0; //(RandomFloat()-.5f) * 60.0f;
				
				if (AddParticleToGroup(theNode->ParticleGroup, &pt, &delta, RandomFloat() + 1.5f, FULL_ALPHA))
					goto new_group;
			}
		}
	}
}


/**************************** UPDATE LOSE FIRE *****************************/

static void UpdateLoseFire(void)
{
TQ3Vector3D	delta;
TQ3Point3D  pt;

	gFireTimer += gFramesPerSecondFrac;
	if (gFireTimer > .02f)
	{
		gFireTimer = 0;


				/* LEFT */
				
		if (gFireGroup1 == -1)
		{
new_group:		
			gFireGroup1 = NewParticleGroup(
											PARTICLE_TYPE_FALLINGSPARKS,	// type
											0,							// flags
											-80,						// gravity
											8000,						// magnetism
											10,							// base scale
											.1,							// decay rate
											.4,							// fade rate
											PARTICLE_TEXTURE_FIRE);		// texture
		}

		if (gFireGroup1 != -1)
		{
			for (int i = 0; i < 5; i++)
			{
				pt.x = -500 + (RandomFloat()-.5f) * 50.0f;
				pt.y = 110 + (RandomFloat()-.5f) * 20.0f;
				pt.z = -50 + (RandomFloat()-.5f) * 50.0f;
				
				delta.x = 0; //(RandomFloat()-.5f) * 60.0f;
				delta.y = RandomFloat() * 50.0f;
				delta.z = 0; //(RandomFloat()-.5f) * 60.0f;
				
				if (AddParticleToGroup(gFireGroup1, &pt, &delta, RandomFloat() + 1.8f, FULL_ALPHA))
					goto new_group;
			}
		}

				/*********/
				/* RIGHT */
				/*********/
				
		if (gFireGroup2 == -1)
		{
new_group2:		
			gFireGroup2 = NewParticleGroup(
											PARTICLE_TYPE_FALLINGSPARKS,	// type
											0,							// flags
											-80,						// gravity
											8000,						// magnetism
											10,							// base scale
											.1,							// decay rate
											.4,							// fade rate
											PARTICLE_TEXTURE_FIRE);		// texture
		}

		if (gFireGroup2 != -1)
		{
			for (int i = 0; i < 5; i++)
			{
				pt.x = 540 + (RandomFloat()-.5f) * 50.0f;
				pt.y = 110 + (RandomFloat()-.5f) * 20.0f;
				pt.z = -50 + (RandomFloat()-.5f) * 50.0f;
				
				delta.x = 0; //(RandomFloat()-.5f) * 60.0f;
				delta.y = RandomFloat() * 50.0f;
				delta.z = 0; //(RandomFloat()-.5f) * 60.0f;
				
				if (AddParticleToGroup(gFireGroup2, &pt, &delta, RandomFloat() + 1.8f, FULL_ALPHA))
					goto new_group2;
			}
		}

	}
}













