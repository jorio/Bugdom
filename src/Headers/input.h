//
// input.h
//

#ifndef __INPUT_H
#define __INPUT_H


#define NUM_MOUSE_SENSITIVITY_LEVELS		5
#define DEFAULT_MOUSE_SENSITIVITY_LEVEL		(NUM_MOUSE_SENSITIVITY_LEVELS/2)


			/* ASCII */
			
#define	CHAR_RETURN			0x0d	/* ASCII code for Return key */
#define CHAR_UP				0x1e
#define CHAR_DOWN			0x1f
#define	CHAR_LEFT			0x1c
#define	CHAR_RIGHT			0x1d
#define	CHAR_DELETE			0x08
#define CHAR_FORWARD_DELETE	0x7f


	/* KEYBOARD EQUATE */
		
enum
{
	kKey_Pause				= kVK_Escape,
	
	kKey_SwivelCameraLeft	= kVK_ANSI_Comma,
	kKey_SwivelCameraRight	= kVK_ANSI_Period,
	kKey_ZoomIn				= kVK_ANSI_2,
	kKey_ZoomOut			= kVK_ANSI_1,
		
	kKey_ToggleMusic 		= kVK_ANSI_M,
	kKey_RaiseVolume 		= kVK_ANSI_Equal,
	kKey_LowerVolume 		= kVK_ANSI_Minus,
	
	kKey_MorphPlayer		= kVK_Space,
	kKey_BuddyAttack		= kVK_Tab,
#if __APPLE__
	kKey_Jump				= kVK_Command,
	kKey_KickBoost			= kVK_Option,
#else
	kKey_Jump				= kVK_Option,
	kKey_KickBoost			= kVK_Control,
#endif
	
	kKey_AutoWalk			= kVK_Shift,
	kKey_Forward			= kVK_UpArrow,
	kKey_Backward			= kVK_DownArrow,
	kKey_Left				= kVK_LeftArrow,
	kKey_Right				= kVK_RightArrow,
	
	kKey_ToggleFullscreen	= kVK_F11,
};



//============================================================================================


void UpdateInput(void);
Boolean GetNewKeyState(unsigned short key);
Boolean GetKeyState(unsigned short key);
Boolean AreAnyNewKeysPressed(void);
void UpdateKeyMap(void);

void GetMouseCoord(Point *point);

void GetMouseDelta(float *dx, float *dy);

void CaptureMouse(Boolean doCapture);



#endif



