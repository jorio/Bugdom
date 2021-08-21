#pragma once

#include <QD3D.h>

enum
{
	TEXTMESH_ALIGN_LEFT,
	TEXTMESH_ALIGN_CENTER,
	TEXTMESH_ALIGN_RIGHT,
};

typedef struct
{
	short			slot;
	TQ3Point3D		coord;
	TQ3Point3D		meshOrigin;
	TQ3Vector2D		shadowOffset;
	TQ3ColorRGBA	color;
	int 	 		align;
	float 			scale;
	float 			letterSpacing;
	bool			withShadow;
	TQ3ColorRGBA	shadowColor;
} TextMeshDef;

void TextMesh_Init(void);
void TextMesh_Shutdown(void);
void TextMesh_FillDef(TextMeshDef* def);
TQ3TriMeshData* TextMesh_CreateMesh(const TextMeshDef* def, const char* text);
TQ3TriMeshData* TextMesh_SetMesh(const TextMeshDef* def, const char* text, TQ3TriMeshData* recycleMesh);
ObjNode* TextMesh_Create(const TextMeshDef* def, const char* text);
