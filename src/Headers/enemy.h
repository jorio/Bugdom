//
// enemy_skeleton.h
//

#include "terrain.h"
#include "splineitems.h"

#define	MAX_ENEMIES	11

#define	DEFAULT_ENEMY_COLLISION_CTYPES	(CTYPE_MISC|CTYPE_HURTENEMY|CTYPE_ENEMY|CTYPE_LIQUID)
#define	DEATH_ENEMY_COLLISION_CTYPES	(CTYPE_MISC|CTYPE_ENEMY)

#define ENEMY_GRAVITY		2000.0f


		/* ENEMY KIND */
		
#define	NUM_ENEMY_KINDS	20			// always keep at or more than actual types
enum
{
	ENEMY_KIND_BOXERFLY = 0,
	ENEMY_KIND_SLUG,
	ENEMY_KIND_ANT,
	ENEMY_KIND_FIREANT,
	ENEMY_KIND_PONDFISH,
	ENEMY_KIND_MOSQUITO,
	ENEMY_KIND_SPIDER,
	ENEMY_KIND_CATERPILLER,
	ENEMY_KIND_LARVA,
	ENEMY_KIND_FLYINGBEE,
	ENEMY_KIND_WORKERBEE,
	ENEMY_KIND_QUEENBEE,
	ENEMY_KIND_ROACH,
	ENEMY_KIND_SKIPPY,
	ENEMY_KIND_KINGANT,
	ENEMY_KIND_TICK,
	ENEMY_KIND_FIREFLY
};


#define	PONDFISH_JOINT_HEAD		4


enum
{
	ANT_ANIM_STAND,
	ANT_ANIM_WALK,
	ANT_ANIM_THROWSPEAR,
	ANT_ANIM_PICKUP,
	ANT_ANIM_FALLONBUTT,
	ANT_ANIM_GETOFFBUTT,
	ANT_ANIM_DIE,
	ANT_ANIM_WALKCARRY,
	ANT_ANIM_PICKUPROCK,
	ANT_ANIM_THROWROCK
};

enum
{
	PONDFISH_ANIM_WAIT,
	PONDFISH_ANIM_JUMPATTACK,
	PONDFISH_ANIM_MOUTHFULL
};


enum
{
	FIREANT_ANIM_STAND,
	FIREANT_ANIM_WALK,
	FIREANT_ANIM_STANDFIRE,
	FIREANT_ANIM_FLY,
	FIREANT_ANIM_FALLONBUTT,
	FIREANT_ANIM_GETOFFBUTT,
	FIREANT_ANIM_DIE,
	FIREANT_ANIM_COPYRIGHT
};

#define	BreathParticleGroup	SpecialL[3]

#define	QUEENBEE_HEALTH				7.0f
#define	ANTKING_HEALTH				5.0f


//=====================================================================
//=====================================================================
//=====================================================================


			/* ENEMY */
			
ObjNode *MakeEnemySkeleton(Byte skeletonType, float x, float z, float scale);
extern	void DeleteEnemy(ObjNode *theEnemy);
extern	Boolean DoEnemyCollisionDetect(ObjNode *theEnemy, unsigned long ctype);
extern	void UpdateEnemy(ObjNode *theNode);
Boolean EnemyGotHurt(ObjNode *theEnemy, float damage);
extern	void InitEnemyManager(void);
extern	ObjNode *FindClosestEnemy(TQ3Point3D *pt, float *dist);
void EnemyGotBonked(ObjNode *theEnemy);
void MoveEnemy(ObjNode *theNode);
Boolean DetachEnemyFromSpline(ObjNode *theNode, void (*moveCall)(ObjNode*));


			/* BOXERFLY */
			
extern	Boolean AddEnemy_BoxerFly(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_BoxerFly(long splineNum, SplineItemType *itemPtr);
Boolean BallHitBoxerFly(ObjNode *me, ObjNode *enemy);
Boolean KillBoxerFly(ObjNode *theNode, float dx, float dy, float dz);

			/* PONDFISH */
			
extern	Boolean AddEnemy_PondFish(TerrainItemEntryType *itemPtr, long x, long z);


			/* SLUG */
			
Boolean PrimeEnemy_Slug(long splineNum, SplineItemType *itemPtr);

void SetCrawlingEnemyJointTransforms(ObjNode* theNode, int splineIndexDelta,
	float footOffset, float collisionBoxSize, float undulateScale, float* undulatePhase);

		/* CATERPILLER */
		
Boolean PrimeEnemy_Caterpiller(long splineNum, SplineItemType *itemPtr);


			/* ANT */
			
extern	Boolean AddEnemy_Ant(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_Ant(long splineNum, SplineItemType *itemPtr);
Boolean BallHitAnt(ObjNode *me, ObjNode *enemy);
Boolean KillAnt(ObjNode *theNode);
Boolean KnockAntOnButt(ObjNode *enemy, float dx, float dy, float dz, float damage);

			/* FIREANT */
			
extern	Boolean AddEnemy_FireAnt(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_FireAnt(long splineNum, SplineItemType *itemPtr);
Boolean BallHitFireAnt(ObjNode *me, ObjNode *enemy);
Boolean KillFireAnt(ObjNode *theNode);
Boolean KnockFireAntOnButt(ObjNode *enemy, float dx, float dy, float dz);
void FireAntBreathFire(ObjNode *theNode);

			/* MOSQUITO */
			
extern	Boolean AddEnemy_Mosquito(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_Mosquito(long splineNum, SplineItemType *itemPtr);
Boolean BallHitMosquito(ObjNode *me, ObjNode *enemy);
Boolean KillMosquito(ObjNode *theNode, float dx, float dy, float dz);

			/* SPIDER */

Boolean AddEnemy_Spider(TerrainItemEntryType *itemPtr, long x, long z);
Boolean BallHitSpider(ObjNode *me, ObjNode *enemy);
Boolean KnockSpiderOnButt(ObjNode *enemy, float dx, float dy, float dz, float damage);
Boolean KillSpider(ObjNode *theNode);
Boolean PrimeEnemy_Spider(long splineNum, SplineItemType *itemPtr);

			/* LARVA */

Boolean AddEnemy_Larva(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_Larva(long splineNum, SplineItemType *itemPtr);
ObjNode *MakeLarvaEnemy(float x, float z);
void LarvaGotBopped(ObjNode *enemy);
Boolean KillLarva(ObjNode *theNode);


			/* FLYING BEE */
			
Boolean AddEnemy_FlyingBee(TerrainItemEntryType *itemPtr, long x, long z);
Boolean KillFlyingBee(ObjNode *theNode, float dx, float dy, float dz);
Boolean MakeFlyingBee(TQ3Point3D *where);
Boolean BallHitFlyingBee(ObjNode *me, ObjNode *enemy);


		/* WORKER BEE */

Boolean AddEnemy_WorkerBee(TerrainItemEntryType *itemPtr, long x, long z);
Boolean BallHitWorkerBee(ObjNode *me, ObjNode *enemy);
Boolean KillWorkerBee(ObjNode *theNode);
Boolean PrimeEnemy_WorkerBee(long splineNum, SplineItemType *itemPtr);


		/* QUEEN BEE */

Boolean AddEnemy_QueenBee(TerrainItemEntryType *itemPtr, long x, long z);
Boolean BallHitQueenBee(ObjNode *me, ObjNode *enemy);
Boolean KillQueenBee(ObjNode *theNode);
Boolean KnockQueenBeeOnButt(ObjNode *enemy, float dx, float dz, float damage);

		/* ROACH */

Boolean AddEnemy_Roach(TerrainItemEntryType *itemPtr, long x, long z);
Boolean BallHitRoach(ObjNode *me, ObjNode *enemy);
Boolean KillRoach(ObjNode *theNode);
Boolean PrimeEnemy_Roach(long splineNum, SplineItemType *itemPtr);
Boolean KnockRoachOnButt(ObjNode *enemy, float dx, float dy, float dz, float damage);


		/* SKIPPY */

Boolean AddEnemy_Skippy(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_Skippy(long splineNum, SplineItemType *itemPtr);
Boolean KillSkippy(ObjNode *theNode);



		/* KING ANT */

Boolean AddEnemy_KingAnt(TerrainItemEntryType *itemPtr, long x, long z);
Boolean BallHitKingAnt(ObjNode *me, ObjNode *enemy);
Boolean KillKingAnt(ObjNode *theNode);
Boolean KnockKingAntOnButt(ObjNode *enemy, float dx, float dz, float damage);


		/* TICK */
		
void MakeTickEnemy(TQ3Point3D *where);
void TickGotBopped(ObjNode *enemy);
Boolean KillTick(ObjNode *theNode);

		
			/* FIREFLY */
			
Boolean AddFireFly(TerrainItemEntryType *itemPtr, long x, long z);
Boolean KillFireFly(ObjNode *theNode);




