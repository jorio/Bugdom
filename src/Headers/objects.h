//
// Object.h
//
#pragma once

#include "qd3d_support.h"

#define INVALID_NODE_FLAG	0xdeadbeef			// put into CType when node is deleted


#define	PLAYER_SLOT		200
#define	ENEMY_SLOT		(PLAYER_SLOT+10)
#define	SLOT_OF_DUMB	3000
#define CAMERA_SLOT		(SLOT_OF_DUMB+1)
#define	INFOBAR_SLOT	(CAMERA_SLOT+1)
#define	MORPH_SLOT		(SLOT_OF_DUMB + 2000)

	
enum
{
	SKELETON_GENRE,
	DISPLAY_GROUP_GENRE,
	CUSTOM_GENRE,
	EVENT_GENRE
};


#define	DEFAULT_GRAVITY		2000.0f

#define	AUTO_FADE_MAX_DIST		(gAutoFadeStartDist+500.0f)
#define	AUTO_FADE_RANGE			(AUTO_FADE_MAX_DIST - gAutoFadeStartDist)


enum
{
	kAttachGeometry_TransferMeshOwnership		= (1 << 1),
	kAttachGeometry_TransferTextureOwnership	= (1 << 2),
	kAttachGeometry_CloneMeshes					= (1 << 3),
};


enum
{
	kDrawOrder_Terrain		= -128,
	kDrawOrder_Cyclorama,
	kDrawOrder_Fences,
	kDrawOrder_Shadows,

	kDrawOrder_Default = 0,

	kDrawOrder_Ripples,
	kDrawOrder_GlowyParticles,

	kDrawOrder_UI,
	kDrawOrder_FadeOverlay,
	kDrawOrder_DebugUI,
};


//========================================================

extern	void InitObjectManager(void);
extern	ObjNode	*MakeNewObject(NewObjectDefinitionType *newObjDef);
extern	void MoveObjects(void);
ObjNode *MakeNewCustomDrawObject(NewObjectDefinitionType *newObjDef, TQ3BoundingSphere *cullSphere, void drawFunc(ObjNode *));
extern	void DrawObjects(const QD3DSetupOutputType *setupInfo);
extern	void DeleteAllObjects(void);
extern	void DeleteObject(ObjNode	*theNode);
extern	void DetachObject(ObjNode *theNode);
extern	void GetObjectInfo(ObjNode *theNode);
extern	void UpdateObject(ObjNode *theNode);
extern	ObjNode *MakeNewDisplayGroupObject(NewObjectDefinitionType *newObjDef);
void AttachGeometryToDisplayGroupObject(ObjNode* theNode, int numMeshes, TQ3TriMeshData** meshList, int flags);
extern	void CreateBaseGroup(ObjNode *theNode);
extern	void UpdateObjectTransforms(ObjNode *theNode);
extern	void MakeObjectTransparent(ObjNode *theNode, float transPercent);
void AttachObject(ObjNode *theNode);

extern	void MoveStaticObject(ObjNode *theNode);

extern	void CalcNewTargetOffsets(ObjNode *theNode, float scale);

//===================


extern	void AllocateCollisionBoxMemory(ObjNode *theNode, short numBoxes);
extern	void CalcObjectBoxFromNode(ObjNode *theNode);
extern	void CalcObjectBoxFromGlobal(ObjNode *theNode);
extern	void SetObjectCollisionBounds(ObjNode *theNode, short top, short bottom, short left,
							 short right, short front, short back);
extern	void UpdateShadow(ObjNode *theNode);
extern	void CheckAllObjectsInConeOfVision(void);
ObjNode	*AttachShadowToObject(ObjNode *theNode, float scaleX, float scaleZ, Boolean checkBlockers);
ObjNode	*AttachGlowShadowToObject(ObjNode *theNode, float scaleX, float scaleZ, Boolean checkBlockers);
extern	void StopObjectStreamEffect(ObjNode *theNode);
extern	void KeepOldCollisionBoxes(ObjNode *theNode);

void DrawCollisionBoxes(const ObjNode* theNode);
