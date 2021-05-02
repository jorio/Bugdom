/****************************/
/*   ENEMY: TICK.C			*/
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveTick(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_TICK				4

#define	TICK_CHASE_RANGE		700.0f

#define TICK_TURN_SPEED		3.0f
#define TICK_CHASE_SPEED		100.0f
#define TICK_SPLINE_SPEED		0.03f

#define	MAX_TICK_RANGE			(TICK_CHASE_RANGE+1000.0f)	// max distance this enemy can go from init coord

#define	TICK_HEALTH			1.0f		
#define	TICK_DAMAGE			0.05f
#define	TICK_SCALE			1.2f



/*********************/
/*    VARIABLES      */
/*********************/


/************************ MAKE TICK ENEMY *************************/

void MakeTickEnemy(TQ3Point3D *where)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= GLOBAL2_MGroupNum_Tick;	
	gNewObjectDefinition.type 		= GLOBAL2_MObjType_Tick;	
	gNewObjectDefinition.coord		= *where;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 373;
	gNewObjectDefinition.moveCall 	= MoveTick;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= TICK_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;

				/* SET BETTER INFO */
			
	newObj->Health 		= TICK_HEALTH;
	newObj->Damage 		= TICK_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_TICK;
	
	
				/* SET COLLISION INFO */
				
	newObj->CType		= CTYPE_ENEMY|CTYPE_BLOCKCAMERA|CTYPE_BOPPABLE|
						CTYPE_SPIKED|CTYPE_KICKABLE|CTYPE_HURTNOKNOCK;
	newObj->CBits		= CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj, 50,0,-30,30,30,-30);


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_TICK]++;
}



/********************* MOVE TICK **************************/

static void MoveTick(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	r,aim;

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	if (theNode->CType == 0)							// see if flattened
		return;

	GetObjectInfo(theNode);
	

			/* MOVE TOWARD PLAYER */
			
	aim = TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, TICK_TURN_SPEED, false);			

	r = theNode->Rot.y;
	gDelta.x = sin(r) * -TICK_CHASE_SPEED;
	gDelta.z = cos(r) * -TICK_CHASE_SPEED;
	
	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR);	// calc y coord

			

				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	UpdateEnemy(theNode);		
}


#pragma mark -

/****************** TICK GOT BOPPED ************************/
//
// Called during player collision handler.
//

void TickGotBopped(ObjNode *enemy)
{	
		/* FLATTEN */
		
	enemy->Scale.y *= .2f;	
	UpdateObjectTransforms(enemy);	
	
	enemy->CType = 0;
}


/******************* KILL TICK **********************/

Boolean KillTick(ObjNode *theNode)
{
	QD3D_ExplodeGeometry(theNode, 500.0f, 0, 1, .3);
	DeleteEnemy(theNode);
	return(true);
}








