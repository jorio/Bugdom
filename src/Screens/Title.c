/****************************/
/*   		TITLE.C		    */
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

static void SetupTitleScreen(void);
static Boolean TitleWaitAndDraw(float duration);
static void TitleDrawStuff(const QD3DSetupOutputType *setupInfo);
static void MoveTitleCamera(void);
static void MoveBugdomName(ObjNode *theNode);
static void MoveTitleRollie(ObjNode *theNode);
static void MoveTitleAnt(ObjNode *ant);


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	TITLE_MObjType_Tree,
	TITLE_MObjType_Background,
	TITLE_MObjType_B,
	TITLE_MObjType_U,
	TITLE_MObjType_G,
	TITLE_MObjType_D,
	TITLE_MObjType_O,
	TITLE_MObjType_M
};

/*********************/
/*    VARIABLES      */
/*********************/

static TQ3Point3D	gCameraFrom = {0, 80, 300 };
static TQ3Point3D	gCameraTo = {0, 140, 0 };

static	ObjNode *gLadyBug;

/********************** DO TITLE SCREEN *************************/

void DoTitleScreen(void)
{
			/*********/
			/* SETUP */
			/*********/
			
	PlaySong(SONG_MENU,true);

	SetupTitleScreen();
	QD3D_CalcFramesPerSecond();
	QD3D_CalcFramesPerSecond();
	
	
	if (TitleWaitAndDraw(14))
		goto bail;
	MakeFadeEvent(false);
	TitleWaitAndDraw(1.3);
	
	
			/* CLEANUP */

bail: 
	GammaFadeOut(false);
	GameScreenToBlack();
	DeleteAllObjects();
	DeleteAllParticleGroups();
	FreeAllSkeletonFiles(-1);
	DeleteAll3DMFGroups();
	QD3D_DisposeShards();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);		
	Pomme_FlushPtrTracking(true);
}



/****************** SETUP TITLE SCREEN **************************/

static void SetupTitleScreen(void)
{
int						i;
FSSpec					spec;
QD3DSetupInputType		viewDef;
TQ3ColorRGB				lightColor = { 1.0, 1.0, .9 };
TQ3Vector3D				fillDirection1 = { .3, -.6, -1 };			// key
TQ3Vector3D				fillDirection2 = { -.7, -.2, -.9 };			// fill
ObjNode					*bugdom,*ant1,*rollie;
Byte					letters[] = {TITLE_MObjType_B, TITLE_MObjType_U,
									TITLE_MObjType_G, TITLE_MObjType_D,
									TITLE_MObjType_O, TITLE_MObjType_M};

		/* INIT OTHER SYSTEMS */

	QD3D_InitShards();


			/*************/
			/* MAKE VIEW */
			/*************/

	QD3D_NewViewDef(&viewDef);
	
	viewDef.camera.hither 			= 50;
	viewDef.camera.yon 				= 3000;
	viewDef.styles.usePhong 		= false;
	viewDef.camera.from 			= gCameraFrom;
	viewDef.camera.to	 			= gCameraTo;
	viewDef.camera.fov 				= .9;
	
	viewDef.lights.numFillLights 	= 1;
	viewDef.lights.ambientBrightness = 0.3;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[0] 	= lightColor;
	viewDef.lights.fillColor[1] 	= lightColor;
	viewDef.lights.fillBrightness[0] = 1.1;
	viewDef.lights.fillBrightness[1] = .2;


	viewDef.view.clearColor = TQ3ColorRGBA_FromInt(0x295a8cff);


	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);

			/************/
			/* LOAD ART */
			/************/

	InitParticleSystem();		// Must be once we have a valid GL context

	LoadASkeleton(SKELETON_TYPE_ME);
	LoadASkeleton(SKELETON_TYPE_FIREANT);
	LoadASkeleton(SKELETON_TYPE_LADYBUG);
 
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Title.3dmf", &spec);
	LoadGrouped3DMF(&spec,MODEL_GROUP_LEVELINTRO);	

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Global_Models1.3dmf", &spec);
	LoadGrouped3DMF(&spec,MODEL_GROUP_GLOBAL1);	
	

			/*******************/
			/* MAKE BACKGROUND */
			/*******************/
			
			/* TREE & SCENE */
				
	gNewObjectDefinition.group 		= MODEL_GROUP_TITLE;	
	gNewObjectDefinition.type 		= TITLE_MObjType_Tree;	
	gNewObjectDefinition.coord.x	= 0;
	gNewObjectDefinition.coord.y	= 0;
	gNewObjectDefinition.coord.z	= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1;
	gNewObjectDefinition.drawOrder	= kDrawOrder_Terrain;
	MakeNewDisplayGroupObject(&gNewObjectDefinition);

			/* SPHERE */
				
	gNewObjectDefinition.group 		= MODEL_GROUP_TITLE;	
	gNewObjectDefinition.type 		= TITLE_MObjType_Background;	
	gNewObjectDefinition.coord.x	= 0;
	gNewObjectDefinition.coord.y	= 0;
	gNewObjectDefinition.coord.z	= -600;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER | STATUS_BIT_NOFOG | STATUS_BIT_DONTCULL | STATUS_BIT_NOZWRITE;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.7;
	gNewObjectDefinition.drawOrder	= kDrawOrder_Terrain-1;
	ObjNode* cyc = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	QD3D_MirrorMeshesZ(cyc);

	
			/* LADYBUG */
		
	gNewObjectDefinition.type 		= SKELETON_TYPE_LADYBUG;
	gNewObjectDefinition.animNum 	= 2;							
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.z 	= -550;
	gNewObjectDefinition.coord.y 	= 100;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 2.9;
	gNewObjectDefinition.scale 		= .9;
	gNewObjectDefinition.drawOrder	= kDrawOrder_Default;
	gLadyBug = MakeNewSkeletonObject(&gNewObjectDefinition);			

	AttachShadowToObject(gLadyBug, 6,6, false);
	
	gLadyBug->ShadowNode->StatusBits |= STATUS_BIT_NOTRICACHE;
	gLadyBug->ShadowNode->Coord.y = 1;
	UpdateObjectTransforms(gLadyBug->ShadowNode);
	
	
			/* FIREANT */
			
	gNewObjectDefinition.coord.z 	-= 50;
	gNewObjectDefinition.type 		= SKELETON_TYPE_FIREANT;
	gNewObjectDefinition.animNum 	= FIREANT_ANIM_FLY;							
	gNewObjectDefinition.coord.y 	= 1000;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot--;
	gNewObjectDefinition.moveCall 	= MoveTitleAnt;
	gNewObjectDefinition.rot 		= 2.9;
	gNewObjectDefinition.scale 		= 1.0;
	ant1 = MakeNewSkeletonObject(&gNewObjectDefinition);			
	ant1->Delta.y = -400;

			/***************/
			/* BUGDOM NAME */
			/***************/

	gNewObjectDefinition.coord.x 	= 300;
	gNewObjectDefinition.coord.y 	= 455;
	gNewObjectDefinition.coord.z 	= -405;

	for (i = 0; i < 6; i++)
	{
		float	dx = cos(.12) * 105.0f;
		float	dy = sin(.12) * 105.0f;
	
		gNewObjectDefinition.group 		= MODEL_GROUP_TITLE;	
		gNewObjectDefinition.type 		= letters[i];	
		gNewObjectDefinition.coord.x 	+= dx;
		gNewObjectDefinition.coord.y 	+= dy;
		gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.moveCall 	= MoveBugdomName;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= 3.4;
		bugdom = MakeNewDisplayGroupObject(&gNewObjectDefinition);	
		bugdom->Mode = 0;
		bugdom->SpecialF[0] = (5 - i) * .15f;		
		bugdom->Rot.z = .12;
		bugdom->Flag[3] = 0;			// hop counter
	}
	
		/****************/
		/* ROLLIE MCFLY */
		/****************/
	
	gNewObjectDefinition.type 		= SKELETON_TYPE_ME;
	gNewObjectDefinition.animNum 	= PLAYER_ANIM_WALK;							
	gNewObjectDefinition.coord.x 	= -600 * 1.6;
	gNewObjectDefinition.coord.y 	= 1;
	gNewObjectDefinition.coord.z 	= -850 * 1.6;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveTitleRollie;
	gNewObjectDefinition.rot 		= -2.3;
	gNewObjectDefinition.scale 		= 1.7;
	rollie = MakeNewSkeletonObject(&gNewObjectDefinition);			
	
	rollie->Delta.x = -sin(rollie->Rot.y) * 220.0f;
	rollie->Delta.z = -cos(rollie->Rot.y) * 220.0f;
	
	rollie->Skeleton->AnimSpeed = 2.0;
	
	AttachShadowToObject(rollie, 8,8, false);
	
		/* FADE EVENT */
		
	MakeFadeEvent(true);
}


/********************* MOVE TITLE ROLLIE *********************/

static void MoveTitleRollie(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	switch(theNode->Mode)
	{
		case	0:
				gCoord.x += gDelta.x * fps;
				gCoord.z += gDelta.z * fps;
				
				theNode->SpecialF[0] += fps;			
				if (theNode->SpecialF[0] >= 5.5f)			// see if lookup
				{
					MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_LOOKUP, 3);
					theNode->Mode = 1;
					theNode->SpecialF[0] = 0;
				}
				else										// see if slow
				if (theNode->SpecialF[0] >= 4.5f)
				{
					ApplyFrictionToDeltas(90.0f * fps, &gDelta);
				}
				break;
				
		case	1:
				theNode->SpecialF[0] += fps;			
				if (theNode->SpecialF[0] >= 4.0f)			// see if continue
				{
					theNode->Mode = 2;
					MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_ROLLUP, 9);
				}
				break;
				
		case	2:
				if (theNode->Skeleton->AnimHasStopped)		// see if done rolling up
				{
					gPlayerMode = PLAYER_MODE_BUG;
					InitPlayer_Ball(theNode, &gCoord);
					theNode = gPlayerObj;
					theNode->MoveCall = MoveTitleRollie;
					theNode->Mode = 3;				
					goto update_shadow;
				}
				break;				
				
		case	3:
				gDelta.x += -sin(theNode->Rot.y) * 1300.0f * fps;
				gDelta.z += -cos(theNode->Rot.y) * 1300.0f * fps;
				gCoord.x += gDelta.x * fps;
				gCoord.z += gDelta.z * fps;
				theNode->Speed = sqrt(gDelta.x * gDelta.x + gDelta.z * gDelta.z);
				
				theNode->Rot.x -= theNode->Speed * .02f * fps;				
				break;
	}

	UpdateObject(theNode);
	
update_shadow:	

	theNode->ShadowNode->Scale.x = theNode->ShadowNode->Scale.y = theNode->ShadowNode->Scale.z = 6.0;
	theNode->ShadowNode->Coord.y = 3;
	theNode->ShadowNode->Coord.x = gCoord.x;
	theNode->ShadowNode->Coord.z = gCoord.z;
	UpdateObjectTransforms(theNode->ShadowNode);
}



/***************** MOVE BUGDOM NAME **********************/

static void MoveBugdomName(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	s;

	GetObjectInfo(theNode);

	switch(theNode->Mode)
	{
			/* WAIT MODE */
			
		case	0:
				theNode->SpecialF[0] -= fps;
				if (theNode->SpecialF[0] <= 0.0f)
				{
					if (theNode->Flag[3] < 15)			// see if done
					{
						theNode->Flag[3]++;
						theNode->Mode = 1;
						theNode->SpecialF[0] = 0;
						theNode->InitCoord = gCoord;
					}
				}
				break;
				
		case	1:
				theNode->SpecialF[0] += fps * 3.5f;
				if (theNode->SpecialF[0] > PI/2)
					theNode->SpecialF[0] = PI/2;
				s = sin(theNode->SpecialF[0]) * 45.0f;
				
				gCoord.x = theNode->InitCoord.x - cos(theNode->Rot.z) * s;
				gCoord.y = theNode->InitCoord.y - sin(theNode->Rot.z) * s;
				
				gCoord.y += sin(theNode->SpecialF[0] * 2.0f) * 20.0f;
				
				theNode->Scale.y = theNode->Scale.x + sin(theNode->SpecialF[0] * 2.0f) * .6f;
				
				if (theNode->SpecialF[0] == PI/2)
				{
					theNode->Mode = 0;
					theNode->SpecialF[0] = .2;
				}

	}

	UpdateObject(theNode);
}


/******************* MOVE TITLE ANT ********************/

static void MoveTitleAnt(ObjNode *ant)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*shadow;

	GetObjectInfo(ant);
	

	switch(ant->Mode)
	{
				/* ATTACK */
				
		case	0:		
				gDelta.y += 90.0f * fps;				
				gCoord.y += gDelta.y * fps;					// move down
				if (gCoord.y < 200.0f)						// see if there
				{
					gCoord.y = 200.0f;
					ant->Mode = 1;							// next mode				
					MorphToSkeletonAnim(gLadyBug->Skeleton, 4, 3);
					gDelta.y = 0;
				}
				break;
				
					
				/* LIFT LADYBUG */
					
		case	1:
				gDelta.y += 60.0f * fps;
		
				gCoord.y += gDelta.y * fps;					// move up and away
				gLadyBug->Coord.y += gDelta.y * fps;
				UpdateObjectTransforms(gLadyBug);
								
						/* SHRINK SHADOW */
						
				shadow = gLadyBug->ShadowNode;
				if (shadow)
				{
					shadow->Scale.x -= 1.9f * fps;
					if (shadow->Scale.x <= 0.0f)
					{
						DeleteObject(shadow);
						gLadyBug->ShadowNode = nil;
					}	
					else
					{
						shadow->Scale.z = shadow->Scale.x;
						UpdateObjectTransforms(shadow);					
					}
				}
				
				break;
	}
	
	UpdateObject(ant);
}



#pragma mark -



/******************** WAIT AND DRAW *********************/

static Boolean TitleWaitAndDraw(float duration)
{
	do
	{
		UpdateInput();
		MoveObjects();
		MoveTitleCamera();
		MoveParticleGroups();
		QD3D_MoveShards();
		QD3D_DrawScene(gGameViewInfoPtr,TitleDrawStuff);
		QD3D_CalcFramesPerSecond();				
		DoSDLMaintenance();
		duration -= gFramesPerSecondFrac;
		
		if (GetSkipScreenInput())
			return(true);		
		
	}while(duration > 0.0f);

	return(false);
}

/***************** TITLE DRAW STUFF ******************/

static void TitleDrawStuff(const QD3DSetupOutputType *setupInfo)
{
	DrawObjects(setupInfo);
	QD3D_DrawShards(setupInfo);
	DrawParticleGroup(setupInfo);
}


/****************** MOVE TITLE CAMERA *********************/

static void MoveTitleCamera(void)
{
TQ3Point3D	from,to;
float		fps = gFramesPerSecondFrac;

	from = gGameViewInfoPtr->currentCameraCoords;
	to = gGameViewInfoPtr->currentCameraLookAt;

	to.y 	+= 6.0f * fps;
	from.z 	-= 5.0f * fps;
	from.y  += 8.0f * fps;
	
	QD3D_UpdateCameraFromTo(gGameViewInfoPtr, &from, &to);	
}









