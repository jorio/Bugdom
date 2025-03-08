/****************************/
/*   	SKELETON.C    	    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static SkeletonObjDataType *MakeNewSkeletonBaseData(short sourceSkeletonNum);
static void DisposeSkeletonDefinitionMemory(SkeletonDefType *skeleton);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

static SkeletonDefType		*gLoadedSkeletonsList[MAX_SKELETON_TYPES];
static TQ3BoundingSphere	gSkeletonBoundingSpheres[MAX_SKELETON_TYPES];



/**************** INIT SKELETON MANAGER *********************/

void InitSkeletonManager(void)
{
	CalcAccelerationSplineCurve();									// calc accel curve

	SDL_memset(gLoadedSkeletonsList, 0, sizeof(gLoadedSkeletonsList));
}


/******************** LOAD A SKELETON ****************************/

void LoadASkeleton(Byte num)
{
	GAME_ASSERT(num < MAX_SKELETON_TYPES);

	if (gLoadedSkeletonsList[num] == nil)					// check if already loaded
		gLoadedSkeletonsList[num] = LoadSkeletonFile(num);

			/* CALC BOUNDING SPHERE OF OBJECT */

	QD3D_CalcObjectBoundingSphere(
			gLoadedSkeletonsList[num]->numDecomposedTriMeshes,
			gLoadedSkeletonsList[num]->decomposedTriMeshPtrs,
			&gSkeletonBoundingSpheres[num]
			);

			/* IF ANT KING, MAKE FIERY TEXTURES NULL-SHADED */

	if (num == SKELETON_TYPE_KINGANT)
	{
		gLoadedSkeletonsList[num]->decomposedTriMeshPtrs[3]->texturingMode |= kQ3TexturingModeExt_NullShaderFlag;	// eyebrows
		gLoadedSkeletonsList[num]->decomposedTriMeshPtrs[8]->texturingMode |= kQ3TexturingModeExt_NullShaderFlag;	// hair
		gLoadedSkeletonsList[num]->decomposedTriMeshPtrs[9]->texturingMode |= kQ3TexturingModeExt_NullShaderFlag;	// beard
	}
}



/****************** FREE SKELETON FILE **************************/
//
// Disposes of all memory used by a skeleton file (from File.c)
//

void FreeSkeletonFile(Byte skeletonType)
{
	if (gLoadedSkeletonsList[skeletonType])										// make sure this really exists
	{
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

			/* MAKE COPY OF TRIMESHES FOR LOCAL USE */

	GAME_ASSERT(newNode->NumMeshes == 0);

	const SkeletonDefType* skeletonDef = gLoadedSkeletonsList[type];
	newNode->NumMeshes = skeletonDef->numDecomposedTriMeshes;

	for (int i = 0; i < skeletonDef->numDecomposedTriMeshes; i++)
	{
		GAME_ASSERT_MESSAGE(!newNode->MeshList[i], "Node already had a mesh at that index!");

		newNode->MeshList[i] = Q3TriMeshData_Duplicate(skeletonDef->decomposedTriMeshPtrs[i]);
		newNode->OwnsMeshMemory[i] = true;
	}

			/*  SET INITIAL DEFAULT POSITION */

	UpdateObjectTransforms(newNode);

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

	skeleton->decomposedTriMeshPtrs = (TQ3TriMeshData **)AllocPtr(sizeof(TQ3TriMeshData*)*MAX_DECOMPOSED_TRIMESHES);
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
	if (skeleton == nil)
		return;

	int numJoints = skeleton->NumBones;

			/* NUKE THE SKELETON BONE POINT & NORMAL INDEX ARRAYS */
			
	for (int j=0; j < numJoints; j++)
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

			/* DISPOSE JOINT INFO */
			
	for (int j=0; j < numJoints; j++)
	{
		Free2DArray((void**) skeleton->JointKeyframes[j].keyFrames);		// dispose 2D array of keyframe data

		skeleton->JointKeyframes[j].keyFrames = nil;
	}

			/* DISPOSE DECOMPOSED DATA ARRAYS */

	// DON'T call Q3TriMeshData_Dispose on every decomposedTriMeshPtrs as they're just pointers
	// to trimeshes in the 3DMF. We dispose of the 3DMF at the end.
	if (skeleton->decomposedTriMeshPtrs)
	{
		DisposePtr((Ptr)skeleton->decomposedTriMeshPtrs);
		skeleton->decomposedTriMeshPtrs = nil;
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

			/* DISPOSE OF 3DMF */

	if (skeleton->associated3DMF)
	{
		Q3MetaFile_Dispose(skeleton->associated3DMF);
		skeleton->associated3DMF = nil;
	}

			/* DISPOSE OF TEXTURES */

	if (skeleton->textureNames)
	{
		glDeleteTextures(skeleton->numTextures, skeleton->textureNames);
		DisposePtr((Ptr) skeleton->textureNames);
		skeleton->numTextures = 0;
		skeleton->textureNames = nil;
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
	GAME_ASSERT_MESSAGE(skeletonDefPtr, "Skeleton data isn't loaded!");


			/* ALLOC MEMORY FOR NEW SKELETON OBJECT DATA STRUCTURE */
			
	skeletonData = (SkeletonObjDataType *)AllocPtr(sizeof(SkeletonObjDataType));
	GAME_ASSERT(skeletonData);


			/* INIT NEW SKELETON */

	skeletonData->skeletonDefinition = skeletonDefPtr;						// point to source animation data
	skeletonData->AnimSpeed = 1.0;
	skeletonData->JointsAreGlobal = false;

	return(skeletonData);
}


/************************ FREE SKELETON BASE DATA ***************************/

void FreeSkeletonBaseData(SkeletonObjDataType *data)
{
	
			/* FREE THE SKELETON DATA */
			
	DisposePtr((Ptr)data);			
}

