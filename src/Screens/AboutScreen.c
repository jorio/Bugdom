// ABOUT SCREEN.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom


/****************************/
/*    EXTERNALS             */
#include <SDL_opengl.h>
#include <version.h>

/****************************/

extern	float						gFramesPerSecondFrac,gFramesPerSecond;
extern	WindowPtr					gCoverWindow;
extern	NewObjectDefinitionType		gNewObjectDefinition;
extern	QD3DSetupOutputType			*gGameViewInfoPtr;
extern	FSSpec						gDataSpec;
extern	PrefsType					gGamePrefs;
extern 	ObjNode 					*gFirstNodePtr;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void AboutScreenDrawStuff(const QD3DSetupOutputType *setupInfo);
static void MakeAboutScreenObjects(int slideNumber);


/****************************/
/*    CONSTANTS             */
/****************************/

static const TQ3ColorRGB kNameColor			= { 152/255.0f, 1.0f, 205/255.0f };
static const TQ3ColorRGB kHeadingColor		= { 1,1,1 };
static const TQ3ColorRGB kDimmedColor		= { .6f,.6f,.6f };

static const short kAboutScreenObjNodeSlot = 1337;

/*********************/
/*    VARIABLES      */
/*********************/


/********************** DO BONUS SCREEN *************************/

void DoAboutScreens(void)
{
	/*********/
	/* SETUP */
	/*********/

	SetupUIStuff();
	QD3D_CalcFramesPerSecond();

	/**************/
	/* PROCESS IT */
	/**************/

	MakeFadeEvent(true);

	for (int i = 0; i < 3; i++)
	{
		NukeObjectsInSlot(999);							// hack to get rid of background
		NukeObjectsInSlot(kAboutScreenObjNodeSlot);		// nuke text from previous slide

		MakeAboutScreenObjects(i);

		FlushMouseButtonPress();

		while (1)
		{
			UpdateInput();
			MoveObjects();
			QD3D_MoveParticles();
			QD3D_DrawScene(gGameViewInfoPtr, AboutScreenDrawStuff);
			QD3D_CalcFramesPerSecond();
			DoSDLMaintenance();

			if (GetSkipScreenInput())
				break;
		}
	}

	/* CLEANUP */

	CleanupUIStuff();
}

/****************** SETUP FILE SCREEN **************************/

static void MakeCreditPart(
		float x,
		float y,
		const char* heading,
		const char* text1,
		const char* text2
		)
{
	const float LH = 13;
	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);
	tmd.withShadow = false;
	tmd.slot = kAboutScreenObjNodeSlot;
	tmd.scale = .25f;
	tmd.coord = (TQ3Point3D) {x,y,0};
	tmd.lowercaseScale = 1;
	tmd.characterSpacing *= 1.33f;

	tmd.color = kHeadingColor;
	TextMesh_Create(&tmd, heading);

	tmd.color = kNameColor;
	tmd.coord.y -= LH;
	TextMesh_Create(&tmd, text1);
	tmd.coord.y -= LH * .75f;
	tmd.scale = .15f;
	TextMesh_Create(&tmd, text2);

}

static void MakeAboutScreenObjects(int slideNumber)
{
	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);
	tmd.slot = kAboutScreenObjNodeSlot;
	tmd.coord.y += 110;
	tmd.withShadow = false;
	tmd.lowercaseScale = 1;
	tmd.characterSpacing *= 1.33f;
	tmd.color = kHeadingColor;
	tmd.scale = .66f;

	const float LH = 13;

	switch (slideNumber)
	{
		case 0:
		{
			TextMesh_Create(&tmd, "CREDITS");

			float XSPREAD = 80;

			tmd.scale = 0.25f;

			float y = tmd.coord.y - LH*4;

			MakeCreditPart(0,			y, "Designed & Developed by", "Brian Greenstone & Toucan Studio, Inc.", "");

			y -= LH*4;

			MakeCreditPart(-XSPREAD,	y, "Programming", "Brian Greenstone", "Pangea Software");
			MakeCreditPart(XSPREAD,		y, "Art Direction", "Scott Harper", "Toucan Studio, Inc.");

			y -= LH*4;

			MakeCreditPart(-XSPREAD,	y, "Musical Direction", "Mike Beckett", "Nuclear Kangaroo Music");
			MakeCreditPart(XSPREAD,		y, "Enhanced Update", "Iliyas Jorio", "github.com/jorio/bugdom");

			tmd.coord.x = 0;
			tmd.coord.y = -110;
			tmd.scale *= .66f;
			tmd.color = kDimmedColor;
			TextMesh_Create(&tmd, "Copyright 1999 Pangea Software, Inc.");
			tmd.coord.y -= LH * .66f;
			TextMesh_Create(&tmd, "''Bugdom'' is a registered trademark of Pangea Software, Inc.");

			break;
		}

		case 1:
		{
			TextMesh_Create(&tmd, "CONTROLS");

			FSSpec diagramSpec;
			FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Images:GamepadDiagram.tga", &diagramSpec);
			TQ3SurfaceShaderObject diagramSurfaceShader = QD3D_TGAToTexture(&diagramSpec);
			GAME_ASSERT(diagramSurfaceShader);

			float y = 25;

			gNewObjectDefinition.coord.x = 0;
			gNewObjectDefinition.coord.y = y;
			gNewObjectDefinition.coord.z = -3;
			gNewObjectDefinition.flags 	= STATUS_BIT_NOTRICACHE|STATUS_BIT_NULLSHADER|STATUS_BIT_NOZWRITE;
			gNewObjectDefinition.slot 	= kAboutScreenObjNodeSlot;
			gNewObjectDefinition.moveCall = nil;
			gNewObjectDefinition.rot 	= 0;
			gNewObjectDefinition.scale = 50.0f;
			MakeNewDisplayGroupObject_TexturedQuad(diagramSurfaceShader, 750.0f/400.0f);

			tmd.scale = 0.2f;
			tmd.align = TEXTMESH_ALIGN_LEFT;
			tmd.coord.x =  95;
			tmd.coord.y =  34+y; tmd.color = TQ3ColorRGB_FromInt(0x0599f8); TextMesh_Create(&tmd, "KICK");
			tmd.coord.y =  22+y; tmd.color = TQ3ColorRGB_FromInt(0xfff139); TextMesh_Create(&tmd, "BUDDY");
			tmd.coord.y =  10+y; tmd.color = TQ3ColorRGB_FromInt(0xdf2020); TextMesh_Create(&tmd, "MORPH");
			tmd.coord.y =  -2+y; tmd.color = TQ3ColorRGB_FromInt(0x23ab23); TextMesh_Create(&tmd, "JUMP/BOOST");
			tmd.coord.y = -24+y; tmd.color = TQ3ColorRGB_FromInt(0x7e7e7e); TextMesh_Create(&tmd, "CAMERA");
			tmd.align = TEXTMESH_ALIGN_RIGHT;
			tmd.coord.x = -95; tmd.coord.y = 10+y; TextMesh_Create(&tmd, "WALK/ROLL");

			tmd.scale = 0.2f;
			tmd.align = TEXTMESH_ALIGN_LEFT;
			tmd.coord.x = -12-60;
			tmd.coord.y -= LH*6;
			TextMesh_Create(&tmd, "MOUSE & KEYBOARD:");

#define MAKE_CONTROL_TEXT(key, caption) \
			tmd.coord.y -= 11;                   \
			tmd.coord.x = -12;		tmd.color = kNameColor;		TextMesh_Create(&tmd, key); \
			tmd.coord.x = -12-60;	tmd.color = kHeadingColor;	TextMesh_Create(&tmd, caption);

			MAKE_CONTROL_TEXT("Mouse / Arrows"			, "WALK/ROLL");
#if __APPLE__
			MAKE_CONTROL_TEXT("Left Click / Option"		, "KICK/BOOST");
			MAKE_CONTROL_TEXT("Right Click / Command"	, "JUMP");
#else
			MAKE_CONTROL_TEXT("Left Click / Ctrl"		, "KICK/BOOST");
			MAKE_CONTROL_TEXT("Right Click / Alt"		, "JUMP");
#endif
			MAKE_CONTROL_TEXT("Middle Click / Space"	, "MORPH");
			MAKE_CONTROL_TEXT("Shift"					, "AUTO-WALK");
			MAKE_CONTROL_TEXT("Tab"						, "BUDDY");
			MAKE_CONTROL_TEXT("ESC"						, "PAUSE");
#undef MAKE_CONTROL_TEXT
			break;
		}

		case 2:
		{
			TextMesh_Create(&tmd, "INFO & UPDATES");

			float y = tmd.coord.y - LH*4;

			MakeCreditPart(0, y-LH*0, "The Makers of Bugdom:", "www.pangeasoft.net", "");
			MakeCreditPart(0, y-LH*4, "Get Updates at:", "github.com/jorio/bugdom", "");

			char sdlVersionString[256];
			SDL_version compiled;
			SDL_version linked;
			SDL_VERSION(&compiled);
			SDL_GetVersion(&linked);
			snprintf(sdlVersionString, sizeof(sdlVersionString), "C:%d.%d.%d, L:%d.%d.%d, %s",
					 compiled.major, compiled.minor, compiled.patch,
					 linked.major, linked.minor, linked.patch,
					 SDL_GetPlatform());

			tmd.scale = 0.2f;
			tmd.coord.x = -100;
			tmd.coord.y = -75;
			tmd.color = kDimmedColor;
			tmd.align = TEXTMESH_ALIGN_LEFT;
#define MAKE_TECH_TEXT(key, caption) \
            tmd.coord.y -= 10; tmd.coord.x = -80; TextMesh_Create(&tmd, key);	tmd.coord.x = -30; TextMesh_Create(&tmd, caption);

			MAKE_TECH_TEXT("Game ver:",	PROJECT_VERSION);
			MAKE_TECH_TEXT("Renderer:",	(const char*)glGetString(GL_RENDERER));
			MAKE_TECH_TEXT("OpenGL:",	(const char*)glGetString(GL_VERSION));
			MAKE_TECH_TEXT("GLSL:",		(const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
			MAKE_TECH_TEXT("SDL:",		sdlVersionString);
#undef MAKE_TECH_TEXT
			break;
		}
	}
}


/******************* DRAW BONUS STUFF ********************/

static void AboutScreenDrawStuff(const QD3DSetupOutputType *setupInfo)
{
	DrawObjects(setupInfo);
	QD3D_DrawParticles(setupInfo);
#if _DEBUG
	PickableQuads_Draw(setupInfo->viewObject);
#endif
}

