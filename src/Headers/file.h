//
// file.h
//
#pragma once

		/***********************/
		/* RESOURCE STURCTURES */
		/***********************/
		
			/* Hedr */
			
typedef struct
{
	int16_t	version;			// 0xaa.bb
	int16_t	numAnims;			// gNumAnims
	int16_t	numJoints;			// gNumJoints
	int16_t	num3DMFLimbs;		// gNumLimb3DMFLimbs
}SkeletonFile_Header_Type;

			/* Bone resource */
			//
			// matches BoneDefinitionType except missing 
			// point and normals arrays which are stored in other resources.
			// Also missing other stuff since arent saved anyway.
			
typedef struct
{
	int32_t 			parentBone;			 		// index to previous bone
	char				name[32];					// text string name for bone
	TQ3Point3D			coord;						// absolute coord (not relative to parent!) 
	uint16_t			numPointsAttachedToBone;	// # vertices/points that this bone has
	uint16_t			numNormalsAttachedToBone;	// # vertex normals this bone has
	uint32_t			reserved[8];				// reserved for future use
}File_BoneDefinitionType;



			/* AnHd */
			
typedef struct
{
	uint8_t nameLength;
	char	name[32];
	int16_t	numAnimEvents;
}SkeletonFile_AnimHeader_Type;


//=================================================

			/* SAVE FILE SLOTS */

#define NUM_SAVE_FILES 3

			/* SAVE GAME */

typedef struct
{
	uint32_t	version;
	uint32_t	score;
	int64_t		timestamp;
	float		health;
	float		ballTimer;
	uint8_t		realLevel;
	uint8_t		numLives;
	uint8_t		numGoldClovers;
} SaveGameType;

//=================================================

extern	SkeletonDefType *LoadSkeletonFile(short skeletonType);
extern	void	OpenGameFile(Str255 filename,short *fRefNumPtr, Str255 errString);
extern	OSErr LoadPrefs(PrefsType *prefBlock);
extern	void SavePrefs(PrefsType *prefs);
extern	void SaveGame(int slot);
extern	OSErr GetSaveGameData(int slot, SaveGameType* saveData);
extern	OSErr LoadSavedGame(int slot);
extern	OSErr DeleteSavedGame(int slot);

void LoadPlayfield(FSSpec *specPtr);

void LoadLevelArt(void);



















