//
// splineitems.h
//




//=====================================================

void PrimeSplines(void);
void GetCoordOnSpline(SplineDefType *splinePtr, float placement, float *x, float *z);
Boolean IsSplineItemVisible(ObjNode *theNode);
void AddToSplineObjectList(ObjNode *theNode);
void AddToSplineObjectList(ObjNode *theNode);
void MoveSplineObjects(void);
long GetObjectCoordOnSpline(ObjNode *theNode, float *x, float *z);
Boolean RemoveFromSplineObjectList(ObjNode *theNode);
void EmptySplineObjectList(void);
void IncreaseSplineIndex(ObjNode *theNode, float speed);
void IncreaseSplineIndexZigZag(ObjNode *theNode, float speed);
void DrawSplines(void);
