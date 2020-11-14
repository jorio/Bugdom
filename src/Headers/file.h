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
	short	version;			// 0xaa.bb
	short	numAnims;			// gNumAnims
	short	numJoints;			// gNumJoints
	short	num3DMFLimbs;		// gNumLimb3DMFLimbs
}SkeletonFile_Header_Type;

			/* Bone resource */
			//
			// matches BoneDefinitionType except missing 
			// point and normals arrays which are stored in other resources.
			// Also missing other stuff since arent saved anyway.
			
typedef struct
{
	long 				parentBone;			 		// index to previous bone
	unsigned char		name[32];					// text string name for bone
	TQ3Point3D			coord;						// absolute coord (not relative to parent!) 
	u_short				numPointsAttachedToBone;	// # vertices/points that this bone has
	u_short				numNormalsAttachedToBone;	// # vertex normals this bone has
	u_long				reserved[8];				// reserved for future use
}File_BoneDefinitionType;



			/* Joit */
			
typedef struct
{
	TQ3Vector3D		maxRot;						// max rot values of joint
	TQ3Vector3D		minRot;						// min rot values of joint
	long 			parentBone; 		// index to previous link joint definition
	unsigned char	name[32];						// text string name for joint
	long			limbIndex;					// index into limb list
}Joit_Rez_Type;




			/* AnHd */
			
typedef struct
{
	Str32	animName;			
	short	numAnimEvents;	
}SkeletonFile_AnimHeader_Type;


//=================================================

extern	SkeletonDefType *LoadSkeletonFile(short skeletonType);
extern	void	OpenGameFile(Str255 filename,short *fRefNumPtr, Str255 errString);
extern	OSErr LoadPrefs(PrefsType *prefBlock);
extern	void SavePrefs(PrefsType *prefs);
extern	void SaveGame(void);
extern	OSErr LoadSavedGame(void);
extern	Ptr	LoadAFile(FSSpec *fsSpec);

void LoadPlayfield(FSSpec *specPtr);

void LoadLevelArt(void);
OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld);

void GetDemoTimer(void);
void SaveDemoTimer(void);

void SetDefaultDirectory(void);
void CreatePathToDataFolder(void);


















