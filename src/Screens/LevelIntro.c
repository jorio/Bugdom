/****************************/
/*   	LEVEL INTRO.C	    */
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

static void SetupLevelIntroScreen(void);
static void DropLevelLetters(void);
static Boolean WaitAndDraw(float duration);
static void MoveDroppingLetter(ObjNode *theNode);
static void MoveIntroCamera(void);

static void DoLawn1Intro(void);
static void MoveIntroAnt(ObjNode *theNode);
static void DoLawn2Intro(void);
static void MoveIntroBoxerfly(ObjNode *theNode);

static void DoPond1Intro(void);
static void MoveIntroFish(ObjNode *theNode);

static void DoForest1Intro(void);
static void MoveIntroFoot(ObjNode *theNode);

static void DoNightIntro(void);
static void MoveIntroFireAnt(ObjNode *theNode);

static void IntroDrawStuff(const QD3DSetupOutputType *setupInfo);

static void DoForest2Intro(void);
static void MoveIntroDragonFly(ObjNode *theNode);
static void MoveParachuteLetter(ObjNode *letter);

static void DoHive1Intro(void);
static void MoveIntroBee(ObjNode *theNode);

static void DoAntHill1Intro(void);
static void MoveAntHillLetter(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	LINTRO_MObjType_Log,
	LINTRO_MObjType_0,
	LINTRO_MObjType_1,
	LINTRO_MObjType_2,
	LINTRO_MObjType_3,
	LINTRO_MObjType_4,
	LINTRO_MObjType_5,
	LINTRO_MObjType_6,
	LINTRO_MObjType_7,
	LINTRO_MObjType_8,
	LINTRO_MObjType_9,
	LINTRO_MObjType_L,
	LINTRO_MObjType_E,
	LINTRO_MObjType_V,
	LINTRO_MObjType_LawnBackground,
	LINTRO_MObjType_ForestBackground,
	LINTRO_MObjType_NightBackground,
	LINTRO_MObjType_CherryBomb,
	LINTRO_MObjType_Parachute
};

enum
{
	INTRO_CAM_MODE_RIGHTDRIFT,
	INTRO_CAM_MODE_PULLBACK
};

const TQ3ColorRGBA kIntroClearColors[NUM_LEVEL_TYPES] =
{
		[LEVEL_TYPE_LAWN	] = {0.259f, 0.482f, 0.678f, 1.000f},
		[LEVEL_TYPE_POND	] = {0.259f, 0.482f, 0.678f, 1.000f},
		[LEVEL_TYPE_FOREST	] = {1.000f, 0.290f, 0.063f, 1.000f},
		[LEVEL_TYPE_HIVE	] = {0.259f, 0.482f, 0.678f, 1.000f},
		[LEVEL_TYPE_NIGHT	] = {0.000f, 0.000f, 0.000f, 1.000f},
		[LEVEL_TYPE_ANTHILL	] = {0.259f, 0.482f, 0.678f, 1.000f},
};

/*********************/
/*    VARIABLES      */
/*********************/

static TQ3Point3D	gCameraFrom = {0, 100, 900 };

static Byte	gIntroCamMode;

static ObjNode *gLetterObj[10];


/********************** SHOW LEVEL INTRO SCREEN *************************/

void ShowLevelIntroScreen(void)
{

			/* START AUDIO */
 		 				
 	switch(gLevelType)
 	{
 		case	LEVEL_TYPE_NIGHT:
				PlaySong(SONG_NIGHT,true);
				break;
				
		case	LEVEL_TYPE_FOREST:
				PlaySong(SONG_FOREST,true);
				break;
		

		case	LEVEL_TYPE_POND:
				PlaySong(SONG_POND,true);
				break;

		case	LEVEL_TYPE_ANTHILL:
				PlaySong(SONG_ANTHILL,true);
				break;

		case	LEVEL_TYPE_HIVE:
				PlaySong(SONG_HIVE,true);
				break;
		
 		default:
				PlaySong(SONG_GARDEN,true);
	}
		 

			/*********/
			/* SETUP */
			/*********/
			
	SetupLevelIntroScreen();
	QD3D_CalcFramesPerSecond();
	QD3D_CalcFramesPerSecond();

		/**************/
		/* PROCESS IT */
		/**************/
				
		
		/* DO LEVEL CUSTOM */
				
	switch(gRealLevel)
	{
		case	LEVEL_NUM_TRAINING:					
				DoLawn1Intro();
				break;
				
		case	LEVEL_NUM_LAWN:					
				DoLawn2Intro();
				break;

		case	LEVEL_NUM_POND:
				DoPond1Intro();
				break;

		case	LEVEL_NUM_BEACH:
				DoForest1Intro();
				break;
				
		case	LEVEL_NUM_FLIGHT:
				DoForest2Intro();
				break;

		case	LEVEL_NUM_HIVE:
				DoHive1Intro();
				break;

		case	LEVEL_NUM_NIGHT:
				DoNightIntro();
				break;

		case	LEVEL_NUM_ANTHILL:
				DoAntHill1Intro();
				break;

		default:		
				DropLevelLetters();
	}
	
			/* CLEANUP */

	GammaFadeOut(false);
	GameScreenToBlack();
	
	DeleteAllObjects();
	DeleteAllParticleGroups();
	FreeAllSkeletonFiles(-1);
	DeleteAll3DMFGroups();
	QD3D_DisposeShards();
	DisposeSoundBank(SOUNDBANK_MAIN);
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);		
	Pomme_FlushPtrTracking(true);
}


/****************** SETUP LEVEL INTRO SCREEN **************************/

static void SetupLevelIntroScreen(void)
{
FSSpec					spec;
QD3DSetupInputType		viewDef;
TQ3Point3D				cameraTo = {0, 0, 0 };
TQ3ColorRGB				lightColor = { 1.0, 1.0, .9 };
TQ3Vector3D				fillDirection1 = { .6, -.9, -1 };			// key
TQ3Vector3D				fillDirection2 = { -.7, -.2, -.9 };			// fill

		/* INIT OTHER SYSTEMS */

	QD3D_InitShards();

			/* LOAD SOUNDS */

	LoadSoundBank(SOUNDBANK_MAIN);


			/*************/
			/* MAKE VIEW */
			/*************/

	QD3D_NewViewDef(&viewDef);
	
	viewDef.camera.hither 			= 70;
	viewDef.camera.yon 				= 4000;
	viewDef.camera.fov 				= 1.0;
	viewDef.styles.usePhong 		= false;
	viewDef.camera.from.x 			= gCameraFrom.x;
	viewDef.camera.from.y 			= gCameraFrom.y;
	viewDef.camera.from.z 			= gCameraFrom.z;
	viewDef.camera.to	 			= cameraTo;
	
	viewDef.lights.numFillLights 	= 1;
	viewDef.lights.ambientBrightness = 0.2;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[0] 	= lightColor;
	viewDef.lights.fillColor[1] 	= lightColor;
	viewDef.lights.fillBrightness[0] = 1.3;
	viewDef.lights.fillBrightness[1] = .2;


	viewDef.view.clearColor = kIntroClearColors[gLevelType];


	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);

			/************/
			/* LOAD ART */
			/************/

	InitParticleSystem();		// Must be once we have a valid GL context

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:LevelIntro.3dmf", &spec);
	LoadGrouped3DMF(&spec,MODEL_GROUP_LEVELINTRO);	

	switch(gRealLevel)
	{
		case	LEVEL_NUM_TRAINING:
				LoadASkeleton(SKELETON_TYPE_ANT);
				break;
				
		case	LEVEL_NUM_LAWN:
				LoadASkeleton(SKELETON_TYPE_BOXERFLY);
				break;
			
		case	LEVEL_NUM_POND:
				LoadASkeleton(SKELETON_TYPE_PONDFISH);
				break;
				
		case	LEVEL_NUM_BEACH:
				LoadASkeleton(SKELETON_TYPE_FOOT);
				break;
				
		case	LEVEL_NUM_FLIGHT:
				LoadASkeleton(SKELETON_TYPE_DRAGONFLY);
				break;

		case	LEVEL_NUM_HIVE:
				LoadASkeleton(SKELETON_TYPE_FLYINGBEE);
				break;
			
		case	LEVEL_NUM_NIGHT:
				LoadASkeleton(SKELETON_TYPE_FIREANT);
				break;
			
	}

			/*******************/
			/* MAKE BACKGROUND */
			/*******************/
			
	if (gRealLevel != LEVEL_NUM_FLIGHT)
	{
				/* LOG */

		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;	
		gNewObjectDefinition.type 		= LINTRO_MObjType_Log;	
		gNewObjectDefinition.coord.x	= 0;
		gNewObjectDefinition.coord.y	= -300;
		gNewObjectDefinition.coord.z	= 0;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= 1.5;
		ObjNode* log = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				/* MAKE EXTRA LOGS FOR ULTRA-WIDE DISPLAYS */

		for (int i = 1; i < 4; i++)
		{
			gNewObjectDefinition.coord.x	= i*2.0f * gNewObjectDefinition.scale * log->MeshList[0]->bBox.max.x;
			MakeNewDisplayGroupObject(&gNewObjectDefinition);

			gNewObjectDefinition.coord.x	= -i*2.0f * gNewObjectDefinition.scale * log->MeshList[0]->bBox.max.x;
			MakeNewDisplayGroupObject(&gNewObjectDefinition);
		}
	}

		/* BACKGROUND CYC */
				
	switch(gLevelType)
	{
		case	LEVEL_TYPE_FOREST:
				gNewObjectDefinition.type 		= LINTRO_MObjType_ForestBackground;	
				break;

		case	LEVEL_TYPE_NIGHT:
		case	LEVEL_TYPE_ANTHILL:
				gNewObjectDefinition.type 		= LINTRO_MObjType_NightBackground;	
				break;

		default:
				gNewObjectDefinition.type 		= LINTRO_MObjType_LawnBackground;	
	}
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;	
	gNewObjectDefinition.coord.x	= 0;
	gNewObjectDefinition.coord.y	= 0;
	gNewObjectDefinition.coord.z	= 400;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER | STATUS_BIT_NOFOG | STATUS_BIT_NOZWRITE;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 6.0;
	ObjNode* cyc = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	QD3D_MirrorMeshesZ(cyc);


		/* FADE EVENT */
		
	GameScreenToBlack();
	MakeFadeEvent(true);
}

#pragma mark -


/*************** DROP LEVEL LETTERS *******************/

static void DropLevelLetters(void)
{
int		i,n,d1,d2;
static Byte level[] = {0,1,2,1,0};
ObjNode	*obj;
float	x;

	if (WaitAndDraw(1))
		return;

			/*******************/
			/* DROP IN "LEVEL" */
			/*******************/
			
	x = -300;
	for (i = 0; i < 5; i++)	
	{	
			/* CREATE LETTER */
			
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;	
		gNewObjectDefinition.type 		= LINTRO_MObjType_L + level[i];	
		gNewObjectDefinition.coord.x	= x;
		gNewObjectDefinition.coord.y	= 800;
		gNewObjectDefinition.coord.z	= 0;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= MoveDroppingLetter;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= .9;
		obj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		obj->Delta.y = -400;

		if (WaitAndDraw(.5))
			return;
		
		x += 100.0f;
	}

		/*******************/
		/* DROP IN NUMBER  */
		/*******************/

	x += 100;
			
		/* DETERMINE DIGITS */
			
	n = gRealLevel + 1;
	d1 = n/10;
	d2 = n%10;
			
		/* 1ST DIGIT */
		
	if (d1 != 0)
	{
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;
		gNewObjectDefinition.type 		= LINTRO_MObjType_0 + d1;	
		gNewObjectDefinition.coord.x	= x;
		obj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		obj->Delta.y = -400;
		if (WaitAndDraw(.5))
			return;
		x += 100;
	}
	
		/* 2ND DIGIT */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= LINTRO_MObjType_0 + d2;	
	gNewObjectDefinition.coord.x	= x;
	obj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	obj->Delta.y = -400;
	if (WaitAndDraw(5))
		return;
}

/***************** MOVE DROPPING LETTER *******************/

static void MoveDroppingLetter(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	gDelta.y -= 1000.0f * fps;

	gCoord.y += gDelta.y * fps;
	if (gCoord.y < 55.0f)
	{
		gCoord.y = 55.0f;
		gDelta.y *= -.3f;
	}
		
	UpdateObject(theNode);

}

#pragma mark -

/*************** DO LAWN 1 INTRO *******************/

static void DoLawn1Intro(void)
{
ObjNode	*ant,*letter;
int		i;
float	x;
static Byte	letters[] = {LINTRO_MObjType_L, LINTRO_MObjType_E, LINTRO_MObjType_V, LINTRO_MObjType_E,
						LINTRO_MObjType_L, LINTRO_MObjType_1};

	gIntroCamMode = INTRO_CAM_MODE_RIGHTDRIFT;

		/*******************/
		/* CREATE THE ANTS */
		/*******************/

	x = -1700;		
	for (i = 0; i < 6; i++)
	{		
			/* MAKE AN ANT */
			
		gNewObjectDefinition.type 		= SKELETON_TYPE_ANT;
		gNewObjectDefinition.animNum 	= ANT_ANIM_WALKCARRY;							
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= 68;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.moveCall 	= MoveIntroAnt;
		gNewObjectDefinition.rot 		= -PI/2;
		gNewObjectDefinition.scale 		= 1.2;
		ant = MakeNewSkeletonObject(&gNewObjectDefinition);			
		ant->Delta.x = 250;
		ant->Skeleton->CurrentAnimTime = i / 6.0 * ant->Skeleton->MaxAnimTime;
	
			/* MAKE THE LETTER */
			
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;	
		gNewObjectDefinition.type 		= letters[i];	
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= 1.0;
		letter = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		ant->ChainNode = letter;		
		
		if (i == 4)					// spacing before number
			x += 300;
		else
			x += 150;	
	}
		
	if (WaitAndDraw(12))
		return;
}


/******************** MOVE INTRO ANT ***********************/

static void MoveIntroAnt(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*letter;
static TQ3Point3D off = {0,110, -20};

		/* MOVE THE ANT */
		
	GetObjectInfo(theNode);
	gCoord.x += gDelta.x * fps;
	UpdateObject(theNode);
	
	theNode->Skeleton->AnimSpeed = gDelta.x * 0.004f;
	
		/* UPDATE LETTER */
		
	letter = theNode->ChainNode;	
	FindCoordOnJoint(theNode, 1, &off, &letter->Coord);		// find coord of head
	UpdateObjectTransforms(letter);
}


/*************** DO LAWN 2 INTRO *******************/

static void DoLawn2Intro(void)
{
TQ3Point3D from,to;
ObjNode	*fly;
int		i;
float	x;
static Byte	letters[] = {LINTRO_MObjType_E, LINTRO_MObjType_E, LINTRO_MObjType_V, LINTRO_MObjType_E,
						LINTRO_MObjType_L, LINTRO_MObjType_2};

	gIntroCamMode = INTRO_CAM_MODE_PULLBACK;

	from = gGameViewInfoPtr->currentCameraCoords;
	from.z += 500;
	from.x -= 300;
	to = gGameViewInfoPtr->currentCameraLookAt;
	to.x -= 300;
	QD3D_UpdateCameraFromTo(gGameViewInfoPtr, &from, &to);	


		/* MAKE LETTERS */
		
	x = -500;		
	for (i = 0; i < 6; i++)
	{		
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;	
		gNewObjectDefinition.type 		= letters[i];	
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= 75;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.slot 		= 200;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= 1.3;
		gLetterObj[i] = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		if (i == 4)					// spacing before number
			x += 300;
		else
			x += 160;	
	}

	// Source port note: The game used to wait 1 second here before creating the boxerfly at x=-1500, dx=200.
	// But that made it pop suddenly into view in widescreen. Instead we're going to create the boxerfly a bit
	// further to the left and make it fly faster.

		/* MAKE THE BOXERFLY */
		
	gNewObjectDefinition.type 		= SKELETON_TYPE_BOXERFLY;
	gNewObjectDefinition.animNum 	= 0;							
	gNewObjectDefinition.coord.x 	= -2200;		// Source port change: up from -1500 (see above)
	gNewObjectDefinition.coord.y 	= 40;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveIntroBoxerfly;
	gNewObjectDefinition.rot 		= -PI/2;
	gNewObjectDefinition.scale 		= 1.0;
	fly = MakeNewSkeletonObject(&gNewObjectDefinition);			
		
	fly->Delta.x = 325;								// Source port change: up from 200 (see above)
	
	fly->Mode = 0;
		
	if (WaitAndDraw(10))
		return;		
}


/***************** MOVE INTRO BOXERFLY *******************/

static void MoveIntroBoxerfly(ObjNode *theNode)
{
float fps = gFramesPerSecondFrac;
ObjNode	*newObj;
TQ3Vector3D delta;

	GetObjectInfo(theNode);
	
	switch(theNode->Mode)
	{
		case	0:
				if (gCoord.x > -690.0f)
				{
					gCoord.x = -690.0f;
					gDelta.x = 0;
					
					if (theNode->Skeleton->AnimNum == 0)
						MorphToSkeletonAnim(theNode->Skeleton, 1, 4);
				}
				else
					gCoord.x += gDelta.x * fps;
					
					
						/* SEE IF PUNCH NOW */	
						
				if (theNode->Flag[0])		
				{
					theNode->Flag[0] = false;
					theNode->Mode = 1;
					
						/* MAKE CORRECT LETTER */
						
					gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;	
					gNewObjectDefinition.type 		= LINTRO_MObjType_L;	
					gNewObjectDefinition.coord		= gLetterObj[0]->Coord;
					gNewObjectDefinition.slot 		= 200;
					gNewObjectDefinition.flags 		= 0;
					gNewObjectDefinition.moveCall 	= nil;
					gNewObjectDefinition.rot 		= 0;
					gNewObjectDefinition.scale 		= 1.3;
					newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
						
					DeleteObject(gLetterObj[0]);	
					gLetterObj[0] = newObj;
					
						/* MAKE EXPLOSION */
						
					int32_t pg = NewParticleGroup(
											PARTICLE_TYPE_FALLINGSPARKS,	// type
											0,							// flags
											500,						// gravity
											0,							// magnetism
											25,							// base scale
											.2,							// decay rate
											.8,							// fade rate
											PARTICLE_TEXTURE_GREENRING);// texture
					
					if (pg != -1)
					{
						for (int i = 0; i < 70; i++)
						{
							delta.x = (RandomFloat()-.5f) * 1200.0f;
							delta.y = RandomFloat() * 1100.0f;
							delta.z = (RandomFloat()-.5f) * 1200.0f;
							AddParticleToGroup(pg, &newObj->Coord, &delta, RandomFloat() + 1.0f, FULL_ALPHA);
						}
					}
				}
				break;
	
	
				
		case	1:
				if (theNode->Skeleton->AnimHasStopped)
					MorphToSkeletonAnim(theNode->Skeleton, 0, 4);
				break;
	}
			
	UpdateObject(theNode);
	
	
			/* WOBBLE THE LETTER */
			
	if (gLetterObj[0]->Type == LINTRO_MObjType_L)
	{
		theNode = gLetterObj[0];
	
		theNode->Scale.x = theNode->Scale.z + sin(theNode->SpecialF[0] += fps * 5.0f) * .25f;
		theNode->Scale.y = theNode->Scale.z + cos(theNode->SpecialF[1] += fps * 8.0f) * .25f;
	
		UpdateObjectTransforms(theNode);
	} 
}



#pragma mark -

/*************** DO POND 1 INTRO *******************/

static void DoPond1Intro(void)
{
TQ3Point3D from,to;
ObjNode	*fish;
int		i;
float	x;
static Byte	letters[] = {LINTRO_MObjType_L, LINTRO_MObjType_E, LINTRO_MObjType_V, LINTRO_MObjType_E,
						LINTRO_MObjType_L, LINTRO_MObjType_3};

			/* SETUP CAMERA */
			
	gIntroCamMode = INTRO_CAM_MODE_PULLBACK;

	from = gGameViewInfoPtr->currentCameraCoords;
	from.z += 400;
	from.y += 100;
	to = gGameViewInfoPtr->currentCameraLookAt;
	to.y += 0;
	QD3D_UpdateCameraFromTo(gGameViewInfoPtr, &from, &to);	


		/**********************/
		/* CREATE THE LETTERS */
		/**********************/

	x = -500;		
	for (i = 0; i < 6; i++)
	{		
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;	
		gNewObjectDefinition.type 		= letters[i];	
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= 75;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.slot 		= 200;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= 1.3;
		gLetterObj[i] = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		if (i == 4)					// spacing before number
			x += 300;
		else
			x += 160;	
	}
	
		/*********************/
		/* FISH EATS LETTERS */
		/*********************/

	for (i = 0; i < 6; i++)
	{
			/* ANIMATE */
			
		if (WaitAndDraw(1.0))
			return;

			/* CREATE THE FISH */
			
		gNewObjectDefinition.type 		= SKELETON_TYPE_PONDFISH;
		gNewObjectDefinition.animNum 	= PONDFISH_ANIM_JUMPATTACK;							
		gNewObjectDefinition.coord.x 	= gLetterObj[i]->Coord.x - 900;
		gNewObjectDefinition.coord.y 	= -1800;
		gNewObjectDefinition.coord.z 	= -800;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.moveCall 	= MoveIntroFish;
		gNewObjectDefinition.rot 		= PI+.8;
		gNewObjectDefinition.scale 		= 2.5;
		fish = MakeNewSkeletonObject(&gNewObjectDefinition);			
		fish->Delta.x = 900;
		fish->Delta.y = 3800;
		fish->Delta.z = 800;			
		fish->Skeleton->AnimSpeed = .8;
		fish->SpecialL[0] = i;
			
	}

	WaitAndDraw(2.5);
}


/******************** MOVE INTRO FISH ***********************/

static void MoveIntroFish(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;
ObjNode	*l;

	GetObjectInfo(theNode);
	gDelta.y -= 3500.0f * fps;				// gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;
	
	if (gCoord.y < theNode->InitCoord.y)	// see if fallen out of range
	{
		DeleteObject(theNode);
		return;
	}
	
	UpdateObject(theNode);	


			/* SEE IF EAT LETTER */
			
	if (!theNode->Flag[2])
	{
		if (gCoord.z > -80.0f)
		{
			theNode->Flag[2] = true;
			i = theNode->SpecialL[0];
			theNode->ChainNode = gLetterObj[i];		// chain letter to fish
		}
	}
	
			/* UPDATE LETTER */
	else
	{
		float	s;
		TQ3Matrix4x4	m,m2,m3;
				
		l = theNode->ChainNode;
		
		s = l->Scale.x / theNode->Scale.x;						// compensate for fish's scale
		Q3Matrix4x4_SetScale(&m, s,s,s);
		FindJointFullMatrix(theNode,5,&m2);						// get matrix
		MatrixMultiplyFast(&m,&m2,&m3);		

		Q3Matrix4x4_SetTranslate(&m, 0, 100, 0); 				// get mouth offset	
		MatrixMultiplyFast(&m,&m3,&l->BaseTransformMatrix);		
	}
}




#pragma mark -

/*************** DO FOREST 1 INTRO *******************/

static void DoForest1Intro(void)
{
TQ3Point3D from,to;
int		i;
float	x;
static Byte	letters[] = {LINTRO_MObjType_L, LINTRO_MObjType_E, LINTRO_MObjType_V, LINTRO_MObjType_E,
						LINTRO_MObjType_L, LINTRO_MObjType_4};

			/* SETUP CAMERA */
			
	gIntroCamMode = INTRO_CAM_MODE_PULLBACK;

	from = gGameViewInfoPtr->currentCameraCoords;
	from.z += 400;
	from.y += 100;
	to = gGameViewInfoPtr->currentCameraLookAt;
	to.y += 0;
	QD3D_UpdateCameraFromTo(gGameViewInfoPtr, &from, &to);	


		/**********************/
		/* CREATE THE LETTERS */
		/**********************/

	x = -500;		
	for (i = 0; i < 6; i++)
	{		
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;	
		gNewObjectDefinition.type 		= letters[i];	
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= 75;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.slot 		= 200;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= 1.3;
		gLetterObj[i] = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		if (i == 4)					// spacing before number
			x += 300;
		else
			x += 160;	
	}
	
		/***********************/
		/* FOOT STOMPS LETTERS */
		/***********************/

	if (WaitAndDraw(1))
		return;

		/* CREATE THE FOOT */
		
	gNewObjectDefinition.type 		= SKELETON_TYPE_FOOT;
	gNewObjectDefinition.animNum 	= 0;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 1800;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveIntroFoot;
	gNewObjectDefinition.rot 		= -PI/2;
	gNewObjectDefinition.scale 		= 11.0;
	MakeNewSkeletonObject(&gNewObjectDefinition);			
			
	WaitAndDraw(4);
}


/******************** MOVE INTRO FOOT ***********************/

static void MoveIntroFoot(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;

	GetObjectInfo(theNode);
	
	switch(theNode->SpecialL[0])
	{
			/* MOVE FOOT DOWN */
			
		case	0:
				gDelta.y -= 3500.0f * fps;				// gravity
				gCoord.y += gDelta.y * fps;
				
						/* SEE IF LANDED */
						
				if (gCoord.y <= -30)
				{
					gDelta.y = 0;
					gCoord.y = -30;
					theNode->SpecialL[0] = 1;
					
					for (i = 0; i < 6; i++)						// squash letters
					{
						gLetterObj[i]->Scale.y *= .25f;		
						gLetterObj[i]->Coord.y -= 95.0f;
						UpdateObjectTransforms(gLetterObj[i]);		
					}
				}
				break;
	
			/* LANDED */
			
		case	1:
				theNode->SpecialF[0] += fps;
				if (theNode->SpecialF[0] > 1.0f)
				{
					theNode->SpecialL[0] = 2;
					SetSkeletonAnim(theNode->Skeleton, 1);
				}
				break;
				
			/* GO UP */
			
		case	2:
				gDelta.y += 3000 * fps;
				gCoord.y += gDelta.y * fps;
				break;
	}
	UpdateObject(theNode);	
}


#pragma mark -

/*************** DO FOREST 2 INTRO *******************/

static void DoForest2Intro(void)
{
TQ3Point3D from,to;

			/* SETUP CAMERA */
			
	gIntroCamMode = INTRO_CAM_MODE_PULLBACK;

	from = gGameViewInfoPtr->currentCameraCoords;
	from.z += 400;
	from.y += 200;
	to = gGameViewInfoPtr->currentCameraLookAt;
	to.y += 200;
	QD3D_UpdateCameraFromTo(gGameViewInfoPtr, &from, &to);	

	
		/* CREATE THE DRAGONFLY */
		
	gNewObjectDefinition.type 		= SKELETON_TYPE_DRAGONFLY;
	gNewObjectDefinition.animNum 	= 0;
	gNewObjectDefinition.coord.x 	= -900;
	gNewObjectDefinition.coord.y 	= 700;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveIntroDragonFly;
	gNewObjectDefinition.rot 		= -PI/2;
	gNewObjectDefinition.scale 		= 1.5;
	MakeNewSkeletonObject(&gNewObjectDefinition);			


	WaitAndDraw(10);
}


/******************** MOVE INTRO DRAGONFLY ***********************/

static void MoveIntroDragonFly(ObjNode *theNode)
{
static Byte	letters[] = {LINTRO_MObjType_L, LINTRO_MObjType_E, LINTRO_MObjType_V, LINTRO_MObjType_E,
						LINTRO_MObjType_L, LINTRO_MObjType_5};
float	fps = gFramesPerSecondFrac;
ObjNode	*letter,*chute;

	GetObjectInfo(theNode);
	
	gCoord.x += 400.0f * fps;


		/******************************/
		/* SEE IF DROP ANOTHER LETTER */
		/******************************/
		
	if (theNode->SpecialL[0] < 6)
	{
		theNode->SpecialF[0] += fps;
		if (theNode->SpecialF[0] > .5f)
		{
			if (theNode->SpecialL[0] == 5)
				theNode->SpecialF[0] = -.4;
			else
				theNode->SpecialF[0] = 0;
		
		
					/* CREATE LETTER */
		
			gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;	
			gNewObjectDefinition.type 		= letters[theNode->SpecialL[0]++];
			gNewObjectDefinition.coord		= gCoord;
			gNewObjectDefinition.coord.y 	-= 100;
			gNewObjectDefinition.slot 		= 200;
			gNewObjectDefinition.flags 		= 0;
			gNewObjectDefinition.moveCall 	= MoveParachuteLetter;
			gNewObjectDefinition.rot 		= 0;
			gNewObjectDefinition.scale 		= .3;
			letter = MakeNewDisplayGroupObject(&gNewObjectDefinition);
			
			letter->Delta.x = (RandomFloat()-.5f) * 60.0f;
			letter->Delta.z = (RandomFloat()-.5f) * 30.0f;


					/* CREATE PARACHUTE */

			gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;	
			gNewObjectDefinition.type 		= LINTRO_MObjType_Parachute;	
			gNewObjectDefinition.coord.y	+= 100;
			gNewObjectDefinition.slot++;
			gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES;
			gNewObjectDefinition.moveCall 	= nil;
			gNewObjectDefinition.rot 		= 0;
			gNewObjectDefinition.scale 		= .1;
			chute = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			letter->ChainNode = chute;
			
		}
	}

	UpdateObject(theNode);	
}

/***************** MOVE PARACHUTE LETTER ********************/

static void MoveParachuteLetter(ObjNode *letter)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*chute;

	GetObjectInfo(letter);
	gCoord.y -= 100 * fps;		
	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	
	if (letter->Scale.x < 1.1f)
	{
		letter->Scale.x += fps * 1.5f;
		if (letter->Scale.x > 1.1f)
			letter->Scale.x = 1.1f;
		letter->Scale.y = letter->Scale.z = letter->Scale.x;
	}
	
	UpdateObject(letter);
	
	
			/* CHUTE */
			
	chute = letter->ChainNode;
	
	chute->Coord.y = gCoord.y + 50.0f;
	chute->Coord.x = gCoord.x;
	chute->Coord.z = gCoord.z;
	if (chute->Scale.x < 1.0f)
	{
		chute->Scale.x += fps * 1.5f;
		if (chute->Scale.x > 1.0f)
			chute->Scale.x = 1.0f;
		chute->Scale.y = chute->Scale.z = chute->Scale.x;
	}
	
	chute->Rot.z = sin(chute->SpecialF[0] += fps * 6.0f) * .2f;
	
	UpdateObjectTransforms(chute);
	
}


#pragma mark -

/*************** DO NIGHT INTRO *******************/

static void DoNightIntro(void)
{
TQ3Point3D from,to;
ObjNode	*ant;
int		i;
float	x;

			/* SETUP CAMERA */
			
	gIntroCamMode = INTRO_CAM_MODE_PULLBACK;

	from = gGameViewInfoPtr->currentCameraCoords;
	from.z += 400;
	from.y += 100;
	to = gGameViewInfoPtr->currentCameraLookAt;
	to.y += 0;
	QD3D_UpdateCameraFromTo(gGameViewInfoPtr, &from, &to);	


		/********************/
		/* CREATE THE BOMBS */
		/********************/

	x = -500;		
	for (i = 0; i < 6; i++)
	{		
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;	
		gNewObjectDefinition.type 		= LINTRO_MObjType_CherryBomb;	
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= -50;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.slot 		= 200;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= .25;
		gLetterObj[i] = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		if (i == 4)					// spacing before number
			x += 300;
		else
			x += 160;	
	}
	
		/***************/
		/* MAKE AN ANT */
		/***************/
		
	gNewObjectDefinition.type 		= SKELETON_TYPE_FIREANT;
	gNewObjectDefinition.animNum 	= FIREANT_ANIM_FLY;							
	gNewObjectDefinition.coord.x 	= -1300;
	gNewObjectDefinition.coord.y 	= 450;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveIntroFireAnt;
	gNewObjectDefinition.rot 		= -PI/2;
	gNewObjectDefinition.scale 		= 2.0;
	ant = MakeNewSkeletonObject(&gNewObjectDefinition);			
	ant->Delta.x = 330;
	ant->BreathParticleGroup = -1;
			
	WaitAndDraw(10);
}


/******************** MOVE INTRO FIREANT ***********************/

static void MoveIntroFireAnt(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;
static Byte	letters[] = {LINTRO_MObjType_L, LINTRO_MObjType_E, LINTRO_MObjType_V, LINTRO_MObjType_E,
						LINTRO_MObjType_L, LINTRO_MObjType_8};

		/* MOVE THE ANT */
		
	GetObjectInfo(theNode);
	gCoord.x += gDelta.x * fps;
	UpdateObject(theNode);
	FireAntBreathFire(theNode);	


	/* SEE IF BLOW SOMETHING UP */
		
	for (i = 0; i < 6; i++)
	{		
		if (gLetterObj[i])							// see if not already blown up
		{
			if (gCoord.x >= (gLetterObj[i]->Coord.x-140))
			{
					/* CREATE LETTER HERE */
					
				gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;	
				gNewObjectDefinition.type 		= letters[i];	
				gNewObjectDefinition.coord		= gLetterObj[i]->Coord;
				gNewObjectDefinition.coord.y 	+= 85;
				gNewObjectDefinition.slot 		= 50;
				gNewObjectDefinition.flags 		= 0;
				gNewObjectDefinition.moveCall 	= nil;
				gNewObjectDefinition.rot 		= 0;
				gNewObjectDefinition.scale 		= 1.0;
				MakeNewDisplayGroupObject(&gNewObjectDefinition);


						/* EXPLODE BOMB */
						
				ExplodeFirecracker(gLetterObj[i], false);
				gLetterObj[i] = nil;
				
				
			}
		}
	}
}

#pragma mark -

/*************** DO HIVE 1 INTRO *******************/

#define	SEP	120

static void DoHive1Intro(void)
{
TQ3Point3D from,to;
ObjNode	*bee;
int		i;
float	x;
static const Byte	letters[] = {LINTRO_MObjType_L, LINTRO_MObjType_E, LINTRO_MObjType_V, LINTRO_MObjType_E,
						LINTRO_MObjType_L};

static const TQ3Point2D beeCoords[] =
{
	{SEP,     0},
	{SEP * 2, 0},
	{0,       SEP},
	{SEP * 3, SEP},
	{0,       SEP * 2},
	{SEP * 3, SEP * 2},
	{0,       SEP * 3},
	{SEP,     SEP * 3},
	{SEP * 2, SEP * 3},
	{0,       SEP * 4},
	{0,       SEP * 5},
	{SEP,     SEP * 6},
	{SEP * 2, SEP * 6},
};

			/* SETUP CAMERA */
			
	gIntroCamMode = INTRO_CAM_MODE_PULLBACK;

	from = gGameViewInfoPtr->currentCameraCoords;
	from.z += 400;
	from.y += 100;
	to = gGameViewInfoPtr->currentCameraLookAt;
	to.y += 0;
	QD3D_UpdateCameraFromTo(gGameViewInfoPtr, &from, &to);	


		/**********************/
		/* CREATE THE LETTERS */
		/**********************/

	x = -450;		
	for (i = 0; i < 5; i++)
	{		
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;	
		gNewObjectDefinition.type 		= letters[i];	
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= 75;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.slot 		= 200;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= 1.3;
		gLetterObj[i] = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		x += 160;	
	}
	
		/*************/
		/* MAKE BEES */
		/*************/

	gNewObjectDefinition.coord.z 	= 430;
		
	for (i = 0; i < 13; i++)
	{
		gNewObjectDefinition.type 		= SKELETON_TYPE_FLYINGBEE;
		gNewObjectDefinition.animNum 	= 0;							
		gNewObjectDefinition.coord.x 	= beeCoords[i].x - 200;
		gNewObjectDefinition.coord.y 	= beeCoords[i].y + 500;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.moveCall 	= MoveIntroBee;
		gNewObjectDefinition.rot 		= RandomFloat()*PI2;
		gNewObjectDefinition.scale 		= .7;
		bee = MakeNewSkeletonObject(&gNewObjectDefinition);			
		bee->Delta.y = -120;
		
		bee->Skeleton->CurrentAnimTime = RandomFloat()*bee->Skeleton->MaxAnimTime;
	}
			
	WaitAndDraw(15);
}


/****************** MOVE INTRO BEE *************************/

static void MoveIntroBee(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	gCoord.y += gDelta.y * fps;
	
	theNode->Rot.y += fps;
	
	UpdateObject(theNode);
}


#pragma mark -

/*************** DO ANT HILL 1 INTRO *******************/

static void DoAntHill1Intro(void)
{
TQ3Point3D from,to;
int		i;
float	x;
static Byte	letters[] = {LINTRO_MObjType_L, LINTRO_MObjType_E, LINTRO_MObjType_V, LINTRO_MObjType_E,
						LINTRO_MObjType_L, LINTRO_MObjType_9};

			/* SETUP CAMERA */
			
	gIntroCamMode = INTRO_CAM_MODE_PULLBACK;

	from = gGameViewInfoPtr->currentCameraCoords;
	from.z += 400;
	from.y += 100;
	to = gGameViewInfoPtr->currentCameraLookAt;
	to.y += 0;
	QD3D_UpdateCameraFromTo(gGameViewInfoPtr, &from, &to);	


		/**********************/
		/* CREATE THE LETTERS */
		/**********************/

	x = -500;		
	for (i = 0; i < 6; i++)
	{		
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;	
		gNewObjectDefinition.type 		= letters[i];	
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= 75;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.slot 		= 200;
		gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITE | STATUS_BIT_KEEPBACKFACES_2PASS;
		gNewObjectDefinition.moveCall 	= MoveAntHillLetter;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= 1.2;
		gLetterObj[i] = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		gLetterObj[i]->Health = -.5f - RandomFloat()*1.0f;
		MakeObjectTransparent(gLetterObj[i], 0);

		if (i == 4)					// spacing before number
			x += 300;
		else
			x += 160;	
	}
	

	WaitAndDraw(8);
}


/********************** MOVE ANT HILL LETTER ***********************/

static void MoveAntHillLetter(ObjNode *theNode)
{
float fps = gFramesPerSecondFrac;

	theNode->Health += fps * .5f;
	
	if (theNode->Health > 0.0f)
	{
		MakeObjectTransparent(theNode, theNode->Health);

		if (theNode->Health >= 1.0f)
		{
			theNode->StatusBits &= ~STATUS_BIT_NOZWRITE;
		}
	}
}






#pragma mark -


/******************** WAIT AND DRAW *********************/

static Boolean WaitAndDraw(float duration)
{
	do
	{
		UpdateInput();
		MoveObjects();
		MoveIntroCamera();
		MoveParticleGroups();
		QD3D_MoveShards();
		QD3D_DrawScene(gGameViewInfoPtr,IntroDrawStuff);
		QD3D_CalcFramesPerSecond();				
		DoSDLMaintenance();
		duration -= gFramesPerSecondFrac;
		
		if (GetSkipScreenInput())
			return(true);		
		
	}while(duration > 0.0f);

	return(false);
}

/***************** INTRO DRAW STUFF ******************/

static void IntroDrawStuff(const QD3DSetupOutputType *setupInfo)
{
	DrawObjects(setupInfo);
	QD3D_DrawShards(setupInfo);
	DrawParticleGroup(setupInfo);
}


/****************** MOVE INTRO CAMERA *********************/

static void MoveIntroCamera(void)
{
TQ3Point3D	from,to;
float		fps = gFramesPerSecondFrac;

	from = gGameViewInfoPtr->currentCameraCoords;
	to = gGameViewInfoPtr->currentCameraLookAt;


	switch(gIntroCamMode)
	{
		case	INTRO_CAM_MODE_RIGHTDRIFT:
				to.x += 20.0f * fps;
				from.y += 10.0f * fps;
				from.z += 5.0f * fps;
				break;
				
		case	INTRO_CAM_MODE_PULLBACK:
				from.z -= 10*fps;
				from.x += 10*fps;
				from.y += 5*fps;
				break;
	}
	
	
	QD3D_UpdateCameraFromTo(gGameViewInfoPtr, &from, &to);	
}



