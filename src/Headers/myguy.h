//
// myguy.h
//

#pragma once

#define	PLAYER_RADIUS				60.0f		// both bug & ball share same radius value so that morphing near fences doesnt cause fall-thru

#define BUG_LIMB_NUM_PELVIS			0
#define BUG_LIMB_NUM_HEAD			7

#define	PLAYER_BUG_FOOTOFFSET		0.0f				// dist to foot from origin
#define	PLAYER_BUG_HEADOFFSET		180.0f				// dist to head from origin

#define	PLAYER_BALL_FOOTOFFSET		(50.0f)				// dist to foot from origin
#define	PLAYER_BALL_HEADOFFSET		(45.0f)				// dist to head from origin

#define	HurtTimer		SpecialF[0]				// timer for duration of hurting
#define	InvincibleTimer	SpecialF[4]				// timer for invicibility after being hurt
#define	RotDeltaX		SpecialF[1]
#define	ExitTimer		SpecialF[0]

enum
{
	PLAYER_MODE_BALL,
	PLAYER_MODE_BUG
};


#define	PLAYER_COLLISION_CTYPE	(CTYPE_MISC|CTYPE_ENEMY|CTYPE_HURTME|CTYPE_TRIGGER|CTYPE_LIQUID|CTYPE_AUTOTARGET|CTYPE_VISCOUS)

enum
{
	PLAYER_ANIM_STAND	=	0,
	PLAYER_ANIM_WALK,
	PLAYER_ANIM_ROLLUP,
	PLAYER_ANIM_UNROLL,
	PLAYER_ANIM_KICK,
	PLAYER_ANIM_JUMP,
	PLAYER_ANIM_FALL,
	PLAYER_ANIM_LAND,
	PLAYER_ANIM_SWIM,
	PLAYER_ANIM_FALLONBUTT,
	PLAYER_ANIM_RIDEWATERBUG,
	PLAYER_ANIM_RIDEDRAGONFLY,
	PLAYER_ANIM_BEINGEATEN,
	PLAYER_ANIM_DEATH,
	PLAYER_ANIM_BLOODSUCK,
	PLAYER_ANIM_WEBBED,
	PLAYER_ANIM_ROPESWING,
	PLAYER_ANIM_CARRIED,
	PLAYER_ANIM_SITONTHRONE,
	PLAYER_ANIM_LOOKUP,
	PLAYER_ANIM_CAPTURED
};


#define	INVINCIBILITY_DURATION				3.0f	// from hurt (remember this includes the hurt animation time)
#define	INVINCIBILITY_DURATION_DEATH		3.0f	// from death



//=======================================================

void InitPlayerAtStartOfLevel(void);
void PlayerGotHurt(ObjNode *what, float damage, Boolean overrideShield, Boolean playerIsCurrent,
					Boolean canKnockOnButt, float invincibleDuration);
extern	void ResetPlayer(void);
Boolean DoPlayerCollisionDetect(void);
void PlayerHitEnemy(ObjNode *enemy);
void KillPlayer(Boolean changeAnims);

		/* BALL */
		
void InitPlayer_Ball(ObjNode *oldObj, TQ3Point3D *where);

		/* BUG */
		
ObjNode *InitPlayer_Bug(ObjNode *oldObj, TQ3Point3D *where, float rotY, int animNum);
void KnockPlayerBugOnButt(TQ3Vector3D *delta, Boolean allowBall, Boolean playerIsCurrent);
void PlayerGrabRootSwing(ObjNode *root, int joint);


		/* BUDDY */
		
void CreateMyBuddy(float x, float z);


void UpdatePlayerShield(void);



