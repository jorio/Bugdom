/****************************/
/*   	3DMF.C 		   	    */
/* (c)1996 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


extern	QD3DSetupOutputType		*gGameViewInfoPtr;

#include <string.h>				// strcasecmp
#ifdef _MSC_VER
	#define strncasecmp _strnicmp
	#define strcasecmp _stricmp
#endif


/****************************/
/*    PROTOTYPES            */
/****************************/

static TQ3Status MyRead3DMFModel(TQ3FileObject file, TQ3Object *model);
static TQ3FileObject	Create3DMFFileObject(const FSSpec *myFSSpec);

/****************************/
/*    CONSTANTS             */
/****************************/

// List of objects for which we do NOT want to discard transparent texels ("alpha test").
// By default, alpha testing is applied to all objects with transparent textures.
const struct
{
	const char*		filename;			// 3DMF file name, uppercase. Set to NULL to signify end of list.
	uint64_t		exceptionMask;		// Mask of object IDs to ignore
}
gAlphaTestBlacklist[] =
{
	{
		"GLOBAL_MODELS1.3DMF"
		, (1 << GLOBAL1_MObjType_Shadow)
		| (1 << GLOBAL1_MObjType_Ripple)
		| (1 << GLOBAL1_MObjType_ShockWave)
	},
	{
		"NIGHT_MODELS.3DMF"
		, (1 << NIGHT_MObjType_GasCloud)
		| (1 << NIGHT_MObjType_GlowShadow)
		| (1 << NIGHT_MObjType_FireFlyGlow)
	},
	{
		"ANTHILL_MODELS.3DMF"
		, (1 << ANTHILL_MObjType_GasCloud)
		| (1 << ANTHILL_MObjType_Staff)
	},
	{	// End sentinel
		nil,
		0
	}
};

/*********************/
/*    VARIABLES      */
/*********************/

FSSpec	gLast3DMF_FSSpec;

TQ3Object			gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
TQ3BoundingSphere	gObjectGroupRadiusList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
TQ3BoundingBox 		gObjectGroupBBoxList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
short				gNumObjectsInGroupList[MAX_3DMF_GROUPS];


/******************* INIT 3DMF MANAGER *************************/

void Init3DMFManager(void)
{
short	i;

	for (i=0; i < MAX_3DMF_GROUPS; i++)
		gNumObjectsInGroupList[i] = 0;

}

/*************** LOAD 3DMF MODEL **************/
//
// INPUT: inFile = FSSpec of file to load
//
// OUTPUT: nil = didnt happen
//

TQ3Object	Load3DMFModel(const FSSpec *inFile)
{
TQ3FileObject		fileObj;
TQ3Object			theModel;

	gLast3DMF_FSSpec = *inFile;								// also keep a global copy


			/* CREATE A FILE OBJECT */
				
	fileObj = Create3DMFFileObject(inFile);
	if (fileObj == nil)
		theModel = nil;
	else
	{

			/* READ THE 3DMF FILE */
			
		if (MyRead3DMFModel(fileObj, &theModel) == kQ3Failure)
			theModel = nil;
	}
	
	Q3File_Close(fileObj);
	Q3Object_Dispose(fileObj);



	return(theModel);
}


/*************** CREATE 3DMF FILE OBJECT ****************/
//
// Creates and returns a File Object which contains the 3DMF file.
//
// INPUT: myFSSpec = file to create object for
//

static TQ3FileObject	Create3DMFFileObject(const FSSpec *myFSSpec)
{
TQ3FileObject		myFileObj;
TQ3StorageObject	myStorageObj;
		
		/* CREATE NEW STORAGE OBJECT WHICH IS THE 3DMF FILE */
			
	myStorageObj = (TQ3StorageObject)Q3FSSpecStorage_New(myFSSpec);
	GAME_ASSERT(myStorageObj);

	
			/* CREATE NEW FILE OBJECT */
			
	myFileObj = (TQ3FileObject)Q3File_New();
	GAME_ASSERT(myFileObj);

			/* SET THE STORAGE FOR THE FILE OBJECT */
			
	TQ3Status status = Q3File_SetStorage(myFileObj, myStorageObj);
	GAME_ASSERT(status);

	Q3Object_Dispose(myStorageObj);
			
	return(myFileObj);
}



/***************** READ 3DMF MODEL ************************/
//
// Reads the 3DMF file and saves into new TQ3Object.
//
// INPUT:  file = File Object containing reference to 3DMF file
//
// OUTPUT: model =object containing all the important data in the 3DMF file
//
//

static TQ3Status MyRead3DMFModel(TQ3FileObject file, TQ3Object *model)
{
TQ3GroupObject	myGroup;
TQ3Object		myObject;

		/* INIT MODEL TO BE RETURNED */
		
	*model = nil;												// assume return nothing
	myGroup = myObject = nil;
	

		/* OPEN THE FILE OBJECT & EXIT GRACEFULLY IF UNSUCCESSFUL */

	if (Q3File_OpenRead(file,nil) != kQ3Success)							// open the file
		DoFatalAlert("Reading 3DMF file failed!");
	
				/**************************************/
				/* READ ALL THE OBJECTS FROM THE FILE */
				/**************************************/
	do
	{	
			/* READ A METAFILE OBJECT FROM THE FILE OBJECT */

		myObject = (TQ3Object)Q3File_ReadObject(file);
		if (myObject == nil)
		{
			if (myGroup)
				Q3Object_Dispose(myGroup);

			printf("!!! MyRead3DMFModel: Q3File_ReadObject returned nil object!\n");
			QD3D_ShowRecentError();
			break;
		}

		/* ADD ANY DRAWABLE OBJECTS TO A GROUP */
		
		if (Q3Object_IsDrawable(myObject))								// see if is a drawable object
		{
			if (myGroup)												// if group exists
				Q3Group_AddObject(myGroup,myObject);					// add object to group
			else
			if (*model == nil)											// if no model data yet / this is first
			{
				*model = myObject;										// set model to this object
				myObject = nil;											// clear this object
			}
			else														// this isn't the first & only object, so add to group
			{
				myGroup = (void *)Q3DisplayGroup_New();							// make new group
				GAME_ASSERT(myGroup);
				
				Q3Group_AddObject(myGroup,*model);						// add existing model to group
				Q3Group_AddObject(myGroup,myObject);					// add object to group
				Q3Object_Dispose(*model);								// dispose extra ref
				*model = myGroup;										// set return value to the new group
			}
		}
		
		if (myObject != nil)											// dispose extra ref
			Q3Object_Dispose(myObject);
			
	} while(!Q3File_IsEndOfFile(file));
	
	
			/* SEE IF ANYTHING WENT WRONG */
			
	if (Q3Error_Get(nil) != kQ3ErrorNone)
	{
		if (*model != nil)
		{
			Q3Object_Dispose(*model);
			*model = nil;
		}		
		return(kQ3Failure);
	}
	
	return(kQ3Success);
}
	
	
	
/******************** LOAD GROUPED 3DMF ***********************/
//
// Loads and registers a grouped 3DMF file by reading each element out of the file into
// a master object list.
//
// INPUT: fileName = name of file to load (nil = select one)
//		  groupNum = group # to assign this file.


void LoadGrouped3DMF(FSSpec *spec, Byte groupNum)
{
TQ3Object		the3DMFFile;
TQ3Uns32		nObjects;
TQ3Status 		status;
TQ3Uns32		i;
TQ3GroupPosition	position;

	GAME_ASSERT(groupNum < MAX_3DMF_GROUPS);

			/* DISPOSE OF ANY OLD OBJECTS */

#if 1
	GAME_ASSERT_MESSAGE(gNumObjectsInGroupList[groupNum] == 0, "3DMF group was not freed before reuse");
#else
	Free3DMFGroup(groupNum);
#endif

			/* LOAD NEW GEOMETRY */
			
	the3DMFFile = Load3DMFModel(spec);
	GAME_ASSERT(the3DMFFile);

			/* FIND OUT WHICH OBJECTS NEED ALPHA TEST */

	uint64_t alphaTestObjectMask = ~0u;								// by default, apply alpha test to all objects

	for (i = 0; gAlphaTestBlacklist[i].filename; i++)				// see if blacklist says any objects should NOT have alpha test
	{
		if (!strcasecmp(spec->cName, gAlphaTestBlacklist[i].filename))		// model file matches entry in blacklist
		{
			alphaTestObjectMask &= ~gAlphaTestBlacklist[i].exceptionMask;
			break;
		}
	}

			/* BUILD OBJECT LIST */
		
	status = Q3Group_CountObjects(the3DMFFile, &nObjects);			// get # objects in group.  Assume each object is a separate linked item
	GAME_ASSERT(status);
	GAME_ASSERT(nObjects <= MAX_OBJECTS_IN_GROUP);					// see if overflow

	status = Q3Group_GetFirstPosition(the3DMFFile, &position);		// get 1st object in list
	GAME_ASSERT(status);

			/*********************/
			/* SCAN THRU OBJECTS */
			/*********************/
			
	for (i = 0; i < nObjects; i++)
	{
		status = Q3Group_GetPositionObject(the3DMFFile, position, &gObjectGroupList[groupNum][i]);	// get ref to object
		GAME_ASSERT(status);

		QD3D_CalcObjectBoundingSphere(gGameViewInfoPtr, gObjectGroupList[groupNum][i], &gObjectGroupRadiusList[groupNum][i]); 	
		QD3D_CalcObjectBoundingBox(gGameViewInfoPtr,gObjectGroupList[groupNum][i],&gObjectGroupBBoxList[groupNum][i]); // save bbox
		
		
		Q3Group_GetNextPosition(the3DMFFile, &position);			// get position of next object
	}

	gNumObjectsInGroupList[groupNum] = nObjects;					// set # objects.
	
			/* NUKE ORIGINAL FILE */
			
	Q3Object_Dispose(the3DMFFile);



			/*********************************************************************/
			/* DISCARD TRANSPARENT TEXELS (Source port addition for performance) */
			/*********************************************************************/

	for (i = 0; i < nObjects; i++)
	{
		if (alphaTestObjectMask & (1uLL << i))						// should we apply the alpha test to this object?
		{
			ForEachTriMesh(gObjectGroupList[groupNum][i], QD3D_SetTextureAlphaThreshold_TriMesh, NULL);
		}
	}
}


/******************** DELETE 3DMF GROUP **************************/

void Free3DMFGroup(Byte groupNum)
{
short	i;

	for (i = 0; i < gNumObjectsInGroupList[groupNum]; i++)			// dispose of old objects (or at least this reference)
	{
		if (gObjectGroupList[groupNum][i] != nil)
		{
			Q3Object_Dispose(gObjectGroupList[groupNum][i]);
			gObjectGroupList[groupNum][i] = nil;
		}
	}
	
	for (i = 0; i < MAX_OBJECTS_IN_GROUP; i++)						// make sure to init the entire list to be safe
		gObjectGroupList[groupNum][i] = nil;

	gNumObjectsInGroupList[groupNum] = 0;
}


/******************* DELETE ALL 3DMF GROUPS ************************/

void DeleteAll3DMFGroups(void)
{
short	i;

	for (i=0; i < MAX_3DMF_GROUPS; i++)
	{
		if (gNumObjectsInGroupList[i])				// see if this group empty
			Free3DMFGroup(i);
	}
}






