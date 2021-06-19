// TEXT MESH.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include "game.h"

typedef struct
{
	float x;
	float y;
	float w;
	float h;
	float xoff;
	float yoff;
	float xadv;
} AtlasGlyph;

static GLuint gFontTexture = 0;
static int gNumGlyphs = 0;
static float gLineHeight = 0;
static AtlasGlyph* gAtlasGlyphs = NULL;

static const TextMeshDef gDefaultTextMeshDef =
{
	.coord				= { 0, 0, 0 },
	.scale				= .5f,
	.withShadow			= true,
	.color				= { 1, 1, 1, 1 },
	.shadowColor		= { 0, 0, 0, 1 },
	.slot				= 100,
	.align				= TEXTMESH_ALIGN_CENTER,
	.shadowOffset		= { 2, -2 },
	.letterSpacing		= 0,
};

static void MakeText(
		const TextMeshDef* def,
		const char* text,
		bool isShadow)
{

	float x = 0;
	float y = gLineHeight * .7f;
	float spacing = def->letterSpacing;

	float lineWidth = 0;
	int numQuads = 0;
	for (const char* c = text; *c; c++)
	{
		GAME_ASSERT(*c >= ' ');
		GAME_ASSERT(*c <= '~');
		const AtlasGlyph g = gAtlasGlyphs[*c - ' '];
		lineWidth += g.xadv + spacing;
		if (*c != ' ')
			numQuads++;
	}

	// Adjust start x for text alignment
	if (def->align == TEXTMESH_ALIGN_CENTER)
		x -= lineWidth * .5f;
	else if (def->align == TEXTMESH_ALIGN_RIGHT)
		x -= lineWidth;

	TQ3TriMeshData* mesh = MakeQuadMesh(numQuads, 1.0f, 1.0f);
	mesh->texturingMode = kQ3TexturingModeAlphaBlend;
	mesh->glTextureName = gFontTexture;

	int p = 0;
	for (const char* c = text; *c; c++)
	{
		GAME_ASSERT(*c >= ' ');
		GAME_ASSERT(*c <= '~');
		const AtlasGlyph g = gAtlasGlyphs[*c - ' '];

		if (*c == ' ')
		{
			x += g.xadv + spacing;
			continue;
		}

		float qx = x + g.xoff + g.w*.5f;
		float qy = y - g.yoff - g.h*.5f;

		mesh->points[p + 0] = (TQ3Point3D) { qx - g.w*.5f, qy - g.h*.5f, 0 };
		mesh->points[p + 1] = (TQ3Point3D) { qx + g.w*.5f, qy - g.h*.5f, 0 };
		mesh->points[p + 2] = (TQ3Point3D) { qx + g.w*.5f, qy + g.h*.5f, 0 };
		mesh->points[p + 3] = (TQ3Point3D) { qx - g.w*.5f, qy + g.h*.5f, 0 };
		mesh->vertexUVs[p + 0] = (TQ3Param2D) { g.x/512.0f,		(g.y+g.h)/256.0f };
		mesh->vertexUVs[p + 1] = (TQ3Param2D) { (g.x+g.w)/512.0f,	(g.y+g.h)/256.0f };
		mesh->vertexUVs[p + 2] = (TQ3Param2D) { (g.x+g.w)/512.0f,	g.y/256.0f };
		mesh->vertexUVs[p + 3] = (TQ3Param2D) { g.x/512.0f,		g.y/256.0f };

		x += g.xadv + spacing;
		p += 4;
	}

	GAME_ASSERT(p == mesh->numPoints);

	gNewObjectDefinition.genre		= DISPLAY_GROUP_GENRE;
	gNewObjectDefinition.group 		= MODEL_GROUP_ILLEGAL;
	gNewObjectDefinition.slot		= def->slot;
	gNewObjectDefinition.coord.x 	= def->coord.x + (isShadow ? def->shadowOffset.x*def->scale : 0);
	gNewObjectDefinition.coord.y 	= def->coord.y + (isShadow ? def->shadowOffset.y*def->scale : 0);
	gNewObjectDefinition.coord.z 	= def->coord.z + (isShadow? -0.1f: 0);
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER | STATUS_BIT_NOZWRITE;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0.0f;
	gNewObjectDefinition.scale 		= def->scale;
	ObjNode* textNode = MakeNewObject(&gNewObjectDefinition);
	AttachGeometryToDisplayGroupObject(textNode, 1, &mesh, true, false);
	textNode->RenderModifiers.diffuseColor = isShadow? def->shadowColor: def->color;
	textNode->BoundingSphere.isEmpty = kQ3False;
	textNode->BoundingSphere.radius = def->scale * lineWidth / 2.0f;

	UpdateObjectTransforms(textNode);
}

static void SkipLine(const char** dataPtr)
{
	const char* data = *dataPtr;

	while (*data)
	{
		char c = data[0];
		data++;
		if (c == '\r' && *data != '\n')
			break;
		if (c == '\n')
			break;
	}

	GAME_ASSERT(*data);
	*dataPtr = data;
}

static void ParseSFL(const char* data)
{
	int nArgs = 0;
	int junk = 0;

	SkipLine(&data);	// Skip font name

	nArgs = sscanf(data, "%d %f", &junk, &gLineHeight);
	GAME_ASSERT(nArgs == 2);
	SkipLine(&data);

	SkipLine(&data);	// Skip image filename

	nArgs = sscanf(data, "%d", &gNumGlyphs);
	GAME_ASSERT(nArgs == 1);
	SkipLine(&data);

	gAtlasGlyphs = (AtlasGlyph*) NewPtrClear(gNumGlyphs * sizeof(AtlasGlyph));

	for (int i = 0; i < gNumGlyphs; i++)
	{
		nArgs = sscanf(
				data,
				"%d %f %f %f %f %f %f %f",
				&junk,
				&gAtlasGlyphs[i].x,
				&gAtlasGlyphs[i].y,
				&gAtlasGlyphs[i].w,
				&gAtlasGlyphs[i].h,
				&gAtlasGlyphs[i].xoff,
				&gAtlasGlyphs[i].yoff,
				&gAtlasGlyphs[i].xadv);
		GAME_ASSERT(nArgs == 8);

		SkipLine(&data);
	}
}

void TextMesh_Init(void)
{
	OSErr err;

	gFontTexture = QD3D_LoadTextureFile(3000, kRendererTextureFlags_GrayscaleIsAlpha);

	short refNum = OpenGameFile(":images:textures:3000.sfl");

	// Get number of bytes until EOF
	long eof = 0;
	GetEOF(refNum, &eof);

	// Prep data buffer
	Ptr data = NewPtrClear(eof+1);

	// Read file into data buffer
	err = FSRead(refNum, &eof, data);
	GAME_ASSERT(err == noErr);
	FSClose(refNum);

	ParseSFL(data);
}

void TextMesh_Shutdown(void)
{
	DisposePtr((Ptr) gAtlasGlyphs);
	gAtlasGlyphs = NULL;

	if (gFontTexture)
	{
		glDeleteTextures(1, &gFontTexture);
		gFontTexture = 0;
	}
}

void TextMesh_FillDef(TextMeshDef* def)
{
	*def = gDefaultTextMeshDef;
}

void TextMesh_Create(const TextMeshDef* def, const char* text)
{
	MakeText(def, text, false);
	if (def->withShadow)
		MakeText(def, text, true);
}
