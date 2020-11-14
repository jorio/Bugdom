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
TQ3TriMeshData	**gLocalTriMeshesOfSkelType = nil;


/**************** INIT SKELETON MANAGER *********************/

void InitSkeletonManager(void)
{
short	i;

	CalcAccelerationSplineCurve();									// calc accel curve

	for (i =0; i < MAX_SKELETON_TYPES; i++)
		gLoadedSkeletonsList[i] = nil;
		
		/* ALLOCATE LOCAL TRIMESHES FOR ALL SKELETON TYPES */

	Alloc_2d_array(TQ3TriMeshData, gLocalTriMeshesOfSkelType, MAX_SKELETON_TYPES, MAX_DECOMPOSED_TRIMESHES);
}


/******************** LOAD A SKELETON ****************************/

void LoadASkeleton(Byte num)
{
short	i,numDecomp;

	if (num >= MAX_SKELETON_TYPES)
		DoFatalAlert("\pLoadASkeleton: MAX_SKELETON_TYPES exceeded!");
		
	if (gLoadedSkeletonsList[num] == nil)					// check if already loaded
		gLoadedSkeletonsList[num] = LoadSkeletonFile(num);
			
			/* CALC BOUNDING SPHERE OF OBJECT */
				
	CalcSkeletonBoundingSphere(num);
	

		/* MAKE LOCAL COPY OF DECOMPOSED TRIMESH */
		
	numDecomp = gLoadedSkeletonsList[num]->numDecomposedTriMeshes;
	gNumDecomposedTriMeshesInSkeleton[num] = numDecomp;
	for (i=0; i < numDecomp; i++)
		QD3D_DuplicateTriMeshData(&gLoadedSkeletonsList[num]->decomposedTriMeshes[i],&gLocalTriMeshesOfSkelType[num][i]);
	
}


/*************** CALC SKELETON BOUNDING SPHERE ************************/

static void CalcSkeletonBoundingSphere(long n)
{
long	i;

	if (gGameViewInfoPtr == nil)
		DoFatalAlert("\pCalcSkeletonBoundingSphere: setupInfo = nil");

	if (Q3View_StartBoundingSphere(gGameViewInfoPtr->viewObject, kQ3ComputeBoundsExact) != kQ3Success)
		DoFatalAlert("\pCalcSkeletonBoundingSphere: Q3View_StartBoundingSphere failed");
	do
	{
		for (i = 0; i < gLoadedSkeletonsList[n]->numDecomposedTriMeshes; i++)
		{
			if (Q3TriMesh_Submit(&gLoadedSkeletonsList[n]->decomposedTriMeshes[i],
							gGameViewInfoPtr->viewObject) != kQ3Success)
				DoFatalAlert("\pCalcSkeletonBoundingSphere: Q3TriMesh_Submit failed");
										
		}	
	}while(Q3View_EndBoundingSphere(gGameViewInfoPtr->viewObject, &gSkeletonBoundingSpheres[n]) == kQ3ViewStatusRetraverse);
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
	if (newNode->Skeleton == nil)
		DoFatalAlert("\pMakeNewSkeletonObject: MakeNewSkeletonBaseData == nil");

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
	if (skeleton->NumAnimEvents == nil)
		DoFatalAlert("\pNot enough memory to alloc NumAnimEvents");

	Alloc_2d_array(AnimEventType, skeleton->AnimEventsList, numAnims, MAX_ANIM_EVENTS);

//	skeleton->AnimEventsList = (AnimEventType **)AllocPtr(sizeof(AnimEventType *)*numAnims);	// alloc 1st dimension (for each anim)
//	if (skeleton->AnimEventsList == nil)
//		DoFatalAlert("\pNot enough memory to alloc AnimEventsList");
//	for (i=0; i < numAnims; i++)
//	{
//		skeleton->AnimEventsList[i] = (AnimEventType *)AllocPtr(sizeof(AnimEventType)*MAX_ANIM_EVENTS);	// alloc 2nd dimension (for max events)
//		if (skeleton->AnimEventsList[i] == nil)
//			DoFatalAlert("\pNot enough memory to alloc AnimEventsList");
//	}	
	
			/* ALLOC BONE INFO */
			
	skeleton->Bones = (BoneDefinitionType *)AllocPtr(sizeof(BoneDefinitionType)*numJoints);	
	if (skeleton->Bones == nil)
		DoFatalAlert("\pNot enough memory to alloc Bones");


		/* ALLOC DECOMPOSED DATA */
			
	skeleton->decomposedTriMeshes = (TQ3TriMeshData *)AllocPtr(sizeof(TQ3TriMeshData)*MAX_DECOMPOSED_TRIMESHES);		
	if (skeleton->decomposedTriMeshes == nil)
		DoFatalAlert("\pNot enough memory to alloc decomposedTriMeshes");

	skeleton->decomposedPointList = (DecomposedPointType *)AllocPtr(sizeof(DecomposedPointType)*MAX_DECOMPOSED_POINTS);		
	if (skeleton->decomposedPointList == nil)
		DoFatalAlert("\pNot enough memory to alloc decomposedPointList");

	skeleton->decomposedNormalsList = (TQ3Vector3D *)AllocPtr(sizeof(TQ3Vector3D)*MAX_DECOMPOSED_NORMALS);		
	if (skeleton->decomposedNormalsList == nil)
		DoFatalAlert("\pNot enough memory to alloc decomposedNormalsList");
			
	
}


/*************** DISPOSE SKELETON DEFINITION MEMORY ***************************/
//
// Disposes of all alloced memory (from above) used by a skeleton file definition.
//

static void DisposeSkeletonDefinitionMemory(SkeletonDefType *skeleton)
{	
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
	
	Free_2d_array(skeleton->AnimEventsList);
	
//	for (i=0; i < numAnims; i++)
//		DisposePtr((Ptr)skeleton->AnimEventsList[i]);
//	DisposePtr((Ptr)skeleton->AnimEventsList);

			/* DISPOSE JOINT INFO */
			
	for (j=0; j < numJoints; j++)
	{
		Free_2d_array(skeleton->JointKeyframes[j].keyFrames);		// dispose 2D array of keyframe data

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
	if (skeletonDefPtr == nil)
	{
		DoAlert("\pMakeNewSkeletonBaseData: Skeleton data isnt loaded!");
		ShowSystemErr(sourceSkeletonNum);
	}
		

			/* ALLOC MEMORY FOR NEW SKELETON OBJECT DATA STRUCTURE */
			
	skeletonData = (SkeletonObjDataType *)AllocPtr(sizeof(SkeletonObjDataType));
	if (skeletonData == nil)
		DoFatalAlert("\pMakeNewSkeletonBaseData: Cannot alloc new SkeletonObjDataType");


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


















