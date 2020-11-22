/****************************/
/*      MISC ROUTINES       */
/* (c)1996-99 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/


extern	unsigned long gOriginalSystemVolume;
extern	short		gMainAppRezFile;
extern	Boolean		gGameOverFlag,gAbortedFlag,gQD3DInitialized;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	FSSpec		gDataSpec;
extern	SDL_Window*	gSDLWindow;


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
	printf("BUGDOM ALERT: %s\n", s);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bugdom", s, gSDLWindow);
}


/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(const char* s)
{
	printf("BUGDOM FATAL ALERT: %s\n", s);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bugdom", s, gSDLWindow);
	CleanQuit();
}

/*********************** DO FATAL ALERT 2 *******************/

void DoFatalAlert2(const char* s1, const char* s2)
{
	printf("BUGDOM FATAL ALERT: %s - %s\n", s1, s2);
	static char alertbuf[1024];
	snprintf(alertbuf, 1024, "%s\n%s", s1, s2);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bugdom", alertbuf, gSDLWindow);
	ExitToShell();
}


/************ CLEAN QUIT ***************/

void CleanQuit(void)
{	
Boolean beenHere = false;

    if (!beenHere)
    {
        beenHere = true;

        DeleteAll3DMFGroups();
        FreeAllSkeletonFiles(-1);
                
        if (gGameViewInfoPtr != nil)                // see if nuke an existing draw context
            QD3D_DisposeWindowSetup(&gGameViewInfoPtr);

    	GameScreenToBlack();

    	if (gQD3DInitialized)
    		Q3Exit();

    }


	StopAllEffectChannels();
	KillSong();
	UseResFile(gMainAppRezFile);

	CleanupDisplay();								// unloads Draw Sprocket
	
	InitCursor();
//	SetDefaultOutputVolume((gOriginalSystemVolume<<16)|gOriginalSystemVolume); // reset system volume
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	ExitToShell();		
}

/********************** WAIT **********************/

void Wait(u_long ticks)
{
u_long	start;
	
	start = TickCount();

	while (TickCount()-start < ticks); 

}


/***************** NUM TO HEX *****************/
//
// NumToHex - fixed length, returns a C string
//

unsigned char *NumToHex(unsigned short n)
{
static unsigned char format[] = "0xXXXX";				// Declare format static so we can return a pointer to it.
char *conv = "0123456789ABCDEF";
short i;

	for (i = 0; i < 4; n >>= 4, ++i)
			format[5 - i] = conv[n & 0xf];
	return format;
}


/***************** NUM TO HEX 2 **************/
//
// NumToHex2 -- allows variable length, returns a ++PASCAL++ string.
//

unsigned char *NumToHex2(unsigned long n, short digits)
{
static unsigned char format[] = "$XXXXXXXX";				// Declare format static so we can return a pointer to it
char *conv = "0123456789ABCDEF";
short i;

	if (digits > 8 || digits < 0)
			digits = 8;
	format[0] = digits + 1;							// adjust length byte of output string

	for (i = 0; i < digits; n >>= 4, ++i)
			format[(digits + 1) - i] = conv[n & 0xf];
	return format;
}


/*************** NUM TO DECIMAL *****************/
//
// NumToDecimal --  returns a ++PASCAL++ string.
//

unsigned char *NumToDec(unsigned long n)
{
static unsigned char format[] = "XXXXXXXX";				// Declare format static so we can return a pointer to it
char *conv = "0123456789";
short		 i,digits;
unsigned long temp;

	if (n < 10)										// fix digits
		digits = 1;
	else if (n < 100)
		digits = 2;
	else if (n < 1000)
		digits = 3;
	else if (n < 10000)
		digits = 4;
	else if (n < 100000)
		digits = 5;
	else
		digits = 6;

	format[0] = digits;								// adjust length byte of output string

	for (i = 0; i < digits; ++i)
	{
		temp = n/10;
		format[digits-i] = conv[n-(temp*10)];
		n = n/10;
	}
	return format;
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


#if 0
#pragma mark -


/******************* FLOAT TO STRING *******************/

void FloatToString(float num, Str255 string)
{
Str255	sf;
long	i,f;

	i = num;						// get integer part
	
	
	f = (fabs(num)-fabs((float)i)) * 10000;		// reduce num to fraction only & move decimal --> 5 places	

	if ((i==0) && (num < 0))		// special case if (-), but integer is 0
	{
		string[0] = 2;
		string[1] = '-';
		string[2] = '0';
	}
	else
		NumToString(i,string);		// make integer into string
		
	NumToString(f,sf);				// make fraction into string
	
	string[++string[0]] = '.';		// add "." into string
	
	if (f >= 1)
	{
		if (f < 1000)
			string[++string[0]] = '0';	// add 1000's zero
		if (f < 100)
			string[++string[0]] = '0';	// add 100's zero
		if (f < 10)
			string[++string[0]] = '0';	// add 10's zero
	}
	
	for (i = 0; i < sf[0]; i++)
	{
		string[++string[0]] = sf[i+1];	// copy fraction into string
	}
}
#endif

#pragma mark -

/****************** ALLOC HANDLE ********************/

Handle	AllocHandle(long size)
{
Handle	hand;

	hand = NewHandle(size);							// alloc in APPL
	if (hand == nil)
	{
		DoAlert("AllocHandle: failed!");
		return(nil);
	}
	return(hand);									
}


/****************** ALLOC PTR ********************/

Ptr	AllocPtr(long size)
{
Ptr	pr;

	pr = NewPtr(size);						// alloc in Application
	return(pr);
}



#pragma mark -


/***************** P STRING TO C ************************/

void PStringToC(char *pString, char *cString)
{
Byte	pLength,i;

	pLength = pString[0];
	
	for (i=0; i < pLength; i++)					// copy string
		cString[i] = pString[i+1];
		
	cString[pLength] = 0x00;					// add null character to end of c string
}


/***************** DRAW C STRING ********************/

void DrawCString(char *string)
{
	while(*string != 0x00)
		DrawChar(*string++);
}


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


/************* COPY PSTR **********************/

void CopyPStr(ConstStr255Param	inSourceStr, StringPtr	outDestStr)
{
short	dataLen = inSourceStr[0] + 1;
	
	BlockMoveData(inSourceStr, outDestStr, dataLen);
	outDestStr[0] = dataLen - 1;
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
