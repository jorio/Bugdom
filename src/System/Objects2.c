/*********************************/
/*    OBJECT MANAGER 		     */
/* (c)1993-1997 Pangea Software  */
/* By Brian Greenstone      	 */
/*********************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/



/****************************/
/*    CONSTANTS             */
/****************************/

#define	SHADOW_Y_OFF	6.0f

/**********************/
/*     VARIABLES      */
/**********************/

#define	CheckForBlockers	Flag[0]


//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT COLLISION ------

/**************** ALLOCATE COLLISION BOX MEMORY *******************/

void AllocateCollisionBoxMemory(ObjNode *theNode, short numBoxes)
{
			/* FREE OLD STUFF */
			
	if (theNode->CollisionBoxes)
	{
		DisposePtr((Ptr)theNode->CollisionBoxes);
		DisposePtr((Ptr)theNode->OldCollisionBoxes);
		theNode->CollisionBoxes = nil;
		theNode->OldCollisionBoxes = nil;
	}

				/* SET # */
				
	theNode->NumCollisionBoxes = numBoxes;
	GAME_ASSERT(numBoxes > 0);


				/* CURRENT LIST */
				
	theNode->CollisionBoxes		= (CollisionBoxType *) NewPtr(sizeof(CollisionBoxType) * numBoxes);
	theNode->OldCollisionBoxes	= (CollisionBoxType *) NewPtr(sizeof(CollisionBoxType) * numBoxes);
	GAME_ASSERT(theNode->CollisionBoxes);
	GAME_ASSERT(theNode->OldCollisionBoxes);
}


/*******************************  KEEP OLD COLLISION BOXES **************************/
//
// Also keeps old coordinate
//

void KeepOldCollisionBoxes(ObjNode *theNode)
{
	for (int i = 0; i < theNode->NumCollisionBoxes; i++)
	{
		theNode->OldCollisionBoxes[i] = theNode->CollisionBoxes[i];
	}

	theNode->OldCoord = theNode->Coord;			// remember coord also
}


/**************** CALC OBJECT BOX FROM NODE ******************/
//
// This does a simple 1 box calculation for basic objects.
//
// Box is calculated based on theNode's coords.
//

void CalcObjectBoxFromNode(ObjNode *theNode)
{
CollisionBoxType *boxPtr;

	GAME_ASSERT_MESSAGE(theNode->CollisionBoxes, "CalcObjectBox on objnode with no CollisionBoxType");
	GAME_ASSERT(theNode->NumCollisionBoxes == 1);  // Source port
		
	boxPtr = theNode->CollisionBoxes;					// get ptr to 1st box (presumed only box)

	boxPtr->left 	= theNode->Coord.x + (float)theNode->LeftOff;
	boxPtr->right 	= theNode->Coord.x + (float)theNode->RightOff;
	boxPtr->back 	= theNode->Coord.z + (float)theNode->BackOff;
	boxPtr->front 	= theNode->Coord.z + (float)theNode->FrontOff;
	boxPtr->top 	= theNode->Coord.y + (float)theNode->TopOff;
	boxPtr->bottom 	= theNode->Coord.y + (float)theNode->BottomOff;

}


/**************** CALC OBJECT BOX FROM GLOBAL ******************/
//
// This does a simple 1 box calculation for basic objects.
//
// Box is calculated based on gCoord
//

void CalcObjectBoxFromGlobal(ObjNode *theNode)
{
CollisionBoxType *boxPtr;

	if (theNode == nil)
		return;

	if (theNode->CollisionBoxes == nil)
		return;
		
	boxPtr = theNode->CollisionBoxes;					// get ptr to 1st box (presumed only box)

	boxPtr->left 	= gCoord.x  + (float)theNode->LeftOff;
	boxPtr->right 	= gCoord.x  + (float)theNode->RightOff;
	boxPtr->back 	= gCoord.z  + (float)theNode->BackOff;
	boxPtr->front 	= gCoord.z  + (float)theNode->FrontOff;
	boxPtr->top 	= gCoord.y  + (float)theNode->TopOff;
	boxPtr->bottom 	= gCoord.y  + (float)theNode->BottomOff;
}


/******************* SET OBJECT COLLISION BOUNDS **********************/
//
// Sets an object's collision offset/bounds.  Adjust accordingly for input rotation 0..3 (clockwise)
//

void SetObjectCollisionBounds(ObjNode *theNode, short top, short bottom, short left,
							 short right, short front, short back)
{

	AllocateCollisionBoxMemory(theNode, 1);					// alloc 1 collision box
	theNode->TopOff 		= top;
	theNode->BottomOff 	= bottom;	
	theNode->LeftOff 	= left;
	theNode->RightOff 	= right;
	theNode->FrontOff 	= front;
	theNode->BackOff 	= back;

	CalcObjectBoxFromNode(theNode);
	KeepOldCollisionBoxes(theNode);
}



//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT SHADOWS ------

/******************* ATTACH SHADOW TO OBJECT ************************/

ObjNode	*AttachShadowToObject(ObjNode *theNode, float scaleX, float scaleZ, Boolean checkBlockers)
{
ObjNode	*shadowObj;
									
	gNewObjectDefinition.group 		= GLOBAL1_MGroupNum_Shadow;	
	gNewObjectDefinition.type 		= GLOBAL1_MObjType_Shadow;	
	gNewObjectDefinition.coord 		= theNode->Coord;
	gNewObjectDefinition.coord.y 	+= SHADOW_Y_OFF;
	gNewObjectDefinition.flags		= STATUS_BIT_NOZWRITE | STATUS_BIT_NULLSHADER | gAutoFadeStatusBits;

	if (theNode->Slot >= SLOT_OF_DUMB+1)					// shadow *must* be after parent!
		gNewObjectDefinition.slot 	= theNode->Slot+1;
	else
		gNewObjectDefinition.slot 	= SLOT_OF_DUMB+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= scaleX;
	shadowObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (shadowObj == nil)
		return(nil);

	theNode->ShadowNode = shadowObj;

	shadowObj->RenderModifiers.drawOrder = kDrawOrder_Shadows;	// draw shadow below water (overridden in UpdateShadow)

	shadowObj->SpecialF[0] = scaleX;							// need to remeber scales for update
	shadowObj->SpecialF[1] = scaleZ;

	shadowObj->CheckForBlockers = checkBlockers;

	return(shadowObj);
}


/******************* ATTACH GLOW SHADOW TO OBJECT ************************/
//
// Use for fireflies on the night level.
//

ObjNode	*AttachGlowShadowToObject(ObjNode *theNode, float scaleX, float scaleZ, Boolean checkBlockers)
{
ObjNode	*shadowObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= NIGHT_MObjType_GlowShadow;	
	gNewObjectDefinition.coord 		= theNode->Coord;
	gNewObjectDefinition.coord.y 	+= SHADOW_Y_OFF;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITE|STATUS_BIT_NULLSHADER|STATUS_BIT_GLOW|STATUS_BIT_NOFOG|gAutoFadeStatusBits;
									
	if (theNode->Slot >= SLOT_OF_DUMB+1)					// shadow *must* be after parent!
		gNewObjectDefinition.slot 	= theNode->Slot+1;
	else
		gNewObjectDefinition.slot 	= SLOT_OF_DUMB+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= scaleX;
	shadowObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (shadowObj == nil)
		return(nil);

	theNode->ShadowNode = shadowObj;

	shadowObj->SpecialF[0] = scaleX;							// need to remeber scales for update
	shadowObj->SpecialF[1] = scaleZ;

	shadowObj->CheckForBlockers = checkBlockers;

	return(shadowObj);
}




/************************ UPDATE SHADOW *************************/

void UpdateShadow(ObjNode *theNode)
{
ObjNode *shadowNode,*thisNodePtr;
long	x,y,z;
float	dist;

	if (theNode == nil)
		return;

	shadowNode = theNode->ShadowNode;
	if (shadowNode == nil)
		return;
		
			/* SEE IF SHADOW OWNER IS IN WATER */
			//
			// Owner is in water dont draw shadows
			//
			
	if (theNode->StatusBits & STATUS_BIT_UNDERWATER)
	{
		shadowNode->StatusBits |= STATUS_BIT_HIDDEN;
		return;
	}
	else
		shadowNode->StatusBits &= ~STATUS_BIT_HIDDEN;


	shadowNode->RenderModifiers.drawOrder = kDrawOrder_Shadows;			// reset default draw order for shadow


	x = theNode->Coord.x;												// get integer copy for collision checks
	y = theNode->Coord.y + theNode->BottomOff;
	z = theNode->Coord.z;

	shadowNode->Coord = theNode->Coord;
	shadowNode->Rot.y = theNode->Rot.y;


		/****************************************************/
		/* SEE IF SHADOW IS ON BLOCKER OBJECT OR ON TERRAIN */
		/****************************************************/
		
	if (shadowNode->CheckForBlockers)
	{
		thisNodePtr = gFirstNodePtr;
		do
		{
			if (thisNodePtr->CType & CTYPE_BLOCKSHADOW)						// look for things which can block the shadow
			{
				if (thisNodePtr->CollisionBoxes)
				{
					if (y < thisNodePtr->CollisionBoxes[0].bottom)
						goto next;
					if (x < thisNodePtr->CollisionBoxes[0].left)
						goto next;
					if (x > thisNodePtr->CollisionBoxes[0].right)
						goto next;
					if (z > thisNodePtr->CollisionBoxes[0].front)
						goto next;
					if (z < thisNodePtr->CollisionBoxes[0].back)
						goto next;

						/* SHADOW IS ON OBJECT  */

					// Use same draw order as object we're standing on top of
					shadowNode->RenderModifiers.drawOrder = thisNodePtr->RenderModifiers.drawOrder;

					shadowNode->Coord.y = thisNodePtr->CollisionBoxes[0].top + SHADOW_Y_OFF;
					
					if (thisNodePtr->CType & CTYPE_LIQUID)						// if liquid, move to top
					{
						shadowNode->Coord.y += gLiquidCollisionTopOffset[thisNodePtr->Kind];
					}
					
					shadowNode->Scale.x = shadowNode->SpecialF[0];				// use preset scale
					shadowNode->Scale.z = shadowNode->SpecialF[1];
					UpdateObjectTransforms(shadowNode);
					return;
					
				}
			}		
	next:					
			thisNodePtr = (ObjNode *)thisNodePtr->NextNode;		// next node
		}
		while (thisNodePtr != nil);
	}		
		
			/************************/
			/* SHADOW IS ON TERRAIN */
			/************************/

	RotateOnTerrain(shadowNode, SHADOW_Y_OFF);							// set transform matrix

			/* CALC SCALE OF SHADOW */
			
	dist = (y - shadowNode->Coord.y) * (1.0f/400.0f);					// as we go higher, shadow gets smaller
	if (dist > .5f)
		dist = .5f;
	else
	if (dist < 0.0f)
		dist = 0;
		
	dist = 1.0f - dist;
	
	shadowNode->Scale.x = dist * shadowNode->SpecialF[0];				// this scale wont get updated until next frame (RotateOnTerrain).
	shadowNode->Scale.z = dist * shadowNode->SpecialF[1];
	
	SetObjectTransformMatrix(shadowNode);								// update transforms
}



//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT CULLING ------


/**************** CHECK ALL OBJECTS IN CONE OF VISION *******************/
//
// Checks every ObjNode to see if the object is in the code of vision
//

void CheckAllObjectsInConeOfVision(void)
{
float				radius;
ObjNode				*theNode;

	theNode = gFirstNodePtr;														// get & verify 1st node
	if (theNode == nil)
		return;

					/* PROCESS EACH OBJECT */
					
	do
	{	
		if (theNode->StatusBits & STATUS_BIT_ALWAYSCULL)
			goto try_cull;
			
		if (theNode->StatusBits & STATUS_BIT_HIDDEN)			// if hidden then treat as OFF
			goto draw_off;

		if (0 == theNode->NumMeshes)							// quick check if any geometry at all
			if (theNode->Genre != SKELETON_GENRE)				// TODO NOQUESA: i don't think this condition still matters with new renderer
				goto next;

		if (theNode->StatusBits & STATUS_BIT_DONTCULL)			// see if dont want to use our culling
			goto draw_on;

try_cull:
		radius = theNode->BoundingSphere.radius;				// get radius of object
		TQ3Point3D worldCoord =
		{
			theNode->Coord.x + theNode->BoundingSphere.origin.x,
			theNode->Coord.y + theNode->BoundingSphere.origin.y,
			theNode->Coord.z + theNode->BoundingSphere.origin.z,
		};
		if (!IsSphereInFrustum_XZ(&worldCoord, radius))
			goto draw_off;

draw_on:
		theNode->StatusBits &= ~STATUS_BIT_ISCULLED;							// clear cull bit
		goto next;

draw_off:
		theNode->StatusBits |= STATUS_BIT_ISCULLED;								// set cull bit
	
	
				/* NEXT NODE */
next:			
		theNode = theNode->NextNode;		// next node
	}
	while (theNode != nil);	
}


//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- MISC OBJECT FUNCTIONS ------


/************************ STOP OBJECT STREAM EFFECT *****************************/

void StopObjectStreamEffect(ObjNode *theNode)
{
	if (theNode->EffectChannel != -1)
	{
		StopAChannel(&theNode->EffectChannel);
	}
}


/******************** CALC NEW TARGET OFFSETS **********************/

void CalcNewTargetOffsets(ObjNode *theNode, float scale)
{
	theNode->TargetOff.x = (RandomFloat()-0.5f) * scale;
	theNode->TargetOff.y = (RandomFloat()-0.5f) * scale;
}
