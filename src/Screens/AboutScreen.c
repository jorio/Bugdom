/****************************/
/* ABOUT SCREEN             */
/* Source port addition     */
/****************************/


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

			if (GetNewKeyState(kVK_Escape)
				|| GetNewKeyState(kVK_Return)
				|| GetNewKeyState(kVK_Space)
				|| GetNewKeyState(kVK_ANSI_KeypadEnter)
				|| FlushMouseButtonPress())
			{
				break;
			}
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
	//tmd.withShadow = 0;
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
	tmd.coord.y -= LH;
	TextMesh_Create(&tmd, text2);

}

static void MakeAboutScreenObjects(int slideNumber)
{
	TextMeshDef tmd;
	TextMesh_FillDef(&tmd);
	tmd.slot = kAboutScreenObjNodeSlot;
	tmd.coord.y += 110;
	//tmd.withShadow = false;
	tmd.lowercaseScale = 1;
	tmd.characterSpacing *= 1.33f;

	const float LH = 13;

	switch (slideNumber)
	{
		case 0:
		{
			float XSPREAD = 80;

			tmd.scale = 0.66f;
			TextMesh_Create(&tmd, "CREDITS");

			tmd.scale = 0.25f;

			float y = tmd.coord.y - LH*4;

			MakeCreditPart(0,			y, "Designed and Developed by", "Brian Greenstone and Toucan Studio, Inc.", "");

			y -= LH*4;

			MakeCreditPart(-XSPREAD,	y, "Programming", "Brian Greenstone", "Pangea Software");
			MakeCreditPart(XSPREAD,		y, "Art Direction", "Scott Harper", "Toucan Studio, Inc.");

			y -= LH*4;

			MakeCreditPart(-XSPREAD,	y, "Musical Direction", "Mike Beckett", "Nuclear Kangaroo Music");
			MakeCreditPart(XSPREAD,		y, "Enhanced Update", "Iliyas Jorio", "");

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
			float XLEFT = -115;
			float XRIGHT = 25;

			tmd.color = kHeadingColor;
			tmd.scale = 0.66f;
			TextMesh_Create(&tmd, "Controls");

			tmd.coord.y -= LH*4;
			tmd.scale = 0.25f;
			tmd.align = TEXTMESH_ALIGN_LEFT;

			tmd.coord.y -= LH; tmd.coord.x = XLEFT;	tmd.color = kNameColor; TextMesh_Create(&tmd, "Mouse / Arrows");			tmd.coord.x = XRIGHT;	tmd.color = kHeadingColor; TextMesh_Create(&tmd, "Move Player");
			tmd.coord.y -= LH; tmd.coord.x = XLEFT;	tmd.color = kNameColor; TextMesh_Create(&tmd, "Left Click / Option");		tmd.coord.x = XRIGHT;	tmd.color = kHeadingColor; TextMesh_Create(&tmd, "Kick / Speed Boost");
			tmd.coord.y -= LH; tmd.coord.x = XLEFT;	tmd.color = kNameColor; TextMesh_Create(&tmd, "Right Click / Command");		tmd.coord.x = XRIGHT;	tmd.color = kHeadingColor; TextMesh_Create(&tmd, "Jump");
			tmd.coord.y -= LH; tmd.coord.x = XLEFT;	tmd.color = kNameColor; TextMesh_Create(&tmd, "Middle Click / Space");		tmd.coord.x = XRIGHT;	tmd.color = kHeadingColor; TextMesh_Create(&tmd, "Morph into ball");
			tmd.coord.y -= LH; tmd.coord.x = XLEFT;	tmd.color = kNameColor; TextMesh_Create(&tmd, "Shift");						tmd.coord.x = XRIGHT;	tmd.color = kHeadingColor; TextMesh_Create(&tmd, "Auto-Walk");
			tmd.coord.y -= LH; tmd.coord.x = XLEFT;	tmd.color = kNameColor; TextMesh_Create(&tmd, "Tab");						tmd.coord.x = XRIGHT;	tmd.color = kHeadingColor; TextMesh_Create(&tmd, "Buddy Bug Onslaught");
			tmd.coord.y -= LH; tmd.coord.x = XLEFT;	tmd.color = kNameColor; TextMesh_Create(&tmd, "ESC");						tmd.coord.x = XRIGHT;	tmd.color = kHeadingColor; TextMesh_Create(&tmd, "Pause");

			tmd.align = TEXTMESH_ALIGN_CENTER;
			tmd.color = kDimmedColor;
			tmd.coord.x = 0;
			tmd.coord.y = -110;
			tmd.scale *= .66f;
			TextMesh_Create(&tmd, "CONTROL REMAPPING WILL BE IMPLEMENTED IN A");
			tmd.coord.y -= LH*.66f; TextMesh_Create(&tmd, "LATER VERSION OF THIS SOURCE PORT. STAY TUNED!");
			break;
		}

		case 2:
		{
			tmd.color = kHeadingColor;
			tmd.scale = 0.66f;
			TextMesh_Create(&tmd, "INFO AND UPDATES");

			float y = tmd.coord.y - LH*4;

			MakeCreditPart(0, y-LH*0, "The Makers of Bugdom:", "www.pangeasoft.net", "");
			MakeCreditPart(0, y-LH*4, "Get Updates at:", "github.com/jorio/bugdom", "");

			tmd.scale = 0.2f;
			tmd.coord.x = -100;
			tmd.coord.y = -75;
			tmd.color = kDimmedColor;
			tmd.align = TEXTMESH_ALIGN_LEFT;
			tmd.coord.y -= 10; tmd.coord.x = -80; TextMesh_Create(&tmd, "Game ver:");	tmd.coord.x = -30; TextMesh_Create(&tmd, PROJECT_VERSION);
			tmd.coord.y -= 10; tmd.coord.x = -80; TextMesh_Create(&tmd, "Renderer:");	tmd.coord.x = -30; TextMesh_Create(&tmd, (const char*)glGetString(GL_RENDERER));
			tmd.coord.y -= 10; tmd.coord.x = -80; TextMesh_Create(&tmd, "OpenGL:");		tmd.coord.x = -30; TextMesh_Create(&tmd, (const char*)glGetString(GL_VERSION));
			tmd.coord.y -= 10; tmd.coord.x = -80; TextMesh_Create(&tmd, "GLSL:");		tmd.coord.x = -30; TextMesh_Create(&tmd, (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

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

