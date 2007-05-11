//
// fences.h
//

#ifndef FENCE_H
#define FENCE_H

typedef	struct
{
	long	x,z;
}FencePointType;


typedef struct
{
	float	top,bottom,left,right;
}RectF;

typedef struct
{
	u_short			type;				// type of fence
	short			numNubs;			// # nubs in fence
	FencePointType	**nubList;			// handle to nub list	
	RectF			bBox;				// bounding box of fence area	
	TQ3Vector2D		*sectionVectors;	// for each section/span, this is the vector from nub(n) to nub(n+1)
}FenceDefType;


//============================================

void InitFenceManager(void);
void PrimeFences(void);
void DoFenceCollision(ObjNode *theNode, float radiusScale);
void DrawFences(const QD3DSetupOutputType *setupInfo);
void DisposeFenceShaders(void);


#endif


