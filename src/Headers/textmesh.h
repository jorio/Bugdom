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
	TQ3ColorRGBA	color;
	int 	 		align;
	float 			scale;
	float 			letterSpacing;
	bool			withShadow;
	TQ3ColorRGBA	shadowColor;
	TQ3Vector2D		shadowOffset;
} TextMeshDef;

void TextMesh_Init(void);
void TextMesh_Shutdown(void);
void TextMesh_FillDef(TextMeshDef* def);
void TextMesh_Create(const TextMeshDef* def, const char* text);
