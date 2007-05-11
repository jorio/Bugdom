/*********************************/
/*    OBJECT MANAGER 		     */
/* (c)1993-1997 Pangea Software  */
/* By Brian Greenstone      	 */
/*********************************/


/***************/
/* EXTERNALS   */
/***************/

#include "3dmath.h"

extern	TQ3Object	gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TQ3BoundingSphere		gObjectGroupRadiusList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TQ3Matrix4x4	gCameraWorldToViewMatrix,gCameraViewToFrustumMatrix;
extern	short		gNumObjectsInGroupList[MAX_3DMF_GROUPS];
extern	float		gFramesPerSecondFrac;
extern	ObjNode		*gPlayerObj,*gFirstNodePtr;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	TQ3Point3D	gCoord;
extern	TQ3Object	gKeepBackfaceStyleObject;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	u_long		gAutoFadeStatusBits;
extern	PrefsType	gGamePrefs;

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
Ptr	mem;

			/* FREE OLD STUFF */
			
	if (theNode->CollisionBoxes)
		DisposePtr((Ptr)theNode->CollisionBoxes);		

				/* SET # */
				
	theNode->NumCollisionBoxes = numBoxes;
	if (numBoxes == 0)
		DoFatalAlert("\pAllocateCollisionBoxMemory with 0 boxes?");


				/* CURRENT LIST */
				
	mem = AllocPtr(sizeof(CollisionBoxType)*numBoxes);
	if (mem == nil)
		DoFatalAlert("\pCouldnt alloc collision box memory");
	theNode->CollisionBoxes = (CollisionBoxType *)mem;
}


/*******************************  KEEP OLD COLLISION BOXES **************************/
//
// Also keeps old coordinate
//

void KeepOldCollisionBoxes(ObjNode *theNode)
{
long	i;

	for (i = 0; i < theNode->NumCollisionBoxes; i++)
	{
		theNode->CollisionBoxes[i].oldTop = theNode->CollisionBoxes[i].top;
		theNode->CollisionBoxes[i].oldBottom = theNode->CollisionBoxes[i].bottom;
		theNode->CollisionBoxes[i].oldLeft = theNode->CollisionBoxes[i].left;
		theNode->CollisionBoxes[i].oldRight = theNode->CollisionBoxes[i].right;
		theNode->CollisionBoxes[i].oldFront = theNode->CollisionBoxes[i].front;
		theNode->CollisionBoxes[i].oldBack = theNode->CollisionBoxes[i].back;
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

	if (theNode->CollisionBoxes == nil)
		DoFatalAlert("\pCalcObjectBox on objnode with no CollisionBoxType");
		
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
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITE|STATUS_BIT_NULLSHADER|STATUS_BIT_NOFOG|gAutoFadeStatusBits;
									
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

//	MakeObjectKeepBackfaces(shadowObj);

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

					shadowNode->Coord.y = thisNodePtr->CollisionBoxes[0].top + SHADOW_Y_OFF;
					
					if (thisNodePtr->CType & CTYPE_LIQUID)						// if liquid, move to top
					{
						switch(thisNodePtr->Kind)
						{
							case	LIQUID_WATER:
									shadowNode->Coord.y += WATER_COLLISION_TOPOFF;
									break;
									
							case	LIQUID_SLIME:
									shadowNode->Coord.y += SLIME_COLLISION_TOPOFF;
									break;

							case	LIQUID_HONEY:
									shadowNode->Coord.y += HONEY_COLLISION_TOPOFF;
									break;

							case	LIQUID_LAVA:
									shadowNode->Coord.y += LAVA_COLLISION_TOPOFF;
									break;						
						}
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
float				radius,w,w2;
float				rx,ry,px,py,pz;
ObjNode				*theNode;
register float		n00,n01,n02;
register float		n10,n11,n12;
register float		n20,n21,n22;
register float		n30,n31,n32;
float		m00,m01,m02,m03;
float		m10,m11,m12,m13;
float		m20,m21,m22,m23;
float		m30,m31,m32,m33;
float				worldX,worldY,worldZ;
float				hither,yon;

	theNode = gFirstNodePtr;														// get & verify 1st node
	if (theNode == nil)
		return;

					/* PRELOAD WORLD -> VIEW MATRIX */

	n00 = gCameraWorldToViewMatrix.value[0][0];		n01 = gCameraWorldToViewMatrix.value[0][1];	  n02 = gCameraWorldToViewMatrix.value[0][2];
	n10 = gCameraWorldToViewMatrix.value[1][0];		n11 = gCameraWorldToViewMatrix.value[1][1];	  n12 = gCameraWorldToViewMatrix.value[1][2];	
	n20 = gCameraWorldToViewMatrix.value[2][0];		n21 = gCameraWorldToViewMatrix.value[2][1];	  n22 = gCameraWorldToViewMatrix.value[2][2];	
	n30 = gCameraWorldToViewMatrix.value[3][0];		n31 = gCameraWorldToViewMatrix.value[3][1];	  n32 = gCameraWorldToViewMatrix.value[3][2];	
				

					/* PRELOAD VIEW -> FRUSTUM MATRIX */
					
	m00 = gCameraViewToFrustumMatrix.value[0][0];	m01 = gCameraViewToFrustumMatrix.value[0][1]; m02 = gCameraViewToFrustumMatrix.value[0][2]; m03 = gCameraViewToFrustumMatrix.value[0][3];
	m10 = gCameraViewToFrustumMatrix.value[1][0];	m11 = gCameraViewToFrustumMatrix.value[1][1]; m12 = gCameraViewToFrustumMatrix.value[1][2]; m13 = gCameraViewToFrustumMatrix.value[1][3];
	m20 = gCameraViewToFrustumMatrix.value[2][0];	m21 = gCameraViewToFrustumMatrix.value[2][1]; m22 = gCameraViewToFrustumMatrix.value[2][2]; m23 = gCameraViewToFrustumMatrix.value[2][3];
	m30 = gCameraViewToFrustumMatrix.value[3][0];	m31 = gCameraViewToFrustumMatrix.value[3][1]; m32 = gCameraViewToFrustumMatrix.value[3][2]; m33 = gCameraViewToFrustumMatrix.value[3][3];


	hither = -gGameViewInfoPtr->hither;							// preload these constants into registers
	yon = - gGameViewInfoPtr->yon;

					/* PROCESS EACH OBJECT */
					
	do
	{	
		if (theNode->StatusBits & STATUS_BIT_ALWAYSCULL)
			goto try_cull;
			
		if (theNode->StatusBits & STATUS_BIT_HIDDEN)			// if hidden then treat as OFF
			goto draw_off;
		
		if (theNode->BaseGroup == nil)							// quick check if any geometry at all
			if (theNode->Genre != SKELETON_GENRE)
				goto next;

		if (theNode->StatusBits & STATUS_BIT_DONTCULL)			// see if dont want to use our culling
			goto draw_on;

try_cull:
		radius = theNode->BoundingSphere.radius;				// get radius of object

			/******************************/
			/* CALC WORLD COORD OF OBJECT */
			/******************************/

					/* CALC WORLD Z */
				
		px = theNode->Coord.x + theNode->BoundingSphere.origin.x;	// get origin
		py = theNode->Coord.y + theNode->BoundingSphere.origin.y;
		pz = theNode->Coord.z + theNode->BoundingSphere.origin.z;		
		worldZ = (n02*px) + (n12*py) + (n22*pz) + n32;						
				
					/* SEE IF BEHIND CAMERA */
					
		if (worldZ >= hither)									
		{
			if ((worldZ - radius) > hither)							// is entire sphere behind camera?
				goto draw_off;
				
					/* PARTIALLY BEHIND */
					
			worldZ -= radius;										// move edge over hither plane so cone calc will work
		}
		else
		{
				/* SEE IF BEYOND YON PLANE */
			
			if ((worldZ + radius) < yon)							// see if too far away
				goto draw_off;
		}
		
		

				/* CALC WORLD X & Y */
				
		worldX = (n00*px) + (n10*py) + (n20*pz) + n30;						
		worldY = (n01*px) + (n11*py) + (n21*pz) + n31;



			/*****************************/
			/* SEE IF WITHIN VISION CONE */
			/*****************************/
	
					/* TRANSFORM WORLD COORD & RADIUS */
			
		w = (m03*worldX) + (m13*worldY) + (m23*worldZ) + m33;						// transf world X
		px = ((m00*worldX) + (m10*worldY) + (m20*worldZ) + m30) * w;
		w2 = (m03*radius) + (m13*radius) + (m23*worldZ) + m33;						// transf world radius X
		rx = ((m00*radius) + (m10*radius) + (m20*worldZ) + m30) * w2;
	
		if ((px + rx) < -1.0f)														// see if sphere "would be" out of bounds
			goto draw_off;
		if ((px - rx) > 1.0f)
			goto draw_off;


		py = ((m01*worldX) + (m11*worldY) + (m21*worldZ) + m31) * w;				// transf world Y
		ry = ((m01*radius) + (m11*radius) + (m21*worldZ) + m31) * w2;				// transf world radius Y
		
		if ((py + ry) < -1.0f)														// see if sphere "would be" out of bounds	
			goto draw_off;
		if ((py - ry) > 1.0f)
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
