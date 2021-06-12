//
// window.h
//

#define GAME_VIEW_WIDTH		(640)
#define GAME_VIEW_HEIGHT	(480)

extern void	InitWindowStuff(void);
extern	void MakeFadeEvent(Boolean	fadeIn);

extern	void CleanupDisplay(void);
extern	void GammaFadeOut(void);
extern	void GammaOn(void);

extern	void GameScreenToBlack(void);

void QD3D_OnWindowResized(int windowWidth, int windowHeight);
void SetFullscreenMode(void);

