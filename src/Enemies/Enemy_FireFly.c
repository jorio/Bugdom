/****************************/
/*   ENEMY: FIREFLY.C		*/
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode					*gCurrentNode;
extern	SplineDefType			**gSplineList;
extern	TQ3Point3D				gCoord,gMyCoord;
extern	float					gFramesPerSecondFrac;
extern	TQ3Vector3D			gDelta;
extern	ObjNode				*gPlayerObj;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	u_short					gLevelType;
extern	short	  				gNumTerrainItems;
extern	u_long					gAutoFadeStatusBits;
extern	TerrainItemEntryType 	**gMasterItemList;
extern	Byte					gPlayerMode;
extern	signed char	gNumEnemyOfKind[];
extern	CollisionRec	gCollisionList[];

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveFireFly(ObjNode *theNode);
static void UpdateFireFly(ObjNode *theNode);
static void OrbitFireFly(ObjNode *theNode);
static void FireFlyChasePlayer(ObjNode *theNode);
static void FireFlyCarryPlayer(ObjNode *theNode);
static void FindFireFlyTarget(ObjNode *firefly);
static void FireFlyGoAway(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_FIREFLY				4

#define	FIREFLY_SCALE				.6f
#define	FLARE_SCALE					2.2f

#define	FIREFLY_CHASE_RANGE		200.0f

#define FIREFLY_TURN_SPEED			4.0f

#define	FIREFLY_HEALTH				1.0f		
#define	FIREFLY_DAMAGE				0.04f
#define	FIREFLY_FLIGHT_HEIGHT		400.0f

#define	NAB_HEIGHT		300.0f
#define CARRY_HEIGHT	1000.0f


enum
{
	FIREFLY_ANIM_FLY
};

enum
{
	FIREFLY_MODE_ORBIT,
	FIREFLY_MODE_CHASE,
	FIREFLY_MODE_CARRY,
	FIREFLY_MODE_DONE
};


/*********************/
/*    VARIABLES      */
/*********************/

ObjNode	*gCurrentCarryingFireFly = nil;
ObjNode	*gCurrentChasingFireFly = nil;			// only 1 firefly can chase at a time

float	gFireFlyTargetX,gFireFlyTargetZ;

#define	FireFlyTargetID	Special[0]


/************************ ADD FIREFLY *************************/
//
// parm[0] = target ID.
//

Boolean AddFireFly(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj,*glow;

	if (gLevelType != LEVEL_TYPE_NIGHT)
		DoFatalAlert("AddFireFly: not on this level, bud!");

			/************************/
			/* MAKE SKELETON OBJECT */
			/************************/
			//
			// NOTE: we dont call MakeEnemySkeleton because we need this to be *before* player in linked list.
			//
			
	gNewObjectDefinition.type 		= SKELETON_TYPE_FIREFLY;
	gNewObjectDefinition.animNum 	= FIREFLY_ANIM_FLY;							
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR) + FIREFLY_FLIGHT_HEIGHT;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;	
	gNewObjectDefinition.slot 		= PLAYER_SLOT-1;
	gNewObjectDefinition.moveCall 	= MoveFireFly;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= FIREFLY_SCALE;
	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	if (newObj == nil)
		DoFatalAlert("AddFireFly: MakeNewSkeletonObject failed!");

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list
	
	newObj->Kind 	= ENEMY_KIND_FIREFLY;				
	newObj->Mode 	= FIREFLY_MODE_ORBIT;	
	
			/* SET COLLISION INFO */
			
	newObj->CType 	= CTYPE_ENEMY;
	newObj->CBits 	= CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj,100,-100,-100,100,100,-100);	// ... but we need the box for its own collision
	

				/* OFFSET & GET ORBITING */
				
	newObj->Coord.x += (RandomFloat()-.5f) * 400.0f;
	newObj->Coord.y += (RandomFloat()-.5f) * 300.0f;
	newObj->Coord.z += (RandomFloat()-.5f) * 400.0f;

	newObj->Delta.x = (RandomFloat()-.5f) * 500.0f;
	newObj->Delta.y = 0; 
	newObj->Delta.z = (RandomFloat()-.5f) * 500.0f;
		
	newObj->FireFlyTargetID = itemPtr->parm[0];						// get target ID #
	
				/* MAKE GLOW SHADOW */
				
	AttachGlowShadowToObject(newObj, 11, 11, true);
	

				/*************/
				/* MAKE GLOW */
				/*************/	
			
	gNewObjectDefinition.group 		= NIGHT_MGroupNum_FireFlyGlow;	
	gNewObjectDefinition.type 		= NIGHT_MObjType_FireFlyGlow;
	gNewObjectDefinition.coord		= newObj->Coord;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER|STATUS_BIT_GLOW|STATUS_BIT_NOZWRITE|STATUS_BIT_NOFOG;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB+30;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.scale 		= FLARE_SCALE;
	glow = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	MakeObjectKeepBackfaces(glow);

	newObj->ChainNode = glow;
	
//	gNumEnemies++;			// NOTE: FIREFLIES DONT COUNT NORMALLY LIKE OTHER ENEMIES!!!!
	gNumEnemyOfKind[ENEMY_KIND_FIREFLY]++;
	
	return(true);
}



/********************* MOVE FIREFLY **************************/

static void MoveFireFly(ObjNode *theNode)
{

			/* SEE IF GONE */
			
	if (TrackTerrainItem(theNode))						
	{
		if (theNode == gCurrentChasingFireFly)				// see if this was a chaser
			gCurrentChasingFireFly = nil;
			
		DeleteEnemy(theNode);
		return;
	}

	switch(theNode->Mode)
	{
		case	FIREFLY_MODE_ORBIT:
				OrbitFireFly(theNode);
				break;
				
		case	FIREFLY_MODE_CHASE:
				FireFlyChasePlayer(theNode);
				break;
	
		case	FIREFLY_MODE_CARRY:
				FireFlyCarryPlayer(theNode);
				break;
		
		case	FIREFLY_MODE_DONE:
				FireFlyGoAway(theNode);
				break;
	}
}


/***************** ORBIT FIREFLY ****************/

static void OrbitFireFly(ObjNode *theNode)
{
float		fps = gFramesPerSecondFrac;
TQ3Vector3D	grav;
float		d;

	GetObjectInfo(theNode);
	
	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, FIREFLY_TURN_SPEED, false);			
	
	
		/* DO ORBITAL DYNAMICS */
		
	grav.x = theNode->InitCoord.x - gCoord.x;			// calc vector to center
	grav.y = theNode->InitCoord.y - gCoord.y;
	grav.z = theNode->InitCoord.z - gCoord.z;	
	FastNormalizeVector(grav.x, grav.y, grav.z ,&grav);
	
	d =  Q3Point3D_Distance(&gCoord, &theNode->InitCoord);
	if (d != 0.0f)
	{
		if (d < 40.0f)
			d = 40.0f;
		d = 1.0f / (d);		// calc 1/distance to center
	}
	else
		d = 40;
	
	d = (fps * d * 200000.0f);
		
	gDelta.x += grav.x * d;
	gDelta.y += grav.y * d;
	gDelta.z += grav.z * d;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;
	
	
				/****************/
				/* DO COLLISION */
				/****************/
				
	HandleCollisions(theNode,CTYPE_MISC);

		/* SEE IF CLOSE ENOUGH TO ATTACK */
		
	if (gCurrentChasingFireFly == nil)
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < FIREFLY_CHASE_RANGE)
		{
			theNode->Mode = FIREFLY_MODE_CHASE;	
			gCurrentChasingFireFly = theNode;
		}
	}

	UpdateFireFly(theNode);
}


/***************** FIREFLY CHASE PLAYER ****************/

static void FireFlyChasePlayer(ObjNode *theNode)
{
float		fps = gFramesPerSecondFrac;
float		d,y;

	GetObjectInfo(theNode);
	
	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, FIREFLY_TURN_SPEED, false);			
	
			/* MOVE TOWARD PLAYER */
			
	d =  Q3Point3D_Distance(&gCoord, &gMyCoord) * fps * .01f;
	
	if (gCoord.x < gMyCoord.x)
	{
		gDelta.x += (gMyCoord.x - gCoord.x) * d;
		gCoord.x += gDelta.x * fps;
		if (gCoord.x > gMyCoord.x)
		{
			gCoord.x = gMyCoord.x;
			gDelta.x = 0;		
		}
	}
	else
	{
		gDelta.x += (gMyCoord.x - gCoord.x) * d;
		gCoord.x += gDelta.x * fps;
		if (gCoord.x < gMyCoord.x)
		{
			gCoord.x = gMyCoord.x;
			gDelta.x = 0;		
		}
	}
	
	if (gCoord.z < gMyCoord.z)
	{
		gDelta.z += (gMyCoord.z - gCoord.z) * d;
		gCoord.z += gDelta.z * fps;
		if (gCoord.z > gMyCoord.z)
		{
			gCoord.z = gMyCoord.z;
			gDelta.z = 0;		
		}
	}
	else
	{
		gDelta.z += (gMyCoord.z - gCoord.z) * d;
		gCoord.z += gDelta.z * fps;
		if (gCoord.z < gMyCoord.z)
		{
			gCoord.z = gMyCoord.z;
			gDelta.z = 0;		
		}
	}

	y = gMyCoord.y + NAB_HEIGHT;
	if (gCoord.y < y)
	{
		gDelta.y += (y - gCoord.y) * d;
		gCoord.y += gDelta.y * fps;
		if (gCoord.y > y)
		{
			gCoord.y = y;
			gDelta.y = 0;		
		}
	}
	else
	{
		gDelta.y += (y - gCoord.y) * d;
		gCoord.y += gDelta.y * fps;
		if (gCoord.y < y)
		{
			gCoord.y = y;
			gDelta.y = 0;		
		}
	}
	
	
				/****************/
				/* DO COLLISION */
				/****************/
				
	HandleCollisions(theNode,CTYPE_MISC);
	
	
				/*********************/
				/* SEE IF NAB PLAYER */
				/*********************/

		/* PLAYER MUST BE IN A GOOD ANIM */
		
	if (gPlayerMode == PLAYER_MODE_BUG)
	{
		short	anim = gPlayerObj->Skeleton->AnimNum;
		
		if ((anim == PLAYER_ANIM_DEATH) || (anim ==	PLAYER_ANIM_CARRIED) ||		// dont nab during death or other things
			(anim == PLAYER_ANIM_FALLONBUTT))
		{
			goto update;
		}
	}

		/* CHECK OTHER CONDITIONS */
		
	if (gCurrentCarryingFireFly == nil)
	{
		if ((gCoord.y < (gMyCoord.y + NAB_HEIGHT+20)) && (gCoord.y > gMyCoord.y))				// see if in correct y range
		{
			if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < 40.0f)	// see if in good x/z range
			{
				theNode->Mode = FIREFLY_MODE_CARRY;
				
					/* SET PLAYER ANIM */
					
				if (gPlayerMode == PLAYER_MODE_BALL)				
					InitPlayer_Bug(gPlayerObj, &gPlayerObj->Coord, gPlayerObj->Rot.y, PLAYER_ANIM_CARRIED);
				else												
					MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_CARRIED, 7);	
					
				gCurrentCarryingFireFly = theNode;
				FindFireFlyTarget(theNode);				
			}
		}
	}

update:
	UpdateFireFly(theNode);
}


/***************** FIREFLY GO AWAY ****************/

static void FireFlyGoAway(ObjNode *theNode)
{
float		fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);
		
	gDelta.y += 800.0f * fps;
		
	gCoord.x += gDelta.x * fps;						// move it
	gCoord.y += gDelta.y * fps;						
	gCoord.z += gDelta.z * fps;						
	
	UpdateFireFly(theNode);
}	


/***************** FIREFLY CARRY PLAYER ****************/

static void FireFlyCarryPlayer(ObjNode *theNode)
{
float		fps = gFramesPerSecondFrac;
float		r,speed,y;

	GetObjectInfo(theNode);
	
			/* SEE IF SHOULD DROP OFF PLAYER NOW */
			
	if (CalcQuickDistance(gCoord.x, gCoord.z, gFireFlyTargetX, gFireFlyTargetZ) < 200.0f)
	{
		gCurrentCarryingFireFly = nil;				// not chasing or carrying anymore
		gCurrentChasingFireFly = nil;
		theNode->Mode = FIREFLY_MODE_DONE;	
	}	
	

			/* AIM AND MOVE TO FINAL TARGET */
				
	TurnObjectTowardTarget(theNode, &gCoord, gFireFlyTargetX, gFireFlyTargetZ, FIREFLY_TURN_SPEED*1.5, false);			

	r = theNode->Rot.y;	
	
	theNode->Speed += fps * 500.0f;					// accelerate
	if (theNode->Speed > 1000.0f)
		theNode->Speed = 1000.0f;
		
	speed = theNode->Speed;
	
	gDelta.x = -sin(r) * speed;						// calc deltas
	gDelta.z = -cos(r) * speed;


			/* CHECK Y */
			
	y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR);		// get ground y here
	if (gCoord.y < (y + CARRY_HEIGHT))							// see if below hover height
	{
		if (gCoord.y < (y + 250.0f))							// see if scraping ground
		{
			gCoord.y = y + 250.0f;
			gDelta.y = 0;
		}
		else
			gDelta.y += 2000.0f * fps;
	}
	else											// above hover height
	{
		if (gDelta.y > 1.0f)
			gDelta.y *= .5f;						// slow it down
		else
			gDelta.y -= 1500.0f * fps;
	}
	
	
			/* MOVE */
			
	gCoord.x += gDelta.x * fps;						// move it
	gCoord.y += gDelta.y * fps;						
	gCoord.z += gDelta.z * fps;						
	
	
		/* KEEP ABOVE LIQUID */
		
	{
		TQ3Point3D	p;
		
		p.x = gCoord.x;
		p.y = gCoord.y - 100.0f;
		p.z = gCoord.z;
		
		if (DoSimplePointCollision(&p, CTYPE_LIQUID))
		{
			gCoord.y = gCollisionList[0].objectPtr->CollisionBoxes[0].top + 100.0f;
	
		}
	}
	
	UpdateFireFly(theNode);
	
}	





#pragma mark -


/***************** UPDATE FIREFLY **********************/

static void UpdateFireFly(ObjNode *theNode)
{
ObjNode		*glow;

	UpdateEnemy(theNode);	
	
	
			/* UPDATE THE GLOW */
			
	glow = theNode->ChainNode;
	if (glow)
	{
		static const TQ3Vector3D up = {0,1,0};
		TQ3Matrix4x4	m,m2;
		float			s;
		
		s = FLARE_SCALE + RandomFloat()*.3f;							// random scale flutter
		
		SetLookAtMatrixAndTranslate(&m, &up, &gCoord,  &gGameViewInfoPtr->currentCameraCoords);
		Q3Matrix4x4_SetScale(&m2, s, s, s);
		MatrixMultiplyFast(&m2,&m, &glow->BaseTransformMatrix);
		SetObjectTransformMatrix(glow);

		glow->Coord = theNode->Coord;									// update true coord for culling
	}				


		/* UPDATE BUZZ */
		
	if (theNode->EffectChannel == -1)
		theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_BUZZ, &theNode->Coord, kMiddleC+3, .3);
	else
		Update3DSoundChannel(EFFECT_BUZZ, &theNode->EffectChannel, &theNode->Coord);

}





/******************** FIND FIREFLY TARGET *******************/
//
// Scans thru item list.
//

static void FindFireFlyTarget(ObjNode *firefly)
{
long					i,id;
TerrainItemEntryType	*itemPtr;

	id = firefly->FireFlyTargetID;

	itemPtr = *gMasterItemList; 											// get pointer to data inside the LOCKED handle

				/* SCAN FOR FIREFLY TARGET ITEM */

	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_FIREFLYTARGET)						// see if it's the right item
		{
			if (itemPtr[i].parm[0] == id)									// see if ID's match
			{
				gFireFlyTargetX = itemPtr[i].x * MAP2UNIT_VALUE;			// get target coord & convert to world coords
				gFireFlyTargetZ = itemPtr[i].y * MAP2UNIT_VALUE;
				return;
			}
		}
	}
	
	
		/* OOPS!  NO TARGET IS FOUND FOR THIS, SO LETS GO TO ANY TARGET WE CAN FIND */
			
	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_FIREFLYTARGET)					// see if it's the right item
		{
			gFireFlyTargetX = itemPtr[i].x * MAP2UNIT_VALUE;			// get target coord & convert to world coords
			gFireFlyTargetZ = itemPtr[i].y * MAP2UNIT_VALUE;
			return;
		}
	}
			
	DoFatalAlert("FindFireFlyTarget: no targets found!");
}



/******************* KILL FIREFLY **********************/

Boolean KillFireFly(ObjNode *theNode)
{
	if (theNode == gCurrentChasingFireFly)				// see if this was a chaser
		gCurrentChasingFireFly = nil;

	if (theNode == gCurrentCarryingFireFly)				// see if I am being carried by this guy
		gCurrentCarryingFireFly = nil;

	DeleteEnemy(theNode);
	return(true);
}

