/****************************/
/*   	COLLISION.c		    */
/* (c)1997-99 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "3dmath.h"

 
extern	TQ3Point3D	gCoord;
extern	TQ3Vector3D	gDelta;
extern	TQ3Matrix4x4	gWorkMatrix;
extern	ObjNode		*gFirstNodePtr,*gPlayerObj;
extern	long	gTerrainUnitDepth,gTerrainUnitWidth;
extern	const float	gOneOver_TERRAIN_POLYGON_SIZE;
extern	float		gFramesPerSecond,gFramesPerSecondFrac;
extern	TQ3Vector3D		gRecentTerrainNormal[2];
extern	Boolean		gDoCeiling;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void CollisionDetect(ObjNode *baseNode, u_long CType, short startNumCollisions);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_COLLISIONS				60
#define	MAX_TEMP_COLL_TRIANGLES		300

enum
{
	WH_HEAD	=	1,
	WH_FOOT =	1<<1
};

/****************************/
/*    VARIABLES             */
/****************************/


CollisionRec	gCollisionList[MAX_COLLISIONS];
short			gNumCollisions = 0;
Byte			gTotalSides;

short				gNumCollTriangles;
CollisionTriangleType	gCollTriangles[MAX_TEMP_COLL_TRIANGLES];
TQ3BoundingBox		gCollTrianglesBBox;


/******************* COLLISION DETECT *********************/
//
// INPUT: startNumCollisions = value to start gNumCollisions at should we need to keep existing data in collision list
//

static void CollisionDetect(ObjNode *baseNode, u_long CType, short startNumCollisions)
{
ObjNode 	*thisNode;
u_long		sideBits,cBits,cType;
float		relDX,relDY,relDZ;						// relative deltas
float		realDX,realDY,realDZ;					// real deltas
float		x,y,z,oldX,oldZ,oldY;
short		numBaseBoxes,targetNumBoxes,target;
CollisionBoxType *baseBoxList;
CollisionBoxType *targetBoxList;
long		leftSide,rightSide,frontSide,backSide,bottomSide,topSide,oldBottomSide;

	gNumCollisions = startNumCollisions;								// clear list

			/* GET BASE BOX INFO */
			
	numBaseBoxes = baseNode->NumCollisionBoxes;
	if (numBaseBoxes == 0)
		return;
	baseBoxList = baseNode->CollisionBoxes;

	leftSide 		= baseBoxList->left;
	rightSide 		= baseBoxList->right;
	frontSide 		= baseBoxList->front;
	backSide 		= baseBoxList->back;
	bottomSide 		= baseBoxList->bottom;
	topSide 		= baseBoxList->top;
	oldBottomSide 	= baseBoxList->oldBottom;

	x = gCoord.x;									// copy coords into registers
	y = gCoord.y;
	z = gCoord.z;


	oldY = baseNode->OldCoord.y;	
	oldX = baseNode->OldCoord.x;
	oldZ = baseNode->OldCoord.z;
	
			/* CALC REAL DELTA */
	
	if (startNumCollisions == 0)
	{
						// need to do this version for mplatforms to work correctly for unknown reasons	
		realDX = gDelta.x;
		realDY = gDelta.y;
		realDZ = gDelta.z;
		
		if (baseNode->MPlatform)						// see if factor in moving platform
		{
			ObjNode *plat = baseNode->MPlatform;
		
			realDX += plat->Delta.x;
			realDY += plat->Delta.y;
			realDZ += plat->Delta.z;	
		}
	}
	else
	{
		realDX = gCoord.x - oldX;
		realDY = gCoord.y - oldY;
		realDZ = gCoord.z - oldZ;
	}


			/****************************/		
			/* SCAN AGAINST ALL OBJECTS */
			/****************************/		
		
	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		cType = thisNode->CType;	
		if (cType == INVALID_NODE_FLAG)				// see if something went wrong
			break;
	
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;
		
		if (!(cType & CType))							// see if we want to check this Type
			goto next;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)		// don't collide against these
			goto next;		
				
		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;
	
		if (thisNode == baseNode)								// dont collide against itself
			goto next;
	
		if (baseNode->ChainNode == thisNode)					// don't collide against its own chained object
			goto next;
			
				/******************************/		
				/* NOW DO COLLISION BOX CHECK */
				/******************************/		
					
		targetNumBoxes = thisNode->NumCollisionBoxes;			// see if target has any boxes
		if (targetNumBoxes)
		{
			targetBoxList = thisNode->CollisionBoxes;
		
		
				/******************************************/
				/* CHECK BASE BOX AGAINST EACH TARGET BOX */
				/*******************************************/
				
			for (target = 0; target < targetNumBoxes; target++)
			{
						/* DO RECTANGLE INTERSECTION */
			
				if (rightSide < targetBoxList[target].left)
					continue;
					
				if (leftSide > targetBoxList[target].right)
					continue;
					
				if (frontSide < targetBoxList[target].back)
					continue;
					
				if (backSide > targetBoxList[target].front)
					continue;
					
				if (bottomSide > targetBoxList[target].top)
					continue;

				if (topSide < targetBoxList[target].bottom)
					continue;
					
									
						/* THERE HAS BEEN A COLLISION SO CHECK WHICH SIDE PASSED THRU */
			
				sideBits = 0;
				cBits = thisNode->CBits;					// get collision info bits
								
				if (cBits & CBITS_TOUCHABLE)				// if it's generically touchable, then add it without side info
					goto got_sides;
				
				relDX = realDX - thisNode->Delta.x;			// calc relative deltas
				relDY = realDY - thisNode->Delta.y;				
				relDZ = realDZ - thisNode->Delta.z;				
			
								/* CHECK FRONT COLLISION */
			
				if ((cBits & SIDE_BITS_BACK) && (relDZ > 0.0f))						// see if target has solid back & we are going relatively +Z
				{
					if (baseBoxList->oldFront < targetBoxList[target].oldBack)		// get old & see if already was in target (if so, skip)
					{
						if ((baseBoxList->front >= targetBoxList[target].back) &&	// see if currently in target
							(baseBoxList->front <= targetBoxList[target].front))
							sideBits = SIDE_BITS_FRONT;
					}
				}
				else			
								/* CHECK BACK COLLISION */
			
				if ((cBits & SIDE_BITS_FRONT) && (relDZ < 0.0f))					// see if target has solid front & we are going relatively -Z
				{
					if (baseBoxList->oldBack > targetBoxList[target].oldFront)		// get old & see if already was in target	
						if ((baseBoxList->back <= targetBoxList[target].front) &&	// see if currently in target
							(baseBoxList->back >= targetBoxList[target].back))
							sideBits = SIDE_BITS_BACK;
				}

		
								/* CHECK RIGHT COLLISION */
			
			
				if ((cBits & SIDE_BITS_LEFT) && (relDX > 0.0f))						// see if target has solid left & we are going relatively right
				{
					if (baseBoxList->oldRight < targetBoxList[target].oldLeft)		// get old & see if already was in target	
						if ((baseBoxList->right >= targetBoxList[target].left) &&	// see if currently in target
							(baseBoxList->right <= targetBoxList[target].right))
							sideBits |= SIDE_BITS_RIGHT;
				}
				else

							/* CHECK COLLISION ON LEFT */

				if ((cBits & SIDE_BITS_RIGHT) && (relDX < 0.0f))					// see if target has solid right & we are going relatively left
				{
					if (baseBoxList->oldLeft > targetBoxList[target].oldRight)		// get old & see if already was in target	
						if ((baseBoxList->left <= targetBoxList[target].right) &&	// see if currently in target
							(baseBoxList->left >= targetBoxList[target].left))
							sideBits |= SIDE_BITS_LEFT;
				}	

								/* CHECK TOP COLLISION */
			
				if ((cBits & SIDE_BITS_BOTTOM) && (relDY > 0.0f))					// see if target has solid bottom & we are going relatively up
				{
					if (baseBoxList->oldTop < targetBoxList[target].oldBottom)		// get old & see if already was in target	
						if ((baseBoxList->top >= targetBoxList[target].bottom) &&	// see if currently in target
							(baseBoxList->top <= targetBoxList[target].top))
							sideBits |= SIDE_BITS_TOP;
				}

							/* CHECK COLLISION ON BOTTOM */

				else
				if ((cBits & SIDE_BITS_TOP) && (relDY < 0.0f))						// see if target has solid top & we are going relatively down
				{
					if (baseBoxList->oldBottom >= targetBoxList[target].oldTop)		// get old & see if already was in target	
					{
						if ((baseBoxList->bottom <= targetBoxList[target].top) &&	// see if currently in target
							(baseBoxList->bottom >= targetBoxList[target].bottom))
						{
							sideBits |= SIDE_BITS_BOTTOM;
						}
					}
				}	

								 /* SEE IF ANYTHING TO ADD */
								
				if (cType & CTYPE_IMPENETRABLE)										// if its impenetrable, add to list regardless of sides
					goto got_sides;				
										 							 
				if (!sideBits)														// see if anything actually happened
					continue;

						/* ADD TO COLLISION LIST */
got_sides:
				gCollisionList[gNumCollisions].baseBox = 0;
				gCollisionList[gNumCollisions].targetBox = target;
				gCollisionList[gNumCollisions].sides = sideBits;
				gCollisionList[gNumCollisions].type = COLLISION_TYPE_OBJ;
				gCollisionList[gNumCollisions].objectPtr = thisNode;
				gNumCollisions++;	
				gTotalSides |= sideBits;											// remember total of this
			}
		}
next:	
		thisNode = thisNode->NextNode;												// next target node
	}while(thisNode != nil);


#if USE_PATH_LAYER
				/*******************************/
				/*   DO BACKGROUND COLLISIONS  */
				/*******************************/
				//
				// it's important to do these last so that any
				// collision will take final priority!
				//

	if (CType & CTYPE_BGROUND)														// see if do BG collision
		AddBGCollisions(baseNode,realDX,realDZ, CType);
#endif

	GAME_ASSERT(gNumCollisions <= MAX_COLLISIONS);									// see if overflowed (memory corruption ensued)
}


/***************** HANDLE COLLISIONS ********************/
//
// This is a generic collision handler.  Takes care of
// all processing.
//
// INPUT:  cType = CType bit mask for collision matching
//
// OUTPUT: totalSides
//

Byte HandleCollisions(ObjNode *theNode, unsigned long	cType)
{
Byte		totalSides;
short		i;
float		originalX,originalY,originalZ;
long		offset,maxOffsetX,maxOffsetZ,maxOffsetY;
long		offXSign,offZSign,offYSign;
Byte		base,target;
ObjNode		*targetObj;
CollisionBoxType *baseBoxPtr,*targetBoxPtr;
long		leftSide,rightSide,frontSide,backSide,bottomSide;
CollisionBoxType *boxList;
short		numSolidHits, numPasses = 0;
Boolean		hitImpenetrable = false;
short		oldNumCollisions;

	gNumCollisions = oldNumCollisions = 0;
	gTotalSides = 0;

	originalX = gCoord.x;									// remember starting coords
	originalY = gCoord.y;									
	originalZ = gCoord.z;								

again:
	numSolidHits = 0;

	CalcObjectBoxFromGlobal(theNode);						// calc current collision box

	CollisionDetect(theNode,cType, gNumCollisions);							// get collision info
		
	totalSides = 0;
	maxOffsetX = maxOffsetZ = maxOffsetY = -10000;
	offXSign = offYSign = offZSign = 0;

			/* GET BASE BOX INFO */
			
	if (theNode->NumCollisionBoxes == 0)					// it's gotta have a collision box
		return(0);
	boxList = theNode->CollisionBoxes;
	leftSide = boxList->left;
	rightSide = boxList->right;
	frontSide = boxList->front;
	backSide = boxList->back;
	bottomSide = boxList->bottom;


			/* SCAN THRU ALL RETURNED COLLISIONS */	
	
	for (i=oldNumCollisions; i < gNumCollisions; i++)						// handle all collisions
	{
		totalSides |= gCollisionList[i].sides;				// keep sides info
		base = gCollisionList[i].baseBox;					// get collision box index for base & target
		target = gCollisionList[i].targetBox;
		targetObj = gCollisionList[i].objectPtr;			// get ptr to target objnode
		
		baseBoxPtr = boxList + base;						// calc ptrs to base & target collision boxes
		if (targetObj)
		{
			targetBoxPtr = targetObj->CollisionBoxes;	
			targetBoxPtr += target;
		}
		
				/*********************************************/
				/* HANDLE ANY SPECIAL OBJECT COLLISION TYPES */
				/*********************************************/
				
		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
				/* SEE IF THIS OBJECT HAS SINCE BECOME INVALID */
				
			if (targetObj->CType == INVALID_NODE_FLAG)				
				continue;
		
				/* HANDLE TRIGGERS */
		
			if ((targetObj->CType & CTYPE_TRIGGER) && (cType & CTYPE_TRIGGER))	// target must be trigger and we must have been looking for them as well
			{
				if (!HandleTrigger(targetObj,theNode,gCollisionList[i].sides))	// returns false if handle as non-solid trigger
					gCollisionList[i].sides = 0;
					
				maxOffsetX = gCoord.x - originalX;								// see if trigger caused a move
				maxOffsetZ = gCoord.z - originalZ;				
			}		
		}

					/********************************/
					/* HANDLE OBJECT COLLISIONS 	*/	
					/********************************/

		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
			if (gCollisionList[i].sides & ALL_SOLID_SIDES)						// see if object with any solidness
			{
				numSolidHits++;
		
				if (targetObj->CType & CTYPE_IMPENETRABLE)							// if this object is impenetrable, then throw out any other collision offsets
				{
					hitImpenetrable = true;
					maxOffsetX = maxOffsetZ = maxOffsetY = -10000;
					offXSign = offYSign = offZSign = 0;			
				}
			
				if (gCollisionList[i].sides & SIDE_BITS_BACK)						// SEE IF BACK HIT
				{
					offset = (targetBoxPtr->front - baseBoxPtr->back)+1;			// see how far over it went
					if (offset > maxOffsetZ)
					{
						maxOffsetZ = offset;
						offZSign = 1;
					}
					gDelta.z = 0;
				}
				else
				if (gCollisionList[i].sides & SIDE_BITS_FRONT)						// SEE IF FRONT HIT
				{
					offset = (baseBoxPtr->front - targetBoxPtr->back)+1;			// see how far over it went
					if (offset > maxOffsetZ)
					{
						maxOffsetZ = offset;
						offZSign = -1;
					}
					gDelta.z = 0;
				}

				if (gCollisionList[i].sides & SIDE_BITS_LEFT)						// SEE IF HIT LEFT
				{
					offset = (targetBoxPtr->right - baseBoxPtr->left)+1;			// see how far over it went
					if (offset > maxOffsetX)
					{
						maxOffsetX = offset;
						offXSign = 1;
					}
					gDelta.x = 0;
				}
				else
				if (gCollisionList[i].sides & SIDE_BITS_RIGHT)						// SEE IF HIT RIGHT
				{
					offset = (baseBoxPtr->right - targetBoxPtr->left)+1;			// see how far over it went
					if (offset > maxOffsetX)
					{
						maxOffsetX = offset;
						offXSign = -1;
					}
					gDelta.x = 0;
				}

				if (gCollisionList[i].sides & SIDE_BITS_BOTTOM)						// SEE IF HIT BOTTOM
				{
					offset = (targetBoxPtr->top - baseBoxPtr->bottom)+1;			// see how far over it went
					if (offset > maxOffsetY)
					{
						maxOffsetY = offset;
						offYSign = 1;
					}
					gDelta.y = 0;
				}
				else
				if (gCollisionList[i].sides & SIDE_BITS_TOP)						// SEE IF HIT TOP
				{
					offset = (baseBoxPtr->top - targetBoxPtr->bottom)+1;			// see how far over it went
					if (offset > maxOffsetY)
					{
						maxOffsetY = offset;
						offYSign = -1;
					}
					gDelta.y =0;						
				}
			}
		}
		
					/********************************/
					/* HANDLE TRIANGLE COLLISIONS 	*/	
					/********************************/
#if 0
		else
		if (gCollisionList[i].type == COLLISION_TYPE_TRIANGLE)
		{
					/* ADJUST Y */
					
			originalY = gCollisionList[i].planeIntersectY - theNode->BottomOff + 1;
			maxOffsetY = 0;					
			gDelta.y = -1;													// keep some force down			
		}
#endif	
		
		if (hitImpenetrable)						// if that was impenetrable, then we dont need to check other collisions
		{
			break;			
		}
	}

		/* IF THERE WAS A SOLID HIT, THEN WE NEED TO UPDATE AND TRY AGAIN */
			
	if (numSolidHits > 0)
	{
				/* ADJUST MAX AMOUNTS */
				
		gCoord.x = originalX + (maxOffsetX * offXSign);
		gCoord.z = originalZ + (maxOffsetZ * offZSign);
		gCoord.y = originalY + (maxOffsetY * offYSign);			// y is special - we do some additional rouding to avoid the jitter problem
			
			
				/* SEE IF NEED TO SET GROUND FLAG */
				
		if (totalSides & SIDE_BITS_BOTTOM)
			theNode->StatusBits |= STATUS_BIT_ONGROUND;	
	
	
				/* SEE IF DO ANOTHER PASS */
				
		numPasses++;		
		if ((numPasses < 3) && (!hitImpenetrable))				// see if can do another pass and havnt hit anything impenetrable
		{
			goto again;
		}
	}
			


			
	return(totalSides);
}


#if USE_PATH_LAYER

/**************** ADD BG COLLISIONS *********************/

static void AddBGCollisions(ObjNode *theNode, float realDX, float realDZ, u_long cType)
{
short		oldRow,left,right,oldCol,back,front;
short		count,num;
u_short		bits;
CollisionBoxType *boxList;
float		leftSide,rightSide,frontSide,backSide;
float		oldLeftSide,oldRightSide,oldFrontSide,oldBackSide;
Boolean		checkAltTiles = cType & CTYPE_BGROUND2;		// see if check alt tiles also

			/* GET BASE BOX INFO */
			
	if (theNode->NumCollisionBoxes == 0)				// it's gotta have a collision box
		return;
	boxList = theNode->CollisionBoxes;
	leftSide = boxList->left;
	rightSide = boxList->right;
	frontSide = boxList->front;
	backSide = boxList->back;

	oldLeftSide = boxList->oldLeft;
	oldRightSide = boxList->oldRight;
	oldFrontSide = boxList->oldFront;
	oldBackSide = boxList->oldBack;


				/* SEE IF OFF MAP */

	if (frontSide >= gTerrainUnitDepth)	  			// see if front is off of map
		return;
	if (backSide < 0)				 				// see if back is off of map
		return;
	if (rightSide >= gTerrainUnitWidth)				// see if right is off of map
		return;
	if (leftSide < 0)								// see if left is off of map
		return;


				/**************************/
				/* CHECK FRONT/BACK SIDES */
				/**************************/

	if (realDZ > 0)												// see if front (+z)
	{
				/* SEE IF FRONT SIDE HIT BACKGROUND */

		oldRow = oldFrontSide*gOneOver_TERRAIN_POLYGON_SIZE;	// get old front side & see if in same row as current
		front = frontSide*gOneOver_TERRAIN_POLYGON_SIZE;		// calc front row
		if (front == oldRow)									// if in same row as before,then skip cuz didn't change
			goto check_x;

		left = leftSide*gOneOver_TERRAIN_POLYGON_SIZE; 			// calc left column
		right = rightSide*gOneOver_TERRAIN_POLYGON_SIZE;	  	// calc right column
		count = num = (right-left)+1;							// calc # of tiles wide to check.  num = original count value

		for (; count > 0; count--)
		{
			bits = GetTileCollisionBitsAtRowCol(front,left,checkAltTiles);	// get collision info at this path tile
			if (bits & SIDE_BITS_TOP)							// see if tile solid on back
			{
				gCollisionList[gNumCollisions].sides = SIDE_BITS_FRONT;
				gCollisionList[gNumCollisions].type = COLLISION_TYPE_TILE;
				gNumCollisions++;
				gTotalSides |= SIDE_BITS_FRONT;
				goto check_x;
			}
			left++;												// next column ->
		}
	}
	else
	if (realDZ < 0)												// see if away
	{
						/* SEE IF TOP SIDE HIT BACKGROUND */

		oldRow = oldBackSide*gOneOver_TERRAIN_POLYGON_SIZE;	  	// get old back side & see if in same row as current
		back = backSide*gOneOver_TERRAIN_POLYGON_SIZE;
		if (back == oldRow)										// if in same row as before,then skip
			goto check_x;

		left = leftSide*gOneOver_TERRAIN_POLYGON_SIZE; 			// calc left column
		right = rightSide*gOneOver_TERRAIN_POLYGON_SIZE;	  	// calc right column
		count = num = (right-left)+1;							// calc # of tiles wide to check.  num = original count value


		for (; count > 0; count--)
		{
			bits = GetTileCollisionBitsAtRowCol(back,left,checkAltTiles); 	// get collision info for this tile
			if (bits & SIDE_BITS_BOTTOM)						// see if tile solid on front
			{
				gCollisionList[gNumCollisions].sides = SIDE_BITS_BACK;
				gCollisionList[gNumCollisions].type = COLLISION_TYPE_TILE;
				gNumCollisions++;
				gTotalSides |= SIDE_BITS_BACK;
				goto check_x;
			}
			left++;												// next column ->
		}
	}

				/************************/
				/* DO HORIZONTAL CHECK  */
				/************************/

check_x:

	if (realDX > 0)												// see if right
	{
				/* SEE IF RIGHT SIDE HIT BACKGROUND */

		oldCol = oldRightSide*gOneOver_TERRAIN_POLYGON_SIZE; 	// get old right side & see if in same col as current
		right = rightSide*gOneOver_TERRAIN_POLYGON_SIZE;
		if (right == oldCol)				 					// if in same col as before,then skip
			return;

		back = backSide*gOneOver_TERRAIN_POLYGON_SIZE;
		front = frontSide*gOneOver_TERRAIN_POLYGON_SIZE;
		count = num = (front-back)+1;	 						// calc # of tiles high to check.  num = original count value


		for (; count > 0; count--)
		{
			bits = GetTileCollisionBitsAtRowCol(back,right,checkAltTiles); 	// get collision info for this tile
			if (bits & SIDE_BITS_LEFT)							// see if tile solid on left
			{
				gCollisionList[gNumCollisions].sides = SIDE_BITS_RIGHT;
				gCollisionList[gNumCollisions].type = COLLISION_TYPE_TILE;
				gNumCollisions++;
				gTotalSides |= SIDE_BITS_RIGHT;
				return;
			}
			back++;												// next row
		}
	}
	else
	if (realDX < 0)												// see if left
	{
						/* SEE IF LEFT SIDE HIT BACKGROUND */

		oldCol = oldLeftSide*gOneOver_TERRAIN_POLYGON_SIZE;  	// get old left side & see if in same col as current
		left = leftSide*gOneOver_TERRAIN_POLYGON_SIZE;
		if (left == oldCol)										// if in same col as before,then skip
			return;

		back = backSide*gOneOver_TERRAIN_POLYGON_SIZE;
		front = frontSide*gOneOver_TERRAIN_POLYGON_SIZE;
		count = num = (front-back)+1;							// calc # of tiles high to check.  num = original count value


		for (; count > 0; count--)
		{
			bits = GetTileCollisionBitsAtRowCol(back,left,checkAltTiles); 	// get collision info for this tile
			if (bits & SIDE_BITS_RIGHT)							// see if tile solid on right
			{
				gCollisionList[gNumCollisions].sides = SIDE_BITS_LEFT;
				gCollisionList[gNumCollisions].type = COLLISION_TYPE_TILE;
				gNumCollisions++;
				gTotalSides |= SIDE_BITS_LEFT;
				return;
			}
			back++;										// next row
		}
	}
}
#endif

/*==========================================================================================*/
/*============================== COLLISION TRIANGLES =======================================*/
/*==========================================================================================*/

#pragma mark ========== COLLISION TRIANGLE GENERATION ==========

#if 0
/******************* CREATE COLLISION TRIANGES FOR OBJECT *************************/
//
// Scans an object's geometry and creates a list of collision triangles.
//

void CreateCollisionTrianglesForObject(ObjNode *theNode)
{
short	i;

	if (theNode->BaseGroup == nil)
		return;
		
			/* CREATE TEMPORARY TRIANGLE LIST */
			
	gNumCollTriangles = 0;							// clear counter
	Q3Matrix4x4_SetIdentity(&gWorkMatrix);			// init to identity matrix
	gCollTrianglesBBox.min.x =						// init bounding box
	gCollTrianglesBBox.min.y =
	gCollTrianglesBBox.min.z = 1000000;
	gCollTrianglesBBox.max.x =
	gCollTrianglesBBox.max.y =
	gCollTrianglesBBox.max.z = -1000000;
	
	ScanForTriangles_Recurse(theNode->BaseGroup);
	
	
		/* ALLOC MEM & COPY TEMP LIST INTO REAL LIST */
			
	AllocateCollisionTriangleMemory(theNode, gNumCollTriangles);					// alloc memory
	
	theNode->CollisionTriangles->numTriangles = gNumCollTriangles;					// set # triangles
	
				/* WIDEN & COPY BBOX */
				
	theNode->CollisionTriangles->bBox.min.x = gCollTrianglesBBox.min.x - 250;
	theNode->CollisionTriangles->bBox.min.y = gCollTrianglesBBox.min.y - 250;
	theNode->CollisionTriangles->bBox.min.z = gCollTrianglesBBox.min.z - 250;

	theNode->CollisionTriangles->bBox.max.x = gCollTrianglesBBox.max.x + 250;
	theNode->CollisionTriangles->bBox.max.y = gCollTrianglesBBox.max.y + 250;
	theNode->CollisionTriangles->bBox.max.z = gCollTrianglesBBox.max.z + 250;
	
	for (i=0; i < gNumCollTriangles; i++)
		theNode->CollisionTriangles->triangles[i] = gCollTriangles[i];				// copy each triangle
	
}


/****************** SCAN FOR TRIANGLES - RECURSE ***********************/

static void ScanForTriangles_Recurse(TQ3Object obj)
{
TQ3GroupPosition	position;
TQ3Object   		object,baseGroup;
TQ3ObjectType		oType;
TQ3Matrix4x4  		stashMatrix,transform;

				/*******************************/
				/* SEE IF ACCUMULATE TRANSFORM */
				/*******************************/
				
	if (Q3Object_IsType(obj,kQ3ShapeTypeTransform))
	{
  		Q3Transform_GetMatrix(obj,&transform);
  		MatrixMultiply(&transform,&gWorkMatrix,&gWorkMatrix);
 	}
	else

				/*************************/
				/* SEE IF FOUND GEOMETRY */
				/*************************/

	if (Q3Object_IsType(obj,kQ3ShapeTypeGeometry))
	{
		oType = Q3Geometry_GetType(obj);									// get geometry type
		if (oType == kQ3GeometryTypeTriMesh)
		{
			GetTrianglesFromTriMesh(obj);
		}
	}
	else
	
			/* SEE IF RECURSE SUB-GROUP */

	if (Q3Object_IsType(obj,kQ3ShapeTypeGroup))
 	{
 		baseGroup = obj;
  		stashMatrix = gWorkMatrix;										// push matrix
  		for (Q3Group_GetFirstPosition(obj, &position); position != nil;
  			 Q3Group_GetNextPosition(obj, &position))					// scan all objects in group
 		{
   			Q3Group_GetPositionObject (obj, position, &object);			// get object from group
			if (object != NULL)
   			{
    			ScanForTriangles_Recurse(object);						// sub-recurse this object
    			Q3Object_Dispose(object);								// dispose local ref
   			}
  		}
  		gWorkMatrix = stashMatrix;										// pop matrix  		
	}
}


/************************ GET TRIANGLES FROM TRIMESH ****************************/

static void GetTrianglesFromTriMesh(TQ3Object obj)
{
TQ3TriMeshData		triMeshData;
short				v,t,i0,i1,i2;
TQ3Point3D			*points;
float				nX,nY,nZ,x,y,z;
TQ3PlaneEquation 	*plane;
TQ3Vector3D			vv;
float				m00,m01,m02,m10,m11,m12,m20,m21,m22,m30,m31,m32;


			/* GET DATA */
			
	Q3TriMesh_GetData(obj,&triMeshData);							// get trimesh data	
		
	
		/* TRANSFORM ALL POINTS */
			
	points = &triMeshData.points[0];								// point to points list
	
	m00 = gWorkMatrix.value[0][0];	m01 = gWorkMatrix.value[0][1];	m02 = gWorkMatrix.value[0][2];	// load matrix into registers
	m10 = gWorkMatrix.value[1][0];	m11 = gWorkMatrix.value[1][1];	m12 = gWorkMatrix.value[1][2];
	m20 = gWorkMatrix.value[2][0];	m21 = gWorkMatrix.value[2][1];	m22 = gWorkMatrix.value[2][2];
	m30 = gWorkMatrix.value[3][0];	m31 = gWorkMatrix.value[3][1];	m32 = gWorkMatrix.value[3][2];
	
	for (v = 0; v < triMeshData.numPoints; v++)						// scan thru all verts
	{	
		x = (m00*points[v].x) + (m10*points[v].y) + (m20*points[v].z) + m30;			// transform x value
		y = (m01*points[v].x) + (m11*points[v].y) + (m21*points[v].z) + m31;			// transform y
		points[v].z = (m02*points[v].x) + (m12*points[v].y) + (m22*points[v].z) + m32;	// transform z
		points[v].x = x;
		points[v].y = y;	
	}
	
				/* CHECK EACH FACE */
						
	for (t = 0; t < triMeshData.numTriangles; t++)					// scan thru all faces
	{
		i0 = triMeshData.triangles[t].pointIndices[0];				// get indecies of 3 points
		i1 = triMeshData.triangles[t].pointIndices[1];			
		i2 = triMeshData.triangles[t].pointIndices[2];
		
		Q3Point3D_CrossProductTri(&points[i0],&points[i1],&points[i2], &vv);	// calc face normal		
		nX = vv.x;
		nY = vv.y;
		nZ = vv.z;

			/*****************************/
			/* WE GOT A USEABLE TRIANGLE */
			/*****************************/

		x = points[i0].x;											// get coords of 1st pt
		y = points[i0].y;	
		z = points[i0].z;	
		
				/* COPY VERTEX COORDS */
				
		gCollTriangles[gNumCollTriangles].verts[0] = points[i0];	// keep coords of the 3 vertices
		gCollTriangles[gNumCollTriangles].verts[1] = points[i1];
		gCollTriangles[gNumCollTriangles].verts[2] = points[i2];
		plane = &gCollTriangles[gNumCollTriangles].planeEQ;			// get ptr to temporary collision triangle list

				/* SAVE FACE NORMAL */
										
		FastNormalizeVector(nX, nY, nZ, &plane->normal);			// normalize & save into plane equation

				/* CALC PLANE CONSTANT */

		plane->constant = 	(plane->normal.x * x) +					// calc dot product for plane constant
						  	(plane->normal.y * y) +
							(plane->normal.z * z);			
							
				/* UPDATE BOUNDING BOX */
					
		if (x < gCollTrianglesBBox.min.x)							// check all 3 vertices to update bbox
			gCollTrianglesBBox.min.x = x;
		if (y < gCollTrianglesBBox.min.y)
			gCollTrianglesBBox.min.y = y;
		if (z < gCollTrianglesBBox.min.z)
			gCollTrianglesBBox.min.z = z;
		if (x > gCollTrianglesBBox.max.x)
			gCollTrianglesBBox.max.x = x;
		if (y > gCollTrianglesBBox.max.y)
			gCollTrianglesBBox.max.y = y;
		if (z > gCollTrianglesBBox.max.z)
			gCollTrianglesBBox.max.z = z;

		if (points[i1].x < gCollTrianglesBBox.min.x)
			gCollTrianglesBBox.min.x = points[i1].x;
		if (points[i1].y < gCollTrianglesBBox.min.y)
			gCollTrianglesBBox.min.y = points[i1].y;
		if (points[i1].z < gCollTrianglesBBox.min.z)
			gCollTrianglesBBox.min.z = points[i1].z;
		if (points[i1].x > gCollTrianglesBBox.max.x)
			gCollTrianglesBBox.max.x = points[i1].x;
		if (points[i1].y > gCollTrianglesBBox.max.y)
			gCollTrianglesBBox.max.y = points[i1].y;
		if (points[i1].z > gCollTrianglesBBox.max.z)
			gCollTrianglesBBox.max.z = points[i1].z;

		if (points[i2].x < gCollTrianglesBBox.min.x)
			gCollTrianglesBBox.min.x = points[i2].x;
		if (points[i2].y < gCollTrianglesBBox.min.y)
			gCollTrianglesBBox.min.y = points[i2].y;
		if (points[i2].z < gCollTrianglesBBox.min.z)
			gCollTrianglesBBox.min.z = points[i2].z;
		if (points[i2].x > gCollTrianglesBBox.max.x)
			gCollTrianglesBBox.max.x = points[i2].x;
		if (points[i2].y > gCollTrianglesBBox.max.y)
			gCollTrianglesBBox.max.y = points[i2].y;
		if (points[i2].z > gCollTrianglesBBox.max.z)
			gCollTrianglesBBox.max.z = points[i2].z;
			
		gNumCollTriangles++;										// inc counter				
		if (gNumCollTriangles > MAX_TEMP_COLL_TRIANGLES)
			DoFatalAlert("GetTrianglesFromTriMesh: too many triangles in list!");
	}
	
			/* CLEANUP */
			
	Q3TriMesh_EmptyData(&triMeshData);
}


/**************** ALLOCATE COLLISION TRIANGLE MEMORY *******************/

static void AllocateCollisionTriangleMemory(ObjNode *theNode, long numTriangles)
{
			/* FREE OLD STUFF */
	
	if (theNode->CollisionTriangles)
		DisposeCollisionTriangleMemory(theNode);

	if (numTriangles == 0)
		DoFatalAlert("AllocateCollisionTriangleMemory: numTriangles = 0?");
	
			/* ALLOC MEMORY */
			
	theNode->CollisionTriangles = (TriangleCollisionList *)AllocPtr(sizeof(TriangleCollisionList));				// alloc main block
	if (theNode->CollisionTriangles == nil)
		DoFatalAlert("AllocateCollisionTriangleMemory: Alloc failed!");
	
	theNode->CollisionTriangles->triangles = (CollisionTriangleType *)AllocPtr(sizeof(CollisionTriangleType) * numTriangles);	// alloc triangle array

	theNode->CollisionTriangles->numTriangles = numTriangles;						// set #	
}


/******************** DISPOSE COLLISION TRIANGLE MEMORY ***********************/

void DisposeCollisionTriangleMemory(ObjNode *theNode)
{
	if (theNode->CollisionTriangles == nil)
		return;

	if (theNode->CollisionTriangles->triangles)							// nuke triangle list
		DisposePtr((Ptr)theNode->CollisionTriangles->triangles);

	DisposePtr((Ptr)theNode->CollisionTriangles);						// nuke collision data

	theNode->CollisionTriangles = nil;									// clear ptr
}

#endif

#pragma mark ========== POINT COLLISION ==========

/****************** IS POINT IN POLY ****************************/
/*
 * Quadrants:
 *    1 | 0
 *    -----
 *    2 | 3
 */
//
//	INPUT:	pt_x,pt_y	:	point x,y coords
//			cnt			:	# points in poly
//			polypts		:	ptr to array of 2D points
//

Boolean IsPointInPoly2D(float pt_x, float pt_y, Byte numVerts, TQ3Point2D *polypts)
{
Byte 		oldquad,newquad;
float 		thispt_x,thispt_y,lastpt_x,lastpt_y;
signed char	wind;										// current winding number 
Byte		i;

			/************************/
			/* INIT STARTING VALUES */
			/************************/
			
	wind = 0;
    lastpt_x = polypts[numVerts-1].x;  					// get last point's coords  
    lastpt_y = polypts[numVerts-1].y;    
    
	if (lastpt_x < pt_x)								// calc quadrant of the last point
	{
    	if (lastpt_y < pt_y)
    		oldquad = 2;
 		else
 			oldquad = 1;
 	}
 	else
    {
    	if (lastpt_y < pt_y)
    		oldquad = 3;
 		else
 			oldquad = 0;
	}
    

			/***************************/
			/* WIND THROUGH ALL POINTS */
			/***************************/
    
    for (i=0; i<numVerts; i++)
    {
   			/* GET THIS POINT INFO */
    			
		thispt_x = polypts[i].x;						// get this point's coords
		thispt_y = polypts[i].y;

		if (thispt_x < pt_x)							// calc quadrant of this point
		{
	    	if (thispt_y < pt_y)
	    		newquad = 2;
	 		else
	 			newquad = 1;
	 	}
	 	else
	    {
	    	if (thispt_y < pt_y)
	    		newquad = 3;
	 		else
	 			newquad = 0;
		}

				/* SEE IF QUADRANT CHANGED */
				
        if (oldquad != newquad)
        {
			if (((oldquad+1)&3) == newquad)				// see if advanced
            	wind++;
			else
        	if (((newquad+1)&3) == oldquad)				// see if backed up
				wind--;
    		else
			{
				float	a,b;
				
             		/* upper left to lower right, or upper right to lower left.
             		   Determine direction of winding  by intersection with x==0. */
                                             
    			a = (lastpt_y - thispt_y) * (pt_x - lastpt_x);			
                b = lastpt_x - thispt_x;
                a += lastpt_y * b;
                b *= pt_y;

				if (a > b)
                	wind += 2;
 				else
                	wind -= 2;
    		}
  		}
  		
  				/* MOVE TO NEXT POINT */
  				
   		lastpt_x = thispt_x;
   		lastpt_y = thispt_y;
   		oldquad = newquad;
	}
	

	return(wind); 										// non zero means point in poly
}





/****************** IS POINT IN TRIANGLE ****************************/
/*
 * Quadrants:
 *    1 | 0
 *    -----
 *    2 | 3
 */
//
//	INPUT:	pt_x,pt_y	:	point x,y coords
//			cnt			:	# points in poly
//			polypts		:	ptr to array of 2D points
//

Boolean IsPointInTriangle(float pt_x, float pt_y, float x0, float y0, float x1, float y1, float x2, float y2)
{
Byte 		oldquad,newquad;
float		m;
signed char	wind;										// current winding number 

			/*********************/
			/* DO TRIVIAL REJECT */
			/*********************/
			
	m = x0;												// see if to left of triangle							
	if (x1 < m)
		m = x1;
	if (x2 < m)
		m = x2;
	if (pt_x < m)
		return(false);

	m = x0;												// see if to right of triangle							
	if (x1 > m)
		m = x1;
	if (x2 > m)
		m = x2;
	if (pt_x > m)
		return(false);

	m = y0;												// see if to back of triangle							
	if (y1 < m)
		m = y1;
	if (y2 < m)
		m = y2;
	if (pt_y < m)
		return(false);

	m = y0;												// see if to front of triangle							
	if (y1 > m)
		m = y1;
	if (y2 > m)
		m = y2;
	if (pt_y > m)
		return(false);


			/*******************/
			/* DO WINDING TEST */
			/*******************/
			
		/* INIT STARTING VALUES */
			
    
	if (x2 < pt_x)								// calc quadrant of the last point
	{
    	if (y2 < pt_y)
    		oldquad = 2;
 		else
 			oldquad = 1;
 	}
 	else
    {
    	if (y2 < pt_y)
    		oldquad = 3;
 		else
 			oldquad = 0;
	}
    

			/***************************/
			/* WIND THROUGH ALL POINTS */
			/***************************/

	wind = 0;
    

//=============================================
			
	if (x0 < pt_x)									// calc quadrant of this point
	{
    	if (y0 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y0 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */
			
    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;
			
         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */
                                         
			a = (y2 - y0) * (pt_x - x2);			
            b = x2 - x0;
            a += y2 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}
				
	oldquad = newquad;

//=============================================

	if (x1 < pt_x)							// calc quadrant of this point
	{
    	if (y1 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y1 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */
			
    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;
			
         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */
                                         
			a = (y0 - y1) * (pt_x - x0);			
            b = x0 - x1;
            a += y0 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}
			
	oldquad = newquad;

//=============================================
			
	if (x2 < pt_x)							// calc quadrant of this point
	{
    	if (y2 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y2 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */
			
    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;
			
         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */
                                         
			a = (y1 - y2) * (pt_x - x1);			
            b = x1 - x2;
            a += y1 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}
	
	return(wind); 										// non zero means point in poly
}






/******************** DO SIMPLE POINT COLLISION *********************************/
//
// OUTPUT: # collisions detected
//

short DoSimplePointCollision(TQ3Point3D *thePoint, u_long cType)
{
ObjNode	*thisNode;
short	targetNumBoxes,target;
CollisionBoxType *targetBoxList;

	gNumCollisions = 0;

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (!(thisNode->CType & cType))							// see if we want to check this Type
			goto next;

		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)	// don't collide against these
			goto next;
		
		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;

	
				/* GET BOX INFO FOR THIS NODE */
					
		targetNumBoxes = thisNode->NumCollisionBoxes;			// if target has no boxes, then skip
		if (targetNumBoxes == 0)
			goto next;
		targetBoxList = thisNode->CollisionBoxes;
	
	
			/***************************************/
			/* CHECK POINT AGAINST EACH TARGET BOX */
			/***************************************/
			
		for (target = 0; target < targetNumBoxes; target++)
		{
					/* DO RECTANGLE INTERSECTION */
		
			if (thePoint->x < targetBoxList[target].left)
				continue;
				
			if (thePoint->x > targetBoxList[target].right)
				continue;
				
			if (thePoint->z < targetBoxList[target].back)
				continue;
				
			if (thePoint->z > targetBoxList[target].front)
				continue;
				
			if (thePoint->y > targetBoxList[target].top)
				continue;

			if (thePoint->y < targetBoxList[target].bottom)
				continue;
				
								
					/* THERE HAS BEEN A COLLISION */

			gCollisionList[gNumCollisions].targetBox = target;
			gCollisionList[gNumCollisions].type = COLLISION_TYPE_OBJ;
			gCollisionList[gNumCollisions].objectPtr = thisNode;
			gNumCollisions++;	
		}
		
next:	
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);

	return(gNumCollisions);
}


/******************** DO SIMPLE BOX COLLISION *********************************/
//
// OUTPUT: # collisions detected
//

short DoSimpleBoxCollision(float top, float bottom, float left, float right,
						float front, float back, u_long cType)
{
ObjNode			*thisNode;
short			targetNumBoxes,target;
CollisionBoxType *targetBoxList;

	gNumCollisions = 0;

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (!(thisNode->CType & cType))							// see if we want to check this Type
			goto next;

		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)	// don't collide against these
			goto next;
		
		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;

	
				/* GET BOX INFO FOR THIS NODE */
					
		targetNumBoxes = thisNode->NumCollisionBoxes;			// if target has no boxes, then skip
		if (targetNumBoxes == 0)
			goto next;
		targetBoxList = thisNode->CollisionBoxes;
	
	
			/*********************************/
			/* CHECK AGAINST EACH TARGET BOX */
			/*********************************/
			
		for (target = 0; target < targetNumBoxes; target++)
		{
					/* DO RECTANGLE INTERSECTION */
		
			if (right < targetBoxList[target].left)
				continue;
				
			if (left > targetBoxList[target].right)
				continue;
				
			if (front < targetBoxList[target].back)
				continue;
				
			if (back > targetBoxList[target].front)
				continue;
				
			if (bottom > targetBoxList[target].top)
				continue;

			if (top < targetBoxList[target].bottom)
				continue;
				
								
					/* THERE HAS BEEN A COLLISION */

			gCollisionList[gNumCollisions].targetBox = target;
			gCollisionList[gNumCollisions].type = COLLISION_TYPE_OBJ;
			gCollisionList[gNumCollisions].objectPtr = thisNode;
			gNumCollisions++;	
		}
		
next:	
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);

	return(gNumCollisions);
}


/******************** DO SIMPLE BOX COLLISION AGAINST PLAYER *********************************/

Boolean DoSimpleBoxCollisionAgainstPlayer(float top, float bottom, float left, float right,
										float front, float back)
{
short			targetNumBoxes,target;
CollisionBoxType *targetBoxList;


			/* GET BOX INFO FOR THIS NODE */
				
	targetNumBoxes = gPlayerObj->NumCollisionBoxes;			// if target has no boxes, then skip
	if (targetNumBoxes == 0)
		return(false);
	targetBoxList = gPlayerObj->CollisionBoxes;


		/***************************************/
		/* CHECK POINT AGAINST EACH TARGET BOX */
		/***************************************/
		
	for (target = 0; target < targetNumBoxes; target++)
	{
				/* DO RECTANGLE INTERSECTION */
	
		if (right < targetBoxList[target].left)
			continue;
			
		if (left > targetBoxList[target].right)
			continue;
			
		if (front < targetBoxList[target].back)
			continue;
			
		if (back > targetBoxList[target].front)
			continue;
			
		if (bottom > targetBoxList[target].top)
			continue;

		if (top < targetBoxList[target].bottom)
			continue;

		return(true);				
	}
	
	return(false);
}



/******************** DO SIMPLE POINT COLLISION AGAINST PLAYER *********************************/

Boolean DoSimplePointCollisionAgainstPlayer(TQ3Point3D *thePoint)
{
short	targetNumBoxes,target;
CollisionBoxType *targetBoxList;

	

			/* GET BOX INFO FOR THIS NODE */
				
	targetNumBoxes = gPlayerObj->NumCollisionBoxes;			// if target has no boxes, then skip
	if (targetNumBoxes == 0)
		return(false);
	targetBoxList = gPlayerObj->CollisionBoxes;


		/***************************************/
		/* CHECK POINT AGAINST EACH TARGET BOX */
		/***************************************/
		
	for (target = 0; target < targetNumBoxes; target++)
	{
				/* DO RECTANGLE INTERSECTION */
	
		if (thePoint->x < targetBoxList[target].left)
			continue;
			
		if (thePoint->x > targetBoxList[target].right)
			continue;
			
		if (thePoint->z < targetBoxList[target].back)
			continue;
			
		if (thePoint->z > targetBoxList[target].front)
			continue;
			
		if (thePoint->y > targetBoxList[target].top)
			continue;

		if (thePoint->y < targetBoxList[target].bottom)
			continue;

		return(true);				
	}
	
	return(false);
}

/******************** DO SIMPLE BOX COLLISION AGAINST OBJECT *********************************/

Boolean DoSimpleBoxCollisionAgainstObject(float top, float bottom, float left, float right,
										float front, float back, ObjNode *targetNode)
{
short			targetNumBoxes,target;
CollisionBoxType *targetBoxList;


			/* GET BOX INFO FOR THIS NODE */
				
	targetNumBoxes = targetNode->NumCollisionBoxes;			// if target has no boxes, then skip
	if (targetNumBoxes == 0)
		return(false);
	targetBoxList = targetNode->CollisionBoxes;


		/***************************************/
		/* CHECK POINT AGAINST EACH TARGET BOX */
		/***************************************/
		
	for (target = 0; target < targetNumBoxes; target++)
	{
				/* DO RECTANGLE INTERSECTION */
	
		if (right < targetBoxList[target].left)
			continue;
			
		if (left > targetBoxList[target].right)
			continue;
			
		if (front < targetBoxList[target].back)
			continue;
			
		if (back > targetBoxList[target].front)
			continue;
			
		if (bottom > targetBoxList[target].top)
			continue;

		if (top < targetBoxList[target].bottom)
			continue;

		return(true);				
	}
	
	return(false);
}


#if 0
/*************************** DO TRIANGLE COLLISION **********************************/

void DoTriangleCollision(ObjNode *theNode, unsigned long CType)
{
ObjNode	*thisNode;
float	x,y,z,oldX,oldY,oldZ;

	x = gCoord.x;	y = gCoord.y; 	z = gCoord.z;
	oldX = theNode->OldCoord.x;
	oldY = theNode->OldCoord.y;
	oldZ = theNode->OldCoord.z;

	gNumCollisions = 0;											// clear list
	gTotalSides = 0;

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode->CollisionTriangles == nil)				// must be triangles here
			goto next;
		
		if (!(thisNode->CType & CType))							// see if we want to check this Type
			goto next;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)		// don't collide against these
			goto next;		
				
		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;
	
		if (thisNode == theNode)								// dont collide against itself
			goto next;
	
		if (theNode->ChainNode == thisNode)						// don't collide against its own chained object
			goto next;

				/* SINCE NOTHING HIT, ADD TRIANGLE COLLISIONS */
				
		AddTriangleCollision(thisNode, x, y, z, oldX, oldZ, oldY);
next:	
		thisNode = thisNode->NextNode;												// next target node
	}while(thisNode != nil);

}


/******************* ADD TRIANGLE COLLISION ********************************/

static void AddTriangleCollision(ObjNode *thisNode, float x, float y, float z, float oldX, float oldZ, float oldY)
{
TQ3PlaneEquation 		*planeEQ;
TriangleCollisionList	*collisionRec = thisNode->CollisionTriangles;
CollisionTriangleType	*triangleList;
short					numTriangles,i;
TQ3Point3D 				intersectPt;
TQ3Vector3D				myVector,myNewVector;
float					normalX,normalY,normalZ;
float					dotProduct,reflectedX,reflectedZ,reflectedY;
TQ3Vector3D				normalizedDelta,myNormalizedVector;
float					oldMag,newMag,oldSpeed,impactFriction;
short					count;
float					offsetOldX,offsetOldY,offsetOldZ;

	if (!collisionRec)
		return;
		
	count = 0;													// init iteration count

try_again:
			/* FIRST CHECK IF INSIDE BOUNDING BOX */
			//
			// note: the bbox must be larger than the true bounds of the geometry
			//		for this to work correctly.  Since it's possible that in 1 frame of
			//		anim animation an object has gone thru a polygon and out of the bbox
			//		we enlarge the bbox (earlier) and cross our fingers that this will be sufficient.
			//
	
	if (x < collisionRec->bBox.min.x)
		return;
	if (x > collisionRec->bBox.max.x)
		return;
	if (y < collisionRec->bBox.min.y)
		return;
	if (y > collisionRec->bBox.max.y)
		return;
	if (z < collisionRec->bBox.min.z)
		return;
	if (z > collisionRec->bBox.max.z)
		return;

	myVector.x = x - oldX;														// calc my directional vector
	myVector.y = y - oldY;
	myVector.z = z - oldZ;
	
	if ((myVector.x == 0.0f) && (myVector.y == 0.0f) && (myVector.z == 0.0f))	// see if not moving
		return;
	
	FastNormalizeVector(myVector.x,myVector.y,myVector.z, &myNormalizedVector);	// normalize my delta vector
					
	offsetOldX = oldX - myNormalizedVector.x * .5f;								// back up the starting point by a small amount to fix for exact overlap
	offsetOldY = oldY - myNormalizedVector.y * .5f;
	offsetOldZ = oldZ - myNormalizedVector.z * .5f;				

				
		/**************************************************************/
		/* WE'RE IN THE BBOX, SO NOW SEE IF WE INTERSECTED A TRIANGLE */
		/**************************************************************/
	
	numTriangles = collisionRec->numTriangles;									// get # triangles to check
	triangleList = collisionRec->triangles;										// get pointer to triangles
	
	for (i = 0; i < numTriangles; i++)
	{
		planeEQ = &triangleList[i].planeEQ;										// get pointer to plane equation

				/* IGNORE BACKFACES */
				
		dotProduct = Q3Vector3D_Dot(&planeEQ->normal, &myNormalizedVector);		// calc dot product between triangle normal & my motion vector
		if (dotProduct > 0.0f)													// if angle > 90 then skip this
			continue;


					/* SEE WHERE LINE INTERSECTS PLANE */
					
		if (!IntersectionOfLineSegAndPlane(planeEQ, x, y, z, offsetOldX, offsetOldY, offsetOldZ, &intersectPt))
			continue;

						
					/* SEE IF INTERSECT PT IS INSIDE THE TRIANGLE */

		if (IsPointInTriangle3D(&intersectPt, &triangleList[i].verts[0], &planeEQ->normal))
			goto got_it;
		else
			continue;
	}	
	
				/* DIDNT ENCOUNTER ANY TRIANGLE COLLISIONS */
				
	return;


			/***********************************/
			/*       HANDLE THE COLLISION      */
			/***********************************/
got_it:

			/* ADJUST COORD TO INTERSECT POINT */
											
	gCoord = intersectPt;														// set new coord to intersect point	
	myNewVector.x = gCoord.x - oldX;											// vector to new coord (intersect point)
	myNewVector.y = gCoord.y - oldY;
	myNewVector.z = gCoord.z - oldZ;


			/* CALC MAGNITUDE OF NEW AND OLD VECTORS */
			
	oldMag = sqrt((myVector.x*myVector.x) + (myVector.y*myVector.y) + (myVector.z*myVector.z));
	
	if (count == 0)
		oldSpeed = oldMag * gFramesPerSecond;									// calc speed value
	
	newMag = sqrt((myNewVector.x*myNewVector.x) + (myNewVector.y*myNewVector.y) + (myNewVector.z*myNewVector.z));
	newMag = oldMag-newMag;														// calc diff between mags
	

	impactFriction = (400.0f - oldSpeed) / 100.0f;
	if (impactFriction <= 0.5f)
		impactFriction = 0.5f;
	if (impactFriction > 1.0f)
		impactFriction = 1.0f;

	
			/************************************/			
			/* CALC REFLECTION VECTOR TO BOUNCE */
			/************************************/

	normalX = planeEQ->normal.x;												// get triangle normal
	normalY = planeEQ->normal.y;
	normalZ = planeEQ->normal.z;
	dotProduct = normalX * myNormalizedVector.x;
	dotProduct += normalY * myNormalizedVector.y;
	dotProduct += normalZ * myNormalizedVector.z;
	dotProduct += dotProduct;
	reflectedX = normalX * dotProduct - myNormalizedVector.x;
	reflectedY = normalY * dotProduct - myNormalizedVector.y;
	reflectedZ = normalZ * dotProduct - myNormalizedVector.z;
	FastNormalizeVector(reflectedX,reflectedY,reflectedZ, &normalizedDelta);	// normalize reflection vector
	

			/*********************/
			/* BOUNCE THE DELTAS */
			/*********************/
				
	gDelta.x = normalizedDelta.x * -oldSpeed * impactFriction;					// convert reflected vector into new deltas
	gDelta.y = normalizedDelta.y * -oldSpeed * impactFriction;
	gDelta.z = normalizedDelta.z * -oldSpeed * impactFriction;

		
		/**************************************************/
		/* MAKE UP FOR THE LOST MAGNITUDE BY MOVING COORD */
		/**************************************************/
		
	if ((gDelta.x != 0.0f) || ((gDelta.y != 0.0f) || (gDelta.z == 0.0f)))		// special check for no delta
		FastNormalizeVector(gDelta.x,gDelta.y,gDelta.z, &normalizedDelta);		// normalize my delta vector
	else
		normalizedDelta.x = normalizedDelta.y = normalizedDelta.z = 0;
		
	oldX = gCoord.x;															// for next pass, the intersect point is "old"
	oldY = gCoord.y;
	oldZ = gCoord.z;
	
	gCoord.x += normalizedDelta.x * newMag * 1.0f;								// move it
	gCoord.y += normalizedDelta.y * newMag * 1.0f;
	gCoord.z += normalizedDelta.z * newMag * 1.0f;


			/***********************************************/
			/* NOW WE NEED TO PERFORM COLLISION TEST AGAIN */
			/***********************************************/
			
	if (++count < 5)											// make sure this doesnt get stuck
	{
		x = gCoord.x;											// new coords = gCoord to try again
		y = gCoord.y;
		z = gCoord.z;
		goto try_again;
	}
}
#endif


#pragma mark ========== TERRAIN COLLISION ==========




/******************* HANDLE FLOOR AND CEILING COLLISION *********************/
//
// OUTPUT: 	newCoord = adjusted coord if collision occurred
//			newDelta = adjusted/reflected delta if collision occurred
//			Boolean = true if is on ground.
//
// INPUT:	newCoord = current coord
//			oldCoord = coord before the move
//			newDelta = current deltas
//			oldDelta = current deltas
//			footYOff = y dist from coord to foot
//			headYOff = y dist from coord to head
//			speed	 = speed of newDelta (the length of the vector)
//

Boolean HandleFloorAndCeilingCollision(TQ3Point3D *newCoord, const TQ3Point3D *oldCoord,
									TQ3Vector3D *newDelta, const TQ3Vector3D *oldDelta,
									float footYOff, float headYOff, float speed)
{
Boolean				hitFoot,hitHead, isOnGround;
float				floorY,ceilingY;
float				distToFloor,distToCeiling;
float				x,y,z,fps;
Byte				whatHappened = 0;
			
	fps = gFramesPerSecondFrac;
	
again:	
			/*************************************/
			/* CALC DISTANCES TO FLOOR & CEILING */
			/*************************************/
			
	floorY = GetTerrainHeightAtCoord(newCoord->x, newCoord->z, FLOOR);		// get center Y
	
	if (gDoCeiling)
	{
		ceilingY = GetTerrainHeightAtCoord(newCoord->x, newCoord->z, CEILING);
		if (ceilingY < floorY)
			ceilingY = floorY;
	}
	else
		ceilingY = 1000000;
	
	distToFloor 	= (newCoord->y - footYOff) - floorY;
	distToCeiling	= ceilingY - (newCoord->y + headYOff);
			
			
			/* DETERMINE IF HEAD AND/OR FOOT HAS HIT */
				
	hitHead = distToCeiling <= 0.0f;
	hitFoot = distToFloor <= 0.0f;

				
		/***************************************/
		/* HANDLE FOOT & HEAD DOUBLE COLLISION */
		/***************************************/	
		//
		// In this case, we just pop the coord back to where it was because we
		// know that we were safe there on the previous frame.
		// Then we calculate new deltas based on the old delta reflected around
		// the wall's normal.
		//
	
	if (hitFoot && hitHead)
	{
		TQ3Vector3D	wallVector,temp;
both:		
		*newCoord = *oldCoord;										// move back to where it was

			/* CALC VECTOR AWAY FROM WALL */
			
		x = gRecentTerrainNormal[FLOOR].x + gRecentTerrainNormal[CEILING].x;
		y = gRecentTerrainNormal[FLOOR].y + gRecentTerrainNormal[CEILING].y;
		z = gRecentTerrainNormal[FLOOR].z + gRecentTerrainNormal[CEILING].z;
		FastNormalizeVector(x, y, z, &wallVector);
		
		
			/* REFLECT OLD DELTA VECTOR OFF OF WALL VECTOR */
		
		FastNormalizeVector(oldDelta->x, oldDelta->y, oldDelta->z, &temp);
		x = wallVector.x + (wallVector.x + temp.x);
		y = wallVector.y + (wallVector.y + temp.y);
		z = wallVector.z + (wallVector.z + temp.z);
		FastNormalizeVector(x, y, z, newDelta);

		newDelta->x *= speed;
		newDelta->y *= speed;
		newDelta->z *= speed;

	
			/* RECALC THESE */
						
		floorY 		= GetTerrainHeightAtCoord(newCoord->x, newCoord->z, FLOOR);
		ceilingY 	= GetTerrainHeightAtCoord(newCoord->x, newCoord->z, CEILING);
				
	}
	else
	{
				/******************************/
				/* SEE IF HIT HEAD ON CEILING */
				/******************************/
				//
				// Just our head hit, so we need to adjust the y coord
				// down and then also adjust the distance to the floor.
				//
				// To be safe, we *should* jump back above and run this test again
				// since the foot may have been pushed under the floor by this.
				//
				
		if (hitHead)
		{
			if (whatHappened & WH_FOOT)				// see if also just hit foot
				goto both;
				
			newCoord->y += distToCeiling;			// move me down
			distToFloor += distToCeiling;			// recalc dist to floor
			
			if (newDelta->y > 0.0f)					// bounce y
				newDelta->y *= -.3f;
				
			distToCeiling = 0;
			
			if (whatHappened == 0)
			{
				whatHappened = WH_HEAD;					// set head flag and test again
				goto again;
			}
		}
			
							
				/********************/
				/* SEE IF ON GROUND */
				/********************/
				//
				// The foot went under the floor.  We do some tweaky math here to make this
				// work nicely.  We dont want to bounce off the floor in a physics-correct
				// manner, but rather we want to "roll" up the floor smoothly.
				// 
				// We move the coord up to put it on top of the ground, and then
				// we calculate new deltas.  This is the tweaky part, so be careful with it.
				//

		else
		if (hitFoot)
		{
			if (whatHappened & WH_HEAD)				// see if also just hit head
				goto both;
		
			newCoord->y = floorY+footYOff;
			
						/* IF WENT UPHILL, THEN AFFECT DY */
						
			if (newCoord->y > oldCoord->y)
			{
				float	scale,dy;
				
				dy = newCoord->y - oldCoord->y;						// calc actual dy
				newDelta->y = dy / fps;				 				// calc motion dy 
				
				scale = speed / Q3Vector3D_Length(newDelta);		// calc velocity change
				
				newDelta->x *= scale;								// scale new delta down to match old velocity
				newDelta->y *= scale;
				newDelta->z *= scale;

#if 0					
					/* IF MOTION VECTOR CHANGED GREATLY, ABSORB ENERGY */
				{
					TQ3Vector3D	n1,n2;
					float		dot;
					
					FastNormalizeVector(oldDelta->x,oldDelta->y,oldDelta->z, &n1);
					FastNormalizeVector(newDelta->x,newDelta->y,newDelta->z, &n2);
					dot = Q3Vector3D_Dot(&n1, &n2);
					
					if (dot < .75f)									// if angle was steep enough, then absorb energy
					{
						newDelta->x *= dot;
						newDelta->y *= dot;
						newDelta->z *= dot;
					}
				}
#endif
			}			
			
						/* IF DY IS STILL DOWN, THEN BOUNCE DY */
			
			if (newDelta->y < 0.0f)
				newDelta->y *= -.3f;
				
				
						/* SET INFO TO TELL THAT IS ON GROUND */
						
			distToFloor = 0;
			isOnGround = true;
			
			if ((whatHappened == 0) && (gDoCeiling))
			{
				whatHappened = WH_FOOT;					// set foot flag and test again
				goto again;
			}
		}
		
					/*****************/
					/* NOT ON GROUND */
					/*****************/
		else
			isOnGround = false;
	}	
	
	return(isOnGround);
}
















