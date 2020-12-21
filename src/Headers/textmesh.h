#pragma once

#include <Quesa.h>

typedef struct
{
	short			slot;
	TQ3Point3D		coord;
	TQ3ColorRGB		color;
	bool	 		centered;
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
