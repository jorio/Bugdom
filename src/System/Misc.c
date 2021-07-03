/****************************/
/*      MISC ROUTINES       */
/* (c)1996-99 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
#include <stdio.h>


/****************************/
/*    CONSTANTS             */
/****************************/




/**********************/
/*     VARIABLES      */
/**********************/

short	gPrefsFolderVRefNum;
long	gPrefsFolderDirID;



unsigned long seed0 = 0, seed1 = 0, seed2 = 0;


/**********************/
/*     PROTOTYPES     */
/**********************/


/****************** DO SYSTEM ERROR ***************/

void ShowSystemErr(long err)
{
Str255		numStr;
	printf("BUGDOM FATAL ERROR #%ld\n", err);
	NumToStringC(err, numStr);
	DoAlert (numStr);
	CleanQuit();
}

/****************** DO SYSTEM ERROR : NONFATAL ***************/
//
// nonfatal
//
void ShowSystemErr_NonFatal(long err)
{
Str255		numStr;
	printf("BUGDOM NON-FATAL ERROR #%ld\n", err);
	NumToStringC(err, numStr);
	DoAlert (numStr);
}

/*********************** DO ALERT *******************/

void DoAlert(const char* s)
{
	if (gSDLWindow)
		SDL_SetWindowFullscreen(gSDLWindow, 0);
	printf("BUGDOM ALERT: %s\n", s);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bugdom", s, gSDLWindow);
}


/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(const char* s)
{
	if (gSDLWindow)
		SDL_SetWindowFullscreen(gSDLWindow, 0);
	printf("BUGDOM FATAL ALERT: %s\n", s);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bugdom", s, gSDLWindow);
	CleanQuit();
}

/*********************** DO FATAL ALERT 2 *******************/

void DoFatalAlert2(const char* s1, const char* s2)
{
	if (gSDLWindow)
		SDL_SetWindowFullscreen(gSDLWindow, 0);
	printf("BUGDOM FATAL ALERT: %s - %s\n", s1, s2);
	static char alertbuf[1024];
	snprintf(alertbuf, 1024, "%s\n%s", s1, s2);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bugdom", alertbuf, gSDLWindow);
	ExitToShell();
}

/*********************** DO ASSERT *******************/

void DoAssert(const char* msg, const char* file, int line)
{
	if (gSDLWindow)
		SDL_SetWindowFullscreen(gSDLWindow, 0);
	printf("BUGDOM ASSERTION FAILED: %s - %s:%d\n", msg, file, line);
	static char alertbuf[1024];
	snprintf(alertbuf, 1024, "%s\n%s:%d", msg, file, line);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bugdom: Assertion Failed!", alertbuf, gSDLWindow);
	ExitToShell();
}

/************ CLEAN QUIT ***************/

void CleanQuit(void)
{
Boolean beenHere = false;

	if (!beenHere)
	{
		GammaFadeOut();

		beenHere = true;

		DeleteAll3DMFGroups();
		FreeAllSkeletonFiles(-1);

		if (gGameViewInfoPtr != nil)                // see if nuke an existing draw context
			QD3D_DisposeWindowSetup(&gGameViewInfoPtr);

		GameScreenToBlack();

		FreeInfobarArt();
	}

	StopAllEffectChannels();
	KillSong();
	UseResFile(gMainAppRezFile);

	InitCursor();
//	SetDefaultOutputVolume((gOriginalSystemVolume<<16)|gOriginalSystemVolume); // reset system volume
	ExitToShell();		
}

/********************** WAIT **********************/

void Wait(u_long ticks)
{
u_long	start;
	
	start = TickCount();

	while (TickCount()-start < ticks); 

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

unsigned long MyRandomLong(void)
{
  return seed2 ^= (((seed1 ^= (seed2>>5)*1568397607UL)>>7)+
                   (seed0 = (seed0+1)*3141592621UL))*2435386481UL;
}


/************************* RANDOM RANGE *************************/
//
// THE RANGE *IS* INCLUSIVE OF MIN AND MAX
//

u_short	RandomRange(unsigned short min, unsigned short max)
{
u_short		qdRdm;											// treat return value as 0-65536
u_long		range, t;

	qdRdm = MyRandomLong();
	range = max+1 - min;
	t = (qdRdm * range)>>16;	 							// now 0 <= t <= range
	
	return( t+min );
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

void SetMyRandomSeed(unsigned long seed)
{
	seed0 = seed;
	seed1 = 0;
	seed2 = 0;	
	
}

/**************** INIT MY RANDOM SEED *******************/

void InitMyRandomSeed(void)
{
	seed0 = 0x2a80ce30;
	seed1 = 0;
	seed2 = 0;	
}


#pragma mark -

/****************** ALLOC HANDLE ********************/

Handle	AllocHandle(long size)
{
	return NewHandleClear(size);
}


/****************** ALLOC PTR ********************/

Ptr	AllocPtr(long size)
{
	return NewPtrClear(size);
}


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
long		createdDirID;

			/* MAKE FSSPEC FOR DATA FOLDER */
			// Source port note: our custom main sets gDataSpec already

	iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons", &gDataSpec);
	if (iErr)
	{
		DoAlert("Can't find game assets!");
		CleanQuit();
	}



			/* CHECK PREFERENCES FOLDER */
			
	iErr = FindFolder(kOnSystemDisk,kPreferencesFolderType,kDontCreateFolder,		// locate the folder
					&gPrefsFolderVRefNum,&gPrefsFolderDirID);
	if (iErr != noErr)
		DoAlert("Warning: Cannot locate the Preferences folder.");

	iErr = DirCreate(gPrefsFolderVRefNum,gPrefsFolderDirID,"Bugdom",&createdDirID);		// make folder in there
	


}


/******************** REGULATE SPEED ***************/

void RegulateSpeed(short fps)
{
UInt32		n;
static UInt32 oldTick = 0;
	
	n = 60 / fps;
	while ((TickCount() - oldTick) < n);			// wait for n ticks
	oldTick = TickCount();							// remember current time
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
