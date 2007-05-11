//
// camera.h
//

#define	HITHER_DISTANCE	20.0f
#define	YON_DISTANCE	2500.0f

void InitCamera(void);
void UpdateCamera(void);
extern	void CalcCameraMatrixInfo(QD3DSetupOutputType *);
extern	void ResetCameraSettings(void);
void DrawLensFlare(const QD3DSetupOutputType *setupInfo);
void DisposeLensFlares(void);


