/*********************************/
/*    OBJECT MANAGER 		     */
/* (c)1993-1997 Pangea Software  */
/* By Brian Greenstone      	 */
/*********************************/


/***************/
/* EXTERNALS   */
/***************/

#include "3dmath.h"

extern	TQ3TriMeshFlatGroup		gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TQ3BoundingSphere		gObjectGroupRadiusList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TQ3Matrix4x4	gCameraWorldToViewMatrix,gCameraViewToFrustumMatrix;
extern	short		gNumObjectsInGroupList[MAX_3DMF_GROUPS];
extern	float		gFramesPerSecondFrac;
extern	ObjNode		*gPlayerObj;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	TQ3TriMeshData	**gLocalTriMeshesOfSkelType;
extern	Boolean		gShowDebug;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void FlushObjectDeleteQueue(void);
static void DrawCollisionBoxes(ObjNode *theNode, TQ3ViewObject view);
static void DrawBoundingSphere(ObjNode *theNode, TQ3ViewObject view);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	OBJ_DEL_Q_SIZE	100


/**********************/
/*     VARIABLES      */
/**********************/

											// OBJECT LIST
ObjNode		*gFirstNodePtr = nil;
					
ObjNode		*gCurrentNode,*gMostRecentlyAddedNode,*gNextNode;
			
										
NewObjectDefinitionType	gNewObjectDefinition;

TQ3Point3D	gCoord;
TQ3Vector3D	gDelta;

long		gNumObjsInDeleteQueue = 0;
ObjNode		*gObjectDeleteQueue[OBJ_DEL_Q_SIZE];

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
	
					/* CLEAR ENTIRE OBJECT LIST */
		
	gFirstNodePtr = nil;									// no node yet
}


/*********************** MAKE NEW OBJECT ******************/
//
// MAKE NEW OBJECT & RETURN PTR TO IT
//
// The linked list is sorted from smallest to largest!
//

ObjNode	*MakeNewObject(NewObjectDefinitionType *newObjDef)
{
ObjNode	*newNodePtr;

				/* MAKE SURE SCALE != 0 */

	float scale = newObjDef->scale;
	if (scale == 0.0f)
		scale = 0.0001f;

				/* INITIALIZE NEW NODE */
	
	newNodePtr = (ObjNode*) NewPtrClear(sizeof(ObjNode));	// source port change: use NewPtrClear so all fields start at 0
	GAME_ASSERT(newNodePtr);

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

	newNodePtr->BoundingSphere.radius = 40;				// set default bounding sphere at offset 0,0,0

	newNodePtr->EffectChannel = -1;						// no streaming sound effect
	newNodePtr->ParticleGroup = -1;						// no particle group
	newNodePtr->SplineObjectIndex = -1;					// no index yet

	if (newObjDef->flags & STATUS_BIT_ONSPLINE)
		newNodePtr->SplineMoveCall = newObjDef->moveCall;	// save spline move routine
	else
		newNodePtr->MoveCall = newObjDef->moveCall;			// save move routine

				/* INSERT NODE INTO LINKED LIST */
				
	newNodePtr->StatusBits |= STATUS_BIT_DETACHED;		// its not attached to linked list yet
	AttachObject(newNodePtr);

				/* CLEANUP */
				
	gMostRecentlyAddedNode = newNodePtr;					// remember this
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

	if (newObjDef->flags & STATUS_BIT_CLONE)
	{
		DoFatalAlert("TODO NOQUESA: clone DGO\n");
		//AttachGeometryToDisplayGroupObject(newObj, Q3Object_Duplicate(gObjectGroupList[group][type]));
	}
	else
	{
		TQ3TriMeshFlatGroup* meshList = &gObjectGroupList[group][type];
		AttachGeometryToDisplayGroupObject(newObj, meshList->numMeshes, meshList->meshes);
	}

			/* CALC RADIUS */
			
	newObj->BoundingSphere.origin.x = gObjectGroupRadiusList[group][type].origin.x * newObj->Scale.x;	
	newObj->BoundingSphere.origin.y = gObjectGroupRadiusList[group][type].origin.y * newObj->Scale.y;	
	newObj->BoundingSphere.origin.z = gObjectGroupRadiusList[group][type].origin.z * newObj->Scale.z;	
	newObj->BoundingSphere.radius = gObjectGroupRadiusList[group][type].radius * newObj->Scale.x;

	return(newObj);
}


/************* MAKE NEW CUSTOM DRAW OBJECT *************/

ObjNode *MakeNewCustomDrawObject(NewObjectDefinitionType *newObjDef, TQ3BoundingSphere *cullSphere,
						 void drawFunc(ObjNode *, TQ3ViewObject))
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


/******************* RESET DISPLAY GROUP OBJECT *********************/
//
// If the ObjNode's "Type" field has changed, call this to dispose of
// the old BaseGroup and create a new one with the correct model attached.
//

void ResetDisplayGroupObject(ObjNode *theNode)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
	DisposeObjectBaseGroup(theNode);									// dispose of old group
	CreateBaseGroup(theNode);											// create new group object

	GAME_ASSERT(theNode->Type < gNumObjectsInGroupList[theNode->Group]);	// see if illegal

	AttachGeometryToDisplayGroupObject(theNode,gObjectGroupList[theNode->Group][theNode->Type]);	// attach geometry to group
#endif
}



/************************* ADD GEOMETRY TO DISPLAY GROUP OBJECT ***********************/
//
// Attaches a geometry object to the BaseGroup object. MakeNewDisplayGroupObject must have already been
// called which made the group & transforms.
//

void AttachGeometryToDisplayGroupObject(ObjNode* theNode, int numMeshes, TQ3TriMeshData** meshList)
{
	for (int i = 0; i < numMeshes; i++)
	{
		int nodeMeshIndex = theNode->NumMeshes;

		theNode->NumMeshes++;
		GAME_ASSERT(theNode->NumMeshes <= MAX_DECOMPOSED_TRIMESHES);

		theNode->MeshList[nodeMeshIndex] = meshList[i];
		theNode->OwnsMeshMemory[nodeMeshIndex] = false;
		theNode->OwnsMeshTexture[nodeMeshIndex] = false;
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

#if 0	// NOQUESA
					/* CREATE A MATRIX XFORM */

	transObject = (TQ3TransformObject)Q3MatrixTransform_New(&theNode->BaseTransformMatrix);			// make matrix xform object
	GAME_ASSERT(transObject);

	myGroupPosition = (TQ3GroupPosition)Q3Group_AddObject(theNode->BaseGroup, transObject);		// add to base group
	GAME_ASSERT(myGroupPosition);

	theNode->BaseTransformObject = transObject;									// keep extra LEGAL ref (remember to dispose later)
#endif
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
					
		if ((!(thisNodePtr->StatusBits & STATUS_BIT_NOMOVE)) &&	(thisNodePtr->MoveCall != nil))
		{
			thisNodePtr->MoveCall(thisNodePtr);				// call object's move routine
		}
		thisNodePtr = gNextNode;							// next node
	}
	while (thisNodePtr != nil);



			/* CALL SOUND MAINTENANCE HERE FOR CONVENIENCE */
			
	DoSoundMaintenance();
	
			/* FLUSH THE DELETE QUEUE */
			
	FlushObjectDeleteQueue();
}




/**************************** DRAW OBJECTS ***************************/

void DrawObjects(const QD3DSetupOutputType *setupInfo)
{
ObjNode		*theNode;
TQ3Status	myStatus;
short		i,numTriMeshes;
unsigned long	statusBits;
Boolean			autoFade = false;
Boolean			noCache = false;
Boolean			useNullShader = false;
Boolean			noZWrites = false;
Boolean			noFog = false;
Boolean			glow = false;
float			cameraX, cameraZ;
static const TQ3ColorRGB	white = {1,1,1};	
short			skelType;		

	if (gFirstNodePtr == nil)									// see if there are any objects
		return;

				/* FIRST DO OUR CULLING */
				
	CheckAllObjectsInConeOfVision();
	
	theNode = gFirstNodePtr;

	
			/* GET CAMERA COORDS */
			
	cameraX = setupInfo->currentCameraCoords.x;
	cameraZ = setupInfo->currentCameraCoords.z;
	
	QD3D_SetMultisampling(true);
	
			/***********************/
			/* MAIN NODE TASK LOOP */
			/***********************/			
	do
	{
		statusBits = theNode->StatusBits;						// get obj's status bits

		printf("TODO NOQUESA: Restore objnode culling!!!\n");
#if 0	// NOQUESA
		if (statusBits & STATUS_BIT_ISCULLED)					// see if is culled
			goto next;		
		
		if (statusBits & STATUS_BIT_HIDDEN)						// see if is hidden
			goto next;		
#endif

		if (theNode->CType == INVALID_NODE_FLAG)				// see if already deleted
			goto next;		

//		if (statusBits & STATUS_BIT_REFLECTIONMAP)				// dont draw here if this is reflection mapped
//			goto next;

			
				/**************************/
				/* CHECK TRIANGLE CACHING */
				/**************************/
				
		if (statusBits & STATUS_BIT_NOTRICACHE)					// see if disable caching
		{
			if (!noCache)										// only disable if currently on
			{
				QD3D_SetTriangleCacheMode(false);
				noCache = true;
			}
		}
		else
		if (noCache)											// if caching disabled, reenable it
		{
			QD3D_SetTriangleCacheMode(true);
			noCache = false;
		}

		
			/******************/
			/* CHECK AUTOFADE */
			/******************/
			
		if (gAutoFadeStartDist != 0.0f)							// see if this level has autofade
		{
			printf("TODO NOQUESA: submit autofade xparency\n");
#if 0	// NOQUESA
			if (statusBits & STATUS_BIT_AUTOFADE)
			{
				TQ3ColorRGB	xcolor;			
				float		dist;
				
				dist = CalcQuickDistance(cameraX, cameraZ, theNode->Coord.x, theNode->Coord.z);			// see if in fade zone
				if (dist < gAutoFadeStartDist)
					goto no_auto_fade;
				
				dist -= gAutoFadeStartDist;							// calc xparency %
				dist = 1.0f - (dist * (1.0f/AUTO_FADE_RANGE));				
				if (dist < 0.0f)
					goto next;
					
				xcolor.r = xcolor.g = xcolor.b = dist;
				
				Q3Attribute_Submit(kQ3AttributeTypeTransparencyColor, &xcolor, view);
				autoFade = true;
			}
			else
			{
	no_auto_fade:			
				if (autoFade)
				{
					Q3Attribute_Submit(kQ3AttributeTypeTransparencyColor, &white, view);			
					autoFade = false;
				}
			}
#endif
		}
					
		
			/*********************/
			/* CHECK NULL SHADER */
			/*********************/

/* NOQUESA: rendermods should be enough
		if (statusBits & STATUS_BIT_NULLSHADER)
		{
			if (!useNullShader)
			{
				Q3Shader_Submit(setupInfo->nullShaderObject, view);
				useNullShader = true;
			}
		}
		else
		if (useNullShader)
		{
			useNullShader = false;
			Q3Shader_Submit(setupInfo->shaderObject, view);
		}
*/
		
			/*********************/
			/* CHECK NO Z-WRITES */
			/*********************/

/* NOQUESA: rendermods should be enough
		if (statusBits & STATUS_BIT_NOZWRITE)
		{
			if (!noZWrites)
			{
				QD3D_SetZWrite(false);
				noZWrites = true;
			}
		}
		else
		if (noZWrites)
		{
			noZWrites = false;
			QD3D_SetZWrite(true);
		}
*/
	
			/****************/
			/* CHECK NO FOG */
			/****************/

/* NOQUESA: rendermods should be enough
		if (statusBits & STATUS_BIT_NOFOG)
		{
			if (!noFog)
			{
				QD3D_DisableFog(setupInfo);
				noFog = true;
			}
		}
		else
		if (noFog)
		{
			noFog = false;
			QD3D_ReEnableFog(setupInfo);
		}
*/
				
			/********************/
			/* CHECK GLOW BLEND */
			/********************/

		if (statusBits & STATUS_BIT_GLOW) printf("TODO NOQUESA: additive blending!\n");
/* NOQUESA: rendermods should be enough
		if (statusBits & STATUS_BIT_GLOW)
		{
			if (!glow)
			{
				QD3D_SetAdditiveBlending(true);
				glow = true;
			}
		}
		else
		if (glow)
		{
			glow = false;
			QD3D_SetAdditiveBlending(false);
		}
*/
	
		
			/************************/
			/* SHOW COLLISION BOXES */
			/************************/
			
		if (gShowDebug)
		{
			printf("TODO NOQUESA: gShowDebug\n");
			/*
			DrawCollisionBoxes(theNode,view);
			DrawBoundingSphere(theNode, view);
			 */
		}

				
			/***********************/
			/* SUBMIT THE GEOMETRY */
			/***********************/
						
		switch(theNode->Genre)
		{
			case	SKELETON_GENRE:
					UpdateSkinnedGeometry(theNode);													// update skeleton geometry
					printf("TODO NOQUESA: submit skeleton geometry\n");
#if 0	// NOQUESA
					numTriMeshes = theNode->Skeleton->skeletonDefinition->numDecomposedTriMeshes;
					skelType = theNode->Type;
					for (i = 0; i < numTriMeshes; i++)												// submit each trimesh of it
						Q3TriMesh_Submit(&gLocalTriMeshesOfSkelType[skelType][i], view);
#endif
					break;
			
			case	DISPLAY_GROUP_GENRE:
					Render_SubmitMeshList(
							theNode->NumMeshes,
							theNode->MeshList,
							&theNode->BaseTransformMatrix,
							&theNode->RenderModifiers,
							&theNode->Coord);
					/*
					if (theNode->BaseGroup)
					{
						myStatus = Q3Object_Submit(theNode->BaseGroup, view);
						GAME_ASSERT(myStatus);
					}
					*/
					break;
					
			case	CUSTOM_GENRE:
					if (theNode->CustomDrawFunction)
					{
						printf("TODO NOQUESA: CustomDrawFunction!\n");
						//theNode->CustomDrawFunction(theNode, view);
					}
					break;
		}


			/* NEXT NODE */		
next:
		theNode = (ObjNode *)theNode->NextNode;
	}while (theNode != nil);


				/*****************************/
				/* RESET SETTINGS TO DEFAULT */
				/*****************************/

#if 0	// TODO NOQUESA
	if (autoFade)
		Q3Attribute_Submit(kQ3AttributeTypeTransparencyColor, &white, view);			

	if (noCache)												// if caching disabled, reenable it
		QD3D_SetTriangleCacheMode(true);

	if (useNullShader)
		Q3Shader_Submit(setupInfo->shaderObject, view);

	if (noZWrites)
		QD3D_SetZWrite(true);

	if (noFog)
		QD3D_ReEnableFog(setupInfo);

	if (glow)
		QD3D_SetAdditiveBlending(false);

	SubmitReflectionMapQueue(setupInfo);						// draw anything in the reflection map queue
#endif
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
		
	FlushObjectDeleteQueue();
#if 1	//NOQUESA
	printf("TODO NOQUESA: InitReflectionMapQueue?\n");
#else
	InitReflectionMapQueue();						// also purge data from here
#endif
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
		DeleteObject(theNode->ChainNode);

	if (theNode->ShadowNode)
		DeleteObject(theNode->ShadowNode);


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
	}

//	if (theNode->CollisionTriangles)				// free collision triangle memory
//		DisposeCollisionTriangleMemory(theNode);

	
			/* SEE IF STOP SOUND CHANNEL */
			
	StopObjectStreamEffect(theNode);


		/* SEE IF NEED TO DEREFERENCE A QD3D OBJECT */
	
	DisposeObjectBaseGroup(theNode);		


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


	gObjectDeleteQueue[gNumObjsInDeleteQueue++] = theNode;
	if (gNumObjsInDeleteQueue >= OBJ_DEL_Q_SIZE)
		FlushObjectDeleteQueue();
	
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


/***************** FLUSH OBJECT DELETE QUEUE ****************/

static void FlushObjectDeleteQueue(void)
{
long	i,num;

	num = gNumObjsInDeleteQueue;
	
	for (i = 0; i < num; i++)
		DisposePtr((Ptr)gObjectDeleteQueue[i]);					

	gNumObjsInDeleteQueue = 0;
}


/****************** DISPOSE OBJECT BASE GROUP **********************/

void DisposeObjectBaseGroup(ObjNode *theNode)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
TQ3Status	status;

	if (theNode->BaseGroup != nil)
	{
		status = Q3Object_Dispose(theNode->BaseGroup);
		GAME_ASSERT(status);

		theNode->BaseGroup = nil;
	}
	
	if (theNode->BaseTransformObject != nil)							// also nuke extra ref to transform object
	{
		Q3Object_Dispose(theNode->BaseTransformObject);
		theNode->BaseTransformObject = nil;
	}
#endif
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


				/* UPDATE TRANSFORM OBJECT */
				
	SetObjectTransformMatrix(theNode);
}


/***************** SET OBJECT TRANSFORM MATRIX *******************/
//
// This call simply resets the base transform object so that it uses the latest
// base transform matrix
//

void SetObjectTransformMatrix(ObjNode *theNode)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
TQ3Status 				error;

	if (theNode->CType == INVALID_NODE_FLAG)		// see if invalid
		return;
		
	if (theNode->BaseTransformObject)				// see if this has a trans obj
	{
		error = Q3MatrixTransform_Set(theNode->BaseTransformObject,&theNode->BaseTransformMatrix);
		GAME_ASSERT(error);
	}
#endif
}

/********************* MAKE OBJECT KEEP BACKFACES ***********************/
//
// Puts a backfacing style object in the base group.
//

void MakeObjectKeepBackfaces(ObjNode *theNode)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
//TQ3Status			status;

	GAME_ASSERT(theNode->BaseGroup);

//	status = Q3Group_GetFirstPosition(theNode->BaseGroup, &position);
//	if ((status == kQ3Failure) || (position == nil))
//		DoFatalAlert("MakeObjectKeepBackfaces: Q3Group_GetFirstPosition failed!");
	
	TQ3GroupPosition pos = Q3Group_AddObject(theNode->BaseGroup, gKeepBackfaceStyleObject);
	GAME_ASSERT(pos);
#endif
}




/********************* MAKE OBJECT TRANSPARENT ***********************/
//
// Puts a transparency attribute object in the base group.
//
// INPUT: transPecent = 0..1.0   0 = totally trans, 1.0 = totally opaque

void MakeObjectTransparent(ObjNode *theNode, float transPercent)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
TQ3GroupPosition	position;
TQ3Status			status;
//TQ3ObjectType		oType;
TQ3Object			attrib;
TQ3ColorRGB			transColor;
TQ3AttributeType	attribType;

	GAME_ASSERT(theNode->BaseGroup);

//	oType = Q3Group_GetType(theNode->BaseGroup);
//	if (oType == kQ3ObjectTypeInvalid)
//		DoFatalAlert("MakeObjectTransparent: BaseGroup is not a Group Object!");


			/* GET CURRENT ATTRIBUTE OBJECT OR MAKE NEW ONE */

	status = Q3Group_GetFirstPositionOfType(theNode->BaseGroup, kQ3SharedTypeSet, &position);	// see if attribute object in the group
	if (position != nil)																		// YES
	{
		status = Q3Group_GetPositionObject(theNode->BaseGroup, position, &attrib);				// get the attribute object
		GAME_ASSERT(status);

		if (transPercent >= 1.0f)															// if totally opaque then remove this attrib
		{
			Q3AttributeSet_Clear(attrib, kQ3AttributeTypeTransparencyColor);				// remove this attrib type from attrib
			
			attribType = kQ3AttributeTypeNone;
			Q3AttributeSet_GetNextAttributeType(attrib, &attribType);						// if nothing in attrib set then remove attrib obj
			if (attribType == kQ3AttributeTypeNone)
			{
				TQ3Object	removedObj;
				
				removedObj = (TQ3Object)Q3Group_RemovePosition(theNode->BaseGroup, position);			// remove attrib from group		
				if (removedObj)
					Q3Object_Dispose(removedObj);											// now throw it out
			}
			goto bye;
		}		
	}
	
					/* NO ATTRIB OBJ IN GROUP, SO MAKE NEW ATTRIB SET */
	else
	{
		if (transPercent >= 1.0)															// if totally opaque then dont do it
			return;

		attrib = Q3AttributeSet_New();														// make new attrib set
		GAME_ASSERT(attrib);
			
				/* ADD NEW ATTRIB SET TO GROUP */
					
		position = Q3Group_AddObject(theNode->BaseGroup, attrib);
		GAME_ASSERT(position);
	}
				
					/* ADD TRANSPARENCY */
					
	transColor.r = transColor.g = transColor.b = transPercent;
	status = Q3AttributeSet_Add(attrib, kQ3AttributeTypeTransparencyColor, &transColor);
	GAME_ASSERT(status);

bye:
	Q3Object_Dispose(attrib);								// dispose of extra ref
#endif
}


#pragma mark -


/************************ DRAW COLLISION BOXES ****************************/

static void DrawCollisionBoxes(ObjNode *theNode, TQ3ViewObject view)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
int	n,i;
CollisionBoxType	*c;
TQ3PolyLineData	line;
TQ3Vertex3D		v[5];
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

			/* SETUP */
			
		line.numVertices = 5;
		line.vertices = &v[0];
		line.segmentAttributeSet = nil;
		line.polyLineAttributeSet = nil;
		v[0].attributeSet = nil;
		v[1].attributeSet = nil;
		v[2].attributeSet = nil;
		v[3].attributeSet = nil;
		v[4].attributeSet = nil;
	
			/* DRAW TOP */
			
		v[0].point.x = left;
		v[0].point.y = top;
		v[0].point.z = back;
		v[1].point.x = left;
		v[1].point.y = top;
		v[1].point.z = front;
		v[2].point.x = right;
		v[2].point.y = top;
		v[2].point.z = front;
		v[3].point.x = right;
		v[3].point.y = top;
		v[3].point.z = back;
		v[4].point.x = left;
		v[4].point.y = top;
		v[4].point.z = back;
		Q3PolyLine_Submit(&line, view);

			/* DRAW BOTTOM */
			
		v[0].point.x = left;
		v[0].point.y = bottom;
		v[0].point.z = back;
		v[1].point.x = left;
		v[1].point.y = bottom;
		v[1].point.z = front;
		v[2].point.x = right;
		v[2].point.y = bottom;
		v[2].point.z = front;
		v[3].point.x = right;
		v[3].point.y = bottom;
		v[3].point.z = back;
		v[4].point.x = left;
		v[4].point.y = bottom;
		v[4].point.z = back;
		Q3PolyLine_Submit(&line, view);

			/* DRAW LEFT */
			
		line.numVertices = 2;
		v[0].point.x = left;
		v[0].point.y = top;
		v[0].point.z = back;
		v[1].point.x = left;
		v[1].point.y = bottom;
		v[1].point.z = back;
		Q3PolyLine_Submit(&line, view);
		v[0].point.x = left;
		v[0].point.y = top;
		v[0].point.z = front;
		v[1].point.x = left;
		v[1].point.y = bottom;
		v[1].point.z = front;
		Q3PolyLine_Submit(&line, view);

			/* DRAW RIGHT */
			
		v[0].point.x = right;
		v[0].point.y = top;
		v[0].point.z = back;
		v[1].point.x = right;
		v[1].point.y = bottom;
		v[1].point.z = back;
		Q3PolyLine_Submit(&line, view);
		v[0].point.x = right;
		v[0].point.y = top;
		v[0].point.z = front;
		v[1].point.x = right;
		v[1].point.y = bottom;
		v[1].point.z = front;
		Q3PolyLine_Submit(&line, view);
	}
#endif
}

/************************ DRAW BOUNDING SPHERE (FOR CULLING) ****************************/

static void DrawBoundingSphere(ObjNode* theNode, TQ3ViewObject view)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
	static TQ3SubdivisionStyleData sub;
	sub.method = kQ3SubdivisionMethodConstant;
	sub.c1 = 64;
	sub.c2 = 64;
	Q3SubdivisionStyle_Submit(&sub, view);

	TQ3Point3D C = theNode->Coord;
	C.x += theNode->BoundingSphere.origin.x;
	C.y += theNode->BoundingSphere.origin.y;
	C.z += theNode->BoundingSphere.origin.z;
	float R = theNode->BoundingSphere.radius;
	static const TQ3Vector3D up = {0,1,0};
	TQ3Matrix4x4 m;
	SetLookAtMatrix(&m, &up, &C, &gGameViewInfoPtr->currentCameraCoords);

	TQ3Vector3D majorV = {R,0,0};
	TQ3Vector3D minorV = {0,R,0};
	Q3Point3D_Transform((TQ3Point3D*) &majorV, &m, (TQ3Point3D*) &majorV);
	Q3Vector3D_Normalize(&majorV, &majorV);
	majorV.x *= R;
	majorV.y *= R;
	majorV.z *= R;

	TQ3EllipseData ellipseData;
	ellipseData.uMin = 0;
	ellipseData.uMax = 1;
	ellipseData.ellipseAttributeSet = nil;
	ellipseData.origin = C;
	ellipseData.majorRadius = majorV;
	ellipseData.minorRadius = minorV;
	Q3Ellipse_Submit(&ellipseData, view);
#endif
}


