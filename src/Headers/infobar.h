//
// infobar.h
//

enum
{
	UPDATE_WEAPONICON 	= (1),
	UPDATE_LIVES 		= (1<<1),
	UPDATE_HEALTH		= (1<<2),
	UPDATE_TIMER		= (1<<3),
	UPDATE_LADYBUGS		= (1<<4),
	UPDATE_HANDS		= (1<<5),
	UPDATE_BLUECLOVER 	= (1<<6),
	UPDATE_GOLDCLOVER	= (1<<7),
	UPDATE_BOSS			= (1<<8)
};


extern	void InitInfobar(void);
extern	void InitInventoryForGame(void);
void LoadInfobarArt(void);
void UpdateInfobar(void);
void LoseHealth(float amount);
void ProcessBallTimer(void);
void GetKey(long keyID);
void UseKey(long keyID);
Boolean DoWeHaveTheKey(long keyID);
void GetMoney(void);
void UseMoney(void);
Boolean DoWeHaveEnoughMoney(void);
void LoseBallTime(float amount);
void InitInventoryForArea(void);
void GetLadyBug(void);
void GetHealth(float amount);

void GetGreenClover(void);
void GetBlueClover(void);
void GetGoldClover(void);







