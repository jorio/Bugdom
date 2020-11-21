//
// window.h
//

#define GAME_VIEW_WIDTH		(640)
#define GAME_VIEW_HEIGHT	(480)




#if 0  // Source port removal
extern	DSpContextReference 	gDisplayContext;
#endif




extern void	InitWindowStuff(void);
extern void	DumpGWorld(GWorldPtr, WindowPtr);
extern void	DumpGWorld2(GWorldPtr, WindowPtr, Rect *);
extern void	DumpGWorldToGWorld(GWorldPtr, GWorldPtr, Rect *, Rect *);
extern void	DoLockPixels(GWorldPtr);
extern	void MakeFadeEvent(Boolean	fadeIn);
void DumpGWorld(GWorldPtr thisWorld, WindowPtr thisWindow);
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

