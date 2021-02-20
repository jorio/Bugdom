/****************************/
/* 	ENVIRONMENTMAP.C   		*/
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <QD3D.h>
#include <QD3DMath.h>

#include "qd3d_support.h"
#include "misc.h"
#include "environmentmap.h"

extern	QD3DSetupOutputType		*gGameViewInfoPtr;

/****************************/
/*    PROTOTYPES            */
/****************************/

/****************************/
/*    CONSTANTS             */
/****************************/

#define ENVMAP_MAX_VERTICES_PER_MESH	5000

/*********************/
/*    VARIABLES      */
/*********************/

TQ3Param2D				gEnvMapUVs[ENVMAP_MAX_VERTICES_PER_MESH];

/****************************** ENVIRONMENT MAP TRI MESH *****************************************/
//
// After calling this function, draw your trimesh using gEnvMapUVs as texture coordinates
// (instead of the trimesh's actual UVs)

void EnvironmentMapTriMesh(
		const TQ3TriMeshData *mesh,
		const TQ3Matrix4x4* transform)
{
	if (!mesh->hasTexture)
		return;

	TQ3Matrix4x4		invTranspose;
#if 1
	printf("TODO NOQUESA: EnvMapTriMesh camPlacement\n");
	TQ3Point3D bogus = (TQ3Point3D){0,0,0};
	const TQ3Point3D*	camCoord = &bogus;
#else
	const TQ3Point3D*	camCoord = &gGameViewInfoPtr->cameraPlacement.cameraLocation;
#endif

	GAME_ASSERT(transform);
	GAME_ASSERT(mesh->numPoints <= ENVMAP_MAX_VERTICES_PER_MESH);

	Q3Matrix4x4_Invert(transform, &invTranspose);						// calc inverse-transpose matrix
	Q3Matrix4x4_Transpose(&invTranspose, &invTranspose);

		/****************************/
		/* CALC UVS FOR EACH VERTEX */
		/****************************/	

	for (int vertNum = 0; vertNum < mesh->numPoints; vertNum++)
	{
		TQ3Vector3D surfaceNormal;

					/* TRANSFORM VERTEX NORMAL */

		Q3Vector3D_Transform(&mesh->vertexNormals[vertNum], &invTranspose, &surfaceNormal);
		Q3Vector3D_Normalize(&surfaceNormal, &surfaceNormal);

					/* CALC VECTOR TO VERTEX */

		TQ3Vector3D eyeVector;
		Q3Vector3D_Transform((const TQ3Vector3D*)&mesh->points[vertNum], transform, &eyeVector);
		eyeVector.x -= camCoord->x;
		eyeVector.y -= camCoord->y;
		eyeVector.z -= camCoord->z;

					/* REFLECT VECTOR AROUND VERTEX NORMAL */
		// Reflection vector: N(2(N.V)) - V
		// N - Surface Normal
		// V - View Direction
		float dot = 2.0f * Q3Vector3D_Dot(&surfaceNormal, &eyeVector);	// compute 2(N.V)
		TQ3Vector3D reflectedVector =
		{
			surfaceNormal.x * dot - eyeVector.x,
			surfaceNormal.y * dot - eyeVector.y,
			surfaceNormal.z * dot - eyeVector.z,
		};
		Q3Vector3D_Normalize(&reflectedVector, &reflectedVector);		// normalize result

					/* CALC UV */

		gEnvMapUVs[vertNum].u = (reflectedVector.x * .5f) + .5f;
		gEnvMapUVs[vertNum].v = (reflectedVector.y * .5f) + .5f;
	}
}
