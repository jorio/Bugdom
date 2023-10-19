// PICK.C
// (C) 2021 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include "game.h"

#define MAX_TRANSFORMED_POINTS 1024

// Set to 1 to debug pickable quads
#define DRAW_PICKABLE_QUADS 0

/********** CREATE CLICKABLE RECTANGULAR OBJNODE **********/

ObjNode* NewPickableQuad(TQ3Point3D coord, float width, float height, int32_t pickID)
{
	gNewObjectDefinition.genre	= DISPLAY_GROUP_GENRE;
	gNewObjectDefinition.group 	= MODEL_GROUP_ILLEGAL;
	gNewObjectDefinition.slot	= 0;
	gNewObjectDefinition.coord	= coord;
	gNewObjectDefinition.flags	= STATUS_BIT_NULLSHADER | STATUS_BIT_NOZWRITE | STATUS_BIT_HIDDEN;
	gNewObjectDefinition.moveCall = nil;
	gNewObjectDefinition.rot	= 0;
	gNewObjectDefinition.scale = 1.0f;
	ObjNode* quadNode = MakeNewObject(&gNewObjectDefinition);

	TQ3TriMeshData* quadMesh = MakeQuadMesh(1, width, height);
	quadMesh->texturingMode = kQ3TexturingModeOff;
	AttachGeometryToDisplayGroupObject(quadNode, 1, &quadMesh, kAttachGeometry_TransferMeshOwnership);

	UpdateObjectTransforms(quadNode);

	quadNode->IsPickable = true;
	quadNode->PickID = pickID;

#if DRAW_PICKABLE_QUADS
	quadNode->StatusBits &= ~STATUS_BIT_HIDDEN;
#endif

	return quadNode;
}

/********** NUKE ALL PICKABLE OBJNODE **********/

void DisposePickableObjects(void)
{
	ObjNode* node = gFirstNodePtr;
	while (node)
	{
		ObjNode* nextNode = node->NextNode;
		if (node->IsPickable)
			DeleteObject(node);
		node = nextNode;
	}
}

/******************** PICK OBJECT ********************/
//
// INPUT: mouseX, mouseY = point of click in screen space
//
// OUTPUT: TRUE = something got picked
//			pickID = pickID of picked object
//

bool PickObject(int mouseX, int mouseY, int32_t *pickID)
{
	TQ3Point3D transformedPoints[MAX_TRANSFORMED_POINTS];

	TQ3Point3D mouse = {mouseX, mouseY, 0};
	Q3Point3D_Transform(&mouse, &gWindowToFrustum, &mouse);

	for (ObjNode* node = gFirstNodePtr; node; node = node->NextNode)
	{
		if (!node->IsPickable)
			continue;

		TQ3Matrix4x4 nodeTransform;
		Q3Matrix4x4_Multiply(&node->BaseTransformMatrix, &gCameraWorldToFrustumMatrix, &nodeTransform);

		for (int meshID = 0; meshID < node->NumMeshes; meshID++)
		{
			const TQ3TriMeshData* mesh = node->MeshList[meshID];

			GAME_ASSERT(mesh->numPoints <= MAX_TRANSFORMED_POINTS);

			Q3Point3D_To3DTransformArray(mesh->points, &nodeTransform, transformedPoints, mesh->numPoints);

			for (int t = 0; t < mesh->numTriangles; t++)
			{
				const uint32_t* pointIndices = mesh->triangles[t].pointIndices;
				const TQ3Point3D* p0 = &transformedPoints[pointIndices[0]];
				const TQ3Point3D* p1 = &transformedPoints[pointIndices[1]];
				const TQ3Point3D* p2 = &transformedPoints[pointIndices[2]];
				if (IsPointInTriangle(mouse.x, mouse.y, p0->x, p0->y, p1->x, p1->y, p2->x, p2->y))
				{
					*pickID = node->PickID;
					return true;
				}
			}
		}
	}

	return false;
}

