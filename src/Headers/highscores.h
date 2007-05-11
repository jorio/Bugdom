//
// highscores.h
//

/******************* HIGHSCORES ITEMS *************************/

enum
{
	SCORES_ObjType_Vine,
	SCORES_ObjType_Background,
	SCORES_ObjType_NameFrame,
	SCORES_ObjType_0,
	SCORES_ObjType_1,
	SCORES_ObjType_2,
	SCORES_ObjType_3,
	SCORES_ObjType_4,
	SCORES_ObjType_5,
	SCORES_ObjType_6,
	SCORES_ObjType_7,
	SCORES_ObjType_8,
	SCORES_ObjType_9,
	SCORES_ObjType_A,
	SCORES_ObjType_B,
	SCORES_ObjType_C,
	SCORES_ObjType_D,
	SCORES_ObjType_E,
	SCORES_ObjType_F,
	SCORES_ObjType_G,
	SCORES_ObjType_H,
	SCORES_ObjType_I,
	SCORES_ObjType_J,
	SCORES_ObjType_K,
	SCORES_ObjType_L,
	SCORES_ObjType_M,
	SCORES_ObjType_N,
	SCORES_ObjType_O,
	SCORES_ObjType_P,
	SCORES_ObjType_Q,
	SCORES_ObjType_R,
	SCORES_ObjType_S,
	SCORES_ObjType_T,
	SCORES_ObjType_U,
	SCORES_ObjType_V,
	SCORES_ObjType_W,
	SCORES_ObjType_X,
	SCORES_ObjType_Y,
	SCORES_ObjType_Z,
	SCORES_ObjType_Period,
	SCORES_ObjType_Pound,
	SCORES_ObjType_Question,
	SCORES_ObjType_Exclamation,
	SCORES_ObjType_Dash,
	SCORES_ObjType_Apostrophe,
	SCORES_ObjType_Colon,
	SCORES_ObjType_Cursor
};


enum
{
	SCORES_GroupNum_NameFrame = MODEL_GROUP_HIGHSCORES
};



//=================================================================

void ShowHighScoresScreen(unsigned long newScore);
void ClearHighScores(void);
