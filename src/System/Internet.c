/****************************/
/*      INTERNET ROUTINES   */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

extern	PrefsType	gGamePrefs;
extern	unsigned char	gRegInfo[64];
extern	short	gPrefsFolderVRefNum;
extern	long	gPrefsFolderDirID;
extern	Str255  gRegFileName;


/****************************/
/*    PROTOTYPES            */
/****************************/

static OSStatus DownloadURL(const char *urlString);
static Boolean IsInternetAvailable(void);

static OSErr SkipToPound(void);
static OSErr FindBufferStart(void);
static Boolean InterpretTag(void);
static void ParseNextWord(Str255 s);
static void InterpretVersionName(void);
static void InterpretNoteName(void);
static void InterpretBadSerialsName(void);


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	kNotHTTPURLErr 			= -666,
	kWhatImplementationErr 	= -667
};

enum
{
	kTransferBufferSize = 4096
};


/**********************/
/*     VARIABLES      */
/**********************/

static	Handle		gHTTPDataHandle = nil;
static	int			gHTTPDataSize = 0;
static	int			gHTTPDataIndex = 0;


/************************** READ HTTP DATA: VERSION INFO **********************************/

void ReadHTTPData_VersionInfo(void)
{
OSStatus err;
char urlString[] = "http://www.pangeasoft.net/bug/files/updatedata";
	

	
				/* DONT DO IT UNLESS THERE'S A TCP/IP CONNECTION */
				
	if (!IsInternetAvailable())
		return;
		
	
	
			/* ALLOCATE BUFFER TO STORE DATA */
			
	if (gHTTPDataHandle == nil)
	{
		gHTTPDataSize = 20000;
		gHTTPDataHandle = AllocHandle(gHTTPDataSize);					// alloc 20k buffer
	}			
	
	
			/* DOWNLOAD DATA TO BUFFER */
			
	err = DownloadURL(urlString);		
	if (err)
		return;
			

			/****************/
			/* PARSE BUFFER */
			/****************/

			/* FIND BUFFER START */
	
	FindBufferStart();
	
			/* READ AND HANDLE ALL TAGS */
			
	while (!InterpretTag())
	{
	
	};							
	
	
			/* CLEANUP */
			
	DisposeHandle(gHTTPDataHandle);
	gHTTPDataHandle = nil;
	gHTTPDataSize = 0;
}


/****************** INTERPRET TAG **********************/
//
// Returns true when done or error
//

static Boolean InterpretTag(void)
{
u_long	name, *np;
char	*c;

	SkipToPound();													// skip to next tag

	c = *gHTTPDataHandle;											// point to buffer
	np = (u_long *)(c + gHTTPDataIndex);							// point to tag name
	name = *np;														// get tag name
	gHTTPDataIndex += 4;											// skip over it

	switch(name)
	{	
		case	'BVER':												// BEST VERSION name
				InterpretVersionName();
				break;

		case	'NOTE':												// NOTE name
				InterpretNoteName();
				break;
				
		case	'BSER':												// BAD SERIALS name
				InterpretBadSerialsName();
				break;

		case	'TEND':												// END name
				return(true);
	}
	
	return(false);
}			


/******************* INTERPRET VERSION NAME ************************/
//
// We got a VERS tag, so let's read the version and see if our app's verion is this recent.
//

static void InterpretVersionName(void)
{
Str255		s;
NumVersion	*fileVers;
NumVersion	bestVers;
Handle		rez;

			/* READ VERSION STRING */
			
	ParseNextWord(s);
	
		
		/* CONVERT VERSION STRING TO NUMVERSION */
		
	bestVers.majorRev = s[1] - '0';					// convert character to number
	bestVers.minorAndBugRev = s[3] - '0';
	bestVers.minorAndBugRev <<= 4;
	bestVers.minorAndBugRev |= s[5] - '0';
		
	
			/* GET OUR VERSION */
			
	rez = GetResource('vers', 1);					// get vers resource	
	fileVers = (NumVersion *)(*rez);				// get ptr to vers data


		/* SEE IF THERE'S AN UPDATE */
			
	if (bestVers.majorRev > fileVers->majorRev)		// see if there's a major rev better
	{
		goto update;	
	}
	else
	if ((bestVers.majorRev == fileVers->majorRev) &&		// see if there's a minor rev better
		(bestVers.minorAndBugRev > fileVers->minorAndBugRev))
	{
update:
		ParamText(s,NIL_STRING,NIL_STRING,NIL_STRING);
		NoteAlert(137,nil);
	}

	ReleaseResource(rez);							// free rez
}


/********************* INTERPRET NOTE NAME ***************************/
//
// We got a NOTE tag, so let's interpret it and display the note in a dialog.
//

static void InterpretNoteName(void)
{
int		noteID,i;
char	*c = *gHTTPDataHandle;											// point to buffer
Str255	s;

		/****************************/
		/* READ NOTE'S 4-DIGIT CODE */
		/****************************/

	noteID = (c[gHTTPDataIndex++] - '0') * 1000;
	noteID += (c[gHTTPDataIndex++] - '0') * 100;
	noteID += (c[gHTTPDataIndex++] - '0') * 10;
	noteID += (c[gHTTPDataIndex++] - '0');
	
		
		/* SEE IF THIS NOTE HAS ALREADY BEEN SEEN */
			
	if (gGamePrefs.didThisNote[noteID])
		return;


			/********************/
			/* READ NOTE STRING */
			/********************/
			
	i = 1;
	s[0] = 0;
	while(c[gHTTPDataIndex] != '#')							// scan until next # marker
	{
		s[i++] = c[gHTTPDataIndex++];
		s[0]++;
	}

			/* DO ALERT WITH NOTE */
			
	ParamText(s,NIL_STRING,NIL_STRING,NIL_STRING);
	NoteAlert(138,nil);
	
	gGamePrefs.didThisNote[noteID] = 0xff;
	SavePrefs(&gGamePrefs);
}


/********************* INTERPRET BAD SERIALS NAME ***************************/
//
// We got a BSER tag, so let's read each serial and see if it's ours.
//

static void InterpretBadSerialsName(void)
{
#if SHAREWARE									// only care if shareware version
int		i,count;
Str255	s;
OSErr   iErr;
FSSpec  spec;
Handle	hand;

	count = 0;
	
	while(true)
	{
		ParseNextWord(s);												// read next word
		if (s[1] == '*')												// see if this is the end marker
			break;
			
			
			/* SAVE CODE INTO PERMANENT REZ */
			
		hand = GetResource('BseR', 128+count);							// get existing rez
		if (hand)
		{
			BlockMove(&s[1], *hand, REG_LENGTH);						// copy code into handle		
			ChangedResource(hand);
		}
		else
		{
			hand = AllocHandle(REG_LENGTH);								// alloc handle
			BlockMove(&s[1], *hand, REG_LENGTH);						// copy code into handle		
			AddResource(hand, 'BseR', 128+count, "\p");					// write rez to file
		}
		WriteResource(hand);
		ReleaseResource(hand);
		count++;
			
			
			/* COMPARE THIS ILLEGAL SERIAL WITH OURS */
			
		for (i = 0; i < REG_LENGTH; i++)
		{
			if (gRegInfo[i] != s[1+i])									// doesn't match
				goto next_code;
		}
		
				/* CODE WAS A MATCH, SO WIPE IT */

		iErr = FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gRegFileName, &spec);
	    if (iErr == noErr)
		{
			FSpDelete(&spec);											// delete the registration file
			DoAlert("\pThe serial number being used is invalid.  Please enter a valid registration code to continue playing.");
		}
		gGamePrefs.lastVersCheckDate.year = 0;							// reset date so will check again next launch
		SavePrefs(&gGamePrefs);
		CleanQuit();
		return;
			
next_code:;			
	}
#endif	
}



/********************* PARSE NEXT WORD ****************************/
//
// Reads the next "word' from the buffer.  A word is text between
// any spaces or returns.
// 

static void ParseNextWord(Str255 s)
{
char 	*c = *gHTTPDataHandle;														// point to buffer
int		i = 1;

		/* SKIP BLANKS AT FRONT */
		
	while((c[gHTTPDataIndex] == ' ') || (c[gHTTPDataIndex] == CHAR_RETURN))
	{
		gHTTPDataIndex++;
	}


		/* READ THE GOOD CHARS */
		
	do
	{
		s[i] = c[gHTTPDataIndex];												// copy from buffer to string
		gHTTPDataIndex++;
		s[0] = i;
		i++;
	}while((c[gHTTPDataIndex] != ' ') && (c[gHTTPDataIndex] != CHAR_RETURN));	// stop when reach space or CR
}


/********************* FIND BUFFER START ***************************/
//
// Skips to the #$#$ tag in the buffer which denotes the beginning of our data
//

static OSErr FindBufferStart(void)
{
	gHTTPDataIndex = 0; 						// start @ beginning of file		
	
	while(true)
	{
		char *c = *gHTTPDataHandle;				// point to buffer
		
		SkipToPound();
		
		if ((c[gHTTPDataIndex] == '$') &&		// see if $#$ 
			(c[gHTTPDataIndex+1] == '#') &&
			(c[gHTTPDataIndex+2] == '$'))
		{
			SkipToPound();						// skip to next tag
			return(noErr);		
		}
		
		if (gHTTPDataIndex > gHTTPDataSize)		// see if out of buffer range
			return(1);
	};
	
}



/********************** SKIP TO POUND ************************/
//
// Skips to the next # character in the buffer
//

static OSErr SkipToPound(void)
{
char	*c;

	c = *gHTTPDataHandle;						// point to buffer

	while(c[gHTTPDataIndex] != '#')				// scan for #
	{
		gHTTPDataIndex++;
		if (gHTTPDataIndex >= gHTTPDataSize)	// see if out of buffer range
			return(1);
	}

	gHTTPDataIndex++;							// skip past the #

	return(noErr);
}


#pragma mark -


/************************** DOWNLOAD URL **********************************/

static OSStatus DownloadURL(const char *urlString)
{
OSStatus 	err;

	if (gHTTPDataHandle == nil)
	{
		DoFatalAlert("\pDownloadURL: gHTTPDataHandle == nil");
	
	}

			/* DOWNLOAD DATA */

	err = URLSimpleDownload(
						urlString,					// pass the URL string
						nil,						// download to handle, not FSSpec
						gHTTPDataHandle,			// to this handle
						0, 							// no flags
						nil,						// no callback eventProc
						nil);							// no userContex
						


	return(err);
	

}


#pragma mark -

/************************ IS INTERNET AVAILABLE ****************************/

static Boolean IsInternetAvailable(void)
{
OSErr				iErr;
InetInterfaceInfo 	info;

		// NOTE:  this will fail when run in Classic mode :(
		
		
	iErr = OTInetGetInterfaceInfo(&info, kDefaultInetInterface);


	if (iErr)
		return(false);

	return(true);
}







