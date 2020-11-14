/****************************/
/*   		PICK.C    	    */
/* (c)1997-99 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	ObjNode				*gMenuIcons[NUM_MENU_ICONS];
extern	ObjNode				*gSaveYes,*gSaveNo;


/****************************/
/*    PROTOTYPES            */
/****************************/



/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/



/*************** PICK MAIN MENU ICON ********************/
//
// INPUT: point = point of click
//
// OUTPUT: TRUE = something got picked
//			pickID = pickID of picked object
//

Boolean	PickMainMenuIcon(TQ3Point2D point,u_long *pickID)
{
TQ3PickObject			myPickObject;
TQ3Uns32			myNumHits;
TQ3Status				myErr;
int						i;
				
					/* CREATE PICK OBJECT */
				
	myPickObject = CreateDefaultPickObject(&point);						// create pick object


					/*************/
					/* PICK LOOP */
					/*************/
					
	myErr = Q3View_StartPicking(gGameViewInfoPtr->viewObject,myPickObject);
	if (myErr == kQ3Failure)
		DoFatalAlert(" PICK: Q3View_StartRendering failed!");
	do
	{
		for (i = 0;i < NUM_MENU_ICONS; i++)
			Q3Object_Submit(gMenuIcons[i]->BaseGroup, gGameViewInfoPtr->viewObject);
	} while (Q3View_EndPicking(gGameViewInfoPtr->viewObject) == kQ3ViewStatusRetraverse);
		
			
			/*********************************/
			/* SEE WHETHER ANY HITS OCCURRED */	
			/*********************************/

	if (Q3Pick_GetNumHits(myPickObject, &myNumHits) != kQ3Failure)
	{
		if (myNumHits > 0)
		{
					/* PROCESS THE HIT */

			Q3Pick_GetPickDetailData(myPickObject,0,kQ3PickDetailMaskPickID,pickID);	// get pickID
			Q3Object_Dispose(myPickObject);												// dispose of pick object
			return(true);
		}
	}
	
			/* DIDNT HIT ANYTHING */

	Q3Object_Dispose(myPickObject);	
	return(false);
}



/****************** CREATE DEFAULT PICK OBJECT *********************/

TQ3PickObject CreateDefaultPickObject(TQ3Point2D *point)
{
TQ3WindowPointPickData	myWPPickData;
TQ3PickObject			myPickObject;

			/* CREATE PICK DATA */
									
	myWPPickData.data.sort = kQ3PickSortNearToFar;		// set sort type
	myWPPickData.data.mask = kQ3PickDetailMaskPickID |	// what do I want to get back?
							kQ3PickDetailMaskXYZ |
							kQ3PickDetailMaskObject|
							kQ3PickDetailMaskDistance;
	
	myWPPickData.data.numHitsToReturn = 1;				// only get 1 hit
	
	myWPPickData.point 				= *point;			// set window point to pick
	myWPPickData.vertexTolerance 	= 4.0;
	myWPPickData.edgeTolerance 		= 4.0;


			/* CREATE WINDOW POINT PICK OBJECT */
			
	myPickObject = Q3WindowPointPick_New(&myWPPickData);
	if (myPickObject == nil)
		DoFatalAlert(" Q3WindowPointPick_New failed!");
		
	return(myPickObject);
}




/*************** PICK SAVE GAME ICON ********************/
//
// INPUT: point = point of click
//
// OUTPUT: TRUE = something got picked
//			pickID = pickID of picked object
//

Boolean	PickSaveGameIcon(TQ3Point2D point,u_long *pickID)
{
TQ3PickObject			myPickObject;
TQ3Uns32			myNumHits;
TQ3Status				myErr;
				
					/* CREATE PICK OBJECT */
				
	myPickObject = CreateDefaultPickObject(&point);						// create pick object


					/*************/
					/* PICK LOOP */
					/*************/
					
	myErr = Q3View_StartPicking(gGameViewInfoPtr->viewObject,myPickObject);
	if (myErr == kQ3Failure)
		DoFatalAlert("PickSaveGameIcon: Q3View_StartRendering failed!");
	do
	{
		Q3Object_Submit(gSaveYes->BaseGroup, gGameViewInfoPtr->viewObject);
		Q3Object_Submit(gSaveNo->BaseGroup, gGameViewInfoPtr->viewObject);
	} while (Q3View_EndPicking(gGameViewInfoPtr->viewObject) == kQ3ViewStatusRetraverse);
		
			
			/*********************************/
			/* SEE WHETHER ANY HITS OCCURRED */	
			/*********************************/

	if (Q3Pick_GetNumHits(myPickObject, &myNumHits) != kQ3Failure)
	{
		if (myNumHits > 0)
		{
					/* PROCESS THE HIT */

			Q3Pick_GetPickDetailData(myPickObject,0,kQ3PickDetailMaskPickID,pickID);	// get pickID
			Q3Object_Dispose(myPickObject);												// dispose of pick object
			return(true);
		}
	}
	
			/* DIDNT HIT ANYTHING */

	Q3Object_Dispose(myPickObject);	
	return(false);
}





