/****************************/
/*   	QD3D SUPPORT.C	    */
/* By Brian Greenstone      */
/* (c)1997-99 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <SDL_opengl.h>
#include <stdio.h>
#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void CreateLights(QD3DLightDefType *lightDefPtr);


/****************************/
/*    CONSTANTS             */
/****************************/

static const int kDebugTextMeshQuadCapacity = 1024;


/*********************/
/*    VARIABLES      */
/*********************/


RenderStats						gRenderStats;

int								gWindowWidth				= GAME_VIEW_WIDTH;
int								gWindowHeight				= GAME_VIEW_HEIGHT;

float	gFramesPerSecond = MIN_FPS;				// this is used to maintain a constant timing velocity as frame rates differ
float	gFramesPerSecondFrac = 1.0f/MIN_FPS;



		/* DEBUG STUFF */
		
static TQ3Point3D		gNormalWhere;
static TQ3Vector3D		gNormal;

static TQ3TriMeshData*	gDebugTextMesh = nil;
static TQ3TriMeshData*	gPillarboxMesh = nil;



//=======================================================================================================
//=============================== VIEW WINDOW SETUP STUFF ===============================================
//=======================================================================================================


/*********************** QD3D: NEW VIEW DEF **********************/
//
// fills a view def structure with default values.
//

void QD3D_NewViewDef(QD3DSetupInputType *viewDef)
{
TQ3ColorRGBA		clearColor = {0,0,0,1};
TQ3Point3D			cameraFrom = { 0, 40, 200.0 };
TQ3Point3D			cameraTo = { 0, 0, 0 };
TQ3Vector3D			cameraUp = { 0.0, 1.0, 0.0 };
TQ3ColorRGB			ambientColor = { 1.0, 1.0, .8 };
TQ3Vector3D			fillDirection1 = { 1, -1, .3 };
TQ3Vector3D			fillDirection2 = { -.8, .8, -.2 };

	Q3Vector3D_Normalize(&fillDirection1,&fillDirection1);
	Q3Vector3D_Normalize(&fillDirection2,&fillDirection2);

	viewDef->view.clearColor 		= clearColor;
	viewDef->view.paneClip.left 	= 0;
	viewDef->view.paneClip.right 	= 0;
	viewDef->view.paneClip.top 		= 0;
	viewDef->view.paneClip.bottom 	= 0;
	viewDef->view.dontClear			= false;

	viewDef->styles.interpolation 	= kQ3InterpolationStyleVertex; 
	viewDef->styles.backfacing 		= kQ3BackfacingStyleRemove; 
	viewDef->styles.fill			= kQ3FillStyleFilled; 
	viewDef->styles.usePhong 		= false; 

	viewDef->camera.from 			= cameraFrom;
	viewDef->camera.to 				= cameraTo;
	viewDef->camera.up 				= cameraUp;
	viewDef->camera.hither 			= 10;
	viewDef->camera.yon 			= 3000;
	viewDef->camera.fov 			= .9;

	viewDef->lights.ambientBrightness = 0.1;
	viewDef->lights.ambientColor 	= ambientColor;
	viewDef->lights.numFillLights 	= 2;
	viewDef->lights.fillDirection[0] = fillDirection1;
	viewDef->lights.fillDirection[1] = fillDirection2;
	viewDef->lights.fillColor[0] 	= ambientColor;
	viewDef->lights.fillColor[1] 	= ambientColor;
	viewDef->lights.fillBrightness[0] = .9;
	viewDef->lights.fillBrightness[1] = .2;
	
	viewDef->lights.useFog 		= true;
	viewDef->lights.fogStart 	= .8;
	viewDef->lights.fogEnd 		= 1.0;
	viewDef->lights.fogDensity	= 1.0;
	viewDef->lights.fogMode		= kQ3FogModePlaneBasedLinear;
	viewDef->lights.useCustomFogColor = false;
	viewDef->lights.fogColor	= clearColor;
}

/************** SETUP QD3D WINDOW *******************/

void QD3D_SetupWindow(QD3DSetupInputType *setupDefPtr, QD3DSetupOutputType **outputHandle)
{
TQ3Vector3D	v = {0,0,0};
QD3DSetupOutputType	*outputPtr;

			/* ALLOC MEMORY FOR OUTPUT DATA */

	*outputHandle = (QD3DSetupOutputType *)AllocPtr(sizeof(QD3DSetupOutputType));
	outputPtr = *outputHandle;
	GAME_ASSERT(outputPtr);


				/* PASS BACK INFO */

	outputPtr->aspectRatio			= 1.0f;								// aspect ratio is set at every frame depending on window size
	outputPtr->paneClip				= setupDefPtr->view.paneClip;
	outputPtr->needPaneClip 		= setupDefPtr->view.paneClip.left != 0 || setupDefPtr->view.paneClip.right != 0
									|| setupDefPtr->view.paneClip.bottom != 0 || setupDefPtr->view.paneClip.top != 0;
	outputPtr->hither 				= setupDefPtr->camera.hither;		// remember hither/yon
	outputPtr->yon 					= setupDefPtr->camera.yon;
	outputPtr->fov					= setupDefPtr->camera.fov;

	outputPtr->currentCameraUpVector	= setupDefPtr->camera.up;
	outputPtr->currentCameraLookAt		= setupDefPtr->camera.to;
	outputPtr->currentCameraCoords		= setupDefPtr->camera.from;

	outputPtr->isActive = true;								// it's now an active structure
	
	outputPtr->lightList = setupDefPtr->lights;				// copy light list
	
	QD3D_MoveCameraFromTo(outputPtr,&v,&v);					// call this to set outputPtr->currentCameraCoords & camera matrix



				/* SET UP OPENGL RENDERER PROPERTIES NOW THAT WE HAVE A CONTEXT */

	SDL_GL_SetSwapInterval(gCommandLine.vsync);

	CreateLights(&setupDefPtr->lights);

	Render_InitState(&setupDefPtr->view.clearColor);

	if (setupDefPtr->lights.useFog)
	{
		Render_EnableFog(
				setupDefPtr->camera.hither,
				setupDefPtr->camera.yon,
				setupDefPtr->lights.fogStart,
				setupDefPtr->lights.fogEnd,
				setupDefPtr->lights.useCustomFogColor ? setupDefPtr->lights.fogColor : setupDefPtr->view.clearColor);
	}

	CHECK_GL_ERROR();


				/* INIT TEXT SYSTEM SO WE CAN DRAW TEXT ANYWHERE IN THE GAME */

	TextMesh_Init();
}


/***************** QD3D_DisposeWindowSetup ***********************/
//
// Disposes of all data created by QD3D_SetupWindow
//

void QD3D_DisposeWindowSetup(QD3DSetupOutputType **dataHandle)
{
QD3DSetupOutputType	*data;

	data = *dataHandle;
	GAME_ASSERT(data);										// see if this setup exists

	QD3D_UpdateDebugTextMesh(nil);							// dispose debug text mesh
	TextMesh_Shutdown();

	if (gPillarboxMesh)
	{
		Q3TriMeshData_Dispose(gPillarboxMesh);
		gPillarboxMesh = nil;
	}

	data->isActive = false;									// now inactive

	Render_EndScene();
	
		/* FREE MEMORY & NIL POINTER */
		
	DisposePtr((Ptr)data);
	*dataHandle = nil;
}


/********************* CREATE LIGHTS ************************/

static void CreateLights(QD3DLightDefType *lightDefPtr)
{
			/************************/
			/* CREATE AMBIENT LIGHT */
			/************************/

	if (lightDefPtr->ambientBrightness != 0)						// see if ambient exists
	{
		GLfloat ambient[4] =
		{
			lightDefPtr->ambientBrightness * lightDefPtr->ambientColor.r,
			lightDefPtr->ambientBrightness * lightDefPtr->ambientColor.g,
			lightDefPtr->ambientBrightness * lightDefPtr->ambientColor.b,
			1
		};
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
	}

			/**********************/
			/* CREATE FILL LIGHTS */
			/**********************/

	for (int i = 0; i < lightDefPtr->numFillLights; i++)
	{
		static GLfloat lightamb[4] = { 0.0, 0.0, 0.0, 1.0 };
		GLfloat lightVec[4];
		GLfloat	diffuse[4];

					/* SET FILL DIRECTION */

		Q3Vector3D_Normalize(&lightDefPtr->fillDirection[i], &lightDefPtr->fillDirection[i]);
		lightVec[0] = -lightDefPtr->fillDirection[i].x;		// negate vector because OGL is stupid
		lightVec[1] = -lightDefPtr->fillDirection[i].y;
		lightVec[2] = -lightDefPtr->fillDirection[i].z;
		lightVec[3] = 0;									// when w==0, this is a directional light, if 1 then point light
		glLightfv(GL_LIGHT0+i, GL_POSITION, lightVec);

					/* SET COLOR */

		glLightfv(GL_LIGHT0+i, GL_AMBIENT, lightamb);

		diffuse[0] = lightDefPtr->fillColor[i].r * lightDefPtr->fillBrightness[i];
		diffuse[1] = lightDefPtr->fillColor[i].g * lightDefPtr->fillBrightness[i];
		diffuse[2] = lightDefPtr->fillColor[i].b * lightDefPtr->fillBrightness[i];
		diffuse[3] = 1;

		glLightfv(GL_LIGHT0+i, GL_DIFFUSE, diffuse);

		glEnable(GL_LIGHT0+i);								// enable the light
	}

	for (int i = lightDefPtr->numFillLights; i < MAX_FILL_LIGHTS; i++)
	{
		glDisable(GL_LIGHT0 + i);
	}
}


/******************* QD3D DRAW SCENE *********************/

void QD3D_DrawScene(QD3DSetupOutputType *setupInfo, void (*drawRoutine)(const QD3DSetupOutputType *))
{
	GAME_ASSERT(setupInfo);
	GAME_ASSERT(setupInfo->isActive);									// make sure it's legit

			/* START RENDERING */

	Render_StartFrame();

			/* SET UP VIEWPORT */

	if (setupInfo->needPaneClip || gGamePrefs.force4x3AspectRatio)
	{
		// Set scissor
		TQ3Area pane	= Render_GetAdjustedViewportRect(setupInfo->paneClip, GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT);
		int paneWidth	= pane.max.x-pane.min.x;
		int paneHeight	= pane.max.y-pane.min.y;
		setupInfo->aspectRatio = paneWidth / (paneHeight + .001f);
		Render_SetViewport(pane.min.x, gWindowHeight-pane.max.y, paneWidth, paneHeight);
	}
	else
	{
		setupInfo->aspectRatio = gWindowWidth / (gWindowHeight + .001f);
		Render_SetViewport(0, 0, gWindowWidth, gWindowHeight);
	}

			/* SET UP CAMERA */

	CalcCameraMatrixInfo(setupInfo);						// update camera matrix


			/***************/
			/* RENDER LOOP */
			/***************/

	if (drawRoutine)
		drawRoutine(setupInfo);


			/******************/
			/* DONE RENDERING */
			/*****************/

	Render_FlushQueue();

	Render_Enter2D_Full640x480();
	SubmitInfobarOverlay();			// draw 2D elements on top
	if (gGammaFadeFactor < 1.0f)
		Render_DrawFadeOverlay(gGammaFadeFactor);
	QD3D_DrawDebugTextMesh();
	Render_FlushQueue();
	Render_Exit2D();

	if (gGamePrefs.force4x3AspectRatio)
	{
		float myAR = (float)gWindowWidth / (float)gWindowHeight;
		float targetAR = (float)GAME_VIEW_WIDTH / (float)GAME_VIEW_HEIGHT;
		if (fabsf(myAR - targetAR) > (1.0f / GAME_VIEW_WIDTH))
		{
			QD3D_DrawPillarbox();
		}
	}

	Render_EndFrame();

	SDL_GL_SwapWindow(gSDLWindow);
}


//=======================================================================================================
//=============================== CAMERA STUFF ==========================================================
//=======================================================================================================

#pragma mark -

/*************** QD3D_UpdateCameraFromTo ***************/

void QD3D_UpdateCameraFromTo(QD3DSetupOutputType *setupInfo, TQ3Point3D *from, TQ3Point3D *to)
{
	setupInfo->currentCameraCoords = *from;					// set camera coords
	setupInfo->currentCameraLookAt = *to;					// set camera look at
	UpdateListenerLocation();
}


/*************** QD3D_UpdateCameraFrom ***************/

void QD3D_UpdateCameraFrom(QD3DSetupOutputType *setupInfo, TQ3Point3D *from)
{
	setupInfo->currentCameraCoords = *from;					// set camera coords
	UpdateListenerLocation();
}


/*************** QD3D_MoveCameraFromTo ***************/

void QD3D_MoveCameraFromTo(QD3DSetupOutputType *setupInfo, TQ3Vector3D *moveVector, TQ3Vector3D *lookAtVector)
{
	setupInfo->currentCameraCoords.x += moveVector->x;		// set camera coords
	setupInfo->currentCameraCoords.y += moveVector->y;
	setupInfo->currentCameraCoords.z += moveVector->z;

	setupInfo->currentCameraLookAt.x += lookAtVector->x;	// set camera look at
	setupInfo->currentCameraLookAt.y += lookAtVector->y;
	setupInfo->currentCameraLookAt.z += lookAtVector->z;

	UpdateListenerLocation();
}


//=======================================================================================================
//=============================== TEXTURE MAP STUFF =====================================================
//=======================================================================================================

#pragma mark -

/**************** QD3D GET TEXTURE MAP ***********************/
//
// Loads a numbered TGA file inside :Data:Images:Textures as an OpenGL texture.
//
// INPUT: textureRezID = TGA file number to get.
//			flags = see RendererTextureFlags
//
// OUTPUT: OpenGL texture name.
//

GLuint QD3D_LoadTextureFile(int textureRezID, int flags)
{
char					path[128];
FSSpec					spec;
uint8_t*				pixelData = nil;
TGAHeader				header;
OSErr					err;

	snprintf(path, sizeof(path), ":Images:Textures:%d.tga", textureRezID);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &spec);

			/* LOAD RAW RGBA DATA FROM TGA FILE */

	err = ReadTGA(&spec, &pixelData, &header, true);
	GAME_ASSERT(err == noErr);

	GAME_ASSERT(header.bpp == 32);
	GAME_ASSERT(header.imageType == TGA_IMAGETYPE_CONVERTED_RGBA);

			/* PRE-PROCESS IMAGE */

	int internalFormat = GL_RGB;

	if (flags & kRendererTextureFlags_ForcePOT)
	{
		int potWidth = POTCeil32(header.width);
		int potHeight = POTCeil32(header.height);
		
		if (potWidth != header.width || potHeight != header.height)
		{
			uint8_t* potPixelData = (uint8_t*) AllocPtr(potWidth * potHeight * 4);

			for (int y = 0; y < header.height; y++)
			{
				memcpy(potPixelData + y*potWidth*4, pixelData + y*header.width*4, header.width*4);
			}

			DisposePtr((Ptr) pixelData);
			pixelData = potPixelData;
			header.width = potWidth;
			header.height = potHeight;
		}
	}
#if OSXPPC
	else if (POTCeil32(header.width) != header.width || POTCeil32(header.height) != header.height)
	{
		printf("WARNING: Non-POT texture #%d\n", textureRezID);
	}
#endif

	if (flags & kRendererTextureFlags_SolidBlackIsAlpha)
	{
		for (int p = 0; p < 4 * header.width * header.height; p += 4)
		{
			bool isBlack = !pixelData[p+0] && !pixelData[p+1] && !pixelData[p+2];
			pixelData[p+3] = isBlack? 0x00: 0xFF;
		}

		// Apply edge padding to avoid seams
		TQ3Pixmap pm =
		{
			.image		= pixelData,
			.width		= header.width,
			.height		= header.height,
			.rowBytes	= header.width * (header.bpp / 8),
			.pixelSize	= 0,
			.pixelType	= kQ3PixelTypeRGBA32,
			.bitOrder	= kQ3EndianBig,
			.byteOrder	= kQ3EndianBig,
		};
		Q3Pixmap_ApplyEdgePadding(&pm);

		internalFormat = GL_RGBA;
	}
	else if (flags & kRendererTextureFlags_GrayscaleIsAlpha)
	{
		for (int p = 0; p < 4 * header.width * header.height; p += 4)
		{
			// put Blue into Alpha & leave map white
			pixelData[p+3] = pixelData[p+2];	// put blue into alpha
			pixelData[p+0] = 255;
			pixelData[p+1] = 255;
			pixelData[p+2] = 255;
		}
		internalFormat = GL_RGBA;
	}
	else if (flags & kRendererTextureFlags_KeepOriginalAlpha)
	{
		internalFormat = GL_RGBA;
	}
	else
	{
		internalFormat = GL_RGB;
	}

			/* LOAD TEXTURE */

	GLuint glTextureName = Render_LoadTexture(
			internalFormat,
			header.width,
			header.height,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			pixelData,
			flags);

			/* CLEAN UP */

	DisposePtr((Ptr) pixelData);

	return glTextureName;
}


#pragma mark -

//=======================================================================================================
//=============================== MISC ==================================================================
//=======================================================================================================

/************** QD3D CALC FRAMES PER SECOND *****************/

void QD3D_CalcFramesPerSecond(void)
{
	static uint64_t performanceFrequency = 0;
	static uint64_t prevTime = 0;
	uint64_t currTime;

	if (performanceFrequency == 0)
	{
		performanceFrequency = SDL_GetPerformanceFrequency();
	}

	slow_down:
	currTime = SDL_GetPerformanceCounter();
	uint64_t deltaTime = currTime - prevTime;

	if (deltaTime <= 0)
	{
		gFramesPerSecond = MIN_FPS;						// avoid divide by 0
	}
	else
	{
		gFramesPerSecond = performanceFrequency / (float)(deltaTime);

		if (gFramesPerSecond > MAX_FPS)					// keep from cooking the GPU
		{
			if (gFramesPerSecond - MAX_FPS > 1000)		// try to sneak in some sleep if we have 1 ms to spare
			{
				SDL_Delay(1);
			}
			goto slow_down;
		}

		if (gFramesPerSecond < MIN_FPS)					// (avoid divide by 0's later)
		{
			gFramesPerSecond = MIN_FPS;
		}
	}

	// In debug builds, speed up with KP_PLUS or LT on gamepad
#if _DEBUG
	if (GetKeyState_SDL(SDL_SCANCODE_KP_PLUS))
#else
	if (GetKeyState_SDL(SDL_SCANCODE_GRAVE) && GetKeyState_SDL(SDL_SCANCODE_KP_PLUS))
#endif
	{
		gFramesPerSecond = MIN_FPS;
	}
#if _DEBUG
	else if (gSDLController)
	{
		float analogSpeedUp = SDL_GameControllerGetAxis(gSDLController, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 32767.0f;
		if (analogSpeedUp > .25f)
		{
			gFramesPerSecond = MIN_FPS;
		}
	}
#endif

	gFramesPerSecondFrac = 1.0f / gFramesPerSecond;		// calc fractional for multiplication

	prevTime = currTime;								// reset for next time interval
}

#pragma mark -

/********************* SHOW NORMAL **************************/

void ShowNormal(TQ3Point3D *where, TQ3Vector3D *normal)
{
	gNormalWhere = *where;
	gNormal = *normal;

}

/********************* DRAW NORMAL **************************/

void DrawNormal(void)
{
	glBegin(GL_LINES);
	glVertex3f(gNormalWhere.x, gNormalWhere.y, gNormalWhere.z);
	glVertex3f(gNormalWhere.x + gNormal.x * 400.0f, gNormalWhere.y + gNormal.y * 400.0f, gNormalWhere.z + gNormal.z * 400.0f);
	glEnd();
}

/************ LAY OUT TEXT IN DEBUG TEXT MESH *****************/
//
// Pass in NULL or empty string to destroy the mesh.
//

void QD3D_UpdateDebugTextMesh(const char* text)
{
	// If passing in NULL or an empty string, clear the mesh
	if (!text || !text[0])
	{
		if (gDebugTextMesh)
		{
			Q3TriMeshData_Dispose(gDebugTextMesh);
			gDebugTextMesh = nil;
		}
		return;
	}

	int numTriangles	= kDebugTextMeshQuadCapacity * 2;
	int numPoints		= kDebugTextMeshQuadCapacity * 4;

	// Create the mesh if needed
	if (!gDebugTextMesh)
	{
		gDebugTextMesh = Q3TriMeshData_New(numTriangles, numPoints, kQ3TriMeshDataFeatureVertexUVs);
	}

	// Reset triangle & point count in mesh so TextMesh_SetMesh knows the mesh's capacity
	gDebugTextMesh->numTriangles	= numTriangles;
	gDebugTextMesh->numPoints		= numPoints;

	// Lay out the text
	TextMesh_SetMesh(nil, text, gDebugTextMesh);
}

/************ SUBMIT DEBUG TEXT MESH FOR DRAWING *****************/
//
// Must be in 640x480 2D mode.
// Does nothing if there's no text to draw.
//

void QD3D_DrawDebugTextMesh(void)
{
	// Static because matrix must survive beyond this call
	static TQ3Matrix4x4 m;

	// No text to draw
	if (!gDebugTextMesh)
		return;

	float s = .33f;
	Q3Matrix4x4_SetScale(&m, s * 1.333f * gWindowHeight / gWindowWidth, -s, 1.0f);
	m.value[3][0] = 2;
	m.value[3][1] = 72;
	m.value[3][2] = 0;
	Render_SubmitMesh(gDebugTextMesh, &m, &kDefaultRenderMods_DebugUI, &kQ3Point3D_Zero);
}

/************ DRAW PILLARBOX COVER QUADS *****************/

void QD3D_DrawPillarbox(void)
{
	if (!gPillarboxMesh)
	{
		gPillarboxMesh = Q3TriMeshData_New(2*2, 2*4, kQ3TriMeshDataFeatureNone);

		int tri = 0;
		for (int quad = 0; quad < 2; quad++)
		{
			gPillarboxMesh->triangles[tri].pointIndices[0] = quad*4 + 0;
			gPillarboxMesh->triangles[tri].pointIndices[1] = quad*4 + 1;
			gPillarboxMesh->triangles[tri].pointIndices[2] = quad*4 + 2;
			tri++;

			gPillarboxMesh->triangles[tri].pointIndices[0] = quad*4 + 0;
			gPillarboxMesh->triangles[tri].pointIndices[1] = quad*4 + 2;
			gPillarboxMesh->triangles[tri].pointIndices[2] = quad*4 + 3;
			tri++;
		}
	}

	TQ3Vector2D fitted = FitRectKeepAR(GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT, gWindowWidth, gWindowHeight);

	const float ww = gWindowWidth;
	const float wh = gWindowHeight;

	if (gWindowWidth > fitted.x)
	{
		// tall pillars at left and right edges
		float pillar = ceilf((ww - fitted.x) * 0.5f);

		gPillarboxMesh->points[0] = (TQ3Point3D){ 0, 0, 0 };
		gPillarboxMesh->points[1] = (TQ3Point3D){ pillar, 0, 0 };
		gPillarboxMesh->points[2] = (TQ3Point3D){ pillar, wh, 0 };
		gPillarboxMesh->points[3] = (TQ3Point3D){ 0, wh, 0 };

		gPillarboxMesh->points[4] = (TQ3Point3D){ ww - pillar, 0, 0 };
		gPillarboxMesh->points[5] = (TQ3Point3D){ ww, 0, 0 };
		gPillarboxMesh->points[6] = (TQ3Point3D){ ww, wh, 0 };
		gPillarboxMesh->points[7] = (TQ3Point3D){ ww - pillar, wh, 0 };
	}
	else
	{
		// wide pillars at top and bottom edges
		float pillar = ceilf((wh - fitted.y) * 0.5f);

		gPillarboxMesh->points[0] = (TQ3Point3D){ 0, 0, 0 };
		gPillarboxMesh->points[1] = (TQ3Point3D){ ww, 0, 0 };
		gPillarboxMesh->points[2] = (TQ3Point3D){ ww, pillar, 0 };
		gPillarboxMesh->points[3] = (TQ3Point3D){ 0, pillar, 0 };

		gPillarboxMesh->points[4] = (TQ3Point3D){ 0, wh - pillar, 0 };
		gPillarboxMesh->points[5] = (TQ3Point3D){ ww, wh - pillar, 0 };
		gPillarboxMesh->points[6] = (TQ3Point3D){ ww, wh, 0 };
		gPillarboxMesh->points[7] = (TQ3Point3D){ 0, wh, 0 };
	}


	Render_SetViewport(0, 0, gWindowWidth, gWindowHeight);
	Render_Enter2D_NativeResolution();
	Render_SubmitMesh(gPillarboxMesh, NULL, &kDefaultRenderMods_Pillarbox, &kQ3Point3D_Zero);
	Render_FlushQueue();
	Render_Exit2D();
}
