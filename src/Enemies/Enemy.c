/****************************/
/*   	ENEMY.C  			*/
/* (c)1997-98 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static Boolean KillEnemy(ObjNode *theEnemy);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

short		gNumEnemyOfKind[NUM_ENEMY_KINDS];
short		gNumEnemies;


/*********************  INIT ENEMY MANAGER **********************/

void InitEnemyManager(void)
{
short	i;

	gTheQueen = nil;
	gAntKingObj = nil;
	gNumEnemies = 0;

	for (i=0; i < NUM_ENEMY_KINDS; i++)
		gNumEnemyOfKind[i] = 0;

	gCurrentGasParticleGroup = -1;
}


/********************** DELETE ENEMY **************************/

void DeleteEnemy(ObjNode *theEnemy)
{
	if (!(theEnemy->StatusBits & STATUS_BIT_ONSPLINE))		// spline enemies dont factor into the enemy counts!
	{
		gNumEnemyOfKind[theEnemy->Kind]--;					// dec kind count
		if (gNumEnemyOfKind[theEnemy->Kind] < 0)
		{
			DoAlert("DeleteEnemy: < 0");
			gNumEnemyOfKind[theEnemy->Kind] = 0;
		}

		if (theEnemy->Kind != ENEMY_KIND_FIREFLY)			// remember that fireflies dont count normally!
			gNumEnemies--;										// dec global count
	}

	DeleteObject(theEnemy);								// nuke the obj
}


/******************** KILL ENEMY *************************/
//
// OUTPUT:	true = enemy objnode was deleted.
//

static Boolean KillEnemy(ObjNode *theEnemy)
{
			/* HANDLE DEATH */
			
	switch(theEnemy->Kind)
	{
		case	ENEMY_KIND_ANT:
				return (KillAnt(theEnemy));

		case	ENEMY_KIND_FIREANT:
				return (KillFireAnt(theEnemy));

		case	ENEMY_KIND_SPIDER:
				return (KillSpider(theEnemy));
				
		case	ENEMY_KIND_MOSQUITO:
				return (KillMosquito(theEnemy,0,0,0));

		case	ENEMY_KIND_BOXERFLY:
				return (KillBoxerFly(theEnemy,0,0,0));

		case	ENEMY_KIND_FLYINGBEE:
				return (KillFlyingBee(theEnemy,0,0,0));

		case	ENEMY_KIND_WORKERBEE:
				return (KillWorkerBee(theEnemy));

		case	ENEMY_KIND_QUEENBEE:
				return (KillQueenBee(theEnemy));

		case	ENEMY_KIND_ROACH:
				return (KillRoach(theEnemy));
				
		case	ENEMY_KIND_SKIPPY:
				return (KillSkippy(theEnemy));

		case	ENEMY_KIND_TICK:
				return (KillTick(theEnemy));

		case	ENEMY_KIND_LARVA:
				return (KillLarva(theEnemy));
				
		case	ENEMY_KIND_FIREFLY:
				return (KillFireFly(theEnemy));
				
		case	ENEMY_KIND_KINGANT:
				return (KillKingAnt(theEnemy));
				
				
	}	
	
	return(false);
}




/****************** DO ENEMY COLLISION DETECT ***************************/
//
// For use by non-skeleton enemies.
//
// OUTPUT: true = was deleted
//

Boolean DoEnemyCollisionDetect(ObjNode *theEnemy, unsigned long ctype)
{
short	i;
ObjNode	*hitObj;
float	realSpeed;

	theEnemy->StatusBits &= ~STATUS_BIT_ONGROUND;						// assume not on ground

			/* AUTOMATICALLY HANDLE THE BORING STUFF */
			
	HandleCollisions(theEnemy, ctype);


			/******************************/
			/* SCAN FOR INTERESTING STUFF */
			/******************************/
			
	theEnemy->StatusBits &= ~STATUS_BIT_UNDERWATER;				// assume not in water volume

	for (i=0; i < gNumCollisions; i++)						
	{
			hitObj = gCollisionList[i].objectPtr;				// get ObjNode of this collision
			ctype = hitObj->CType;

			if (ctype == INVALID_NODE_FLAG)						// see if has since become invalid
				continue;
			
					/* HURT */
					
			if (ctype & CTYPE_HURTENEMY)
			{
				if (EnemyGotHurt(theEnemy,hitObj->Damage))		// handle hit (returns true if was deleted)
					return(true);
			}


				/* LIQUID */
					
			if (ctype & CTYPE_LIQUID)
			{
				theEnemy->StatusBits |= STATUS_BIT_UNDERWATER;
			}
	}
	
				/* CHECK PARTICLE COLLISION */

	if (ParticleHitObject(theEnemy, PARTICLE_FLAGS_HURTENEMY))
	{
		if (EnemyGotHurt(theEnemy,.3))						// handle hit (returns true if was deleted)
			return(true);
	}
			
	
				/* CHECK FENCE COLLISION */
				
	DoFenceCollision(theEnemy,1);


			/********************************/	
			/* DO FLOOR & CEILING COLLISION */
			/********************************/	
			
	realSpeed = FastVectorLength3D(gDelta.x, gDelta.y, gDelta.z);// calc real 3D speed for terrain collision
	if (HandleFloorAndCeilingCollision(&gCoord, &theEnemy->OldCoord, &gDelta, &gDelta,
										-theEnemy->BottomOff, theEnemy->TopOff,
										realSpeed))
	{
		theEnemy->StatusBits |= STATUS_BIT_ONGROUND|STATUS_BIT_ONTERRAIN;	
	}

	return(false);
}



/******************* ENEMY GOT HURT *************************/
//
// INPUT:	theEnemy = node of theEnemy which was hit.
//
// OUTPUT: true = was deleted
//

Boolean EnemyGotHurt(ObjNode *theEnemy, float damage)
{
Boolean	wasDeleted;


			/* LOSE HEALTH */
			
	theEnemy->Health -= damage;
	
	
			/* HANDLE DEATH OF ENEMY */
			
	if (theEnemy->Health <= 0.0f)
		wasDeleted = KillEnemy(theEnemy);
	else
		wasDeleted = false;
	
	return(wasDeleted);
}



/*********************** UPDATE ENEMY ******************************/

void UpdateEnemy(ObjNode *theNode)
{
	theNode->Speed = Q3Vector3D_Length(&gDelta);	

	UpdateObject(theNode);
}


/********************* FIND CLOSEST ENEMY *****************************/
//
// OUTPUT: nil if no enemies
//

ObjNode *FindClosestEnemy(TQ3Point3D *pt, float *dist)
{
ObjNode		*thisNodePtr,*best = nil;
float	d,minDist = 10000000;

			
	thisNodePtr = gFirstNodePtr;
	
	do
	{
		if (thisNodePtr->Slot >= SLOT_OF_DUMB)					// see if reach end of usable list
			break;
	
		if (thisNodePtr->CType & CTYPE_ENEMY)
		{
			d = CalcQuickDistance(pt->x,pt->z,thisNodePtr->Coord.x, thisNodePtr->Coord.z);
			if (d < minDist)
			{
				minDist = d;
				best = thisNodePtr;
			}
		}	
		thisNodePtr = (ObjNode *)thisNodePtr->NextNode;		// next node
	}
	while (thisNodePtr != nil);

	*dist = minDist;
	return(best);
}


/******************* MAKE ENEMY SKELETON *********************/
//
// This routine creates a non-character skeleton which is an enemy.
//
// INPUT:	itemPtr->parm[0] = skeleton type 0..n
//
// OUTPUT:	ObjNode or nil if err.
//

ObjNode *MakeEnemySkeleton(Byte skeletonType, float x, float z, float scale)
{
ObjNode	*newObj;
	
			/****************************/
			/* MAKE NEW SKELETON OBJECT */
			/****************************/

	gNewObjectDefinition.type 		= skeletonType;
	gNewObjectDefinition.animNum 	= 0;							// assume default anim is #0
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z, FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= ENEMY_SLOT + skeletonType;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= scale;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	GAME_ASSERT(newObj);

				/* SET DEFAULT COLLISION INFO */
				
	newObj->CType = CTYPE_ENEMY|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	
	return(newObj);
}


/*************************** ENEMY GOT BONKED ********************************/

void EnemyGotBonked(ObjNode *theEnemy)
{
	if (EnemyGotHurt(theEnemy, 1.0))				// hurt the enemy & see if was killed
		return;
}


/********************* MOVE ENEMY **********************/

void MoveEnemy(ObjNode *theNode)
{
	(void) theNode;

	gCoord.y += gDelta.y*gFramesPerSecondFrac;					// move
	gCoord.x += gDelta.x*gFramesPerSecondFrac;
	gCoord.z += gDelta.z*gFramesPerSecondFrac;
}


/***************** DETACH ENEMY FROM SPLINE *****************/
//
// OUTPUT: true if was on spline, false if wasnt
//

Boolean DetachEnemyFromSpline(ObjNode *theNode, void (*moveCall)(ObjNode*))
{
		/* MAKE SURE ALL COMPONENTS ARE IN LINKED LIST */

	AttachObject(theNode);
	AttachObject(theNode->ShadowNode);
	AttachObject(theNode->ChainNode);

	if (!RemoveFromSplineObjectList(theNode))			// remove from spline
		return(false);	
	
	if (theNode->Kind != ENEMY_KIND_FIREFLY)
		gNumEnemies++;								// count as a normal enemy now (if not firefly)
		
	gNumEnemyOfKind[theNode->Kind]++;
	
	theNode->InitCoord  = theNode->Coord;			// remember where started
	
	theNode->MoveCall = moveCall;


	return(true);
}















