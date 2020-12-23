/****************************/
/*   	BONES.C	    	    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "3dmath.h"

extern	TQ3TriMeshData		**gLocalTriMeshesOfSkelType;
extern	const TQ3Float32	gTextureAlphaThreshold;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void DecomposeATriMesh(TQ3Object theTriMesh);
static void DecompRefMo_Recurse(TQ3Object inObj);
static void DecomposeReferenceModel(TQ3Object theModel);
static void UpdateSkinnedGeometry_Recurse(short joint, short skelType);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/


SkeletonDefType		*gCurrentSkeleton;
SkeletonObjDataType	*gCurrentSkelObjData;

static	TQ3Matrix4x4		gMatrix;

static	TQ3BoundingBox		gBBox = {{0,0,0}, {0,0,0}, kQ3False};

static	TQ3Vector3D			gTransformedNormals[MAX_DECOMPOSED_NORMALS];	// temporary buffer for holding transformed normals before they're applied to their trimeshes


/******************** LOAD BONES REFERENCE MODEL *********************/
//
// INPUT: inSpec = spec of 3dmf file to load.
//

void LoadBonesReferenceModel(const FSSpec	*inSpec, SkeletonDefType *skeleton)
{
TQ3Object		newModel;

	newModel = Load3DMFModel(inSpec);
	GAME_ASSERT(newModel);


	ForEachTriMesh(newModel, QD3D_SetTextureAlphaThreshold_TriMesh, (void*) &gTextureAlphaThreshold);	// DISCARD TRANSPARENT TEXELS (Source port addition)


	gCurrentSkeleton = skeleton;
	DecomposeReferenceModel(newModel);
	
	Q3Object_Dispose(newModel);				// we dont need the original model anymore
}


/****************** DECOMPOSE REFERENCE MODEL ************************/

static void DecomposeReferenceModel(TQ3Object theModel)
{
	gCurrentSkeleton->numDecomposedTriMeshes = 0;
	gCurrentSkeleton->numDecomposedPoints = 0;
	gCurrentSkeleton->numDecomposedNormals = 0;


		/* DO SUBRECURSIVE SCAN */
		
	DecompRefMo_Recurse(theModel);

}


/***************** DECOM REF MO: RECURSE ***************************/

static void DecompRefMo_Recurse(TQ3Object inObj)
{
TQ3GroupPosition	position;
TQ3Object   		newObj;
TQ3ObjectType		oType;

				/* SEE IF FOUND GEOMETRY */

	if (Q3Object_IsType(inObj,kQ3ShapeTypeGeometry))
	{
		oType = Q3Geometry_GetType(inObj);									// get geometry type
		switch(oType)
		{
			case	kQ3GeometryTypeTriMesh:
					DecomposeATriMesh(inObj);
					break;
		}
	}
	else
	
			/* SEE IF RECURSE SUB-GROUP */

	if (Q3Object_IsType(inObj,kQ3ShapeTypeGroup))
 	{
  		for (Q3Group_GetFirstPosition(inObj, &position); position != nil;	// scan all objects in group
  			 Q3Group_GetNextPosition(inObj, &position))	
 		{
   			Q3Group_GetPositionObject (inObj, position, &newObj);			// get object from group
			if (newObj != NULL)
   			{
    			DecompRefMo_Recurse(newObj);								// sub-recurse this object
    			Q3Object_Dispose(newObj);									// dispose local ref
   			}
  		}
	}
}


/******************* DECOMPOSE A TRIMESH ***********************/

static void DecomposeATriMesh(TQ3Object theTriMesh)
{
TQ3Status			status;
unsigned long		numVertecies,vertNum;
TQ3Point3D			*vertexList;
long				i,n,refNum,pointNum;
TQ3Vector3D			*normalPtr;
TQ3TriMeshData		*triMeshData;
DecomposedPointType	*decomposedPoint;

	n = gCurrentSkeleton->numDecomposedTriMeshes;												// get index into list of trimeshes
	GAME_ASSERT(n < MAX_DECOMPOSED_TRIMESHES);

			/* GET TRIMESH DATA */
			
	status = Q3TriMesh_GetData(theTriMesh, &gCurrentSkeleton->decomposedTriMeshes[n]);			// get trimesh data
	GAME_ASSERT(status);

	triMeshData = &gCurrentSkeleton->decomposedTriMeshes[n];
		
	numVertecies = triMeshData->numPoints;														// get # verts in trimesh
	vertexList = triMeshData->points;															// point to vert list
	normalPtr  = (TQ3Vector3D *)(triMeshData->vertexAttributeTypes[0].data); 					// point to normals

				/*******************************/
				/* EXTRACT VERTECIES & NORMALS */
				/*******************************/
				
	for (vertNum = 0; vertNum < numVertecies; vertNum++)
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
long	numTriMeshes,i,skelType;
SkeletonObjDataType	*currentSkelObjData;

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
	gCurrentSkelObjData = currentSkelObjData = theNode->Skeleton;
	if (gCurrentSkelObjData == nil)
		return;
	
	gCurrentSkeleton = currentSkelObjData->skeletonDefinition;
	GAME_ASSERT(gCurrentSkeleton);

	if (currentSkelObjData->JointsAreGlobal)
		Q3Matrix4x4_SetIdentity(&gMatrix);
	else
		gMatrix = theNode->BaseTransformMatrix;	

	gBBox.min.x = gBBox.min.y = gBBox.min.z = 10000000;
	gBBox.max.x = gBBox.max.y = gBBox.max.z = -gBBox.min.x;								// init bounding box calc
	
	if (gCurrentSkeleton->Bones[0].parentBone != NO_PREVIOUS_JOINT)
		DoFatalAlert("UpdateSkinnedGeometry: joint 0 isnt base - fix code Brian!");
	
	skelType = theNode->Type;
		
	UpdateSkinnedGeometry_Recurse(0, skelType);											// start @ base
	
			/* UPDATE ALL TRIMESH BBOXES */
			
	numTriMeshes = currentSkelObjData->skeletonDefinition->numDecomposedTriMeshes;
	
	for (i = 0; i < numTriMeshes; i++)
	{
		gLocalTriMeshesOfSkelType[skelType][i].bBox = gBBox;				// apply to local copy of trimesh
	}
}


/******************** UPDATE SKINNED GEOMETRY: RECURSE ************************/

static void UpdateSkinnedGeometry_Recurse(short joint, short skelType)
{
long					numChildren,numPoints,p,i,numRefs,r,triMeshNum,p2,c,numNormals,n;
TQ3Matrix4x4			oldM;
TQ3Vector3D				*normalAttribs;
BoneDefinitionType		*bonePtr;
float					minX,maxX,maxY,minY,maxZ,minZ;
long					numDecomposedPoints,numDecomposedNormals;
float					m00,m01,m02,m10,m11,m12,m20,m21,m22,m30,m31,m32;
float					newX,newY,newZ;
SkeletonObjDataType		*currentSkelObjData = gCurrentSkelObjData;
const SkeletonDefType	*currentSkeleton = gCurrentSkeleton;
const float				*jointMat;
float					*matPtr;
DecomposedPointType		*decomposedPointList = currentSkeleton->decomposedPointList;
TQ3TriMeshData			*localTriMeshes = &gLocalTriMeshesOfSkelType[skelType][0];

	minX = minY = minZ = 10000000;
	maxX = maxY = maxZ = -minX;									// calc local bbox with registers for speed

	numDecomposedPoints = currentSkeleton->numDecomposedPoints;
	numDecomposedNormals = currentSkeleton->numDecomposedNormals;

				/*********************************/
				/* FACTOR IN THIS JOINT'S MATRIX */
				/*********************************/
				
	jointMat = &currentSkelObjData->jointTransformMatrix[joint].value[0][0];
	matPtr = &gMatrix.value[0][0];
	
	if (!gCurrentSkelObjData->JointsAreGlobal)
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

			normalAttribs = (TQ3Vector3D *)(localTriMeshes[triMeshNum].vertexAttributeTypes[0].data);	// point to normals attribute list in local trimesh
			normalAttribs[p2] = gTransformedNormals[n];								// copy transformed normal into triMesh
		}
		else																		// handle multi-case
		{		
			for (r = 0; r < numRefs; r++)
			{		
				triMeshNum = decomposedPointList[i].whichTriMesh[r];					
				p2 = decomposedPointList[i].whichPoint[r];								
				n = decomposedPointList[i].whichNormal[r];								

				normalAttribs = (TQ3Vector3D *)(localTriMeshes[triMeshNum].vertexAttributeTypes[0].data);
				normalAttribs[p2] = gTransformedNormals[n];
			}
		}
	}

			/************************/
			/* TRANSFORM THE POINTS */
			/************************/

	if (!gCurrentSkelObjData->JointsAreGlobal)
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
	
			localTriMeshes[triMeshNum].points[p2].x = newX;								// set the point in local copy of trimesh
			localTriMeshes[triMeshNum].points[p2].y = newY;
			localTriMeshes[triMeshNum].points[p2].z = newZ;
		}
		else																			// multi-refs
		{		
			for (r = 0; r < numRefs; r++)
			{
				triMeshNum = decomposedPointList[i].whichTriMesh[r];					
				p2 = decomposedPointList[i].whichPoint[r];								
		
				localTriMeshes[triMeshNum].points[p2].x = newX;	
				localTriMeshes[triMeshNum].points[p2].y = newY;
				localTriMeshes[triMeshNum].points[p2].z = newZ;
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
		UpdateSkinnedGeometry_Recurse(currentSkeleton->childIndecies[joint][c], skelType);
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










