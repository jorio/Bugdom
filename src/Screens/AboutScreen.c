// ABOUT SCREEN.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "version.h"
#include <stdio.h>


/****************************/
/*    PROTOTYPES            */
/****************************/

static void AboutScreenDrawStuff(const QD3DSetupOutputType *setupInfo);
static void MakeAboutScreenObjects(int slideNumber);


/****************************/
/*    CONSTANTS             */
/****************************/

static const TQ3ColorRGBA kNameColor		= {0.6f, 1.0f, 0.8f, 1.0f};
#if OSXPPC
static const TQ3ColorRGBA kMouseColor		= {0,0,0,0};
#else
static const TQ3ColorRGBA kMouseColor		= {0.6f, 1.0f, 0.8f, 1.0f};
#endif
static const TQ3ColorRGBA kHeadingColor		= {1.0f, 1.0f, 1.0f, 1.0f};
static const TQ3ColorRGBA kDimmedColor		= {0.6f, 0.6f, 0.6f, 1.0f};

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

	SetupUIStuff(kUIBackground_Black);
	ShutdownAnalogCursor();
	QD3D_CalcFramesPerSecond();

	/**************/
	/* PROCESS IT */
	/**************/

	MakeFadeEvent(true);

	for (int i = 0; i < 3; i++)
	{
#if NOJOYSTICK
		// Skip gamepad slide
		if (i == 1)
			i++;
#endif

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
	tmd.align = TEXTMESH_ALIGN_CENTER;

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
	tmd.align = TEXTMESH_ALIGN_CENTER;
	tmd.slot = kAboutScreenObjNodeSlot;
	tmd.coord.y += 110;
	tmd.withShadow = false;
	tmd.color = kHeadingColor;

	const float LH = 13;

	switch (slideNumber)
	{
		case 0:
		{
			TextMesh_Create(&tmd, "Credits");

			float XSPREAD = 65;

			tmd.scale = 0.25f;

			float y = tmd.coord.y - LH*4;

			MakeCreditPart(0,			y, "Designed & Developed by", "Brian Greenstone & Toucan Studio, Inc.", "");

			y -= LH*4;

			MakeCreditPart(-XSPREAD,	y, "Programming", "Brian Greenstone", "Pangea Software");
			MakeCreditPart(XSPREAD,		y, "Art Direction", "Scott Harper", "Toucan Studio, Inc.");

			y -= LH*4;

			MakeCreditPart(-XSPREAD,	y, "Musical Direction", "Mike Beckett", "Nuclear Kangaroo Music");
			MakeCreditPart(XSPREAD,		y, "Enhanced Update", "Iliyas Jorio", "github.com/jorio");

			break;
		}

		case 1:
		{
			TextMesh_Create(&tmd, "Gamepad Controls");

			GLuint diagramTexture = QD3D_LoadTextureFile(3500, kRendererTextureFlags_ClampBoth | kRendererTextureFlags_SolidBlackIsAlpha);

			float y = -10;

			gNewObjectDefinition.genre		= DISPLAY_GROUP_GENRE;
			gNewObjectDefinition.group 		= MODEL_GROUP_ILLEGAL;
			gNewObjectDefinition.coord.x = 0;
			gNewObjectDefinition.coord.y = y;
			gNewObjectDefinition.coord.z = 0;
			gNewObjectDefinition.flags 	= STATUS_BIT_NULLSHADER|STATUS_BIT_NOZWRITE;
			gNewObjectDefinition.slot 	= kAboutScreenObjNodeSlot;
			gNewObjectDefinition.moveCall = nil;
			gNewObjectDefinition.rot 	= 0;
			gNewObjectDefinition.scale = 2*50.0f;
			ObjNode* diagramNode = MakeNewObject(&gNewObjectDefinition);

			TQ3TriMeshData* diagramQuad = MakeQuadMesh(1, 754.0f/400.0f, 400.0f/400.0f);
			diagramQuad->texturingMode = kQ3TexturingModeAlphaTest;
			diagramQuad->glTextureName = diagramTexture;
			AttachGeometryToDisplayGroupObject(diagramNode, 1, &diagramQuad,
					kAttachGeometry_TransferMeshOwnership | kAttachGeometry_TransferTextureOwnership);

			UpdateObjectTransforms(diagramNode);

			tmd.scale = 0.2f;
			tmd.align = TEXTMESH_ALIGN_LEFT;
			tmd.coord.x =  95;
			tmd.coord.y =  34+y; tmd.color = TQ3ColorRGBA_FromInt(0x0599f8ff); TextMesh_Create(&tmd, "Kick");
			tmd.coord.y =  22+y; tmd.color = TQ3ColorRGBA_FromInt(0xfff139ff); TextMesh_Create(&tmd, "Buddy Bug");
			tmd.coord.y =  10+y; tmd.color = TQ3ColorRGBA_FromInt(0xdf2020ff); TextMesh_Create(&tmd, "Morph");
			tmd.coord.y =  -2+y; tmd.color = TQ3ColorRGBA_FromInt(0x23ab23ff); TextMesh_Create(&tmd, "Jump/Boost");
			tmd.coord.y = -24+y; tmd.color = TQ3ColorRGBA_FromInt(0xFFFFFFff); TextMesh_Create(&tmd, "Camera");
			tmd.align = TEXTMESH_ALIGN_RIGHT;
			tmd.coord.x = -95; tmd.coord.y = 10+y; TextMesh_Create(&tmd, "Walk/Roll");
			tmd.align = TEXTMESH_ALIGN_CENTER;
			tmd.coord.x = -40; tmd.coord.y = 57+y; tmd.color = TQ3ColorRGBA_FromInt(0x3e4642ff); TextMesh_Create(&tmd, "Zoom in");
			tmd.coord.x =  40; tmd.coord.y = 57+y; tmd.color = TQ3ColorRGBA_FromInt(0x3e4642ff); TextMesh_Create(&tmd, "Zoom out");
			break;
		}

		case 2:
		{
#if OSXPPC
			TextMesh_Create(&tmd, "Keyboard Controls");
#else
			TextMesh_Create(&tmd, "Mouse & Keyboard Controls");
#endif

			tmd.scale = 0.2f;
			tmd.align = TEXTMESH_ALIGN_LEFT;
			float x = 0;
#if OSXPPC
			x = 30;
			tmd.coord.x = 30;
			tmd.coord.y = 75;
#else
			tmd.coord.x = -20;
			tmd.coord.y = 75;
#endif

#define MAKE_CONTROL_TEXT(key, mouse, caption) \
			tmd.coord.y -= 14;                   \
			tmd.coord.x = x-12;		tmd.color = kNameColor;		TextMesh_Create(&tmd, key); \
			tmd.coord.x = x-12+40;	tmd.color = kMouseColor;	TextMesh_Create(&tmd, mouse); \
			tmd.coord.x = x-12-80;	tmd.color = kHeadingColor;	TextMesh_Create(&tmd, caption);

#if !OSXPPC
			MAKE_CONTROL_TEXT("Shift (when using mouse)"		, ""				, "Auto-Walk");
#endif
			MAKE_CONTROL_TEXT("Arrows"		, "or     Mouse"	, "Walk/Roll");
#if __APPLE__
			MAKE_CONTROL_TEXT("Option"		, "or     Left click"		, "Kick/Boost");
			MAKE_CONTROL_TEXT("Command"		, "or     Right click"		, "Jump");
#else
			MAKE_CONTROL_TEXT("Ctrl"		, "or     Left click"		, "Kick/Boost");
			MAKE_CONTROL_TEXT("Alt"			, "or     Right click"		, "Jump");
#endif
			MAKE_CONTROL_TEXT("Space"		, "or     Middle click"	, "Morph");
			MAKE_CONTROL_TEXT("Tab"			, ""				, "Buddy Bug");
			MAKE_CONTROL_TEXT("< / >"		, ""				, "Turn Camera");
			MAKE_CONTROL_TEXT("1 / 2"		, ""				, "Zoom in/out");
			MAKE_CONTROL_TEXT("ESC"			, ""				, "Pause");
#undef MAKE_CONTROL_TEXT

			tmd.coord.y -= 45;

#if !OSXPPC
			tmd.align = TEXTMESH_ALIGN_CENTER;
			tmd.coord.x = 0;
			tmd.color = TQ3ColorRGBA_FromInt(0xE0B000FF);
			TextMesh_Create(&tmd, "We strongly recommend using the mouse and the shift key for motion.");
			tmd.coord.y -= 14;
			TextMesh_Create(&tmd, "This combo gives you the most accurate control over the player.");
#endif
			break;
		}
	}
}


/******************* DRAW BONUS STUFF ********************/

static void AboutScreenDrawStuff(const QD3DSetupOutputType *setupInfo)
{
	DrawObjects(setupInfo);
	QD3D_DrawParticles(setupInfo);
}


#pragma mark -

/******************* LEGAL SPLASH SCREEN ********************/

static void MakeLegalScreenObjects(void)
{
	const float LH = 13;

	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);
	tmd.align = TEXTMESH_ALIGN_CENTER;
	tmd.slot = kAboutScreenObjNodeSlot;
	tmd.coord.y = 66;
	tmd.withShadow = false;
	tmd.color = kNameColor;
	tmd.scale = 0.3f;
	//tmd.coord.y -= LH * 4;

	TextMesh_Create(&tmd, "Bugdom " PROJECT_VERSION);

	tmd.scale = 0.2f;
	tmd.coord.y = LH/2;
	TextMesh_Create(&tmd, "pangeasoft.net/bug");
	tmd.coord.y -= LH;
	TextMesh_Create(&tmd, "jorio.itch.io/bugdom");

	tmd.coord.y = -66;
	tmd.scale *= .66f;
	tmd.color = kDimmedColor;
	TextMesh_Create(&tmd, "Original game: \251 1999 Pangea Software, Inc.   Modern version: \251 2023 Iliyas Jorio.");
	tmd.coord.y -= LH * .66f;
	TextMesh_Create(&tmd, "\223Bugdom\224 is a registered trademark of Pangea Software, Inc.");
}

void DoLegalScreen(void)
{
	/*********/
	/* SETUP */
	/*********/

	SetupUIStuff(kUIBackground_Black);

	NukeObjectsInSlot(kAboutScreenObjNodeSlot);		// nuke text from previous slide
	MakeLegalScreenObjects();

	QD3D_CalcFramesPerSecond();
	
	MakeFadeEvent(true);
	FlushMouseButtonPress();

	/**************/
	/* PROCESS IT */
	/**************/

	float timeout = 8.0f;

	while (timeout > 0)
	{
		UpdateInput();
		MoveObjects();
		QD3D_MoveParticles();
		QD3D_DrawScene(gGameViewInfoPtr, AboutScreenDrawStuff);
		QD3D_CalcFramesPerSecond();
		DoSDLMaintenance();

		if (GetSkipScreenInput())
			break;

		timeout -= gFramesPerSecondFrac;
	}

	/* CLEANUP */

	CleanupUIStuff();
}
