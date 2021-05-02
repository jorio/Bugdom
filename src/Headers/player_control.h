//
// player_control.h
//

#pragma once

#define	PLAYER_SLOPE_ACCEL		2800.0f

#define	PLAYER_GRAVITY			5200.0f


//==================================================

Boolean DoPlayerMovementAndCollision(Boolean noControl);
void DoFrictionAndGravity(float friction);
void CheckPlayerMorph(void);

