/****************************/
/*  		INFOBAR.C		*/
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Byte			gPlayerMode;
extern	float			gFramesPerSecond,gFramesPerSecondFrac;
extern	WindowPtr		gCoverWindow;
extern	Boolean			gGameOverFlag,gAbortedFlag,gRestoringSavedGame;
extern	ObjNode			*gPlayerObj,*gHiveObj,*gTheQueen,*gAntKingObj;
extern	u_short			gRealLevel;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	TQ3Matrix4x4		gCameraWorldToFrustumMatrix,gCameraFrustumToWindowMatrix,gCameraWorldToWindowMatrix,gCameraWindowToWorldMatrix;
extern	TQ3Point3D		gMyCoord;
extern	long			gNumSuperTilesDeep,gNumSuperTilesWide;	
extern	FSSpec			gDataSpec;
extern	short		gMainAppRezFile,gTextureRezfile;
extern	TerrainItemEntryType 	**gMasterItemList;
extern	short	  				gNumTerrainItems;
extern	PrefsType	gGamePrefs;
extern	Boolean		gShowBottomBar;


/****************************/
/*    PROTOTYPES            */
/****************************/

static short GetSpriteWidth(long spriteNum);
static void DrawSprite(long spriteNum, long x, long y);
static void EraseSprite(long spriteNum, long x, long y);
static void ShowHealth(void);
static void LoadSpriteResources(FSSpec *spec);
static void ShowBallTimerAndInventory(Boolean ballTimerOnly);
static void ShowLadyBugs(void);
static void ShowLives(void);
static void ShowBlueClover(void);
static void ShowGoldClover(void);
static void ShowBossHealth(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	BOSS_WIDTH			200

#define	LIVES_X				11
#define	LIVES_Y				0
#define	LIVES_WIDTH			42

#define	TIMER_X				320
#define	TIMER_Y				6
#if 0	// Source port removal: nitro gauge (timer) dimensions now read in from template image
#define	TIMER_WIDTH			90
#define	TIMER_HEIGHT		83
#endif

#define	HEALTH_X			TIMER_X
#define	HEALTH_Y			50
#define	HEALTH_WIDTH		92
#define	HEALTH_HEIGHT		7


#define	HAND_X				TIMER_X
#define	HAND_Y				1

#define	BALL_X				(TIMER_X-13)
#define	BALL_Y				17

#define	LADYBUG_X			396
#define	LADYBUG_Y			0

#define	GOLD_CLOVER_X		141
#define	GOLD_CLOVER_Y		0

#define	BLUE_CLOVER_X		196
#define	BLUE_CLOVER_Y		0


#define	INFOBAR_Z		-15.0f


		/* INFOBAR OBJTYPES */
enum
{
	INFOBAR_ObjType_Quit,
	INFOBAR_ObjType_Resume
};


		/* SPRITES */
				
#define	MAX_SPRITES		50			// must be at least as big as needed

enum
{
		/* MISC */
		
	SPRITE_LIFE1,
	SPRITE_LIFE2,
	SPRITE_LIFE3,
	
	SPRITE_GOLDCLOVER1,
	SPRITE_GOLDCLOVER2,
	SPRITE_GOLDCLOVER3,
	SPRITE_GOLDCLOVER4,

	SPRITE_BLUECLOVER1,
	SPRITE_BLUECLOVER2,
	SPRITE_BLUECLOVER3,
	SPRITE_BLUECLOVER4,
	
	SPRITE_LADYBUG,
	SPRITE_LADYBUG_ALL,
	SPRITE_LADYBUG_SMALL,

	SPRITE_EMPTYHAND_L,
	SPRITE_EMPTYHAND_R,
	SPRITE_MONEY_L,
	SPRITE_MONEY_R,
	SPRITE_GREENKEY_L,
	SPRITE_GREENKEY_R,
	SPRITE_BLUEKEY_L,
	SPRITE_BLUEKEY_R,
	SPRITE_REDKEY_L,
	SPRITE_REDKEY_R,
	SPRITE_ORANGEKEY_L,
	SPRITE_ORANGEKEY_R,
	SPRITE_PURPLEKEY_L,
	SPRITE_PURPLEKEY_R,
	
	SPRITE_BALL
	
			// check MAX_SPRITES above
};


#define	MAX_KEY_TYPES	5


/*********************/
/*    VARIABLES      */
/*********************/

u_long 			gInfobarUpdateBits = 0;
short			gNumLives;
float			gMyHealth;
float			gBallTimer;
int				gOldTimerN;

u_long			gScore;
static Boolean	gGotKey[MAX_KEY_TYPES];
short			gMoney;
short			gNumLadyBugsThisArea;
short			gNumLadyBugsOnThisLevel;

short			gNumGreenClovers,gNumGoldClovers,gNumBlueClovers;

static Boolean		gInfobarArtLoaded = false;

static long			gNumSprites = 0;
static GWorldPtr	gSprites[MAX_SPRITES];

GWorldPtr	gInfoBarTop = nil;

static Byte		gLeftArmType, gRightArmType;
static Byte		gOldLeftArmType, gOldRightArmType;

static Boolean	gBallIconIsDisplayed;

static const int		gNitroGaugeMaxSkipsPerRow = 8;
static Rect		gNitroGaugeRect;
static Handle	gNitroGaugeData		= nil;
static Handle 	gNitroGaugeAlphaSkips	= nil;

/**************** INIT INVENTORY FOR GAME *********************/

void InitInventoryForGame(void)
{
	if (!gRestoringSavedGame)
	{
		gScore			= 0;
		gMyHealth		= 1.0;
		gNumLives 		= 3;
		gBallTimer		= 1;				// full ball capability
		gNumGoldClovers = 0;
	}
}


/**************** INIT INVENTORY FOR AREA *********************/

void InitInventoryForArea(void)
{
long					i;
TerrainItemEntryType	*itemPtr;

	for (i = 0; i < MAX_KEY_TYPES; i++)					// we dont have any keys
		gGotKey[i] = false;

	gNumLadyBugsThisArea = 0;
	gMoney 		= 0;
	gOldTimerN = -1;
	gNumGreenClovers = gNumBlueClovers = 0;
	
	
			/* COUNT # LADYBUGS ON THIS LEVEL */
			
	gNumLadyBugsOnThisLevel = 0;
	itemPtr = *gMasterItemList; 						// get pointer to data inside the LOCKED handle
	for (i= 0; i < gNumTerrainItems; i++)
		if (itemPtr[i].type == MAP_ITEM_LADYBUG)
		{
			gNumLadyBugsOnThisLevel++;
		}
	
}


/*************** INIT INFOBAR **********************/
//
// Doesnt init inventories, just the physical infobar itself.
//

void InitInfobar(void)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
FSSpec		spec;
Rect		r;

	SetPort(GetWindowPort(gCoverWindow));

			/* SEE IF DISPOSE OLD TOP */
			
	if (gInfoBarTop)
	{
		DisposeGWorld(gInfoBarTop);
		gInfoBarTop = nil;
	}


			/****************/
			/* DRAW INFOBAR */
			/****************/

			/* DO TOP */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Images:InfobarTop.pict", &spec);
	DrawPictureIntoGWorld(&spec, &gInfoBarTop);
	SetRect(&r, 0,0, 640,62);
	DumpGWorld2(gInfoBarTop, gCoverWindow,&r);
	
	
			/* DO BOTTOM */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Images:InfobarBottom.pict", &spec);
	DrawPictureToScreen(&spec, 0, 480-60);


			/* PRIME SCREEN */

	gOldLeftArmType = gLeftArmType = SPRITE_EMPTYHAND_L;
	gOldRightArmType = gRightArmType = SPRITE_EMPTYHAND_R;
	gBallIconIsDisplayed = false;

	gInfobarUpdateBits = 0xffffffff;
	UpdateInfobar();
#endif
}


/******************* UPDATE INFOBAR *********************/

void UpdateInfobar(void)
{
unsigned long	bits;

	bits = gInfobarUpdateBits;

		/* UPDATE HEALTH */
		
	if (bits & UPDATE_HEALTH)
		ShowHealth();

		/* UPDATE LIVES */
		
	if (bits & UPDATE_LIVES)
		ShowLives();


		/* UPDATE BALL TIMER */
		
	if (bits & UPDATE_TIMER)
		ShowBallTimerAndInventory(true);
	
		
		/* UPDATE LADY BUGS */
		
	if (bits & UPDATE_LADYBUGS)
		ShowLadyBugs();
	
	
		/* UPDATE HANDS */
			
	if (bits & UPDATE_HANDS)
		ShowBallTimerAndInventory(false);
	
	
		/* CLOVERS */
		
	if (bits & UPDATE_BLUECLOVER)
		ShowBlueClover();
	if (bits & UPDATE_GOLDCLOVER)
		ShowGoldClover();
	
		/* BOSS HEALTH */
		
	if (bits & UPDATE_BOSS)
		ShowBossHealth();	
	
	gInfobarUpdateBits = 0;
}



#pragma mark -


/****************** LOAD INFOBAR ART ********************/

static void LoadNitroGaugeTemplate(void)
{
	GAME_ASSERT_MESSAGE(gNitroGaugeData == nil, "nitro gauge template already loaded");

	FSSpec spec;
	TGAHeader tga;
	OSErr err;
	
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Images:NitroGauge.tga", &spec);
	err = ReadTGA(&spec, &gNitroGaugeData, &tga);
	GAME_ASSERT(err == noErr);
	GAME_ASSERT(tga.imageType == TGA_IMAGETYPE_RAW_GRAYSCALE);
	
	gNitroGaugeRect.left		= TIMER_X - tga.width/2;
	gNitroGaugeRect.right		= gNitroGaugeRect.left + tga.width;
	gNitroGaugeRect.top			= TIMER_Y;
	gNitroGaugeRect.bottom		= gNitroGaugeRect.top + tga.height;

	// Prepare alpha skip table. The nitro gauge is mostly made up of transparent pixels,
	// so this will speed up rendering a little.

	gNitroGaugeAlphaSkips = NewHandleClear(tga.height * gNitroGaugeMaxSkipsPerRow * sizeof(int));
	uint8_t*	grayscale	= (uint8_t*) *gNitroGaugeData;
	int*		skipTable	= (int *) *gNitroGaugeAlphaSkips;
	for (int y = 0; y < tga.height; y++)
	{
		int skipsThisRow = 0;
		int* currentSkip = nil;
		for (int x = 0; x < tga.width; x++)
		{
			if ((*grayscale++) != 0)
			{
				currentSkip = nil;
			}
			else if (!currentSkip)
			{
				GAME_ASSERT(skipsThisRow < gNitroGaugeMaxSkipsPerRow);
				currentSkip = &skipTable[skipsThisRow++];
				*currentSkip = 1;
			}
			else
			{
				(*currentSkip)++;
			}
		}
		skipTable += gNitroGaugeMaxSkipsPerRow;
	}
}

void LoadInfobarArt(void)
{				
FSSpec	spec;

	if (gInfobarArtLoaded)
		return;


		/*****************************************************/
		/* READ THE PICTS FROM REZ FILE & CONVERT TO GWORLDS */
		/*****************************************************/

	gNumSprites = 0;
		
		
			/* READ MISC SPRITES */
						
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Images:Infobar", &spec);
	LoadSpriteResources(&spec);
	
	
	UseResFile(gMainAppRezFile);
	UseResFile(gTextureRezfile);
	
	
			/* READ NITRO GAUGE TEMPLATE (SOURCE PORT ADDITION) */

	LoadNitroGaugeTemplate();



	gInfobarArtLoaded = true;
}


void FreeInfobarArt(void)
{
	if (gNitroGaugeData != nil)
	{
		DisposeHandle(gNitroGaugeData);
		gNitroGaugeData = nil;
	}
	
	if (gNitroGaugeAlphaSkips != nil)
	{
		DisposeHandle(gNitroGaugeAlphaSkips);
		gNitroGaugeAlphaSkips = nil;
	}
	
	for (int i = 0; i < gNumSprites; i++)
	{
		DisposeGWorld(gSprites[i]);
		gSprites[i] = nil;
	}
	gNumSprites = 0;
}


/******************* LOAD SPRITE RESOURCES **********************/

static void LoadSpriteResources(FSSpec *spec)
{
int			i,n;
int			fRefNum;
OSErr		iErr;
GDHandle	oldGD;
GWorldPtr	oldGW;

	GetGWorld(&oldGW, &oldGD);										// save current port

	fRefNum = FSpOpenResFile(spec,fsRdPerm);
	GAME_ASSERT(fRefNum != -1);
	UseResFile(fRefNum);
			
			/* READ 'EM IN */
				
	n = Count1Resources('PICT');						// get # PICTs in here
	GAME_ASSERT(gNumSprites+n <= MAX_SPRITES);

	for (i = 0; i < n; i++)
	{
		PicHandle	pict;
		Rect		r;

		pict = GetPicture(128+i);										// load pict
		HLock((Handle)pict);
		r = (*pict)->picFrame;											// get size of pict
		
				/* MAKE GWORLD */
				
		iErr = NewGWorld(&gSprites[gNumSprites], 16, &r, nil, nil, 0);	// try app mem
		GAME_ASSERT(noErr == iErr);

		SetGWorld(gSprites[gNumSprites], nil);
		DoLockPixels(gSprites[gNumSprites]);
		DrawPicture(pict, &r);											// draw pict into gworld

		ReleaseResource((Handle)pict);									// nuke pict
		
		gNumSprites++;
	}
			/* CLOSE REZ FILE */
			
	CloseResFile(fRefNum);

	SetGWorld (oldGW, oldGD);										// restore port
}


/********************** DRAW SPRITE ****************************/

static void DrawSprite(long spriteNum, long x, long y)
{
Rect	r, pr;

	GAME_ASSERT_MESSAGE(spriteNum < gNumSprites, "illegal sprite #");

	GetPortBounds(gSprites[spriteNum], &pr);

	r.left 		= x;
	r.right 	= r.left + (pr.right - pr.left);
	r.top 		= y;
	r.bottom 	= r.top + (pr.bottom - pr.top);

	DumpGWorldTransparent(gSprites[spriteNum], gCoverWindow, &r);
}


/********************** ERASE SPRITE ****************************/

static void EraseSprite(long spriteNum, long x, long y)
{
Rect	r, pr;

	if (gInfoBarTop == nil)
		return;

	GAME_ASSERT_MESSAGE(spriteNum < gNumSprites, "illegal sprite #");

	GetPortBounds(gSprites[spriteNum], &pr);


	r.left 		= x;
	r.right 	= r.left + (pr.right - pr.left);
	r.top 		= y;
	r.bottom 	= r.top + (pr.bottom - pr.top);
	DumpGWorld3(gInfoBarTop, gCoverWindow, &r, &r);
}


/****************** GET SPRITE WIDTH **********************/

static short GetSpriteWidth(long spriteNum)
{
Rect	r;

	GAME_ASSERT_MESSAGE(spriteNum < gNumSprites, "illegal sprite #");

	GetPortBounds(gSprites[spriteNum], &r);

	return (r.right - r.left);
}


#pragma mark -

/****************** DRAW NITRO GAUGE ************************/
// Source port rewrite.
// Original code used FrameArc(). We use a reference image instead.
// arcSpan is in degrees. 0: gauge empty; 180: gauge full.

static void DrawNitroGauge(int arcSpan)
{
	Boolean wantMargin = (arcSpan < 178) && (arcSpan > 2);

	uint8_t* templateRow = *((uint8_t**)gNitroGaugeData);
	int nitroGaugeWidth = gNitroGaugeRect.right - gNitroGaugeRect.left;
	int nitroGaugeHeight = gNitroGaugeRect.bottom - gNitroGaugeRect.top;
	
	PixMapHandle pm = GetGWorldPixMap(gCoverWindow);
	int pmWidth		= (**pm).bounds.right - (**pm).bounds.left;

	uint32_t* outRow = (uint32_t*) GetPixBaseAddr(pm);
	outRow += gNitroGaugeRect.left;
	outRow += gNitroGaugeRect.top * pmWidth;

	int* skipTable = (int *) *gNitroGaugeAlphaSkips;

	for (int y = 0; y < nitroGaugeHeight; y++)
	{
		int* skipData = &skipTable[y * gNitroGaugeMaxSkipsPerRow];
		int x = 0;
		while (x < nitroGaugeWidth)
		{
			uint8_t t = templateRow[x];
			if (t == 0)
			{
				x += (*skipData++);
				continue;
			}

			t--;	// move value from [1;181] to [0;180] (zero is reserved for mask)
			
			if (t <= arcSpan)						// green (original: 0x0000,0xBDEF,0x294A)
			{
				outRow[x] = 0x00bd29FF; 			// (BGRA)
			}
			else if (wantMargin && t <= arcSpan+3)	// margin line (original: 0xffff,0xF7BD,0x0000)
			{
				outRow[x] = 0x00f7ffFF;				// (BGRA)
			}
			else									// black
			{
				outRow[x] = 0x000000FF;
			}

			x++;
		}

		templateRow += nitroGaugeWidth;
		outRow += pmWidth;
	}

	DamagePortRegion(&gNitroGaugeRect);
}


/****************** SHOW BALL TIMER AND INVENTORY ************************/
//
// Shows the ball timer composited with the inventory hands
//
// INPUT:	ballTimerOnly = true if only need to update ball timer
//

static void ShowBallTimerAndInventory(Boolean ballTimerOnly)
{
int		n;
Boolean	eraseBugStuff;


	SetPort(GetWindowPort(gCoverWindow));


		/* IF ONLY NEED TO UPDATE TIMER, THEN SEE IF NECESSARY */
		
	if (gBallTimer < 0.0f)
		gBallTimer = 0.0;
	n = 180.0f * gBallTimer;
	
	if (ballTimerOnly)
	{
		if (abs(n - gOldTimerN) < 2)			// if hasnt changed enough, then dont bother
			return;
	}
	gOldTimerN = n;


			/* SEE IF ERASE BUG RELATED ICONS */
			
	if ((gPlayerMode == PLAYER_MODE_BALL) && (!gBallIconIsDisplayed))
		eraseBugStuff = true;
	else
		eraseBugStuff = false;
		

			/***********************************/
			/* SEE IF NEED TO ERASE ARMS FIRST */
			/***********************************/

	if ((gOldLeftArmType != gLeftArmType) || eraseBugStuff)				// erase if changed
		EraseSprite(gOldLeftArmType, HAND_X - GetSpriteWidth(gOldLeftArmType), HAND_Y);

	if ((gOldRightArmType != gRightArmType) || eraseBugStuff)
		EraseSprite(gOldRightArmType, HAND_X, HAND_Y);


			/* SEE IF ERASE BALL */
			
	if ((gPlayerMode == PLAYER_MODE_BUG) && (gBallIconIsDisplayed))
	{
		EraseSprite(SPRITE_BALL, BALL_X, BALL_Y);
		gBallIconIsDisplayed = false;
	}

				/**************/
				/* DRAW METER */
				/**************/

	DrawNitroGauge(n);


		/***********************/
		/* DRAW HAND INVENTORY */
		/***********************/

	if (gPlayerMode == PLAYER_MODE_BUG)
	{
		DrawSprite(gLeftArmType, HAND_X - GetSpriteWidth(gLeftArmType), HAND_Y);
		DrawSprite(gRightArmType, HAND_X, HAND_Y);
		gBallIconIsDisplayed = false;
	}
	else
	if ((!gBallIconIsDisplayed) || eraseBugStuff)				// draw ball icon instead
	{
		DrawSprite(SPRITE_BALL, BALL_X, BALL_Y);
		gBallIconIsDisplayed = true;
	}

	gOldLeftArmType = gLeftArmType;
	gOldRightArmType = gRightArmType;
}
	


/***************** PROCESS BALL TIMER *********************/
//
// Called by UpdatePlayer_Ball
//

void ProcessBallTimer(void)
{
float	amount;

	if (gGamePrefs.easyMode)						// lose less time in easy mode
		amount = gFramesPerSecondFrac * .03f;
	else
		amount = gFramesPerSecondFrac * .04f;
		
	LoseBallTime(amount);		
}


/******************** LOSE BALL TIME **************************/

void LoseBallTime(float amount)
{
	gBallTimer -= amount;
	if (gBallTimer <= 0.0f)
	{
		gBallTimer = 0;
		
		if (gPlayerMode == PLAYER_MODE_BALL)
			InitPlayer_Bug(gPlayerObj, &gMyCoord, gPlayerObj->Rot.y, PLAYER_ANIM_UNROLL);
	}
	gInfobarUpdateBits |= UPDATE_TIMER;	
}



#pragma mark -

/*************************** GET KEY ******************************/
//
// Called when player picks up a key.  This adds the key to the inventory.
//

void GetKey(long keyID)
{
	GAME_ASSERT_MESSAGE(keyID < MAX_KEY_TYPES, "illegal key ID#");	// make sure it's legal

	gGotKey[keyID] = true;
	
	
		/* SEE WHICH HAND TO PUT IT IN */
		
	keyID = SPRITE_GREENKEY_L + (keyID*2);
		
	if (gLeftArmType == SPRITE_EMPTYHAND_L)
		gLeftArmType = keyID;
	else
	if (gRightArmType == SPRITE_EMPTYHAND_R)
		gRightArmType = keyID+1;
	
	if (gPlayerMode == PLAYER_MODE_BUG)			// only update infobar if I'm the bug
		gInfobarUpdateBits |= UPDATE_HANDS;	
	
}


/*********************** USE KEY ****************************/
//
// Called when player uses a key to open a door.  This removes it from the inventory.
//

void UseKey(long keyID)
{
	gGotKey[keyID] = false;
	
		/* REMOVE MONEY FROM HAND */
			
	keyID = SPRITE_GREENKEY_L + (keyID*2);
			
	if (gLeftArmType == keyID)
		gLeftArmType = SPRITE_EMPTYHAND_L;
	else
	if (gRightArmType == (keyID+1))
		gRightArmType = SPRITE_EMPTYHAND_R;
	
	if (gPlayerMode == PLAYER_MODE_BUG)			// only update infobar if I'm the bug
		gInfobarUpdateBits |= UPDATE_HANDS;
}


/******************* DO WE HAVE THE KEY ************************/
//
// This is called when a door is touched to see if we have the needed
// key for this door.
//

Boolean DoWeHaveTheKey(long keyID)
{
	if (gGotKey[keyID])
		return(true);

	return(false);
}


#pragma mark -

/*************************** GET MONEY ******************************/
//
// Called when player picks up money.  This adds the money to the inventory.
//

void GetMoney(void)
{
	gMoney++;

		/* SEE WHICH HAND TO PUT IT IN */
		
	if (gLeftArmType == SPRITE_EMPTYHAND_L)
		gLeftArmType = SPRITE_MONEY_L;
	else
	if (gRightArmType == SPRITE_EMPTYHAND_R)
		gRightArmType = SPRITE_MONEY_R;
	
	if (gPlayerMode == PLAYER_MODE_BUG)			// only update infobar if I'm the bug
		gInfobarUpdateBits |= UPDATE_HANDS;	
}


/*********************** USE MONEY ****************************/
//
// Called when player uses money to ride a taxi.  This removes it from the inventory.
//

void UseMoney(void)
{
	gMoney--;
	
		/* REMOVE MONEY FROM HAND */
			
	if (gLeftArmType == SPRITE_MONEY_L)
		gLeftArmType = SPRITE_EMPTYHAND_L;
	else
	if (gRightArmType == SPRITE_MONEY_R)
		gRightArmType = SPRITE_EMPTYHAND_R;
	
	gInfobarUpdateBits |= UPDATE_HANDS;	
}



/******************* DO WE HAVE ENOUGH MONEY ************************/
//
// This is called when a taxi is touched to see if we have the needed
// money to ride.
//

Boolean DoWeHaveEnoughMoney(void)
{
	if (gMoney > 0)
	{
		return(true);
	}

	return(false);
}


#pragma mark -

/********************* GET LADY BUG ***********************/

void GetLadyBug(void)
{
	gNumLadyBugsThisArea++;
	gInfobarUpdateBits |= UPDATE_LADYBUGS;
}


/**************** SHOW LADY BUGS *********************/

static void ShowLadyBugs(void)
{
int	i,x;

			/* SHOW THE BIG ICON */
			
	if (gNumLadyBugsThisArea >= gNumLadyBugsOnThisLevel)
		DrawSprite(SPRITE_LADYBUG_ALL, LADYBUG_X, LADYBUG_Y);
	else
		DrawSprite(SPRITE_LADYBUG, LADYBUG_X, LADYBUG_Y);
	

	x = LADYBUG_X + 80;
	for (i = 0; i < gNumLadyBugsThisArea; i++)
	{
		if (i&1)
		{
			DrawSprite(SPRITE_LADYBUG_SMALL, x, LADYBUG_Y+30);
			x += 22;
		}
		else
			DrawSprite(SPRITE_LADYBUG_SMALL, x, LADYBUG_Y+7);
	}
}

#pragma mark -


/*************************** GET HEALTH ******************************/

void GetHealth(float amount)
{
	gMyHealth += amount;
	if (gMyHealth > 1.0f)
		gMyHealth = 1.0;
	gInfobarUpdateBits |= UPDATE_HEALTH;	
}


/******************** LOSE HEALTH *******************/

void LoseHealth(float amount)
{
	if (gGamePrefs.easyMode)								// lose less health in easy mode
		amount *= .5f;

	gMyHealth -= amount;
	if (gMyHealth <= 0.0f)	
	{
		gMyHealth = 0;
		KillPlayer(true);
	}
	gInfobarUpdateBits |= UPDATE_HEALTH;
}


/****************** SHOW HEALTH ************************/

static void ShowHealth(void)
{
int		n;
Rect	r;
RGBColor mc = {63421,0x0000,6342};
RGBColor ec = {0x0000,0x0000,0x0000};
RGBColor fc = {0xffff,63421,0x0000};
	
	
	n = (float)HEALTH_WIDTH * gMyHealth;
	
			/* RED FILLER */
			
	r.left = HEALTH_X - HEALTH_WIDTH/2;
	r.right = r.left + n;
	r.top = HEALTH_Y;
	r.bottom = r.top + HEALTH_HEIGHT;
	if (r.right > r.left)
	{
		RGBForeColor(&mc);
		PaintRect(&r);
	}

			/* EMPTY FILLER */
			
	r.left = r.right;
	r.right = (HEALTH_X - HEALTH_WIDTH/2) + HEALTH_WIDTH;
	
	if (r.right > r.left)
	{
		RGBForeColor(&ec);
		PaintRect(&r);
	}

			/* MARGIN LINE */
	
	n = r.right;		
	r.right = r.left + 2;
	
	if (r.right < (n-2))
	{
		RGBForeColor(&fc);
		PaintRect(&r);
	}
	
}


/******************** SHOW LIVES **********************/

static void ShowLives(void)
{
int	i,x;

	x = LIVES_X;
	for (i = 0; i < 3; i++)
	{
		if (i < (gNumLives-1))
			DrawSprite(SPRITE_LIFE1+i, x, LIVES_Y);
		else
			EraseSprite(SPRITE_LIFE1+i, x, LIVES_Y);

		x += LIVES_WIDTH;
	}	
}


#pragma mark -


/******************* GET GREEN CLOVER *************************/
//
// Greens dont appear in infobar
//

void GetGreenClover(void)
{
	gNumGreenClovers++;
}


/******************* GET BLUE CLOVER *************************/

void GetBlueClover(void)
{
	gNumBlueClovers++;
	gInfobarUpdateBits |= UPDATE_BLUECLOVER;
}

/******************* GET GOLD CLOVER *************************/

void GetGoldClover(void)
{
	gNumGoldClovers++;
	gInfobarUpdateBits |= UPDATE_GOLDCLOVER;
}


/***************** SHOW BLUE CLOVER *****************/

static void ShowBlueClover(void)
{
int	n;

	if (gNumBlueClovers)
	{
		if (gNumBlueClovers > 4)
			n = 3;
		else
			n = gNumBlueClovers-1;
			
		DrawSprite(SPRITE_BLUECLOVER1 + n, BLUE_CLOVER_X, BLUE_CLOVER_Y);
	}
	else
		EraseSprite(SPRITE_BLUECLOVER1, BLUE_CLOVER_X, BLUE_CLOVER_Y);
}


/***************** SHOW GOLD CLOVER *****************/

static void ShowGoldClover(void)
{
int	n;

	if (gNumGoldClovers)
	{
		if (gNumGoldClovers > 4)
			n = 3;
		else
			n = gNumGoldClovers-1;
			
		DrawSprite(SPRITE_GOLDCLOVER1 + n, GOLD_CLOVER_X, GOLD_CLOVER_Y);
	}
	else
		EraseSprite(SPRITE_GOLDCLOVER1, GOLD_CLOVER_X, GOLD_CLOVER_Y);
}


#pragma mark -


/******************** SHOW BOSS HEALTH **************************/

static void ShowBossHealth(void)
{
float	health;
Rect	r;
int		w,x;

		/* DETERMINE HEALTH */
		
	switch(gRealLevel)
	{
		case	LEVEL_NUM_FLIGHT:
				if (gHiveObj == nil)
					health = 1.0;
				else
					health = gHiveObj->Health;
				break;
		
		case	LEVEL_NUM_QUEENBEE:
				if (gTheQueen == nil)
					health = 1.0;
				else
					health = gTheQueen->Health / QUEENBEE_HEALTH;
				break;
		
		case	LEVEL_NUM_ANTKING:
				if (gAntKingObj == nil)
					health = 1.0;
				else
					health = gAntKingObj->Health / ANTKING_HEALTH;
				break;
				
		default:
				return;
	}

	if (health < 0)				// Source port fix: hive health can go negative if you keep shooting at it
		health = 0;
	
		/***********/	
		/* DRAW IT */
		/***********/
			
	r.top = 440;
	r.bottom = r.top + 20;
	r.left = 320-(BOSS_WIDTH/2);
	x = r.right = r.left + BOSS_WIDTH;
			
		/* FRAME */

	SetPort(GetWindowPort(gCoverWindow));
	ForeColor(blackColor);
	{
		const int thickness = 2;
		Rect frameRect = r;
		frameRect.left		-= thickness;
		frameRect.top		-= thickness;
		frameRect.right		+= thickness;
		frameRect.bottom	+= thickness;
		for (int i = 0; i <= thickness; i++)
		{
			++frameRect.left;
			++frameRect.top;
			--frameRect.right;
			--frameRect.bottom;
			FrameRect(&frameRect);
		}
	}

		/* METER */
		
	r.top += 2;
	r.bottom -= 2;
	r.left += 2;
	w = health * (float)BOSS_WIDTH;
	r.right = r.left + w - 4; 	
	if (w > 0)
	{
		ForeColor(redColor);
		PaintRect(&r);
	}
		
		/* EMPTY */
	
	if (w < BOSS_WIDTH)
	{
		r.left = r.right;
		r.right = x-2;
		ForeColor(blackColor);
		PaintRect(&r);
	}
}



void SubmitInfobarOverlay(void)
{
	Overlay_SubmitQuad(0,	0,		640,	62,		0,	0,				1.0f,	62.0f/480.0f);
	if (gShowBottomBar)
	{
		Overlay_SubmitQuad(0,	420,	640,	60,		0,	420.0f/480.0f,	1.0f,	60.0f/480.0f);
	}
}