/****************************/
/*   	HIGHSCORES.C    	*/
/* (c)1996-99 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	float			gFramesPerSecond,gFramesPerSecondFrac;
extern	WindowPtr		gCoverWindow;
extern	long			gScreenXOffset,gScreenYOffset;
extern	TQ3Object		gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	short	gPrefsFolderVRefNum;
extern	long	gPrefsFolderDirID;
extern	FSSpec	gDataSpec;
extern	char	gTypedAsciiKey;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupHighScoresScreen(void);
static void AddNewScore(unsigned long newScore);
static void EnterPlayerName(unsigned long newScore);
static void MoveCursor(ObjNode *theNode);
static void TypeNewKey(char c);
static void UpdateNameAndCursor(Boolean doCursor, float x, float y, float z);
static void SaveHighScores(void);
static void LoadHighScores(void);
static void PrepHighScoresShow(void);

/***************************/
/*    CONSTANTS            */
/***************************/

#define	NUM_SCORES		10
#define	MAX_NAME_LENGTH	11

#define	LETTER_SEPARATION	19
#define	LEFT_EDGE			-100

typedef struct
{
	char			name[MAX_NAME_LENGTH];
	unsigned long	score;
}HighScoreType;



/***************************/
/*    VARIABLES            */
/***************************/

static HighScoreType	gNewName;	
static ObjNode			*gNewNameObj[MAX_NAME_LENGTH];

static Byte		gCursorPosition;
static ObjNode	*gCursorObj,*gScoreCyc;

HighScoreType	gHighScores[NUM_SCORES];	

Boolean			gEnteringName = false;


/************** SHOW HIGHSCORES SCREEN *******************/
//
// INPUT: 	newScore = score to try to add, 0 == just show me
//

void ShowHighScoresScreen(unsigned long newScore)
{
TQ3Vector3D	camDelta = {0,0,0};

	PlaySong(SONG_HIGHSCORES,true);

			/* INIT */
			
	SetupHighScoresScreen();
	MakeFadeEvent(true);
	
	
		/* ADD NEW SCORE */
			
	AddNewScore(newScore);
	

		/*************************/
		/* PREP HIGH SCORES SHOW */
		/*************************/
		
	PrepHighScoresShow();

		
			/*******************/
			/* SHOW THE SCORES */
			/*******************/

	do
	{
		QD3D_CalcFramesPerSecond();					
		MoveObjects();
		
				/* MOVE CAMERA */
				
		camDelta.y = gFramesPerSecondFrac * -45.0f;
		QD3D_MoveCameraFromTo(gGameViewInfoPtr, &camDelta, &camDelta);			// update camera position		
		
		gScoreCyc->Coord.y -= gFramesPerSecondFrac * 20.0f;
		UpdateObjectTransforms(gScoreCyc);
		
		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);	
		DoSDLMaintenance();

		UpdateInput();
		if (Button())
			break;
		if (AreAnyNewKeysPressed())
			break;		
			
	}while(gGameViewInfoPtr->currentCameraCoords.y > -500);
	
	
			/* CLEANUP */
				
	DeleteAllObjects();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);
	Free3DMFGroup(MODEL_GROUP_HIGHSCORES);
	DisposeSoundBank(SOUND_BANK_DEFAULT);
	GammaFadeOut();
	GameScreenToBlack();
}


/********************* SETUP HIGHSCORES SCREEN ******************************/

static void SetupHighScoresScreen(void)
{
TQ3Point3D			cameraFrom = { 0, 00, 400.0 };
TQ3Point3D			cameraTo = { 0.0, 0, 0.0 };
TQ3Vector3D			cameraUp = { 0.0, 1.0, 0.0 };
TQ3ColorARGB		clearColor = {1,0,0,0};
TQ3ColorRGB			ambientColor = { 1.0, 1.0, 1.0 };
TQ3Vector3D			fillDirection1 = { .7, -.1, -0.3 };
TQ3Vector3D			fillDirection2 = { -1, -.3, -.4 };
FSSpec				file;
QD3DSetupInputType		viewDef;
	
	DeleteAllObjects();
	
			/***********************/
			/* SET QD3D PARAMETERS */
			/***********************/
			
	QD3D_NewViewDef(&viewDef, gCoverWindow);			
	viewDef.view.clearColor 		= clearColor;
		
	viewDef.camera.from 			= cameraFrom;
	viewDef.camera.to 				= cameraTo;
	viewDef.camera.up 				= cameraUp;
	viewDef.camera.hither 			= 5;
	viewDef.camera.yon 				= 2000;
	viewDef.camera.fov 				= .7;

	viewDef.lights.ambientBrightness = 0.2;
	viewDef.lights.ambientColor 	= ambientColor;
	viewDef.lights.numFillLights 	= 2;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[0] 	= ambientColor;
	viewDef.lights.fillColor[1] 	= ambientColor;
	viewDef.lights.fillBrightness[0] = 1.0;
	viewDef.lights.fillBrightness[1] = .4;


	viewDef.lights.useFog = true;

	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);

		/***************/
		/* LOAD MODELS */
		/***************/
		
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:HighScores.3dmf", &file);
	LoadGrouped3DMF(&file, MODEL_GROUP_HIGHSCORES);

	
		/* LOAD CURRENT SCORES */

	LoadHighScores();	
	
	
		/* LOAD AUDIO */
		
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Main.sounds", &file);
	LoadSoundBank(&file, SOUND_BANK_DEFAULT);
}


/*********************** LOAD HIGH SCORES ********************************/

static void LoadHighScores(void)
{
OSErr				iErr;
short				refNum;
FSSpec				file;
long				count;

				/* OPEN FILE */
					
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":Bugdom:HighScores", &file);
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);	
	if (iErr == fnfErr)
		ClearHighScores();
	else
	if (iErr)
		DoFatalAlert("LoadHighScores: Error opening High Scores file!");
	else
	{
		count = sizeof(HighScoreType) * NUM_SCORES;
		iErr = FSRead(refNum, &count,  &gHighScores[0]);								// read data from file
		if (iErr)
		{
			FSClose(refNum);			
			FSpDelete(&file);												// file is corrupt, so delete
//			DoAlert("LoadHighScores: FSRead failed!");
//			ShowSystemErr(iErr);
			return;
		}
		FSClose(refNum);			
	}	
}


/************************ SAVE HIGH SCORES ******************************/

static void SaveHighScores(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

				/* CREATE BLANK FILE */
				
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":Bugdom:HighScores", &file);
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, 'BalZ', 'Skor', smSystemScript);					// create blank file
	if (iErr)
		goto err;


				/* OPEN FILE */
					
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":Bugdom:HighScores", &file);
	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
	{
err:	
		DoAlert("Unable to Save High Scores file!");
		return;
	}

				/* WRITE DATA */
				
	count = sizeof(HighScoreType) * NUM_SCORES;
	FSWrite(refNum, &count, &gHighScores[0]);	
	FSClose(refNum);			

}


/**************** CLEAR HIGH SCORES **********************/

void ClearHighScores(void)
{
short				i,j;
char				blank[] = "               ";

			/* INIT SCORES */
			
	for (i=0; i < NUM_SCORES; i++)
	{
		for (j=0; j < MAX_NAME_LENGTH; j++)
			gHighScores[i].name[j] = blank[j];
		gHighScores[i].score = 0;
	}

	SaveHighScores();		
}


/*************************** ADD NEW SCORE ****************************/

static void AddNewScore(unsigned long newScore)
{
short	slot,i;

			/* FIND INSERT SLOT */
	
	for (slot=0; slot < NUM_SCORES; slot++)
	{
		if (newScore > gHighScores[slot].score)
			goto	got_slot;
	}
	return;
	
	
got_slot:
			/* GET PLAYER'S NAME */
			
	EnterPlayerName(newScore);
	
			/* INSERT INTO LIST */

	for (i = NUM_SCORES-1; i > slot; i--)						// make hole
		gHighScores[i] = gHighScores[i-1];
	gHighScores[slot] = gNewName;								// insert new name 'n stuff
	
			/* UPDATE THE FILE */
			
	SaveHighScores();
}

#pragma mark -

/******************** ENTER PLAYER NAME **************************/

static void EnterPlayerName(unsigned long newScore)
{
ObjNode		*frameObj;
TQ3Point3D	camPt = gGameViewInfoPtr->currentCameraCoords;
float		camWobble = 0;
short		i;
#if 0
EventRecord	theEvent;
#endif

				/*********/
				/* SETUP */
				/*********/
			
			/*************************/
			/* CREATE THE BACKGROUND */
			/*************************/
	
	gNewObjectDefinition.type 		= SCORES_ObjType_Background;
	gNewObjectDefinition.group 		= MODEL_GROUP_HIGHSCORES;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER|STATUS_BIT_NOFOG;
	gNewObjectDefinition.slot 		= 10;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.0;
	gScoreCyc = MakeNewDisplayGroupObject(&gNewObjectDefinition);
			
			/* MAKE NAME FRAME */
			
	gNewObjectDefinition.group = MODEL_GROUP_HIGHSCORES;
	gNewObjectDefinition.type = SCORES_ObjType_NameFrame;
	gNewObjectDefinition.coord.x = 0;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = 0;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = 10;
	gNewObjectDefinition.moveCall = nil;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 1;
	frameObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* MAKE CURSOR */
			
	gNewObjectDefinition.type = SCORES_ObjType_Cursor;
	gNewObjectDefinition.coord.x = LEFT_EDGE;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = 0;
	gNewObjectDefinition.moveCall = MoveCursor;
	gCursorObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	MakeObjectKeepBackfaces(gCursorObj);
	
	for (i=0; i < MAX_NAME_LENGTH; i++)					// init name to blank
	{
		gNewName.name[i] = ' ';
		gNewNameObj[i] = nil;
	}
	gNewName.score = newScore;							// set new score
	gCursorPosition = 0;
	
	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);		// use event manager to read keyboard, so start fresh
	
	
			/*************/
			/* MAIN LOOP */
			/*************/
		
	QD3D_CalcFramesPerSecond();					
		
	gEnteringName = true;
		
	do
	{
		QD3D_CalcFramesPerSecond();					
		MoveObjects();
		
				/* CHECK FOR KEY */
				
		UpdateInput();

		char newKey = '\0';
		if (GetNewKeyState(kVK_LeftArrow))
			newKey = CHAR_LEFT;
		else if (GetNewKeyState(kVK_RightArrow))
			newKey = CHAR_RIGHT;
		else if (GetNewKeyState(kVK_Delete))
			newKey = CHAR_DELETE;
		else if (GetNewKeyState(kVK_ForwardDelete))
			newKey = CHAR_FORWARD_DELETE;
		else
			newKey = gTypedAsciiKey;

		if (newKey)
		{
			TypeNewKey(newKey);
			UpdateNameAndCursor(true,LEFT_EDGE,0,0);
		}
		
				/* MOVE CAMERA */
				
		camWobble += gFramesPerSecondFrac;
		camPt.x = sin(camWobble) * 50;
		QD3D_UpdateCameraFrom(gGameViewInfoPtr, &camPt);	// update camera position
		
				/* DRAW SCENE */
				
		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);	
		DoSDLMaintenance();
	}while(!GetNewKeyState(KEY_RETURN));

			/* CLEANUP */

	DeleteAllObjects();
	gEnteringName = false;
}


/****************** TYPE NEW KEY **********************/
//
// When player types new letter for name, this enters it.
//

static void TypeNewKey(char c)
{
short	i;
			/*******************/
			/* SEE IF EDIT KEY */
			/*******************/

	if (c == CHAR_RETURN)
		return;

				/* DELETE */
				
	if (c == CHAR_DELETE)
	{
		if (gCursorPosition > 0)
		{
			for (i = gCursorPosition-1; i < (MAX_NAME_LENGTH-1); i++)
				gNewName.name[i] = gNewName.name[i+1];
			gNewName.name[MAX_NAME_LENGTH-1] = ' ';
			gCursorPosition--;
		}
		return;
	}

				/* FORWARD DELETE */

	if (c == CHAR_FORWARD_DELETE)
	{
		for (i = gCursorPosition; i < (MAX_NAME_LENGTH-1); i++)
			gNewName.name[i] = gNewName.name[i+1];
		gNewName.name[MAX_NAME_LENGTH-1] = ' ';
		return;
	}

				/* LEFT ARROW */
		
	if (c == CHAR_LEFT)
	{
		if (gCursorPosition > 0)
			gCursorPosition--;
		return;
	}
				/* RIGHT ARROW */
	
	if (c == CHAR_RIGHT)
	{
		if (gCursorPosition < (MAX_NAME_LENGTH-1))
			gCursorPosition++;
		return;
	}

			/* ADD NEW LETTER TO NAME */
			
	gNewName.name[gCursorPosition] = c;							// add to string
	if (gCursorPosition < (MAX_NAME_LENGTH-1))
		gCursorPosition++;
		
	PlayEffect(EFFECT_SELECT);				
}



/*************** MOVE CURSOR *************************/

static void MoveCursor(ObjNode *theNode)
{

			/* MAKE BLINK */
			
	theNode->SpecialF[0] -= gFramesPerSecondFrac;
	if (theNode->SpecialF[0] <= 0)
	{
		theNode->StatusBits ^= STATUS_BIT_HIDDEN;
		theNode->SpecialF[0] = 0.2;
	}
}


/******************** UPDATE NAME AND CURSOR ************************/
//
// INPUT: 	doCursor == false if just do name, not cursor.  Also, won't delete old name.
//			x,y,z = coord offsets for name
//

static void UpdateNameAndCursor(Boolean doCursor, float x, float y, float z)
{
short	i;
char	c;

	if (doCursor)
	{
				/* UPDATE CURSOR COORD */
				
		gCursorObj->Coord.x = LEFT_EDGE + (gCursorPosition*LETTER_SEPARATION);
		UpdateObjectTransforms(gCursorObj);


				/* DELETE OLD NAME */
				
		for (i=0; i < MAX_NAME_LENGTH; i++)								// init name to blank
		{
			if (gNewNameObj[i])
				DeleteObject(gNewNameObj[i]);
			gNewNameObj[i] = nil;
		}
	}

			/* MAKE NEW NAME */
			
	for (i=0; i < MAX_NAME_LENGTH; i++)								// init name to blank
	{
		c = gNewName.name[i];
		if (c != ' ')
		{
			if ((c >= '0') && (c <= '9'))							// see if char is a number
				gNewObjectDefinition.type = SCORES_ObjType_0 + (c - '0');
			else
			if ((c >= 'A') && (c <= 'Z'))							// see if char is a letter
				gNewObjectDefinition.type = SCORES_ObjType_A + (c - 'A');
			else
			if ((c >= 'a') && (c <= 'z'))							// see if char is a (lowercase) letter
				gNewObjectDefinition.type = SCORES_ObjType_A + (c - 'a');
			else
			switch(c)
			{
				case	'#':
						gNewObjectDefinition.type = SCORES_ObjType_Pound;
						break;
				case	'!':
						gNewObjectDefinition.type = SCORES_ObjType_Exclamation;
						break;
				case	'?':
						gNewObjectDefinition.type = SCORES_ObjType_Question;
						break;
				case	CHAR_APOSTROPHE:
						gNewObjectDefinition.type = SCORES_ObjType_Apostrophe;
						break;
				case	'.':
						gNewObjectDefinition.type = SCORES_ObjType_Period;
						break;
				case	':':
						gNewObjectDefinition.type = SCORES_ObjType_Colon;
						break;
				case	'-':
						gNewObjectDefinition.type = SCORES_ObjType_Dash;
						break;
				default:
						continue;
			}


			gNewObjectDefinition.group = MODEL_GROUP_HIGHSCORES;
			gNewObjectDefinition.coord.x = x + (i*LETTER_SEPARATION);
			gNewObjectDefinition.coord.y = y;
			gNewObjectDefinition.coord.z = z;
			gNewObjectDefinition.flags = 0;
			gNewObjectDefinition.slot = 10;
			gNewObjectDefinition.moveCall = nil;
			gNewObjectDefinition.rot = 0;
			gNewObjectDefinition.scale = 1;
			gNewNameObj[i] = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		}
	}
}

#pragma mark -

/******************************* PREP HIGH SCORES SHOW *******************************************/
//
// Creates all the objects for the "show highscores".
//

static void PrepHighScoresShow(void)
{
short	slot,place;
ObjNode	*newObj;
unsigned long score,digit;
static TQ3Point3D	from = {0,1100+40,650};
static TQ3Point3D	to = {0,1100,0};
float		x,y;
const static float xCoords[NUM_SCORES][2] =
{
	50,			-50,
	-84,		84,
	-84,		84,
	-84,		84,
	-84,		84,
	-84,		84,
	-84,		84,
	-84,		84,
	-84,		84,
	-84,		84
};

const static float yCoords[NUM_SCORES][2] =
{
	1120,		1070,
	792, 		778,
	661,		649,
	535,		521,
	407,		392,
	278,		264,
	149,		135,
	25,			40,
	-102,		-113,
	-229,		-242
};

			/* INIT CAMERA */
			
	QD3D_UpdateCameraFromTo(gGameViewInfoPtr, &from, &to);
	
	
			/*******************/
			/* CREATE THE VINE */
			/*******************/
	
	gNewObjectDefinition.type 		= SCORES_ObjType_Vine;
	gNewObjectDefinition.group 		= MODEL_GROUP_HIGHSCORES;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 10;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	MakeObjectKeepBackfaces(newObj);

			/*************************/
			/* CREATE THE BACKGROUND */
			/*************************/
	
	gNewObjectDefinition.type 		= SCORES_ObjType_Background;
	gNewObjectDefinition.group 		= MODEL_GROUP_HIGHSCORES;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= from.y - 500;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER|STATUS_BIT_NOFOG;
	gNewObjectDefinition.slot 		= 10;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 6.0;
	gScoreCyc = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	
			/**********************/
			/* CREATE SCORES LIST */
			/**********************/
	
	for (slot = 0; slot < NUM_SCORES; slot++)
	{
		float	z;
		
		if (slot == 0)
			z = 30;
		else
			z = 0;
			
		
				/* CREATE NAME */

		gNewName = gHighScores[slot];
		UpdateNameAndCursor(false,xCoords[slot][1],yCoords[slot][1],z);
		
				/* CREATE SCORE */
			
		score = gNewName.score;
		place = 0;
				
		x = xCoords[slot][0];
		y = yCoords[slot][0] + 15;
		while((score > 0) || (place < 1))
		{
			digit = score % 10;								// get digit @ end of #
			score /= 10;									// shift down
			
			gNewObjectDefinition.type 		= SCORES_ObjType_0 + digit;
			gNewObjectDefinition.group 		= MODEL_GROUP_HIGHSCORES;
			gNewObjectDefinition.coord.x 	= x - (place*(LETTER_SEPARATION));
			gNewObjectDefinition.coord.y 	= y;
			gNewObjectDefinition.coord.z 	= z;
			gNewObjectDefinition.flags 		= 0;
			gNewObjectDefinition.slot 		= 10;
			gNewObjectDefinition.moveCall 	= nil;
			gNewObjectDefinition.rot 		= 0;
			gNewObjectDefinition.scale 		= 1.0;
			newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
			
			place++;
		}
	}
}






