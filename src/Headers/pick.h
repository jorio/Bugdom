//
// pick.h
//

ObjNode* NewPickableQuad(TQ3Point3D	coord, float width, float height, int32_t pickID);

void DisposePickableObjects(void);

bool PickObject(int mouseX, int mouseY, int32_t *pickID);

