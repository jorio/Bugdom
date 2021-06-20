/****************************/
/*   	3DMF.C 		   	    */
/* (c)1996 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

#include <string.h>				// strcasecmp
#ifdef _MSC_VER
#define strncasecmp _strnicmp
	#define strcasecmp _stricmp
#endif


/****************************/
/*    PROTOTYPES            */
/****************************/


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

TQ3MetaFile*				gObjectGroupFile[MAX_3DMF_GROUPS];
GLuint*						gObjectGroupTextures[MAX_3DMF_GROUPS];
TQ3TriMeshFlatGroup			gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
TQ3BoundingSphere	gObjectGroupRadiusList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
TQ3BoundingBox 		gObjectGroupBBoxList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
short				gNumObjectsInGroupList[MAX_3DMF_GROUPS];


/******************* INIT 3DMF MANAGER *************************/

void Init3DMFManager(void)
{
	for (int i = 0; i < MAX_3DMF_GROUPS; i++)
	{
		gObjectGroupFile[i] = nil;
		gObjectGroupTextures[i] = nil;
		gNumObjectsInGroupList[i] = 0;
	}
}


/******************** LOAD GROUPED 3DMF ***********************/
//
// Loads and registers a grouped 3DMF file by reading each element out of the file into
// a master object list.
//
// INPUT: fileName = name of file to load (nil = select one)
//		  groupNum = group # to assign this file.


void LoadGrouped3DMF(FSSpec *spec, Byte groupNum)
{
	GAME_ASSERT(groupNum < MAX_3DMF_GROUPS);

	GAME_ASSERT_MESSAGE(gNumObjectsInGroupList[groupNum] == 0, "3DMF group was not freed before reuse");
	GAME_ASSERT_MESSAGE(!gObjectGroupFile[groupNum], "3DMF group file not freed before reuse");
	GAME_ASSERT_MESSAGE(!gObjectGroupTextures[groupNum], "3DMF group textures not freed before reuse");

			/* LOAD NEW GEOMETRY */

	TQ3MetaFile* the3DMFFile = Q3MetaFile_Load3DMF(spec);
	GAME_ASSERT(the3DMFFile);

	Patch3DMF(spec->cName, the3DMFFile);

	gObjectGroupFile[groupNum] = the3DMFFile;

			/* UPLOAD TEXTURES TO GPU */

	gObjectGroupTextures[groupNum] = (GLuint*) NewPtrClear(the3DMFFile->numTextures * sizeof(GLuint));

	Render_Load3DMFTextures(the3DMFFile, gObjectGroupTextures[groupNum]);

			/* BUILD OBJECT LIST */

	int nObjects = the3DMFFile->numTopLevelGroups;
	GAME_ASSERT(nObjects > 0);
	GAME_ASSERT(nObjects <= MAX_OBJECTS_IN_GROUP);

			/*********************/
			/* SCAN THRU OBJECTS */
			/*********************/

	for (int i = 0; i < nObjects; i++)
	{
		TQ3TriMeshFlatGroup meshList = the3DMFFile->topLevelGroups[i];
		gObjectGroupList[groupNum][i] = meshList;
		GAME_ASSERT(0 != meshList.numMeshes);
		GAME_ASSERT(nil != meshList.meshes);

		QD3D_CalcObjectBoundingSphere(meshList.numMeshes, meshList.meshes, &gObjectGroupRadiusList[groupNum][i]);
		QD3D_CalcObjectBoundingBox(meshList.numMeshes, meshList.meshes, &gObjectGroupBBoxList[groupNum][i]); // save bbox
	}

	gNumObjectsInGroupList[groupNum] = nObjects;					// set # objects.
}


/******************** DELETE 3DMF GROUP **************************/

void Free3DMFGroup(Byte groupNum)
{
	if (gObjectGroupTextures[groupNum] != nil)
	{
		GAME_ASSERT(gObjectGroupFile[groupNum] != nil);
		glDeleteTextures(gObjectGroupFile[groupNum]->numTextures, gObjectGroupTextures[groupNum]);
		DisposePtr((Ptr) gObjectGroupTextures[groupNum]);
		gObjectGroupTextures[groupNum] = nil;
	}

	if (gObjectGroupFile[groupNum] != nil)
	{
		Q3MetaFile_Dispose(gObjectGroupFile[groupNum]);
		gObjectGroupFile[groupNum] = nil;
	}

	memset(gObjectGroupList[groupNum], 0, sizeof(gObjectGroupList[groupNum]));	// make sure to init the entire list to be safe

	gNumObjectsInGroupList[groupNum] = 0;
}


/******************* DELETE ALL 3DMF GROUPS ************************/

void DeleteAll3DMFGroups(void)
{
short	i;

	for (i=0; i < MAX_3DMF_GROUPS; i++)
	{
		if (gNumObjectsInGroupList[i])				// see if this group empty
			Free3DMFGroup(i);
	}
}



/******************* PATCH 3DMF SO IT LOOKS BETTER ************************/

void Patch3DMF(const char* filename, TQ3MetaFile* model)
{
#if 0	// NOQUESA
	// Remove seams
	if (!strcasecmp(cName, "antking.3dmf")			// Fire on head has seam
		|| !strcasecmp(cName, "workerbee.3dmf"))	// Corner of wing has seam
	{
		ForEachTriMesh(newModel, SetUVClamp, (void *) SETUVCLAMP_CLAMP_BOTH, ~0ull);
	}

	if (!strcasecmp(cName, "highscores.3dmf"))
		ForEachTriMesh(objects[0], SetUVClamp, (void *) SETUVCLAMP_WRAPU_CLAMPV, 1 << 7);	// Grass fence

	if (!strcasecmp(cName, "title.3dmf"))
		ForEachTriMesh(objects[0], SetUVClamp, (void *) SETUVCLAMP_WRAPU_CLAMPV, 1 << 5);	// Grass fence

	else if (!strcasecmp(cName, "winlose.3dmf"))
		ForEachTriMesh(objects[1], SetUVClamp, (void *) SETUVCLAMP_WRAPU_CLAMPV, 1 << 0);	// WINLOSE_MObjType_WinRoom: Grass fence
#endif

	// skippy: fill alpha hole in eye texture
	if (!strcasecmp(filename, "skippy.3dmf"))
	{
		GAME_ASSERT(model->numTextures > 1);
		model->textures[1]->pixelType = kQ3PixelTypeRGB16;
	}

	// antking: fill alpha hole in body/eye/crown textures
	if (!strcasecmp(filename, "antking.3dmf"))
	{
		GAME_ASSERT(model->numTextures > 7);
		model->textures[0]->pixelType = kQ3PixelTypeRGB16;	// body 1
		model->textures[1]->pixelType = kQ3PixelTypeRGB16;	// face (including eyes)
		model->textures[2]->pixelType = kQ3PixelTypeRGB16;	// body 2
		model->textures[7]->pixelType = kQ3PixelTypeRGB16;	// crown
	}

	// winlose: fill alpha hole in wall/ground textures
	if (!strcasecmp(filename, "winlose.3dmf"))
	{
		GAME_ASSERT(model->numTextures > 28);
		model->textures[16]->pixelType = kQ3PixelTypeRGB16;	// flower pot top cover
		model->textures[18]->pixelType = kQ3PixelTypeRGB16;	// green
		model->textures[19]->pixelType = kQ3PixelTypeRGB16;	// brown
		model->textures[28]->pixelType = kQ3PixelTypeRGB16;	// purple
	}

	// spider: the black parts of the teeth texture are supposed to be transparent
	if (!strcasecmp(filename, "spider.3dmf"))
	{
		GAME_ASSERT(model->numTextures > 2);
		TQ3Pixmap* teethTexture = model->textures[2];
		teethTexture->pixelType = kQ3PixelTypeARGB16;
		GAME_ASSERT(teethTexture->byteOrder == kQ3EndianLittle); // Pomme should have converted it to little-endian for us
		for (uint32_t p = 0; p < teethTexture->rowBytes * teethTexture->height; p += 2)
		{
			if (teethTexture->image[p] != 0 || teethTexture->image[p+1] != 0)
				teethTexture->image[p+1] |= 0x80;		// alpha bit (little endian)
		}
	}
}
