/****************************/
/*    SELF RUNNING DEMO.C   */
/* (c)1996 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/


extern	short			gMainAppRezFile;
extern	Byte			gCurrentLevel,gMyCharacterType;
extern	Boolean			gAbortedFlag,gGameOverFlag;


/****************************/
/*    PROTOTYPES            */
/****************************/


/****************************/
/*    CONSTANTS             */
/****************************/

#define	DEMO_SEED			1234L



/**********************/
/*     VARIABLES      */
/**********************/

DemoCacheKeyType	**gDemoCacheHandle,*gDemoCachePtr;
long				gDemoCacheIndex;

Byte		gDemoMode = DEMO_MODE_OFF;
Boolean		gAbortDemoFlag,gGameIsDemoFlag;


/*************** START RECORDING DEMO *****************/
//
// Begins to record key commands for playback later
//
// The demo data is in the following format:
//     short	# iterations of following KeyMap
//	   KeyMap	the KeyMap in question
//

void StartRecordingDemo(void)
{
	SysBeep(0);
	
			/* ALLOC MEMORY FOR DEMO DATA */
			
	gDemoCacheHandle = (DemoCacheKeyType **)AllocHandle(MAX_DEMO_SIZE);	// alloc memory for it
	if (gDemoCacheHandle == nil)
		DoFatalAlert("Cant allocate memory for record buffer.");
	HLockHi((Handle)gDemoCacheHandle);
	gDemoCachePtr = *gDemoCacheHandle;
	gDemoCacheIndex = 0;									// start @ -1 b/c 1st instance will ++ it.
	gDemoCachePtr[0].count = 0;				// init first entry
	gDemoCachePtr[0].keyMap[0] = 0;	
	gDemoCachePtr[0].keyMap[1] = 0;
	gDemoCachePtr[0].keyMap[2] = 0;
	gDemoCachePtr[0].keyMap[3] = 0;			

			/* INIT STATE */
			
	gAbortDemoFlag = false;
	gGameIsDemoFlag = false;


	gDemoMode = DEMO_MODE_RECORD;
	InitMyRandomSeed();								// always use same seed!
	QD3D_CalcFramesPerSecond();								// set the fps stuff now
}


/***************** SAVE DEMO DATA *************************/
//
// Save keyboard data for demo playback later
//

void SaveDemoData(void)
{
Handle		resHandle;

	if (gDemoMode != DEMO_MODE_RECORD)							// be sure we were recording
		return;

	gDemoCachePtr[gDemoCacheIndex++].count = END_DEMO_MARK;		// put end mark @ end of file

	SetHandleSize((Handle)gDemoCacheHandle, sizeof(DemoCacheKeyType) * gDemoCacheIndex);		// resize data


				/* NUKE OLD RESOURCE */

	resHandle = GetResource('dEmo',1000);
	if (resHandle != nil)
	{
		RemoveResource(resHandle);								// nuke old one
		ReleaseResource(resHandle);
	}
			
			/* SAVE DATA AS RESOURCE */
			
	resHandle = (Handle)gDemoCacheHandle;
	AddResource(resHandle, 'dEmo', 1000, "Demo Data" );		// add resource

	if ( ResError() )
		DoFatalAlert("SaveDemoData: AddResource failed!");
	WriteResource( resHandle );									// update it
	ReleaseResource(resHandle);									// nuke resource / recorded data

			/* CLEAN UP */
				
	gDemoCacheHandle = nil;	
	gDemoCachePtr = nil;
	gDemoMode = DEMO_MODE_OFF;
}


/********************* INIT DEMO PLAYBACK ****************/

void InitDemoPlayback(void)
{
			/* INIT STUFF */
			
	gAbortDemoFlag 	= false;
	gGameIsDemoFlag = true;
	gDemoMode 		= DEMO_MODE_PLAYBACK;
	InitMyRandomSeed();								// always use same seed!

				/* LOAD DEMO DATA */

	gDemoCacheHandle = (DemoCacheKeyType **)GetResource('dEmo',1000);	// read the resource
	if (gDemoCacheHandle == nil)
		DoFatalAlert("Error reading Demo Resource!");
			
	DetachResource((Handle)gDemoCacheHandle);						// detach resource	
	HLockHi((Handle)gDemoCacheHandle);
	gDemoCachePtr = *gDemoCacheHandle;
	
	gDemoCacheIndex = 0;
	QD3D_CalcFramesPerSecond();								// set the fps stuff now
	
	
}


/******************* STOP DEMO ******************/

void StopDemo(void)
{
	if (gDemoMode == DEMO_MODE_PLAYBACK)
	{
		DisposeHandle((Handle)gDemoCacheHandle);
		gDemoCachePtr = nil;
		gDemoCacheHandle = nil;
		gDemoMode = DEMO_MODE_OFF;			// set back to OFF
		gAbortDemoFlag = true;
		gAbortedFlag = true;
		gGameOverFlag = true;
		UpdateInput();						// read keyboard to reset it all
	}
}



