/****************************/
/*   	ENEMY_SKELETON.C    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#if 0  // Source port removal
#include <QD3D.h>
#include <QD3DGroup.h>
#include <QD3DMath.h>
#endif

#include "globals.h"
#include "objects.h"
#include "misc.h"
#include "skeletonanim.h"
#include "skeletonobj.h"
#include "skeletonjoints.h"
#if 0  // Source port removal
#include "limb.h"
#endif
#include "file.h"
#include "collision.h"
#include "terrain.h"
#if 0  // Source port removal
#include "enemy_skeleton.h"
#endif
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




