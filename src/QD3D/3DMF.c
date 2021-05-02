/****************************/
/*   	3DMF.C 		   	    */
/* (c)1996 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

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


/*********************/
/*    VARIABLES      */
/*********************/

TQ3MetaFile*				gObjectGroupFile[MAX_3DMF_GROUPS];
TQ3TriMeshFlatGroup			gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
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
	printf("TODO NOQUESA: %s\n", __func__);
	return nil;
#if 0	// NOQUESA
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
#endif
}


/*************** CREATE 3DMF FILE OBJECT ****************/
//
// Creates and returns a File Object which contains the 3DMF file.
//
// INPUT: myFSSpec = file to create object for
//

static TQ3FileObject	Create3DMFFileObject(const FSSpec *myFSSpec)
{
	printf("TODO NOQUESA: %s\n", __func__);
	return nil;
#if 0	// NOQUESA
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
#endif
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
	printf("TODO NOQUESA: %s\n", __func__);
	return kQ3Failure;
#if 0	// NOQUESA
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

			//printf("!!! MyRead3DMFModel: Q3File_ReadObject returned nil object!\n");   // This is fine with Quesa
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
#endif
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
	GAME_ASSERT(groupNum < MAX_3DMF_GROUPS);

			/* DISPOSE OF ANY OLD OBJECTS */

#if 1
	GAME_ASSERT_MESSAGE(gNumObjectsInGroupList[groupNum] == 0, "3DMF group was not freed before reuse");
#else
	Free3DMFGroup(groupNum);
#endif

			/* LOAD NEW GEOMETRY */

	TQ3MetaFile* the3DMFFile = Q3MetaFile_Load3DMF(spec);
	GAME_ASSERT(the3DMFFile);

	gObjectGroupFile[groupNum] = the3DMFFile;

			/* UPLOAD TEXTURES TO GPU */

	Render_Load3DMFTextures(the3DMFFile);

			/* BUILD OBJECT LIST */

	int nObjects = the3DMFFile->numTopLevelGroups;
	GAME_ASSERT(nObjects > 0);
	GAME_ASSERT(nObjects <= MAX_OBJECTS_IN_GROUP);

			/*********************/
			/* SCAN THRU OBJECTS */
			/*********************/

	for (int i = 0; i < nObjects; i++)
	{
		TQ3TriMeshFlatGroup meshList = the3DMFFile->topLevelGroups[i];
		gObjectGroupList[groupNum][i] = meshList;
		GAME_ASSERT(0 != meshList.numMeshes);
		GAME_ASSERT(nil != meshList.meshes);

		QD3D_CalcObjectBoundingSphere(gGameViewInfoPtr, &gObjectGroupList[groupNum][i], &gObjectGroupRadiusList[groupNum][i]);
		QD3D_CalcObjectBoundingBox(meshList.numMeshes, meshList.meshes, &gObjectGroupBBoxList[groupNum][i]); // save bbox
	}

	gNumObjectsInGroupList[groupNum] = nObjects;					// set # objects.


	PatchGrouped3DMF(spec->cName, gObjectGroupList[groupNum], nObjects);
}


/******************** DELETE 3DMF GROUP **************************/

void Free3DMFGroup(Byte groupNum)
{
	if (gObjectGroupFile[groupNum] != nil)
	{
		Render_Dispose3DMFTextures(gObjectGroupFile[groupNum]);
		Q3MetaFile_Dispose(gObjectGroupFile[groupNum]);
		gObjectGroupFile[groupNum] = nil;
	}

	memset(gObjectGroupList[groupNum], 0, sizeof(gObjectGroupList[groupNum]));	// make sure to init the entire list to be safe

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






