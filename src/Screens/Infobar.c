/****************************/
/*  		INFOBAR.C		*/
/* By Brian Greenstone      */
/* (c)1999 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include <stdio.h>


/****************************/
/*    PROTOTYPES            */
/****************************/

static int GetSpriteWidth(int spriteNum);
static void DrawSprite(int spriteNum, int x, int y);
static void EraseSprite(int spriteNum, int x, int y);
static void ShowHealth(void);
static void LoadSpriteResources(void);
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

#define	INFOBAR_TEXTURE_WIDTH	1024
#define	INFOBAR_TEXTURE_HEIGHT	128
#define	BOTTOM_BAR_Y_IN_TEXTURE	64


		/* INFOBAR OBJTYPES */
enum
{
	INFOBAR_ObjType_Quit,
	INFOBAR_ObjType_Resume
};


		/* SPRITES */

enum
{
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

	SPRITE_BALL,

	SPRITE_INFOBARTOP,
	SPRITE_INFOBARBOTTOM,

	MAX_SPRITES
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

static uint32_t*	gSprites[MAX_SPRITES];
static uint32_t*	gSpriteMasks[MAX_SPRITES];
static int			gSpriteWidths[MAX_SPRITES];
static int			gSpriteHeights[MAX_SPRITES];

static TQ3TriMeshData*	gInfobarTopMesh = nil;
static TQ3TriMeshData*	gInfobarBottomMesh = nil;

static uint32_t*	gInfobarTexture = nil;
static GLuint		gInfobarTextureName = 0;
static Rect			gInfobarTextureDirtyRect;
static Boolean		gInfobarTextureIsDirty = true;

static Byte		gLeftArmType, gRightArmType;
static Byte		gOldLeftArmType, gOldRightArmType;

static Boolean	gBallIconIsDisplayed;
static Boolean	gBossHealthWasUpdated;

static const int	gNitroGaugeMaxSkipsPerRow = 8;
static Rect			gNitroGaugeRect;
static uint8_t*		gNitroGaugeData		= nil;
static int*			gNitroGaugeAlphaSkips	= nil;

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


	gBossHealthWasUpdated = false;
}



void DisposeInfobarTexture(void)
{
	if (gInfobarTexture)
	{
		DisposePtr((Ptr) gInfobarTexture);
		gInfobarTexture = nil;
	}

	if (gInfobarTextureName)
	{
		glDeleteTextures(1, &gInfobarTextureName);
		gInfobarTextureName = 0;
	}

	if (gInfobarTopMesh)
	{
		Q3TriMeshData_Dispose(gInfobarTopMesh);
		gInfobarTopMesh = nil;
	}

	if (gInfobarBottomMesh)
	{
		Q3TriMeshData_Dispose(gInfobarBottomMesh);
		gInfobarBottomMesh = nil;
	}

	gInfobarTextureIsDirty = true;
}

/*************** INIT INFOBAR **********************/
//
// Doesnt init inventories, just the physical infobar itself.
//

void InitInfobar(void)
{
	GAME_ASSERT(gInfobarArtLoaded);

			/* SEE IF DISPOSE OLD TOP / BOTTOM */

	DisposeInfobarTexture();

			/* CREATE TEXTURE BUFFER */

	gInfobarTexture = (uint32_t*) NewPtrClear(sizeof(uint32_t) * 1024 * 128);

			/* DO TOP */

	DrawSprite(SPRITE_INFOBARTOP, 0, 0);

			/* DO BOTTOM */

	DrawSprite(SPRITE_INFOBARBOTTOM, 0, BOTTOM_BAR_Y_IN_TEXTURE);

			/* CREATE TEXTURE */

	gInfobarTextureName = Render_LoadTexture(
			GL_RGB,
			1024,
			128,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			gInfobarTexture,
			kRendererTextureFlags_ClampBoth
	);
	CHECK_GL_ERROR();

			/* CREATE TOP MESH */

	float uMult = 1.0f / INFOBAR_TEXTURE_WIDTH;
	float vMult = 1.0f / INFOBAR_TEXTURE_HEIGHT;

	GAME_ASSERT_MESSAGE(!gInfobarTopMesh, "infobar top mesh already created");

	gInfobarTopMesh = MakeQuadMesh_UI(
			0, 0, 640, 62,
			0,
			0,
			uMult * 640,
			vMult * 62
	);
	gInfobarTopMesh->texturingMode = kQ3TexturingModeOpaque;
	gInfobarTopMesh->glTextureName = gInfobarTextureName;

			/* CREATE BOTTOM MESH */

	GAME_ASSERT_MESSAGE(!gInfobarBottomMesh, "infobar bottom mesh already created");

	if (!gGamePrefs.showBottomBar)
	{
		// Make a mesh that only shows the boss's health
		gInfobarBottomMesh = MakeQuadMesh_UI(
				(640-BOSS_WIDTH)/2, 420+20, (640+BOSS_WIDTH)/2, 420+40,
				uMult * ((640-BOSS_WIDTH)/2 + 0.5f),
				vMult * (BOTTOM_BAR_Y_IN_TEXTURE + 20 + 0.5f),
				uMult * ((640+BOSS_WIDTH)/2 - 0.5f),
				vMult * (BOTTOM_BAR_Y_IN_TEXTURE + 40 - 0.5f));
	}
	else
	{
		gInfobarBottomMesh = MakeQuadMesh_UI(
				0, 420, 640, 480,
				uMult * 0,
				vMult * BOTTOM_BAR_Y_IN_TEXTURE,
				uMult * 640,
				vMult * (BOTTOM_BAR_Y_IN_TEXTURE + 60));
	}
	gInfobarBottomMesh->texturingMode = kQ3TexturingModeOpaque;
	gInfobarBottomMesh->glTextureName = gInfobarTextureName;


			/* PRIME SCREEN */

	gOldLeftArmType = gLeftArmType = SPRITE_EMPTYHAND_L;
	gOldRightArmType = gRightArmType = SPRITE_EMPTYHAND_R;
	gBallIconIsDisplayed = false;

	gInfobarUpdateBits = 0xffffffff;
	UpdateInfobar();
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

/******************* LOAD SPRITE RESOURCES **********************/

static void LoadSpriteResources(void)
{
FSSpec		spec;
OSErr		err;
uint8_t*	pixelData;
TGAHeader	header;
char		path[256];

			/* READ 'EM IN */

	for (int i = 0; i < MAX_SPRITES; i++)
	{
		snprintf(path, sizeof(path), ":Images:Infobar:%d.tga", 128 + i);

		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &spec);
		err = ReadTGA(&spec, &pixelData, &header, true);
		GAME_ASSERT(!err);

		gSprites[i] = (uint32_t*) pixelData;
		gSpriteWidths[i] = header.width;
		gSpriteHeights[i] = header.height;

		gSpriteMasks[i] = (uint32_t*) NewPtrClear(4 * header.width * header.height);


#if __BIG_ENDIAN__
		const uint32_t alphaMask = 0x000000FF;
#else
		const uint32_t alphaMask = 0xFF000000;
#endif

		for (int p = 0; p < header.width * header.height; p++)
		{
			gSpriteMasks[i][p] = gSprites[i][p]==alphaMask? 0x00000000: 0xFFFFFFFF;
		}
	}
}

/****************** LOAD INFOBAR ART ********************/

static void LoadNitroGaugeTemplate(void)
{
	GAME_ASSERT_MESSAGE(gNitroGaugeData == nil, "nitro gauge template already loaded");

	FSSpec spec;
	TGAHeader tga;
	OSErr err;
	
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Images:Infobar:NitroGauge.tga", &spec);
	err = ReadTGA(&spec, &gNitroGaugeData, &tga, false);
	GAME_ASSERT(err == noErr);
	GAME_ASSERT(tga.imageType == TGA_IMAGETYPE_RAW_GRAYSCALE);
	
	gNitroGaugeRect.left		= TIMER_X - tga.width/2;
	gNitroGaugeRect.right		= gNitroGaugeRect.left + tga.width;
	gNitroGaugeRect.top			= TIMER_Y;
	gNitroGaugeRect.bottom		= gNitroGaugeRect.top + tga.height;

	// Prepare alpha skip table. The nitro gauge is mostly made up of transparent pixels,
	// so this will speed up rendering a little.

	gNitroGaugeAlphaSkips	= (int*) NewPtrClear(tga.height * gNitroGaugeMaxSkipsPerRow * sizeof(int));
	uint8_t*	grayscale	= gNitroGaugeData;
	int*		skipTable	= gNitroGaugeAlphaSkips;
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
	if (gInfobarArtLoaded)
		return;

			/* READ SPRITES */

	LoadSpriteResources();

			/* READ NITRO GAUGE TEMPLATE */

	LoadNitroGaugeTemplate();

	gInfobarArtLoaded = true;
}


void FreeInfobarArt(void)
{
	if (gNitroGaugeData != nil)
	{
		DisposePtr((Ptr) gNitroGaugeData);
		gNitroGaugeData = nil;
	}
	
	if (gNitroGaugeAlphaSkips != nil)
	{
		DisposePtr((Ptr) gNitroGaugeAlphaSkips);
		gNitroGaugeAlphaSkips = nil;
	}

	if (gInfobarArtLoaded)
	{
		for (int i = 0; i < MAX_SPRITES; i++)
		{
			DisposePtr((Ptr) gSprites[i]);
			DisposePtr((Ptr) gSpriteMasks[i]);
			gSprites[i] = nil;
			gSpriteMasks[i] = nil;
		}
	}

	gInfobarArtLoaded = false;
}

static uint32_t* GetInfobarTextureOffset(int x, int y)
{
	uint32_t* out = gInfobarTexture + (y * INFOBAR_TEXTURE_WIDTH + x);
	GAME_ASSERT(out >= gInfobarTexture);
	GAME_ASSERT(out < gInfobarTexture + 4 * INFOBAR_TEXTURE_WIDTH * INFOBAR_TEXTURE_HEIGHT);
	return out;
}

static void DamageInfobarTextureRect(int x, int y, int w, int h)
{
	int left	= x;
	int top		= y;
	int right	= x + w;
	int bottom	= y + h;

	if (!gInfobarTextureIsDirty)
	{
		gInfobarTextureDirtyRect.left		= left;
		gInfobarTextureDirtyRect.top		= top;
		gInfobarTextureDirtyRect.right		= right;
		gInfobarTextureDirtyRect.bottom		= bottom;
	}
	else
	{
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define MAX(a,b) ((a)>(b) ? (a) : (b))
		// Already dirty, expand existing dirty rect
		gInfobarTextureDirtyRect.top    = MIN(gInfobarTextureDirtyRect.top,    top);
		gInfobarTextureDirtyRect.left   = MIN(gInfobarTextureDirtyRect.left,   left);
		gInfobarTextureDirtyRect.bottom = MAX(gInfobarTextureDirtyRect.bottom, bottom);
		gInfobarTextureDirtyRect.right  = MAX(gInfobarTextureDirtyRect.right,  right);
#undef MIN
#undef MAX
	}

	gInfobarTextureIsDirty = true;
}


/********************** DRAW SPRITE ****************************/

static void DrawSprite(int spriteNum, int x, int y)
{
	GAME_ASSERT(gInfobarArtLoaded);
	GAME_ASSERT_MESSAGE(spriteNum < MAX_SPRITES, "illegal sprite #");

	uint32_t* out = (uint32_t*) GetInfobarTextureOffset(x, y);
	const uint32_t* in = gSprites[spriteNum];
	const uint32_t* inMask = gSpriteMasks[spriteNum];

	const int spriteWidth = gSpriteWidths[spriteNum];
	const int spriteHeight = gSpriteHeights[spriteNum];

	for (int row = 0; row < spriteHeight; row++)
	{
		for (int col = 0; col < spriteWidth; col++)
		{
			*out = (*out & ~*inMask) | *in;
			out++;
			in++;
			inMask++;
		}

		out += INFOBAR_TEXTURE_WIDTH - spriteWidth;
	}

	GAME_ASSERT(out <= gInfobarTexture + INFOBAR_TEXTURE_WIDTH * INFOBAR_TEXTURE_HEIGHT);

	DamageInfobarTextureRect(x, y, spriteWidth, spriteHeight);
}


/********************** ERASE SPRITE ****************************/

static void EraseSprite(int spriteNum, int x, int y)
{
	GAME_ASSERT(gInfobarArtLoaded);
	GAME_ASSERT_MESSAGE(spriteNum < MAX_SPRITES, "illegal sprite #");

	const int spriteWidth = gSpriteWidths[spriteNum];
	const int spriteHeight = gSpriteHeights[spriteNum];

	const int backgroundWidth = gSpriteWidths[SPRITE_INFOBARTOP];

	uint32_t* out = (uint32_t*) GetInfobarTextureOffset(x, y);

	const uint32_t* in = gSprites[SPRITE_INFOBARTOP] + x + backgroundWidth * y;
	//GAME_ASSERT(HandleBoundsCheck(....));

	for (int row = 0; row < spriteHeight; row++)
	{
		memcpy(out, in, 4*spriteWidth);
		out += INFOBAR_TEXTURE_WIDTH;
		in += backgroundWidth;
	}

	GAME_ASSERT(out <= gInfobarTexture + (INFOBAR_TEXTURE_WIDTH * INFOBAR_TEXTURE_HEIGHT * 4));

	DamageInfobarTextureRect(x, y, spriteWidth, spriteHeight);
}


/****************** GET SPRITE WIDTH **********************/

static int GetSpriteWidth(int spriteNum)
{
	GAME_ASSERT(gInfobarArtLoaded);
	GAME_ASSERT_MESSAGE(spriteNum < MAX_SPRITES, "illegal sprite #");

	return gSpriteWidths[spriteNum];
}


#pragma mark -

/****************** DRAW NITRO GAUGE ************************/
// Source port rewrite.
// Original code used FrameArc(). We use a reference image instead.
// arcSpan is in degrees. 0: gauge empty; 180: gauge full.

static void DrawNitroGauge(int arcSpan)
{
	Boolean wantMargin = (arcSpan < 178) && (arcSpan > 2);

	uint8_t* templateRow = gNitroGaugeData;
	int nitroGaugeWidth = gNitroGaugeRect.right - gNitroGaugeRect.left;
	int nitroGaugeHeight = gNitroGaugeRect.bottom - gNitroGaugeRect.top;
	
	uint32_t* outRow = GetInfobarTextureOffset(gNitroGaugeRect.left, gNitroGaugeRect.top);

	int* skipTable = gNitroGaugeAlphaSkips;

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
			
			uint32_t fillColor = 0;

			if (t <= arcSpan)						// green (original: 0x0000,0xBDEF,0x294A)
			{
				fillColor = 0x00bd29FF;
			}
			else if (wantMargin && t <= arcSpan+3)	// margin line (original: 0xffff,0xF7BD,0x0000)
			{
				fillColor = 0xfff700FF;
			}
			else									// black
			{
				fillColor = 0x000000FF;
			}

			outRow[x] = UnpackU32BE(&fillColor);

			x++;
		}

		templateRow += nitroGaugeWidth;
		outRow += INFOBAR_TEXTURE_WIDTH;
	}

	DamageInfobarTextureRect(gNitroGaugeRect.left, gNitroGaugeRect.top, nitroGaugeWidth, nitroGaugeHeight);
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

	if (!gBallIconIsDisplayed)
	{
		if ((gOldLeftArmType != gLeftArmType) || eraseBugStuff)				// erase if changed
			EraseSprite(gOldLeftArmType, HAND_X - GetSpriteWidth(gOldLeftArmType), HAND_Y);

		if ((gOldRightArmType != gRightArmType) || eraseBugStuff)
			EraseSprite(gOldRightArmType, HAND_X, HAND_Y);
	}


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

		if (gPlayerMode == PLAYER_MODE_BALL &&
			BallHasHeadroomToMorphToBug())			// extend ball mode as long as bug head would materialize in ceiling
		{
			InitPlayer_Bug(gPlayerObj, &gMyCoord, gPlayerObj->Rot.y, PLAYER_ANIM_UNROLL);
		}
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

static void FillInfobarRect(const Rect r, uint32_t fillColor)
{
	fillColor = UnpackU32BE(&fillColor);

	uint32_t* dst = GetInfobarTextureOffset(r.left, r.top);

	for (int y = r.top; y < r.bottom; y++)
	{
		for (int x = 0; x < r.right - r.left; x++)
		{
			dst[x] = fillColor;
		}
		dst += INFOBAR_TEXTURE_WIDTH;
	}

	DamageInfobarTextureRect(r.left, r.top, r.right - r.left, r.bottom - r.top);
}

static void ShowHealth(void)
{
int		n;
Rect	r;

	n = (float)HEALTH_WIDTH * gMyHealth;

			/* RED FILLER */
			
	r.left = HEALTH_X - HEALTH_WIDTH/2;
	r.right = r.left + n;
	r.top = HEALTH_Y;
	r.bottom = r.top + HEALTH_HEIGHT;

	if (r.right > r.left)
	{
		FillInfobarRect(r, 0xF70018FF);
	}

			/* EMPTY FILLER */

	r.left = r.right;
	r.right = (HEALTH_X - HEALTH_WIDTH/2) + HEALTH_WIDTH;

	if (r.right > r.left)
	{
		FillInfobarRect(r, 0x000000FF);
	}

			/* MARGIN LINE */

	n = r.right;		
	r.right = r.left + 2;

	if (r.right < (n-2))
	{
		FillInfobarRect(r, 0xFFF700FF);
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
			
	r.top = BOTTOM_BAR_Y_IN_TEXTURE + 20;
	r.bottom = r.top + 20;
	r.left = 320-(BOSS_WIDTH/2);
	x = r.right = r.left + BOSS_WIDTH;
			
		/* FRAME */

	FillInfobarRect(r, 0x000000FF);

		/* METER */
		
	r.top += 2;
	r.bottom -= 2;
	r.left += 2;
	w = health * (float)BOSS_WIDTH;
	r.right = r.left + w - 4; 	
	if (w > 0)
	{
		FillInfobarRect(r, 0xdd0806FF);
	}
		
		/* EMPTY */
	
	if (w < BOSS_WIDTH)
	{
		r.left = r.right;
		r.right = x-2;
		FillInfobarRect(r, 0x000000FF);
	}


	gBossHealthWasUpdated = true;
}


void SubmitInfobarOverlay(void)
{
	if (!gInfobarTextureName)
		return;

	// If the screen port has dirty pixels ("damaged"), update the texture
	if (gInfobarTextureIsDirty)
	{
		Render_UpdateTexture(
				gInfobarTextureName,
				gInfobarTextureDirtyRect.left,
				gInfobarTextureDirtyRect.top,
				gInfobarTextureDirtyRect.right - gInfobarTextureDirtyRect.left,
				gInfobarTextureDirtyRect.bottom - gInfobarTextureDirtyRect.top,
				GL_RGBA,
				GL_UNSIGNED_BYTE,
				GetInfobarTextureOffset(gInfobarTextureDirtyRect.left, gInfobarTextureDirtyRect.top),
				INFOBAR_TEXTURE_WIDTH);

		// Clear damage
		gInfobarTextureIsDirty = false;
	}

	Render_SubmitMesh(gInfobarTopMesh, NULL, &kDefaultRenderMods_UI, &kQ3Point3D_Zero);

	if (gGamePrefs.showBottomBar || gBossHealthWasUpdated)
		Render_SubmitMesh(gInfobarBottomMesh, NULL, &kDefaultRenderMods_UI, &kQ3Point3D_Zero);
}
