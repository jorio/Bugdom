/****************************/
/*   	CAMERA.C    	    */
/* (c)1997-98 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveCamera_Manual(void);
static void InitLensFlares(void);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	CAM_MINY			20.0f

#define	CAMERA_CLOSEST		150.0f
#define	CAMERA_FARTHEST		800.0f

#define	NUM_FLARE_TYPES		4
#define	NUM_FLARES			6

/*********************/
/*    VARIABLES      */
/*********************/


Boolean				gDrawLensFlare;

TQ3Matrix4x4		gCameraWorldToFrustumMatrix;
TQ3Matrix4x4		gCameraWorldToViewMatrix;
TQ3Matrix4x4		gCameraViewToFrustumMatrix;

static float		gCameraLookAtAccel,gCameraFromAccelY,gCameraFromAccel;
static float		gCameraDistFromMe, gCameraHeightFactor,gCameraLookAtYOff;

float				gPlayerToCameraAngle = 0.0f;

static TQ3Point3D 	gTargetTo,gTargetFrom;

static GLuint		gLensFlareTextureNames[NUM_FLARE_TYPES] = {0,0,0,0};
static GLuint		gMoonFlareTextureName = 0;

static float	gFlareOffsetTable[]=
{
	1.0,
	.6,
	.3,
	1.0/8.0,
	-.25,
	-.5
};


static float	gFlareScaleTable[]=
{
	1,
	.1,
	.3,
	.2,
	.7,
	.3
};

static Byte	gFlareImageTable[]=
{
	0,
	1,
	2,
	3,
	2,
	1
};

static TQ3TriMeshData* gFlareMeshes[NUM_FLARES] = {nil,nil,nil,nil,nil,nil};

static RenderModifiers gFlareRenderMods;


/***************** INIT LENS FLARES *********************/

static void InitLensFlares(void)
{
static const TQ3Param2D uvs[4] = { {0,1}, {1,1}, {1,0}, {0,0} };

		/******************************/
		/* CREATE LENS FLARE TEXTURES */
		/******************************/

	if (!gDrawLensFlare)								// only load them for levels that need it
		return;

			/* SEE IF LOAD SPECIAL MOON FLARE TEXTURE */

	GAME_ASSERT_MESSAGE(!gMoonFlareTextureName, "Moon flare texture already loaded!");

	if (gLevelType == LEVEL_TYPE_NIGHT)
	{
		gMoonFlareTextureName = QD3D_NumberedTGAToTexture(1004, false, kRendererTextureFlags_ClampBoth);
		GAME_ASSERT(gMoonFlareTextureName);
	}

			/* LOAD ALL DEFAULT FLARE TEXTURES */

	for (int i = 0; i < NUM_FLARE_TYPES; i++)
	{
		GAME_ASSERT_MESSAGE(!gLensFlareTextureNames[i], "Lens flare texture already loaded!");

		gLensFlareTextureNames[i] = QD3D_NumberedTGAToTexture(1000+i, false, kRendererTextureFlags_ClampBoth);
		GAME_ASSERT(gLensFlareTextureNames[i]);
	}

			/* MAKE FLARE MESHES */

	for (int i = 0; i < NUM_FLARES; i++)
	{
		GAME_ASSERT_MESSAGE(!gFlareMeshes[i], "Flare mesh already allocated!");

		TQ3TriMeshData* mesh = Q3TriMeshData_New(2, 4, kQ3TriMeshDataFeatureVertexUVs);
		gFlareMeshes[i] = mesh;

		mesh->texturingMode = kQ3TexturingModeAlphaBlend;
		mesh->glTextureName = gLensFlareTextureNames[gFlareImageTable[i]];
		memcpy(mesh->vertexUVs, uvs, sizeof(uvs));

		mesh->triangles[0].pointIndices[0] = 0;
		mesh->triangles[0].pointIndices[1] = 1;
		mesh->triangles[0].pointIndices[2] = 2;

		mesh->triangles[1].pointIndices[0] = 0;
		mesh->triangles[1].pointIndices[1] = 2;
		mesh->triangles[1].pointIndices[2] = 3;
	}

			/* RENDERER BITS */

	Render_SetDefaultModifiers(&gFlareRenderMods);
	gFlareRenderMods.statusBits
			|= STATUS_BIT_NOZWRITE
			| STATUS_BIT_NOFOG
			| STATUS_BIT_NULLSHADER
			| STATUS_BIT_GLOW
			;
}


/***************** DISPOSE LENS FLARES ******************/
//
// Called by CleanupLevel to dispose of the lens flare textures and other memory
//

void DisposeLensFlares(void)
{
			/* NUKE MOON TEXTURE */

	if (gMoonFlareTextureName)							// nuke any old moon shader
	{
		glDeleteTextures(1, &gMoonFlareTextureName);
		gMoonFlareTextureName = 0;
	}

			/* NUKE THE USUAL FLARE TEXTURES */

	for (int i = 0; i < NUM_FLARE_TYPES; i++)
	{
		if (gLensFlareTextureNames[i])
		{
			glDeleteTextures(1, &gLensFlareTextureNames[i]);
			gLensFlareTextureNames[i] = 0;
		}
	}

			/* NUKE FLARE MESHES */

	for (int i = 0; i < NUM_FLARES; i++)
	{
		if (gFlareMeshes[i])
		{
			Q3TriMeshData_Dispose(gFlareMeshes[i]);
			gFlareMeshes[i] = nil;
		}
	}
}


/*************** INIT CAMERA ***********************/
//
// This MUST be called after I've (the player) been created so that we know
// where to put the camera.
//

void InitCamera(void)
{		
int	i;

	InitLensFlares();

	ResetCameraSettings();
					
			/******************************/
			/* SET CAMERA STARTING COORDS */
			/******************************/
			
			
			/* FIRST CHOOSE AN ARBITRARY POINT BEHIND PLAYER */
			
	if (gPlayerObj)
	{
		float	dx,dz;
	
		dx = sin(gPlayerObj->Rot.y) * 10.0f;
		dz = cos(gPlayerObj->Rot.y) * 10.0f;
		
		gGameViewInfoPtr->currentCameraCoords.x = gPlayerObj->Coord.x + dx;			// camera coords
		gGameViewInfoPtr->currentCameraCoords.z = gPlayerObj->Coord.z + dz;
		gGameViewInfoPtr->currentCameraCoords.y = gPlayerObj->Coord.y + 300.0f;

		gGameViewInfoPtr->currentCameraLookAt.x = gPlayerObj->Coord.x;				// camera lookAt
		gGameViewInfoPtr->currentCameraLookAt.y = gPlayerObj->Coord.y + 100.0f;
		gGameViewInfoPtr->currentCameraLookAt.z = gPlayerObj->Coord.z;
	}
			
			
			/* CALL SOME STUFF TO GET IT INTO POSITION */
			
	gFramesPerSecondFrac = 1.0/20.0;
	for (i = 0; i < 100; i++)
		UpdateCamera();														// prime it

	QD3D_UpdateCameraFromTo(gGameViewInfoPtr, &gTargetFrom, &gTargetTo);	// set desired numbers	
}


/******************** RESET CAMERA SETTINGS **************************/

void ResetCameraSettings(void)
{
	gCameraLookAtAccel 	= 9;
	gCameraFromAccel 	= 4.5;
	gCameraFromAccelY	= 1.5;
	gCameraDistFromMe 	= 500;
	gCameraHeightFactor = 0.3;
	gCameraLookAtYOff 	= 95; 
}

/*************** UPDATE CAMERA ***************/

void UpdateCamera(void)
{
float	fps = gFramesPerSecondFrac;
	

			/************************/
			/* HANDLE CAMERA MOTION */
			/************************/

	if (GetKeyState(kKey_ZoomIn))	
	{
		gCameraDistFromMe -= fps * 200.0f;		// closer camera
		if (gCameraDistFromMe < CAMERA_CLOSEST)
			gCameraDistFromMe = CAMERA_CLOSEST;
	}
	else
	if (GetKeyState(kKey_ZoomOut))	
	{
		gCameraDistFromMe += fps * 200.0f;		// farther camera
		if (gCameraDistFromMe > CAMERA_FARTHEST)
			gCameraDistFromMe = CAMERA_FARTHEST;
	}
		

	MoveCamera_Manual();
	
}

/********************** CALC CAMERA MATRIX INFO ************************/
//
// Must be called inside render start/end loop!!!
//

void CalcCameraMatrixInfo(QD3DSetupOutputType *setupInfo)
{
			/* INIT PROJECTION MATRIX */

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(
			Q3Math_RadiansToDegrees(setupInfo->fov),	// fov
			Render_GetViewportAspectRatio(setupInfo->paneClip),		// aspect
			setupInfo->hither,		// hither
			setupInfo->yon);		// yon



			/* INIT MODELVIEW MATRIX */

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(
			setupInfo->currentCameraCoords.x,	setupInfo->currentCameraCoords.y,	setupInfo->currentCameraCoords.z,
			setupInfo->currentCameraLookAt.x,	setupInfo->currentCameraLookAt.y,	setupInfo->currentCameraLookAt.z,
			setupInfo->currentCameraUpVector.x,	setupInfo->currentCameraUpVector.y,	setupInfo->currentCameraUpVector.z);


			/* UPDATE LIGHT POSITIONS */

	for (int i = 0; i < setupInfo->lightList.numFillLights; i++)
	{
		GLfloat lightVec[4];

		lightVec[0] = -setupInfo->lightList.fillDirection[i].x;			// negate vector because OGL is stupid
		lightVec[1] = -setupInfo->lightList.fillDirection[i].y;
		lightVec[2] = -setupInfo->lightList.fillDirection[i].z;
		lightVec[3] = 0;									// when w==0, this is a directional light, if 1 then point light
		glLightfv(GL_LIGHT0+i, GL_POSITION, lightVec);
	}



			/* GET CAMERA VIEW MATRIX INFO */

	/* // TODO NOQUESA: original QD3D version
	Q3View_GetWorldToFrustumMatrixState(viewPtr->viewObject, &gCameraWorldToFrustumMatrix);
	Q3View_GetFrustumToWindowMatrixState(viewPtr->viewObject, &gCameraFrustumToWindowMatrix);
	MatrixMultiplyFast(&gCameraWorldToFrustumMatrix, &gCameraFrustumToWindowMatrix, &gCameraWorldToWindowMatrix);
	
	Q3Camera_GetWorldToView(viewPtr->cameraObject, &gCameraWorldToViewMatrix);	
	Q3Camera_GetViewToFrustum(viewPtr->cameraObject, &gCameraViewToFrustumMatrix);
	*/


	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)&gCameraWorldToViewMatrix);				// get camera's world to view matrix
	glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *)&gCameraViewToFrustumMatrix);			// get camera's view to frustrum matrix (to calc screen coords)
	Q3Matrix4x4_Multiply(&gCameraWorldToViewMatrix, &gCameraViewToFrustumMatrix, &gCameraWorldToFrustumMatrix);		// calc world to frustum matrix


#if 0 // TODO NOQUESA
			/* CALCULATE THE ADJUSTMENT MATRIX */
			//
			// this gets a view->world matrix for putting stuff in the infobar.
			//

	//Q3Matrix4x4_Invert(&gCameraWorldToWindowMatrix,&gCameraWindowToWorldMatrix);  // Original QD3D version
	Q3Matrix4x4_Invert(&gCameraWorldToViewMatrix,&gCameraWindowToWorldMatrix);
#endif


			/* PREPARE FRUSTUM PLANES FOR SPHERE VISIBILITY CHECKS */
			// (Source port addition)

	UpdateFrustumPlanes(&gCameraWorldToFrustumMatrix);



	CHECK_GL_ERROR();
}

/**************** MOVE CAMERA: MANUAL ********************/

static void MoveCamera_Manual(void)
{
TQ3Point3D	from,to,target;
float		distX,distZ,distY,dist;
TQ3Vector2D	pToC;
float		myX,myY,myZ;
float		fps = gFramesPerSecondFrac;

	if (!gPlayerObj)
		return;

	gPlayerToCameraAngle = PI - CalcYAngleFromPointToPoint(PI-gPlayerToCameraAngle, gMyCoord.x, gMyCoord.z,	// calc angle of camera around player
												 gGameViewInfoPtr->currentCameraCoords.x,
												 gGameViewInfoPtr->currentCameraCoords.z);	


	myX = gMyCoord.x;
	myY = gMyCoord.y + gPlayerObj->BottomOff;
	myZ = gMyCoord.z;
	
			/**********************/
			/* CALC LOOK AT POINT */
			/**********************/

	target.x = myX;								// accelerate "to" toward target "to"
	target.y = myY + gCameraLookAtYOff;
	target.z = myZ;
	gTargetTo = target;

	distX = target.x - gGameViewInfoPtr->currentCameraLookAt.x;
	distY = target.y - gGameViewInfoPtr->currentCameraLookAt.y;
	distZ = target.z - gGameViewInfoPtr->currentCameraLookAt.z;

	if (distX > 350.0f)		
		distX = 350.0f;
	else
	if (distX < -350.0f)
		distX = -350.0f;
	if (distZ > 350.0f)
		distZ = 350.0f;
	else
	if (distZ < -350.0f)
		distZ = -350.0f;

	to.x = gGameViewInfoPtr->currentCameraLookAt.x+(distX * (fps * gCameraLookAtAccel));
	to.y = gGameViewInfoPtr->currentCameraLookAt.y+(distY * (fps * (gCameraLookAtAccel*.7)));
	to.z = gGameViewInfoPtr->currentCameraLookAt.z+(distZ * (fps * gCameraLookAtAccel));


			/*******************/
			/* CALC FROM POINT */
			/*******************/
				
	pToC.x = gGameViewInfoPtr->currentCameraCoords.x - myX;				// calc player->camera vector
	pToC.y = gGameViewInfoPtr->currentCameraCoords.z - myZ;
	Q3Vector2D_Normalize(&pToC,&pToC);									// normalize it
	
	target.x = myX + (pToC.x * gCameraDistFromMe);						// target is appropriate dist based on cam's current coord
	target.z = myZ + (pToC.y * gCameraDistFromMe);


			/* MOVE CAMERA TOWARDS POINT */
					
	distX = target.x - gGameViewInfoPtr->currentCameraCoords.x;
	distZ = target.z - gGameViewInfoPtr->currentCameraCoords.z;
	
	if (distX > 500.0f)													// pin max accel factor
		distX = 500.0f;
	else
	if (distX < -500.0f)
		distX = -500.0f;
	if (distZ > 500.0f)
		distZ = 500.0f;
	else
	if (distZ < -500.0f)
		distZ = -500.0f;
	
	distX = distX * (fps * gCameraFromAccel);
	distZ = distZ * (fps * gCameraFromAccel);

			/* SEE IF GOING TO CRASH SCROLL CODE */

	if (distX > TERRAIN_SUPERTILE_UNIT_SIZE)
		distX = TERRAIN_SUPERTILE_UNIT_SIZE;
	else
	if (distX < -TERRAIN_SUPERTILE_UNIT_SIZE)
		distX = -TERRAIN_SUPERTILE_UNIT_SIZE;

	if (distZ > TERRAIN_SUPERTILE_UNIT_SIZE)
		distZ = TERRAIN_SUPERTILE_UNIT_SIZE;
	else
	if (distZ < -TERRAIN_SUPERTILE_UNIT_SIZE)
		distZ = -TERRAIN_SUPERTILE_UNIT_SIZE;	
	
	from.x = gGameViewInfoPtr->currentCameraCoords.x+distX;
	from.y = gGameViewInfoPtr->currentCameraCoords.y;  // from.y mustn't be NaN for matrix transform in cam swivel below -- actual Y computed later
	from.z = gGameViewInfoPtr->currentCameraCoords.z+distZ;

			
	/*****************************/
	/* SEE IF USER ROTATE CAMERA */
	/*****************************/

	if (gCameraControlDelta.x != 0)
	{
		TQ3Matrix4x4	m;
		float			r = gCameraControlDelta.x * fps;
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

		/*******************************/
		/* MOVE ABOVE ANY SOLID OBJECT */
		/*******************************/
		//
		// well... not any solid object, just reasonable ones
		//

	if (DoSimpleBoxCollision(target.y + 100.0f, target.y - 100.0f,
							target.x - 100.0f, target.x + 100.0f,
							target.z + 100.0f, target.z - 100.0f,
							CTYPE_BLOCKCAMERA))
	{
		ObjNode *obj = gCollisionList[0].objectPtr;					// get collided object
		if (obj)
		{
			CollisionBoxType *coll = obj->CollisionBoxes;			// get object's collision box
			if (coll)
			{
				dist = coll->top - target.y;
				if (dist < 500.0f)									// see if its reasonable
					target.y = coll->top + 100.0f;					// set target on top of object			
				else
				if (dist >= 500.0f)
					target.y += 500.0f;
			}
		}
	}

	gTargetFrom = target;
	
	dist = (target.y - gGameViewInfoPtr->currentCameraCoords.y)*gCameraFromAccelY;	// calc dist from current y to desired y
	from.y = gGameViewInfoPtr->currentCameraCoords.y+(dist*fps);

	if (from.y < (to.y+CAM_MINY))													// make sure camera never under the "to" point (insures against camera flipping over)
		from.y = (to.y+CAM_MINY);	
	
	
	
	if (gDoCeiling)
	{
				/* MAKE SURE NOT ABOVE CEILING */
		
		dist = GetTerrainHeightAtCoord(from.x, from.z, CEILING) - 60.0f;
		if (from.y > dist)
			from.y = dist;
	}

			/* MAKE SURE NOT UNDERGROUND */
			
	dist = GetTerrainHeightAtCoord(from.x, from.z, FLOOR) + 60.0f;
	if (from.y < dist)
		from.y = dist;


				/* SEE IF KEEP CAMERA POS STEADY */
				
	if (gLevelType == LEVEL_TYPE_FOREST)
		if (gPlayerMode == PLAYER_MODE_BUG)
			if (gPlayerObj->Skeleton->AnimNum == PLAYER_ANIM_BEINGEATEN)
				from = gGameViewInfoPtr->currentCameraCoords;


				/**********************/
				/* UPDATE CAMERA INFO */	
				/**********************/

	QD3D_UpdateCameraFromTo(gGameViewInfoPtr,&from,&to);
}


#pragma mark ======== LENS FLARE ==========

/*********************** DRAW LENS FLARE ***************************/

void DrawLensFlare(const QD3DSetupOutputType *setupInfo)
{
float			x,y,dot;
TQ3Point3D		sunCoord;
TQ3Point3D		sunFrustumCoord;
TQ3Point3D		from;
TQ3Vector3D		axis,lookAtVector,sunVector;
float			length;
const bool doMoon = gMoonFlareTextureName != 0;

	if (!gDrawLensFlare)
		return;

	from = setupInfo->currentCameraCoords;

			/* CALC SUN COORD */

	sunCoord.x = from.x - gLightDirection1.x * 3500.0f;
	sunCoord.y = from.y - gLightDirection1.y * 3500.0f;
	sunCoord.z = from.z - gLightDirection1.z * 3500.0f;

	/* CALC DOT PRODUCT BETWEEN VIEW AND LIGHT VECTORS TO SEE IF OUT OF RANGE */

	FastNormalizeVector(from.x - sunCoord.x,
						from.y - sunCoord.y,
						from.z - sunCoord.z,
						&sunVector);

	FastNormalizeVector(setupInfo->currentCameraLookAt.x - from.x,
						setupInfo->currentCameraLookAt.y - from.y,
						setupInfo->currentCameraLookAt.z - from.z,
						&lookAtVector);

	dot = -Q3Vector3D_Dot(&lookAtVector, &sunVector);
	if (dot <= 0.0f)
		return;

			/* CALC SCREEN COORDINATE OF LIGHT */

	Q3Point3D_Transform(&sunCoord, &gCameraWorldToFrustumMatrix, &sunFrustumCoord);

			/* CALC VECTOR FROM CENTER TO LIGHT */

	length = sqrtf(sunFrustumCoord.x*sunFrustumCoord.x + sunFrustumCoord.y*sunFrustumCoord.y);
	FastNormalizeVector(sunFrustumCoord.x, sunFrustumCoord.y, 0, &axis);

			/* CALC SCREEN COORDS */
			// TODO: just position the flare relative to the frustum instead of relative to 640x480 minus clipped area

	float screenHalfWidth = 640.0f / 2.0f;
	float screenHalfHeight = (480.0f - setupInfo->paneClip.top - setupInfo->paneClip.bottom) / 2.0f;

			/***************/
			/* DRAW FLARES */
			/***************/

	for (int i = 0; i < NUM_FLARES; i++)
	{
		TQ3TriMeshData* mesh = gFlareMeshes[i];

		float	s,o;

		if (i > 0)									// always draw sun, but fade flares based on dot
			mesh->diffuseColor.a = dot;				// flare brightness == dot product

		o = gFlareOffsetTable[i];
		if (doMoon && (i == 0))						// see if do moon
			s = .3f;								// special scale for moon
		else
			s = gFlareScaleTable[i];

		x = screenHalfWidth  * (1.0f + (axis.x * length * o));
		y = screenHalfHeight * (1.0f - (axis.y * length * o)) + setupInfo->paneClip.top;

		float extent = 120.0f * s;
		mesh->points[0] = (TQ3Point3D) { x - extent, y + extent, 0 };
		mesh->points[1] = (TQ3Point3D) { x + extent, y + extent, 0 };
		mesh->points[2] = (TQ3Point3D) { x + extent, y - extent, 0 };
		mesh->points[3] = (TQ3Point3D) { x - extent, y - extent, 0 };

		if (doMoon && (i == 0))							// see if do moon
			mesh->glTextureName = gMoonFlareTextureName;
		else
			mesh->glTextureName = gLensFlareTextureNames[gFlareImageTable[i]];
	}

	Render_SubmitMeshList(NUM_FLARES, gFlareMeshes, NULL, &gFlareRenderMods, NULL);
}

