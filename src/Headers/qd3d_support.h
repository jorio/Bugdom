//
// qd3d_support.h
//

#pragma once

#define	MIN_FPS				9
#define	MAX_FPS				500

#define	MAX_FILL_LIGHTS		4

typedef	struct
{
	Boolean					dontClear;
	TQ3ColorRGBA			clearColor;
	Rect					paneClip;			// not pane size, but clip:  left = amount to clip off left
}QD3DViewDefType;


typedef	struct
{
	TQ3InterpolationStyle	interpolation;
	TQ3BackfacingStyle		backfacing;
	TQ3FillStyle			fill;
	Boolean					usePhong;
}QD3DStyleDefType;


typedef struct
{
	TQ3Point3D				from;
	TQ3Point3D				to;
	TQ3Vector3D				up;
	float					hither;
	float					yon;
	float					fov;
}QD3DCameraDefType;

typedef	struct
{
	Boolean			useFog;
	float			fogStart;
	float			fogEnd;
	float			fogDensity;
	short			fogMode;
	Boolean			useCustomFogColor;		// if false (by default), fog will use view clear color instead of fogColor below
	TQ3ColorRGBA	fogColor;

	float			ambientBrightness;
	TQ3ColorRGB		ambientColor;
	long			numFillLights;
	TQ3Vector3D		fillDirection[MAX_FILL_LIGHTS];
	TQ3ColorRGB		fillColor[MAX_FILL_LIGHTS];
	float			fillBrightness[MAX_FILL_LIGHTS];
}QD3DLightDefType;


		/* QD3DSetupInputType */
		
typedef struct
{
	QD3DViewDefType			view;
	QD3DStyleDefType		styles;
	QD3DCameraDefType		camera;
	QD3DLightDefType		lights;
}QD3DSetupInputType;


		/* QD3DSetupOutputType */

typedef struct
{
	Boolean					isActive;
	bool					needPaneClip;
	Rect					paneClip;			// not pane size, but clip:  left = amount to clip off left
	float					aspectRatio;
	TQ3Point3D				currentCameraCoords;
	TQ3Point3D				currentCameraLookAt;
	TQ3Vector3D				currentCameraUpVector;
	float					fov;
	float					hither,yon;
	QD3DLightDefType		lightList;			// a copy of the input light data from the SetupInputType
}QD3DSetupOutputType;


//===========================================================

extern	void QD3D_SetupWindow(QD3DSetupInputType *setupDefPtr, QD3DSetupOutputType **outputHandle);
extern	void QD3D_DisposeWindowSetup(QD3DSetupOutputType **dataHandle);
extern	void QD3D_UpdateCameraFromTo(QD3DSetupOutputType *setupInfo, TQ3Point3D *from, TQ3Point3D *to);
extern	void QD3D_DrawScene(QD3DSetupOutputType *setupInfo, void (*drawRoutine)(const QD3DSetupOutputType *));
extern	void QD3D_UpdateCameraFrom(QD3DSetupOutputType *setupInfo, TQ3Point3D *from);
extern	void QD3D_MoveCameraFromTo(QD3DSetupOutputType *setupInfo, TQ3Vector3D *moveVector, TQ3Vector3D *lookAtVector);
extern	void	QD3D_CalcFramesPerSecond(void);
GLuint QD3D_LoadTextureFile(int textureRezID, int flags);
void QD3D_NewViewDef(QD3DSetupInputType *viewDef);

void ShowNormal(TQ3Point3D *where, TQ3Vector3D *normal);
void DrawNormal(void);

void QD3D_UpdateDebugTextMesh(const char* text);
void QD3D_DrawDebugTextMesh(void);

void QD3D_DrawPillarbox(void);

#define TQ3ColorRGB_FromInt(c) (TQ3ColorRGB){ (((c)>>16)&0xFF)/255.0f, (((c)>>8)&0xFF)/255.0f, ((c)&0xFF)/255.0f }
#define TQ3ColorRGBA_FromInt(c) (TQ3ColorRGBA){ (((c)>>24)&0xFF)/255.0f, (((c)>>16)&0xFF)/255.0f, (((c)>>8)&0xFF)/255.0f, ((c)&0xFF)/255.0f }

