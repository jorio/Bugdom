/****************************/
/*   	MISCSCREENS.C	    */
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

static void MovePangeaLogoPart(ObjNode *theNode);
static void MoveLogoBG(ObjNode *theNode);



/****************************/
/*    CONSTANTS             */
/****************************/

#define NUM_PAUSE_TEXTURES 4


/*********************/
/*    VARIABLES      */
/*********************/

TQ3TriMeshData* gPauseQuad = nil;



/************************ DO PAUSED ******************************/

void DoPaused(void)
{
float		dummy;
const float imageAspectRatio = 200.0f/152.0f;

	Pomme_PauseAllChannels(true);

	SDL_ShowCursor(1);

	gGammaFadeFactor = 1.0f;

			/* PRELOAD TEXTURES */

	GLuint textures[NUM_PAUSE_TEXTURES];
	for (int i = 0; i < NUM_PAUSE_TEXTURES; i++)
		textures[i] = QD3D_LoadTextureFile(1500+i, kRendererTextureFlags_ClampBoth);

			/* CREATE MESH */

	GAME_ASSERT_MESSAGE(!gPauseQuad, "Pause quad already created!");

	gPauseQuad = MakeQuadMesh(1, 1.0f, 1.0f);
	gPauseQuad->hasVertexNormals = false;
	gPauseQuad->texturingMode = kQ3TexturingModeOpaque;
	gPauseQuad->glTextureName = textures[3];

	float xs = .4f;
	float ys = xs/imageAspectRatio;

	gPauseQuad->points[0] = (TQ3Point3D) { -xs, -ys, 0 };
	gPauseQuad->points[1] = (TQ3Point3D) { +xs, -ys, 0 };
	gPauseQuad->points[2] = (TQ3Point3D) { +xs, +ys, 0 };
	gPauseQuad->points[3] = (TQ3Point3D) { -xs, +ys, 0 };

			/*******************/
			/* LET USER SELECT */
			/*******************/

	int curState = 3;
	while (1)
	{
		int rawMouseX, rawMouseY;
		SDL_GetMouseState(&rawMouseX, &rawMouseY);

		TQ3Point3D mouse = {rawMouseX, rawMouseY, 0};
		Q3Point3D_Transform(&mouse, &gWindowToFrustumCorrectAspect, &mouse);

		int newState = 3; // empty
		if      (mouse.x < -xs) newState = 3;		// empty
		else if (mouse.x > +xs) newState = 3;		// empty
		else if (mouse.y > ys*+.15f) newState = 3;	// empty
		else if (mouse.y > ys*-.17f) newState = 0;	// Resume
		else if (mouse.y > ys*-.47f) newState = 1;	// End Game
		else if (mouse.y > ys*-.81f) newState = 2;	// Quit
		else newState = 3;

		if (curState != newState)
		{
			curState = newState;
			GAME_ASSERT(curState >= 0 && curState < NUM_PAUSE_TEXTURES);
			gPauseQuad->glTextureName = textures[newState];
			if (curState != 3)
				PlayEffect(EFFECT_SELECT);
		}

		UpdateInput();
		if (GetNewKeyState(kKey_Pause))						// see if ESC out
		{
			curState = 0;
			break;
		}

		QD3D_DrawScene(gGameViewInfoPtr, DrawTerrain);
		DoSDLMaintenance();
		SDL_Delay(10);

		if (FlushMouseButtonPress() && newState != 3)		// ensure mouse is within frame to proceed
		{
			break;
		}
	}
	while(FlushMouseButtonPress());							// wait for button up

			/* FREE MESH/TEXTURES */

	glDeleteTextures(NUM_PAUSE_TEXTURES, textures);
	Q3TriMeshData_Dispose(gPauseQuad);
	gPauseQuad = nil;

			/* HANDLE RESULT */

	switch (curState)
	{
		case	1:
				gGameOverFlag = true;
				break;
				
		case	2:
				CleanQuit();
	
	}

			/* CLEANUP */
			
	QD3D_CalcFramesPerSecond();
	QD3D_CalcFramesPerSecond();
	
	gPlayerObj->AccelVector.x =					// neutralize this
	gPlayerObj->AccelVector.y = 0;

	GetMouseDelta(&dummy,&dummy);				// read this once to prevent mouse boom

	Pomme_PauseAllChannels(false);
}



#pragma mark -

/********************** DO PANGEA LOGO **********************************/

void DoPangeaLogo(void)
{
OSErr		err;
ObjNode		*backObj;
TQ3Point3D			cameraFrom = { 0, 0, 70.0 };
QD3DSetupInputType		viewDef;
FSSpec			spec;
TQ3ColorRGB				lightColor = { 1.0, 1.0, .9 };
TQ3Vector3D				fillDirection1 = { 1, -.4, -.8 };			// key
TQ3Vector3D				fillDirection2 = { -.7, -.2, -.9 };			// fill

float			fotime = 0;
Boolean			fo = false;

	GameScreenToBlack();

			/* MAKE VIEW */

	QD3D_NewViewDef(&viewDef);
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 2000;
	viewDef.camera.fov 				= .9;
	viewDef.camera.from				= cameraFrom;
	viewDef.view.clearColor			= TQ3ColorRGBA_FromInt(0x000000FF);
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
			
	err = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:Pangea.3dmf", &spec);		// load other models
	GAME_ASSERT(err == noErr);
	LoadGrouped3DMF(&spec,MODEL_GROUP_TITLE);	




			/***************/
			/* MAKE MODELS */
			/***************/
				
			/* BACKGROUND */
			
	gNewObjectDefinition.group 		= MODEL_GROUP_PANGEA;	
	gNewObjectDefinition.type 		= 1;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= -180;	
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER;
	gNewObjectDefinition.slot 		= 50;
	gNewObjectDefinition.moveCall 	= MoveLogoBG;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 10;
	MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* LOGO */
		
	gNewObjectDefinition.group 		= MODEL_GROUP_PANGEA;	
	gNewObjectDefinition.type 		= 0;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= -100;	
	gNewObjectDefinition.flags 		= STATUS_BIT_REFLECTIONMAP;
	gNewObjectDefinition.slot 		= 50;
	gNewObjectDefinition.moveCall 	= MovePangeaLogoPart;
	gNewObjectDefinition.rot 		= PI/2;
	gNewObjectDefinition.scale 		= .18;
	backObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	
	
	MakeFadeEvent(true);	
	
			/*************/
			/* MAIN LOOP */
			/*************/
			
	PlaySong(SONG_PANGEA,false);			
	QD3D_CalcFramesPerSecond();					

	while((!gResetSong) && gSongPlayingFlag)					// wait until song stops
	{
		QD3D_CalcFramesPerSecond();					
	
		UpdateInput();
		if (GetSkipScreenInput())
		{
			if (!fo)
				GammaFadeOut();
			break;
		}

		MoveObjects();

		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);	

		fotime += gFramesPerSecondFrac;
		if ((fotime > 7.0f) && (!fo))
		{
			MakeFadeEvent(false);
			fo = true;
		}		

		DoSDLMaintenance();
	}


			/***********/
			/* CLEANUP */
			/***********/
			
	DeleteAllObjects();
	Free3DMFGroup(MODEL_GROUP_TITLE);
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);		
	KillSong();
	GameScreenToBlack();
}


/********************* MOVE PANGEA LOGO PART ***************************/

static void MovePangeaLogoPart(ObjNode *theNode)
{
//	theNode->Coord.z += gFramesPerSecondFrac * 45;		
	theNode->Rot.y -= gFramesPerSecondFrac * .3;		
	UpdateObjectTransforms(theNode);
}


/****************** MOVE LOGO BG ******************/

static void MoveLogoBG(ObjNode *theNode)
{
	theNode->Coord.z += 15.0f * gFramesPerSecondFrac;
	UpdateObjectTransforms(theNode);
}
