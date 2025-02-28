//
// miscscreens.h
//

void DoPaused(void);
void ShowLevelIntroScreen(void);
void DoSettingsScreen(void);
void DoAboutScreens(void);
void DoLegalScreen(void);
void DoPangeaLogo(void);
void DoWinScreen(void);
void DoLoseScreen(void);
void DoModelDebug(void);
void DoTitleScreen(void);
bool DoLevelSelect(void);



// UIRoutines.c

enum
{
	kUIBackground_Cyclorama,
	kUIBackground_Black,
	kUIBackground_White,
};

void SetupUIStuff(int backgroundType);
void CleanupUIStuff(void);
void MakeSpiderButton(TQ3Point3D coord, const char* caption, int pickID);
int UpdateHoveredPickID(void);
void NukeObjectsInSlot(u_short objNodeSlotID);
