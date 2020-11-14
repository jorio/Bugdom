/*******************************/
/*   	ME BALL.C			   */
/* (c)1997-98 Pangea Software  */
/* By Brian Greenstone         */
/*******************************/


/****************************/
/*    EXTERNALS             */
/****************************/


extern	ObjNode					*gCurrentNode,*gPlayerObj;
extern	float					gFramesPerSecondFrac,gFramesPerSecond,gPlayerToCameraAngle;
extern	TQ3Point3D				gCoord,gMyCoord;
extern	TQ3Vector3D				gDelta;
extern	Byte					gPlayerMode;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	u_short					gMyLatestPathTileNum,gMyLatestTileAttribs;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	TQ3GroupPosition		gMyLightPosition;
extern	float					gPlayerMaxSpeed,gMyDistToFloor,gBallTimer;
extern	TQ3Object				gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	unsigned long 			gInfobarUpdateBits;
extern	Boolean					gPlayerCanMove,gPlayerUsingKeyControl;
extern	TQ3TriMeshData			**gLocalTriMeshesOfSkelType;
extern	PrefsType	gGamePrefs;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void UpdatePlayer_Ball(ObjNode *theNode);
static void MovePlayer_Ball(ObjNode *theNode);
static void DoPlayerControl_Ball(ObjNode *theNode, float slugFactor);
static void SpinBall(ObjNode *theNode);
static void LeaveNitroTrail(void);
static void StartNitroTrail(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	PLAYER_BALL_SCALE			1.0 



#define	PLAYER_BALL_ACCEL			66.0f
#define	PLAYER_BALL_FRICTION_ACCEL	400.0f
#define	PLAYER_MAX_SPEED_BALL		2800.0f;

#define	PLAYER_BALL_JUMPFORCE		1400.0f

#define	NITRO_RING_SIZE				16			// #particles in a nitro ring

/*********************/
/*    VARIABLES      */
/*********************/

static float	gNitroTimer,gNitroTrailTick;
int				gNitroParticleGroup;

TQ3Matrix4x4	gBallRotationMatrix;

Boolean			gPlayerKnockOnButt = false;
TQ3Vector3D		gPlayerKnockOnButtDelta;



/*************************** INIT PLAYER: BALL ****************************/
//
// Creates an ObjNode for the player in the Ball form.
//
//
// INPUT:	oldObj = old objNode to base some parameters on.  nil = no old player obj
//			where = floor coord where to init the player.
//

void InitPlayer_Ball(ObjNode *oldObj, TQ3Point3D *where)
{
ObjNode	*newObj;
long	numTriMeshes,i;
float	rotY;

	if (gPlayerMode != PLAYER_MODE_BUG)
		DoFatalAlert("\pInitPlayer_Ball: to become ball, player must be bug");
					
	gPlayerKnockOnButt = false;
					
	Q3Matrix4x4_SetIdentity(&gBallRotationMatrix);
					
	rotY = oldObj->Rot.y;

			/* REBUILD SKELETON IN ZERO-ROTATION POSITION */

	oldObj->Rot.y = 0;	
	UpdateObjectTransforms(oldObj);		
	GetModelCurrentPosition(oldObj->Skeleton);
	UpdateSkinnedGeometry(oldObj);													// update skeleton geometry
	

					
					/********************/
					/* CREATE MY OBJECT */
					/********************/	
	
	gMyCoord.x = where->x;
	gMyCoord.y = where->y + PLAYER_BALL_FOOTOFFSET;
	gMyCoord.z = where->z;
	
	
	gNewObjectDefinition.genre 		= DISPLAY_GROUP_GENRE;
	gNewObjectDefinition.group 		= 0;
	gNewObjectDefinition.type		= 0;
	gNewObjectDefinition.scale 		= PLAYER_BALL_SCALE;
	gNewObjectDefinition.coord		= gMyCoord;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= PLAYER_SLOT;
	gNewObjectDefinition.moveCall 	= MovePlayer_Ball;
	gNewObjectDefinition.rot 		= rotY;
	newObj							= MakeNewObject(&gNewObjectDefinition);
	
			/* MAKE BASE GROUP */
	
	CreateBaseGroup(newObj);											// create group object

	
				/***********************************/
				/* ADD SKELETON TRIMESHES TO GROUP */
				/***********************************/

	UpdateSkinnedGeometry(oldObj);
	
	numTriMeshes = oldObj->Skeleton->skeletonDefinition->numDecomposedTriMeshes;
	for (i = 0; i < numTriMeshes; i++)
	{
		TQ3TriMeshData		*data;
		TQ3GeometryObject	triMeshObj;
		u_long				p;
		float				xoff,yoff,zoff;
		
					/* OFFSET THE GEOMETRY */
					//
					// Remember, geometry is in world-space, so convert to local space.
					//
		
		xoff = -oldObj->Coord.x;
		yoff = -oldObj->Coord.y - PLAYER_BALL_FOOTOFFSET;
		zoff = -oldObj->Coord.z;
		
		data = &gLocalTriMeshesOfSkelType[SKELETON_TYPE_ME][i];
		for (p = 0; p < data->numPoints; p++)
		{
			data->points[p].x += xoff;
			data->points[p].y += yoff;
			data->points[p].z += zoff;
		}		
		Q3BoundingBox_SetFromPoints3D(&data->bBox, data->points, data->numPoints, sizeof(TQ3Point3D));	// recalc bbox
		
		
					/* MAKE OBJ & PUT INTO GROUP */
					
		triMeshObj = Q3TriMesh_New(data);										// convert data into trimesh
		if (triMeshObj == nil)
			DoFatalAlert("\pInitPlayer_Ball: Q3TriMesh_New failed!");
		AttachGeometryToDisplayGroupObject(newObj, triMeshObj);					// add to display group
		Q3Object_Dispose(triMeshObj);											// nuke extra ref
	}			

	
				/**********************/
				/* SET COLLISION INFO */
				/**********************/
	
	newObj->BoundingSphere.radius	=	PLAYER_RADIUS;			
	
	newObj->CType = CTYPE_PLAYER;
	newObj->CBits = CBITS_TOUCHABLE;
	
		/* note: box must be same for both bug & ball to avoid collison fallthru against solids */

	SetObjectCollisionBounds(newObj,PLAYER_BALL_HEADOFFSET, -PLAYER_BALL_FOOTOFFSET,
							-42, 42, 42, -42);


				/* TRANSPLANT THE SHADOW */
				
	newObj->ShadowNode = oldObj->ShadowNode;				// use existing shadow
	oldObj->ShadowNode = nil;								// detach from existing
	UpdateShadow(newObj);

			/*******************************************/
			/* COPY SOME STUFF FROM OLD OBJ & NUKE OLD */
			/*******************************************/
			
	if (oldObj)
	{
		newObj->Damage 			= oldObj->Damage;
		newObj->InvincibleTimer = oldObj->InvincibleTimer;
		newObj->Delta 			= oldObj->Delta;
		newObj->Rot.x = newObj->Rot.z = 0;
		
				/* COPY OLD COLLISION BOX */

		newObj->OldCoord 		= oldObj->OldCoord;
				
		newObj->CollisionBoxes[0].oldLeft	=	oldObj->CollisionBoxes[0].oldLeft;
		newObj->CollisionBoxes[0].oldRight	=	oldObj->CollisionBoxes[0].oldRight;
		newObj->CollisionBoxes[0].oldTop	=	oldObj->CollisionBoxes[0].oldTop;
		newObj->CollisionBoxes[0].oldBottom	=	oldObj->CollisionBoxes[0].oldBottom;
		newObj->CollisionBoxes[0].oldFront	=	oldObj->CollisionBoxes[0].oldFront;
		newObj->CollisionBoxes[0].oldBack	=	oldObj->CollisionBoxes[0].oldBack;
		
		DeleteObject(oldObj);
		oldObj = nil;
	}	
	
	
	
				/* SET GLOBALS */
		
	gPlayerObj 		= newObj;		
	gPlayerMode 	= PLAYER_MODE_BALL;
	gPlayerMaxSpeed = PLAYER_MAX_SPEED_BALL; 
	gNitroTimer		= 0;
	gNitroParticleGroup = -1;

	gInfobarUpdateBits |= UPDATE_HANDS;	
}




/******************** MOVE ME: BALL ***********************/

static void MovePlayer_Ball(ObjNode *theNode)
{
	gPlayerCanMove = true;
	
	GetObjectInfo(theNode);

		
			/* UPDATE INVINCIBILITY */
			
	if (theNode->InvincibleTimer > 0.0f)
	{
		theNode->InvincibleTimer -= gFramesPerSecondFrac;
		if (theNode->InvincibleTimer < 0.0f)
			theNode->InvincibleTimer = 0;
	}
		

			/* DO CONTROL */

	DoPlayerControl_Ball(theNode,1.0);


			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BALL_FRICTION_ACCEL);
	if (DoPlayerMovementAndCollision(false))
		goto update;


		/* SEE IF LEAVE NITRO TRAIL */
		
	if (gNitroTimer > 0.0f)
	{
		LeaveNitroTrail();
		gNitroTimer -= gFramesPerSecondFrac;
		if (gNitroTimer <= 0.0f)
			gNitroParticleGroup = -1;
	}
		
			/* UPDATE IT */
			
update:			
	UpdatePlayer_Ball(gPlayerObj);
}


/************************ UPDATE PLAYER: BALL ***************************/

static void UpdatePlayer_Ball(ObjNode *theNode)
{
	SpinBall(theNode);

	gMyCoord = gCoord;
	UpdateObject(theNode);	
	
	ProcessBallTimer();	
	
		/* FINAL CHECK: SEE IF NEED TO KNOCK ON BUTT */
		
	if (gPlayerKnockOnButt)
	{
		gPlayerKnockOnButt = false;
		KnockPlayerBugOnButt(&gPlayerKnockOnButtDelta, true, true);
	}	
	
		/* UPDATE SHIELD */
		
	UpdatePlayerShield();
}


/**************** DO PLAYER CONTROL: BALL ***************/
//
// Moves a player based on its control bit settings.
// These settings are already set either by keyboard interpretation or reading off the network.
//
// INPUT:	theNode = the node of the player
//			slugFactor = how much of acceleration to apply (varies if jumping et.al)
//


static void DoPlayerControl_Ball(ObjNode *theNode, float slugFactor)
{
float	mouseDX, mouseDY;
float	dx, dy;

			/********************/
			/* GET MOUSE DELTAS */
			/********************/
			//
			// NOTE: can only call this once per frame since
			//		this resets the mouse coord.
			//
			
	GetMouseDelta(&dx, &dy);

			
	if (gPlayerUsingKeyControl && gGamePrefs.playerRelativeKeys)
	{
		gPlayerObj->AccelVector.y = 
		gPlayerObj->AccelVector.x = 0;	
	}
	else
	{
		mouseDX = dx * .05f;
		mouseDY = dy * .05f;
		theNode->AccelVector.y = mouseDY * PLAYER_BALL_ACCEL * slugFactor;
		theNode->AccelVector.x = mouseDX * PLAYER_BALL_ACCEL * slugFactor;	
	}
	
		/* SEE IF DO NITRO */
		
	if (gNitroTimer <= 0.0f)
	{
		if (GetNewKeyState(KEY_OPTION))
		{
			theNode->Speed = gPlayerMaxSpeed;		// boost to full speed	
			gDelta.z *= 100.0f;						// boost this up really high (will get tweaked later)
			gDelta.x *= 100.0f;
			PlayEffect3D(EFFECT_SPEEDBOOST, &gCoord);
			StartNitroTrail();			
			gBallTimer -= .05f;						// lose a bit more ball time
		}
	}
}


/******************** SPIN BALL ************************/

static void SpinBall(ObjNode *theNode)
{
float	d,fps = gFramesPerSecondFrac;

			/* REGULATE SPIN IF ON GROUND */
			
//	if (gMyDistToFloor < 4.0f)
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		d = theNode->RotDeltaX = -theNode->Speed * .01f;
	}
	
			/* OTHERWISE DECAY SPIN IN AIR */
	else
	{
		if (theNode->RotDeltaX < 0.0f)
		{
			theNode->RotDeltaX += fps*12.0f;
			if (theNode->RotDeltaX > 0.0f)
				theNode->RotDeltaX = 0;
		}
		
		d = theNode->RotDeltaX;
	}
	
	theNode->Rot.x += d * fps;					// rotate on x

			/* AIM IN DIRECTION OF MOTION */
				
	if ((!gPlayerUsingKeyControl) || (!gGamePrefs.playerRelativeKeys))
		TurnObjectTowardTarget(theNode, &gCoord, gCoord.x + gDelta.x, gCoord.z + gDelta.z, 8.0, false);			

}


#pragma mark -

/****************** START NITRO TRAIL *********************/

static void StartNitroTrail(void)
{
	gNitroTimer = .6;
	gNitroTrailTick = 0;	
}



/***************** LEAVE NITRO TRAIL *********************/

static void LeaveNitroTrail(void)
{
int				i;
TQ3Matrix4x4	m;
static const TQ3Vector3D up = {0,1,0};

	gNitroTrailTick += gFramesPerSecondFrac;
	if (gNitroTrailTick < .05f)
		return;
	
	gNitroTrailTick = 0;

			/* CALC MATRIX TO AIM & PLACE A NITRO RING */
			
	SetLookAtMatrixAndTranslate(&m, &up, &gPlayerObj->OldCoord, &gCoord);


				/* MAKE PARTICLE GROUP */
				
	if (gNitroParticleGroup == -1)
	{
new_pgroup:	
		gNitroParticleGroup = NewParticleGroup(0,							// magic num
												PARTICLE_TYPE_FALLINGSPARKS,	// type
												0,		// flags
												600,							// gravity
												10000,						// magnetism
												25,							// base scale
												-1.9,						// decay rate
												1.0,							// fade rate
												PARTICLE_TEXTURE_GREENRING);	// texture
	}

	if (gNitroParticleGroup == -1)
		return;


			/* ADD PARTICLES TO GROUP */

	for (i = 0; i < NITRO_RING_SIZE; i++)
	{
		TQ3Vector3D	delta;
		TQ3Point3D	pt;
		float		a;
		
		
		a = PI2 * ((float)i * (1.0f/(float)NITRO_RING_SIZE));		
		pt.x = sin(a) * 50.0f;
		pt.y = cos(a) * 50.0f;
		pt.z = 0;
	
		Q3Point3D_Transform(&pt, &m, &pt);
	
		delta.x = (RandomFloat()-.5f) * 200.0f;
		delta.y = 200.0f + (RandomFloat()-.5f) * 200.0f;
		delta.z = (RandomFloat()-.5f) * 200.0f;
	
		if (AddParticleToGroup(gNitroParticleGroup, &pt, &delta, 1.0 + RandomFloat(), FULL_ALPHA))
			goto new_pgroup;
	}
}







