//
// player_control.h
//

#ifndef PLAYERCONTROL_H
#define PLAYERCONTROL_H

#define	PLAYER_SLOPE_ACCEL		2800.0f

#define	PLAYER_GRAVITY			5200.0f


//==================================================

Boolean DoPlayerMovementAndCollision(Boolean noControl);
void DoFrictionAndGravity(float friction);
void CheckPlayerMorph(void);



#endif
