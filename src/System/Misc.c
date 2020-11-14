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
extern	float			gDemoVersionTimer;
extern	FSSpec		gDataSpec;

static Boolean ValidateRegistrationNumber(unsigned char *regInfo);
static void DoRegistrationDialog(unsigned char *out);


/****************************/
/*    CONSTANTS             */
/****************************/

#define		ERROR_ALERT_ID		401




/**********************/
/*     VARIABLES      */
/**********************/

short	gPrefsFolderVRefNum;
long	gPrefsFolderDirID;



unsigned long seed0 = 0, seed1 = 0, seed2 = 0;

Boolean     gGameIsRegistered = false;

unsigned char	gRegInfo[64];
Str255  gRegFileName = "\p:Bugdom:Info";


/**********************/
/*     PROTOTYPES     */
/**********************/


/****************** DO SYSTEM ERROR ***************/

void ShowSystemErr(long err)
{
Str255		numStr;

#if 0
	if (gDisplayContext)
		GammaOn();
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	UseResFile(gMainAppRezFile);
#endif
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

#if 0
	if (gDisplayContext)
		GammaOn();
#endif
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
//	UseResFile(gMainAppRezFile);
	NumToStringC(err, numStr);
	DoAlert (numStr);
}

/*********************** DO ALERT *******************/

void DoAlert(Str255 s)
{

#if 1
	printf("BUGDOM ALERT: %s\n", s);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bugdom", s, NULL);
#else
	if (gDisplayContext)
		GammaOn();
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	InitCursor();
	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);	
	ParamText(s,NIL_STRING,NIL_STRING,NIL_STRING);
	NoteAlert(ERROR_ALERT_ID,nil);
	
#endif
}


/*********************** DO ALERT NUM *******************/

void DoAlertNum(int n)
{

#if 1
	static char alertbuf[1024];
	snprintf(alertbuf, 1024, "Alert #%d", n);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bugdom", alertbuf, NULL);
#else
	if (gDisplayContext)
		GammaOn();
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	InitCursor();
	NoteAlert(n,nil);
	
#endif
}


		
/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(Str255 s)
{
#if 1
	printf("BUGDOM FATAL ALERT: %s\n", s);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bugdom", s, NULL);
#else
OSErr	iErr;
	
	if (gDisplayContext)
		GammaOn();
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	UseResFile(gMainAppRezFile);

	InitCursor();
	ParamText(s,NIL_STRING,NIL_STRING,NIL_STRING);
	iErr = NoteAlert(ERROR_ALERT_ID,nil);
#endif
	CleanQuit();
}

/*********************** DO FATAL ALERT 2 *******************/

void DoFatalAlert2(Str255 s1, Str255 s2)
{
#if 1
	printf("BUGDOM FATAL ALERT: %s - %s\n", s1, s2);
	static char alertbuf[1024];
	snprintf(alertbuf, 1024, "%s\n%s", s1, s2);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bugdom", alertbuf, NULL);
#else
	if (gDisplayContext)
		GammaOn();
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	UseResFile(gMainAppRezFile);
	InitCursor();
	ParamText(s1,s2,NIL_STRING,NIL_STRING);
	Alert(402,nil);
//	ShowMenuBar();
#endif
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
static unsigned char format[] = "\p$XXXXXXXX";				// Declare format static so we can return a pointer to it
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
static unsigned char format[] = "\pXXXXXXXX";				// Declare format static so we can return a pointer to it
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
		DoAlert("\pAllocHandle: failed!");
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
		DoAlert("\pWarning: Cannot locate the Preferences folder.");

	iErr = DirCreate(gPrefsFolderVRefNum,gPrefsFolderDirID,"\pBugdom",&createdDirID);		// make folder in there
	


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


#pragma mark -

#if OEM

#include <NameRegistry.h>
#include <string.h>

static OSStatus GetMacName (StringPtr *macName);


/*********************** VERIFY APPLE OEM MACHINE **************************/

static Boolean VerifyAppleOEMMachine(void)
{
Boolean					result 			= true;
OSStatus				err				= noErr;
StringPtr				macName			= nil,
						macNameGestalt	= nil;


				/*******************************/
				/* GET THE MACHINE NAME STRING */
				/*******************************/
				
//	err = Gestalt ('mnam', (long*)&macNameGestalt);
//	if (err == noErr)
//	{
//		/* Make a copy since we can't modify the returned value. */
//		/* Make sure we dispose of the copy when we are done with it. */
//		macName = (StringPtr)NewPtr (macNameGestalt[0] + 1);
//		if (macName != nil)
//			BlockMoveData (macNameGestalt, macName, macNameGestalt[0] + 1);
//	}
//	else
	{
		/* We have to dispose of macName when we are done with it. */
		err = GetMacName (&macName);
	}


		/*******************************************************/
		/* SEE IF MACHINE NAME MATCHES ANY OF THE OEM MACHINES */
		/*******************************************************/


	if (macName != nil)
	{
		if ((CompareString(macName, "\pPowerMac2,2", nil) == 0) 	||			// see if either machine name is an EXACT match
			(CompareString(macName, "\pPowerBook2,2", nil) == 0) 	||
			(CompareString(macName, "\pPowerMac2,1", nil) == 0) 	||
			(CompareString(macName, "\pPowerBook2,1", nil) == 0))
		{
			result = true;
		}
		else
			result = false;

		DisposePtr ((Ptr)macName);
		macName = nil;
	}
	return(result);
}

/* GetMacName returns a c formatted (null terminated string with the
 * model property string.
 * Input  macName - pointer to a buffer where the model property name
 *                  will be returned, if the call succeeds
 *
 * Output function result - noErr indicates that the model name was
 *                          read successfully
 *        macName - will contain the model name property if noErr
 *
 * Notes:
 *	  Caller is responsible for disposing of macName. Use DisposePtr.
 */
 
static OSStatus GetMacName (StringPtr *macName)
{
OSStatus                err = noErr;
RegEntryID              compatibleEntry;
RegPropertyValueSize	length;
RegCStrEntryNamePtr		compatibleValue;

	if (macName != nil)
	{
		*macName = 0;

	    err = RegistryEntryIDInit (&compatibleEntry);

	    if (err == noErr)
	        err = RegistryCStrEntryLookup (nil, "Devices:device-tree", &compatibleEntry);

		if (err == noErr)
			err = RegistryPropertyGetSize (&compatibleEntry, "compatible", &length);

		if (err == noErr)
		{
			compatibleValue = (RegCStrEntryNamePtr)NewPtr (length);
			err = MemError ();
		}

	    if (err == noErr)
	        err = RegistryPropertyGet (&compatibleEntry, "compatible", compatibleValue, &length);

		if (err == noErr)
		{
			SetPtrSize (compatibleValue, strlen (compatibleValue) + 1);
			/* SetPtrSize shouldn't fail because we are shrinking the pointer, but make sure. */
			err = MemError ();
		}

		if (err == noErr)
			*macName = c2pstr (compatibleValue);

	    (void)RegistryEntryIDDispose (&compatibleEntry);
	}

    return err;
}


#endif



#pragma mark -


/********************** CHECK GAME REGISTRATION *************************/

void CheckGameRegistration(void)
{
OSErr   iErr;
FSSpec  spec;
short		fRefNum;
long        	numBytes = REG_LENGTH;
unsigned char	regInfo[REG_LENGTH];

            /* GET SPEC TO REG FILE */

	iErr = FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gRegFileName, &spec);
    if (iErr)
        goto game_not_registered;


            /*************************/
            /* VALIDATE THE REG FILE */
            /*************************/

            /* READ REG DATA */

    if (FSpOpenDF(&spec,fsCurPerm,&fRefNum) != noErr)
        goto game_not_registered;
    	
	FSRead(fRefNum,&numBytes,regInfo);
	
    FSClose(fRefNum);

            /* VALIDATE IT */

    if (!ValidateRegistrationNumber(&regInfo[0]))
        goto game_not_registered;        

    gGameIsRegistered = true;
    return;

        /* GAME IS NOT REGISTERED YET, SO DO DIALOG */

game_not_registered:

    DoRegistrationDialog(&regInfo[0]);

    if (gGameIsRegistered)                                  // see if write out reg file
    {
	    FSpDelete(&spec);	                                // delete existing file if any
	    iErr = FSpCreate(&spec,'BalZ','xxxx',-1);
        if (iErr == noErr)
        {
        	numBytes = REG_LENGTH;
			FSpOpenDF(&spec,fsCurPerm,&fRefNum);
			FSWrite(fRefNum,&numBytes,regInfo);
		    FSClose(fRefNum);
     	}  
    }
    
            /* DEMO MODE */
    else
    {
		/* SEE IF TIMER HAS EXPIRED */

        GetDemoTimer();
    	if (gDemoVersionTimer > (60 * 60))		// let play for n minutes
    	{
			DoAlertNum(136);
    		CleanQuit();
    	}
    }

}


/********************* VALIDATE REGISTRATION NUMBER ******************/

static Boolean ValidateRegistrationNumber(unsigned char *regInfo)
{
#if 1
	SOURCE_PORT_MINOR_PLACEHOLDER();
	return true;
#else
int		c;
short   i,j;
static unsigned char pirateNumbers[6][REG_LENGTH] =
{
	"AAAAAALLLLLL",
	"LLLLLLAAAAAA",
	"LALALALALALA",
	"ALALALALALAL",
	"AIADGCJFILDL",
	"ABCDEFGHIJKL",
};


			/*************************/
            /* VALIDATE ENTERED CODE */
            /*************************/
    
    for (i = 0, j = REG_LENGTH-1; i < REG_LENGTH; i += 2, j -= 2)     // get checksum 
    {
        Byte    value,c,d;

		if ((regInfo[i] >= 'a') && (regInfo[i] <= 'z'))	// convert to upper case
			regInfo[i] = 'A' + (regInfo[i] - 'a');

		if ((regInfo[j] >= 'a') && (regInfo[j] <= 'z'))	// convert to upper case
			regInfo[j] = 'A' + (regInfo[j] - 'a');

        value = regInfo[i] - 'A';           // convert letter to digit 0..9
        c = ('L' - regInfo[j]);             // convert character to number

        d = c - value;                      // the difference should be == i

        if (d != 0)
            return(false);
    }


			/**********************************/
			/* CHECK FOR KNOWN PIRATE NUMBERS */
			/**********************************/
			
	for (j = 0; j < 6; j++)
	{
		for (i = 0; i < REG_LENGTH; i++)
		{
			if (regInfo[i] != pirateNumbers[j][i])					// see if doesn't match
				goto next_code;		
		}
		
				/* THIS CODE IS PIRATED */
				
		return(false);
		
next_code:;		
	}
			

			/*******************************/
			/* SECONDARY CHECK IN REZ FILE */
			/*******************************/

	c = CountResources('BseR');						// count how many we've got stored
	for (j = 0; j < c; j++)
	{
		Handle	hand = GetResource('BseR',128+j);			// read the #
	
		for (i = 0; i < REG_LENGTH; i++)
		{
			if (regInfo[i] != (*hand)[i])			// see if doesn't match
				goto next2;		
		}
	
				/* THIS CODE IS PIRATED */
				
		return(false);
	
next2:		
		ReleaseResource(hand);		
	}



    return(true);
#endif
}



/****************** DO REGISTRATION DIALOG *************************/

static void DoRegistrationDialog(unsigned char *out)
{
#if 1
	SOURCE_PORT_PLACEHOLDER();
#else
DialogPtr 		myDialog;
Boolean			dialogDone = false, isValid;
short			itemType,itemHit;
ControlHandle	itemHandle;
Rect			itemRect;
Str255          regInfo;

	InitCursor();

	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	myDialog = GetNewDialog(128,nil,MOVE_TO_FRONT);
	
				/* DO IT */
				
	while(dialogDone == false)
	{
		ModalDialog(nil, &itemHit);
		switch (itemHit)
		{
			case	1:									        // Register
					GetDialogItem(myDialog,4,&itemType,(Handle *)&itemHandle,&itemRect);
					GetDialogItemText((Handle)itemHandle,regInfo);
                    BlockMove(&regInfo[1], &regInfo[0], 100);         // shift out length byte

                    isValid = ValidateRegistrationNumber(regInfo);    // validate the number
    
                    if (isValid == true)
                    {
                        gGameIsRegistered = true;
                        dialogDone = true;
                        BlockMove(regInfo, out, REG_LENGTH);		// copy to output
                    }
                    else
                    {
                        DoAlert("\pSorry, that registration code is not valid.  Please try again.");
                    }
					break;

            case    2:                                  // Demo
                    dialogDone = true;
                    break;
					
			case 	3:									// QUIT
                    CleanQuit();
					break;					
		}
	}
	DisposeDialog(myDialog);
	HideCursor();
#endif
}
















