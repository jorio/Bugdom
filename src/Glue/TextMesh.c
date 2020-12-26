#include "bugdom.h"

extern	NewObjectDefinitionType		gNewObjectDefinition;
extern	FSSpec						gDataSpec;
extern	TQ3Object					gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];

static const TextMeshDef gDefaultTextMeshDef =
{
	.coord				= { 0, 0, 0 },
	.scale				= 1.0f,
	.withShadow			= true,
	.color				= { 1, 1, 1 },
	.shadowColor		= { 0, 0, 0 },
	.slot				= 100,
	.align				= TEXTMESH_ALIGN_CENTER,
	.shadowOffset		= { 2, -2 },
	.characterSpacing	= 2,
	.spaceWidth			= 16,
	.lowercaseScale		= 0.75f,
	.uppercaseScale		= 1.0f,
};

static void GetTriMeshBoundingBox(TQ3TriMeshData triMeshData, void* userData_outPtrBBox)
{
	TQ3BoundingBox* outBBox = (TQ3BoundingBox*) userData_outPtrBBox;
	*outBBox = triMeshData.bBox;
}

static ObjNode* MakeDiacritic(
		Byte type,
		short slot,
		float x,
		float y,
		float z,
		float scale
		)
{
	gNewObjectDefinition.group 		= MODEL_GROUP_TEXTMESH;
	gNewObjectDefinition.type		= type;
	gNewObjectDefinition.slot		= slot;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0.0f;
	gNewObjectDefinition.scale 		= scale;

	ObjNode* glyph = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	return glyph;
}

static void MakeText(
		const TextMeshDef* def,
		const char* text,
		bool isShadow)
{
	float			x				= def->coord.x + (isShadow ? def->shadowOffset.x*def->scale : 0);
	float 			y				= def->coord.y + (isShadow ? def->shadowOffset.y*def->scale : 0);
	float 			z				= def->coord.z + (isShadow? -0.1f: 0);
	float			startX			= x;

	ObjNode*		firstTextNode	= nil;
	ObjNode*		lastTextNode	= nil;

	TQ3AttributeSet attrs = Q3AttributeSet_New();
	Q3AttributeSet_Add(attrs, kQ3AttributeTypeDiffuseColor, isShadow? &def->shadowColor: &def->color);

	for (; *text; text++)
	{
		char c = *text;
		bool isLower = false;
		int type = SCORES_ObjType_0;

		switch (c)
		{
			case ' ':
				x += def->spaceWidth * def->scale;
				continue;

			case '0':	case '1':	case '2':	case '3':	case '4':
			case '5':	case '6':	case '7':	case '8':	case '9':
				type = SCORES_ObjType_0 + (c - '0'); break;

			case '.':	type = SCORES_ObjType_Period;		break;
			case '#':	type = SCORES_ObjType_Pound;		break;
			case '?':	type = SCORES_ObjType_Question;		break;
			case '!':	type = SCORES_ObjType_Exclamation;	break;
			case '-':	type = SCORES_ObjType_Dash;			break;
			case ',':	type = SCORES_ObjType_Apostrophe;	break;
			case '\'':	type = SCORES_ObjType_Apostrophe;	break;
			case ':':	type = SCORES_ObjType_Colon;		break;
			case '/':	type = SCORES_ObjType_I;			break;
			case '(':	type = SCORES_ObjType_C;			break;
			case ')':	type = SCORES_ObjType_C;			break;
			case '&':	type = SCORES_ObjType_3;			break;

			default:
				if (c >= 'A' && c <= 'Z')
				{
					type = SCORES_ObjType_A + (c - 'A');
				}
				else if (c >= 'a' && c <= 'z')
				{
					type = SCORES_ObjType_A + (c - 'a');
					isLower = true;
				}
				else
				{
					type = SCORES_ObjType_Question;
				}
				break;
		}

		float scale = def->scale * (isLower? def->lowercaseScale: def->uppercaseScale);

		gNewObjectDefinition.group 		= MODEL_GROUP_TEXTMESH;
		gNewObjectDefinition.type		= type;
		gNewObjectDefinition.slot		= def->slot;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= y;
		gNewObjectDefinition.coord.z 	= z;
		gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0.0f;
		gNewObjectDefinition.scale 		= scale;

		ObjNode* glyph = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		// Get glyph width via the bounding box of its mesh
		TQ3BoundingBox glyphBBox;
		glyphBBox.min.x = 0;
		glyphBBox.max.x = 0;
		ForEachTriMesh(glyph->BaseGroup, GetTriMeshBoundingBox, (void *) &glyphBBox);
		float glyphWidth = glyphBBox.max.x - glyphBBox.min.x;

		// Hack to create missing glyph from another with a little transform
		if (c == ',')
		{
			glyph->Coord.y -= 22 * scale;
			glyph->Rot.z = -0.4f;
		}
		else if (c == '/')
		{
			glyph->Scale.y *= 1.4f;
			glyph->Rot.z = -0.4f;
			glyphWidth *= 3.0f;
		}
		else if (c == '(')
		{
			glyph->Scale.x *= 0.35f;
			glyph->Scale.y *= 1.9f;
			glyph->Coord.y += 7 * scale;
			glyphWidth *= 0.7f;
		}
		else if (c == ')')
		{
			glyph->Scale.x *= 0.35f;
			glyph->Scale.y *= 1.9f;
			glyph->Coord.y -= 7 * scale;
			glyph->Rot.z = M_PI;
			glyphWidth *= 0.7f;
		}
		else if (c == '&')
		{
			glyph->Rot.z = M_PI;

			ObjNode* diacritic1 = MakeDiacritic(SCORES_ObjType_Apostrophe, def->slot, x + scale * glyphWidth * 0.4f + scale * glyphWidth / 2.0f, y + scale * 6.0f, z, scale);
			ObjNode* diacritic2 = MakeDiacritic(SCORES_ObjType_Apostrophe, def->slot, x + scale * glyphWidth * 0.4f + scale * glyphWidth / 2.0f, y - scale * 19.5f, z, scale);
			AttachGeometryToDisplayGroupObject(diacritic1, attrs);
			AttachGeometryToDisplayGroupObject(diacritic2, attrs);
		}

		lastTextNode = glyph;
		if (!firstTextNode)
			firstTextNode = glyph;

		AttachGeometryToDisplayGroupObject(glyph, attrs);

		glyph->SpecialL[0] = 0;
		glyph->SpecialF[0] = 0;

		x = glyph->Coord.x
			+ def->characterSpacing * scale
			+ scale * glyphWidth;

		glyph->Coord.x += scale * glyphWidth / 2.0f;
		UpdateObjectTransforms(glyph);
	}

	// Center text horizontally
	if (def->align != TEXTMESH_ALIGN_LEFT)
	{
		float xAlignFactor = def->align == TEXTMESH_ALIGN_CENTER ? 0.5f : 1.0f;
		float totalWidth = x - startX;
		for (ObjNode* node = firstTextNode; node; node = node->NextNode)
		{
			node->Coord.x -= (totalWidth - def->characterSpacing * def->scale) * xAlignFactor;
			UpdateObjectTransforms(node);
			if (node == lastTextNode)
				break;
		}
	}

	Q3Object_Dispose(attrs);
}

void TextMesh_Load(void)
{
	FSSpec spec;

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:HighScores.3dmf", &spec);
	LoadGrouped3DMF(&spec, MODEL_GROUP_TEXTMESH);

	// Nuke color attribute from glyph models so we can draw them in whatever color we like later.
	for (int i = SCORES_ObjType_0; i <= SCORES_ObjType_Colon; i++)
	{
		ForEachTriMesh(gObjectGroupList[MODEL_GROUP_TEXTMESH][i], QD3D_ClearDiffuseColor_TriMesh, NULL);
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