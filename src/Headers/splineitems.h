//
// splineitems.h
//


void PrimeSplines(void);

int GetCoordOnSpline(const SplineDefType* spline, float placement, float* x, float* z);
int GetObjectCoordOnSpline(ObjNode* theNode, float* x, float* z);

Boolean IsSplineItemVisible(ObjNode *theNode);
void AddToSplineObjectList(ObjNode *theNode);
void AddToSplineObjectList(ObjNode *theNode);
void MoveSplineObjects(void);
Boolean RemoveFromSplineObjectList(ObjNode *theNode);
void EmptySplineObjectList(void);
float IncreaseSplineIndex(ObjNode *theNode, float speed);
float IncreaseSplineIndexZigZag(ObjNode *theNode, float speed);
void DrawSplines(void);

int GetSplinePointsPerSpan(int numNubs, const SplinePointType* nubs, int* outPointsPerSpan);
SplinePointType** BakeSpline(int numNubs, const SplinePointType* nubs, int* pointsPerSpan);
