/****************************/
/*   	QD3D SUPPORT.C	    */
/* (c)1997-99 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <SDL_opengl.h>

#include "3dmath.h"

extern	WindowPtr			gCoverWindow;
extern	long		gScreenXOffset,gScreenYOffset;
extern	Boolean		gShowDebug;
extern	Byte		gDemoMode;
extern	PrefsType	gGamePrefs;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void CreateDrawContext(QD3DViewDefType *viewDefPtr);
static void SetStyles(QD3DStyleDefType *styleDefPtr);
static void CreateCamera(QD3DSetupInputType *setupDefPtr);
static void CreateLights(QD3DLightDefType *lightDefPtr);
static void CreateView(QD3DSetupInputType *setupDefPtr);
static void DrawPICTIntoMipmap(PicHandle pict,long width, long height, TQ3Mipmap *mipmap, Boolean blackIsAlpha);
static void Data16ToMipmap(Ptr data, short width, short height, TQ3Mipmap *mipmap);
static void Data16ToPixmap(Ptr data, short width, short height, TQ3StoragePixmap *pixmap);
static void DrawNormal(TQ3ViewObject view);
static void QASetInt(TQADrawContext	*, int, int);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

static TQ3CameraObject			gQD3D_CameraObject;
TQ3GroupObject					gQD3D_LightGroup;
static TQ3ViewObject			gQD3D_ViewObject;
static TQ3DrawContextObject		gQD3D_DrawContext;
static TQ3RendererObject		gQD3D_RendererObject;
static TQ3ShaderObject			gQD3D_ShaderObject,gQD3D_NullShaderObject;
static	TQ3StyleObject			gQD3D_BackfacingStyle;
static	TQ3StyleObject			gQD3D_FillStyle;
static	TQ3StyleObject			gQD3D_InterpolationStyle;

float	gFramesPerSecond = DEFAULT_FPS;				// this is used to maintain a constant timing velocity as frame rates differ
float	gFramesPerSecondFrac = 1/DEFAULT_FPS;

float	gAdditionalClipping = 0;

static TQ3FogMode			gFogMode;
static float		gFogStart,gFogEnd,gFogDensity;
static TQ3ColorARGB	gFogColor;

float			gYon;

Boolean		gQD3DInitialized = false;


		/* SOURCE PORT EXTRAS */

// Source port addition: this is a Quesa feature, enabled by default,
// that renders translucent materials more accurately at an angle.
// However, it looks "off" in the game -- shadow quads, shield spheres,
// water patches all appear darker than they would on original hardware.
static TQ3Boolean gQD3D_AngleAffectsAlpha = kQ3False;

static Boolean gQD3D_FreshDrawContext = false;



		/* DEBUG STUFF */
		
static TQ3Point3D		gNormalWhere;
static TQ3Vector3D		gNormal;




/******************** QD3D: BOOT ******************************/
//
// NOTE: The QuickDraw3D libraries should be included in the project as a "WEAK LINK" so that I can
// 		get an error if the library can't load.  Otherwise, the Finder reports a useless error to the user.
//

void QD3D_Boot(void)
{
TQ3Status	status;


				/* LET 'ER RIP! */
				
	status = Q3Initialize();
	GAME_ASSERT(status);

	gQD3DInitialized = true;
}



//=======================================================================================================
//=============================== VIEW WINDOW SETUP STUFF ===============================================
//=======================================================================================================


/*********************** QD3D: NEW VIEW DEF **********************/
//
// fills a view def structure with default values.
//

void QD3D_NewViewDef(QD3DSetupInputType *viewDef, WindowPtr theWindow)
{
TQ3ColorARGB		clearColor = {0,0,0,0};
TQ3Point3D			cameraFrom = { 0, 40, 200.0 };
TQ3Point3D			cameraTo = { 0, 0, 0 };
TQ3Vector3D			cameraUp = { 0.0, 1.0, 0.0 };
TQ3ColorRGB			ambientColor = { 1.0, 1.0, .8 };
TQ3Vector3D			fillDirection1 = { 1, -1, .3 };
TQ3Vector3D			fillDirection2 = { -.8, .8, -.2 };

	Q3Vector3D_Normalize(&fillDirection1,&fillDirection1);
	Q3Vector3D_Normalize(&fillDirection2,&fillDirection2);

	if (theWindow == nil)
		viewDef->view.useWindow 	=	false;							// assume going to pixmap
	else
		viewDef->view.useWindow 	=	true;							// assume going to window
	viewDef->view.displayWindow 	= theWindow;
	viewDef->view.rendererType 		= kQ3RendererTypeOpenGL;
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
}

/************** SETUP QD3D WINDOW *******************/

void QD3D_SetupWindow(QD3DSetupInputType *setupDefPtr, QD3DSetupOutputType **outputHandle)
{
TQ3Vector3D	v = {0,0,0};
TQ3Status	status;
QD3DSetupOutputType	*outputPtr;

			/* ALLOC MEMORY FOR OUTPUT DATA */

	*outputHandle = (QD3DSetupOutputType *)AllocPtr(sizeof(QD3DSetupOutputType));
	outputPtr = *outputHandle;
	GAME_ASSERT(outputPtr);

				/* SETUP */

	CreateView(setupDefPtr);
	
	CreateCamera(setupDefPtr);										// create new CAMERA object
	CreateLights(&setupDefPtr->lights);
	SetStyles(&setupDefPtr->styles);	

				/* DISPOSE OF EXTRA REFERENCES */
				
	status = Q3Object_Dispose(gQD3D_RendererObject);				// (is contained w/in gQD3D_ViewObject)
	GAME_ASSERT(status);

				/* PASS BACK INFO */
				
	outputPtr->viewObject 			= gQD3D_ViewObject;
	outputPtr->interpolationStyle 	= gQD3D_InterpolationStyle;
	outputPtr->fillStyle 			= gQD3D_FillStyle;
	outputPtr->backfacingStyle 		= gQD3D_BackfacingStyle;
	outputPtr->shaderObject 		= gQD3D_ShaderObject;
	outputPtr->nullShaderObject 	= gQD3D_NullShaderObject;
	outputPtr->cameraObject 		= gQD3D_CameraObject;
	outputPtr->lightGroup 			= gQD3D_LightGroup;
	outputPtr->drawContext 			= gQD3D_DrawContext;
	outputPtr->window 				= setupDefPtr->view.displayWindow;	// remember which window
	outputPtr->paneClip 			= setupDefPtr->view.paneClip;
	outputPtr->hither 				= setupDefPtr->camera.hither;		// remember hither/yon
	outputPtr->yon 					= setupDefPtr->camera.yon;
	gYon 							= setupDefPtr->camera.yon;			// keep a quick global around for this
	
	outputPtr->isActive = true;								// it's now an active structure
	
	outputPtr->lightList = setupDefPtr->lights;				// copy light list
	
	QD3D_MoveCameraFromTo(outputPtr,&v,&v);					// call this to set outputPtr->currentCameraCoords & camera matrix
	
	
			/* FOG */
			
	if (setupDefPtr->lights.useFog)
	{
		QD3D_SetRaveFog(setupDefPtr->lights.fogStart,setupDefPtr->lights.fogEnd,setupDefPtr->lights.fogDensity,
						&setupDefPtr->view.clearColor,setupDefPtr->lights.fogMode);
	}
}


/***************** QD3D_DisposeWindowSetup ***********************/
//
// Disposes of all data created by QD3D_SetupWindow
//

void QD3D_DisposeWindowSetup(QD3DSetupOutputType **dataHandle)
{
QD3DSetupOutputType	*data;

	gQD3D_DrawContext = nil;										// this is no longer valid

	data = *dataHandle;
	GAME_ASSERT(data);												// see if this setup exists

	DisposeBackdropTexture(); // Source port addition - release backdrop GL texture

	Q3Object_Dispose(data->viewObject);
	Q3Object_Dispose(data->interpolationStyle);
	Q3Object_Dispose(data->backfacingStyle);
	Q3Object_Dispose(data->fillStyle);
	Q3Object_Dispose(data->cameraObject);
	Q3Object_Dispose(data->lightGroup);
	Q3Object_Dispose(data->drawContext);
	Q3Object_Dispose(data->shaderObject);
	Q3Object_Dispose(data->nullShaderObject);
		
	data->isActive = false;									// now inactive
	
		/* FREE MEMORY & NIL POINTER */
		
	DisposePtr((Ptr)data);
	*dataHandle = nil;
}


/******************* CREATE GAME VIEW *************************/

static void CreateView(QD3DSetupInputType *setupDefPtr)
{
TQ3Status	status;

				/* CREATE NEW VIEW OBJECT */
				
	gQD3D_ViewObject = Q3View_New();
	GAME_ASSERT(gQD3D_ViewObject);

			/* CREATE & SET DRAW CONTEXT */
	
	CreateDrawContext(&setupDefPtr->view); 											// init draw context
	
	status = Q3View_SetDrawContext(gQD3D_ViewObject, gQD3D_DrawContext);			// assign context to view
	GAME_ASSERT(status);

			/* CREATE & SET RENDERER */

	gQD3D_RendererObject = Q3Renderer_NewFromType(setupDefPtr->view.rendererType);	// create new RENDERER object
	GAME_ASSERT(gQD3D_RendererObject);

	status = Q3View_SetRenderer(gQD3D_ViewObject, gQD3D_RendererObject);				// assign renderer to view
	GAME_ASSERT(status);
				
		
		/* SET RENDERER FEATURES */
		
#if 0
	TQ3Uns32	hints;
	Q3InteractiveRenderer_GetRAVEContextHints(gQD3D_RendererObject, &hints);
	hints &= ~kQAContext_NoZBuffer; 				// Z buffer is on 
	hints &= ~kQAContext_DeepZ; 					// shallow z
	hints &= ~kQAContext_NoDither; 					// yes-dither
	Q3InteractiveRenderer_SetRAVEContextHints(gQD3D_RendererObject, hints);	
#endif

	// Source port addition: set bilinear texturing according to user preference
	Q3InteractiveRenderer_SetRAVETextureFilter(
		gQD3D_RendererObject,
		gGamePrefs.textureFiltering ? kQATextureFilter_Best : kQATextureFilter_Fast);

#if 0
	Q3InteractiveRenderer_SetDoubleBufferBypass(gQD3D_RendererObject,kQ3True);
#endif

	// Source port addition: turn off Quesa's angle affect on alpha to preserve the original look of shadows, water, shields etc.
	Q3Object_SetProperty(gQD3D_RendererObject, kQ3RendererPropertyAngleAffectsAlpha,
						 sizeof(gQD3D_AngleAffectsAlpha), &gQD3D_AngleAffectsAlpha);
}


/**************** CREATE DRAW CONTEXT *********************/

static void CreateDrawContext(QD3DViewDefType *viewDefPtr)
{
TQ3DrawContextData		drawContexData;
TQ3SDLDrawContextData	myMacDrawContextData;
extern SDL_Window*		gSDLWindow;

	int ww, wh;
	SDL_GL_GetDrawableSize(gSDLWindow, &ww, &wh);

			/* SEE IF DOING PIXMAP CONTEXT */
			
	GAME_ASSERT_MESSAGE(viewDefPtr->useWindow, "Pixmap context not supported!");

			/* FILL IN DRAW CONTEXT DATA */

	if (viewDefPtr->dontClear)
		drawContexData.clearImageMethod = kQ3ClearMethodNone;
	else
		drawContexData.clearImageMethod = kQ3ClearMethodWithColor;		
	
	drawContexData.clearImageColor = viewDefPtr->clearColor;				// color to clear to
	drawContexData.pane = GetAdjustedPane(ww, wh, viewDefPtr->paneClip);

#if DEBUG_WIREFRAME
	drawContexData.clearImageColor.a = 1.0f;
	drawContexData.clearImageColor.r = 0.0f;
	drawContexData.clearImageColor.g = 0.5f;
	drawContexData.clearImageColor.b = 1.0f;
#endif

	drawContexData.paneState = kQ3True;										// use bounds?
	drawContexData.maskState = kQ3False;									// no mask
	drawContexData.doubleBufferState = kQ3True;								// double buffering

	myMacDrawContextData.drawContextData = drawContexData;					// set MAC specifics
	myMacDrawContextData.sdlWindow = gSDLWindow;							// assign window to draw to


			/* CREATE DRAW CONTEXT */

	gQD3D_DrawContext = Q3SDLDrawContext_New(&myMacDrawContextData);
	GAME_ASSERT(gQD3D_DrawContext);


	gQD3D_FreshDrawContext = true;
}


/**************** SET STYLES ****************/
//
// Creates style objects which define how the scene is to be rendered.
// It also sets the shader object.
//

static void SetStyles(QD3DStyleDefType *styleDefPtr)
{

				/* SET INTERPOLATION (FOR SHADING) */
					
	gQD3D_InterpolationStyle = Q3InterpolationStyle_New(styleDefPtr->interpolation);
	GAME_ASSERT(gQD3D_InterpolationStyle);

					/* SET BACKFACING */

	gQD3D_BackfacingStyle = Q3BackfacingStyle_New(styleDefPtr->backfacing);
	GAME_ASSERT(gQD3D_BackfacingStyle);

				/* SET POLYGON FILL STYLE */
						
#if DEBUG_WIREFRAME
	gQD3D_FillStyle = Q3FillStyle_New(kQ3FillStyleEdges);
#else
	gQD3D_FillStyle = Q3FillStyle_New(styleDefPtr->fill);
#endif
	GAME_ASSERT(gQD3D_FillStyle);


					/* SET THE SHADER TO USE */

	if (styleDefPtr->usePhong)
	{
		gQD3D_ShaderObject = Q3PhongIllumination_New();
		GAME_ASSERT(gQD3D_ShaderObject);
	}
	else
	{
		gQD3D_ShaderObject = Q3LambertIllumination_New();
		GAME_ASSERT(gQD3D_ShaderObject);
	}


			/* ALSO MAKE NULL SHADER FOR SPECIAL PURPOSES */
			
	gQD3D_NullShaderObject = Q3NULLIllumination_New();

}



/****************** CREATE CAMERA *********************/

static void CreateCamera(QD3DSetupInputType *setupDefPtr)
{
TQ3CameraData					myCameraData;
TQ3ViewAngleAspectCameraData	myViewAngleCameraData;
TQ3Area							pane;
TQ3Status						status;
QD3DCameraDefType 				*cameraDefPtr;

	cameraDefPtr = &setupDefPtr->camera;

		/* GET PANE */
		//
		// Note: Q3DrawContext_GetPane seems to return garbage on pixmaps so, rig it.
		//
		
	if (setupDefPtr->view.useWindow)
	{
		status = Q3DrawContext_GetPane(gQD3D_DrawContext,&pane);				// get window pane info
		GAME_ASSERT(status);
	}
	else
	{
		Rect	r;
		
		GetPortBounds(setupDefPtr->view.gworld, &r);
		
		pane.min.x = pane.min.y = 0;
		pane.max.x = r.right;
		pane.max.y = r.bottom;
	}


				/* FILL IN CAMERA DATA */
				
	myCameraData.placement.cameraLocation = cameraDefPtr->from;			// set camera coords
	myCameraData.placement.pointOfInterest = cameraDefPtr->to;			// set target coords
	myCameraData.placement.upVector = cameraDefPtr->up;					// set a vector that's "up"
	myCameraData.range.hither = cameraDefPtr->hither;					// set frontmost Z dist
	myCameraData.range.yon = cameraDefPtr->yon;							// set farthest Z dist
	myCameraData.viewPort.origin.x = -1.0;								// set view origins?
	myCameraData.viewPort.origin.y = 1.0;
	myCameraData.viewPort.width = 2.0;
	myCameraData.viewPort.height = 2.0;

	myViewAngleCameraData.cameraData = myCameraData;
	myViewAngleCameraData.fov = cameraDefPtr->fov;						// larger = more fisheyed
	myViewAngleCameraData.aspectRatioXToY =
				(pane.max.x-pane.min.x)/(pane.max.y-pane.min.y);

	gQD3D_CameraObject = Q3ViewAngleAspectCamera_New(&myViewAngleCameraData);	 // create new camera
	GAME_ASSERT(gQD3D_CameraObject);

	status = Q3View_SetCamera(gQD3D_ViewObject, gQD3D_CameraObject);		// assign camera to view
	GAME_ASSERT(status);
}


/********************* CREATE LIGHTS ************************/

static void CreateLights(QD3DLightDefType *lightDefPtr)
{
TQ3GroupPosition		myGroupPosition;
TQ3LightData			myLightData;
TQ3DirectionalLightData	myDirectionalLightData;
TQ3LightObject			myLight;
short					i;
TQ3Status	myErr;


			/* CREATE NEW LIGHT GROUP */
			
	gQD3D_LightGroup = (TQ3GroupObject) Q3LightGroup_New();						// make new light group
	GAME_ASSERT(gQD3D_LightGroup);


	myLightData.isOn = kQ3True;									// light is ON
	
			/************************/
			/* CREATE AMBIENT LIGHT */
			/************************/

	if (lightDefPtr->ambientBrightness != 0)						// see if ambient exists
	{
		myLightData.color = lightDefPtr->ambientColor;				// set color of light
		myLightData.brightness = lightDefPtr->ambientBrightness;	// set brightness value
		myLight = Q3AmbientLight_New(&myLightData);					// make it
		GAME_ASSERT(myLight);

		myGroupPosition = (TQ3GroupPosition)Q3Group_AddObject(gQD3D_LightGroup, myLight);	// add to group
		GAME_ASSERT(myGroupPosition);

		Q3Object_Dispose(myLight);									// dispose of light

	}

			/**********************/
			/* CREATE FILL LIGHTS */
			/**********************/
			
	for (i=0; i < lightDefPtr->numFillLights; i++)
	{		
		Q3Vector3D_Normalize(&lightDefPtr->fillDirection[i], &lightDefPtr->fillDirection[i]);
		myLightData.color = lightDefPtr->fillColor[i];						// set color of light
		myLightData.brightness = lightDefPtr->fillBrightness[i];			// set brightness
		myDirectionalLightData.lightData = myLightData;						// refer to general light info
		myDirectionalLightData.castsShadows = kQ3True;						// shadows
		myDirectionalLightData.direction =  lightDefPtr->fillDirection[i];	// set fill vector
		myLight = Q3DirectionalLight_New(&myDirectionalLightData);			// make it
		GAME_ASSERT(myLight);

		myGroupPosition = (TQ3GroupPosition)Q3Group_AddObject(gQD3D_LightGroup, myLight);		// add to group
		GAME_ASSERT(myGroupPosition);

		Q3Object_Dispose(myLight);											// dispose of light
	}
	
			/* ASSIGN LIGHT GROUP TO VIEW */
			
	myErr = Q3View_SetLightGroup(gQD3D_ViewObject, gQD3D_LightGroup);		// assign light group to view
	GAME_ASSERT(myErr);
}



/******************** QD3D CHANGE DRAW SIZE *********************/
//
// Changes size of stuff to fit new window size and/or shink factor
//

void QD3D_ChangeDrawSize(QD3DSetupOutputType *setupInfo)
{
Rect			r;
TQ3Area			pane;
TQ3ViewAngleAspectCameraData	cameraData;
TQ3Status		status;

			/* CHANGE DRAW CONTEXT PANE SIZE */
			
	if (setupInfo->window == nil)
		return;
		
	GetPortBounds(GetWindowPort(setupInfo->window), &r);										// get size of window
	pane.min.x = setupInfo->paneClip.left;													// set pane size
	pane.max.x = r.right-setupInfo->paneClip.right;
	pane.min.y = setupInfo->paneClip.top;
	pane.max.y = r.bottom-setupInfo->paneClip.bottom;

	status = Q3DrawContext_SetPane(setupInfo->drawContext,&pane);							// update pane in draw context
	GAME_ASSERT(status);

				/* CHANGE CAMERA ASPECT RATIO */
				
	status = Q3ViewAngleAspectCamera_GetData(setupInfo->cameraObject,&cameraData);			// get camera data
	GAME_ASSERT(status);

	cameraData.aspectRatioXToY = (pane.max.x-pane.min.x)/(pane.max.y-pane.min.y);			// set new aspect ratio
	status = Q3ViewAngleAspectCamera_SetData(setupInfo->cameraObject,&cameraData);			// set new camera data
	GAME_ASSERT(status);
}


/******************* QD3D DRAW SCENE *********************/

void QD3D_DrawScene(QD3DSetupOutputType *setupInfo, void (*drawRoutine)(const QD3DSetupOutputType *))
{
TQ3Status				myStatus;
TQ3ViewStatus			myViewStatus;

	GAME_ASSERT(setupInfo);

	GAME_ASSERT(setupInfo->isActive);									// make sure it's legit


			/* START RENDERING */

			
	myStatus = Q3View_StartRendering(setupInfo->viewObject);			
	if ( myStatus == kQ3Failure )
	{
		DoAlert("ERROR: Q3View_StartRendering Failed!");
		QD3D_ShowRecentError();
	}

	CalcCameraMatrixInfo(setupInfo);						// update camera matrix


			/* SOURCE PORT STUFF */

	if (gQD3D_FreshDrawContext)
	{
		SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0);

		AllocBackdropTexture(); // Source port addition - alloc GL backdrop texture
		// (must be after StartRendering so we have a valid GL context)

		gQD3D_FreshDrawContext = false;
	}



			/***************/
			/* RENDER LOOP */
			/***************/
	do
	{
				/* DRAW STYLES */

		QD3D_ReEnableFog(setupInfo);
				
		myStatus = Q3Style_Submit(setupInfo->interpolationStyle,setupInfo->viewObject);
		GAME_ASSERT(myStatus);

		myStatus = Q3Style_Submit(setupInfo->backfacingStyle,setupInfo->viewObject);
		GAME_ASSERT(myStatus);
			
		myStatus = Q3Style_Submit(setupInfo->fillStyle, setupInfo->viewObject);
		GAME_ASSERT(myStatus);

		myStatus = Q3Shader_Submit(setupInfo->shaderObject, setupInfo->viewObject);
		GAME_ASSERT(myStatus);

			/* DRAW NORMAL */
			
		if (gShowDebug)
			DrawNormal(setupInfo->viewObject);

			/* CALL INPUT DRAW FUNCTION */

		if (drawRoutine != nil)
			drawRoutine(setupInfo);

		myViewStatus = Q3View_EndRendering(setupInfo->viewObject);
		GAME_ASSERT(myViewStatus != kQ3ViewStatusError);

	} while ( myViewStatus == kQ3ViewStatusRetraverse );	
}


//=======================================================================================================
//=============================== CAMERA STUFF ==========================================================
//=======================================================================================================

#pragma mark -

/*************** QD3D_UpdateCameraFromTo ***************/

void QD3D_UpdateCameraFromTo(QD3DSetupOutputType *setupInfo, TQ3Point3D *from, TQ3Point3D *to)
{
TQ3Status	status;
TQ3CameraPlacement	placement;
TQ3CameraObject		camera;

			/* GET CURRENT CAMERA INFO */

	camera = setupInfo->cameraObject;
			
	status = Q3Camera_GetPlacement(camera, &placement);
	GAME_ASSERT(status);


			/* SET CAMERA LOOK AT */
			
	placement.pointOfInterest = *to;
	setupInfo->currentCameraLookAt = *to;


			/* SET CAMERA COORDS */
			
	placement.cameraLocation = *from;
	setupInfo->currentCameraCoords = *from;				// keep global copy for quick use


			/* UPDATE CAMERA INFO */
			
	status = Q3Camera_SetPlacement(camera, &placement);
	GAME_ASSERT(status);
		
	UpdateListenerLocation();
}


/*************** QD3D_UpdateCameraFrom ***************/

void QD3D_UpdateCameraFrom(QD3DSetupOutputType *setupInfo, TQ3Point3D *from)
{
TQ3Status	status;
TQ3CameraPlacement	placement;

			/* GET CURRENT CAMERA INFO */
			
	status = Q3Camera_GetPlacement(setupInfo->cameraObject, &placement);
	GAME_ASSERT(status);


			/* SET CAMERA COORDS */
			
	placement.cameraLocation = *from;
	setupInfo->currentCameraCoords = *from;				// keep global copy for quick use
	

			/* UPDATE CAMERA INFO */
			
	status = Q3Camera_SetPlacement(setupInfo->cameraObject, &placement);
	GAME_ASSERT(status);

	UpdateListenerLocation();
}


/*************** QD3D_MoveCameraFromTo ***************/

void QD3D_MoveCameraFromTo(QD3DSetupOutputType *setupInfo, TQ3Vector3D *moveVector, TQ3Vector3D *lookAtVector)
{
TQ3Status	status;
TQ3CameraPlacement	placement;

			/* GET CURRENT CAMERA INFO */
			
	status = Q3Camera_GetPlacement(setupInfo->cameraObject, &placement);
	GAME_ASSERT(status);


			/* SET CAMERA COORDS */
			

	placement.cameraLocation.x += moveVector->x;
	placement.cameraLocation.y += moveVector->y;
	placement.cameraLocation.z += moveVector->z;

	placement.pointOfInterest.x += lookAtVector->x;
	placement.pointOfInterest.y += lookAtVector->y;
	placement.pointOfInterest.z += lookAtVector->z;
	
	setupInfo->currentCameraCoords = placement.cameraLocation;	// keep global copy for quick use
	setupInfo->currentCameraLookAt = placement.pointOfInterest;

			/* UPDATE CAMERA INFO */
			
	status = Q3Camera_SetPlacement(setupInfo->cameraObject, &placement);
	GAME_ASSERT(status);

	UpdateListenerLocation();
}


//=======================================================================================================
//=============================== LIGHTS STUFF ==========================================================
//=======================================================================================================

#pragma mark -


/********************* QD3D ADD POINT LIGHT ************************/

TQ3GroupPosition QD3D_AddPointLight(QD3DSetupOutputType *setupInfo,TQ3Point3D *point, TQ3ColorRGB *color, float brightness)
{
TQ3GroupPosition		myGroupPosition;
TQ3LightData			myLightData;
TQ3PointLightData		myPointLightData;
TQ3LightObject			myLight;


	myLightData.isOn = kQ3True;											// light is ON
	
	myLightData.color = *color;											// set color of light
	myLightData.brightness = brightness;								// set brightness
	myPointLightData.lightData = myLightData;							// refer to general light info
	myPointLightData.castsShadows = kQ3False;							// no shadows
	myPointLightData.location = *point;									// set coords
	
	myPointLightData.attenuation = kQ3AttenuationTypeNone;				// set attenuation
	myLight = Q3PointLight_New(&myPointLightData);						// make it
	GAME_ASSERT(myLight);

	myGroupPosition = (TQ3GroupPosition)Q3Group_AddObject(setupInfo->lightGroup, myLight);// add to light group
	GAME_ASSERT(myGroupPosition);

	Q3Object_Dispose(myLight);											// dispose of light
	return(myGroupPosition);
}


/****************** QD3D SET POINT LIGHT COORDS ********************/

void QD3D_SetPointLightCoords(QD3DSetupOutputType *setupInfo, TQ3GroupPosition lightPosition, TQ3Point3D *point)
{
TQ3PointLightData	pointLightData;
TQ3LightObject		light;
TQ3Status			status;

	status = Q3Group_GetPositionObject(setupInfo->lightGroup, lightPosition, &light);	// get point light object from light group
	GAME_ASSERT(status);

	status =  Q3PointLight_GetData(light, &pointLightData);				// get light data
	GAME_ASSERT(status);

	pointLightData.location = *point;									// set coords

	status = Q3PointLight_SetData(light, &pointLightData);				// update light data
	GAME_ASSERT(status);
		
	Q3Object_Dispose(light);
}


/****************** QD3D SET POINT LIGHT BRIGHTNESS ********************/

void QD3D_SetPointLightBrightness(QD3DSetupOutputType *setupInfo, TQ3GroupPosition lightPosition, float bright)
{
TQ3LightObject		light;
TQ3Status			status;

	status = Q3Group_GetPositionObject(setupInfo->lightGroup, lightPosition, &light);	// get point light object from light group
	GAME_ASSERT(status);

	status = Q3Light_SetBrightness(light, bright);
	GAME_ASSERT(status);

	Q3Object_Dispose(light);
}



/********************* QD3D ADD FILL LIGHT ************************/

TQ3GroupPosition QD3D_AddFillLight(QD3DSetupOutputType *setupInfo,TQ3Vector3D *fillVector, TQ3ColorRGB *color, float brightness)
{
TQ3GroupPosition		myGroupPosition;
TQ3LightData			myLightData;
TQ3LightObject			myLight;
TQ3DirectionalLightData	myDirectionalLightData;


	myLightData.isOn = kQ3True;									// light is ON
	
	myLightData.color = *color;									// set color of light
	myLightData.brightness = brightness;						// set brightness
	myDirectionalLightData.lightData = myLightData;				// refer to general light info
	myDirectionalLightData.castsShadows = kQ3False;				// no shadows
	myDirectionalLightData.direction = *fillVector;				// set vector
	
	myLight = Q3DirectionalLight_New(&myDirectionalLightData);	// make it
	GAME_ASSERT(myLight);

	myGroupPosition = (TQ3GroupPosition)Q3Group_AddObject(setupInfo->lightGroup, myLight);	// add to light group
	GAME_ASSERT(myGroupPosition != 0);

	Q3Object_Dispose(myLight);												// dispose of light
	return(myGroupPosition);
}

/********************* QD3D ADD AMBIENT LIGHT ************************/

TQ3GroupPosition QD3D_AddAmbientLight(QD3DSetupOutputType *setupInfo, TQ3ColorRGB *color, float brightness)
{
TQ3GroupPosition		myGroupPosition;
TQ3LightData			myLightData;
TQ3LightObject			myLight;



	myLightData.isOn = kQ3True;									// light is ON
	myLightData.color = *color;									// set color of light
	myLightData.brightness = brightness;						// set brightness
	
	myLight = Q3AmbientLight_New(&myLightData);					// make it
	GAME_ASSERT(myLight);

	myGroupPosition = (TQ3GroupPosition)Q3Group_AddObject(setupInfo->lightGroup, myLight);		// add to light group
	GAME_ASSERT(myGroupPosition != 0);

	Q3Object_Dispose(myLight);									// dispose of light
	
	return(myGroupPosition);
}




/****************** QD3D DELETE LIGHT ********************/

void QD3D_DeleteLight(QD3DSetupOutputType *setupInfo, TQ3GroupPosition lightPosition)
{
TQ3LightObject		light;

	light = (TQ3LightObject)Q3Group_RemovePosition(setupInfo->lightGroup, lightPosition);
	GAME_ASSERT(light);

	Q3Object_Dispose(light);
}


/****************** QD3D DELETE ALL LIGHTS ********************/
//
// Deletes ALL lights from the light group, including the ambient light.
//

void QD3D_DeleteAllLights(QD3DSetupOutputType *setupInfo)
{
TQ3Status				status;

	status = Q3Group_EmptyObjects(setupInfo->lightGroup);
	GAME_ASSERT(status);
}




//=======================================================================================================
//=============================== TEXTURE MAP STUFF =====================================================
//=======================================================================================================

#pragma mark -

/**************** QD3D GET TEXTURE MAP ***********************/
//
// Loads a PICT resource and returns a shader object which is
// based on the PICT converted to a texture map.
//
// INPUT: textureRezID = resource ID of texture PICT to get.
//			myFSSpec != nil if want to load PICT from file instead
//			blackIsAlpha = true if want to turn alpha on and to scan image for alpha pixels
//
// OUTPUT: TQ3ShaderObject = shader object for texture map.  nil == error.
//

TQ3SurfaceShaderObject	QD3D_GetTextureMap(long	textureRezID, FSSpec *myFSSpec, Boolean blackIsAlpha)
{
PicHandle			picture;
TQ3SurfaceShaderObject		shader;
long				pictSize,headerSize;
OSErr				iErr;
short				fRefNum;
char				pictHeader[PICT_HEADER_SIZE];

	if (myFSSpec == nil)
	{
					/* LOAD PICT REZ */
		
		picture = GetPicture (textureRezID);
		if (picture == nil)
		{
		    iErr = ResError();
			DoAlert(" load texture PICT resource");
			ShowSystemErr(iErr);
		}
	}
	else
	{
				/* LOAD PICT FROM FILE */
	
		iErr = FSpOpenDF(myFSSpec,fsRdPerm,&fRefNum);
		if (iErr)
		{
			DoAlert("Sorry, can open that PICT file!");
			return(nil);
		}

		if	(GetEOF(fRefNum,&pictSize) != noErr)		// get size of file		
			goto err;
				
		headerSize = PICT_HEADER_SIZE;					// check the header					
		if (FSRead(fRefNum,&headerSize,pictHeader) != noErr)
			goto err;

		if ((pictSize -= PICT_HEADER_SIZE) <= 0)
			goto err;
			
		if ((picture = (PicHandle)AllocHandle(pictSize)) == nil)
		{
			DoAlert("Sorry, not enough memory to read PICT file!");
			return(nil);
		}
		HLock((Handle)picture);
			
		if (FSRead(fRefNum, &pictSize, (Ptr)*picture) != noErr)
		{
			DisposeHandle((Handle)picture);
			goto err;
		}
			
		FSClose(fRefNum);		
	}
	
	
	shader = QD3D_PICTToTexture(picture,blackIsAlpha);
		
	if (myFSSpec == nil)
		ReleaseResource ((Handle) picture);
	else
		DisposeHandle((Handle)picture);

	return(shader);	
	
err:
	DoAlert("Sorry, error reading PICT file!");
	return(nil);
}


/**************** QD3D PICT TO TEXTURE ***********************/
//
//
// INPUT: picture = handle to PICT.
//
// OUTPUT: TQ3ShaderObject = shader object for texture map.
//

TQ3SurfaceShaderObject	QD3D_PICTToTexture(PicHandle picture, Boolean blackIsAlpha)
{
TQ3Mipmap 				mipmap;
TQ3TextureObject		texture;
TQ3SurfaceShaderObject	shader;
long					width,height;


			/* MAKE INTO STORAGE MIPMAP */

	width = (**picture).picFrame.right  - (**picture).picFrame.left;		// calc dimensions of mipmap
	height = (**picture).picFrame.bottom - (**picture).picFrame.top;
		
	DrawPICTIntoMipmap (picture, width, height, &mipmap, blackIsAlpha);
		

			/* MAKE NEW PIXMAP TEXTURE */
			
	texture = Q3MipmapTexture_New(&mipmap);							// make new mipmap	
	GAME_ASSERT(texture);

	shader = Q3TextureShader_New (texture);
	GAME_ASSERT(shader);

	Q3Object_Dispose (texture);
	Q3Object_Dispose (mipmap.image);			// disposes of extra reference to storage obj

	return(shader);	
}


/**************** QD3D GWORLD TO TEXTURE ***********************/
//
// INPUT: picture = handle to PICT.
//
// OUTPUT: TQ3ShaderObject = shader object for texture map.
//

TQ3SurfaceShaderObject	QD3D_GWorldToTexture(GWorldPtr theGWorld, Boolean pointToGWorld, Boolean blackIsAlpha)
{
TQ3Mipmap 					mipmap;
TQ3TextureObject			texture;
TQ3SurfaceShaderObject		shader;

			/* CREATE MIPMAP */
			
	QD3D_GWorldToMipMap(theGWorld,&mipmap, pointToGWorld, blackIsAlpha);


			/* MAKE NEW MIPMAP TEXTURE */
			
	texture = Q3MipmapTexture_New(&mipmap);							// make new mipmap	
	GAME_ASSERT(texture);
			
	shader = Q3TextureShader_New(texture);
	GAME_ASSERT(shader);

	Q3Object_Dispose (texture);
	Q3Object_Dispose (mipmap.image);					// dispose of extra ref to storage object

	return(shader);	
}


/******************** DRAW PICT INTO MIPMAP ********************/
//
// OUTPUT: mipmap = new mipmap holding texture image
//

static void DrawPICTIntoMipmap(PicHandle pict,long width, long height, TQ3Mipmap *mipmap, Boolean blackIsAlpha)
{
#if 1
	GAME_ASSERT(width  == (**pict).picFrame.right  - (**pict).picFrame.left);
	GAME_ASSERT(height == (**pict).picFrame.bottom - (**pict).picFrame.top);
	
	Ptr pictMapAddr = (**pict).__pomme_pixelsARGB32;
	long pictRowBytes = width * (32 / 8);

	if (blackIsAlpha)
	{
		uint32_t*	rowPtr = (uint32_t *)pictMapAddr;

		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				uint32_t pixel = rowPtr[x];
				if (!(pixel & 0xFFFFFF00))
					rowPtr[x] &= 0xFFFFFF00;
			}
			rowPtr += pictRowBytes / 4;
		}
	}
	
	mipmap->image = Q3MemoryStorage_New((unsigned char*)pictMapAddr, pictRowBytes * height);
	GAME_ASSERT(mipmap->image);

	mipmap->useMipmapping = kQ3False;							// not actually using mipmaps (just 1 srcmap)
	mipmap->pixelType = kQ3PixelTypeARGB32;						// if 32bit, assume alpha

	mipmap->bitOrder = kQ3EndianBig;
	mipmap->byteOrder = kQ3EndianBig;
	mipmap->reserved = 0;
	mipmap->mipmaps[0].width = width;
	mipmap->mipmaps[0].height = height;
	mipmap->mipmaps[0].rowBytes = pictRowBytes;
	mipmap->mipmaps[0].offset = 0;
#else
Rect 					rectGW;
GWorldPtr 				pGWorld;
PixMapHandle 			hPixMap;
OSErr					myErr;
GDHandle				oldGD;
GWorldPtr				oldGW;
PictInfo				thePictInfo;
short					depth;

	GetGWorld(&oldGW, &oldGD);										// save current port


				/* GET PICT INFO TO FIND COLOR DEPTH */
				
	myErr = GetPictInfo(pict, &thePictInfo, 0, 0, systemMethod, 0);
	if (myErr)
	{
		DoAlert("DrawPICTIntoMipmap: GetPictInfo failed!");
		ShowSystemErr(myErr);
	}

	depth = thePictInfo.depth;
	
	if (thePictInfo.commentHandle)									// free pict info
		DisposeHandle((Handle)thePictInfo.commentHandle);
	if (thePictInfo.fontHandle)
		DisposeHandle((Handle)thePictInfo.fontHandle);
	if (thePictInfo.fontNamesHandle)
		DisposeHandle((Handle)thePictInfo.fontNamesHandle);
	if (thePictInfo.theColorTable)
		DisposeCTable(thePictInfo.theColorTable);



				/* CREATE A GWORLD TO DRAW INTO */

	SetRect(&rectGW, 0, 0, width, height);							// set dimensions
	myErr = NewGWorld(&pGWorld, depth, &rectGW, 0, 0, 0);			// try app mem
	if (myErr)
	{
		myErr = NewGWorld(&pGWorld, depth, &rectGW, 0, 0, useTempMem);	// try sys mem
		if (myErr)
			DoFatalAlert("DrawPICTIntoMipmap: NewGWorld failed!");
	}
	
	hPixMap = GetGWorldPixMap(pGWorld);								// get gworld's pixmap
	
	if (depth == 32)
		(**hPixMap).cmpCount = 4;	// we want full 4-component argb (defaults to only rgb)
		

			/* DRAW PICTURE INTO GWORLD */
			
	SetGWorld(pGWorld, nil);	
	LockPixels(hPixMap);
	EraseRect(&rectGW);
	DrawPicture(pict, &rectGW);


			/* MAKE A MIPMAP FROM GWORLD */
			
	QD3D_GWorldToMipMap(pGWorld,mipmap,false, blackIsAlpha);
	
	SetGWorld (oldGW, oldGD);
	DisposeGWorld (pGWorld);
#endif
}


/******************** GWORLD TO MIPMAP ********************/
//
// Creates a mipmap from an existing GWorld
//
// NOTE: Assumes that GWorld is 16bit!!!!
//
// OUTPUT: mipmap = new mipmap holding texture image
//

void QD3D_GWorldToMipMap(GWorldPtr pGWorld, TQ3Mipmap *mipmap, Boolean pointToGWorld, Boolean blackIsAlpha)
{
#if 1
	SOURCE_PORT_PLACEHOLDER();
#else
unsigned long 			pictMapAddr;
PixMapHandle 			hPixMap;
unsigned long 			pictRowBytes;
long					width, height;
short					depth;
	
				/* GET GWORLD INFO */
				
	hPixMap = GetGWorldPixMap(pGWorld);								// calc addr & rowbytes
	
	depth = (**hPixMap).pixelSize;									// get gworld depth
	
	pictMapAddr = (unsigned long )GetPixBaseAddr(hPixMap);
	pictRowBytes = (unsigned long)(**hPixMap).rowBytes & 0x3fff;
	width = ((**hPixMap).bounds.right - (**hPixMap).bounds.left);
	height = ((**hPixMap).bounds.bottom - (**hPixMap).bounds.top);


			/* CONVERT 16-BIT BLACK TO ALPHA */
			
	if (blackIsAlpha && (depth == 16))
	{
		long	x,y;
		u_short	pixel,*rowPtr;		
		
		rowPtr = (u_short *)pictMapAddr;
		
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				pixel = rowPtr[x];					// get pixel
				if (pixel != 0x0000)				// if pure black then 
					rowPtr[x] = pixel | 0x8000;		// set alpha bit on this opaque thing
			}
			rowPtr += pictRowBytes>>1;
		}
	}

#if 0
	if (blackIsAlpha && (depth == 32))
	{	
		long	x,y;
		u_long	pixel,*rowPtr;		
	
		if ((**hPixMap).cmpCount == 3)
			DebugStr("xxxxxxxxx");
		
		rowPtr = (u_long *)pictMapAddr;
		
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				pixel = rowPtr[x];					// get pixel
				if (!(pixel & 0xff000000))
					rowPtr[x] = 0xff00ff00;//((x*4)<<24) | 0xff00;		// set alpha bit on this opaque thing
			}
			rowPtr += pictRowBytes>>2;
		}
	}
#endif	

	
			/* MAKE MIPMAP */

	if (pointToGWorld)	
	{
		LockPixels(hPixMap);										// we don't want this to move on us
		mipmap->image = (void *)Q3MemoryStorage_NewBuffer ((unsigned char *) pictMapAddr, pictRowBytes * height,pictRowBytes * height);
	}
	else
		mipmap->image = (void *)Q3MemoryStorage_New ((unsigned char *) pictMapAddr, pictRowBytes * height);
		
	if (mipmap->image == nil)
		DoFatalAlert("Q3MemoryStorage_New Failed!");

	mipmap->useMipmapping = kQ3False;							// not actually using mipmaps (just 1 srcmap)
	
	if (blackIsAlpha)
	{
		if (depth == 16)
			mipmap->pixelType = kQ3PixelTypeARGB16;						
		else
			mipmap->pixelType = kQ3PixelTypeARGB32;
	}
	else
	{
		if (depth == 16)
			mipmap->pixelType = kQ3PixelTypeRGB16;						
		else
			mipmap->pixelType = kQ3PixelTypeRGB32;
	}
	
	mipmap->bitOrder = kQ3EndianBig;
	mipmap->byteOrder = kQ3EndianBig;
	mipmap->reserved = nil;
	mipmap->mipmaps[0].width = width;
	mipmap->mipmaps[0].height = height;
	mipmap->mipmaps[0].rowBytes = pictRowBytes;
	mipmap->mipmaps[0].offset = 0;
#endif
}


/**************** QD3D: DATA16 TO TEXTURE_NOMIP ***********************/
//
// Converts input data to non mipmapped texture
//
// INPUT: .
//
// OUTPUT: TQ3ShaderObject = shader object for texture map.
//

TQ3SurfaceShaderObject	QD3D_Data16ToTexture_NoMip(Ptr data, short width, short height)
{
TQ3Mipmap 					mipmap;
TQ3TextureObject			texture;
TQ3SurfaceShaderObject		shader;

			/* CREATE MIPMAP */
			
	Data16ToMipmap(data,width,height,&mipmap);


			/* MAKE NEW MIPMAP TEXTURE */
			
	texture = Q3MipmapTexture_New(&mipmap);							// make new mipmap	
	GAME_ASSERT(texture);
			
	shader = Q3TextureShader_New(texture);
	GAME_ASSERT(shader);

	Q3Object_Dispose (texture);
	Q3Object_Dispose (mipmap.image);					// dispose of extra ref to storage object

	return(shader);	
}


/******************** DATA16 TO MIPMAP ********************/
//
// Creates a mipmap from an existing 16bit data buffer (note that buffer is not copied!)
//
// OUTPUT: mipmap = new mipmap holding texture image
//

static void Data16ToMipmap(Ptr data, short width, short height, TQ3Mipmap *mipmap)
{
long	size = width * height * 2;

			/* MAKE 16bit MIPMAP */

	mipmap->image = (void *)Q3MemoryStorage_NewBuffer ((unsigned char *) data, size,size);
	if (mipmap->image == nil)
	{
		DoAlert("Data16ToMipmap: Q3MemoryStorage_New Failed!");
		QD3D_ShowRecentError();
	}

	mipmap->useMipmapping 	= kQ3False;							// not actually using mipmaps (just 1 srcmap)
	mipmap->pixelType 		= kQ3PixelTypeRGB16;						
	mipmap->bitOrder 		= kQ3EndianBig;

	// Source port note: these images come from 'Timg' resources read in File.c.
	// File.c byteswaps the entire Timg, so they're little-endian now.
	mipmap->byteOrder 		= kQ3EndianLittle;

	mipmap->reserved 			= 0;
	mipmap->mipmaps[0].width 	= width;
	mipmap->mipmaps[0].height 	= height;
	mipmap->mipmaps[0].rowBytes = width*2;
	mipmap->mipmaps[0].offset 	= 0;
}


/**************** QD3D: GET MIPMAP STORAGE OBJECT FROM ATTRIB **************************/
//
// NOTE: the mipmap.image and surfaceShader are *valid* references which need to be de-referenced later!!!
//
// INPUT: attribSet
//
// OUTPUT: surfaceShader = shader extracted from attribute set
//

TQ3StorageObject QD3D_GetMipmapStorageObjectFromAttrib(TQ3AttributeSet attribSet)
{
TQ3Status	status;

TQ3TextureObject		texture;
TQ3Mipmap 				mipmap;
TQ3StorageObject		storage;
TQ3SurfaceShaderObject	surfaceShader;

			/* GET SHADER FROM ATTRIB */
			
	status = Q3AttributeSet_Get(attribSet, kQ3AttributeTypeSurfaceShader, &surfaceShader);
	GAME_ASSERT(status);

			/* GET TEXTURE */
			
	status = Q3TextureShader_GetTexture(surfaceShader, &texture);
	GAME_ASSERT(status);

			/* GET MIPMAP */
			
	status = Q3MipmapTexture_GetMipmap(texture,&mipmap);
	GAME_ASSERT(status);

		/* GET A LEGAL REF TO STORAGE OBJ */
			
	storage = mipmap.image;
	
			/* DISPOSE REFS */
			
	Q3Object_Dispose(texture);	
	Q3Object_Dispose(surfaceShader);
	return(storage);
}


/**************** QD3D: DATA16 TO TEXTURE_PIXMAP ***********************/
//
// Converts input data to pixmap texture
//
// INPUT: .
//
// OUTPUT: TQ3ShaderObject = shader object for texture map.
//

TQ3SurfaceShaderObject	QD3D_Data16ToTexture_Pixmap(Ptr data, short width, short height)
{
TQ3StoragePixmap			pixmap;
TQ3TextureObject			texture;
TQ3SurfaceShaderObject		shader;

			/* CREATE PIXMAP */
			
	Data16ToPixmap(data,width,height,&pixmap);


			/* MAKE NEW PIXMAP TEXTURE */
			
	texture = Q3PixmapTexture_New(&pixmap);				
	GAME_ASSERT(texture);
			
	shader = Q3TextureShader_New(texture);
	GAME_ASSERT(shader);

	Q3Object_Dispose (texture);
	Q3Object_Dispose (pixmap.image);					// dispose of extra ref to storage object

	return(shader);	
}


/******************** DATA16 TO PIXMAP ********************/
//
// Creates a pixmap from an existing 16bit data buffer
//
// NOTE: Assumes that GWorld is 16bit!!!!
//
// OUTPUT: pixmap = new pixmap holding texture image
//

static void Data16ToPixmap(Ptr data, short width, short height, TQ3StoragePixmap *pixmap)
{
long	size = width * height * 2;

			/* MAKE 16bit MIPMAP */

	pixmap->image = (void *)Q3MemoryStorage_NewBuffer ((unsigned char *) data, size, size);
	GAME_ASSERT(pixmap->image);

	pixmap->width 		= width;
	pixmap->height 		= height;
	pixmap->rowBytes	= width*2;

	pixmap->pixelSize 	= 16;
	pixmap->pixelType 	= kQ3PixelTypeRGB16;						
	pixmap->bitOrder 	= kQ3EndianBig;
	pixmap->byteOrder 	= kQ3EndianBig;
}


/**************** QD3D: GET PIXMAP STORAGE OBJECT FROM ATTRIB **************************/
//
// NOTE: the pixmap.image and surfaceShader are *valid* references which need to be de-referenced later!!!
//
// INPUT: attribSet
//
// OUTPUT: surfaceShader = shader extracted from attribute set
//

TQ3StorageObject QD3D_GetPixmapStorageObjectFromAttrib(TQ3AttributeSet attribSet)
{
TQ3Status	status;

TQ3TextureObject		texture;
TQ3StoragePixmap 		pixmap;
TQ3StorageObject		storage;
TQ3SurfaceShaderObject	surfaceShader;

			/* GET SHADER FROM ATTRIB */
			
	status = Q3AttributeSet_Get(attribSet, kQ3AttributeTypeSurfaceShader, &surfaceShader);
	GAME_ASSERT(status);

			/* GET TEXTURE */
			
	status = Q3TextureShader_GetTexture(surfaceShader, &texture);
	GAME_ASSERT(status);

			/* GET PIXMAP */
			
	status = Q3PixmapTexture_GetPixmap(texture,&pixmap);
	GAME_ASSERT(status);

		/* GET A LEGAL REF TO STORAGE OBJ */
			
	storage = pixmap.image;
	
			/* DISPOSE REFS */
			
	Q3Object_Dispose(texture);	
	Q3Object_Dispose(surfaceShader);
	return(storage);
}

#pragma mark -

//=======================================================================================================
//=============================== STYLE STUFF =====================================================
//=======================================================================================================


/****************** QD3D:  SET BACKFACE STYLE ***********************/

void SetBackFaceStyle(QD3DSetupOutputType *setupInfo, TQ3BackfacingStyle style)
{
TQ3Status status;
TQ3BackfacingStyle	backfacingStyle;

	status = Q3BackfacingStyle_Get(setupInfo->backfacingStyle, &backfacingStyle);
	GAME_ASSERT(status);

	if (style == backfacingStyle)							// see if already set to that
		return;
		
	status = Q3BackfacingStyle_Set(setupInfo->backfacingStyle, style);
	GAME_ASSERT(status);

}


/****************** QD3D:  SET FILL STYLE ***********************/

void SetFillStyle(QD3DSetupOutputType *setupInfo, TQ3FillStyle style)
{
TQ3Status 		status;
TQ3FillStyle	fillStyle;

	status = Q3FillStyle_Get(setupInfo->fillStyle, &fillStyle);
	GAME_ASSERT(status);

	if (style == fillStyle)							// see if already set to that
		return;
		
	status = Q3FillStyle_Set(setupInfo->fillStyle, style);
	GAME_ASSERT(status);

}


//=======================================================================================================
//=============================== MISC ==================================================================
//=======================================================================================================

/************** QD3D CALC FRAMES PER SECOND *****************/

void	QD3D_CalcFramesPerSecond(void)
{
UnsignedWide	wide;
unsigned long	now;
static	unsigned long then = 0;

			/* HANDLE SPECIAL DEMO MODE STUFF */
			
	if (gDemoMode)
	{
		gFramesPerSecond = DEMO_FPS;
		gFramesPerSecondFrac = 1.0f/gFramesPerSecond;
	
		do											// speed limiter
		{	
			Microseconds(&wide);
		}while((wide.lo - then) < (1000000.0f / 25.0f));
	}


			/* DO REGULAR CALCULATION */
			
	Microseconds(&wide);
	now = wide.lo;
	if (then != 0)
	{
		gFramesPerSecond = 1000000.0f/(float)(now-then);
		if (gFramesPerSecond < DEFAULT_FPS)			// (avoid divide by 0's later)
			gFramesPerSecond = DEFAULT_FPS;
	
#if 0
		if (GetKeyState(KEY_F8))
		{
			RGBColor	color;
			Str255		s;
				
			SetPort(GetWindowPort(gCoverWindow));
			GetForeColor(&color);
			FloatToString(gFramesPerSecond,s);		// print # rounded up to nearest integer
			MoveTo(20,20);
			ForeColor(greenColor);
			TextSize(12);
			TextMode(srcCopy);
			DrawString(s);
			DrawChar(' ');
			RGBForeColor(&color);
		}
#endif
		
		if (gFramesPerSecond < 9.0f)					// this is the minimum we let it go
			gFramesPerSecond = 9.0f;
		
	}
	else
		gFramesPerSecond = DEFAULT_FPS;
		
//	gFramesPerSecondFrac = 1/gFramesPerSecond;		// calc fractional for multiplication
	gFramesPerSecondFrac = __fres(gFramesPerSecond);	
	
	then = now;										// remember time	
}


#pragma mark -

/************ QD3D: SHOW RECENT ERROR *******************/

void QD3D_ShowRecentError(void)
{
TQ3Error	q3Err;
Str255		s;
	
	q3Err = Q3Error_Get(nil);
	if (q3Err == kQ3ErrorOutOfMemory)
		DoFatalAlert("QuickDraw 3D has run out of memory!");
	else
	if (q3Err == kQ3ErrorMacintoshError)
		DoFatalAlert("kQ3ErrorMacintoshError");
	else
	if (q3Err == kQ3ErrorNotInitialized)
		DoFatalAlert("kQ3ErrorNotInitialized");
	else
	if (q3Err == kQ3ErrorReadLessThanSize)
		DoFatalAlert("kQ3ErrorReadLessThanSize");
	else
	if (q3Err == kQ3ErrorViewNotStarted)
		DoFatalAlert("kQ3ErrorViewNotStarted");
	else
	if (q3Err != 0)
	{
		snprintf(s, sizeof(s), "QD3D Error %d\nLook up error code in QuesaErrors.h", q3Err);
		DoFatalAlert(s);
	}
}


/**************** QD3D: COLOR TO QUICKDRAW COLOR ******************/
//
// Converts QD3D colors to RGBColors.
//

void QD3D_ColorToQDColor(TQ3ColorRGB *in, RGBColor *out)
{
	out->red = in->r * 0xffff;
	out->green = in->g * 0xffff;
	out->blue = in->b * 0xffff;



}


/**************** QD3D: QD COLOR TO QD3D COLOR ******************/
//
// Converts QD3D colors to RGBColors.
//

void QD3D_QDColorToColor(RGBColor *in, TQ3ColorRGB *out)
{
	out->r = (float)in->red / (float)0xffff;
	out->g = (float)in->green / (float)0xffff;
	out->b = (float)in->blue / (float)0xffff;

}






#pragma mark -



/********************** SET RAVE FOG ******************************/

void QD3D_SetRaveFog(float fogStart, float fogEnd, float fogDensity, TQ3ColorARGB *fogColor, short fogMode)
{
	GAME_ASSERT(gQD3D_DrawContext);
		
		/* SET FOG PARAMETERS */
		
	gFogMode = (TQ3FogMode)fogMode;
	
	gFogStart 	= fogStart;
	gFogEnd 	= fogEnd;
	gFogDensity = fogDensity;
	gFogColor	= *fogColor;
}



/******************************* DISABLE FOG *********************************/

void QD3D_DisableFog(const QD3DSetupOutputType *setupInfo)
{

	TQ3FogStyleData	fogData;
	
	fogData.state		= kQ3Off;
	Q3FogStyle_Submit(&fogData, setupInfo->viewObject);
}


/******************************* REENABLE FOG *********************************/

void QD3D_ReEnableFog(const QD3DSetupOutputType *setupInfo)
{
		TQ3FogStyleData	fogData;
		
		fogData.state		= kQ3On;
		fogData.mode		= gFogMode;
		fogData.fogStart	= setupInfo->yon * gFogStart;
		fogData.fogEnd		= setupInfo->yon * gFogEnd;
		fogData.density		= gFogDensity;
		fogData.color		= gFogColor;
		
		Q3FogStyle_Submit(&fogData, setupInfo->viewObject);
}



/************************ SET TRIANGLE CACHE MODE *****************************/
//
// For ATI driver, sets triangle caching flag for xparent triangles
//

void QD3D_SetTriangleCacheMode(Boolean isOn)
{
	GAME_ASSERT(gQD3D_DrawContext);

		QASetInt(gQD3D_DrawContext, kQATag_ZSortedHint, isOn);
}	
				
/************************ SET Z WRITE *****************************/
//
// For ATI driver, turns on/off z-buffer writes
//

void QD3D_SetZWrite(Boolean isOn)
{
	if (!gQD3D_DrawContext)
		return;

	QASetInt(gQD3D_DrawContext, kQATag_ZBufferMask, isOn);
}	


/************************ SET BLENDING MODE ************************/

void QD3D_SetAdditiveBlending(Boolean enable)
{
	static const TQ3BlendingStyleData normalStyle = { GL_ONE, GL_ONE_MINUS_SRC_ALPHA };
	static const TQ3BlendingStyleData additiveStyle = { GL_ONE, GL_ONE };
	
	GAME_ASSERT(gQD3D_ViewObject);

	Q3BlendingStyle_Submit(enable ? &additiveStyle : &normalStyle, gQD3D_ViewObject);
}


#pragma mark -

/********************* SHOW NORMAL **************************/

void ShowNormal(TQ3Point3D *where, TQ3Vector3D *normal)
{
	gNormalWhere = *where;
	gNormal = *normal;

}

/********************* DRAW NORMAL **************************/

static void DrawNormal(TQ3ViewObject view)
{
TQ3LineData	line;

	line.lineAttributeSet = nil;

	line.vertices[0].attributeSet = nil;
	line.vertices[0].point = gNormalWhere;

	line.vertices[1].attributeSet = nil;
	line.vertices[1].point.x = gNormalWhere.x + gNormal.x * 400.0f;
	line.vertices[1].point.y = gNormalWhere.y + gNormal.y * 400.0f;
	line.vertices[1].point.z = gNormalWhere.z + gNormal.z * 400.0f;

	Q3Line_Submit(&line, view);
}


#pragma mark -

static void QASetInt(TQADrawContext	*a, int b, int c)
{
#pragma unused (a,b,c)

}







