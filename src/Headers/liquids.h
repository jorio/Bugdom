//
// liquids.h
//

#define	WATER_COLLISION_TOPOFF	75.0f			// offset from top where water collision volume starts
#define	WATER_Y	0.0f

#define	HONEY_COLLISION_TOPOFF	70.0f			// offset from top where water collision volume starts
#define	SLIME_COLLISION_TOPOFF	60.0f			// offset from top where water collision volume starts
#define	LAVA_COLLISION_TOPOFF	60.0f			// offset from top where water collision volume starts

enum
{
	LIQUID_WATER,
	LIQUID_SLIME,
	LIQUID_HONEY,
	LIQUID_LAVA,
	NUM_LIQUID_TYPES
};


//==========================================

void InitLiquids(void);
void DisposeLiquids(void);
void UpdateLiquidAnimation(void);
float FindLiquidY(float x, float z);


Boolean AddHoneyPatch(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddWaterPatch(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddSlimePatch(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddLavaPatch(TerrainItemEntryType *itemPtr, long  x, long z);
