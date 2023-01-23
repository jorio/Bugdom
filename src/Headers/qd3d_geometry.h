//
// qd3d_geometry.h
//

#include "qd3d_support.h"

enum
{
	SHARD_MODE_BOUNCE			= (1),
	SHARD_MODE_UPTHRUST			= (1 << 1),
	SHARD_MODE_HEAVYGRAVITY		= (1 << 2),
	SHARD_MODE_NULLSHADER		= (1 << 3)
};

#define	MAX_SHARDS			80

typedef struct
{
	TQ3Vector3D				rot,rotDelta;
	TQ3Point3D				coord,coordDelta;
	float					decaySpeed,scale;
	Byte					mode;
	TQ3Matrix4x4			matrix;
	TQ3TriMeshData			*mesh;
}ShardType;

void QD3D_CalcObjectBoundingBox(int numMeshes, TQ3TriMeshData** meshList, TQ3BoundingBox* boundingBox);
void QD3D_CalcObjectBoundingSphere(int numMeshes, TQ3TriMeshData** meshList, TQ3BoundingSphere* boundingSphere);
void QD3D_ExplodeGeometry(ObjNode *theNode, float boomForce, Byte shardMode, int shardDensity, float shardDecaySpeed);
void QD3D_MirrorMeshesZ(ObjNode *theNode);
void QD3D_ScrollUVs(TQ3TriMeshData* meshList, float du, float dv);
void QD3D_InitShards(void);
void QD3D_DisposeShards(void);
void QD3D_MoveShards(void);
void QD3D_DrawShards(const QD3DSetupOutputType *setupInfo);

TQ3TriMeshData* MakeQuadMesh(int numQuads, float width, float height);
TQ3TriMeshData* MakeQuadMesh_UI(
		float xyLeft, float xyTop, float xyRight, float xyBottom,
		float uvLeft, float uvTop, float uvRight, float uvBottom);
