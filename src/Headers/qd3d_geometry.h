//
// qd3d_geometry.h
//

#include "qd3d_support.h"


enum
{
	PARTICLE_MODE_BOUNCE = (1),
	PARTICLE_MODE_UPTHRUST = (1<<1),
	PARTICLE_MODE_HEAVYGRAVITY = (1<<2),
	PARTICLE_MODE_NULLSHADER = (1<<3)
};


#define	MAX_PARTICLES2		80

typedef struct
{
	Boolean					isUsed;
	TQ3Vector3D				rot,rotDelta;
	TQ3Point3D				coord,coordDelta;
	float					decaySpeed,scale;
	Byte					mode;
	TQ3Matrix4x4			matrix;
	
	TQ3TriMeshData			triMesh;
	TQ3Point3D				points[3];
	TQ3TriMeshTriangleData	triangle;
	TQ3Param2D				uvs[3];
	TQ3Vector3D				vertNormals[3],faceNormal;
	TQ3TriMeshAttributeData	vertAttribs[2];

}ParticleType;

extern	float QD3D_CalcObjectRadius(TQ3Object theObject);
extern	void QD3D_CalcObjectBoundingBox(QD3DSetupOutputType *setupInfo, TQ3Object theObject, TQ3BoundingBox	*boundingBox);
extern	void QD3D_ExplodeGeometry(ObjNode *theNode, float boomForce, Byte particleMode, long particleDensity, float particleDecaySpeed);
extern	void QD3D_ReplaceGeometryTexture(TQ3Object obj, TQ3SurfaceShaderObject theShader);
void QD3D_ScrollUVs(TQ3Object theObject, float du, float dv, short whichShader);
extern	void QD3D_DuplicateTriMeshData(TQ3TriMeshData *inData, TQ3TriMeshData *outData);
extern	void QD3D_FreeDuplicateTriMeshData(TQ3TriMeshData *inData);
extern	void QD3D_InitParticles(void);
extern	void QD3D_MoveParticles(void);
void QD3D_DrawParticles(const QD3DSetupOutputType *setupInfo);
extern	void QD3D_CopyTriMeshData(const TQ3TriMeshData *inData, TQ3TriMeshData *outData);
extern	void QD3D_FreeCopyTriMeshData(TQ3TriMeshData *data);
void QD3D_CalcObjectBoundingSphere(QD3DSetupOutputType *setupInfo, TQ3Object theObject, TQ3BoundingSphere *sphere);

void ForEachTriMesh(TQ3Object root, void (*callback)(TQ3TriMeshData triMeshData, void* userData), void* userData);

void QD3D_SetTextureAlphaThreshold_TriMesh(TQ3TriMeshData root, void* userData_thresholdFloatPtr);
void QD3D_ClearDiffuseColor_TriMesh(TQ3TriMeshData triMeshData);
void QD3D_SetUVClamp_TriMesh(TQ3TriMeshData triMeshData, Boolean uClamp, Boolean vClamp);
