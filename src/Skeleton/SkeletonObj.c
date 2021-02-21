/****************************/
/*   	SKELETON.C    	    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/




extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode				*gPlayerNode[];
extern	QD3DSetupOutputType		*gGameViewInfoPtr;


/****************************/
/*    PROTOTYPES            */
/****************************/

static SkeletonObjDataType *MakeNewSkeletonBaseData(short sourceSkeletonNum);
static void DisposeSkeletonDefinitionMemory(SkeletonDefType *skeleton);
static void CalcSkeletonBoundingSphere(long n);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

static SkeletonDefType		*gLoadedSkeletonsList[MAX_SKELETON_TYPES];
static TQ3BoundingSphere	gSkeletonBoundingSpheres[MAX_SKELETON_TYPES];

static short	gNumDecomposedTriMeshesInSkeleton[MAX_SKELETON_TYPES];
TQ3TriMeshData	*gLocalTriMeshesOfSkelType[MAX_SKELETON_TYPES][MAX_DECOMPOSED_TRIMESHES];


/**************** INIT SKELETON MANAGER *********************/

void InitSkeletonManager(void)
{
	CalcAccelerationSplineCurve();									// calc accel curve

	memset(gLoadedSkeletonsList, 0, sizeof(gLoadedSkeletonsList));
}


/******************** LOAD A SKELETON ****************************/

void LoadASkeleton(Byte num)
{
short	i,numDecomp;

	GAME_ASSERT(num < MAX_SKELETON_TYPES);

	if (gLoadedSkeletonsList[num] == nil)					// check if already loaded
		gLoadedSkeletonsList[num] = LoadSkeletonFile(num);
			
			/* CALC BOUNDING SPHERE OF OBJECT */
				
	CalcSkeletonBoundingSphere(num);
	

		/* MAKE LOCAL COPY OF DECOMPOSED TRIMESH */
		
	numDecomp = gLoadedSkeletonsList[num]->numDecomposedTriMeshes;
	gNumDecomposedTriMeshesInSkeleton[num] = numDecomp;
	for (i=0; i < numDecomp; i++)
	{
//		QD3D_DuplicateTriMeshData(gLoadedSkeletonsList[num]->decomposedTriMeshPtrs[i], &gLocalTriMeshesOfSkelType[num][i]);
		gLocalTriMeshesOfSkelType[num][i] = Q3TriMeshData_Duplicate(gLoadedSkeletonsList[num]->decomposedTriMeshPtrs[i]);
	}
	
}


/*************** CALC SKELETON BOUNDING SPHERE ************************/

static void CalcSkeletonBoundingSphere(long n)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA

long	i;
TQ3Status status;

	GAME_ASSERT(gGameViewInfoPtr);

	status = Q3View_StartBoundingSphere(gGameViewInfoPtr->viewObject, kQ3ComputeBoundsExact);
	GAME_ASSERT(status);

	do
	{
		for (i = 0; i < gLoadedSkeletonsList[n]->numDecomposedTriMeshes; i++)
		{
			status = Q3TriMesh_Submit(&gLoadedSkeletonsList[n]->decomposedTriMeshes[i], gGameViewInfoPtr->viewObject);
			GAME_ASSERT(status);
		}
	}while(Q3View_EndBoundingSphere(gGameViewInfoPtr->viewObject, &gSkeletonBoundingSpheres[n]) == kQ3ViewStatusRetraverse);

#endif
}



/****************** FREE SKELETON FILE **************************/
//
// Disposes of all memory used by a skeleton file (from File.c)
//

void FreeSkeletonFile(Byte skeletonType)
{
short	i;

	if (gLoadedSkeletonsList[skeletonType])										// make sure this really exists
	{
		for (i=0; i < gNumDecomposedTriMeshesInSkeleton[skeletonType]; i++)		// dispose of the local copies of the decomposed trimeshes
			QD3D_FreeDuplicateTriMeshData(&gLocalTriMeshesOfSkelType[skeletonType][i]);
	
		DisposeSkeletonDefinitionMemory(gLoadedSkeletonsList[skeletonType]);	// free skeleton data
		gLoadedSkeletonsList[skeletonType] = nil;
	}
}


/*************** FREE ALL SKELETON FILES ***************************/
//
// Free's all except for the input type (-1 == none to skip)
//

void FreeAllSkeletonFiles(short skipMe)
{
short	i;

	for (i = 0; i < MAX_SKELETON_TYPES; i++)
	{
		if (i != skipMe)
	 		FreeSkeletonFile(i);
	}
}

#pragma mark -

/***************** MAKE NEW SKELETON OBJECT *******************/
//
// This routine simply initializes the blank object.
// The function CopySkeletonInfoToNewSkeleton actually attaches the specific skeleton
// file to this ObjNode.
//

ObjNode	*MakeNewSkeletonObject(NewObjectDefinitionType *newObjDef)
{
ObjNode	*newNode;
int		type;
float	scale;

	type = newObjDef->type; 
	scale = newObjDef->scale;

			/* CREATE NEW OBJECT NODE */
			
	newObjDef->genre = SKELETON_GENRE;
	newNode = MakeNewObject(newObjDef);		
	if (newNode == nil)
		return(nil);
		
	newNode->StatusBits |= STATUS_BIT_ANIM;						// turn on animation		
		
			/* LOAD SKELETON FILE INTO OBJECT */
			
	newNode->Skeleton = MakeNewSkeletonBaseData(type); 			// alloc & set skeleton data
	GAME_ASSERT(newNode->Skeleton);

	UpdateObjectTransforms(newNode);


			/*  SET INITIAL DEFAULT POSITION */
				
	SetSkeletonAnim(newNode->Skeleton, newObjDef->animNum);		
	UpdateSkeletonAnimation(newNode);
	UpdateSkinnedGeometry(newNode);								// prime the trimesh

	
			/**********************************/
			/* CALC BOUNDING SPHERE OF OBJECT */
			/**********************************/
	

	newNode->BoundingSphere.origin.x = gSkeletonBoundingSpheres[type].origin.x * scale;
	newNode->BoundingSphere.origin.y = gSkeletonBoundingSpheres[type].origin.y * scale;
	newNode->BoundingSphere.origin.z = gSkeletonBoundingSpheres[type].origin.z * scale;
	newNode->BoundingSphere.radius = gSkeletonBoundingSpheres[type].radius * scale;

	return(newNode);
}




/***************** ALLOC SKELETON DEFINITION MEMORY **********************/
//
// Allocates all of the sub-arrays for a skeleton file's definition data.
// ONLY called by ReadDataFromSkeletonFile in file.c.
//
// NOTE: skeleton has already been allocated by LoadSkeleton!!!
//

void AllocSkeletonDefinitionMemory(SkeletonDefType *skeleton)
{
long	numAnims,numJoints;

	numJoints = skeleton->NumBones;											// get # joints in skeleton
	numAnims = skeleton->NumAnims;											// get # anims in skeleton

				/***************************/
				/* ALLOC ANIM EVENTS LISTS */
				/***************************/

	skeleton->NumAnimEvents = (Byte *)AllocPtr(sizeof(Byte)*numAnims);		// array which holds # events for each anim
	GAME_ASSERT(skeleton->NumAnimEvents);

	Alloc_2d_array(AnimEventType, skeleton->AnimEventsList, numAnims, MAX_ANIM_EVENTS);

//	skeleton->AnimEventsList = (AnimEventType **)AllocPtr(sizeof(AnimEventType *)*numAnims);	// alloc 1st dimension (for each anim)
//	if (skeleton->AnimEventsList == nil)
//		DoFatalAlert("Not enough memory to alloc AnimEventsList");
//	for (i=0; i < numAnims; i++)
//	{
//		skeleton->AnimEventsList[i] = (AnimEventType *)AllocPtr(sizeof(AnimEventType)*MAX_ANIM_EVENTS);	// alloc 2nd dimension (for max events)
//		if (skeleton->AnimEventsList[i] == nil)
//			DoFatalAlert("Not enough memory to alloc AnimEventsList");
//	}	
	
			/* ALLOC BONE INFO */
			
	skeleton->Bones = (BoneDefinitionType *)AllocPtr(sizeof(BoneDefinitionType)*numJoints);	
	GAME_ASSERT(skeleton->Bones);


		/* ALLOC DECOMPOSED DATA */
			
	skeleton->decomposedTriMeshPtrs = (TQ3TriMeshData *)AllocPtr(sizeof(TQ3TriMeshData)*MAX_DECOMPOSED_TRIMESHES);
	GAME_ASSERT(skeleton->decomposedTriMeshPtrs);

	skeleton->decomposedPointList = (DecomposedPointType *)AllocPtr(sizeof(DecomposedPointType)*MAX_DECOMPOSED_POINTS);		
	GAME_ASSERT(skeleton->decomposedPointList);

	skeleton->decomposedNormalsList = (TQ3Vector3D *)AllocPtr(sizeof(TQ3Vector3D)*MAX_DECOMPOSED_NORMALS);		
	GAME_ASSERT(skeleton->decomposedNormalsList);
}


/*************** DISPOSE SKELETON DEFINITION MEMORY ***************************/
//
// Disposes of all alloced memory (from above) used by a skeleton file definition.
//

static void DisposeSkeletonDefinitionMemory(SkeletonDefType *skeleton)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA

short	j,numAnims,numJoints;

	if (skeleton == nil)
		return;

	numAnims = skeleton->NumAnims;										// get # anims in skeleton
	numJoints = skeleton->NumBones;	

			/* NUKE THE SKELETON BONE POINT & NORMAL INDEX ARRAYS */
			
	for (j=0; j < numJoints; j++)
	{
		if (skeleton->Bones[j].pointList)
			DisposePtr((Ptr)skeleton->Bones[j].pointList);
		if (skeleton->Bones[j].normalList)
			DisposePtr((Ptr)skeleton->Bones[j].normalList);			
	}
	DisposePtr((Ptr)skeleton->Bones);									// free bones array
	skeleton->Bones = nil;

				/* DISPOSE ANIM EVENTS LISTS */
				
	DisposePtr((Ptr)skeleton->NumAnimEvents);
	
	Free2DArray((void**) skeleton->AnimEventsList);
	skeleton->AnimEventsList = nil;
	
//	for (i=0; i < numAnims; i++)
//		DisposePtr((Ptr)skeleton->AnimEventsList[i]);
//	DisposePtr((Ptr)skeleton->AnimEventsList);

			/* DISPOSE JOINT INFO */
			
	for (j=0; j < numJoints; j++)
	{
		Free2DArray((void**) skeleton->JointKeyframes[j].keyFrames);		// dispose 2D array of keyframe data

		skeleton->JointKeyframes[j].keyFrames = nil;
	}

			/* DISPOSE DECOMPOSED DATA ARRAYS */
	
	for (j = 0; j < skeleton->numDecomposedTriMeshes; j++)			// first dispose of the trimesh data in there
		Q3TriMesh_EmptyData(&skeleton->decomposedTriMeshes[j]);
	
	if (skeleton->decomposedTriMeshes)
	{
		DisposePtr((Ptr)skeleton->decomposedTriMeshes);
		skeleton->decomposedTriMeshes = nil;
	}

	if (skeleton->decomposedPointList)
	{
		DisposePtr((Ptr)skeleton->decomposedPointList);
		skeleton->decomposedPointList = nil;
	}

	if (skeleton->decomposedNormalsList)
	{
		DisposePtr((Ptr)skeleton->decomposedNormalsList);
		skeleton->decomposedNormalsList = nil;
	}

			/* DISPOSE OF MASTER DEFINITION BLOCK */
			
	DisposePtr((Ptr)skeleton);
#endif
}

#pragma mark -


/****************** MAKE NEW SKELETON OBJECT DATA *********************/
//
// Allocates & inits the Skeleton data for an ObjNode.
//

static SkeletonObjDataType *MakeNewSkeletonBaseData(short sourceSkeletonNum)
{
SkeletonDefType		*skeletonDefPtr;
SkeletonObjDataType	*skeletonData;

	skeletonDefPtr = gLoadedSkeletonsList[sourceSkeletonNum];				// get ptr to source skeleton definition info
	GAME_ASSERT_MESSAGE(skeletonDefPtr, "Skeleton data isn't loaded!");


			/* ALLOC MEMORY FOR NEW SKELETON OBJECT DATA STRUCTURE */
			
	skeletonData = (SkeletonObjDataType *)AllocPtr(sizeof(SkeletonObjDataType));
	GAME_ASSERT(skeletonData);


			/* INIT NEW SKELETON */

	skeletonData->skeletonDefinition = skeletonDefPtr;						// point to source animation data
	skeletonData->AnimSpeed = 1.0;
	skeletonData->JointsAreGlobal = false;

			/****************************************/
			/* MAKE COPY OF TRIMESHES FOR LOCAL USE */
			/****************************************/
			

	return(skeletonData);
}


/************************ FREE SKELETON BASE DATA ***************************/

void FreeSkeletonBaseData(SkeletonObjDataType *data)
{
	
			/* FREE THE SKELETON DATA */
			
	DisposePtr((Ptr)data);			
}


















