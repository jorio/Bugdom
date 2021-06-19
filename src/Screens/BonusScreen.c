/****************************/
/*   	BONUS SCREEN.C	    */
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupBonusScreen(void);
static void MoveBonusText(ObjNode *theNode);
static void BuildBonusDigits(void);
static void MoveBonusDigit(ObjNode *theNode);
static void TallyLadyBugs(void);
static void TallyTotalScore(void);
static void BuildScoreDigits(void);
static Boolean AskSaveGame(void);
static void DrawBonusStuff(float duration);
static void TallyGreenClovers(void);
static void TallyGoldClovers(void);
static void TallyBlueClovers(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	LADYBUG_BONUS_POINTS		1500
#define	GREENCLOVER_BONUS_POINTS 	200
#define	GOLDCLOVER_BONUS_POINTS		10000
#define	BLUECLOVER_BONUS_POINTS		3000

#define	MAX_DIGITS_IN_BONUS		6
#define	DIGIT_WIDTH				20.0f

#define	LADYBUG_WIDTH			30.0f
#define	GREEN_CLOVER_WIDTH		20.0f
#define	GOLD_CLOVER_WIDTH		35.0f
#define	BLUE_CLOVER_WIDTH		35.0f

#define	MAX_DIGITS_IN_SCORE		8



/*********************/
/*    VARIABLES      */
/*********************/

static ObjNode	*gBonusDigits[MAX_DIGITS_IN_BONUS];
static float	gBD1[MAX_DIGITS_IN_BONUS];
static float	gBD2[MAX_DIGITS_IN_BONUS];

static int		gBonusValue;

static ObjNode	*gScoreDigits[MAX_DIGITS_IN_SCORE];
static float	gSD1[MAX_DIGITS_IN_SCORE];
static float	gSD2[MAX_DIGITS_IN_SCORE];

ObjNode	*gSaveYes,*gSaveNo;

static float	gMoveTextUpwards;

/********************** DO BONUS SCREEN *************************/

void DoBonusScreen(void)
{
Boolean wantToSave = false;

	
			/*********/
			/* SETUP */
			/*********/

	SetupUIStuff();
	SetupBonusScreen();

	QD3D_CalcFramesPerSecond();
	QD3D_CalcFramesPerSecond();

		/**************/
		/* PROCESS IT */
		/**************/
				
	DrawBonusStuff(1);
	
	TallyLadyBugs();
	TallyGreenClovers();
	TallyBlueClovers();
	TallyGoldClovers();
	TallyTotalScore();
	
	if (gRealLevel < LEVEL_NUM_ANTKING)		// dont save game if I just won the last level
		wantToSave = AskSaveGame();
	else
		DrawBonusStuff(4);				// otherwise, just wait a while for user to see their score
	
			/* CLEANUP */

	CleanupUIStuff();

			/* SHOW FILE PICKER IF WANT TO SAVE */

	if (wantToSave)
	{
		if (gCurrentSaveSlot < 0)
		{
			gCurrentSaveSlot = DoFileSelectScreen(FILE_SELECT_SCREEN_TYPE_SAVE);
		}

		// Re-check current save slot. Maybe user doesn't want to save after all.
		if (gCurrentSaveSlot >= 0)
		{
			SaveGame(gCurrentSaveSlot);
		}
	}
}


/****************** SETUP BONUS SCREEN **************************/

static void SetupBonusScreen(void)
{
	PlaySong(SONG_BONUS,false);

	// Reset move text upwards
	gMoveTextUpwards = 0;

			/* LOAD ART */

	LoadASkeleton(SKELETON_TYPE_LADYBUG);

			/* TEXT */
				
	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;	
	gNewObjectDefinition.type 		= BONUS_MObjType_Text;	
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 110;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= MoveBonusText;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .9;
	MakeNewDisplayGroupObject(&gNewObjectDefinition);


		/*****************************/
		/* CREATE BONUS SCORE DIGITS */
		/*****************************/

	for (int i = 0; i < MAX_DIGITS_IN_BONUS; i++)
	{
		gBonusDigits[i] = nil;
		gBD1[i] = RandomFloat()*PI2;
		gBD2[i] = RandomFloat()*PI2;
	}
	gBonusValue = 0;
	
	BuildBonusDigits();
	
	
		/*****************************/
		/* CREATE TOTAL SCORE DIGITS */
		/*****************************/

	for (int i = 0; i < MAX_DIGITS_IN_SCORE; i++)
	{
		gScoreDigits[i] = nil;
		gSD1[i] = RandomFloat()*PI2;
		gSD2[i] = RandomFloat()*PI2;
	}
	
	
	MakeFadeEvent(true);
}

#pragma mark -

/*************** BUILD BONUS DIGITS ******************/

static void BuildBonusDigits(void)
{
int		i,numDigits,num,digit;
float	x;

	num = gBonusValue;


			/* CLEAR EXISTING DIGITS */
			
	for (i = 0; i < MAX_DIGITS_IN_BONUS; i++)
	{
		if (gBonusDigits[i])
		{
			DeleteObject(gBonusDigits[i]);
			gBonusDigits[i] = nil;
		}
	}

			/* DETERMINE THE NUMBER OF DIGITS NEEDED */
			
	numDigits = 1;
	if (num >= 10)
		numDigits++;
	if (num >= 100)
		numDigits++;
	if (num >= 1000)
		numDigits++;
	if (num >= 10000)
		numDigits++;
	if (num >= 100000)
		numDigits++;
	
	
			/****************************/
			/* CREATE THE DIGIT OBJECTS */
			/****************************/
				
	x = (float)(numDigits-1) * (DIGIT_WIDTH/2);				// start with right digit

	
	for (i = 0; i < numDigits; i++)
	{
		digit = num % 10;									// get digit value
		num /= 10;
	
		gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;	
		gNewObjectDefinition.type 		= BONUS_MObjType_0+digit;	
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= 60;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.slot 		= 101;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= MoveBonusDigit;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= .4;
		gBonusDigits[i] = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		gBonusDigits[i]->Flag[0] = i;
				
		x -= DIGIT_WIDTH;
	}
}


/*************** BUILD SCORE DIGITS ******************/

static void BuildScoreDigits(void)
{
int		i,numDigits,num,digit;
float	x;

	num = gScore;


			/* CLEAR EXISTING DIGITS */
			
	for (i = 0; i < MAX_DIGITS_IN_SCORE; i++)
	{
		if (gScoreDigits[i])
		{
			DeleteObject(gScoreDigits[i]);
			gScoreDigits[i] = nil;
		}
	}

			/* DETERMINE THE NUMBER OF DIGITS NEEDED */
			
	numDigits = 1;
	if (num >= 10)
		numDigits++;
	if (num >= 100)
		numDigits++;
	if (num >= 1000)
		numDigits++;
	if (num >= 10000)
		numDigits++;
	if (num >= 100000)
		numDigits++;
	if (num >= 1000000)
		numDigits++;
	if (num >= 10000000)
		numDigits++;
	if (num >= 100000000)
		numDigits++;
	
	
			/****************************/
			/* CREATE THE DIGIT OBJECTS */
			/****************************/
				
	x = (float)(numDigits-1) * (DIGIT_WIDTH/2);				// start with right digit

	
	for (i = 0; i < numDigits; i++)
	{
		digit = num % 10;									// get digit value
		num /= 10;
	
		gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;	
		gNewObjectDefinition.type 		= BONUS_MObjType_0+digit;	
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= -80;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.slot 		= 101;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= MoveBonusDigit;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= .4;
		gScoreDigits[i] = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		gScoreDigits[i]->Flag[0] = i;
				
		x -= DIGIT_WIDTH;
	}

}




/**************** MOVE BONUS TEXT *********************/

static void MoveBonusText(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Coord.z = cos(theNode->SpecialF[1] += fps*4.7f) * 10.0f;
	theNode->Rot.x = sin(theNode->SpecialF[2] += fps*3.0f) * .5f;

	theNode->Coord.y = theNode->InitCoord.y + gMoveTextUpwards;

	UpdateObjectTransforms(theNode);
}


/**************** MOVE BONUS DIGIT *********************/

static void MoveBonusDigit(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;

	i = theNode->Flag[0];

	theNode->Coord.z = cos(gBD1[i] += fps*3.7f) * 8.0f;
	theNode->Rot.x = sin(gBD2[i] += fps*2.0f) * .3f;

	theNode->Coord.y = theNode->InitCoord.y + gMoveTextUpwards;

	UpdateObjectTransforms(theNode);
}

/**************** MOVE BONUS LADYBUG *********************/

static void MoveBonusLadyBug(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Coord.y =theNode->InitCoord.y + cos(theNode->SpecialF[0] += fps*3.0f) * 5.0f;
	UpdateObjectTransforms(theNode);
}


#pragma mark -

/*************** TALLY LADY BUGS *******************/

static void TallyLadyBugs(void)
{
float	x;
int		i;
ObjNode	*ladybugs[100];

	if (gNumLadyBugsThisArea == 0)
		return;

	x = -((float)(gNumLadyBugsThisArea-1)*(LADYBUG_WIDTH/2.0f));

		/*******************/
		/* COUNT LADY BUGS */
		/*******************/

	for (i = 0; i < gNumLadyBugsThisArea; i++)
	{
			/* GIMME A BUG */
					
		gNewObjectDefinition.type 		= SKELETON_TYPE_LADYBUG;
		gNewObjectDefinition.animNum 	= 0;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= 20;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= MoveBonusLadyBug;
		gNewObjectDefinition.rot 		= PI;
		gNewObjectDefinition.scale 		= .12;
		ladybugs[i] = MakeNewSkeletonObject(&gNewObjectDefinition);	
	
		x += LADYBUG_WIDTH;
		gBonusValue += LADYBUG_BONUS_POINTS;
		BuildBonusDigits();
	
		PlayEffect(EFFECT_BONUSCLICK);
		DrawBonusStuff(.2);
	}	
	
	DrawBonusStuff(1);	
	
			/* DELETE LADY BUGS */
			
	for (i = 0; i < gNumLadyBugsThisArea; i++)
		DeleteObject(ladybugs[i]);

}


/*************** TALLY GREEN CLOVERS *******************/

static void TallyGreenClovers(void)
{
float	x,y;
int		i,n,c;
ObjNode	*clovers[100];

	n = gNumGreenClovers;				// calc # clovers
	if (n == 0)							// see if no clovers
		return;

	DrawBonusStuff(1);	

	if (n < 14)
		x = -((float)(n-1)*(GREEN_CLOVER_WIDTH/2.0f));
	else
		x = -((float)(14-1)*(GREEN_CLOVER_WIDTH/2.0f));
	
	y = 20;

		/*****************/
		/* COUNT CLOVERS */
		/*****************/

	for (c = i = 0; i < n; i++)
	{
			/* GIMME A CLOVER */
			
		gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;	
		gNewObjectDefinition.type 		= BONUS_MObjType_GreenClover;	
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= y;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= MoveBonusLadyBug;
		gNewObjectDefinition.rot 		= PI+PI/2;
		gNewObjectDefinition.scale 		= .05;
		clovers[i] = MakeNewDisplayGroupObject(&gNewObjectDefinition);
			
		x += GREEN_CLOVER_WIDTH;
		gBonusValue += GREENCLOVER_BONUS_POINTS;
		BuildBonusDigits();

		PlayEffect(EFFECT_BONUSCLICK);
		DrawBonusStuff(.2);	
		
		c++;
		if (c >= 14)
		{
			c = 0;
			y -= 15;
			x = -((float)(14-1)*(GREEN_CLOVER_WIDTH/2.0f));
		}
	}	

	DrawBonusStuff(1);	

			/* DELETE CLOVERS */
			
	for (i = 0; i < n; i++)
		DeleteObject(clovers[i]);
}


/*************** TALLY BLUE CLOVERS *******************/

static void TallyBlueClovers(void)
{
float	x;
int		i,n;
ObjNode	*clovers[100];

	n = gNumBlueClovers/4;				// calc # whole clovers
	if (n == 0)							// see if no whole clovers
		return;

	DrawBonusStuff(1);	

	x = -((float)(n-1)*(BLUE_CLOVER_WIDTH/2.0f));

		/*****************/
		/* COUNT CLOVERS */
		/*****************/

	for (i = 0; i < n; i++)
	{
			/* GIMME A CLOVER */
			
		gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;	
		gNewObjectDefinition.type 		= BONUS_MObjType_BlueClover;	
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= 20;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= MoveBonusLadyBug;
		gNewObjectDefinition.rot 		= PI+PI/2;
		gNewObjectDefinition.scale 		= .1;
		clovers[i] = MakeNewDisplayGroupObject(&gNewObjectDefinition);
			
		x += BLUE_CLOVER_WIDTH;
		gBonusValue += BLUECLOVER_BONUS_POINTS;
		BuildBonusDigits();

		PlayEffect(EFFECT_BONUSBELL);
		DrawBonusStuff(.2);	
	}	

	DrawBonusStuff(1);	

			/* DELETE CLOVERS */
			
	for (i = 0; i < n; i++)
		DeleteObject(clovers[i]);
}



/*************** TALLY GOLD CLOVERS *******************/

static void TallyGoldClovers(void)
{
float	x;
int		i,n;
ObjNode	*clovers[100];

	n = gNumGoldClovers/4;				// calc # whole clovers
	if (n == 0)							// see if no whole clovers
		return;

	DrawBonusStuff(1);	

	x = -((float)(n-1)*(GOLD_CLOVER_WIDTH/2.0f));

		/*****************/
		/* COUNT CLOVERS */
		/*****************/

	for (i = 0; i < n; i++)
	{
			/* GIMME A CLOVER */
			
		gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;	
		gNewObjectDefinition.type 		= BONUS_MObjType_GoldClover;	
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= 20;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.moveCall 	= MoveBonusLadyBug;
		gNewObjectDefinition.rot 		= PI+PI/2;
		gNewObjectDefinition.scale 		= .1;
		clovers[i] = MakeNewDisplayGroupObject(&gNewObjectDefinition);
			
		x += GOLD_CLOVER_WIDTH;
		gBonusValue += GOLDCLOVER_BONUS_POINTS;
		BuildBonusDigits();

		PlayEffect(EFFECT_BONUSBELL);
		DrawBonusStuff(.2);	
	}	

	DrawBonusStuff(1);	

			/* DELETE CLOVERS */
			
	for (i = 0; i < n; i++)
		DeleteObject(clovers[i]);
}



/*************** TALLY TOTAL SCORE *******************/

static void TallyTotalScore(void)
{	
	DrawBonusStuff(1);	

		/* CREATE SCORE OBJECT */
		
	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;	
	gNewObjectDefinition.type 		= BONUS_MObjType_Score;	
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= -30;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= MoveBonusText;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .9;
	MakeNewDisplayGroupObject(&gNewObjectDefinition);

	BuildScoreDigits();

	DrawBonusStuff(1);	


		/****************/
		/* TALLY POINTS */
		/****************/

	while(gBonusValue > 0)
	{
		DrawBonusStuff(.05);	
	
		if (gBonusValue >= 500)
		{
			gBonusValue -= 500;
			gScore += 500;
		}
		else
		{
			gScore += gBonusValue;
			gBonusValue = 0;
		}
	
		PlayEffect(EFFECT_BONUSCLICK);
		BuildScoreDigits();
		BuildBonusDigits();
	}	
	
	DrawBonusStuff(1);	
	
}


/*************** ASK SAVE GAME *******************/

static Boolean AskSaveGame(void)
{
int32_t id;

		/*************************/
		/* CREATE YES/NO OBJECTS */
		/*************************/
		
				/* YES */
				
	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;	
	gNewObjectDefinition.type 		= BONUS_MObjType_SaveIcon;	
	gNewObjectDefinition.coord.x 	= -50;
	gNewObjectDefinition.coord.y 	= -100-70;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= MoveBonusText;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .7;
	gSaveYes = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	gSaveYes->IsPickable = true;
	gSaveYes->PickID = 0;

				/* NO */
				
	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;	
	gNewObjectDefinition.type 		= BONUS_MObjType_DontSaveIcon;	
	gNewObjectDefinition.coord.x 	= 50;
	gNewObjectDefinition.coord.y 	= -100-70;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= MoveBonusText;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .7;
	gSaveNo = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	gSaveNo->IsPickable = true;
	gSaveNo->PickID = 1;

	gMoveTextUpwards = 0;
	float moveTextUpwardsTween = 0;
	bool captionsCreatedYet = false;

	InitCursor();
	while(true)
	{
		moveTextUpwardsTween += gFramesPerSecondFrac;

		gMoveTextUpwards = TweenFloat(EaseInOutQuad, moveTextUpwardsTween, 1.0f, 0, 100);

		if (moveTextUpwardsTween > 1.0f && !captionsCreatedYet)
		{
			TextMeshDef tmd;
			TextMesh_FillDef(&tmd);
			tmd.scale = 0.2f;

			tmd.coord = (TQ3Point3D) {-50,-100,0};
			tmd.color = TQ3ColorRGBA_FromInt(0x0080FFFF);
			if (gCurrentSaveSlot >= 0)
			{
				char message[] = "Save to file X";
				message[sizeof(message)-2] = 'A' + gCurrentSaveSlot;
				TextMesh_Create(&tmd, message);
			}
			else
			{
				TextMesh_Create(&tmd, "Save");
			}

			tmd.coord = (TQ3Point3D) {50,-100,0};
			tmd.color = TQ3ColorRGBA_FromInt(0xe54c19ff);
			TextMesh_Create(&tmd, "Don't save yet");

			captionsCreatedYet = true;
		}

		UpdateInput();
		MoveObjects();
		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);
		QD3D_CalcFramesPerSecond();				
		DoSDLMaintenance();
		
		if (FlushMouseButtonPress())
		{
			int mouseX = 0;
			int mouseY = 0;
			SDL_GetMouseState(&mouseX, &mouseY);

			if (PickObject(mouseX, mouseY, &id))
				break;
		}
	}
	HideCursor();

		/* SEE IF SAVE GAME */

	return id == 0;
}


/******************* DRAW BONUS STUFF ********************/

static void DrawBonusStuff(float duration)
{
#if _DEBUG
	printf("Warning: speeding up bonus screen because this is a debug build\n");
	duration = 0.05f;
#endif
	do
	{
		UpdateInput();
		MoveObjects();
		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);
		QD3D_CalcFramesPerSecond();				
		DoSDLMaintenance();
	}while((duration -= gFramesPerSecondFrac) > 0.0f);
}

