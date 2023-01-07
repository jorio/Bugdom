/****************************/
/*   	TRAPS.C			    */
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

static void MoveFootOnSpline(ObjNode *theNode);
static void MoveBat(ObjNode *theNode);
static Boolean SeeIfBatEatsPlayer(ObjNode *bat);
static void MoveFireWall(ObjNode *theNode);
static void MoveShockwave(ObjNode *theNode);
static void MoveRollingBoulder(ObjNode *theNode);
static void MoveFloorSpike(ObjNode *theNode);
static void CalcFootCollisionBoxes(ObjNode *theNode);



/****************************/
/*    CONSTANTS             */
/****************************/


		/* FOOT STUFF */
		
#define	FOOT_SPLINE_SPEED		1.0f
#define	FOOT_SCALE				10.0f

enum
{
	FOOT_ANIM_FLAT,
	FOOT_ANIM_UP,
	FOOT_ANIM_DOWN
};

enum
{
	FOOT_MODE_GOUP,
	FOOT_MODE_GODOWN,
	FOOT_MODE_DOWN
};

		/* BAT STUFF */
		
#define	BAT_SCALE	8.0f

enum
{
	BAT_ANIM_FLYUP,
	BAT_ANIM_DIVE
};


	/* BOULDER */
	
#define	BOULDER_RADIUS		35.0f
#define	BOULDER_SCALE		3.0f
#define	BOULDER_DIST		1200.0f


#define	THORN_SCALE			2.2f


		/* SPIKE */
		
#define	SPIKE_HEIGHT		200.0f
#define	SPIKE_DIST			150.0f

enum
{
	SPIKE_MODE_WAIT,
	SPIKE_MODE_GOUP,
	SPIKE_MODE_GODOWN
};


/*********************/
/*    VARIABLES      */
/*********************/

Boolean		gBatExists;
const TQ3Point3D gBatMouthOff = {0,-8,-20};
ObjNode	*gCurrentEatingBat;

#define	GroundTimer		SpecialF[0]

#define	GotPlayer		Flag[1]

#define	PTimer			SpecialF[0]
#define	PGroupMagic		SpecialL[1]
#define	WallRot			Flag[2]
#define WallLength		SpecialL[0]

#define ValveID			SpecialL[2]
#define	ExtinguishTimer SpecialF[1]


#define	BoulderIsActive	Flag[0]


/************************ PRIME FOOT *************************/

Boolean PrimeFoot(long splineNum, SplineItemType *itemPtr)
{
ObjNode	*newObj;
float	x,z,placement;
	
			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);

			/* MAKE SKELETON OBJNODE */
			
	gNewObjectDefinition.type 		= SKELETON_TYPE_FOOT;
	gNewObjectDefinition.animNum 	= 0;							
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR)+(MyRandomLong()&0x3ff);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 80;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= FOOT_SCALE;
	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);
				
	DetachObject(newObj);									// detach this object from the linked list
		
	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;
		

				/* SET BETTER INFO */
			
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveFootOnSpline;				// set move call

	
			/* SET COLLISION INFO */
			
	newObj->Damage 	= .3;			
	newObj->CType 	= CTYPE_MISC|CTYPE_HURTME|CTYPE_BLOCKCAMERA;
	newObj->CBits 	= CBITS_TOUCHABLE;
	
	AllocateCollisionBoxMemory(newObj, 3);							// alloc 3 collision boxes
	CalcFootCollisionBoxes(newObj);
	

	newObj->Mode = MyRandomLong()&1;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	AddToSplineObjectList(newObj);

	return(true);
}

/********************* CALC FOOT COLLISION BOXES **********************/

static void CalcFootCollisionBoxes(ObjNode *theNode)
{
int				i,j;
TQ3Matrix4x4	m;
CollisionBoxType *boxPtr;
static const TQ3Point3D	pts[12] =
{
	{ -20*FOOT_SCALE,0,-67*FOOT_SCALE }, { 26*FOOT_SCALE,0,-67*FOOT_SCALE },		// toes
	{ -20*FOOT_SCALE,0,-24*FOOT_SCALE }, { 26*FOOT_SCALE,0,-24*FOOT_SCALE },

	{ -20*FOOT_SCALE,0,-24*FOOT_SCALE }, { 26*FOOT_SCALE,0,-24*FOOT_SCALE },		// mid
	{ -20*FOOT_SCALE,0,  6*FOOT_SCALE }, { 26*FOOT_SCALE,0,  6*FOOT_SCALE },

	{ -15*FOOT_SCALE,0,  6*FOOT_SCALE }, { 18*FOOT_SCALE,0,  6*FOOT_SCALE },		// heel
	{ -15*FOOT_SCALE,0, 49*FOOT_SCALE }, { 18*FOOT_SCALE,0, 49*FOOT_SCALE },
};

TQ3Point3D	pts2[12];
float		l,r,f,b;

	theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,
												theNode->Coord.x, theNode->Coord.z);

		/* TRANSFORM BOX POINTS */
		
	Q3Matrix4x4_SetRotate_Y(&m, theNode->Rot.y);		
	m.value[3][0] = theNode->Coord.x;
	m.value[3][1] = theNode->Coord.y;
	m.value[3][2] = theNode->Coord.z;
	
	Q3Point3D_To3DTransformArray(&pts[0], &m, &pts2[0], 12);//, sizeof(TQ3Point3D), sizeof(TQ3Point3D));


		/***************/
		/* SET BBOX #0 */
		/***************/
		
	boxPtr = theNode->CollisionBoxes;

	for (j = i = 0; i < 3; i++, j+=4)
	{
			/* DETERMINE L/R/F/B */

		l = r = pts2[j].x;
		if (pts2[j+1].x < l)
			l = pts2[j+1].x;
		if (pts2[j+1].x > r)
			r = pts2[j+1].x;
		if (pts2[j+2].x < l)
			l = pts2[j+2].x;
		if (pts2[j+2].x > r)
			r = pts2[j+2].x;
		if (pts2[j+3].x < l)
			l = pts2[j+3].x;
		if (pts2[j+3].x > r)
			r = pts2[j+3].x;

		f = b = pts2[j+0].z;
		if (pts2[j+1].z < b)
			b = pts2[j+1].z;
		if (pts2[j+1].z > f)
			f = pts2[j+1].z;
		if (pts2[j+2].z < b)
			b = pts2[j+2].z;
		if (pts2[j+2].z > f)
			f = pts2[j+2].z;
		if (pts2[j+3].z < b)
			b = pts2[j+3].z;
		if (pts2[j+3].z > f)
			f = pts2[j+3].z;


			/* SET POINTS */
			
		if (i == 2)
			boxPtr[i].top = theNode->Coord.y + 3000.0f;
		else
			boxPtr[i].top = theNode->Coord.y + 500.0f;
		
		boxPtr[i].bottom = theNode->Coord.y;
		boxPtr[i].left = l;
		boxPtr[i].right = r;
		boxPtr[i].front = f;
		boxPtr[i].back = b;
	}

}


/*********************** MOVE FOOT ON SPLINE*****************************/

static void MoveFootOnSpline(ObjNode *theNode)
{	
Boolean isVisible; 
float	fps = gFramesPerSecondFrac;
float	y;
Byte	mode = theNode->Mode;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

	switch(mode)
	{
				/********************/
				/* FOOT IS GOING UP */
				/********************/
				
		case	FOOT_MODE_GOUP:
		
					/* MOVE ALONG THE SPLINE */

				IncreaseSplineIndex(theNode, theNode->Speed);

				GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);
				y = GetTerrainHeightAtCoord(theNode->Coord.x, theNode->Coord.z, FLOOR);		
				theNode->Speed = (theNode->Coord.y - y) * FOOT_SPLINE_SPEED;	// speed is based on height off ground


					/* MOVE UP */
					
				y += 500.0f;									// offset for max y
				theNode->Delta.y += 100.0f * fps;				// gravity
				theNode->Coord.y += theNode->Delta.y;			// move down
				if (theNode->Coord.y >= y)						// see if hit ground
				{
					theNode->Coord.y = y;
					theNode->Mode = FOOT_MODE_GODOWN;
					SetSkeletonAnim(theNode->Skeleton, FOOT_ANIM_DOWN);
				}				
		
				break;


				/**********************/
				/* FOOT IS GOING DOWN */
				/**********************/
				
		case	FOOT_MODE_GODOWN:
		
					/* MOVE ALONG THE SPLINE */

				IncreaseSplineIndex(theNode, theNode->Speed);

				GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);
				y = GetTerrainHeightAtCoord(theNode->Coord.x, theNode->Coord.z, FLOOR);		
				theNode->Speed = (theNode->Coord.y - y) * FOOT_SPLINE_SPEED;	// speed is based on height off ground
				
				
					/* MOVE DOWN */
				
				theNode->Delta.y -= 100.0f * fps;				// gravity
				theNode->Coord.y += theNode->Delta.y;			// move down
				if (theNode->Coord.y <= y)						// see if hit ground
				{
					theNode->Speed = 0;							// reset speed to avoid hiccup when going back up
					theNode->Coord.y = y;
					theNode->Mode = FOOT_MODE_DOWN;
					theNode->GroundTimer = .5f;
					MorphToSkeletonAnim(theNode->Skeleton, FOOT_ANIM_FLAT, 6);
					PlayEffect3D(EFFECT_FOOTSTEP, &theNode->Coord);

				}				
				break;

				/*********************/
				/* FOOT IS ON GROUND */
				/*********************/
				
		case	FOOT_MODE_DOWN:
				theNode->Speed = 0;								// reset speed to avoid hiccup when going back up
				theNode->Delta.y = 0;
				theNode->GroundTimer -= fps;							// see if time to go back up
				if (theNode->GroundTimer <= 0.0f)
				{
					theNode->Mode = FOOT_MODE_GOUP;
					SetSkeletonAnim(theNode->Skeleton, FOOT_ANIM_UP);					
				}
				break;
		
	}



			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/
			
	if (isVisible)
	{
		if (mode != FOOT_MODE_DOWN)
		{
			theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,	// calc y rot aim
													theNode->Coord.x, theNode->Coord.z);
		}

		UpdateObjectTransforms(theNode);							// update transforms
		CalcFootCollisionBoxes(theNode);
	}
	
			/* NOT VISIBLE */
	else
	{
//		if (theNode->ShadowNode)									// hide shadow
//			theNode->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;	
	}
}


#pragma mark -

/************************ MAKE BAT *************************/

void MakeBat(float x, float y, float z)
{
ObjNode	*newObj;

	if (gBatExists)										// only 1 bat at a time
		return;

			/************************/
			/* MAKE SKELETON OBJECT */
			/************************/
				
	gNewObjectDefinition.type 		= SKELETON_TYPE_BAT;
	gNewObjectDefinition.animNum 	= BAT_ANIM_DIVE;							
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= y + 1400.0f;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG;	
	gNewObjectDefinition.slot 		= PLAYER_SLOT-1;
	gNewObjectDefinition.moveCall 	= MoveBat;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= BAT_SCALE;
	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	GAME_ASSERT(newObj);
				

	newObj->CType		= CTYPE_BLOCKCAMERA;					// no collision
	newObj->CBits		= 0;

	SetObjectCollisionBounds(newObj,100,-100,-200,200,200,-200);

	gBatExists = true;
}


/********************* MOVE BAT ************************/

static void MoveBat(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* SEE IF GONE */
			
	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
deleteit:
		DeleteObject(theNode);
		gBatExists = false;
		return;
	}

	GetObjectInfo(theNode);
	
	switch(theNode->Skeleton->AnimNum)
	{
		case	BAT_ANIM_DIVE:
				if (!SeeIfBatEatsPlayer(theNode))
				{
					gDelta.y = -2000.0f;
					gCoord.y += gDelta.y * fps;
					gCoord.x = gMyCoord.x;
					gCoord.z = gMyCoord.z;	
					
					if (gCoord.y < GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR))
						goto deleteit;												
				}
				break;

		case	BAT_ANIM_FLYUP:
				gDelta.y += 2500.0f * fps;
				if (gDelta.y > 0.0f)
					gDelta.y = 0;
				theNode->Rot.y -= fps * .9f;
				gDelta.x = sin(theNode->Rot.y) * -1500.0f;
				gDelta.z = cos(theNode->Rot.y) * -1500.0f;
				gCoord.x += gDelta.x * fps;
				gCoord.y += gDelta.y * fps;
				gCoord.z += gDelta.z * fps;
				
				if (theNode->GotPlayer == false)
				{
					if (theNode->StatusBits & STATUS_BIT_ISCULLED)
						goto deleteit;				
				}
				break;
	}
	
	UpdateObject(theNode);
}


/********************** SEE IF BAT EATS PLAYER *************************/
//
// Does simply collision check to see if bat should eat the player.
//

static Boolean SeeIfBatEatsPlayer(ObjNode *bat)
{
TQ3Point3D	pt;

	if (gPlayerMode == PLAYER_MODE_BALL)					// can only eat the bug, not the ball
		return(false);

	
			/* GET COORD OF MOUTH */
			
	FindCoordOnJoint(bat, BAT_JOINT_HEAD, &gBatMouthOff, &pt);

	if (pt.y <= (gMyCoord.y + 30.0f))
	{
		bat->GotPlayer = true;
		gCurrentEatingBat = bat;
		
		MorphToSkeletonAnim(bat->Skeleton, BAT_ANIM_FLYUP, 2);
		
		if (gPlayerObj->Skeleton->AnimNum == PLAYER_ANIM_RIDEDRAGONFLY)		// get me off dragonfly first
			PlayerOffDragonfly();
			
		MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_BEINGEATEN, 7);
		gPlayerObj->CType = 0;							// no more collision
		KillPlayer(false);	
		return(true);
	}
	
	return(false);
}

#pragma mark -

/************************* ADD THORN *********************************/
//
// parm[0] = type 0..1
// parm[1] = rotation 0..3 (counter clockw)
// parm[3]:bit0 = random rotate

Boolean AddThorn(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	y;
CollisionBoxType *boxPtr;
int		rot;
		
	if (gLevelType != LEVEL_TYPE_FOREST)
		DoFatalAlert("AddThorn: not on this level!");

	if (itemPtr->parm[3] & 1)										// get rotation
		rot = MyRandomLong()&3;
	else
		rot = itemPtr->parm[1];
	

	gNewObjectDefinition.group 		= FOREST_MGroupNum_Thorn;	
	gNewObjectDefinition.type 		= FOREST_MObjType_Thorn1 + itemPtr->parm[0];	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= y = GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 150;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)rot * (PI2/4);
	gNewObjectDefinition.scale 		= THORN_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC|CTYPE_HURTME|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	newObj->Damage = .1;


			/*************/
			/* COLLISION */
			/*************/

	AllocateCollisionBoxMemory(newObj, 3);							// alloc 3 collision boxes
	boxPtr = newObj->CollisionBoxes;								// get ptr to 1st box 
	
			
	if (itemPtr->parm[0])
	{
			/* THORN 2 BOXES */

		boxPtr[0].top 		= y + (350*THORN_SCALE);			// trunk
		boxPtr[0].bottom	= y + (0*THORN_SCALE);
		boxPtr[0].left 		= x + (-20*THORN_SCALE);
		boxPtr[0].right 	= x + (20*THORN_SCALE);
		boxPtr[0].front 	= z + (20*THORN_SCALE);
		boxPtr[0].back 		= z + (-20*THORN_SCALE);

		boxPtr[1].top 		= y + (314*THORN_SCALE);
		boxPtr[1].bottom	= y + (0*THORN_SCALE);
		boxPtr[2].top 		= y + (458*THORN_SCALE);
		boxPtr[2].bottom	= y + (146*THORN_SCALE);
		
	
		switch(rot)										// collision differs on rotation
		{
			case	0:
					boxPtr[1].left 		= x + (-15*THORN_SCALE);
					boxPtr[1].right 	= x + (15*THORN_SCALE);
					boxPtr[1].front 	= z + (215*THORN_SCALE);
					boxPtr[1].back 		= z + (20*THORN_SCALE);

					boxPtr[2].left 		= x + (-15*THORN_SCALE);
					boxPtr[2].right 	= x + (15*THORN_SCALE);
					boxPtr[2].front 	= z + (-20*THORN_SCALE);
					boxPtr[2].back 		= z + (-277*THORN_SCALE);
					break;
					

			case	1:
					boxPtr[1].left 		= x + (20*THORN_SCALE);
					boxPtr[1].right 	= x + (215*THORN_SCALE);
					boxPtr[1].front 	= z + (15*THORN_SCALE);
					boxPtr[1].back 		= z + (-15*THORN_SCALE);

					boxPtr[2].left 		= x + (-277*THORN_SCALE);
					boxPtr[2].right 	= x + (-20*THORN_SCALE);
					boxPtr[2].front 	= z + (15*THORN_SCALE);
					boxPtr[2].back 		= z + (-15*THORN_SCALE);
					break;

			case	2:
					boxPtr[1].left 		= x + (-15*THORN_SCALE);
					boxPtr[1].right 	= x + (15*THORN_SCALE);
					boxPtr[1].front 	= z + (-20*THORN_SCALE);
					boxPtr[1].back 		= z + (-215*THORN_SCALE);

					boxPtr[2].left 		= x + (-15*THORN_SCALE);
					boxPtr[2].right 	= x + (15*THORN_SCALE);
					boxPtr[2].front 	= z + (277*THORN_SCALE);
					boxPtr[2].back 		= z + (20*THORN_SCALE);
					break;

			case	3:
					boxPtr[1].left 		= x + (-215*THORN_SCALE);
					boxPtr[1].right 	= x + (-20*THORN_SCALE);
					boxPtr[1].front 	= z + (15*THORN_SCALE);
					boxPtr[1].back 		= z + (-15*THORN_SCALE);

					boxPtr[2].left 		= x + (20*THORN_SCALE);
					boxPtr[2].right 	= x + (277*THORN_SCALE);
					boxPtr[2].front 	= z + (15*THORN_SCALE);
					boxPtr[2].back 		= z + (-15*THORN_SCALE);
					break;
		}
	}
	else
	{
	
			/* THORN 1 BOXES */
			
		boxPtr[0].top 		= y + (283*THORN_SCALE);
		boxPtr[0].bottom	= y + (0*THORN_SCALE);
		boxPtr[1].top 		= y + (283*THORN_SCALE);
		boxPtr[1].bottom	= y + (155*THORN_SCALE);
		boxPtr[2].top 		= y + (283*THORN_SCALE);
		boxPtr[2].bottom	= y + (0*THORN_SCALE);
	
		switch(rot)										// collision differs on rotation
		{
			case	0:
					boxPtr[0].left 		= x + (-35*THORN_SCALE);
					boxPtr[0].right 	= x + (35*THORN_SCALE);
					boxPtr[0].front 	= z + (48*THORN_SCALE);
					boxPtr[0].back 		= z + (-173*THORN_SCALE);

					boxPtr[1].left 		= x + (-35*THORN_SCALE);
					boxPtr[1].right 	= x + (35*THORN_SCALE);
					boxPtr[1].front 	= z + (-173*THORN_SCALE);
					boxPtr[1].back 		= z + (-298*THORN_SCALE);

					boxPtr[2].left 		= x + (-35*THORN_SCALE);
					boxPtr[2].right 	= x + (35*THORN_SCALE);
					boxPtr[2].front 	= z + (-298*THORN_SCALE);
					boxPtr[2].back 		= z + (-400*THORN_SCALE);
					break;
					

			case	1:
					boxPtr[0].left 		= x + (-173*THORN_SCALE);
					boxPtr[0].right 	= x + (48*THORN_SCALE);
					boxPtr[0].front 	= z + (35*THORN_SCALE);
					boxPtr[0].back 		= z + (-35*THORN_SCALE);

					boxPtr[1].left 		= x + (-298*THORN_SCALE);
					boxPtr[1].right 	= x + (-173*THORN_SCALE);
					boxPtr[1].front 	= z + (35*THORN_SCALE);
					boxPtr[1].back 		= z + (-35*THORN_SCALE);

					boxPtr[2].left 		= x + (-400*THORN_SCALE);
					boxPtr[2].right 	= x + (-298*THORN_SCALE);
					boxPtr[2].front 	= z + (35*THORN_SCALE);
					boxPtr[2].back 		= z + (-35*THORN_SCALE);
					break;

			case	2:
					boxPtr[0].left 		= x + (-35*THORN_SCALE);
					boxPtr[0].right 	= x + (35*THORN_SCALE);
					boxPtr[0].front 	= z + (173*THORN_SCALE);
					boxPtr[0].back 		= z + (-48*THORN_SCALE);

					boxPtr[1].left 		= x + (-35*THORN_SCALE);
					boxPtr[1].right 	= x + (35*THORN_SCALE);
					boxPtr[1].front 	= z + (298*THORN_SCALE);
					boxPtr[1].back 		= z + (173*THORN_SCALE);

					boxPtr[2].left 		= x + (-35*THORN_SCALE);
					boxPtr[2].right 	= x + (35*THORN_SCALE);
					boxPtr[2].front 	= z + (400*THORN_SCALE);
					boxPtr[2].back 		= z + (298*THORN_SCALE);
					break;

			case	3:
					boxPtr[0].left 		= x + (-48*THORN_SCALE);
					boxPtr[0].right 	= x + (173*THORN_SCALE);
					boxPtr[0].front 	= z + (35*THORN_SCALE);
					boxPtr[0].back 		= z + (-35*THORN_SCALE);

					boxPtr[1].left 		= x + (173*THORN_SCALE);
					boxPtr[1].right 	= x + (298*THORN_SCALE);
					boxPtr[1].front 	= z + (35*THORN_SCALE);
					boxPtr[1].back 		= z + (-35*THORN_SCALE);

					boxPtr[2].left 		= x + (298*THORN_SCALE);
					boxPtr[2].right 	= x + (400*THORN_SCALE);
					boxPtr[2].front 	= z + (35*THORN_SCALE);
					boxPtr[2].back 		= z + (-35*THORN_SCALE);
					break;
	
		}
	}

	KeepOldCollisionBoxes(newObj);							// set old stuff
	

	return(true);													// item was added
}


#pragma mark -

/************************ ADD FIRE WALL *************************/
//
// parm[0] = valve ID#
// parm[1] = rot 0 = -, 1 = \, 2 = |
// parm[2] = width (in tiles) 0 == default
//

Boolean AddFireWall(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		l;
Byte	r;

	l = itemPtr->parm[2];						// get length of wall
	if (l == 0)
		l = 3;
		
	r = itemPtr->parm[1];						// get rot 0..2
	

			/* MAKE OBJ */
			
	gNewObjectDefinition.genre 		= EVENT_GENRE;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveFireWall;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	newObj = MakeNewObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list

	newObj->PGroupMagic = 0;
	newObj->PTimer = 0;
	
	newObj->ValveID	= itemPtr->parm[0];			// get valve ID#
	newObj->ExtinguishTimer = 0;
	
	newObj->WallRot = r;						// get rot
	
	newObj->WallLength = l;						// get length
	
	return(true);								// item was added
}

/***************** MOVE FIRE WALL ***********************/

static void MoveFireWall(ObjNode *theNode)
{
int		id,l,n;
Byte	rot;
float	d;

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	rot = theNode->WallRot;								// get aim/rot

	l = theNode->WallLength;							// get # tiles width
	n = (l*3);											// # iterations
	d = (float)(l * TERRAIN_POLYGON_SIZE) / (float)n;	// dist per iteration
	
		/***********************/
		/* SEE IF PUT FIRE OUT */
		/***********************/
		
	id = theNode->ValveID;								// get valve ID
	if (gValveIsOpen[id])								// see if valve is open
	{
		theNode->ExtinguishTimer += gFramesPerSecondFrac;	// see if its been long enough to extinguish
		if (theNode->ExtinguishTimer > 3.0f)
		{
			theNode->TerrainItemPtr = nil;				// dont ever come back
			DeleteObject(theNode);
			return;
		}
	}


				/*********************/
				/* DO FIRE PARTICLES */
				/*********************/

	theNode->PTimer -= gFramesPerSecondFrac;
	if (theNode->PTimer < 0.0f)								// see if time to spew fire particles
	{
		theNode->PTimer = .06f;								// reset timer

				/* SEE IF MAKE NEW GROUP */
				
		if ((theNode->ParticleGroup == -1) || (!VerifyParticleGroupMagicNum(theNode->ParticleGroup, theNode->PGroupMagic)))
		{
new_pgroup:		
			theNode->PGroupMagic = MyRandomLong();							// generate a random magic num
			
			theNode->ParticleGroup = NewParticleGroup(theNode->PGroupMagic,		// magic num
												PARTICLE_TYPE_FALLINGSPARKS,	// type
												PARTICLE_FLAGS_HURTPLAYER|PARTICLE_FLAGS_HURTPLAYERBAD|PARTICLE_FLAGS_HOT,	// flags
												-100,						// gravity
												9000,						// magnetism
												25,							// base scale
												0.1,						// decay rate
												.7,							// fade rate
												PARTICLE_TEXTURE_FIRE);		// texture
		}

				/* ADD PARTICLES TO FIRE */
				
		if (theNode->ParticleGroup != -1)
		{
			TQ3Vector3D	delta;
			TQ3Point3D  pt;
			float		x,z;
			int			i;

			x = theNode->Coord.x;
			z = theNode->Coord.z;

			for (i = 0; i < n; i++)
			{
				pt.x = x + (RandomFloat()-.5f) * 50.0f;
				pt.z = z + (RandomFloat()-.5f) * 50.0f;
				pt.y = GetTerrainHeightAtCoord(pt.x,pt.z,FLOOR) + (RandomFloat()-.5f) * 30.0f;
				
				delta.x = (RandomFloat()-.5f) * 50.0f;
				delta.y = (RandomFloat()-.5f) * 80.0f + 100.0f;
				delta.z = (RandomFloat()-.5f) * 50.0f;
				
				if (AddParticleToGroup(theNode->ParticleGroup, &pt, &delta, RandomFloat() + 2.1f, FULL_ALPHA))
					goto new_pgroup;
				
				switch(rot)
				{
					case	0:				// horizontal wall
							x += d;
							break;
							
					case	1:				// diagonal wall
							x += d;
							z += d;
							break;

					case	2:				// vert wall
							z += d;
							break;
				}
			}		
		}
	}
}

#pragma mark -


/******************** MAKE SHOCKWAVE *********************/

void MakeShockwave(TQ3Point3D *coord, float startScale)
{
ObjNode	*newObj;
	
			/* GET Y COORD */
			
	gNewObjectDefinition.group 		= GLOBAL1_MGroupNum_ShockWave;	
	gNewObjectDefinition.type 		= GLOBAL1_MObjType_ShockWave;	
	gNewObjectDefinition.coord.x 	= coord->x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(coord->x,coord->z,FLOOR);
	gNewObjectDefinition.coord.z 	= coord->z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITE | STATUS_BIT_KEEPBACKFACES_2PASS;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-1;
	gNewObjectDefinition.moveCall 	= MoveShockwave;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= startScale;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;

	newObj->CType = CTYPE_HURTME|CTYPE_HURTENEMY;
	newObj->CBits = CBITS_TOUCHABLE;
	newObj->Damage = .25;
	
	SetObjectCollisionBounds(newObj,startScale,0,-startScale,startScale,startScale,-startScale);


	newObj->Health = .6;
	MakeObjectTransparent(newObj, newObj->Health);
}


/************** MOVE SHOCKWAVE **********************/

static void MoveShockwave(ObjNode *theNode)
{
float	s;

			/* SEE IF DONE */
			
	theNode->Health -= gFramesPerSecondFrac;

	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}
	
	GetObjectInfo(theNode);

	s = theNode->Scale.x;
	s += gFramesPerSecondFrac * 600.0f;
	theNode->Scale.x = theNode->Scale.y = theNode->Scale.z = s;

			/* UPDATE COLLISION BOX */
			
	theNode->LeftOff = -s;
	theNode->RightOff = s;
	theNode->TopOff = s;
	theNode->BottomOff = -s;
	theNode->FrontOff = s;
	theNode->BackOff = -s;
	
	MakeObjectTransparent(theNode, theNode->Health);
	
	UpdateObject(theNode);
}


#pragma mark -

/************************* ADD ROLLING BOULDER *********************************/
//
// This boulder rolls after me
//

Boolean AddRollingBoulder(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= GLOBAL1_MGroupNum_Rock;	
	gNewObjectDefinition.type 		= GLOBAL1_MObjType_ThrowRock;	
	gNewObjectDefinition.scale 		= BOULDER_SCALE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR) + 30.0f*BOULDER_SCALE;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 50;
	gNewObjectDefinition.moveCall 	= MoveRollingBoulder;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list


			/* SET COLLISION INFO */
			
	newObj->Damage 	= .3;			
	newObj->CType 	= CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_BLOCKCAMERA;
	newObj->CBits 	= CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,BOULDER_RADIUS*BOULDER_SCALE,-BOULDER_RADIUS*BOULDER_SCALE,
							-BOULDER_RADIUS*BOULDER_SCALE,BOULDER_RADIUS*BOULDER_SCALE,
							BOULDER_RADIUS*BOULDER_SCALE,-BOULDER_RADIUS*BOULDER_SCALE);
							
	return(true);								// item was added
}


/*********************** MOVE ROLLING BOULDER *****************************/

static void MoveRollingBoulder(ObjNode *theNode)
{
float	y,fps = gFramesPerSecondFrac;
float	oldX,oldZ,delta;
	
			/* SEE IF GONE */
			
	if (TrackTerrainItem(theNode))								// check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

		/*********************/
		/* BOULDER IS ACTIVE */
		/*********************/
		
	if (theNode->BoulderIsActive)
	{
		GetObjectInfo(theNode);
		oldX = gCoord.x;
		oldZ = gCoord.z;
		
					/* DO GRAVITY & FRICTION */
									
		gDelta.y += -1400.0f*fps;					// add gravity
		ApplyFrictionToDeltas(30*fps, &gDelta);		// apply motion friction
		
		
					/***********/
					/* MOVE IT */
					/***********/
					
		gCoord.y += gDelta.y*fps;					
		gCoord.x += gDelta.x*fps;
		gCoord.z += gDelta.z*fps;
		
		y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR);		// get y here
		if ((gCoord.y+theNode->BottomOff) < y)						// see if bottom below/on ground
		{
			gCoord.y = y-theNode->BottomOff;
			
			if (gDelta.y < 0.0f)									// was falling?
			{
				gDelta.y = -gDelta.y * .8f;							// bounce it
				if (fabs(gDelta.y) < 1.0f)							// when gets small enough, just make zero
					gDelta.y = 0;
				else				
				if (gDelta.y > 150.0f)
				{
					if (gDelta.y > 400.0f)							// make sure doesnt bounce too much
						gDelta.y = 400.0f;
						
					PlayEffect3D(EFFECT_ROCKSLAM,&gCoord);
				}
			}
			else
				gDelta.y = 0;										// hit while going up slope, so zero delta y
			

		}

			/* DEAL WITH SLOPES */
			
		if (gRecentTerrainNormal[FLOOR].y < .95f)							// if fairly flat, then no sliding effect
		{	
			gDelta.x += gRecentTerrainNormal[FLOOR].x * fps * 900.0f;
			gDelta.z += gRecentTerrainNormal[FLOOR].z * fps * 900.0f;
		}		
		
		delta = sqrt(gDelta.x * gDelta.x + gDelta.z * gDelta.z) * fps;
		theNode->Coord = gCoord;
		TurnObjectTowardTarget(theNode, nil, oldX, oldZ, delta * .5f, false);	
		theNode->Rot.x += delta *.01f;
		
		
				/* DAMAGE IS BASED ON SPEED */
		
		theNode->Damage = (delta / fps) / 2700.0f;		
		if (theNode->Damage > .4f)
			theNode->Damage = .4f;
		
				/* IF MOVING, NOT SOLID */
			
		if (delta > 10.0f)
		{
			theNode->CType |= CTYPE_HURTME|CTYPE_HURTENEMY;				// if moving, it hurts
			theNode->CBits = CBITS_TOUCHABLE;
		}
		else
		{
			theNode->CType &= ~(CTYPE_HURTME|CTYPE_HURTENEMY);			// if not moving, it doesnt hurt
			theNode->CBits = CBITS_ALLSOLID;
		}
					
					
				/* DO COLLISION DETECTION */
				
		HandleCollisions(theNode,CTYPE_MISC);
					
					
	}
	
	
			/**********************/
			/* BOULDER IS WAITING */
			/**********************/
	else
	{
		float	d;
		TQ3Point2D p1,p2;
		
		GetObjectInfo(theNode);
		
		gCoord.y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR) + (BOULDER_RADIUS * BOULDER_SCALE); // update y
		
		p1.x = gCoord.x;
		p1.y = gCoord.z;
		p2.x = gMyCoord.x;
		p2.y = gMyCoord.z;
		
		d = Q3Point2D_Distance(&p1,&p2);
		if (d < BOULDER_DIST)
		{
			theNode->BoulderIsActive = true;
			
			d = 200.0f/d;
			gDelta.x = (p2.x - p1.x) * d;
			gDelta.z = (p2.y - p1.y) * d;			
		}	
	}
	
					/* UPDATE */
					
	UpdateObject(theNode);
}





#pragma mark -

/************************* ADD FLOOR SPIKE *********************************/

Boolean AddFloorSpike(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	if (gLevelType != LEVEL_TYPE_HIVE)
		DoFatalAlert("AddFloorSpike: not on this level!");

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= HIVE_MObjType_FloorSpike;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR) - 5.0f;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 444;
	gNewObjectDefinition.moveCall 	= MoveFloorSpike;
	gNewObjectDefinition.scale 		= 1.0;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list

	newObj->Mode = SPIKE_MODE_WAIT;

			/* SET COLLISION INFO */
			
	newObj->Damage 	= .2;			
	newObj->CType 	= CTYPE_MISC|CTYPE_HURTME;
	newObj->CBits 	= CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,0,-SPIKE_HEIGHT,-20,20,20,-20);
							
	return(true);								// item was added
}


/*********************** MOVE FLOOR SPIKE *****************************/

static void MoveFloorSpike(ObjNode *theNode)
{
float	y,fps = gFramesPerSecondFrac;
	
			/* SEE IF GONE */
			
	if (TrackTerrainItem(theNode))								// check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);

	switch(theNode->Mode)
	{
		case	SPIKE_MODE_WAIT:
				if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) <= SPIKE_DIST)
				{
					theNode->Mode = SPIKE_MODE_GOUP;
					theNode->Delta.y = 500;
				}	
				break;
				
		case	SPIKE_MODE_GOUP:
				gCoord.y += gDelta.y * fps;
				y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR) + SPIKE_HEIGHT;
				if (gCoord.y > y)
				{
					gCoord.y = y;
					theNode->Mode = SPIKE_MODE_GODOWN;
					gDelta.y = -500;
				}
				UpdateObject(theNode);
				break;
				
		case	SPIKE_MODE_GODOWN:
				gCoord.y += gDelta.y * fps;
				y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR) - 5.0f;
				if (gCoord.y < y)
				{
					gCoord.y = y;
					theNode->Mode = SPIKE_MODE_WAIT;
				}
				UpdateObject(theNode);
				break;
	}
}















