/****************************/
/* PATCH 3DMF MODELS        */
/* Source port addition     */
/****************************/

/*
 * These functions tweak some models from the game
 * on a case-by-case basis in order to:
 *
 * - enhance performance by adding add an alpha test
 *   so they aren't considered transparent by Quesa
 *   (which would knock them off the fast rendering path)
 *
 * - make the game look better by removing unsightly seams
 *   by adding UV clamping
 */

/****************************/
/*    EXTERNALS             */
/****************************/

#include "bugdom.h"

#include <string.h>				// strcasecmp
#ifdef _MSC_VER
#define strncasecmp _strnicmp
	#define strcasecmp _stricmp
#endif

extern	const TQ3Float32	gTextureAlphaThreshold;

/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	SETUVCLAMP_WRAP_BOTH	= 0,
	SETUVCLAMP_CLAMPU_WRAPV	= 1,
	SETUVCLAMP_WRAPU_CLAMPV	= 2,
	SETUVCLAMP_CLAMP_BOTH	= 3,
};

/****************************/
/*    STATIC FUNCTIONS      */
/****************************/

static void SetUVClamp(TQ3TriMeshData triMeshData, void* userData)
{
	bool uClamp = ((uintptr_t)userData) & 1;
	bool vClamp = ((uintptr_t)userData) & 2;

	if (!Q3AttributeSet_Contains(triMeshData.triMeshAttributeSet, kQ3AttributeTypeSurfaceShader))
		return;

	TQ3SurfaceShaderObject	shader;
	TQ3Status 				status;

	status = Q3AttributeSet_Get(triMeshData.triMeshAttributeSet, kQ3AttributeTypeSurfaceShader, &shader);
	GAME_ASSERT(status);

	Q3Shader_SetUBoundary(shader, uClamp ? kQ3ShaderUVBoundaryClamp : kQ3ShaderUVBoundaryWrap);
	Q3Shader_SetVBoundary(shader, vClamp ? kQ3ShaderUVBoundaryClamp : kQ3ShaderUVBoundaryWrap);

	/* DISPOSE REFS */

	Q3Object_Dispose(shader);
}

/**************************** QD3D: SET TEXTURE ALPHA THRESHOLD *******************************/
//
// Instructs Quesa to discard transparent texels so meshes don't get knocked off fast rendering path.
//
// The game has plenty of models with textures containing texels that are either fully-opaque
// or fully-transparent (e.g.: the ladybug's eyelashes and wings).
//
// Normally, textures that are not fully-opaque would make Quesa consider the entire mesh
// as having transparency. This would take the mesh out of the fast rendering path, because Quesa
// needs to depth sort the triangles in transparent meshes.
//
// In the case of textures that are either fully-opaque or fully-transparent, though, we don't
// need that extra depth sorting. The transparent texels can simply be discarded instead.

static void SetAlphaTest(TQ3TriMeshData triMeshData, void* userData_thresholdFloatPtr)
{
	// SEE IF HAS A TEXTURE
	if (Q3AttributeSet_Contains(triMeshData.triMeshAttributeSet, kQ3AttributeTypeSurfaceShader))
	{
		TQ3SurfaceShaderObject	shader;
		TQ3TextureObject		texture;
		TQ3Mipmap				mipmap;
		TQ3Status				status;

		status = Q3AttributeSet_Get(triMeshData.triMeshAttributeSet, kQ3AttributeTypeSurfaceShader, &shader);
		GAME_ASSERT(status);

		/* ADD ALPHA TEST ELEMENT */

		status = Q3Object_AddElement(shader, kQ3ElementTypeTextureShaderAlphaTest, userData_thresholdFloatPtr);
		GAME_ASSERT(status);

		/* GET MIPMAP & APPLY EDGE PADDING TO IMAGE */
		/* (TO AVOID BLACK SEAMS WHERE TEXELS ARE BEING DISCARDED) */

		status = Q3TextureShader_GetTexture(shader, &texture);
		GAME_ASSERT(status);

		status = Q3MipmapTexture_GetMipmap(texture, &mipmap);
		GAME_ASSERT(status);

		if (mipmap.pixelType == kQ3PixelTypeARGB16 ||			// Edge padding only effective if image has alpha channel
			mipmap.pixelType == kQ3PixelTypeARGB32)
		{
			ApplyEdgePadding(&mipmap);
		}

		/* DISPOSE REFS */

		Q3Object_Dispose(texture);
		Q3Object_Dispose(shader);
	}
}

/****************************/
/*    PUBLIC FUNCTIONS      */
/****************************/

void PatchSkeleton3DMF(const char* cName, TQ3Object newModel)
{
	// Discard transparent texels for performance
	ForEachTriMesh(newModel, SetAlphaTest, (void *) &gTextureAlphaThreshold, ~0ull);

	// Remove seams
	if (!strcasecmp(cName, "ANTKING.3DMF")			// Fire on head has seam
		|| !strcasecmp(cName, "WORKERBEE.3DMF"))	// Corner of wing has seam
	{
		ForEachTriMesh(newModel, SetUVClamp, (void *) SETUVCLAMP_CLAMP_BOTH, ~0ull);
	}
}

void PatchGrouped3DMF(const char* cName, TQ3Object* objects, int nObjects)
{
	/************************************************************/
	/* ADD UV CLAMPING IN SOME SURFACE SHADERS TO PREVENT SEAMS */
	/* AND DETERMINE WHICH MESHES NEED TRUE ALPHA BLENDING      */
	/************************************************************/

	// Alpha TESTING (rather than blending) is going to be enabled by default on all meshes.
	uint64_t alphaTestObjectMask = ~0u;

#define TEXTURE_NEEDS_ALPHA_BLENDING(objtype) alphaTestObjectMask &= ~(1 << (objtype))
#define PATCH_CYCLORAMA(objtype) ForEachTriMesh(objects[(objtype)], SetUVClamp, (void*)SETUVCLAMP_WRAPU_CLAMPV, ~0ull);

	if (!strcasecmp(cName, "ANTHILL_MODELS.3DMF"))
	{
		TEXTURE_NEEDS_ALPHA_BLENDING(ANTHILL_MObjType_GasCloud);
		TEXTURE_NEEDS_ALPHA_BLENDING(ANTHILL_MObjType_Staff);
	}
	else if (!strcasecmp(cName, "BONUSSCREEN.3DMF"))
	{
		PATCH_CYCLORAMA(BONUS_MObjType_Background);
	}
	else if (!strcasecmp(cName, "FOREST_MODELS.3DMF"))
	{
		PATCH_CYCLORAMA(FOREST_MObjType_Cyc);
	}
	else if (!strcasecmp(cName, "GLOBAL_MODELS1.3DMF"))
	{
		TEXTURE_NEEDS_ALPHA_BLENDING(GLOBAL1_MObjType_Shadow);
		TEXTURE_NEEDS_ALPHA_BLENDING(GLOBAL1_MObjType_Ripple);
		TEXTURE_NEEDS_ALPHA_BLENDING(GLOBAL1_MObjType_ShockWave);
	}
	else if (!strcasecmp(cName, "HIGHSCORES.3DMF"))
	{
		ForEachTriMesh(objects[0], SetUVClamp, (void *) SETUVCLAMP_WRAPU_CLAMPV, 1 << 7);	// Grass fence
	}
	else if (!strcasecmp(cName, "LAWN_MODELS1.3DMF"))
	{
		PATCH_CYCLORAMA(LAWN1_MObjType_Cyc);
	}
	else if (!strcasecmp(cName, "MAINMENU.3DMF"))
	{
		ForEachTriMesh(objects[0], SetUVClamp, (void *) SETUVCLAMP_CLAMP_BOTH, ~0ull);	// Icon
		// don't touch #1
		ForEachTriMesh(objects[2], SetUVClamp, (void *) SETUVCLAMP_CLAMP_BOTH, ~0ull);	// Icon
		ForEachTriMesh(objects[3], SetUVClamp, (void *) SETUVCLAMP_CLAMP_BOTH, ~0ull);	// Icon
		ForEachTriMesh(objects[4], SetUVClamp, (void *) SETUVCLAMP_CLAMP_BOTH, ~0ull);	// Icon
		ForEachTriMesh(objects[5], SetUVClamp, (void *) SETUVCLAMP_CLAMP_BOTH, ~0ull);	// Icon
		ForEachTriMesh(objects[6], SetUVClamp, (void *) SETUVCLAMP_CLAMP_BOTH, ~0ull);	// Icon
		PATCH_CYCLORAMA(7);
	}
	else if (!strcasecmp(cName, "NIGHT_MODELS.3DMF"))
	{
		PATCH_CYCLORAMA(NIGHT_MObjType_Cyc);
		TEXTURE_NEEDS_ALPHA_BLENDING(NIGHT_MObjType_GasCloud);
		TEXTURE_NEEDS_ALPHA_BLENDING(NIGHT_MObjType_GlowShadow);
		TEXTURE_NEEDS_ALPHA_BLENDING(NIGHT_MObjType_FireFlyGlow);
	}
	else if (!strcasecmp(cName, "POND_MODELS.3DMF"))
	{
		ForEachTriMesh(objects[POND_MObjType_LilyPad], SetUVClamp, (void *) SETUVCLAMP_CLAMP_BOTH, ~0ull);	// lilypad has seam in middle
	}
	else if (!strcasecmp(cName, "TITLE.3DMF"))
	{
		ForEachTriMesh(objects[0], SetUVClamp, (void *) SETUVCLAMP_WRAPU_CLAMPV, 1 << 5);	// Grass fence
	}
	else if (!strcasecmp(cName, "WINLOSE.3DMF"))
	{
		ForEachTriMesh(objects[1], SetUVClamp, (void *) SETUVCLAMP_WRAPU_CLAMPV, 1 << 0);	// WINLOSE_MObjType_WinRoom: Grass fence
	}

#undef TEXTURE_NEEDS_ALPHA_BLENDING
#undef PATCH_CYCLORAMA

	/*********************************************************************/
	/* DISCARD TRANSPARENT TEXELS FOR PERFORMANCE (ALPHA TESTING)        */
	/* except on objects that need alpha blending, which we masked above */
	/*********************************************************************/

	for (int i = 0; i < nObjects; i++)
	{
		if (alphaTestObjectMask & (1uLL << i))  // should we apply the alpha test to this object?
		{
			ForEachTriMesh(objects[i], SetAlphaTest, (void *) &gTextureAlphaThreshold, ~0ull);
		}
	}
}