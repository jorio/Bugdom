/****************************/
/* 	ENVIRONMENTMAP.C   		*/
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	Boolean	gCanDo_frsqrte;
extern	TQ3Matrix4x4 		gWorkMatrix;
extern	ObjNode			*gFirstNodePtr;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	TQ3TriMeshData	**gLocalTriMeshesOfSkelType;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void CalcEnvMap_Recurse(TQ3Object obj);
static void EnvironmentMapTriMesh(TQ3Object theTriMesh, TQ3TriMeshData *inData);
inline void ReflectVector(const float viewX, const float viewY, const float viewZ, const TQ3Vector3D *N,
				 TQ3Vector3D *out);

/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_REFLECTIONMAP_QUEUESIZE	60



/*********************/
/*    VARIABLES      */
/*********************/


static short			gReflectionMapQueueSize = 0;
static TQ3TriMeshData	gReflectionMapQueue[MAX_REFLECTIONMAP_QUEUESIZE];
static TQ3TriMeshData	*gReflectionMapQueue2[MAX_REFLECTIONMAP_QUEUESIZE];		// ptr to skeleton's trimesh data
static TQ3Point3D	gCamCoord = {0,0,0};


/******************* INIT REFLECTION MAP QUEUE ************************/

void InitReflectionMapQueue(void)
{
short	i;

	for (i=0; i < gReflectionMapQueueSize; i++)								// free the data
	{
		if (gReflectionMapQueue2[i] == nil)
			Q3TriMesh_EmptyData(&gReflectionMapQueue[i]);		
	}

	gReflectionMapQueueSize = 0;
}


/******************* SUBMIT REFLECTION MAP QUEUE ************************/

void SubmitReflectionMapQueue(const QD3DSetupOutputType *viewInfo)
{
short	i;
TQ3Status	status;

	for (i=0; i < gReflectionMapQueueSize; i++)
	{
		if (gReflectionMapQueue2[i])
			status = Q3TriMesh_Submit(gReflectionMapQueue2[i], viewInfo->viewObject);			
		else
			status = Q3TriMesh_Submit(&gReflectionMapQueue[i], viewInfo->viewObject);	
		if (status != kQ3Success)
			DoFatalAlert("\pSubmitReflectionMapQueue: Q3TriMesh_Submit failed!");
	}
		
}



/************ CALC ENVIRONMENT MAPPING COORDS ************/

void CalcEnvironmentMappingCoords(TQ3Point3D *camCoord)
{
ObjNode	*thisNodePtr;

	gCamCoord = *camCoord;

	InitReflectionMapQueue();	

				/****************************************/
				/* UPDATE OBJECTS WITH ENVIRONMENT MAPS */
				/****************************************/

	thisNodePtr = gFirstNodePtr;
	
	if (thisNodePtr == nil)
		return;
		
	do
	{
		if (thisNodePtr->StatusBits & STATUS_BIT_REFLECTIONMAP)
		{
			
			switch(thisNodePtr->Genre)
			{
				case	SKELETON_GENRE:			
//						n = thisNodePtr->Skeleton->skeletonDefinition->numDecomposedTriMeshes;
//						for (i = 0; i < n; i++)
//							EnvironmentMapTriMesh(nil,&gLocalTriMeshes[i]);
						break;
		
				default:					
						Q3Matrix4x4_SetIdentity(&gWorkMatrix);					// init to identity matrix
						CalcEnvMap_Recurse(thisNodePtr->BaseGroup);				// recurse this one
			}
		}
			
		thisNodePtr = thisNodePtr->NextNode;									// next node
	}
	while (thisNodePtr != nil);


}



/****************** CALC ENV MAP_RECURSE *********************/

static void CalcEnvMap_Recurse(TQ3Object obj)
{
TQ3Matrix4x4		transform;
TQ3GroupPosition	position;
TQ3Object   		object,baseGroup;
TQ3Matrix4x4  		stashMatrix;


				/*******************************/
				/* SEE IF ACCUMULATE TRANSFORM */
				/*******************************/
				
	if (Q3Object_IsType(obj,kQ3ShapeTypeTransform))
	{
  		Q3Transform_GetMatrix(obj,&transform);
  		MatrixMultiply(&transform,&gWorkMatrix,&gWorkMatrix);
 	}
				/*************************/
				/* SEE IF FOUND GEOMETRY */
				/*************************/

	else
	if (Q3Object_IsType(obj,kQ3ShapeTypeGeometry))
	{
		if (Q3Geometry_GetType(obj) == kQ3GeometryTypeTriMesh)
			EnvironmentMapTriMesh(obj,nil);									// map this trimesh
	}
	
			/****************************/
			/* SEE IF RECURSE SUB-GROUP */
			/****************************/

	else
	if (Q3Object_IsType(obj,kQ3ShapeTypeGroup))
 	{
 		baseGroup = obj;
  		stashMatrix = gWorkMatrix;										// push matrix
  		
  		
  		Q3Group_GetFirstPosition(obj, &position);						// get 1st object in group
  		while(position != nil)											// scan all objects in group
 		{
   			Q3Group_GetPositionObject (obj, position, &object);			// get object from group
			if (object != nil)
   			{
					/* RECURSE THIS OBJ */
					
				CalcEnvMap_Recurse(object);								// recurse this object    			
    			Q3Object_Dispose(object);								// dispose local ref
   			}
   			Q3Group_GetNextPosition(obj, &position);					// get next object in group
  		}
  		gWorkMatrix = stashMatrix;										// pop matrix
	}
}



/****************************** ENVIRONMENT MAP TRI MESH *****************************************/
//
// NOTE:  This assumes no face normals, thus if there are face normals, they won't be transformed and weird things will happen.
//
// INPUT:	inData = if set, then input data is coming from already transformed skeleton.
//

static void EnvironmentMapTriMesh(TQ3Object theTriMesh, TQ3TriMeshData *inData)
{
TQ3Status			status;
long				numVertecies,vertNum,numFaceAttribTypes,faceNum,numFaces;
TQ3Point3D			*vertexList;
TQ3TriMeshData		*triMeshDataPtr;
TQ3TriMeshAttributeData	*attribList,*faceAttribList;
short				numVertAttribTypes,a;
TQ3Vector3D			*normals,reflectedVector,*normals2;
TQ3Param2D			*uvList;
Boolean				hasUVs = true;
//TQ3Matrix4x4		invTranspose;
register	float	M00, M01, M02;	
register	float	M10, M11, M12;	
register	float	M20, M21, M22;	
register 	float	x,y,z;

			/* GET POINTER TO TRIMESH DATA STRUCTURE */
			
	if (gReflectionMapQueueSize >= MAX_REFLECTIONMAP_QUEUESIZE)
		DoFatalAlert("\pEnvironmentMapTriMesh: gReflectionMapQueueSize >= MAX_REFLECTIONMAP_QUEUESIZE");
		
				/* GET TRIMESH INFO */
				
	if (inData)
	{
		gReflectionMapQueue2[gReflectionMapQueueSize] = inData;
		triMeshDataPtr = inData;
	}
	else
	{
		triMeshDataPtr = &gReflectionMapQueue[gReflectionMapQueueSize];
		gReflectionMapQueue2[gReflectionMapQueueSize] = nil;
		
		status = Q3TriMesh_GetData(theTriMesh, triMeshDataPtr);						// get trimesh data
		if (status != kQ3Success) 
			DoFatalAlert("\pEnvironmentMapTriMesh: Q3TriMesh_GetData failed!");
	}		
	
	numFaces 		= triMeshDataPtr->numTriangles;
	numVertecies 	= triMeshDataPtr->numPoints;										// get # verts
	vertexList 		= triMeshDataPtr->points;											// point to vert list
	attribList 		= triMeshDataPtr->vertexAttributeTypes;								// point to vert attib list
	numVertAttribTypes = triMeshDataPtr->numVertexAttributeTypes;
	numFaceAttribTypes = triMeshDataPtr->numTriangleAttributeTypes;
	faceAttribList 	= triMeshDataPtr->triangleAttributeTypes;

				/* FIND UV ATTRIBUTE LIST */
				
	for (a = 0; a < numVertAttribTypes; a++)									// scan for uv
	{
		if ((attribList[a].attributeType == kQ3AttributeTypeSurfaceUV) || 
			(attribList[a].attributeType == kQ3AttributeTypeShadingUV))
		{
			uvList = attribList[a].data;										// point to list of normals
			goto got_uv;
		}
	}
	hasUVs = false;																// no uv's

got_uv:

			/* TRANSFORM BOUNDING BOX */
			
	Q3Point3D_To3DTransformArray(&triMeshDataPtr->bBox.min, &gWorkMatrix, &triMeshDataPtr->bBox.min, 2,
							 sizeof(TQ3Point3D), sizeof(TQ3Point3D));				
	

				/* TRANSFORM VERTEX COORDS */
				
	Q3Point3D_To3DTransformArray(vertexList, &gWorkMatrix, vertexList, numVertecies,
								 sizeof(TQ3Point3D), sizeof(TQ3Point3D));				

	
				/* TRANSFORM VERTEX NORMALS */
				//
				// This could be done easily with Q3Vector3D_Transform, but it's slow,
				// so this ugly code does the same thing, but faster
				//

//	Q3Matrix4x4_Invert(&gWorkMatrix, &tempm);							// calc inverse-transpose matrix
//	Q3Matrix4x4_Transpose(&tempm,&invTranspose);			

//	invTranspose = gWorkMatrix;	//---------

	M00 = gWorkMatrix.value[0][0];										// Load matrix values into registers
	M01 = gWorkMatrix.value[0][1];
	M02 = gWorkMatrix.value[0][2];
	M10 = gWorkMatrix.value[1][0];
	M11 = gWorkMatrix.value[1][1];
	M12 = gWorkMatrix.value[1][2];
	M20 = gWorkMatrix.value[2][0];
	M21 = gWorkMatrix.value[2][1];
	M22 = gWorkMatrix.value[2][2];

	for (a = 0; a < numVertAttribTypes; a++)									// scan for normals
	{
		if (attribList[a].attributeType == kQ3AttributeTypeNormal)
		{
			normals = attribList[a].data;										// point to list of normals

			for (vertNum = 0; vertNum < numVertecies; vertNum++)				// transform all normals
			{				
				x = normals[vertNum].x;											// load normal values into regs
				y = normals[vertNum].y;
				z = normals[vertNum].z;
							
				FastNormalizeVector(x * M00 + y * M10 + z * M20,			// do the vector-matrix mult
								x * M01 + y * M11 + z * M21,
								x * M02 + y * M12 + z * M22,
								&normals[vertNum]);								// normalize the result
			}
			break;
		}
	}


				/* TRANSFORM FACE NORMALS */

	for (a = 0; a < numFaceAttribTypes; a++)									// scan for normals
	{
		if (faceAttribList[a].attributeType == kQ3AttributeTypeNormal)
		{
			normals2 = faceAttribList[a].data;									// point to list of normals

			for (faceNum = 0; faceNum < numFaces; faceNum++)				// transform all normals
			{				
				x = normals2[faceNum].x;											// load normal values into regs
				y = normals2[faceNum].y;
				z = normals2[faceNum].z;
							
				FastNormalizeVector(x * M00 + y * M10 + z * M20,			// do the vector-matrix mult
								x * M01 + y * M11 + z * M21,
								x * M02 + y * M12 + z * M22,
								&normals2[faceNum]);								// normalize the result
			}
			break;
		}
	}



		/****************************/
		/* CALC UVS FOR EACH VERTEX */
		/****************************/	

	if (hasUVs)																	// only if has UV's
	{
		float	camX,camY,camZ;
		
		camX = gCamCoord.x;
		camY = gCamCoord.y;
		camZ = gCamCoord.z;
		
		for (vertNum = 0; vertNum < numVertecies; vertNum++)
		{
			float	eyeVectorX,eyeVectorY,eyeVectorZ;
			
					/* CALC VECTOR TO VERTEX */
					
			eyeVectorX = vertexList[vertNum].x - camX;
			eyeVectorY = vertexList[vertNum].y - camY;
			eyeVectorZ = vertexList[vertNum].z - camZ;

		
					/* REFLECT VECTOR AROUND VERTEX NORMAL */

			ReflectVector(eyeVectorX, eyeVectorY, eyeVectorZ,
							&normals[vertNum],&reflectedVector);
		
		
						/* CALC UV */
				
			uvList[vertNum].u = (reflectedVector.x * .5f) + .5f;
			uvList[vertNum].v = (-reflectedVector.y * .5f) + .5f;	

		
		}
	}
	
	gReflectionMapQueueSize++;		
}




/*********************** REFLECT VECTOR *************************/
//
// compute reflection vector 
// which is N(2(N.V)) - V 
// N - Surface Normal 
// V - View Direction 
//
//

inline void ReflectVector(const float viewX, const float viewY, const float viewZ, const TQ3Vector3D *N,
				 TQ3Vector3D *out)
{
float	normalX,normalY,normalZ;
float	dotProduct,reflectedX,reflectedZ,reflectedY;

	normalX = N->x;
	normalY = N->y;
	normalZ = N->z;
							
	/* compute NxV */
	dotProduct = normalX * viewX;
	dotProduct += normalY * viewY;
	dotProduct += normalZ * viewZ;
	
	/* compute 2(NxV) */
	dotProduct += dotProduct;
	
	/* compute final vector */
	reflectedX = normalX * dotProduct - viewX;
	reflectedY = normalY * dotProduct - viewY;
	reflectedZ = normalZ * dotProduct - viewZ;
	
	/* Normalize the result */
		
	FastNormalizeVector(reflectedX,reflectedY,reflectedZ,out);	
}













