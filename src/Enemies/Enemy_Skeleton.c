/****************************/
/*   	ENEMY_SKELETON.C    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <QD3D.h>
#include <QD3DGroup.h>
#include <QD3DMath.h>

#include "globals.h"
#include "objects.h"
#include "misc.h"
#include "skeletonanim.h"
#include "skeletonobj.h"
#include "skeletonjoints.h"
#include "limb.h"
#include "file.h"
#include "collision.h"
#include "terrain.h"
#include "enemy_skeleton.h"
#include "3dmath.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	TQ3ViewObject			gGameViewObject;
extern	ObjNode					*gCurrentNode;
extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	TQ3Point3D			gCoord;
extern	TQ3Vector3D			gDelta;
extern	short				gNumItems;
extern	float				gMostRecentCharacterFloorY;
extern	Byte				gCurrentLevel;


/****************************/
/*    PROTOTYPES            */
/****************************/


/****************************/
/*    CONSTANTS             */
/****************************/

/*********************/
/*    VARIABLES      */
/*********************/




