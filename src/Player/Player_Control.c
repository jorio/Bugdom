/****************************/
/*   	PLAYER_Control.C    */
/* (c)1997-99 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/


/****************************/
/*    CONSTANTS             */
/****************************/

#define	DELTA_SUBDIV			15.0f				// smaller == more subdivisions per frame

#define	MAX_PLAYER_SWIM_SPEED	250.0f

#define	VISCOUS_SPEED_MULTIPLIER 0.17f

#define	KEY_THRUST				3500.0f

static const TQ3Vector2D	gZeroAccel = {0,0};

/*********************/
/*    VARIABLES      */
/*********************/

float	gMyDistToFloor;
Boolean	gPlayerCanMove;


/******************* DO PLAYER MOVEMENT AND COLLISION DETECT *********************/
//
//
// This code functions in 2 parts:
//
// 1. 	Apply acceleration, gravity and friction to delta values, then calculate the target coord
//		based on that delta
//
// 2. 	Given the distance needed to travel, subdivide the motion and delta vectors into smaller
//		values and process in a loop as though they were multiple moves.  This will
//		avoid the problem of the object going thru solids at high speeds and it should
//		also increase the accuracy of the collision physics.
//
// INPUT:	noControl = true if dont want player to be able to move (like during hurt anim)
//
// OUTPUT: true if disabled or killed
//

Boolean DoPlayerMovementAndCollision(Boolean noControl)
{
float				fps = gFramesPerSecondFrac,oldFPS,oldFPSFrac;
TQ3Point3D			oldCoord;
TQ3Vector3D			oldDelta;
float				realSpeed,maxSpeed;
TQ3Vector2D			newVec;
TQ3Matrix3x3		m;
static const TQ3Point2D origin = {0,0};
int					numPasses,pass;
Boolean				killed = false;


				/* CALC MAX SPEED */
				
	if (gPlayerObj->StatusBits & STATUS_BIT_UNDERWATER)				// if in water, then max speed is fixed
		maxSpeed = MAX_PLAYER_SWIM_SPEED;
	else if (gPlayerObj->StatusBits & STATUS_BIT_INVISCOUSTRAP)
		maxSpeed = gPlayerMaxSpeed * VISCOUS_SPEED_MULTIPLIER;
	else
		maxSpeed = gPlayerMaxSpeed;
		
			

	gPlayerObj->StatusBits &= ~(STATUS_BIT_ONGROUND|STATUS_BIT_ONTERRAIN);	// assume not on ground/terrain
	oldDelta = gDelta;


			/**************************************/
			/* PART 1: CALCULATE THE ACTUAL DELTA */
			/**************************************/			

	if (!gPlayerGotKilledFlag)
	{	
		if (noControl)
		{
//			ApplyFrictionToDeltas(20, &gDelta);
		}

					/***********************/
					/* DO KEY-BASED MOTION */
					/***********************/
		else
		{
						
			if (gPlayerUsingKeyControl && gGamePrefs.playerRelativeKeys)						
			{
				if (GetKeyState(kKey_Left))										// see if rotate ccw
					gPlayerObj->Rot.y += fps * 4.0f;	
				else
				if (GetKeyState(kKey_Right))									// see if rotate cw
					gPlayerObj->Rot.y -= fps * 4.0f;	
			
				if (GetKeyState(kKey_Forward))									// see if go forward
				{
					float	rot = gPlayerObj->Rot.y;
					gDelta.x += sin(rot) * (fps * -KEY_THRUST);
					gDelta.z += cos(rot) * (fps * -KEY_THRUST);		
				}
				else
				if (GetKeyState(kKey_Backward))									// see if go backward
				{
					float	rot = gPlayerObj->Rot.y;
					gDelta.x += sin(rot) * (fps * KEY_THRUST);
					gDelta.z += cos(rot) * (fps * KEY_THRUST);		
				}
			}
					/**********************/
					/* MOUSE BASED MOTION */
					/**********************/
			else
			{
						/* ROTATE MOUSE ACCELERATION VECTOR BASED ON CAMERA POS & APPLY TO DELTA */
			
				Q3Matrix3x3_SetRotateAboutPoint(&m, &origin, gPlayerToCameraAngle);		// make a 2D rotation matrix camera-rel
			

				Q3Vector2D_Transform(&gPlayerObj->AccelVector, &m, &newVec);		// rotate the mouse acceleration vector			
				gDelta.x += newVec.x;												// apply acceleration to deltas
				gDelta.z += newVec.y;
				
				
							/* SEE IF ALSO APPLY KEY THRUST */
							
				if (gPlayerCanMove)
				{
					if (GetKeyState(kKey_AutoWalk))										// see if forward thrust
					{
						float	rot = gPlayerObj->Rot.y;
						gDelta.x += sin(rot) * (fps * -KEY_THRUST);
						gDelta.z += cos(rot) * (fps * -KEY_THRUST);		
					}
				}
				else
				{
					gPlayerObj->AccelVector = gZeroAccel; 
				}
			}
		}
	
					/* CALC SPEED */
					
		gPlayerObj->Speed = FastVectorLength2D(gDelta.x, gDelta.z);			// calc 2D speed value
		if ((gPlayerObj->Speed >= 0.0f) && (gPlayerObj->Speed < 10000000.0f))	// check for weird NaN bug
		{
		}
		else
		{
			gPlayerObj->Speed = 0;
			gDelta.x = gDelta.z = 0;
		}

		if (!gPlayerObj->Skeleton)
		{
			// If we're in ball mode, it's OK to have no skeleton
			GAME_ASSERT_MESSAGE(gPlayerMode == PLAYER_MODE_BALL, "Player lost their skeleton! Shouldn't happen outside ball mode!");
		}
		else if (gPlayerObj->Skeleton->AnimNum != PLAYER_ANIM_FALLONBUTT)	// dont limit speed during fall-on-butt
		{
			if (gPlayerObj->Speed > maxSpeed)								// check max/min speeds
			{
				float	scale;
				
				scale = maxSpeed/gPlayerObj->Speed;							// get 100-% over
				gDelta.x *= scale;											// adjust back to max (note, we dont mess with y since gravity and jump need different max's)
				gDelta.z *= scale;
				gPlayerObj->Speed = maxSpeed;
			}
		}
	}


		/*****************************************/
		/* PART 1: MOVE AND COLLIDE IN MULTIPASS */
		/*****************************************/

		/* SUB-DIVIDE DELTA INTO MANAGABLE LENGTHS */

	oldFPS = gFramesPerSecond;							// remember what fps really is
	oldFPSFrac = gFramesPerSecondFrac;
			
	numPasses = (gPlayerObj->Speed*oldFPSFrac) * (1.0f / DELTA_SUBDIV);// calc how many subdivisions to create
	numPasses++;
		
	gFramesPerSecondFrac *= 1.0f / (float)numPasses;	// adjust frame rate during motion and collision
	gFramesPerSecond *= 1.0f / (float)numPasses;
	
	fps = gFramesPerSecondFrac;

	for (pass = 0; pass < numPasses; pass++)
	{
		float	dx,dy,dz;
		
		oldCoord = gCoord;								// remember starting coord
	
	
				/* GET DELTA */
		
		dx = gDelta.x;
		dy = gDelta.y;
		dz = gDelta.z;
				
		if (gPlayerObj->MPlatform)						// see if factor in moving platform
		{
			ObjNode *plat = gPlayerObj->MPlatform;
			dx += plat->Delta.x;
			dy += plat->Delta.y;
			dz += plat->Delta.z;	
		}
	
				/* MOVE IT */
		
		gCoord.x += dx*fps;
		gCoord.y += dy*fps;	
		gCoord.z += dz*fps;


				/******************************/
				/* DO OBJECT COLLISION DETECT */
				/******************************/
				
		if (DoPlayerCollisionDetect())
			killed = true;
		
				/**************************/
				/* SEE IF IN WATER VOLUME */
				/**************************/
				
		if (!killed)
		{
			if (gPlayerObj->StatusBits & STATUS_BIT_UNDERWATER)
			{
						/* BUG IS SWIMMING */
						
				if (gPlayerMode == PLAYER_MODE_BUG)
				{
					if (gPlayerObj->Skeleton->AnimNum != PLAYER_ANIM_SWIM)
					{
						MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_SWIM, 5.0);
					}
				}
				
						/* BALL IS SWIMMING */
				else
				{
					InitPlayer_Bug(gPlayerObj, &gCoord, 0, PLAYER_ANIM_SWIM);
				}

		
						/* SEE IF MAKE SPLASH */
							
				if (gCurrentLiquidType == LIQUID_WATER)
					if (gDelta.y < -800.0f)
						MakeSplash(gCoord.x, gPlayerCurrentWaterY, gCoord.z, .3, 1);
								
				gCoord.y = gPlayerCurrentWaterY - 1.0f;
				gDelta.y = -1.0;				
			}
		}


			/**********************************************/
			/* CHECK & HANDLE TERRAIN FOOT/HEAD COLLISION */
			/**********************************************/

		// Source port fix. The original source used to completely ignore gDelta.y to compute realSpeed.
		// But, at very high (500+) FPS, this slowed the player down to a crawl when walking up gentle slopes.
		realSpeed = FastVectorLength3D(				// calc real 3D speed for terrain collision
				gDelta.x,
				gDelta.y + PLAYER_GRAVITY*fps,		// cancel gravity contribution from DoFrictionAndGravity, OTHERWISE PLAYER WILL WALK UNCONTROLLABLY UPHILL @60FPS
				gDelta.z);

		if (HandleFloorAndCeilingCollision(&gCoord, &oldCoord, &gDelta, &oldDelta,
											-gPlayerObj->BottomOff, gPlayerObj->TopOff,
											realSpeed))
		{
			gPlayerObj->StatusBits |= STATUS_BIT_ONGROUND|STATUS_BIT_ONTERRAIN;	
		}
		
				/* DEAL WITH SLOPES */
				//
				// Using the floor normal here, apply some deltas to it.
				// Only apply slopes when on the ground (or really close to it)
				//
			
		if ((gPlayerObj->StatusBits & STATUS_BIT_ONTERRAIN) || (gMyDistToFloor < 15.0f))
		{
			float	acc;
			
					/* SLOPE ACC FOR BUG */
					
			if (gPlayerMode == PLAYER_MODE_BUG)
			{
				if (gRecentTerrainNormal[FLOOR].y < .25f)		// if really steep then give bug a hard time
				{
					if (fabs(gDelta.y) > 150.0f)
						acc = PLAYER_SLOPE_ACCEL * 8;			// bug hit steep slope fast
					else
						acc = 0;								// bug hit steep slope slow, so dont bounce too hard
				}
				else
					acc = PLAYER_SLOPE_ACCEL/4;					// slope has less effect on bug
			}
					/* SLOPE ACC FOR BALL */
			else
			{
				if (gRecentTerrainNormal[FLOOR].y < .25f)		// if really steep then give ball a hard time
				{
					if (fabs(gDelta.y) > 150.0f)
						acc = PLAYER_SLOPE_ACCEL * 7;			// ball hit steep slope fast
					else
						acc = 0;								// ball hit steep slope slow, so dont bounce too hard
				}
				else
					acc = PLAYER_SLOPE_ACCEL;
			}
			
					/* APPLY SLOPE ACC */
					
			gDelta.x += gRecentTerrainNormal[FLOOR].x * (fps * acc);
			gDelta.z += gRecentTerrainNormal[FLOOR].z * (fps * acc);		
			
			if (gDebugMode == DEBUG_MODE_BOXES)		//------------
				ShowNormal(&gCoord, &gRecentTerrainNormal[FLOOR]);
		}
	}

	gFramesPerSecond = oldFPS;										// restore real FPS values
	gFramesPerSecondFrac = oldFPSFrac;


				/*************************/
				/* CHECK FENCE COLLISION */
				/*************************/
				
	DoFenceCollision(gPlayerObj,.7);					// shrink my radius a little bit
				

	gMyDistToFloor = gCoord.y + gPlayerObj->BottomOff - GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR);
	
	return(killed);
}



/************************ DO FRICTION & GRAVITY ****************************/
//
// Applies friction to the gDeltas
//

void DoFrictionAndGravity(float friction)
{
TQ3Vector2D	v;
float	x,z,fps;

	fps = gFramesPerSecondFrac;
	
			/***************/
			/* DO FRICTION */
			/***************/
			
	friction *= fps;							// adjust friction

	v.x = gDelta.x;
	v.y = gDelta.z;
	
	FastNormalizeVector2D(v.x, v.y, &v);		// get normalized motion vector
	x = -v.x * friction;						// make counter-motion vector
	z = -v.y * friction;
	
	if (gDelta.x < 0.0f)						// decelerate against vector
	{
		gDelta.x += x;
		if (gDelta.x > 0.0f)					// see if sign changed
			gDelta.x = 0;											
	}
	else
	if (gDelta.x > 0.0f)									
	{
		gDelta.x += x;
		if (gDelta.x < 0.0f)								
			gDelta.x = 0;											
	}
	
	if (gDelta.z < 0.0f)								
	{
		gDelta.z += z;
		if (gDelta.z > 0.0f)								
			gDelta.z = 0;											
	}
	else
	if (gDelta.z > 0.0f)									
	{
		gDelta.z += z;
		if (gDelta.z < 0.0f)								
			gDelta.z = 0;											
	}

	if ((gDelta.x == 0.0f) && (gDelta.z == 0.0f))
	{
		gPlayerObj->Speed = 0;
	}

			/**************/
			/* DO GRAVITY */
			/**************/
			
	gDelta.y -= PLAYER_GRAVITY*fps;					// add gravity

	if (gDelta.y < 0.0f)							// if falling, keep dy at least -1.0 to avoid collision jitter on platforms
		if (gDelta.y > (-20.0f * fps))
			gDelta.y = (-20.0f * fps);

}


/************************ CHECK PLAYER MORPH ************************/
//
// See if player should change form
//

void CheckPlayerMorph(void)
{
int	anim;

		/* SEE IF THERE'S ANY BALL-TIME REMAINING */
				
	if (gBallTimer <= 0.0f)
		return;

	if (!GetNewKeyState(kKey_MorphPlayer))
		return;
	
			/****************/	
			/* CHANGE FORMS */
			/****************/	
			
	if (gPlayerMode == PLAYER_MODE_BALL)				// see if turn into bug
	{
			/* SEE IF HEAD WILL MATERIALIZE IN ROCK */

		if (!BallHasHeadroomToMorphToBug())
			return;

		InitPlayer_Bug(gPlayerObj, &gMyCoord, gPlayerObj->Rot.y, PLAYER_ANIM_UNROLL);
		PlayEffect3D(EFFECT_MORPH, &gMyCoord);
	}
	else												// turn into ball
	{
			/* CHECK BUG'S MODE AND SEE IF THIS IS ALLOWED */

		anim = gPlayerObj->Skeleton->AnimNum;
		
		switch(anim)
		{
			case	PLAYER_ANIM_STAND:					// these are the only modes allowed during a morph
			case	PLAYER_ANIM_WALK:
			case	PLAYER_ANIM_ROLLUP:
			case	PLAYER_ANIM_UNROLL:
			case	PLAYER_ANIM_KICK:
					SetSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_ROLLUP);
					PlayEffect3D(EFFECT_MORPH, &gMyCoord);	
					break;
		}
	}	
}











