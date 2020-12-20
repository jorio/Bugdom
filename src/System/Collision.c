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
				gCollisionList[gNumCollisions].objectPtr = thisNode;
				gNumCollisions++;	
				gTotalSides |= sideBits;											// remember total of this
			}
		}
next:	
		thisNode = thisNode->NextNode;												// next target node
	}while(thisNode != nil);


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

		GAME_ASSERT(targetObj);
		
		if (targetObj->CType == INVALID_NODE_FLAG)
		{
			printf("Collided with dead object!\n");
			continue;
		}
		
		targetBoxPtr = targetObj->CollisionBoxes;	
		targetBoxPtr += target;
		
		
				/*********************************************/
				/* HANDLE ANY SPECIAL OBJECT COLLISION TYPES */
				/*********************************************/

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
			if (targetObj && targetObj->CType == INVALID_NODE_FLAG)
			{
				printf("Target object died after trigger!\n");
				continue;
			}
		}

					/********************************/
					/* HANDLE OBJECT COLLISIONS 	*/	
					/********************************/

		if (gCollisionList[i].sides & ALL_SOLID_SIDES)						// see if object with any solidness
		{
			numSolidHits++;

			if (targetObj->CType & CTYPE_IMPENETRABLE)						// if this object is impenetrable, then throw out any other collision offsets
			{
				hitImpenetrable = true;
				maxOffsetX = maxOffsetZ = maxOffsetY = -10000;
				offXSign = offYSign = offZSign = 0;
			}

			if (gCollisionList[i].sides & SIDE_BITS_BACK)					// SEE IF BACK HIT
			{
				offset = (targetBoxPtr->front - baseBoxPtr->back)+1;		// see how far over it went
				if (offset > maxOffsetZ)
				{
					maxOffsetZ = offset;
					offZSign = 1;
				}
				gDelta.z = 0;
			}
			else
			if (gCollisionList[i].sides & SIDE_BITS_FRONT)					// SEE IF FRONT HIT
			{
				offset = (baseBoxPtr->front - targetBoxPtr->back)+1;		// see how far over it went
				if (offset > maxOffsetZ)
				{
					maxOffsetZ = offset;
					offZSign = -1;
				}
				gDelta.z = 0;
			}

			if (gCollisionList[i].sides & SIDE_BITS_LEFT)					// SEE IF HIT LEFT
			{
				offset = (targetBoxPtr->right - baseBoxPtr->left)+1;		// see how far over it went
				if (offset > maxOffsetX)
				{
					maxOffsetX = offset;
					offXSign = 1;
				}
				gDelta.x = 0;
			}
			else
			if (gCollisionList[i].sides & SIDE_BITS_RIGHT)					// SEE IF HIT RIGHT
			{
				offset = (baseBoxPtr->right - targetBoxPtr->left)+1;		// see how far over it went
				if (offset > maxOffsetX)
				{
					maxOffsetX = offset;
					offXSign = -1;
				}
				gDelta.x = 0;
			}

			if (gCollisionList[i].sides & SIDE_BITS_BOTTOM)					// SEE IF HIT BOTTOM
			{
				offset = (targetBoxPtr->top - baseBoxPtr->bottom)+1;		// see how far over it went
				if (offset > maxOffsetY)
				{
					maxOffsetY = offset;
					offYSign = 1;
				}
				gDelta.y = 0;
			}
			else
			if (gCollisionList[i].sides & SIDE_BITS_TOP)					// SEE IF HIT TOP
			{
				offset = (baseBoxPtr->top - targetBoxPtr->bottom)+1;		// see how far over it went
				if (offset > maxOffsetY)
				{
					maxOffsetY = offset;
					offYSign = -1;
				}
				gDelta.y = 0;
			}
		}

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


#pragma mark ========== POINT COLLISION ==========

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


		isOnGround = true;
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
			isOnGround = false;
			
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
















