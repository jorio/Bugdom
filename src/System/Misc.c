/****************************/
/*      MISC ROUTINES       */
/* By Brian Greenstone      */
/* (c)1996-99 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
#include <stdio.h>
#include <stdarg.h>


/****************************/
/*    CONSTANTS             */
/****************************/




/**********************/
/*     VARIABLES      */
/**********************/

short	gPrefsFolderVRefNum;
long	gPrefsFolderDirID;



static uint32_t seed0 = 0x2a80ce30, seed1 = 0, seed2 = 0;


/**********************/
/*     PROTOTYPES     */
/**********************/


/*********************** DO ALERT *******************/

void DoAlert(const char* format, ...)
{
	if (gSDLWindow)
		SDL_SetWindowFullscreen(gSDLWindow, 0);

	char message[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	printf("BUGDOM ALERT: %s\n", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bugdom", message, gSDLWindow);
}


/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(const char* format, ...)
{
	if (gSDLWindow)
		SDL_SetWindowFullscreen(gSDLWindow, 0);

	char message[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	printf("BUGDOM FATAL ALERT: %s\n", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bugdom", message, gSDLWindow);
	ExitToShell();
}


/************ CLEAN QUIT ***************/

void CleanQuit(void)
{
static Boolean beenHere = false;

	if (!beenHere)
	{
		GammaFadeOut(true);

		beenHere = true;

		DeleteAllObjects();
		DeleteAll3DMFGroups();
		FreeAllSkeletonFiles(-1);
		QD3D_DisposeShards();

		if (gGameViewInfoPtr != nil)                // see if nuke an existing draw context
			QD3D_DisposeWindowSetup(&gGameViewInfoPtr);

		GameScreenToBlack();

		FreeInfobarArt();
	}

	StopAllEffectChannels();
	KillSong();

	SDL_ShowCursor(1);
	Pomme_FlushPtrTracking(false);
	Render_EndScene();
	Render_DeleteContext();
	ExitToShell();
}


#pragma mark -


/******************** MY RANDOM LONG **********************/
//
// My own random number generator that returns a LONG
//
// NOTE: call this instead of MyRandomShort if the value is going to be
//		masked or if it just doesnt matter since this version is quicker
//		without the 0xffff at the end.
//

uint32_t MyRandomLong(void)
{
  return seed2 ^= (((seed1 ^= (seed2>>5)*1568397607UL)>>7)+
                   (seed0 = (seed0+1)*3141592621UL))*2435386481UL;
}


/************** RANDOM FLOAT ********************/
//
// returns a random float between 0 and 1
//

float RandomFloat(void)
{
unsigned long	r;
float	f;

	r = MyRandomLong() & 0xfff;		
	if (r == 0)
		return(0);

	f = (float)r;							// convert to float
	f = f * (1.0f/(float)0xfff);			// get # between 0..1
	return(f);
} 
 


/**************** SET MY RANDOM SEED *******************/

void SetMyRandomSeed(uint32_t seed)
{
	seed0 = seed;
	seed1 = 0;
	seed2 = 0;
}

#pragma mark -

/****************** 2D ARRAY ********************/

void** Alloc2DArray(int sizeofType, int n, int m)
{
	GAME_ASSERT_MESSAGE(n > 0, "Alloc2DArray n*m: n cannot be 0");

	char** array = (char**) NewPtrClear(n * sizeof(Ptr));
	GAME_ASSERT(array);

	array[0] = NewPtrClear(n * m * sizeofType);
	GAME_ASSERT(array[0]);

	for (int i = 1; i < n; i++)
		array[i] = array[i-1] + m*sizeofType;

	return (void**) array;
}


void Free2DArray(void** array)
{
	GAME_ASSERT(array);

	DisposePtr((Ptr) array[0]);
	DisposePtr((Ptr) array);
}




#pragma mark -


/******************* VERIFY SYSTEM ******************/

void VerifySystem(void)
{
OSErr	iErr;

			/* MAKE FSSPEC FOR DATA FOLDER */
			// Source port note: our custom main sets gDataSpec already

	iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons", &gDataSpec);
	if (iErr)
	{
		DoAlert("Can't find game assets!");
		CleanQuit();
	}



			/* CHECK PREFERENCES FOLDER */
			
	InitPrefsFolder(false);
}


/***************** APPLY FICTION TO DELTAS ********************/

void ApplyFrictionToDeltas(float f,TQ3Vector3D *d)
{
	if (d->x < 0.0f)
	{
		d->x += f;
		if (d->x > 0.0f)
			d->x = 0;
	}
	else
	if (d->x > 0.0f)
	{
		d->x -= f;
		if (d->x < 0.0f)
			d->x = 0;
	}

	if (d->z < 0.0f)
	{
		d->z += f;
		if (d->z > 0.0f)
			d->z = 0;
	}
	else
	if (d->z > 0.0f)
	{
		d->z -= f;
		if (d->z < 0.0f)
			d->z = 0;
	}
}
