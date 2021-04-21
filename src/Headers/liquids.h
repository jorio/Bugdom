//
// liquids.h
//

#define	WATER_Y	0.0f

enum
{
	LIQUID_WATER,
	LIQUID_HONEY,
	LIQUID_SLIME,
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
