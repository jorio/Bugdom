//
// Terrain.h
//

#pragma once

#include "qd3d_support.h"

#define	MAP_ITEM_MYSTARTCOORD		0				// map item # for my start coords
#define	MAP_ITEM_FIREFLYTARGET		42				// map item # for firefly target
#define	MAP_ITEM_QUEENBEEBASE		49
#define	MAP_ITEM_LADYBUG			1

extern	int				gSuperTileActiveRange;

enum
{
	FLOOR = 0,
	CEILING = 1
};

		/* SUPER TILE MODES */

enum
{
	SUPERTILE_MODE_FREE,
	SUPERTILE_MODE_USED
};

		/* SUPER TILE TEXTURE LODs */

enum
{
	// 3 LODs: 128x128, 64x64, 32x32 (low-detail mode in OS 9 version)
	SUPERTILE_DETAIL_PROGRESSIVE = 0,

	// 1 LOD: 128x128 (high-detail mode in OS 9 version)
	SUPERTILE_DETAIL_SHRUNK = 1,

	// 1 LOD: 160x160 (no texture shrinking)
	SUPERTILE_DETAIL_LOSSLESS = 2,

	// 1 LOD: 160x160 + 32x32 border for seamless texturing
	SUPERTILE_DETAIL_SEAMLESS = 3,

	SUPERTILE_DETAIL_WORST = SUPERTILE_DETAIL_PROGRESSIVE,

#if OSXPPC
	// No NPOT texture support on most PowerPC Macs (at least not on
	// my Mac Mini G4, which apparently only supports OpenGL 1.3)
	SUPERTILE_DETAIL_BEST = SUPERTILE_DETAIL_SHRUNK,
#else
	SUPERTILE_DETAIL_BEST = SUPERTILE_DETAIL_SEAMLESS,
#endif
};

#define	OREOMAP_TILE_SIZE		32 						// pixel w/h of texture tile
#define	TERRAIN_POLYGON_SIZE	160						// size in world units of terrain polygon

#define	TERRAIN_POLYGON_SIZE_Frac	((float)1.0f/(float)TERRAIN_POLYGON_SIZE)

#define	SUPERTILE_SIZE			5  												// size of a super-tile / terrain object zone
#define	NUM_TRIS_IN_SUPERTILE	(SUPERTILE_SIZE * SUPERTILE_SIZE * 2)			// 2 triangles per tile
#define	NUM_VERTICES_IN_SUPERTILE	((SUPERTILE_SIZE+1)*(SUPERTILE_SIZE+1))		// # vertices in a supertile

#define	SUPERTILE_TEXSIZE_SHRUNK		128
#define	SUPERTILE_TEXSIZE_LOSSLESS		(OREOMAP_TILE_SIZE * SUPERTILE_SIZE)
#define	SUPERTILE_TEXSIZE_SEAMLESS		(OREOMAP_TILE_SIZE * (SUPERTILE_SIZE + 2))
#define	SUPERTILE_TEXSIZE_MAX			SUPERTILE_TEXSIZE_SEAMLESS

#define	MAP2UNIT_VALUE			((float)TERRAIN_POLYGON_SIZE/OREOMAP_TILE_SIZE)	//value to xlate Oreo map pixel coords to 3-space unit coords

#define	TERRAIN_SUPERTILE_UNIT_SIZE	(SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE)		// world unit size of a supertile

#define	MAX_SUPERTILE_ACTIVE_RANGE	5					// max value of gSuperTileActiveRange
// The largest map in the game (Night.ter) is 50x40 supertiles == 2000 supertiles.
// Therefore, the maximum useful value for MAX_SUPERTILE_ACTIVE_RANGE is 23 (because (23*2)**2 == 2116 supertiles)

#define SUPERTILE_ACTIVE_RANGE	gSuperTileActiveRange

#define	SUPERTILE_DIST_WIDE		(SUPERTILE_ACTIVE_RANGE*2)
#define	SUPERTILE_DIST_DEEP		(SUPERTILE_ACTIVE_RANGE*2)
#define	MAX_SUPERTILES			(MAX_SUPERTILE_ACTIVE_RANGE*2 * MAX_SUPERTILE_ACTIVE_RANGE*2)


#define	MAX_TERRAIN_TILES		((300*3)+1)										// 10x15 * 3pages + 1 blank/black

#define	MAX_TERRAIN_WIDTH		400
#define	MAX_TERRAIN_DEPTH		400

#define	MAX_SUPERTILES_WIDE		(MAX_TERRAIN_WIDTH/SUPERTILE_SIZE)
#define	MAX_SUPERTILES_DEEP		(MAX_TERRAIN_DEPTH/SUPERTILE_SIZE)

#define MAX_LODS				3
#define MAX_LAYERS				2

//=====================================================================


struct SuperTileMemoryType
{
	Byte				mode;									// free, used, etc.
	Byte				hasLOD[MAX_LODS];						// flag set when LOD exists
	Byte				hiccupTimer;							// timer to delay drawing to avoid hiccup of texture upload
	TQ3Point3D			coord[MAX_LAYERS];						// world coords of supertile center (y for floor & ceiling)
	long				left,back;								// integer coords of back/left corner
	uint32_t			glTextureName[MAX_LAYERS][MAX_LODS];	// OpenGL texture name for floor & ceiling at all LODs
	uint16_t*			textureData[MAX_LAYERS][MAX_LODS];		// pixel data for floor & ceiling at all LODs
	TQ3TriMeshData*		triMeshDataPtrs[MAX_LAYERS];			// trimesh's data for the supertile (floor & ceiling)
	float				radius[MAX_LAYERS];						// radius of this supertile (floor & ceiling)
};
typedef struct SuperTileMemoryType SuperTileMemoryType;

#define	TILENUM_MASK		0x0fff					// b0000111111111111 = mask to filter out tile #
#define	TILE_FLIPX_MASK		(1<<15)
#define	TILE_FLIPY_MASK		(1<<14)
#define	TILE_FLIPXY_MASK 	(TILE_FLIPY_MASK|TILE_FLIPX_MASK)
#define	TILE_ROTATE_MASK	((1<<13)|(1<<12))
#define	TILE_ROT1			(1<<12)
#define	TILE_ROT2			(2<<12)
#define	TILE_ROT3			(3<<12)

#define	EMPTY_SUPERTILE		(-1)


		/* TERRAIN ITEM FLAGS */

enum
{
	ITEM_FLAGS_INUSE	=	(1),
	ITEM_FLAGS_USER1	=	(1<<1),
	ITEM_FLAGS_USER2	=	(1<<1)
};


enum
{
	SPLIT_BACKWARD = 0,
	SPLIT_FORWARD,
	SPLIT_ARBITRARY
};


typedef struct
{
	Byte	splitMode[2];			// split modes for floor & ceiling
}TerrainInfoMatrixType;

typedef struct
{
	float	layerY[2];
}TerrainYCoordType;


//=====================================================================



void CreateSuperTileMemoryList(void);
void DisposeSuperTileMemoryList(void);
extern 	void DisposeTerrain(void);
extern	void DrawTerrain(const QD3DSetupOutputType *setupInfo);
extern	void GetSuperTileInfo(long x, long z, long *superCol, long *superRow, long *tileCol, long *tileRow);
extern	void InitTerrainManager(void);
extern	void ClearScrollBuffer(void);
float	GetTerrainHeightAtCoord(float x, float z, long layer);
void InitCurrentScrollSettings(void);



extern 	void BuildTerrainItemList(void);
extern 	void ScanForPlayfieldItems(long top, long bottom, long left, long right);

Boolean IsPositionOutOfRange(float x, float z);
Boolean IsPositionOutOfRange_Far(float x, float z, float range);

Boolean TrackTerrainItem(ObjNode *theNode);
Boolean TrackTerrainItem_Far(ObjNode* theNode, float range);

void PrimeInitialTerrain(Boolean justReset);
extern 	void FindMyStartCoordItem(void);
void RotateOnTerrain(ObjNode *theNode, float yOffset);
extern	void DoMyTerrainUpdate(void);

void CalcTileNormals(long layer, long row, long col, TQ3Vector3D *n1, TQ3Vector3D *n2);

void CalculateSplitModeMatrix(void);

void DoItemShadowCasting(void);

