//
// main.h
//

#pragma once


enum
{
	LEVEL_TYPE_LAWN,
	LEVEL_TYPE_POND,
	LEVEL_TYPE_FOREST,
	LEVEL_TYPE_HIVE,
	LEVEL_TYPE_NIGHT,
	LEVEL_TYPE_ANTHILL,
	NUM_LEVEL_TYPES
};

enum
{
	LEVEL_NUM_TRAINING,
	LEVEL_NUM_LAWN,
	LEVEL_NUM_POND,
	LEVEL_NUM_BEACH,
	LEVEL_NUM_FLIGHT,
	LEVEL_NUM_HIVE,
	LEVEL_NUM_QUEENBEE,
	LEVEL_NUM_NIGHT,
	LEVEL_NUM_ANTHILL,
	LEVEL_NUM_ANTKING,
	NUM_LEVELS
};

  
//=================================================


extern	int GameMain(void);
extern	void ToolBoxInit(void);

