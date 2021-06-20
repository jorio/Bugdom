//
// items.h
//


enum
{
	LADYBUG_ANIM_FLY,
	LADYBUG_ANIM_UNFOLD,
	LADYBUG_ANIM_WAIT
};



extern	void InitItemsManager(void);
Boolean AddClover(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddGrass(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddWeed(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddSunFlower(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddCosmo(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddPoppy(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddWallEnd(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddTree(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddCatTail(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddDuckWeed(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddLilyFlower(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddLilyPad(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddPondGrass(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddReed(TerrainItemEntryType *itemPtr, long  x, long z);

ObjNode* CreateCyclorama(void);
Boolean AddRockLedge(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddStump(TerrainItemEntryType *itemPtr, long  x, long z);
void RattleHive(ObjNode *hive);


Boolean AddFirecracker(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddHiveDoor(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddDock(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddRootSwing(TerrainItemEntryType *itemPtr, long  x, long z);
void InitRootSwings(void);
void UpdateRootSwings(void);

Boolean AddRock(TerrainItemEntryType *itemPtr, long  x, long z);



Boolean AddHoneyTube(TerrainItemEntryType *itemPtr, long  x, long z);
void UpdateHoneyTubeTextureAnimation(void);
Boolean PrimeHoneycombPlatform(long splineNum, SplineItemType *itemPtr);

void ExplodeFirecracker(ObjNode *fc, Boolean makeShockwave);

Boolean AddBentAntPipe(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddHorizAntPipe(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddFaucet(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddWoodPost(TerrainItemEntryType *itemPtr, long  x, long z);


















