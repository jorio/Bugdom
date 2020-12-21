#pragma once

#include <Quesa.h>

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
	TQ3ColorRGB		color;
	int 	 		align;
	float 			scale;
	float			lowercaseScale;
	float			uppercaseScale;
	float 			characterSpacing;
	float 			spaceWidth;
	bool			withShadow;
	TQ3ColorRGB		shadowColor;
	TQ3Vector2D		shadowOffset;
} TextMeshDef;

void TextMesh_Load(void);
void TextMesh_FillDef(TextMeshDef* def);
void TextMesh_Create(const TextMeshDef* def, const char* text);
