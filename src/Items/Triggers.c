/****************************/
/*    	TRIGGERS	        */
/* (c)1997-99 Pangea Software */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/*******************/
/*   PROTOTYPES    */
/*******************/

static Boolean DoTrig_Nut(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void MoveHoneycombPlatform(ObjNode *theNode);
static Boolean DoTrig_HoneycombPlatform(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void MoveDetonator(ObjNode *theBox);
static Boolean DoTrig_Detonator(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void MoveLawnDoor(ObjNode *theNode);
static Boolean DoTrig_LawnDoor(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void CreateNutContents(ObjNode *theNut);
static void MakePowerup(ObjNode *theNut);
static void MovePowerup(ObjNode *theNode);
static Boolean DoTrig_Powerup(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void MoveWaterValve(ObjNode *theNode);
static Boolean DoTrig_WaterValve(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void ShowThePOW(ObjNode *theNode);
static void MovePowerupShow(ObjNode *theNode);
static void MoveNut(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	MAX_DETONATOR_IDS			255
#define	DETONATOR_SCALE				1.1f
#define	PLUNGER_DOWN_YOFF			(60.0f * DETONATOR_SCALE)


#define	MAX_VALVE_IDS				255

#define	DOOR_OPEN_SPEED				1.5f

enum
{
	HONEYCOMB_PLATFORM_MODE_NORMAL,
	HONEYCOMB_PLATFORM_MODE_FALL,
	HONEYCOMB_PLATFORM_MODE_RISE
};


/**********************/
/*     VARIABLES      */
/**********************/

#define	RegenerateNut			Flag[1]
#define	DetonateNut				Flag[2]
#define NutContents				SpecialL[0]
#define NutParm1				SpecialL[1]
#define	NutDetonatorID			SpecialL[3]

#define	ParticleTimer			SpecialF[0]

#define DetonatorID				SpecialL[0]
#define	IsPlunging				Flag[0]

#define	KeyNum					SpecialL[1]
#define	PowerupNutTerrainPtr	SpecialPtr[2]	// terrain ptr to nut which created powerup

#define	DoorAim					SpecialL[2]
#define	DoorSwingSpeed			SpecialF[0]
#define DoorSwingMax			SpecialF[1]

#define	ResurfacePlatform		Flag[0]

#define	ValveID					SpecialL[0]

Boolean gDetonatorBlown[MAX_DETONATOR_IDS];
Boolean	gValveIsOpen[MAX_VALVE_IDS];


												// TRIGGER HANDLER TABLE
												//========================

Boolean	(*gTriggerTable[])(ObjNode *, ObjNode *, Byte) = 
{
	DoTrig_Nut,
	DoTrig_WaterBug,
	DoTrig_DragonFly,
	DoTrig_HoneycombPlatform,
	DoTrig_Detonator,
	DoTrig_Checkpoint,
	DoTrig_LawnDoor,
	DoTrig_WebBullet,
	DoTrig_ExitLog,
	DoTrig_Powerup,
	DoTrig_WaterValve,
	DoTrig_KingWaterPipe,
	DoTrig_Cage
};


enum
{
	NUT_CONTENTS_BALLTIME,
	NUT_CONTENTS_KEY,
	NUT_CONTENTS_MONEY,
	NUT_CONTENTS_HEALTH,
	NUT_CONTENTS_FREELIFE,
	NUT_CONTENTS_BUDDY,
	NUT_CONTENTS_GREENCLOVER,
	NUT_CONTENTS_GOLDCLOVER,
	NUT_CONTENTS_BLUECLOVER,
	NUT_CONTENTS_SHIELD,
	NUT_CONTENTS_TICK
};


enum
{
	DOOR_MODE_CLOSED,
	DOOR_MODE_OPENING,
	DOOR_MODE_OPEN
};


/******************** HANDLE TRIGGER ***************************/
//
// INPUT: triggerNode = ptr to trigger's node
//		  whoNode = who touched it?
//		  side = side bits from collision.  Which side (of hitting object) hit the trigger
//
// OUTPUT: true if we want to handle the trigger as a solid object
//
// NOTE:  Triggers cannot self-delete in their DoTrig_ calls!!!  Bad things will happen in the hander collision function!
//

Boolean HandleTrigger(ObjNode *triggerNode, ObjNode *whoNode, Byte side)
{
	
	
		/* SEE IF THIS TRIGGER CAN ONLY BE TRIGGERED BY PLAYER */
		
	if (triggerNode->CType & CTYPE_PLAYERTRIGGERONLY)
		if (whoNode != gPlayerObj)
			return(true);


			/* CHECK SIDES */
			
	if (side & SIDE_BITS_BACK)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_FRONT)		// if my back hit, then must be front-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_FRONT)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_BACK)			// if my front hit, then must be back-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_LEFT)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_RIGHT)		// if my left hit, then must be right-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_RIGHT)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_LEFT)			// if my right hit, then must be left-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_TOP)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_BOTTOM)		// if my top hit, then must be bottom-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_BOTTOM)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_TOP)			// if my bottom hit, then must be top-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
		return(true);											// assume it can be solid since didnt trigger
}


#pragma mark -

/************************* ADD NUT *********************************/
//
// parm[0] = nut contents
//			0 = ????
//			1 = key
//			2 = money
//			3 = health
//
// parm[1] = other parms for specific nut contents (key ID, etc)
//
// parm[2] = detonator ID on hive level.
//
// parm[3]: bit 0 = always regenerate powerup
//			bit 1 = can be detonated
//

Boolean AddNut(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode			*newObj;
TQ3Point3D		where;
Byte			contents,id;
Boolean			canBlow;

	contents = itemPtr->parm[0];			// get contents
	id = itemPtr->parm[2];					// get detonator id
	canBlow = itemPtr->parm[3] & (1<<1);	// see if can blow

	if ((contents == NUT_CONTENTS_BUDDY) &&	gMyBuddy)	// dont add buddy nuts if buddy already active
		return(false);

		/* SEE IF NUT HAS DETONATED */

	if ((gLevelType == LEVEL_TYPE_HIVE) && (canBlow))
	{		
		if (gDetonatorBlown[id])						// see if blown
			return(true);								// dont add cuz its blown
	}


				/* CREATE OBJECT */
				
	where.y = GetTerrainHeightAtCoord(x,z,FLOOR);
	where.x = x;
	where.z = z;

	gNewObjectDefinition.group 		= GLOBAL1_MGroupNum_Nut;	
	gNewObjectDefinition.type 		= GLOBAL1_MObjType_Nut;									
	gNewObjectDefinition.coord		= where;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveNut;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.7;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return false;

	newObj->TerrainItemPtr = itemPtr;						// keep ptr to item list
	
	newObj->NutContents 	= contents;						// remember what's in this nut
	
	if (gRealLevel == LEVEL_NUM_BEACH)			// **hack to fix problem with Level 4 map which has all regenerating nuts
		newObj->RegenerateNut = false;
	else
		newObj->RegenerateNut 	= itemPtr->parm[3] & 1;		// see if regenerate this
	newObj->DetonateNut		= canBlow;						// rememberif get detonated
	newObj->NutParm1 		= itemPtr->parm[1];
	newObj->NutDetonatorID 	= id;							// remember detonator ID

//	if (contents == NUT_CONTENTS_MONEY)						// money always regenerates!
//		newObj->RegenerateNut = true;

			/* SET TRIGGER STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_PLAYERTRIGGERONLY|CTYPE_KICKABLE|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_NUT;


			/* SET COLLISION INFO */
			
	SetObjectCollisionBounds(newObj,110,0,-60,60,60,-60);


			/* MAKE SHADOW */
			
	AttachShadowToObject(newObj, 5, 5, false);


	return(true);							// item was added
}


/*********************** MOVE NUT ************************/

static void MoveNut(ObjNode *theNode)
{
		/* SEE IF GONE */
		
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}
	UpdateShadow(theNode);


		/* SEE IF BLOWN UP ON HIVE LEVEL */
		
	if (gLevelType == LEVEL_TYPE_HIVE)
	{
		if (theNode->DetonateNut)							// see if this nut can be detonated
		{
			Byte	id = theNode->NutDetonatorID;			// get detonator ID#
			
			if (gDetonatorBlown[id])						// see if blown
			{
				QD3D_ExplodeGeometry(theNode, 500, 0, 1, .4);
				theNode->TerrainItemPtr = nil;				// dont ever come back
				DeleteObject(theNode);		
				return;		
			}
		}
	}
}


/************** DO TRIGGER - NUT ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Nut(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	if (!(sideBits & SIDE_BITS_BOTTOM))					// if hit on side, slow me down
	{
		gDelta.x *= .2f;				
		gDelta.z *= .2f;
	}

			/********************************/
			/* IF BALL, THEN SMASH OPEN NOW */
			/********************************/
			
	if (gPlayerMode == PLAYER_MODE_BALL)	
	{
		if (whoNode->Speed > 800.0f)					// gotta be going fast enough
		{
			CreateNutContents(theNode);					// create the powerup that's in the nut
			
					/* EXPLODE SHELL */
					
			QD3D_ExplodeGeometry(theNode, 500, 0, 1, .4);
			if (!theNode->RegenerateNut)
				theNode->TerrainItemPtr = nil;			// dont ever come back
			PlayEffect3D(EFFECT_POP, &theNode->Coord);
			DeleteObject(theNode);				
		}
	}
	
	return(true);
}


/********************** KICK NUT *******************************/

void KickNut(ObjNode *kickerObj, ObjNode *nutObj)
{
	(void) kickerObj;
	
	CreateNutContents(nutObj);						// create the item inside the nut
			
	QD3D_ExplodeGeometry(nutObj, 500, 0, 1, .4);	// explode the shell
	if (!nutObj->RegenerateNut)
		nutObj->TerrainItemPtr = nil;				// dont ever come back
	PlayEffect3D(EFFECT_POP, &gCoord);
	DeleteObject(nutObj);							// delete the nut
}


/**************** CREATE NUT CONTENTS ******************/
//
// This is called when a nut is cracked open.
//

static void CreateNutContents(ObjNode *theNut)
{
		/* SPECIAL CHECK FOR BUDDY BUG IF ALREADY HAVE BUDDY BUG */
		
	if ((theNut->NutContents == NUT_CONTENTS_BUDDY) && (gMyBuddy))
	{
		theNut->NutContents = NUT_CONTENTS_GREENCLOVER;		// put green clover in there instead of Buddy Bug
	}


			/* CREATE THE CONTENTS */
			
	switch(theNut->NutContents)
	{
		case	NUT_CONTENTS_BUDDY:
				CreateMyBuddy(theNut->Coord.x, theNut->Coord.z);
				break;
				
		case	NUT_CONTENTS_TICK:
				MakeTickEnemy(&theNut->Coord);
				break;
				
		default:
				MakePowerup(theNut);
	}
}


#pragma mark -

/************************* ADD HONEYCOMB PLATFORM *********************************/
//
// parm[0] = type
//			0 = falling brick platform
//			1 = solid non-falling metal platform
//
// parm[1] = elevation
//
// parm[3]:	bit0 = resurface falling
//			bit1 = small
//

Boolean AddHoneycombPlatform(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
Boolean	isMetal = itemPtr->parm[0] & 1;
Boolean	isSmall = itemPtr->parm[3] & (1<<1);
float	h;


	if (gLevelType != LEVEL_TYPE_HIVE)
		DoFatalAlert("AddHoneycombPlatform: not on this level!");

	if (itemPtr->parm[1] == 0)
		h = 11;
	else
		h = itemPtr->parm[1];
	


	gNewObjectDefinition.group 		= HIVE_MGroupNum_Platform;	
	gNewObjectDefinition.type 		= HIVE_MObjType_BrickPlatform + (int)isMetal;	
	gNewObjectDefinition.coord.x 	= x;
//	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR) + 50.0f + (h * 4.5f);
	gNewObjectDefinition.coord.y 	= (h * -50.0f);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	if (isMetal)
		gNewObjectDefinition.moveCall 	= MoveStaticObject;	
	else
		gNewObjectDefinition.moveCall 	= MoveHoneycombPlatform;	
	
	if (isSmall)
		gNewObjectDefinition.scale 		= HONEYCOMB_PLATFORM_SCALE2;
	else
		gNewObjectDefinition.scale 		= HONEYCOMB_PLATFORM_SCALE;
	
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;						// keep ptr to item list
		
	newObj->ResurfacePlatform = itemPtr->parm[3] & 1;		// see if resurface

			/* SET TRIGGER STUFF */

	if (!isMetal)
	{
		newObj->CType 			= CTYPE_TRIGGER|CTYPE_BLOCKSHADOW|CTYPE_IMPENETRABLE|CTYPE_IMPENETRABLE2|
									CTYPE_PLAYERTRIGGERONLY|CTYPE_MISC|CTYPE_BLOCKCAMERA;
		newObj->CBits 			= CBITS_ALLSOLID;
		newObj->TriggerSides 	= SIDE_BITS_TOP;				// side(s) to activate it
		newObj->Kind 			= TRIGTYPE_HONEYCOMBPLATFORM;
	
		newObj->Mode 			= HONEYCOMB_PLATFORM_MODE_NORMAL;
	}	
	else
	{
		newObj->CType 			= CTYPE_BLOCKSHADOW|CTYPE_MISC|CTYPE_IMPENETRABLE|CTYPE_IMPENETRABLE2;
		newObj->CBits 			= CBITS_ALLSOLID;
	}

			/* SET COLLISION INFO */
			
	if (isSmall)
	{
		SetObjectCollisionBounds(newObj,65*HONEYCOMB_PLATFORM_SCALE2,-300*HONEYCOMB_PLATFORM_SCALE2,
								-200*HONEYCOMB_PLATFORM_SCALE2,200*HONEYCOMB_PLATFORM_SCALE2,
								200*HONEYCOMB_PLATFORM_SCALE2,-200*HONEYCOMB_PLATFORM_SCALE2);
	}
	else
	{
		SetObjectCollisionBounds(newObj,65*HONEYCOMB_PLATFORM_SCALE,-300*HONEYCOMB_PLATFORM_SCALE,
								-200*HONEYCOMB_PLATFORM_SCALE,200*HONEYCOMB_PLATFORM_SCALE,
								200*HONEYCOMB_PLATFORM_SCALE,-200*HONEYCOMB_PLATFORM_SCALE);
	}

	return(true);							
}


/************************** MOVE HONEYCOMB PLATFORM ******************************/

static void MoveHoneycombPlatform(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))		// just check to see if it's gone
	{
delete:	
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);
	
	switch(theNode->Mode)
	{
		case	HONEYCOMB_PLATFORM_MODE_FALL:
				gDelta.y -= 120.0f * gFramesPerSecondFrac;		// gravity
				gCoord.y += gDelta.y * gFramesPerSecondFrac;	// move down
				
				if (gCoord.y < (GetTerrainHeightAtCoord(gCoord.x,gCoord.z,FLOOR) - 200.0f))	// see if gone under ground
				{
					if (theNode->ResurfacePlatform)				// see if go away for good or if come back
					{
						theNode->Mode = HONEYCOMB_PLATFORM_MODE_RISE;
						gDelta.y = 50.0f;
						theNode->CType |= CTYPE_TRIGGER;		// its triggerable again
					}
					else
						goto delete;
				}
				break;

		case	HONEYCOMB_PLATFORM_MODE_RISE:
				gCoord.y += gDelta.y * gFramesPerSecondFrac;	// move up
				if (gCoord.y >= theNode->InitCoord.y)			// see if all the way up
				{
					gCoord.y = theNode->InitCoord.y;	
					theNode->Mode = HONEYCOMB_PLATFORM_MODE_NORMAL;
				}
				break;
	}

				
	UpdateObject(theNode);
}


/************** DO TRIGGER - HONEYCOMB PLATFORM ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_HoneycombPlatform(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	(void) whoNode;
	(void) sideBits;
	
	theNode->Mode = HONEYCOMB_PLATFORM_MODE_FALL;
	theNode->CType &= ~CTYPE_TRIGGER;							// no longer a trigger

	return(true);
}


#pragma mark -

/************************* INIT DETONATORS ******************************/

void InitDetonators(void)
{
int	i;
	
	for (i = 0; i < MAX_DETONATOR_IDS; i++)
		gDetonatorBlown[i] = false;
}

/************************* ADD DETONATOR *********************************/
//
// parm[0] = ID #
// parm[1] = color

Boolean AddDetonator(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode			*boxObj,*plungerObj;
TQ3Point3D		where;
u_long			isPlunged;	
	
	if (gLevelType != LEVEL_TYPE_HIVE)
		DoFatalAlert("AddDetonator: not on this level!");
	
	isPlunged = itemPtr->flags & ITEM_FLAGS_USER1;					// see if plunger already down
	
	where.y = GetTerrainHeightAtCoord(x,z,FLOOR);
	where.x = x;
	where.z = z;

				/*********************/
				/* ADD DETONATOR BOX */
				/*********************/
				//
				// this part isnt a trigger
				//
				
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= HIVE_MObjType_DetonatorGreen + itemPtr->parm[1];									
	gNewObjectDefinition.coord		= where;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT-1;
	gNewObjectDefinition.moveCall 	= MoveDetonator;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= DETONATOR_SCALE;
	boxObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (boxObj == nil)
		return false;
	
	boxObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	boxObj->CBits			= CBITS_ALLSOLID;
	SetObjectCollisionBounds(boxObj,40*DETONATOR_SCALE,0,-30,30,30,-30);
	
	boxObj->DetonatorID		= itemPtr->parm[0];					// save detonator ID#
	
	
				/***************/
				/* ADD PLUNGER */
				/***************/
				//
				// this is the triggerable part
				//
				
	if (isPlunged)
		gNewObjectDefinition.coord.y -= PLUNGER_DOWN_YOFF;
	else
		gNewObjectDefinition.coord.y -= 10.0f;
	gNewObjectDefinition.type 		= HIVE_MObjType_Plunger;									
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	plungerObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (plungerObj == nil)
		return false;

	plungerObj->TerrainItemPtr = itemPtr;					// keep ptr to item list
	
			/* SET TRIGGER STUFF */

	if (isPlunged)
		plungerObj->CType 		= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	else
		plungerObj->CType 		= CTYPE_TRIGGER|CTYPE_PLAYERTRIGGERONLY|CTYPE_AUTOTARGET|CTYPE_AUTOTARGETJUMP|CTYPE_BLOCKCAMERA;
	plungerObj->CBits			= CBITS_ALLSOLID;
	plungerObj->TriggerSides 	= SIDE_BITS_TOP;			// side(s) to activate it
	plungerObj->Kind		 	= TRIGTYPE_DETONATOR;

	plungerObj->IsPlunging 		= false;					// not plunging right now

			/* SET COLLISION INFO */
			
	SetObjectCollisionBounds(plungerObj,185*DETONATOR_SCALE,0,-30,30,30,-30);

	boxObj->ChainNode = plungerObj;							// plunger is a chain off of the box
	
	return(true);											// item was added
}


/********************* MOVE DETONATOR **********************/

static void MoveDetonator(ObjNode *theBox)
{		
ObjNode *plunger;

		/* SEE IF OUT OF RANGE */
		
	if (TrackTerrainItem(theBox))							// just check to see if it's gone
	{
		DeleteObject(theBox);
		return;
	}
	
		/* SEE IF MOVE PLUNGER */

	plunger = theBox->ChainNode;
	if (plunger)
	{
		if (plunger->IsPlunging)
		{
			plunger->Coord.y -= 140.0f * gFramesPerSecondFrac;
			if (plunger->Coord.y <= (theBox->Coord.y - PLUNGER_DOWN_YOFF))	// see if plunger all the way down
			{
				int	id;
				
				plunger->Coord.y = theBox->Coord.y - PLUNGER_DOWN_YOFF;
				plunger->IsPlunging = false;
				
				id = theBox->DetonatorID;					// get ID #
				gDetonatorBlown[id] = true;					// set global flag	
			}
			UpdateObjectTransforms(plunger);				// update coord & collison box
			CalcObjectBoxFromNode(plunger);											
		}
	}
}


/************** DO TRIGGER - DETONATOR ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Detonator(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	(void) whoNode;
	(void) sideBits;
	
	theNode->CType = CTYPE_MISC;							// not triggerable anymore
	theNode->IsPlunging = true;
	theNode->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;		// set item list flag so we'll always know this has detonated

	PlayEffect3D(EFFECT_PLUNGER, &theNode->Coord);

	return(true);
}



#pragma mark -

/************************* ADD LAWN DOOR *********************************/
//
// parm[0] = key ID
// parm[1] = orientation 0..3
//

Boolean AddLawnDoor(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
Byte	aim,keyID;
Boolean	isOpen;

	if (!(gLevelTypeMask & ((1<<LEVEL_TYPE_LAWN) | (1<<LEVEL_TYPE_NIGHT))))
		DoFatalAlert("AddLawnDoor: not on this level!");

	keyID = itemPtr->parm[0];
	aim = itemPtr->parm[1];
	
	isOpen = itemPtr->flags & ITEM_FLAGS_USER1;					// see if door already open
	if (isOpen)
		aim ^= 1;												// rotate open

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	
	if (gLevelType == LEVEL_TYPE_LAWN)
		gNewObjectDefinition.type 	= LAWN1_MObjType_Door_Green + keyID;	
	else
		gNewObjectDefinition.type 	= NIGHT_MObjType_Door_Green + keyID;	
	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveLawnDoor;
	gNewObjectDefinition.rot 		= (float)aim * (PI/2);
	gNewObjectDefinition.scale 		= .6;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr 	= itemPtr;						// keep ptr to item list		
	newObj->KeyNum 			= keyID;						// keep key ID#
	if (isOpen)
		newObj->Mode 		= DOOR_MODE_OPEN;				// door is closed right now
	else
		newObj->Mode 		= DOOR_MODE_CLOSED;				// door is closed right now
	newObj->DoorAim			= aim;							// remember if aim

			/* SET TRIGGER STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_PLAYERTRIGGERONLY|CTYPE_BLOCKCAMERA|CTYPE_IMPENETRABLE|CTYPE_MISC;
	newObj->CBits 			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind 			= TRIGTYPE_TWIGDOOR;

	newObj->BoundingSphere.origin.x = 0;					// source port mod:
	newObj->BoundingSphere.origin.z = 0;					// give door a more generous bounding sphere
	newObj->BoundingSphere.radius = 600.0f;					// (it tended to be culled too aggressively)

			/* SET COLLISION INFO */
		
	switch(aim)
	{
		case	0:
				SetObjectCollisionBounds(newObj,700,0,0,600, 40, -40);
				break;
		
		case	1:
				SetObjectCollisionBounds(newObj,700,0,-40,40, 0, -600);
				break;
			
		case	2:
				SetObjectCollisionBounds(newObj,700,0,-600,0, 40, -40);
				break;
				
		case	3:
				SetObjectCollisionBounds(newObj,700,0,-40,40, 600, 0);
				break;
	}
	
	return(true);							// item was added
}


/************************** MOVE LAWN DOOR ******************************/

static void MoveLawnDoor(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))		// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}
	
	switch(theNode->Mode)
	{
		case	DOOR_MODE_CLOSED:
				break;
				
		case	DOOR_MODE_OPENING:
				theNode->Rot.y += theNode->DoorSwingSpeed * gFramesPerSecondFrac;		// rotate open
				
				if (theNode->DoorSwingSpeed < 0.0f)
				{
					if (theNode->Rot.y <= theNode->DoorSwingMax)
					{
						theNode->Rot.y = theNode->DoorSwingMax;
						theNode->Mode = DOOR_MODE_OPEN;
					}	
				}
				else
				{
					if (theNode->Rot.y >= theNode->DoorSwingMax)
					{
						theNode->Rot.y = theNode->DoorSwingMax;
						theNode->Mode = DOOR_MODE_OPEN;
					}	
				}
				UpdateObjectTransforms(theNode);
				break;
	
		case	DOOR_MODE_OPEN:
				break;	
	}
}


/************** DO TRIGGER - LAWN DOOR ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_LawnDoor(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
int	keyNum;

	(void) whoNode;
	(void) sideBits;

	if (gPlayerMode == PLAYER_MODE_BALL)					// doors wont open for ball
		return(true);

			/****************************************/
			/* SEE IF WE HAVE THE KEY FOR THIS DOOR */
			/****************************************/
			
	keyNum = theNode->KeyNum;								// get key ID#
			
	if (DoWeHaveTheKey(keyNum))
	{
		theNode->Mode = DOOR_MODE_OPENING;					// start door opening
		theNode->CType = 0;									// no collision while opening			
		
				/* SEE WHICH WAY TO OPEN DOOR */
				
		switch(theNode->DoorAim)
		{
			case	0:												// HINGE ON LEFT
					if (gCoord.z < theNode->Coord.z)				// if player in back, then open front
					{
						theNode->DoorSwingSpeed = -DOOR_OPEN_SPEED;
						theNode->DoorSwingMax = theNode->Rot.y - PI/2;
					}
					else
					{
						theNode->DoorSwingSpeed = DOOR_OPEN_SPEED;
						theNode->DoorSwingMax = theNode->Rot.y + PI/2;
					}
					break;

			case	1:												// HINGE ON FRONT
					if (gCoord.x < theNode->Coord.x)				// if player on left, then open right
					{
						theNode->DoorSwingSpeed = -DOOR_OPEN_SPEED;
						theNode->DoorSwingMax = theNode->Rot.y - PI/2;
					}
					else
					{
						theNode->DoorSwingSpeed = DOOR_OPEN_SPEED;
						theNode->DoorSwingMax = theNode->Rot.y + PI/2;
					}
					break;

			case	2:												// HINGE ON RIGHT
					if (gCoord.z < theNode->Coord.z)				// if player in back, then open front
					{
						theNode->DoorSwingSpeed = DOOR_OPEN_SPEED;
						theNode->DoorSwingMax = theNode->Rot.y + PI/2;
					}
					else
					{
						theNode->DoorSwingSpeed = -DOOR_OPEN_SPEED;
						theNode->DoorSwingMax = theNode->Rot.y - PI/2;
					}
					break;
				
			case	3:												// HINGE ON BACK
					if (gCoord.x < theNode->Coord.x)				// if player on left, then open right
					{
						theNode->DoorSwingSpeed = DOOR_OPEN_SPEED;
						theNode->DoorSwingMax = theNode->Rot.y + PI/2;
					}
					else
					{
						theNode->DoorSwingSpeed = -DOOR_OPEN_SPEED;
						theNode->DoorSwingMax = theNode->Rot.y - PI/2;
					}
					break;
		}
			/* MARK TERRAIN ITEM AS OPEN */
			
		theNode->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;
		
		
			/* KEY HAS BEEN USED */
			
		UseKey(keyNum);
		
			/* PLAY DOOR EFFECT */
			
		if (gLevelType == LEVEL_TYPE_LAWN)
			PlayEffect3D(EFFECT_OPENLAWNDOOR, &theNode->Coord);
		else
		if (gLevelType == LEVEL_TYPE_NIGHT)
			PlayEffect3D(EFFECT_OPENNIGHTDOOR, &theNode->Coord);
	}
					
	return(true);
}



#pragma mark -

/*********************** MAKE POWERUP ****************************/
//
// Powerups are inside nuts.
//
// 

static void MakePowerup(ObjNode *theNut)
{
short				group,type;
ObjNode				*newObj;
static const Byte	keyTypes[NUM_LEVEL_TYPES] =
{
	LAWN1_MObjType_Key_Green,	// lawn
	0, 							// pond
	0,							// forest
	0,							// hive
	NIGHT_MObjType_Key_Green,	// night
	0							// ant hill
};

			/* GET INFO */
			
	switch(theNut->NutContents)
	{
		case	NUT_CONTENTS_BALLTIME:
				group = GLOBAL2_MGroupNum_BallTimePOW;
				type = GLOBAL2_MObjType_BallTimePOW;
				break;

		case	NUT_CONTENTS_KEY:
				group = MODEL_GROUP_LEVELSPECIFIC;
				type = keyTypes[gLevelType] + theNut->NutParm1;	// NutParm1 has key ID #
				break;
				
		case	NUT_CONTENTS_MONEY:
//				if (gMoney > 0)								// if already have $$ then turn this into a green clover
//					goto clover;
				group = POND_MGroupNum_Money;
				type = POND_MObjType_Money;
				break;
				
		case	NUT_CONTENTS_HEALTH:
				group = GLOBAL2_MGroupNum_BerryPOW;
				type = GLOBAL2_MObjType_Berry;
				break;
				
		case	NUT_CONTENTS_FREELIFE:
				group = GLOBAL2_MGroupNum_LifePOW;
				type = GLOBAL2_MObjType_FreeLifePOW;
				break;

		case	NUT_CONTENTS_GREENCLOVER:
				group = GLOBAL2_MGroupNum_GreenCloverPOW;
				type = GLOBAL2_MObjType_GreenClover;
				break;

		case	NUT_CONTENTS_GOLDCLOVER:
				group = GLOBAL2_MGroupNum_GoldCloverPOW;
				type = GLOBAL2_MObjType_GoldClover;
				break;

		case	NUT_CONTENTS_BLUECLOVER:
				group = GLOBAL2_MGroupNum_BlueCloverPOW;
				type = GLOBAL2_MObjType_BlueClover;
				break;
				
		case	NUT_CONTENTS_SHIELD:
				group = GLOBAL2_MGroupNum_ShieldPOW;
				type = GLOBAL2_MObjType_ShieldPOW;
				break;
				
		default:
				DoFatalAlert("MakePowerup: this type not supported yet");
				return;

	}

			/*****************/
			/* CREATE OBJECT */
			/*****************/
			
	gNewObjectDefinition.group 		= group;	
	gNewObjectDefinition.type 		= type;
	gNewObjectDefinition.coord.x	= theNut->Coord.x;
	gNewObjectDefinition.coord.y	= theNut->Coord.y;
	gNewObjectDefinition.coord.z	= theNut->Coord.z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MovePowerup;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;

			/* SET TRIGGER STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_PLAYERTRIGGERONLY|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_POW;

			/* SET COLLISION INFO */
			
	SetObjectCollisionBounds(newObj,50,0,-25,25,25,-25);


			/*************************/
			/* SET POWERUP SPECIFICS */
			/*************************/

	newObj->NutContents = theNut->NutContents;			// remember what kind of POW
			
	switch(theNut->NutContents)
	{
		case	NUT_CONTENTS_KEY:
				newObj->KeyNum = theNut->NutParm1;			// key ID# is in here
				break;
	}


		/* REMEMBER TERRAIN PTR TO NUT THAT CREATED THIS */
		
	newObj->PowerupNutTerrainPtr = theNut->TerrainItemPtr;

}


/******************** MOVE POWERUP ***********************/

static void MovePowerup(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* SEE IF TRACK THIS POW */
			
	switch(theNode->NutContents)
	{
		case	NUT_CONTENTS_BALLTIME:
		case	NUT_CONTENTS_HEALTH:
		case	NUT_CONTENTS_FREELIFE:
		case	NUT_CONTENTS_GREENCLOVER:
		case	NUT_CONTENTS_SHIELD:
				if (TrackTerrainItem_Far(theNode, 500))		// just check to see if it's gone
				{
					DeleteObject(theNode);
					return;
				}
				break;
	}


	theNode->Rot.y += fps * 4.0f;
	UpdateObjectTransforms(theNode);
	
	
	
			/***********************************/
			/* MUSHROOM/BALLTIME HAS PARTICLES */
			/***********************************/
			
	if (theNode->NutContents == NUT_CONTENTS_BALLTIME)
	{
		theNode->ParticleTimer += fps;						// regulate flow
		if (theNode->ParticleTimer > .05f)
		{
			theNode->ParticleTimer = 0;
			
						/* MAKE PARTICLE GROUP */

			if (!VerifyParticleGroup(theNode->ParticleGroup))
			{
				theNode->ParticleGroup = NewParticleGroup(
														PARTICLE_TYPE_GRAVITOIDS,		// type
														0,								// flags
														0,								// gravity
														8000,							// magnetism
														5,								// base scale
														.9,								// decay rate
														0,								// fade rate
														PARTICLE_TEXTURE_GREENRING);	// texture
			}

				/* ADD PARTICLES TO GROUP */

			if (theNode->ParticleGroup != -1)
			{			
				int	i;
					
				for (i = 0; i < 3; i++)
				{
					TQ3Vector3D	delta;
					TQ3Point3D	pt;
						
					pt = theNode->Coord;
									
					pt.x += (RandomFloat()-.5f) * 100.0f;
					pt.y += (RandomFloat()-.5f) * 30.0f + 100.0f;
					pt.z += (RandomFloat()-.5f) * 100.0f;
					
					delta.x = (RandomFloat()-.5f) * 40.0f;
					delta.y = (RandomFloat()-.5f) * 30.0f + 30.0f;
					delta.z = (RandomFloat()-.5f) * 40.0f;
					
					AddParticleToGroup(theNode->ParticleGroup, &pt, &delta, RandomFloat() + 1.0f, FULL_ALPHA);
				}
			}			
		}
	}	
}


/************** DO TRIGGER - POWERUP ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Powerup(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	(void) whoNode;
	(void) sideBits;

	switch(theNode->NutContents)
	{
		case	NUT_CONTENTS_KEY:
				GetKey(theNode->KeyNum);			// put key into inventory
				break;
				
		case	NUT_CONTENTS_MONEY:
				GetMoney();
				break;
				
		case	NUT_CONTENTS_HEALTH:
				GetHealth(.5);									// get health
				break;
				
		case	NUT_CONTENTS_BALLTIME:
				gBallTimer += 1.0f;
				if (gBallTimer > 1.0f)
					gBallTimer = 1.0f;
				gInfobarUpdateBits |= UPDATE_TIMER;	
				break;
				
		case	NUT_CONTENTS_FREELIFE:
				gNumLives++;
				gInfobarUpdateBits |= UPDATE_LIVES;	
				break;

		case	NUT_CONTENTS_GREENCLOVER:
				GetGreenClover();
				break;

		case	NUT_CONTENTS_GOLDCLOVER:
				GetGoldClover();
				break;

		case	NUT_CONTENTS_BLUECLOVER:
				GetBlueClover();
				break;
				
		case	NUT_CONTENTS_SHIELD:
				gShieldTimer = SHIELD_TIME;
				break;
								
		default:
				DoFatalAlert("DoTrig_Powerup: powerup type not implemented yet");
	}	
				
	PlayEffect3D(EFFECT_GETPOW, &theNode->Coord);	// play sound
	ShowThePOW(theNode);
	return(false);									// not solid
}


/************************  SHOW THE POW *************************/
//
// Display the powerup so we can see what it was
//

static void ShowThePOW(ObjNode *theNode)
{
	theNode->CType = 0;
	theNode->MoveCall = MovePowerupShow;
	theNode->Health = 2.0;
	theNode->StatusBits |= STATUS_BIT_GLOW|STATUS_BIT_NOZWRITE;
}


/*************** MOVE POWERUP SHOW ******************/

static void MovePowerupShow(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);
	
	gDelta.y += 600.0f * fps;
	gCoord.y += gDelta.y * fps;
	theNode->Health -= fps;
	if (theNode->Health < 1.0f)
	{
		if (theNode->Health <= 0.0f)
		{
			DeleteObject(theNode);
			return;
		}
		else
		{
			MakeObjectTransparent(theNode, theNode->Health);	
		}
	}
	
	theNode->Rot.y += fps * 4.0f;
	
	UpdateObject(theNode);
}



#pragma mark -

/************************* INIT WATER VALVES ******************************/

void InitWaterValves(void)
{
int	i;
	
	for (i = 0; i < MAX_VALVE_IDS; i++)
	{
		gValveIsOpen[i] = false;
	}
}

/************************* ADD WATER VALVE *********************************/
//
// parm[0] = ID #
//

Boolean AddWaterValve(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode			*newObj,*handle;
TQ3Point3D		where;
u_long			isOpen;	
	
	if (gLevelType != LEVEL_TYPE_ANTHILL)
		DoFatalAlert("AddWaterValve: not on this level!");

	isOpen = itemPtr->flags & ITEM_FLAGS_USER1;					// see if valve already open
	
	where.y = GetTerrainHeightAtCoord(x,z,FLOOR);
	where.x = x;
	where.z = z;

				/*****************/
				/* ADD VALVE BOX */
				/*****************/
				
	gNewObjectDefinition.group 		= ANTHILL_MGroupNum_WaterValve;	
	gNewObjectDefinition.type 		= ANTHILL_MObjType_WaterValveBox;									
	gNewObjectDefinition.coord		= where;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveWaterValve;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .25;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return false;

	newObj->TerrainItemPtr = itemPtr;					// keep ptr to item list
	

	newObj->ValveID		= itemPtr->parm[0];
	
	
			/* SET TRIGGER STUFF */

	if (isOpen)
		newObj->CType 		= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	else
		newObj->CType 		= CTYPE_TRIGGER|CTYPE_PLAYERTRIGGERONLY|CTYPE_AUTOTARGET|CTYPE_AUTOTARGETJUMP|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;			// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_WATERVALVE;

			/* SET COLLISION INFO */
			
	SetObjectCollisionBounds(newObj,100,0,-30,30,30,-30);


				/********************/
				/* ADD VALVE HANDLE */
				/********************/
				
	gNewObjectDefinition.type 		= ANTHILL_MObjType_WaterValveHandle;									
	gNewObjectDefinition.coord.y	+= 100;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	handle = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (handle)
	{
		newObj->ChainNode = handle;	
	
	}

	
	return(true);											// item was added
}


/********************* MOVE WATER VALVE **********************/

static void MoveWaterValve(ObjNode *theNode)
{

		/* SEE IF OUT OF RANGE */
		
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}
	
		/* SEE IF UPDATE SOUND */
		
//	if (gValveIsOpen[theNode->ValveID])
//	{
//		if (theNode->EffectChannel == -1)
//		theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_POUR, &gCoord, kMiddleC, 2.0);
//		else
//			Update3DSoundChannel(&theNode->EffectChannel, &theNode->Coord);
//	}
}


/************** DO TRIGGER - WATER VALVE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_WaterValve(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
short	id;
ObjNode *handle;

	(void) sideBits;
	(void) whoNode;

	theNode->CType = CTYPE_MISC;							// not triggerable anymore
	theNode->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;		// set item list flag so we'll always know this has detonated

			/* MARK VALVE AS OPEN */
			
	id = theNode->ValveID;
	gValveIsOpen[id] = true;
	
	
			/* TURN VALVE */
	
	handle = theNode->ChainNode;
	if (handle)
	{
		handle->Rot.y += PI/2;
		UpdateObjectTransforms(handle);
	}
	
	PlayEffect3D(EFFECT_VALVEOPEN, &theNode->Coord);
	
	return(true);
}



















