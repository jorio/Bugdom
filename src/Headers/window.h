//
// window.h
//

#define GAME_VIEW_WIDTH		(640)
#define GAME_VIEW_HEIGHT	(480)

extern void	InitWindowStuff(void);
extern	void MakeFadeEvent(Boolean	fadeIn);

void GammaFadeOut(Boolean fadeSound);
extern	void GammaOn(void);

extern	void GameScreenToBlack(void);

void QD3D_OnWindowResized(int windowWidth, int windowHeight);

void DoSDLMaintenance(void);

int GetNumDisplays(void);
void GetDefaultWindowSize(SDL_DisplayID display, int* w, int* h);
void MoveToPreferredDisplay(void);
void SetFullscreenMode(bool enforceDisplayPref);
