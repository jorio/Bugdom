//
// morphobj.h
//

void DoMorphTest(void);
void MoveMorphObject(ObjNode *theNode);
ObjNode	*MakeNewMorphObject(NewObjectDefinitionType *newObjDef, ObjNode *objA, ObjNode *objB);
void DrawMorphObject(ObjNode *theNode, TQ3ViewObject view);
void InitMorphManager(void);
void FreeMorphData(ObjNode *theNode);

