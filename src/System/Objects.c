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

static void FlushObjectDeleteQueue(int queueID);
static void DisposeObjNodeMemory(ObjNode* node);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	OBJ_DEL_Q_SIZE	100
#define	OBJ_BUDGET		500


/**********************/
/*     VARIABLES      */
/**********************/

static ObjNode gObjNodeMemory[OBJ_BUDGET];
Pool* gObjNodePool = NULL;
static ObjNode gObjNodeTemplate;

											// OBJECT LIST
ObjNode		*gFirstNodePtr = nil;

ObjNode		*gCurrentNode,*gMostRecentlyAddedNode,*gNextNode;
int			gNumObjNodes = 0;

NewObjectDefinitionType	gNewObjectDefinition;

TQ3Point3D	gCoord;
TQ3Vector3D	gDelta;

// Double-buffered deletion queue: objects that get queued for deletion during
// frame N will be deleted at the end of frame N+1.
// This gives live objects a full frame to clean up references to dead objects.
static int			gNumObjsInDeleteQueue[2];
static ObjNode*		gObjectDeleteQueue[2][OBJ_DEL_Q_SIZE];
static int			gObjectDeleteQueueFlipFlop = 0;

Boolean		gDoAutoFade;
float		gAutoFadeStartDist;


//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT CREATION ------

/************************ INIT OBJECT MANAGER **********************/

void InitObjectManager(void)
{
		/* INIT LINKED LIST */

	gCurrentNode = nil;
	gFirstNodePtr = nil;									// no node yet
	gNumObjNodes = 0;

		/* INIT OBJECT POOL */

	memset(gObjNodeMemory, 0, sizeof(gObjNodeMemory));

	if (!gObjNodePool)
		gObjNodePool = Pool_New(OBJ_BUDGET);
	else
		Pool_Reset(gObjNodePool);

		/* MAKE OBJECT TEMPLATE */

	gObjNodeTemplate = (ObjNode)
	{
		.CType					= 0,						// must init ctype to something (INVALID_NODE_FLAG might be set from last delete)
		.Scale					= {1,1,1},
		.BoundingSphere			= {.origin={0,0,0}, .radius=40, .isEmpty=kQ3False},
		.EffectChannel			= -1,						// no effect channel yet
		.ParticleGroup			= -1,						// no particle group
		.SplineObjectIndex		= -1,						// no index yet
		.StatusBits				= STATUS_BIT_DETACHED,		// not attached to linked list yet
	};

	Render_SetDefaultModifiers(&gObjNodeTemplate.RenderModifiers);

		/* INIT NEW OBJ DEF */

	memset(&gNewObjectDefinition, 0, sizeof(NewObjectDefinitionType));
	gNewObjectDefinition.scale = 1;
}


/*********************** MAKE NEW OBJECT ******************/
//
// MAKE NEW OBJECT & RETURN PTR TO IT
//
// The linked list is sorted from smallest to largest!
//

ObjNode	*MakeNewObject(NewObjectDefinitionType *newObjDef)
{
	ObjNode	*newNodePtr = NULL;

		/* TRY TO GET AN OBJECT FROM THE POOL */

	int pooledIndex = Pool_AllocateIndex(gObjNodePool);
	if (pooledIndex >= 0)
	{
		newNodePtr = &gObjNodeMemory[pooledIndex];
	}
	else
	{
		// pool full, alloc new node on heap
		newNodePtr = (ObjNode*) AllocPtr(sizeof(ObjNode));
	}

		/* MAKE SURE WE GOT ONE */

	GAME_ASSERT(newNodePtr);

		/* MAKE SURE SCALE != 0 */

	float scale = newObjDef->scale;
	if (scale == 0.0f)
		scale = 0.0001f;

		/* INITIALIZE NEW NODE */

	*newNodePtr = gObjNodeTemplate;

	newNodePtr->Slot		= newObjDef->slot;
	newNodePtr->Type		= newObjDef->type;
	newNodePtr->Group		= newObjDef->group;
	newNodePtr->Genre		= newObjDef->genre;
	newNodePtr->StatusBits	= newObjDef->flags;

	newNodePtr->CType		= 0;						// must init ctype to something (INVALID_NODE_FLAG might be set from last delete)

	newNodePtr->Coord		= newObjDef->coord;
	newNodePtr->InitCoord	= newObjDef->coord;
	newNodePtr->OldCoord	= newObjDef->coord;

	newNodePtr->Scale.x		= scale;
	newNodePtr->Scale.y		= scale;
	newNodePtr->Scale.z		= scale;

	newNodePtr->Rot.x		= 0;
	newNodePtr->Rot.y		= newObjDef->rot;
	newNodePtr->Rot.z		= 0;

	if (newObjDef->flags & STATUS_BIT_ONSPLINE)
		newNodePtr->SplineMoveCall = newObjDef->moveCall;	// save spline move routine
	else
		newNodePtr->MoveCall = newObjDef->moveCall;			// save move routine

				/* INSERT NODE INTO LINKED LIST */

	newNodePtr->StatusBits |= STATUS_BIT_DETACHED;		// its not attached to linked list yet
	AttachObject(newNodePtr);

				/* CLEANUP */

	gMostRecentlyAddedNode = newNodePtr;					// remember this
	gNumObjNodes++;

	return(newNodePtr);
}

/************* MAKE NEW DISPLAY GROUP OBJECT *************/
//
// This is an ObjNode who's BaseGroup is a group, therefore these objects
// can be transformed (scale,rot,trans).
//

ObjNode *MakeNewDisplayGroupObject(NewObjectDefinitionType *newObjDef)
{
ObjNode	*newObj;
Byte	group,type;

	newObjDef->genre = DISPLAY_GROUP_GENRE;
	
	newObj = MakeNewObject(newObjDef);		
	if (newObj == nil)
		return(nil);

			/* MAKE BASE GROUP & ADD GEOMETRY TO IT */
	
	CreateBaseGroup(newObj);											// create group object
	group = newObjDef->group;											// get group #
	type = newObjDef->type;												// get type #
	
	GAME_ASSERT(type < gNumObjectsInGroupList[group]);					// see if illegal

	TQ3TriMeshFlatGroup* meshList = &gObjectGroupList[group][type];
	AttachGeometryToDisplayGroupObject(
			newObj, meshList->numMeshes, meshList->meshes,
			(newObjDef->flags & STATUS_BIT_CLONE) ? kAttachGeometry_CloneMeshes : 0);

	newObj->RenderModifiers.drawOrder = newObjDef->drawOrder;


			/* CALC RADIUS */
			
	newObj->BoundingSphere.origin.x = gObjectGroupRadiusList[group][type].origin.x * newObj->Scale.x;	
	newObj->BoundingSphere.origin.y = gObjectGroupRadiusList[group][type].origin.y * newObj->Scale.y;	
	newObj->BoundingSphere.origin.z = gObjectGroupRadiusList[group][type].origin.z * newObj->Scale.z;	
	newObj->BoundingSphere.radius = gObjectGroupRadiusList[group][type].radius * newObj->Scale.x;

	return(newObj);
}


/************* MAKE NEW CUSTOM DRAW OBJECT *************/

ObjNode *MakeNewCustomDrawObject(NewObjectDefinitionType *newObjDef, TQ3BoundingSphere *cullSphere, void drawFunc(ObjNode *))
{
ObjNode	*newObj;

	newObjDef->genre = CUSTOM_GENRE;
	
	newObj = MakeNewObject(newObjDef);		
	if (newObj == nil)
		return(nil);
			
	newObj->BoundingSphere = *cullSphere;	
	newObj->CustomDrawFunction = drawFunc;
	
	return(newObj);
}



/************************* ADD GEOMETRY TO DISPLAY GROUP OBJECT ***********************/
//
// Attaches a geometry object to the BaseGroup object. MakeNewDisplayGroupObject must have already been
// called which made the group & transforms.
//

void AttachGeometryToDisplayGroupObject(ObjNode* theNode, int numMeshes, TQ3TriMeshData** meshList, int flags)
{
	bool ownMeshes = flags & (kAttachGeometry_TransferMeshOwnership | kAttachGeometry_CloneMeshes);
	bool ownTextures = flags & kAttachGeometry_TransferTextureOwnership;

	for (int i = 0; i < numMeshes; i++)
	{
		int nodeMeshIndex = theNode->NumMeshes;

		theNode->NumMeshes++;
		GAME_ASSERT(theNode->NumMeshes <= MAX_DECOMPOSED_TRIMESHES);
		GAME_ASSERT(nodeMeshIndex < MAX_DECOMPOSED_TRIMESHES);

		if (flags & kAttachGeometry_CloneMeshes)
		{
			theNode->MeshList[nodeMeshIndex] = Q3TriMeshData_Duplicate(meshList[i]);
		}
		else
		{
			theNode->MeshList[nodeMeshIndex] = meshList[i];
		}

		theNode->OwnsMeshMemory[nodeMeshIndex] = ownMeshes;
		theNode->OwnsMeshTexture[nodeMeshIndex] = ownTextures;
	}
}



/***************** CREATE BASE GROUP **********************/
//
// The base group contains the base transform matrix plus any other objects you want to add into it.
// This routine creates the new group and then adds a transform matrix.
//
// The base is composed of BaseGroup & BaseTransformObject.
//

void CreateBaseGroup(ObjNode *theNode)
{
TQ3Matrix4x4			transMatrix,scaleMatrix,rotMatrix;

				/* CREATE THE GROUP OBJECT */

	theNode->NumMeshes = 0;

					/* SETUP BASE MATRIX */
			
	if ((theNode->Scale.x == 0) || (theNode->Scale.y == 0) || (theNode->Scale.z == 0))
		DoFatalAlert("CreateBaseGroup: A scale component == 0");
		
			
	Q3Matrix4x4_SetScale(&scaleMatrix, theNode->Scale.x, theNode->Scale.y,		// make scale matrix
							theNode->Scale.z);
			
	Q3Matrix4x4_SetRotate_XYZ(&rotMatrix, theNode->Rot.x, theNode->Rot.y,		// make rotation matrix
								 theNode->Rot.z);

	Q3Matrix4x4_SetTranslate(&transMatrix, theNode->Coord.x, theNode->Coord.y,	// make translate matrix
							 theNode->Coord.z);

	MatrixMultiplyFast(&scaleMatrix,											// mult scale & rot matrices
						 &rotMatrix,
						 &theNode->BaseTransformMatrix);

	MatrixMultiply(&theNode->BaseTransformMatrix,							// mult by trans matrix
						 &transMatrix,
						 &theNode->BaseTransformMatrix);
}



//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT PROCESSING ------


/*******************************  MOVE OBJECTS **************************/

void MoveObjects(void)
{
ObjNode		*thisNodePtr;

	if (gFirstNodePtr == nil)								// see if there are any objects
		return;

	thisNodePtr = gFirstNodePtr;
	
	do
	{
		gCurrentNode = thisNodePtr;							// set current object node
		gNextNode	 = thisNodePtr->NextNode;				// get next node now (cuz current node might get deleted)
			
	
		KeepOldCollisionBoxes(thisNodePtr);					// keep old box
		
			
				/* UPDATE ANIMATION */
				
		if (thisNodePtr->StatusBits & STATUS_BIT_ANIM)
			UpdateSkeletonAnimation(thisNodePtr);


					/* NEXT TRY TO MOVE IT */

		if (thisNodePtr->MoveCall != nil)
		{
			thisNodePtr->MoveCall(thisNodePtr);				// call object's move routine
		}
		thisNodePtr = gNextNode;							// next node
	}
	while (thisNodePtr != nil);



			/* CALL SOUND MAINTENANCE HERE FOR CONVENIENCE */
			
	DoSoundMaintenance();
	
			/* FLUSH THE DELETE QUEUE */

	gObjectDeleteQueueFlipFlop = !gObjectDeleteQueueFlipFlop;
	FlushObjectDeleteQueue(gObjectDeleteQueueFlipFlop);
}




/**************************** DRAW OBJECTS ***************************/

void DrawObjects(const QD3DSetupOutputType *setupInfo)
{
ObjNode		*theNode;
unsigned long	statusBits;
float			cameraX, cameraZ;

	if (gFirstNodePtr == nil)									// see if there are any objects
		return;

				/* FIRST DO OUR CULLING */
				
	CheckAllObjectsInConeOfVision();
	
	theNode = gFirstNodePtr;

	
			/* GET CAMERA COORDS */
			
	cameraX = setupInfo->currentCameraCoords.x;
	cameraZ = setupInfo->currentCameraCoords.z;
	
			/***********************/
			/* MAIN NODE TASK LOOP */
			/***********************/			
	do
	{
		statusBits = theNode->StatusBits;						// get obj's status bits

		theNode->RenderModifiers.statusBits = statusBits;		// copy status bits to render mods

		if (theNode->CType == INVALID_NODE_FLAG)				// see if already deleted
			goto next;

		if (statusBits & (STATUS_BIT_ISCULLED | STATUS_BIT_HIDDEN))
			goto next;


			/******************/
			/* CHECK AUTOFADE */
			/******************/

		if (gDoAutoFade && (statusBits & STATUS_BIT_AUTOFADE))		// see if this level has autofade
		{
			float dist = CalcQuickDistance(cameraX, cameraZ, theNode->Coord.x, theNode->Coord.z);	// see if in fade zone
			if (dist >= gAutoFadeStartDist)
			{
				float factor = 1.0f - (dist - gAutoFadeStartDist) / AUTO_FADE_RANGE;	// calc xparency %
				if (factor <= 0.0f)		// too far; fully faded
					goto next;

				theNode->RenderModifiers.autoFadeFactor = factor;
			}
			else
			{
				theNode->RenderModifiers.autoFadeFactor = 1.0f;
			}
		}
		else
		{
			theNode->RenderModifiers.autoFadeFactor = 1.0f;
		}

			/***********************/
			/* SUBMIT THE GEOMETRY */
			/***********************/

		switch(theNode->Genre)
		{
			case	SKELETON_GENRE:
					UpdateSkinnedGeometry(theNode);													// update skeleton geometry
					Render_SubmitMeshList(															// submit each trimesh of it
							theNode->NumMeshes,
							theNode->MeshList,
							nil,		// Don't mult matrix with BaseTransformMatrix -- skeleton code already does it
							&theNode->RenderModifiers,
							&theNode->Coord);
					break;
			
			case	DISPLAY_GROUP_GENRE:
					Render_SubmitMeshList(
							theNode->NumMeshes,
							theNode->MeshList,
							&theNode->BaseTransformMatrix,
							&theNode->RenderModifiers,
							&theNode->Coord);
					break;
					
			case	CUSTOM_GENRE:
					if (theNode->CustomDrawFunction)
					{
						theNode->CustomDrawFunction(theNode);
					}
					break;
		}


			/* NEXT NODE */		
next:
		theNode = (ObjNode *)theNode->NextNode;
	}while (theNode != nil);
}


/********************* MOVE STATIC OBJECT **********************/

void MoveStaticObject(ObjNode *theNode)
{		

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	UpdateShadow(theNode);										// prime it
}



//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT DELETING ------


/******************** DELETE ALL OBJECTS ********************/

void DeleteAllObjects(void)
{
	while (gFirstNodePtr != nil)
		DeleteObject(gFirstNodePtr);

	FlushObjectDeleteQueue(0);
	FlushObjectDeleteQueue(1);
}


/************************ DELETE OBJECT ****************/

void DeleteObject(ObjNode	*theNode)
{
	if (theNode == nil)								// see if passed a bogus node
		return;

	GAME_ASSERT_MESSAGE(							// see if already deleted
			theNode->CType != INVALID_NODE_FLAG,
			"Attempted to Double Delete an Object.  Object was already deleted!");

			/* RECURSIVE DELETE OF CHAIN NODE & SHADOW NODE */
			//
			// should do these first so that base node will have appropriate nextnode ptr
			// since it's going to be used next pass thru the moveobjects loop.  This
			// assumes that all chained nodes are later in list.
			//

	if (theNode->ChainNode)
	{
		DeleteObject(theNode->ChainNode);
		theNode->ChainNode = nil;
	}

	if (theNode->ShadowNode)
	{
		DeleteObject(theNode->ShadowNode);
		theNode->ShadowNode = nil;
	}


			/* SEE IF NEED TO FREE UP SPECIAL MEMORY */
			
	if (theNode->Genre == SKELETON_GENRE)
	{
		FreeSkeletonBaseData(theNode->Skeleton);	// purge all alloced memory for skeleton data
		theNode->Skeleton = nil;
	}
	
	if (theNode->CollisionBoxes != nil)				// free collision box memory
	{
		DisposePtr((Ptr)theNode->CollisionBoxes);
		theNode->CollisionBoxes = nil;
		theNode->NumCollisionBoxes = 0;
	}

	if (theNode->OldCollisionBoxes != nil)			// free old collision box memory
	{
		DisposePtr((Ptr)theNode->OldCollisionBoxes);
		theNode->OldCollisionBoxes = nil;
	}


			/* SEE IF STOP SOUND CHANNEL */

	StopObjectStreamEffect(theNode);


		/* SEE IF NEED TO DEREFERENCE A QD3D OBJECT */

	for (int i = 0; i < theNode->NumMeshes; i++)
	{
		// If the node has ownership of this mesh's OpenGL texture name, delete it
		if (theNode->MeshList[i]->glTextureName && theNode->OwnsMeshTexture[i])
		{
			glDeleteTextures(1, &theNode->MeshList[i]->glTextureName);
			theNode->MeshList[i]->glTextureName = 0;
		}

		// If the node has ownership of this mesh's memory, dispose of it
		if (theNode->OwnsMeshMemory[i])
		{
			Q3TriMeshData_Dispose(theNode->MeshList[i]);
		}

		theNode->MeshList[i] = nil;
		theNode->OwnsMeshMemory[i] = false;
	}
	theNode->NumMeshes = 0;

			/* REMOVE NODE FROM LINKED LIST */

	DetachObject(theNode);


			/* SEE IF MARK AS NOT-IN-USE IN ITEM LIST */
			
	if (theNode->TerrainItemPtr)
		theNode->TerrainItemPtr->flags &= ~ITEM_FLAGS_INUSE;		// clear the "in use" flag
		
		
		/* OR, IF ITS A SPLINE ITEM, THEN UPDATE SPLINE OBJECT LIST */
		
	if (theNode->StatusBits & STATUS_BIT_ONSPLINE)
	{
		RemoveFromSplineObjectList(theNode);
	}


			/* DELETE THE NODE BY ADDING TO DELETE QUEUE */

	theNode->CType = INVALID_NODE_FLAG;							// INVALID_NODE_FLAG indicates its deleted


	const int qid = gObjectDeleteQueueFlipFlop;
	if (gNumObjsInDeleteQueue[qid] >= OBJ_DEL_Q_SIZE)
	{
		// We're out of room in the delete queue. Just delete the node immediately,
		// but that might cause the game to become unstable (unless we're here from DeleteAllNodes).
		DisposeObjNodeMemory(theNode);
	}
	else
	{
		// Defer actual deletion of this node to the next frame.
		gObjectDeleteQueue[qid][gNumObjsInDeleteQueue[qid]++] = theNode;
	}
}


/****************** DETACH OBJECT ***************************/
//
// Simply detaches the objnode from the linked list, patches the links
// and keeps the original objnode intact.
//

void DetachObject(ObjNode *theNode)
{
	if (theNode == nil)
		return;

	if (theNode->StatusBits & STATUS_BIT_DETACHED)	// make sure not already detached
		return;

	if (theNode == gNextNode)						// if its the next node to be moved, then fix things
		gNextNode = theNode->NextNode;

	if (theNode->PrevNode == nil)					// special case 1st node
	{
		gFirstNodePtr = theNode->NextNode;	
		if (gFirstNodePtr)
			gFirstNodePtr->PrevNode = nil;
	}
	else
	if (theNode->NextNode == nil)					// special case last node
	{
		theNode->PrevNode->NextNode = nil;
	}
	else											// generic middle deletion
	{
		theNode->PrevNode->NextNode = theNode->NextNode;
		theNode->NextNode->PrevNode = theNode->PrevNode;
	}
	
	theNode->PrevNode = nil;						// seal links on original node
	theNode->NextNode = nil;
	
	theNode->StatusBits |= STATUS_BIT_DETACHED;	
}


/****************** ATTACH OBJECT ***************************/

void AttachObject(ObjNode *theNode)
{
short	slot;

	if (theNode == nil)
		return;

	if (!(theNode->StatusBits & STATUS_BIT_DETACHED))		// see if already attached
		return;


	slot = theNode->Slot;

	if (gFirstNodePtr == nil)						// special case only entry
	{
		gFirstNodePtr = theNode;
		theNode->PrevNode = nil;
		theNode->NextNode = nil;
	}
	
			/* INSERT AS FIRST NODE */
	else
	if (slot < gFirstNodePtr->Slot)					
	{
		theNode->PrevNode = nil;					// no prev
		theNode->NextNode = gFirstNodePtr; 			// next pts to old 1st
		gFirstNodePtr->PrevNode = theNode; 			// old pts to new 1st
		gFirstNodePtr = theNode;
	}
		/* SCAN FOR INSERTION PLACE */
	else
	{
		ObjNode	 *reNodePtr, *scanNodePtr;
		
		reNodePtr = gFirstNodePtr;
		scanNodePtr = gFirstNodePtr->NextNode;		// start scanning for insertion slot on 2nd node
			
		while (scanNodePtr != nil)
		{
			if (slot < scanNodePtr->Slot)					// INSERT IN MIDDLE HERE
			{
				theNode->NextNode = scanNodePtr;
				theNode->PrevNode = reNodePtr;
				reNodePtr->NextNode = theNode;
				scanNodePtr->PrevNode = theNode;			
				goto out;
			}
			reNodePtr = scanNodePtr;
			scanNodePtr = scanNodePtr->NextNode;			// try next node
		} 
	
		theNode->NextNode = nil;							// TAG TO END
		theNode->PrevNode = reNodePtr;
		reNodePtr->NextNode = theNode;		
	}
out:	
	
	
	theNode->StatusBits &= ~STATUS_BIT_DETACHED;	
}



/***************** DISPOSE OBJECT MEMORY ****************/

static void DisposeObjNodeMemory(ObjNode* node)
{
	GAME_ASSERT(node != NULL);

	ptrdiff_t poolIndex = node - gObjNodeMemory;

	if (poolIndex >= 0 && poolIndex < OBJ_BUDGET)
	{
		// node is pooled, put back into pool
		GAME_ASSERT_MESSAGE(Pool_IsUsed(gObjNodePool, poolIndex), "double-free on pooled node!");
		Pool_ReleaseIndex(gObjNodePool, (int) poolIndex);
	}
	else
	{
		// node was allocated on heap
		DisposePtr((Ptr) node);
	}

	gNumObjNodes--;
}


/***************** FLUSH OBJECT DELETE QUEUE ****************/

static void FlushObjectDeleteQueue(int qid)
{
	int num = gNumObjsInDeleteQueue[qid];

	for (int i = 0; i < num; i++)
	{
		if (gObjectDeleteQueue[qid][i])
		{
			DisposeObjNodeMemory(gObjectDeleteQueue[qid][i]);
		}
	}

	gNumObjsInDeleteQueue[qid] = 0;
}




//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT INFORMATION ------


/********************** GET OBJECT INFO *********************/

void GetObjectInfo(ObjNode *theNode)
{
	gCoord = theNode->Coord;
	gDelta = theNode->Delta;
}


/********************** UPDATE OBJECT *********************/

void UpdateObject(ObjNode *theNode)
{
	if (theNode->CType == INVALID_NODE_FLAG)		// see if already deleted
		return;
		
	theNode->Coord = gCoord;
	theNode->Delta = gDelta;
	UpdateObjectTransforms(theNode);
	if (theNode->CollisionBoxes)
		CalcObjectBoxFromNode(theNode);


		/* UPDATE ANY SHADOWS */
				
	UpdateShadow(theNode);		
}



/****************** UPDATE OBJECT TRANSFORMS *****************/
//
// This updates the skeleton object's base translate & rotate transforms
//

void UpdateObjectTransforms(ObjNode *theNode)
{
TQ3Matrix4x4	m,m2;

	if (theNode->CType == INVALID_NODE_FLAG)		// see if already deleted
		return;

				/********************/
				/* SET SCALE MATRIX */
				/********************/

	Q3Matrix4x4_SetScale(&m, theNode->Scale.x,	theNode->Scale.y, theNode->Scale.z);

	
			/*****************************/
			/* NOW ROTATE & TRANSLATE IT */
			/*****************************/
	
				/* DO XZY ROTATION */
						
	if (theNode->StatusBits & STATUS_BIT_ROTXZY)
	{
		TQ3Matrix4x4	mx,my,mz,mxz;
		
		Q3Matrix4x4_SetRotate_X(&mx, theNode->Rot.x);	
		Q3Matrix4x4_SetRotate_Y(&my, theNode->Rot.y);	
		Q3Matrix4x4_SetRotate_Z(&mz, theNode->Rot.z);	
	
		MatrixMultiplyFast(&mx,&mz, &mxz);
		MatrixMultiplyFast(&mxz,&my, &m2);
	}
				/* STANDARD XYZ ROTATION */
	else
	{
		Q3Matrix4x4_SetRotate_XYZ(&m2, theNode->Rot.x, theNode->Rot.y, theNode->Rot.z);
	}
	
	m2.value[3][0] = theNode->Coord.x;
	m2.value[3][1] = theNode->Coord.y;
	m2.value[3][2] = theNode->Coord.z;
	
	MatrixMultiplyFast(&m,&m2, &theNode->BaseTransformMatrix);
}




/********************* MAKE OBJECT TRANSPARENT ***********************/
//
// Puts a transparency attribute object in the base group.
//
// INPUT: transPecent = 0..1.0   0 = totally trans, 1.0 = totally opaque

void MakeObjectTransparent(ObjNode *theNode, float transPercent)
{
	theNode->RenderModifiers.diffuseColor.a = transPercent;
}


#pragma mark -


/************************ DRAW COLLISION BOXES ****************************/

void DrawCollisionBoxes(const ObjNode* theNode)
{
int	n,i;
CollisionBoxType	*c;
float			left,right,top,bottom,front,back;

	n = theNode->NumCollisionBoxes;							// get # collision boxes	
	c = theNode->CollisionBoxes;							// pt to array
	if (c == nil)
		return;

	for (i = 0; i < n; i++)
	{
		left 	= c[i].left;
		right 	= c[i].right;
		top 	= c[i].top;
		bottom 	= c[i].bottom;
		front 	= c[i].front;
		back 	= c[i].back;


			/* DRAW TOP */

		glBegin(GL_LINE_STRIP);
		glVertex3f(left,		top,		back);
		glVertex3f(left,		top,		front);
		glVertex3f(right,		top,		front);
		glVertex3f(right,		top,		back);
		glVertex3f(left,		top,		back);
		glEnd();

			/* DRAW BOTTOM */

		glBegin(GL_LINE_STRIP);
		glVertex3f(left,		bottom,		back);
		glVertex3f(left,		bottom,		front);
		glVertex3f(right,		bottom,		front);
		glVertex3f(right,		bottom,		back);
		glVertex3f(left,		bottom,		back);
		glEnd();

			/* DRAW LEFT & RIGHT */

		glBegin(GL_LINES);
		glVertex3f(left,		top,		back);
		glVertex3f(left,		bottom,		back);
		glVertex3f(left,		top,		front);
		glVertex3f(left,		bottom,		front);
		glVertex3f(right,		top,		back);
		glVertex3f(right,		bottom,		back);
		glVertex3f(right,		top,		front);
		glVertex3f(right,		bottom,		front);
		glEnd();
	}
}
