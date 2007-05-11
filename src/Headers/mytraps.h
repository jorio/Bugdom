//
// mytraps.h
//

#define	BAT_JOINT_HEAD	1


Boolean PrimeFoot(long splineNum, SplineItemType *itemPtr);
void MakeBat(float x, float y, float z);
Boolean AddThorn(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddFireWall(TerrainItemEntryType *itemPtr, long  x, long z);
void MakeShockwave(TQ3Point3D *coord, float startScale);
Boolean AddRollingBoulder(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddFloorSpike(TerrainItemEntryType *itemPtr, long  x, long z);





