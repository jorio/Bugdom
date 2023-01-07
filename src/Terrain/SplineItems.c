/****************************/
/*   	SPLINE ITEMS.C      */
/* (C)1999 Pangea Software  */
/* (C)2023 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static Boolean NilPrime(long splineNum, SplineItemType *itemPtr);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_SPLINE_OBJECTS		100
#define MAX_PLACEMENT			(1.0f - EPS)		// 0 <= placement <= 0.999, in order to avoid buffer overruns when accessing spline point list


/**********************/
/*     VARIABLES      */
/**********************/

SplineDefType	**gSplineList = nil;
long			gNumSplines = 0;

static long		gNumSplineObjects = 0;
static ObjNode	*gSplineObjectList[MAX_SPLINE_OBJECTS];


/**********************/
/*     TABLES         */
/**********************/

#define	MAX_SPLINE_ITEM_NUM	54				// for error checking!

Boolean (*gSplineItemPrimeRoutines[])(long, SplineItemType *) =
{
		NilPrime,							// My Start Coords
		NilPrime,							// 1: xxxxx
		NilPrime,							// 2: Sugar
		PrimeEnemy_BoxerFly,				// 3: ENEMY: BOXERFLY
		NilPrime,							// 4: bug test
		NilPrime,							// 5: Clover
		NilPrime,							// 6: Grass
		NilPrime,							// 7: Weed
		PrimeEnemy_Slug,					// 8: Slug enemy
		PrimeEnemy_Ant,						// 9: Fireant enemy
		NilPrime,							// 10: Sunflower
		NilPrime,							// 11: Cosmo
		NilPrime,							// 12: Poppy
		NilPrime,							// 13: Wall End
		NilPrime,							// 14: Water Patch
		NilPrime,							// 15: FireAnt
		NilPrime,							// 16: WaterBug
		NilPrime,							// 17: Tree (flight level)
		NilPrime,							// 18: Dragonfly
		NilPrime,							// 19: Cat Tail
		NilPrime,							// 20: Duck Weed
		NilPrime,							// 21: Lily Flower
		NilPrime,							// 22: Lily Pad
		NilPrime,							// 23: Pond Grass
		NilPrime,							// 24: Reed
		NilPrime,							// 25: Pond Fish Enemy
		PrimeHoneycombPlatform,				// 26: Honeycomb platform
		NilPrime,							// 27: Honey Patch
		NilPrime,							// 28: Firecracker
		NilPrime,							// 29: Detonator
		NilPrime,							// 30: Wax Membrane
		PrimeEnemy_Mosquito,				// 31: Mosquito Enemy
		NilPrime,							// 32: Checkpoint
		NilPrime,							// 33: Lawn Door
		NilPrime,							// 34: Dock
		PrimeFoot,							// 35: Foot
		PrimeEnemy_Spider,					// 36: ENEMY: SPIDER
		PrimeEnemy_Caterpiller,				// 37: ENEMY: CATERPILLER
		NilPrime,							// 38: ENEMY: FIREFLY
		NilPrime,							// 39: Exit Log
		NilPrime,							// 40: Root swing
		NilPrime,							// 41: Thorn Bush
		NilPrime,							// 42: FireFly Target Location
		NilPrime,							// 43: Fire Wall
		NilPrime,							// 44: Water Valve
		NilPrime,							// 45: Honey Tube
		PrimeEnemy_Larva,					// 46: ENEMY: LARVA
		NilPrime,							// 47: ENEMY: FLYING BEE
		PrimeEnemy_WorkerBee,				// 48: ENEMY: WORKER BEE
		NilPrime,							// 49: ENEMY: QUEEN BEE
		NilPrime,							// 50: Rock Ledge
		NilPrime,							// 51: Stump
		NilPrime,							// 52: Rolling Boulder
		PrimeEnemy_Roach,					// 53: ENEMY: ROACH
		PrimeEnemy_Skippy,					// 54: ENEMY: SKIPPY
};



/********************* PRIME SPLINES ***********************/
//
// Called during terrain prime function to initialize 
// all items on the splines and recalc spline coords
//

void PrimeSplines(void)
{
long			s,i,type;
SplineItemType 	*itemPtr;
SplineDefType	*spline;
Boolean			flag;
SplinePointType	*points;

			/* ADJUST SPLINE TO GAME COORDINATES */
			
	for (s = 0; s < gNumSplines; s++)
	{
		spline = &(*gSplineList)[s];							// point to this spline
		points = (*spline->pointList);							// point to points list
		
		for (i = 0; i < spline->numPoints; i++)
		{
			points[i].x *= MAP2UNIT_VALUE;
			points[i].z *= MAP2UNIT_VALUE;
		}

	}	
	
	
				/* CLEAR SPLINE OBJECT LIST */
				
	gNumSplineObjects = 0;										// no items in spline object node list yet

	for (s = 0; s < gNumSplines; s++)
	{
		spline = &(*gSplineList)[s];							// point to this spline
		
				/* SCAN ALL ITEMS ON THIS SPLINE */
				
		HLockHi((Handle)spline->itemList);						// make sure this is permanently locked down
		for (i = 0; i < spline->numItems; i++)
		{
			itemPtr = &(*spline->itemList)[i];					// point to this item
			type = itemPtr->type;								// get item type
			GAME_ASSERT(type <= MAX_SPLINE_ITEM_NUM);

			flag = gSplineItemPrimeRoutines[type](s,itemPtr); 	// call item's Prime routine
			if (flag)
				itemPtr->flags |= ITEM_FLAGS_INUSE;				// set in-use flag	
		}
	}
}


/******************** NIL PRIME ***********************/
//
// nothing prime
//

static Boolean NilPrime(long splineNum, SplineItemType *itemPtr)
{
	(void) splineNum;
	(void) itemPtr;

	return(false);
}



/*********************** GET COORD ON SPLINE **********************/

int GetCoordOnSpline(const SplineDefType* spline, float placement, float* x, float* z)
{
	int numPoints = spline->numPoints;

	GAME_ASSERT(spline->numPoints > 0);

	// Clamp placement before accessing array to avoid overrun
	placement = ClampFloat(placement, 0, MAX_PLACEMENT);

	float scaledPlacement = placement * numPoints;

	int index1 = (int)(scaledPlacement);
	int index2 = (index1 < numPoints - 1) ? (index1 + 1) : (0);

	GAME_ASSERT(index1 >= 0 && index1 <= numPoints);
	GAME_ASSERT(index2 >= 0 && index2 <= numPoints);

	const SplinePointType* point1 = &(*spline->pointList)[index1];
	const SplinePointType* point2 = &(*spline->pointList)[index2];

	// Fractional of progression from point1 to point2
	float interpointFrac = scaledPlacement - (int)scaledPlacement;

	// Lerp point1 -> point2
	*x = point1->x * (1 - interpointFrac) + point2->x * interpointFrac;
	*z = point1->z * (1 - interpointFrac) + point2->z * interpointFrac;

	return index1;
}


/********************* IS SPLINE ITEM VISIBLE ********************/
//
// Returns true if the input objnode is in visible range.
// Also, this function handles the attaching and detaching of the objnode
// as needed.
//

Boolean IsSplineItemVisible(ObjNode *theNode)
{
Boolean	visible = true;
long	row,col;

			/* IF IS ON AN ACTIVE SUPERTILE, THEN ASSUME VISIBLE */

	row = theNode->Coord.z * (1.0f/TERRAIN_SUPERTILE_UNIT_SIZE);	// calc supertile row,col
	col = theNode->Coord.x * (1.0f/TERRAIN_SUPERTILE_UNIT_SIZE);
	
	if (gTerrainScrollBuffer[row][col] == EMPTY_SUPERTILE)
		visible = false;
	else
		visible = true;


			/* HANDLE OBJNODE UPDATES */			

	if (visible)
	{
		if (theNode->StatusBits & STATUS_BIT_DETACHED)			// see if need to insert into linked list
		{
			AttachObject(theNode);
			AttachObject(theNode->ShadowNode);
			AttachObject(theNode->ChainNode);
		}
	}
	else
	{
		if (!(theNode->StatusBits & STATUS_BIT_DETACHED))		// see if need to remove from linked list
		{
			DetachObject(theNode);
			DetachObject(theNode->ShadowNode);
			DetachObject(theNode->ChainNode);
		}
	}

	return(visible);
}


#pragma mark ======= SPLINE OBJECTS ================

/******************* ADD TO SPLINE OBJECT LIST ***************************/
//
// Called by object's primer function to add the detached node to the spline item master
// list so that it can be maintained.
//

void AddToSplineObjectList(ObjNode *theNode)
{
	GAME_ASSERT_MESSAGE(gNumSplineObjects < MAX_SPLINE_OBJECTS, "Too many spline objects");

	theNode->SplineObjectIndex = gNumSplineObjects;					// remember where in list this is

	gSplineObjectList[gNumSplineObjects++] = theNode;	
}


/****************** REMOVE FROM SPLINE OBJECT LIST **********************/
//
// OUTPUT:  true = the obj was on a spline and it was removed from it
//			false = the obj was not on a spline.
//

Boolean RemoveFromSplineObjectList(ObjNode *theNode)
{
	theNode->StatusBits &= ~STATUS_BIT_ONSPLINE;		// make sure this flag is off

	if (theNode->SplineObjectIndex != -1)
	{
		gSplineObjectList[theNode->SplineObjectIndex] = nil;			// nil out the entry into the list
		theNode->SplineObjectIndex = -1;
		theNode->SplineItemPtr = nil;
		theNode->SplineMoveCall = nil;
		return(true);
	}
	else
	{
		return(false);	
	}
}


/**************** EMPTY SPLINE OBJECT LIST ***********************/
//
// Called by level cleanup to dispose of the detached ObjNode's in this list.
//

void EmptySplineObjectList(void)
{
int	i;
ObjNode	*o;

	for (i = 0; i < gNumSplineObjects; i++)
	{
		o = gSplineObjectList[i];
		if (o)
			DeleteObject(o);			// This will dispose of all memory used by the node.
										// RemoveFromSplineObjectList will be called by it. 
	}

}


/******************* MOVE SPLINE OBJECTS **********************/

void MoveSplineObjects(void)
{
long	i;
ObjNode	*theNode;

	for (i = 0; i < gNumSplineObjects; i++)
	{
		theNode = gSplineObjectList[i];
		if (theNode)
		{
			if (theNode->SplineMoveCall)
				theNode->SplineMoveCall(theNode);				// call object's spline move routine
		}
	}
}


/*********************** GET OBJECT COORD ON SPLINE **********************/
//
// OUTPUT: 	x,y = coords
//			long = index into point list that the coord was found
//

int GetObjectCoordOnSpline(ObjNode* theNode, float* x, float* z)
{
	return GetCoordOnSpline(&(*gSplineList)[theNode->SplineNum], theNode->SplinePlacement, x, z);
}


/******************* INCREASE SPLINE INDEX *********************/
//
// Moves objects on spline at given speed
//

float IncreaseSplineIndex(ObjNode *theNode, float speed)
{
	SplineDefType* spline = &(*gSplineList)[theNode->SplineNum];

	float placement = theNode->SplinePlacement;

	placement += speed * gFramesPerSecondFrac / spline->numPoints;

	if (placement > MAX_PLACEMENT)
	{
		// Loop to start
		placement -= 1.0f;
		placement = ClampFloat(placement, 0, MAX_PLACEMENT);
	}
	else if (placement < 0)
	{
		// Loop to end
		placement += 1.0f;
		placement = ClampFloat(placement, 0, MAX_PLACEMENT);
	}

	theNode->SplinePlacement = placement;

	return placement;
}


/******************* INCREASE SPLINE INDEX ZIGZAG *********************/
//
// Moves objects on spline at given speed, but zigzags
//

float IncreaseSplineIndexZigZag(ObjNode *theNode, float speed)
{
SplineDefType	*splinePtr;
float			numPointsInSpline;

	speed *= gFramesPerSecondFrac;

	splinePtr = &(*gSplineList)[theNode->SplineNum];			// point to the spline
	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline

			/* GOING BACKWARD */

	if (theNode->StatusBits & STATUS_BIT_REVERSESPLINE)			// see if going backward
	{
		theNode->SplinePlacement -= speed / numPointsInSpline;
		if (theNode->SplinePlacement <= 0.0f)
		{
			theNode->SplinePlacement = 0;
			theNode->StatusBits ^= STATUS_BIT_REVERSESPLINE;	// toggle direction
		}
	}

		/* GOING FORWARD */

	else
	{
		theNode->SplinePlacement += speed / numPointsInSpline;
		if (theNode->SplinePlacement >= MAX_PLACEMENT)
		{
			theNode->SplinePlacement = MAX_PLACEMENT;
			GAME_ASSERT(theNode->SplinePlacement >= 0);
			GAME_ASSERT(theNode->SplinePlacement < 1);
			theNode->StatusBits ^= STATUS_BIT_REVERSESPLINE;	// toggle direction
		}
	}

	return theNode->SplinePlacement;
}


/******************* DRAW SPLINES FOR DEBUGGING *********************/

void DrawSplines(void)
{
	for (int i = 0; i < gNumSplines; i++)
	{
		const SplineDefType* spline = &(*gSplineList)[i];

		if (spline->numPoints == 0)
		{
			continue;
		}

		const SplinePointType* points = *spline->pointList;
		const SplinePointType* nubs = *spline->nubList;
		const int halfway = spline->numPoints / 2;



		if (IsPositionOutOfRange(points[0].x, points[0].z)
			&& IsPositionOutOfRange(points[halfway].x, points[halfway].z))
		{
			continue;
		}

		float liquidY = 0;
		Boolean isAboveLiquid = FindLiquidY(points[halfway].x, points[halfway].z, &liquidY);

		if (isAboveLiquid)
		{
			glBegin(GL_LINE_STRIP);
			for (int j = 0; j < spline->numPoints; j++)
			{
				glVertex3f(points[j].x, liquidY + 100, points[j].z);
			}
			glEnd();

			glBegin(GL_LINES);
			for (int nub = 0; nub < spline->numNubs; nub++)
			{
				glVertex3f(nubs[nub].x, liquidY + 150, nubs[nub].z);
				glVertex3f(nubs[nub].x, liquidY + 50, nubs[nub].z);
			}
			glEnd();
		}
		else
		{
			glBegin(GL_LINE_STRIP);
			for (int j = 0; j < spline->numPoints; j++)
			{
				float x = points[j].x;
				float z = points[j].z;
				float y = GetTerrainHeightAtCoord(x, z, FLOOR) + 10;
				glVertex3f(x, y, z);
			}
			glEnd();

			glBegin(GL_LINES);
			for (int nub = 0; nub < spline->numNubs; nub++)
			{
				float x = nubs[nub].x * 5;
				float z = nubs[nub].z * 5;
				float y = GetTerrainHeightAtCoord(x, z, FLOOR) + 10;
				glVertex3f(x, y+50, z);
				glVertex3f(x, y-50, z);
			}
			glEnd();
		}
	}
}

