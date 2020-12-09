/****************************/
/*   	CAMERA.C    	    */
/* (c)1997-98 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	float					gFramesPerSecond,gFramesPerSecondFrac;
extern	ObjNode					*gPlayerObj;
extern	TQ3Point3D				gMyCoord;
extern	Boolean					gPlayerGotKilledFlag,gDoCeiling;
extern	TQ3Object				gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TQ3Vector3D				gLightDirection1;
extern	u_short					gLevelType;
extern	Byte					gPlayerMode;
extern	CollisionRec			gCollisionList[];
extern	SDL_Window*				gSDLWindow;

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

TQ3Matrix4x4		gCameraWorldToFrustumMatrix,gCameraFrustumToWindowMatrix,
					gCameraWorldToWindowMatrix,gCameraWindowToWorldMatrix;
TQ3Matrix4x4		gCameraWorldToViewMatrix, gCameraViewToFrustumMatrix;

static TQ3RationalPoint4D	gFrustumPlanes[6];

static float		gCameraLookAtAccel,gCameraFromAccelY,gCameraFromAccel;
static float		gCameraDistFromMe, gCameraHeightFactor,gCameraLookAtYOff;

static	TQ3Vector3D	gOldVector;

float				gPlayerToCameraAngle = 0.0f;

static TQ3Point3D 	gTargetTo,gTargetFrom;

static TQ3Point3D	gSunCoord;

static TQ3SurfaceShaderObject	gLensFlareShader[NUM_FLARE_TYPES] = {nil,nil,nil,nil};
static TQ3SurfaceShaderObject	gMoonFlareShader = nil;

static TQ3PolygonData		gLensQuad;
static TQ3Vertex3D			gLensVerts[4] = { {{0,0,0},nil}, {{0,0,0},nil}, {{0,0,0},nil}, {{0,0,0},nil} };

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


/***************** INIT LENS FLARES *********************/

static void InitLensFlares(void)
{
TQ3AttributeSet	attrib;
long			i;
static const TQ3Param2D uvs[4] = { {0,1}, {1,1}, {1,0}, {0,0} };

		/******************************/
		/* CREATE LENS FLARE TEXTURES */
		/******************************/

	if (gDrawLensFlare)									// only load them for levels that need it
	{
			/* SEE IF LOAD SPECIAL MOON FLARE */
				
		if (gLevelType == LEVEL_TYPE_NIGHT)
		{
			gMoonFlareShader = QD3D_GetTextureMapFromPICTResource(1004, false);
			GAME_ASSERT(gMoonFlareShader);
		}
		else
		if (gMoonFlareShader)							// nuke any old moon shader
		{
			Q3Object_Dispose(gMoonFlareShader);
			gMoonFlareShader = nil;
		}
			
			
			/* LOAD ALL DEFAULT FLARES IF NOT ALREADY LOADED */
				
		if (gLensFlareShader[0] == nil)
		{
			for (i = 0; i < NUM_FLARE_TYPES; i++)
			{
				gLensFlareShader[i] = QD3D_GetTextureMapFromPICTResource(1000+i, false);
				GAME_ASSERT(gLensFlareShader[i]);
			}
				
					/* INIT LENSE FLARE GEOMETRY */
					
			gLensQuad.numVertices = 4;
			gLensQuad.vertices = &gLensVerts[0];
			gLensQuad.polygonAttributeSet = nil;	
			
			for (i = 0; i < 4; i++)
			{
				attrib = Q3AttributeSet_New();
				Q3AttributeSet_Add(attrib, kQ3AttributeTypeSurfaceUV, &uvs[i]);
				gLensVerts[i].attributeSet = attrib;
				gLensVerts[i].point.z = -.001;

			}
		}
	}
}


/***************** DISPOSE LENS FLARES ******************/
//
// Called by CleanupLevel to dispose of the lens flare textures and other memory
//

void DisposeLensFlares(void)
{
int	i;

		/* NUKE MOON */
		
	if (gMoonFlareShader)							// nuke any old moon shader
	{
		Q3Object_Dispose(gMoonFlareShader);
		gMoonFlareShader = nil;
	}

	/* NUKE THE USUAL FLARES */
		
	for (i = 0; i < NUM_FLARE_TYPES; i++)
	{
		if (gLensFlareShader[i])
		{
			Q3Object_Dispose(gLensFlareShader[i]);
			gLensFlareShader[i] = nil;
		}
	}
	
		/* ALSO DISPOSE OF UV ATTRIBS */
		
	for (i = 0; i < 4; i++)
	{
		if (gLensVerts[i].attributeSet)
		{
			Q3Object_Dispose(gLensVerts[i].attributeSet);
			gLensVerts[i].attributeSet = nil;
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
		
		gGameViewInfoPtr->currentCameraCoords.x = gPlayerObj->Coord.x + dx;
		gGameViewInfoPtr->currentCameraCoords.z = gPlayerObj->Coord.z + dz;
		gGameViewInfoPtr->currentCameraCoords.y = gPlayerObj->Coord.y + 300.0f;

		gGameViewInfoPtr->currentCameraLookAt.x = gPlayerObj->Coord.x;
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
	
#ifdef FORMAC
	gCameraDistFromMe 	= 300;
#else	
	gCameraDistFromMe 	= 500;
#endif
	
	gCameraHeightFactor = 0.3;
	gCameraLookAtYOff 	= 95; 

	gOldVector.x = 0;
	gOldVector.z = -1;
	gOldVector.y = 0;
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

void CalcCameraMatrixInfo(QD3DSetupOutputType *viewPtr)
{
			/* GET CAMERA VIEW MATRIX INFO */
			
	Q3View_GetWorldToFrustumMatrixState(viewPtr->viewObject, &gCameraWorldToFrustumMatrix);
	Q3View_GetFrustumToWindowMatrixState(viewPtr->viewObject, &gCameraFrustumToWindowMatrix);
	MatrixMultiplyFast(&gCameraWorldToFrustumMatrix, &gCameraFrustumToWindowMatrix, &gCameraWorldToWindowMatrix);
	
	Q3Camera_GetWorldToView(viewPtr->cameraObject, &gCameraWorldToViewMatrix);	
	Q3Camera_GetViewToFrustum(viewPtr->cameraObject, &gCameraViewToFrustumMatrix);	
	
			/* CALCULATE THE ADJUSTMENT MATRIX */
			//
			// this gets a view->world matrix for putting stuff in the infobar.
			//
				
	Q3Matrix4x4_Invert(&gCameraWorldToWindowMatrix,&gCameraWindowToWorldMatrix);


			/* PREPARE FRUSTUM PLANES FOR SPHERE VISIBILITY CHECKS */
			// (Source port addition)

#define M(a,b)	gCameraWorldToFrustumMatrix.value[a][b]
	// Planes 0,1: right, left
	// Planes 2,3: top, bottom
	// Planes 4,5: near, far
	for (int plane = 0; plane < 6; plane++)
	{
		int axis = plane >> 1u;							// 0=X, 1=Y, 2=Z
		float sign = plane%2 == 0 ? -1.0f : 1.0f;		// To pick either frustum plane on the axis.

		float x = M(0,3) + sign * M(0,axis);
		float y = M(1,3) + sign * M(1,axis);
		float z = M(2,3) + sign * M(2,axis);
		float w = M(3,3) + sign * M(3,axis);

		float t = sqrtf(x*x + y*y + z*z);				// Normalize
		gFrustumPlanes[plane].x = x/t;
		gFrustumPlanes[plane].y = y/t;
		gFrustumPlanes[plane].z = z/t;
		gFrustumPlanes[plane].w = w/t;
	}
#undef M
}

/********************** SPHERE VISIBILITY CHECK ************************/

Boolean IsSphereInFrustum(const TQ3Point3D* inWorldPt, float radius)
{
	for (int plane = 0; plane < 6; plane++)
	{
		float planeDot =
				inWorldPt->x * gFrustumPlanes[plane].x +
				inWorldPt->y * gFrustumPlanes[plane].y +
				inWorldPt->z * gFrustumPlanes[plane].z +
				gFrustumPlanes[plane].w;

		if (planeDot <= -radius)
			return false;
	}

	return true;
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

	if (GetKeyState(kKey_SwivelCameraLeft))
	{
		TQ3Matrix4x4	m;
		float			r;
		
		r = fps * -1.5f;		
		Q3Matrix4x4_SetRotateAboutPoint(&m, &to, 0, r, 0);	
		Q3Point3D_Transform(&from, &m, &from);	
	}	
	else
	if (GetKeyState(kKey_SwivelCameraRight))
	{
		TQ3Matrix4x4	m;
		float			r;
		
		r = fps * 1.5f;		
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
TQ3ViewObject	view = setupInfo->viewObject;
short			i;
float			x,y,dot;
TQ3Point3D		sunScreenCoord,from;
float			cx,cy;
float			dx,dy,length;
TQ3Vector3D		axis,lookAtVector,sunVector;
TQ3ColorRGB		transColor;

	if (!gDrawLensFlare)
		return;

	from = setupInfo->currentCameraCoords;

			/* CALC SUN COORD */
			
	gSunCoord.x = from.x - gLightDirection1.x * 3500.0f;
	gSunCoord.y = from.y - gLightDirection1.y * 3500.0f;
	gSunCoord.z = from.z - gLightDirection1.z * 3500.0f;


	/* CALC DOT PRODUCT BETWEEN VIEW AND LIGHT VECTORS TO SEE IF OUT OF RANGE */

	FastNormalizeVector(from.x - gSunCoord.x,
						from.y - gSunCoord.y,
						from.z - gSunCoord.z,
						&sunVector);

	FastNormalizeVector(setupInfo->currentCameraLookAt.x - from.x,
						setupInfo->currentCameraLookAt.y - from.y,
						setupInfo->currentCameraLookAt.z - from.z,
						&lookAtVector);

	dot = -Q3Vector3D_Dot(&lookAtVector, &sunVector);
	if (dot <= 0.0f)
		return;

			/* CALC BRIGHTNESS OF FLARE */
			
	transColor.r = 
	transColor.g = 
	transColor.b = dot;



			/* CALC SCREEN COORDINATE OF LIGHT */
			
	Q3Point3D_Transform(&gSunCoord, &gCameraWorldToWindowMatrix, &sunScreenCoord);

//	if (sunScreenCoord.x < -SUN_RADIUS)					// the sun must be visible for lens flare
//		return;
//	if (sunScreenCoord.y < -SUN_RADIUS)
//		return;		
//	if (sunScreenCoord.x > (GAME_VIEW_WIDTH + SUN_RADIUS))
//		return;
//	if (sunScreenCoord.y > (GAME_VIEW_HEIGHT + SUN_RADIUS))
//		return;
		
			/* CALC CENTER OF SCREEN */

	int windowWidth, windowHeight;
	SDL_GetWindowSize(gSDLWindow, &windowWidth, &windowHeight);
			
	cx = windowWidth/2;
	cy = windowHeight/2;
	

			/* CALC VECTOR FROM CENTER TO LIGHT */
			
	dx = sunScreenCoord.x - cx;
	dy = sunScreenCoord.y - cy;
	length = sqrt(dx*dx + dy*dy);	
	FastNormalizeVector(dx, dy, 0, &axis);
	

			/************/
			/* SET TAGS */
			/************/
			
	Q3Push_Submit(view);
	QD3D_DisableFog(setupInfo);
	QD3D_SetTriangleCacheMode(false);
	QD3D_SetAdditiveBlending(true);
	QD3D_SetZWrite(false);
	Q3Shader_Submit(setupInfo->nullShaderObject, view);							// use null shader
	Q3BackfacingStyle_Submit(kQ3BackfacingStyleRemove, view);


			/***************/
			/* DRAW FLARES */
			/***************/
		
	Q3MatrixTransform_Submit(&gCameraWindowToWorldMatrix, view);
				
	for (i = 0; i < NUM_FLARES; i++)
	{			
		float	s,o;
		
		if (i > 0)									// always draw sun, but fade flares based on dot
			Q3Attribute_Submit(kQ3AttributeTypeTransparencyColor, &transColor, view);	

	
		o = gFlareOffsetTable[i];
		if (gMoonFlareShader && (i == 0))										// see if do moon
			s = .3;
		else
			s = gFlareScaleTable[i];
		
		// Source port addition: adjust lens flare scale to window dimensions
		// (Original dimensions are valid for 640x480)
		s *= windowHeight / 480.0f;

		x = cx + axis.x * length * o;
		y = cy + axis.y * length * o;
				
		gLensVerts[0].point.x = x - (120.0f * s);
		gLensVerts[0].point.y = y + (120.0f * s);

		gLensVerts[1].point.x = x + (120.0f * s);
		gLensVerts[1].point.y = y + (120.0f * s);

		gLensVerts[2].point.x = x+(120.0f * s);
		gLensVerts[2].point.y = y-(120.0f * s);

		gLensVerts[3].point.x = x-(120.0f * s);
		gLensVerts[3].point.y = y-(120.0f * s);
	
		if (gMoonFlareShader && (i == 0))										// see if do moon
			Q3Object_Submit(gMoonFlareShader, view);							// submit texture
		else
			Q3Object_Submit(gLensFlareShader[gFlareImageTable[i]], view);		// submit texture
		Q3Polygon_Submit(&gLensQuad, view);										// draw it
				
	}
	Q3ResetTransform_Submit(view);												// reset matrix


			/* RESTORE MODES */
			
	Q3Shader_Submit(setupInfo->shaderObject, view);								// restore shader
	QD3D_SetZWrite(true);
	QD3D_SetTriangleCacheMode(true);
	QD3D_SetAdditiveBlending(false);
	QD3D_ReEnableFog(setupInfo);
	Q3Pop_Submit(view);
}















