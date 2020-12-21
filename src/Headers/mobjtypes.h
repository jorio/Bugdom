//
// mobjtypes.h
//

enum
{
	MODEL_GROUP_LEVELSPECIFIC =	0,
	MODEL_GROUP_TITLE		=	1,
	MODEL_GROUP_MENU		=	1,
	MODEL_GROUP_BONUS		=	1,
	MODEL_GROUP_PANGEA		=	1,
	MODEL_GROUP_LEVELINTRO	=	1,
	MODEL_GROUP_HIGHSCORES 	=	1,
	MODEL_GROUP_GLOBAL1		=	2,
	MODEL_GROUP_GLOBAL2		=	3,
	MODEL_GROUP_LEVELSPECIFIC2 = 4,
	MODEL_GROUP_TEXTMESH	= 5,
};

#define	MAX_3DMF_GROUPS			6
#define	MAX_OBJECTS_IN_GROUP	64



/******************* LAWN LEVEL *************************/

enum
{
	LAWN1_MObjType_Cyc,

	LAWN1_MObjType_Door_Green,
	LAWN1_MObjType_Door_Teal,
	LAWN1_MObjType_Door_Red,
	LAWN1_MObjType_Door_Orange,
	LAWN1_MObjType_Door_Purple,
	LAWN1_MObjType_Key_Green,
	LAWN1_MObjType_Key_Teal,
	LAWN1_MObjType_Key_Red,
	LAWN1_MObjType_Key_Orange,
	LAWN1_MObjType_Key_Purple,

	LAWN1_MObjType_WaterFaucet
};

enum
{
	LAWN1_MGroupNum_Cyc = MODEL_GROUP_LEVELSPECIFIC,
	LAWN1_MGroupNum_Door = MODEL_GROUP_LEVELSPECIFIC
};


enum
{
	LAWN2_MObjType_Grass,
	LAWN2_MObjType_Grass2,
	LAWN2_MObjType_Weed,
	LAWN2_MObjType_Cosmo,
	LAWN2_MObjType_Poppy,
	LAWN2_MObjType_Sunflower,
	LAWN2_MObjType_Clover,
	LAWN2_MObjType_Clover2,
	
	LAWN2_MObjType_Rock1,
	LAWN2_MObjType_Rock2
};

enum
{
	LAWN2_MGroupNum_Clover = MODEL_GROUP_LEVELSPECIFIC2,
	LAWN2_MGroupNum_Grass = MODEL_GROUP_LEVELSPECIFIC2,
	LAWN2_MGroupNum_Weed = MODEL_GROUP_LEVELSPECIFIC2,
	LAWN2_MGroupNum_Cosmo = MODEL_GROUP_LEVELSPECIFIC2,
	LAWN2_MGroupNum_Poppy = MODEL_GROUP_LEVELSPECIFIC2,
	LAWN2_MGroupNum_Sunflower = MODEL_GROUP_LEVELSPECIFIC2
};


/******************* WATER LEVEL *************************/

enum
{
	POND_MObjType_CatTail,
	POND_MObjType_DuckWeed,
	POND_MObjType_LilyFlower,
	POND_MObjType_LilyPad,
	POND_MObjType_PondGrass,
	POND_MObjType_PondGrass2,
	POND_MObjType_PondGrass3,
	POND_MObjType_Reed,
	POND_MObjType_Reed2,
	POND_MObjType_Money,
	POND_MObjType_Dock,
	POND_MObjType_Caption
};

enum
{
	POND_MGroupNum_CatTail = MODEL_GROUP_LEVELSPECIFIC,
	POND_MGroupNum_DuckWeed = MODEL_GROUP_LEVELSPECIFIC,
	POND_MGroupNum_LilyFlower = MODEL_GROUP_LEVELSPECIFIC,
	POND_MGroupNum_LilyPad = MODEL_GROUP_LEVELSPECIFIC,
	POND_MGroupNum_PondGrass = MODEL_GROUP_LEVELSPECIFIC,
	POND_MGroupNum_Reed = MODEL_GROUP_LEVELSPECIFIC,
	POND_MGroupNum_Money = MODEL_GROUP_LEVELSPECIFIC,
	POND_MGroupNum_Dock = MODEL_GROUP_LEVELSPECIFIC
};


/******************* FOREST LEVEL *************************/

enum
{
	FOREST_MObjType_Cyc,
	FOREST_MObjType_Tree,
	FOREST_MObjType_Grass,
	FOREST_MObjType_Thread,
	FOREST_MObjType_WebBullet,
	FOREST_MObjType_WebSphere,
	FOREST_MObjType_Stump,
	FOREST_MObjType_Hive,
	FOREST_MObjType_Thorn1,
	FOREST_MObjType_Thorn2,
	FOREST_MObjType_FlatRock,
	FOREST_MObjType_Rock,
	FOREST_MObjType_WoodPost
	
};

enum
{
	FOREST_MGroupNum_Cyc = MODEL_GROUP_LEVELSPECIFIC,
	FOREST_MGroupNum_Tree = MODEL_GROUP_LEVELSPECIFIC,
	FOREST_MGroupNum_Thread = MODEL_GROUP_LEVELSPECIFIC,
	FOREST_MGroupNum_WebBullet = MODEL_GROUP_LEVELSPECIFIC,
	FOREST_MGroupNum_WebSphere = MODEL_GROUP_LEVELSPECIFIC,
	FOREST_MGroupNum_Thorn = MODEL_GROUP_LEVELSPECIFIC
};


/******************* BEE HIVE LEVEL *************************/

enum
{
	HIVE_MObjType_BrickPlatform,
	HIVE_MObjType_SteelPlatform,
	HIVE_MObjType_WoodPlatform,
	HIVE_MObjType_Firecracker,
	HIVE_MObjType_DetonatorGreen,
	HIVE_MObjType_DetonatorOrange,
	HIVE_MObjType_DetonatorPurple,
	HIVE_MObjType_DetonatorRed,
	HIVE_MObjType_DetonatorTeal,
	HIVE_MObjType_Plunger,
	
	HIVE_MObjType_HiveDoor_Green,
	HIVE_MObjType_HiveDoor_Orange,
	HIVE_MObjType_HiveDoor_Purple,
	HIVE_MObjType_HiveDoor_Red,
	HIVE_MObjType_HiveDoor_Teal,
	
	HIVE_MObjType_HiveDoor_GreenOpen,
	HIVE_MObjType_HiveDoor_OrangeOpen,
	HIVE_MObjType_HiveDoor_PurpleOpen,
	HIVE_MObjType_HiveDoor_RedOpen,
	HIVE_MObjType_HiveDoor_TealOpen,
	
	HIVE_MObjType_BentTube,
	HIVE_MObjType_SquiggleTube,
	HIVE_MObjType_StraightTube,
	HIVE_MObjType_TaperTube,
	
	HIVE_MObjType_HoneyBlob,
	HIVE_MObjType_FloorSpike,
	
	HIVE_MObjType_Stinger
};

enum
{
	HIVE_MGroupNum_Platform = MODEL_GROUP_LEVELSPECIFIC,
	HIVE_MGroupNum_Firecracker = MODEL_GROUP_LEVELSPECIFIC,
	HIVE_MGroupNum_DetonatorBox = MODEL_GROUP_LEVELSPECIFIC,
	HIVE_MGroupNum_Plunger = MODEL_GROUP_LEVELSPECIFIC,
	HIVE_MGroupNum_HiveDoor = MODEL_GROUP_LEVELSPECIFIC
};



/******************* NIGHT LEVEL *************************/

enum
{
	NIGHT_MObjType_Cyc,
	NIGHT_MObjType_FireFlyGlow,
	NIGHT_MObjType_FlatRock,
	NIGHT_MObjType_Rock,
	NIGHT_MObjType_Thread,
	NIGHT_MObjType_WebBullet,
	NIGHT_MObjType_WebSphere,
	NIGHT_MObjType_CherryBomb,
	NIGHT_MObjType_Firecracker,
	NIGHT_MObjType_Grass,
	NIGHT_MObjType_Grass2,
	NIGHT_MObjType_GlowShadow,
	NIGHT_MObjType_GasCloud,

	NIGHT_MObjType_Door_Green,
	NIGHT_MObjType_Door_Teal,
	NIGHT_MObjType_Door_Red,
	NIGHT_MObjType_Door_Orange,
	NIGHT_MObjType_Door_Purple,
	NIGHT_MObjType_Key_Green,
	NIGHT_MObjType_Key_Teal,
	NIGHT_MObjType_Key_Red,
	NIGHT_MObjType_Key_Orange,
	NIGHT_MObjType_Key_Purple

};

enum
{
	NIGHT_MGroupNum_FireFlyGlow = MODEL_GROUP_LEVELSPECIFIC,
	NIGHT_MGroupNum_FlatRock = MODEL_GROUP_LEVELSPECIFIC,
	NIGHT_MGroupNum_Rock = MODEL_GROUP_LEVELSPECIFIC
};


/******************* ANTHILL LEVEL *************************/

enum
{
	ANTHILL_MObjType_WaterValveBox,
	ANTHILL_MObjType_WaterValveHandle,
	ANTHILL_MObjType_GasCloud,
	ANTHILL_MObjType_BentPipe,
	ANTHILL_MObjType_HorizPipe,
	ANTHILL_MObjType_KingPipe,
	ANTHILL_MObjType_Staff
};

enum
{
	ANTHILL_MGroupNum_WaterValve = MODEL_GROUP_LEVELSPECIFIC
};


/******************* GLOBAL 1 *************************/

enum
{
	GLOBAL1_MObjType_Shadow,
	GLOBAL1_MObjType_ShockWave,
	GLOBAL1_MObjType_Nut,
	GLOBAL1_MObjType_Ripple,
	GLOBAL1_MObjType_WallEnd,
	GLOBAL1_MObjType_Spear,
	GLOBAL1_MObjType_ThrowRock,
	GLOBAL1_MObjType_Straw,
	GLOBAL1_MObjType_Droplet,
	GLOBAL1_MObjType_LadyBugCage,
	GLOBAL1_MObjType_LadyBugPost
};

enum
{
	GLOBAL1_MGroupNum_Shadow = MODEL_GROUP_GLOBAL1,
	GLOBAL1_MGroupNum_ShockWave = MODEL_GROUP_GLOBAL1,
	GLOBAL1_MGroupNum_Nut = MODEL_GROUP_GLOBAL1,
	GLOBAL1_MGroupNum_Ripple = MODEL_GROUP_GLOBAL1,
	GLOBAL1_MGroupNum_WallEnd = MODEL_GROUP_GLOBAL1,
	GLOBAL1_MGroupNum_Spear = MODEL_GROUP_GLOBAL1,
	GLOBAL1_MGroupNum_Rock = MODEL_GROUP_GLOBAL1,
	GLOBAL1_MGroupNum_Straw = MODEL_GROUP_GLOBAL1,
	GLOBAL1_MGroupNum_Droplet = MODEL_GROUP_GLOBAL1,
	GLOBAL1_MGroupNum_LadyBug = MODEL_GROUP_GLOBAL1
};


/******************* GLOBAL 2 *************************/

enum
{
	GLOBAL2_MObjType_Berry,
	GLOBAL2_MObjType_ExitLog,
	GLOBAL2_MObjType_BallTimePOW,
	GLOBAL2_MObjType_FreeLifePOW,
	GLOBAL2_MObjType_ShieldPOW,
	GLOBAL2_MObjType_GoldClover,
	GLOBAL2_MObjType_GreenClover,
	GLOBAL2_MObjType_BlueClover,
	GLOBAL2_MObjType_Tick,
};


enum
{
	GLOBAL2_MGroupNum_BallTimePOW = MODEL_GROUP_GLOBAL2,
	GLOBAL2_MGroupNum_GreenCloverPOW = MODEL_GROUP_GLOBAL2,
	GLOBAL2_MGroupNum_BlueCloverPOW = MODEL_GROUP_GLOBAL2,
	GLOBAL2_MGroupNum_GoldCloverPOW = MODEL_GROUP_GLOBAL2,
	GLOBAL2_MGroupNum_LifePOW = MODEL_GROUP_GLOBAL2,
	GLOBAL2_MGroupNum_BerryPOW = MODEL_GROUP_GLOBAL2,
	GLOBAL2_MGroupNum_ShieldPOW = MODEL_GROUP_GLOBAL2,
	GLOBAL2_MGroupNum_Shield = MODEL_GROUP_GLOBAL2,
	GLOBAL2_MGroupNum_Tick = MODEL_GROUP_GLOBAL2,
	GLOBAL2_MGroupNum_ExitLog = MODEL_GROUP_GLOBAL2
};


/******************* BONUS SCREEN *************************/


enum
{
	BONUS_MObjType_0,
	BONUS_MObjType_1,
	BONUS_MObjType_2,
	BONUS_MObjType_3,
	BONUS_MObjType_4,
	BONUS_MObjType_5,
	BONUS_MObjType_6,
	BONUS_MObjType_7,
	BONUS_MObjType_8,
	BONUS_MObjType_9,
	BONUS_MObjType_Background,
	BONUS_MObjType_Text,
	BONUS_MObjType_Score,
	BONUS_MObjType_SaveIcon,
	BONUS_MObjType_DontSaveIcon,
	BONUS_MObjType_GreenClover,
	BONUS_MObjType_GoldClover,
	BONUS_MObjType_BlueClover
};


