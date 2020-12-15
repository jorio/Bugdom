/****************************/
/* FILE SELECT              */
/* Source port addition     */
/****************************/

/*
 * The original game used to pop up a standard file open/save dialog
 * to manage save games. For portability's sake, the source port
 * implements a rudimentary in-game save file picker instead.
 */


/****************************/
/*    EXTERNALS             */
/****************************/

#include <time.h>

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	TQ3Point3D			gCoord;
extern	WindowPtr			gCoverWindow;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	Boolean		gSongPlayingFlag,gResetSong,gDisableAnimSounds,gSongPlayingFlag;
extern	FSSpec		gDataSpec;
extern	PrefsType	gGamePrefs;
extern	u_short		gLevelType,gRealLevel;
extern	u_long			gScore;
extern	TQ3Object	gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupFileScreen(int type);
static int FileScreenMainLoop(void);


/****************************/
/*    CONSTANTS             */
/****************************/

static const char* gLevelNames[10] =
{
	"Training",
	"The Lawn",
	"The Pond",
	"The Forest",
	"The Hive Attack",
	"The Bee Hive",
	"The Queen Bee",
	"Night Attack",
	"The Ant Hill",
	"The Ant King",
};


#define MAX_PICKABLES 32

static TQ3Object gPickables[MAX_PICKABLES];
static int gNumPickables = 0;
static int gHoveredPick = -1;

static const TQ3ColorRGB gTextShadowColor	= { 0.0f, 0.0f, 0.3f };
static const TQ3ColorRGB gTextColor			= { 1.0f, 0.9f, 0.0f };
static const TQ3ColorRGB gTitleTextColor	= { 1.0f, 0.9f, 0.0f };


static char textBuffer[512];





/*********************/
/*    VARIABLES      */
/*********************/


static TQ3Point3D	gBonusCameraFrom = {0, 0, 250 };


/********************** DO BONUS SCREEN *************************/

int DoFileSelectScreen(int type)
{
	/*********/
	/* SETUP */
	/*********/

	InitCursor();

	SetupFileScreen(type);

	QD3D_CalcFramesPerSecond();

	/**************/
	/* PROCESS IT */
	/**************/

	int picked = FileScreenMainLoop();

	/* CLEANUP */

	for (int i = 0; i < gNumPickables; i++)
	{
		Q3Object_Dispose(gPickables[i]);
		gPickables[i] = nil;
	}
	gNumPickables = 0;

	GammaFadeOut();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DeleteAll3DMFGroups();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);
	DisposeSoundBank(SOUND_BANK_BONUS);
	GameScreenToBlack();

	return picked;
}


/****************** SETUP FILE SCREEN **************************/

static void MoveFloppy(ObjNode *theNode)
{
	float	fps = gFramesPerSecondFrac;

	if (gHoveredPick == theNode->SpecialL[0])
	{
		theNode->SpecialF[0] += fps * 5.0f;
		theNode->Rot.y = cosf(0.5f * theNode->SpecialF[0]) * 0.53f;
		theNode->Coord.y = theNode->InitCoord.y + fabsf(cosf(1.0f*theNode->SpecialF[0])) * 7.0f;
	}
	else
	{
		theNode->Rot.y = 0;
		theNode->SpecialF[0] = 0;
		theNode->Coord.y = theNode->InitCoord.y;
	}

	UpdateObjectTransforms(theNode);
}


static void MakeText(
		const char* text,
		float x,
		float y,
		float z,
		const float baseScale,
		const TQ3ColorRGB* color)
{
	const float characterSpacing = 16.0f;
	const float spaceAdvance = 20.0f;

	float startX = x;
	ObjNode* firstTextNode = nil;
	ObjNode* lastTextNode = nil;

	for (; *text; text++)
	{
		char c = *text;
		bool isLower = false;
		int type = SCORES_ObjType_0;

		switch (c)
		{
			case ' ':
				x += spaceAdvance * baseScale;
				continue;

			case '0':	case '1':	case '2':	case '3':	case '4':
			case '5':	case '6':	case '7':	case '8':	case '9':
				type = SCORES_ObjType_0 + (c - '0'); break;

			case '.': type = SCORES_ObjType_Period; break;
			case '#': type = SCORES_ObjType_Pound; break;
			case '?': type = SCORES_ObjType_Question; break;
			case '!': type = SCORES_ObjType_Exclamation; break;
			case '-': type = SCORES_ObjType_Dash; break;
			case '\'': type = SCORES_ObjType_Apostrophe; break;
			case ':': type = SCORES_ObjType_Colon; break;

			default:
				if (c >= 'A' && c <= 'Z')
				{
					type = SCORES_ObjType_A + (c - 'A');
				}
				else if (c >= 'a' && c <= 'z')
				{
					type = SCORES_ObjType_A + (c - 'a');
					isLower = true;
				}
				else
				{
					type = SCORES_ObjType_Question;
				}
				break;
		}

		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type		= type;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.y 	= y;
		gNewObjectDefinition.coord.z 	= z;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0.0f;
		gNewObjectDefinition.scale 		= baseScale * (isLower? 0.8f: 1.0f);
		ObjNode* glyph = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		lastTextNode = glyph;
		if (!firstTextNode)
			firstTextNode = glyph;

		TQ3AttributeSet attrs = Q3AttributeSet_New();
		Q3AttributeSet_Add(attrs, kQ3AttributeTypeDiffuseColor, color);
		AttachGeometryToDisplayGroupObject(glyph, attrs);

		glyph->SpecialL[0] = 0;
		glyph->SpecialF[0] = 0;

		x = glyph->Coord.x;
		x += characterSpacing * baseScale * (isLower ? 1.0f : 1.2f);
	}

	// Center text horizontally
	float totalWidth = x - startX;
	for (ObjNode* node = firstTextNode; node; node = node->NextNode)
	{
		node->Coord.x -= (totalWidth - characterSpacing * baseScale) / 2.0f;
		UpdateObjectTransforms(node);
		if (node == lastTextNode)
			break;
	}
}

static void MakeTextWithShadow(
		const char* text,
		float x,
		float y,
		const float baseScale,
		const TQ3ColorRGB* color)
{
	MakeText(text, x, y, 0, baseScale, color);
	MakeText(text, x+2*baseScale, y-2*baseScale, -0.001, baseScale, &gTextShadowColor);
}

static void MakeFileSlot(
		const int slotNumber,
		const float x,
		const float y
)
{
	SaveGameType saveData;
	OSErr saveDataErr = GetSaveGameData(slotNumber, &saveData);
	bool saveDataValid = saveDataErr == noErr;



	const float gs = 0.8f;

	// ----------------------
	// Make floppy
	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
	gNewObjectDefinition.type 		= BONUS_MObjType_SaveIcon;
	gNewObjectDefinition.coord.x 	= x - 50 * gs;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= MoveFloppy;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0f * gs;
	ObjNode* newFloppy = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newFloppy->SpecialL[0] = slotNumber;

//	GAME_ASSERT(gNumPickables < MAX_PICKABLES);
//	pickable[gNumPickables++] = newFloppy->BaseGroup;

	snprintf(textBuffer, sizeof(textBuffer), "File %c", 'A' + slotNumber);

	MakeTextWithShadow(textBuffer, x, y + 16 * gs, .6f * gs, &gTextColor);

	if (saveDataValid)
		snprintf(textBuffer, sizeof(textBuffer), "Level %d: %s", 1 + saveData.realLevel, gLevelNames[saveData.realLevel]);
	else
		snprintf(textBuffer, sizeof(textBuffer), "---- blank ----");
	MakeTextWithShadow(textBuffer, x, y - 8 * gs, 0.4f * gs, &gTextColor);

	if (saveDataValid)
	{
		struct tm tm;
		tm = *localtime(&saveData.timestamp);
		strftime(textBuffer, sizeof(textBuffer), "%-e %b %Y   %-l:%M%p", &tm);
		MakeTextWithShadow(textBuffer, x, y - 24 * gs, 0.4f * gs, &gTextColor);
	}


	{
		// INIT PICKABLE QUAD GEOMETRY

		float left		= x - 70;
		float right		= x + 150;
		float top		= y + 25 * gs;
		float bottom	= y - 25 * gs;

		TQ3PolygonData pickableQuad;

		TQ3Vertex3D pickableQuadVertices[4] = {
				{{left, top, 0}, nil},
				{{right, top, 0}, nil},
				{{right, bottom, 0}, nil},
				{{left, bottom, 0}, nil},
		};

		pickableQuad.numVertices = 4;
		pickableQuad.vertices = pickableQuadVertices;
		pickableQuad.polygonAttributeSet = nil;

		TQ3GeometryObject poly = Q3Polygon_New(&pickableQuad);
		GAME_ASSERT(gNumPickables < MAX_PICKABLES);
		gPickables[gNumPickables++] = poly;
	}

}


/**************** MOVE BONUS BACKGROUND *********************/

static void MoveBonusBackground(ObjNode *theNode)
{
	float	fps = gFramesPerSecondFrac;

	theNode->Rot.y += fps * .03f;
	UpdateObjectTransforms(theNode);
}




static void SetupFileScreen(int type)
{
	FSSpec					spec;
	QD3DSetupInputType		viewDef;
	TQ3Point3D				cameraTo = {0, 0, 0 };
	TQ3ColorRGB				lightColor = { 1.0, 1.0, .9 };
	TQ3Vector3D				fillDirection1 = { 1, -.4, -.8 };			// key
	TQ3Vector3D				fillDirection2 = { -.7, -.2, -.9 };			// fill


	/* RESET PICKABLE ITEMS */
	gNumPickables = 0;

	/*************/
	/* MAKE VIEW */
	/*************/

	QD3D_NewViewDef(&viewDef, gCoverWindow);

	viewDef.camera.hither 			= 40;
	viewDef.camera.yon 				= 2000;
	viewDef.camera.fov 				= 1.0;
	viewDef.styles.usePhong 		= false;
	viewDef.camera.from				= gBonusCameraFrom;
	viewDef.camera.to	 			= cameraTo;

	viewDef.lights.numFillLights 	= 2;
	viewDef.lights.ambientBrightness = 0.2;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[0] 	= lightColor;
	viewDef.lights.fillColor[1] 	= lightColor;
	viewDef.lights.fillBrightness[0] = 1.0;
	viewDef.lights.fillBrightness[1] = .2;

	viewDef.view.dontClear = false;
	viewDef.view.clearColor.r =
	viewDef.view.clearColor.g =
	viewDef.view.clearColor.b = 0;


	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);

	/************/
	/* LOAD ART */
	/************/

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:BonusScreen.3dmf", &spec);
	LoadGrouped3DMF(&spec,MODEL_GROUP_BONUS);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:HighScores.3dmf", &spec);
	LoadGrouped3DMF(&spec,MODEL_GROUP_LEVELSPECIFIC);

	// Nuke color attribute from glyph models so we can draw them in whatever color we like later.
	for (int i = SCORES_ObjType_0; i <= SCORES_ObjType_Colon; i++)
	{
		QD3D_ClearDiffuseColor_Recurse(gObjectGroupList[MODEL_GROUP_LEVELSPECIFIC][i]);
	}

	/* LOAD SOUNDS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:bonus.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_BONUS);

	/*******************/
	/* MAKE BACKGROUND */
	/*******************/

	/* BACKGROUND */

	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
	gNewObjectDefinition.type 		= BONUS_MObjType_Background;
	gNewObjectDefinition.coord		= gBonusCameraFrom;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_NULLSHADER;
	gNewObjectDefinition.moveCall 	= MoveBonusBackground;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 30.0;
	MakeNewDisplayGroupObject(&gNewObjectDefinition);

	/* TEXT */

	float y = 110.0f;
	float x = -40.0f;

	if (type == FILE_SELECT_SCREEN_TYPE_LOAD)
	{
		MakeTextWithShadow("Pick a Saved Game", -135.0f, y, 1.0f, &gTitleTextColor);
	}
	else
	{
		MakeTextWithShadow("Save where?", -80.0f, y, 1.0f, &gTitleTextColor);

		snprintf(textBuffer, sizeof(textBuffer), "You are entering Level %d", gRealLevel+2);
		MakeTextWithShadow(textBuffer, -98.0f, y-25.0f, 0.5f, &gTitleTextColor);
	}

	y -= 64.0f;
	MakeFileSlot(0, x, y);
	y -= 64.0f;
	MakeFileSlot(1, x, y);
	y -= 64.0f;
	MakeFileSlot(2, x, y);


	y -= 50.0f;
	//MakeFloppy(floppyx, y-16.0f);
//	MakeText("Continue without saving", x, y, 0.5f); y -= 16.0f;

	//---------------------------------------------------------------

	MakeFadeEvent(true);
}





/*************** PICK FILE SELECT ICON ********************/
//
// INPUT: point = point of click
//
// OUTPUT: TRUE = something got picked
//			pickID = pickID of picked object
//

Boolean	PickFileSelectIcon(TQ3Point2D point,u_long *pickID)
{
	TQ3PickObject			myPickObject;
	TQ3Uns32			myNumHits;
	TQ3Status				myErr;

	/* CREATE PICK OBJECT */

	myPickObject = CreateDefaultPickObject(&point);						// create pick object



	/*************/
	/* PICK LOOP */
	/*************/

	myErr = Q3View_StartPicking(gGameViewInfoPtr->viewObject,myPickObject);
	GAME_ASSERT(myErr);
	do
	{
		for (int j = 0; j < gNumPickables; j++)
		{
			Q3PickIDStyle_Submit(j, gGameViewInfoPtr->viewObject);
			Q3Object_Submit(gPickables[j], gGameViewInfoPtr->viewObject);
		}
	} while (Q3View_EndPicking(gGameViewInfoPtr->viewObject) == kQ3ViewStatusRetraverse);


	/*********************************/
	/* SEE WHETHER ANY HITS OCCURRED */
	/*********************************/

	if (Q3Pick_GetNumHits(myPickObject, &myNumHits) != kQ3Failure)
	{
		if (myNumHits > 0)
		{
			/* PROCESS THE HIT */

			Q3Pick_GetPickDetailData(myPickObject,0,kQ3PickDetailMaskPickID,pickID);	// get pickID
			Q3Object_Dispose(myPickObject);												// dispose of pick object
			return(true);
		}
	}

	/* DIDNT HIT ANYTHING */

	Q3Object_Dispose(myPickObject);
	return(false);
}











/******************* DRAW BONUS STUFF ********************/

static int FileScreenMainLoop()
{
	while (1)
	{
		UpdateInput();
		MoveObjects();

		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);

		QD3D_CalcFramesPerSecond();
		DoSDLMaintenance();

		// See if user hovered/clicked on a pickable item
		gHoveredPick = -1;
		{
			Point		mouse;
			TQ3Point2D	pt;

			SetPort(GetWindowPort(gCoverWindow));
			GetMouse(&mouse);
			pt.x = mouse.h;
			pt.y = mouse.v;

			u_long pickID;
			if (PickFileSelectIcon(pt, &pickID))
			{
				gHoveredPick = pickID;

				if (FlushMouseButtonPress())
				{
					return pickID;
				}
			}
		}

		if (GetNewKeyState(kVK_Escape))
		{
			return -1;
		}
	}
}
