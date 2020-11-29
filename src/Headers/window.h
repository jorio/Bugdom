//
// window.h
//

#define GAME_VIEW_WIDTH		(640)
#define GAME_VIEW_HEIGHT	(480)




#if 0  // Source port removal
extern	DSpContextReference 	gDisplayContext;
#endif




extern void	InitWindowStuff(void);
extern void	DoLockPixels(GWorldPtr);
extern	void MakeFadeEvent(Boolean	fadeIn);
void DumpGWorld2(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *destRect);

extern	void CleanupDisplay(void);
extern	void GammaFadeOut(void);
extern	void GammaFadeIn(void);
extern	void GammaOn(void);

extern	void GameScreenToBlack(void);
extern	void CleanScreenBorder(void);

void DoLockPixels(GWorldPtr world);
void DumpGWorldTransparent(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *destRect);
void DumpGWorld3(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *srcRect, Rect *destRect);


void QD3D_OnWindowResized(int windowWidth, int windowHeight);
void SetFullscreenMode(void);

