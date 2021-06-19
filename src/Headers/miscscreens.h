//
// miscscreens.h
//

void DoPaused(void);
Boolean DoLevelCheatDialog(void);
void ShowLevelIntroScreen(void);
void DoSettingsScreen(void);
void DoAboutScreens(void);
void DoPangeaLogo(void);
void DoWinScreen(void);
void DoLoseScreen(void);
void DoModelDebug(void);
void DoTitleScreen(void);



// UIRoutines.c
void SetupUIStuff(void);
void CleanupUIStuff(void);
void MakeSpiderButton(TQ3Point3D coord, const char* caption, int pickID);
int UpdateHoveredPickID(void);
void NukeObjectsInSlot(u_short objNodeSlotID);
