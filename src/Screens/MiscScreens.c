/****************************/
/*   	MISCSCREENS.C	    */
/* By Brian Greenstone      */
/* (c)1999 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
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



/****************** ADJUST CAMERA WHILE PAUSED **********************/

static void CheckPauseCameraKeys(void)
{
	const float fadeSpeed = 3.0f;
	const float pauseDelay = -10 * fadeSpeed;

	if (GetKeyState(kKey_SwivelCameraLeft))
	{
		gPauseQuad->diffuseColor.a = pauseDelay;
		gCameraControlDelta.x -= 2.0f;
	}
	else if (GetKeyState(kKey_SwivelCameraRight))
	{
		gPauseQuad->diffuseColor.a = pauseDelay;
		gCameraControlDelta.x += 2.0f;
	}

	if (GetKeyState(kKey_ZoomIn))
	{
		gPauseQuad->diffuseColor.a = pauseDelay;
	}
	else if (GetKeyState(kKey_ZoomOut))
	{
		gPauseQuad->diffuseColor.a = pauseDelay;
	}

	if (gPauseQuad->diffuseColor.a < 1)
	{
		UpdateCamera();
		if (gCyclorama && gCyclorama->MoveCall)
			gCyclorama->MoveCall(gCyclorama);
		gPauseQuad->diffuseColor.a += gFramesPerSecondFrac * fadeSpeed;
		if (gPauseQuad->diffuseColor.a > 1.0f)
			gPauseQuad->diffuseColor.a = 1.0f;
	}
}


/************************ DO PAUSED ******************************/

void DoPaused(void)
{
enum
{
	kPauseChoice_Resume,
	kPauseChoice_Bail,
	kPauseChoice_Quit,
	kPauseChoice_Null,
};

float		dummy;
const float imageAspectRatio = 200.0f/152.0f;
int curState = kPauseChoice_Resume;


	PauseAllChannels(true);

	SDL_ShowCursor(1);

	gGammaFadeFactor = 1.0f;
	
	gIsGamePaused = true;

			/* PRELOAD TEXTURES */

	GLuint textures[NUM_PAUSE_TEXTURES];
	int loadFlags = kRendererTextureFlags_ClampBoth;
#if OSXPPC
	loadFlags |= kRendererTextureFlags_ForcePOT;
#endif
	for (int i = 0; i < NUM_PAUSE_TEXTURES; i++)
		textures[i] = QD3D_LoadTextureFile(1500+i, loadFlags);

			/* CREATE MESH */

	GAME_ASSERT_MESSAGE(!gPauseQuad, "Pause quad already created!");

	gPauseQuad = MakeQuadMesh(1, 1.0f, 1.0f);
	gPauseQuad->hasVertexNormals = false;
	gPauseQuad->texturingMode = kQ3TexturingModeAlphaBlend;
	gPauseQuad->glTextureName = textures[curState];		// resume
	gPauseQuad->diffuseColor = (TQ3ColorRGBA) {1, 1, 1, 1};

	float xs = .4f;
	float ys = xs/imageAspectRatio;

	gPauseQuad->points[0] = (TQ3Point3D) { -xs, -ys, 0 };
	gPauseQuad->points[1] = (TQ3Point3D) { +xs, -ys, 0 };
	gPauseQuad->points[2] = (TQ3Point3D) { +xs, +ys, 0 };
	gPauseQuad->points[3] = (TQ3Point3D) { -xs, +ys, 0 };

#if OSXPPC
	gPauseQuad->vertexUVs[1].u = gPauseQuad->vertexUVs[2].u = 200.0f / POTCeil32(200);
	gPauseQuad->vertexUVs[0].v = gPauseQuad->vertexUVs[1].v = 152.0f / POTCeil32(152);
#endif

			/*******************/
			/* LET USER SELECT */
			/*******************/


	TQ3Point2D prevMousePos = GetMousePosition();

	bool ignoreThumbstick = true;

	while (1)
	{
		QD3D_DrawScene(gGameViewInfoPtr, DrawTerrain);
		DoSDLMaintenance();
		UpdateInput();
		SDL_Delay(10);


		if (GetNewKeyState(kKey_Pause) || GetNewKeyState(kKey_UI_Cancel))						// see if ESC out
		{
			curState = kPauseChoice_Resume;
			break;
		}

		if (gPauseQuad->diffuseColor.a >= 0.9f)
		{
			TQ3Point2D newMousePos = GetMousePosition();
			bool mouseMoved = prevMousePos.x != newMousePos.x || prevMousePos.y != newMousePos.y;

			int newState = curState;

			if (mouseMoved)
			{
				TQ3Point3D mouse = {newMousePos.x, newMousePos.y, 0};
				Q3Point3D_Transform(&mouse, &gWindowToFrustumCorrectAspect, &mouse);

				if      (mouse.x < -xs) newState = kPauseChoice_Null;
				else if (mouse.x > +xs) newState = kPauseChoice_Null;
				else if (mouse.y > ys*+.15f) newState = kPauseChoice_Null;
				else if (mouse.y > ys*-.17f) newState = kPauseChoice_Resume;
				else if (mouse.y > ys*-.47f) newState = kPauseChoice_Bail;
				else if (mouse.y > ys*-.81f) newState = kPauseChoice_Quit;
				else newState = kPauseChoice_Null;

				prevMousePos = newMousePos;
			}

			if (!mouseMoved || newState == kPauseChoice_Null)
			{
				int delta = 0;

				TQ3Vector2D thumb = GetThumbStickVector(false);
				if (!ignoreThumbstick)
				{
					if (fabsf(thumb.y) > 0.5f)
					{
						delta = (thumb.y < 0) ? -1 : 1;
						ignoreThumbstick = true;
					}
				}
				else
				{
					if (fabsf(thumb.y) < 0.5f)
					{
						ignoreThumbstick = false;
					}
				}

				if (GetNewKeyState(kKey_Forward))
					delta = -1;

				if (GetNewKeyState(kKey_Backward))
					delta = 1;

				if (delta != 0)
				{
					if (newState == kPauseChoice_Null)
						newState = kPauseChoice_Resume;
					else
					{
						newState += delta;
						if (newState < 0)
							newState = 0;
						else if (newState > kPauseChoice_Quit)
							newState = kPauseChoice_Quit;
					}
				}
			}

			if (curState != newState)
			{
				curState = newState;
				GAME_ASSERT(curState >= 0 && curState < NUM_PAUSE_TEXTURES);
				gPauseQuad->glTextureName = textures[newState];
				if (curState != 3)
					PlayEffect(EFFECT_SELECT);
			}

			// Confirm selection
			if (newState != kPauseChoice_Null		// ensure mouse is within frame to proceed
				&& (FlushMouseButtonPress() || GetNewKeyState(kKey_UI_Confirm) || GetNewKeyState(kKey_UI_PadConfirm)))
			{
				break;
			}
		}

		// Check </> keys to prepare good-looking screenshots
		CheckPauseCameraKeys();
	}
	while(FlushMouseButtonPress());							// wait for button up

	ResetInputState();

	gIsGamePaused = false;

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


	SDL_ShowCursor(0);


			/* CLEANUP */
			
	QD3D_CalcFramesPerSecond();
	QD3D_CalcFramesPerSecond();
	
	gPlayerObj->AccelVector.x =					// neutralize this
	gPlayerObj->AccelVector.y = 0;

	GetMouseDelta(&dummy,&dummy);				// read this once to prevent mouse boom

	PauseAllChannels(false);
}



#pragma mark -

/********************** DO PANGEA LOGO **********************************/

void DoPangeaLogo(void)
{
OSErr		err;
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
			
	err = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Pangea.3dmf", &spec);		// load other models
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
	MakeNewDisplayGroupObject(&gNewObjectDefinition);
	
	
	MakeFadeEvent(true);	
	
			/*************/
			/* MAIN LOOP */
			/*************/
			
	PlaySong(SONG_PANGEA,false);			
	QD3D_CalcFramesPerSecond();					

	while (gSongPlayingFlag)					// wait until song stops
	{
		QD3D_CalcFramesPerSecond();					
	
		UpdateInput();
		if (GetSkipScreenInput())
		{
			if (!fo)
				GammaFadeOut(true);
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
	Pomme_FlushPtrTracking(true);
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
