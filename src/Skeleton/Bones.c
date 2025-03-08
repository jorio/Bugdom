/****************************/
/*   	BONES.C	    	    */
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

static void DecomposeATriMesh(SkeletonDefType* gCurrentSkeleton, TQ3TriMeshData* triMeshData);
static void UpdateSkinnedGeometry_Recurse(ObjNode* skelNode, short joint);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/


static	TQ3Matrix4x4		gMatrix;

static	TQ3BoundingBox		gBBox = {{0,0,0}, {0,0,0}, kQ3False};

static	TQ3Vector3D			gTransformedNormals[MAX_DECOMPOSED_NORMALS];	// temporary buffer for holding transformed normals before they're applied to their trimeshes


/******************** LOAD BONES REFERENCE MODEL *********************/
//
// INPUT: inSpec = spec of 3dmf file to load.
//

void LoadBonesReferenceModel(const FSSpec	*inSpec, SkeletonDefType *skeleton)
{
			/* LOAD 3DMF */

	skeleton->associated3DMF = Q3MetaFile_Load3DMF(inSpec);
	GAME_ASSERT(skeleton->associated3DMF);

			/* UPLOAD TEXTURES TO GPU */

	GAME_ASSERT(skeleton->numTextures == 0);
	GAME_ASSERT(!skeleton->textureNames);

	skeleton->numTextures = skeleton->associated3DMF->numTextures;
	skeleton->textureNames = (GLuint*) NewPtrClear(skeleton->numTextures * sizeof(GLuint));

	// We want to force clamp texture UVs on all skeleton models to avoid seams at the edges of
	// alpha-tested textures. The only exception, which requires repeated texture UVs, is RootSwing.
	bool forceClampUVs = 0 != SDL_strcasecmp("RootSwing.3dmf", inSpec->cName);

	Render_Load3DMFTextures(skeleton->associated3DMF, skeleton->textureNames, forceClampUVs);

			/* DECOMPOSE REFERENCE MODEL */

	skeleton->numDecomposedTriMeshes	= 0;
	skeleton->numDecomposedPoints		= 0;
	skeleton->numDecomposedNormals		= 0;

	for (int i = 0; i < skeleton->associated3DMF->numMeshes; i++)
	{
		DecomposeATriMesh(skeleton, skeleton->associated3DMF->meshes[i]);
	}
}


/******************* DECOMPOSE A TRIMESH ***********************/

static void DecomposeATriMesh(SkeletonDefType* gCurrentSkeleton, TQ3TriMeshData* triMeshData)
{
long				numVertices;
TQ3Point3D			*vertexList;
long				i,n,refNum,pointNum;
TQ3Vector3D			*normalPtr;
DecomposedPointType	*decomposedPoint;

	n = gCurrentSkeleton->numDecomposedTriMeshes;												// get index into list of trimeshes
	GAME_ASSERT(n < MAX_DECOMPOSED_TRIMESHES);

			/* GET TRIMESH DATA */

	gCurrentSkeleton->decomposedTriMeshPtrs[n] = triMeshData;									// get trimesh data
	GAME_ASSERT(triMeshData);

	numVertices = triMeshData->numPoints;														// get # verts in trimesh
	vertexList = triMeshData->points;															// point to vert list
	normalPtr  = triMeshData->vertexNormals;													// point to normals

				/*******************************/
				/* EXTRACT VERTECIES & NORMALS */
				/*******************************/

	for (long vertNum = 0; vertNum < numVertices; vertNum++)
	{				
			/* SEE IF THIS POINT IS ALREADY IN DECOMPOSED LIST */
				
		for (pointNum=0; pointNum < gCurrentSkeleton->numDecomposedPoints; pointNum++)
		{
			decomposedPoint = &gCurrentSkeleton->decomposedPointList[pointNum];					// point to this decomposed point
			
			if (PointsAreCloseEnough(&vertexList[vertNum],&decomposedPoint->realPoint))			// see if close enough to match
			{
					/* ADD ANOTHER REFERENCE */
					
				refNum = decomposedPoint->numRefs;												// get # refs for this point
				GAME_ASSERT(refNum < MAX_POINT_REFS);

				decomposedPoint->whichTriMesh[refNum] = n;										// set triMesh #
				decomposedPoint->whichPoint[refNum] = vertNum;									// set point #
				decomposedPoint->numRefs++;														// inc counter
				goto added_vert;
			}
		}
				/* IT'S A NEW POINT SO ADD TO LIST */
				
		pointNum = gCurrentSkeleton->numDecomposedPoints;
		GAME_ASSERT(pointNum < MAX_DECOMPOSED_POINTS);
		
		refNum = 0;																			// it's the 1st entry (need refNum for below).
		
		decomposedPoint = &gCurrentSkeleton->decomposedPointList[pointNum];					// point to this decomposed point
		decomposedPoint->realPoint = vertexList[vertNum];									// add new point to list			
		decomposedPoint->whichTriMesh[refNum] = n;											// set triMesh #
		decomposedPoint->whichPoint[refNum] = vertNum;										// set point #
		decomposedPoint->numRefs = 1;														// set # refs to 1
		
		gCurrentSkeleton->numDecomposedPoints++;											// inc # decomposed points
		
added_vert:
		
		
				/***********************************************/
				/* ADD THIS POINT'S NORMAL TO THE NORMALS LIST */
				/***********************************************/
				
					/* SEE IF NORMAL ALREADY IN LIST */
					
		Q3Vector3D_Normalize(&normalPtr[vertNum],&normalPtr[vertNum]);						// normalize to be safe
					
		for (i=0; i < gCurrentSkeleton->numDecomposedNormals; i++)
			if (VectorsAreCloseEnough(&normalPtr[vertNum],&gCurrentSkeleton->decomposedNormalsList[i]))	// if already in list, then dont add it again
				goto added_norm;
	

				/* ADD NEW NORMAL TO LIST */
				
		i = gCurrentSkeleton->numDecomposedNormals;										// get # decomposed normals already in list
		GAME_ASSERT(i < MAX_DECOMPOSED_NORMALS);

		gCurrentSkeleton->decomposedNormalsList[i] = normalPtr[vertNum];				// add new normal to list			
		gCurrentSkeleton->numDecomposedNormals++;										// inc # decomposed normals
		
added_norm:
					/* KEEP REF TO NORMAL IN POINT LIST */

		decomposedPoint->whichNormal[refNum] = i;										// save index to normal	
	}

	gCurrentSkeleton->numDecomposedTriMeshes++;											// inc # of trimeshes in decomp list
}



/************************** UPDATE SKINNED GEOMETRY *******************************/
//
// Updates all of the points in the local trimesh data's to coordinate with the
// current joint transforms.
//

void UpdateSkinnedGeometry(ObjNode *theNode)
{
			/* MAKE SURE OBJNODE IS STILL VALID */
			//
			// It's possible that Deleting a Skeleton and then creating a new
			// ObjNode like an explosion can make it seem like the current
			// objNode pointer is still valid when in fact it isn't.
			// Detecting this is too complex to be worth-while, so it's
			// easier to just verify the objNode here instead.
			//
			
	if (theNode->CType == INVALID_NODE_FLAG)
		return;

	GAME_ASSERT(theNode->Skeleton);

	const SkeletonDefType* skeletonDef = theNode->Skeleton->skeletonDefinition;
	GAME_ASSERT(skeletonDef);

	if (theNode->Skeleton->JointsAreGlobal)
		Q3Matrix4x4_SetIdentity(&gMatrix);
	else
		gMatrix = theNode->BaseTransformMatrix;	

	gBBox.min.x = gBBox.min.y = gBBox.min.z = 10000000;
	gBBox.max.x = gBBox.max.y = gBBox.max.z = -gBBox.min.x;								// init bounding box calc

	GAME_ASSERT_MESSAGE(skeletonDef->Bones[0].parentBone == NO_PREVIOUS_JOINT, "joint 0 isnt base - fix code Brian!");

	UpdateSkinnedGeometry_Recurse(theNode, 0);								// start @ base

			/* UPDATE ALL TRIMESH BBOXES */

	GAME_ASSERT(theNode->NumMeshes == skeletonDef->numDecomposedTriMeshes);
	for (int i = 0; i < theNode->NumMeshes; i++)
	{
		theNode->MeshList[i]->bBox = gBBox;				// apply to local copy of trimesh
	}
}


/******************** UPDATE SKINNED GEOMETRY: RECURSE ************************/

static void UpdateSkinnedGeometry_Recurse(ObjNode* skelNode, short joint)
{
long					numChildren,numPoints,p,i,numRefs,r,triMeshNum,p2,c,numNormals,n;
TQ3Matrix4x4			oldM;
TQ3Vector3D				*normalAttribs;
BoneDefinitionType		*bonePtr;
float					minX,maxX,maxY,minY,maxZ,minZ;
float					m00,m01,m02,m10,m11,m12,m20,m21,m22,m30,m31,m32;
float					newX,newY,newZ;
SkeletonObjDataType		*currentSkelObjData = skelNode->Skeleton;
const SkeletonDefType	*currentSkeleton = skelNode->Skeleton->skeletonDefinition;
const float				*jointMat;
float					*matPtr;
DecomposedPointType		*decomposedPointList = currentSkeleton->decomposedPointList;
TQ3TriMeshData			**localTriMeshes = skelNode->MeshList;

	minX = minY = minZ = 10000000;
	maxX = maxY = maxZ = -minX;									// calc local bbox with registers for speed

				/*********************************/
				/* FACTOR IN THIS JOINT'S MATRIX */
				/*********************************/
				
	jointMat = &currentSkelObjData->jointTransformMatrix[joint].value[0][0];
	matPtr = &gMatrix.value[0][0];
	
	if (!currentSkelObjData->JointsAreGlobal)
	{
		MatrixMultiply((TQ3Matrix4x4 *)jointMat, (TQ3Matrix4x4 *)matPtr, (TQ3Matrix4x4 *)matPtr);				

		m00 = matPtr[0];	m01 = matPtr[1];	m02 = matPtr[2];
		m10 = matPtr[4];	m11 = matPtr[5];	m12 = matPtr[6];
		m20 = matPtr[8];	m21 = matPtr[9];	m22 = matPtr[10];
		m30 = matPtr[12];	m31 = matPtr[13];	m32 = matPtr[14];
	}
	else
	{
		m00 = jointMat[0];	m01 = jointMat[1];	m02 = jointMat[2];
		m10 = jointMat[4];	m11 = jointMat[5];	m12 = jointMat[6];
		m20 = jointMat[8];	m21 = jointMat[9];	m22 = jointMat[10];
		m30 = jointMat[12];	m31 = jointMat[13];	m32 = jointMat[14];
	}

			/*************************/
			/* TRANSFORM THE NORMALS */
			/*************************/

		/* APPLY MATRIX TO EACH NORMAL VECTOR */
			
	bonePtr = &currentSkeleton->Bones[joint];									// point to bone def
	numNormals = bonePtr->numNormalsAttachedToBone;								// get # normals attached to this bone
				
				
	for (p=0; p < numNormals; p++)
	{
		float	x,y,z;

		i = bonePtr->normalList[p];												// get index to normal in gDecomposedNormalsList
		
		x = currentSkeleton->decomposedNormalsList[i].x;						// get xyz of normal
		y = currentSkeleton->decomposedNormalsList[i].y;
		z = currentSkeleton->decomposedNormalsList[i].z;

		gTransformedNormals[i].x = (m00*x) + (m10*y) + (m20*z);					// transform the normal
		gTransformedNormals[i].y = (m01*x) + (m11*y) + (m21*z);
		gTransformedNormals[i].z = (m02*x) + (m12*y) + (m22*z);
	}
	
	

			/* APPLY TRANSFORMED VECTORS TO ALL REFERENCES */
	
	numPoints = bonePtr->numPointsAttachedToBone;									// get # points attached to this bone

	for (p = 0; p < numPoints; p++)
	{
		i = bonePtr->pointList[p];													// get index to point in gDecomposedPointList
				
		numRefs = decomposedPointList[i].numRefs;									// get # times this point is referenced
		if (numRefs == 1)															// SPECIAL CASE IF ONLY 1 REF (OPTIMIZATION)
		{
			triMeshNum = decomposedPointList[i].whichTriMesh[0];					// get triMesh # that uses this point
			p2 = decomposedPointList[i].whichPoint[0];								// get point # in the triMesh 
			n = decomposedPointList[i].whichNormal[0];								// get index into gDecomposedNormalsList

			normalAttribs = localTriMeshes[triMeshNum]->vertexNormals;				// point to normals attribute list in local trimesh
			normalAttribs[p2] = gTransformedNormals[n];								// copy transformed normal into triMesh
		}
		else																		// handle multi-case
		{		
			for (r = 0; r < numRefs; r++)
			{		
				triMeshNum = decomposedPointList[i].whichTriMesh[r];					
				p2 = decomposedPointList[i].whichPoint[r];								
				n = decomposedPointList[i].whichNormal[r];								

				normalAttribs = localTriMeshes[triMeshNum]->vertexNormals;
				normalAttribs[p2] = gTransformedNormals[n];
			}
		}
	}

			/************************/
			/* TRANSFORM THE POINTS */
			/************************/

	if (!currentSkelObjData->JointsAreGlobal)
	{
		m00 = matPtr[0];	m01 = matPtr[1];	m02 = matPtr[2];
		m10 = matPtr[4];	m11 = matPtr[5];	m12 = matPtr[6];
		m20 = matPtr[8];	m21 = matPtr[9];	m22 = matPtr[10];
		m30 = matPtr[12];	m31 = matPtr[13];	m32 = matPtr[14];
	}

	for (p = 0; p < numPoints; p++)
	{
		float	x,y,z;
		
		i = bonePtr->pointList[p];														// get index to point in gDecomposedPointList
		x = decomposedPointList[i].boneRelPoint.x;										// get xyz of point
		y = decomposedPointList[i].boneRelPoint.y;
		z = decomposedPointList[i].boneRelPoint.z;

				/* TRANSFORM & UPDATE BBOX */
				
		newX = (m00*x) + (m10*y) + (m20*z) + m30;										// transform x value
		if (newX < minX)																// update bbox
			minX = newX;
		if (newX > maxX)
			maxX = newX;

		newY = (m01*x) + (m11*y) + (m21*z) + m31;										// transform y
		if (newY < minY)																// update bbox
			minY = newY;
		if (newY > maxY)
			maxY = newY;

		newZ = (m02*x) + (m12*y) + (m22*z) + m32;										// transform z
		if (newZ > maxZ)																// update bbox
			maxZ = newZ;
		if (newZ < minZ)
			minZ = newZ;
		
	
				/* APPLY NEW POINT TO ALL REFERENCES */
				
		numRefs = decomposedPointList[i].numRefs;										// get # times this point is referenced
		if (numRefs == 1)																// SPECIAL CASE IF ONLY 1 REF (OPTIMIZATION)
		{
			triMeshNum = decomposedPointList[i].whichTriMesh[0];						// get triMesh # that uses this point
			p2 = decomposedPointList[i].whichPoint[0];									// get point # in the triMesh
	
			localTriMeshes[triMeshNum]->points[p2].x = newX;								// set the point in local copy of trimesh
			localTriMeshes[triMeshNum]->points[p2].y = newY;
			localTriMeshes[triMeshNum]->points[p2].z = newZ;
		}
		else																			// multi-refs
		{		
			for (r = 0; r < numRefs; r++)
			{
				triMeshNum = decomposedPointList[i].whichTriMesh[r];					
				p2 = decomposedPointList[i].whichPoint[r];								
		
				localTriMeshes[triMeshNum]->points[p2].x = newX;
				localTriMeshes[triMeshNum]->points[p2].y = newY;
				localTriMeshes[triMeshNum]->points[p2].z = newZ;
			}
		}
	}

				/* UPDATE GLOBAL BBOX */
				
	if (minX < gBBox.min.x)
		gBBox.min.x = minX;
	if (maxX > gBBox.max.x)
		gBBox.max.x = maxX;

	if (minY < gBBox.min.y)
		gBBox.min.y = minY;
	if (maxY > gBBox.max.y)
		gBBox.max.y = maxY;

	if (minZ < gBBox.min.z)
		gBBox.min.z = minZ;
	if (maxZ > gBBox.max.z)
		gBBox.max.z = maxZ;


			/* RECURSE THRU ALL CHILDREN */
			
	numChildren = currentSkeleton->numChildren[joint];									// get # children
	for (c = 0; c < numChildren; c++)
	{
		oldM = gMatrix;																	// push matrix
		UpdateSkinnedGeometry_Recurse(skelNode, currentSkeleton->childIndecies[joint][c]);
		gMatrix = oldM;																	// pop matrix
	}
}


/******************* PRIME BONE DATA *********************/
//
// After a skeleton file is loaded, this will calc some other needed things.
//

void PrimeBoneData(SkeletonDefType *skeleton)
{
long	i,b,j;

	GAME_ASSERT(skeleton->NumBones != 0);

			
			/* SET THE FORWARD LINKS */
			
	for (b = 0; b < skeleton->NumBones; b++)
	{
		skeleton->numChildren[b] = 0;								// init child counter
		
		for (i = 0; i < skeleton->NumBones; i++)					// look for a child
		{
			if (skeleton->Bones[i].parentBone == b)					// is this "i" a child of "b"?
			{
				j = skeleton->numChildren[b];						// get # children
				GAME_ASSERT(j < MAX_CHILDREN);

				skeleton->childIndecies[b][j] = i;					// set index to child
	
				skeleton->numChildren[b]++;							// inc # children
			}
		}
	}
}










