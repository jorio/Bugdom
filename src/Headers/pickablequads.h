#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void PickableQuads_NewQuad(
		TQ3Point3D	coord,
		float		width,
		float		height,
		TQ3Int32	pickID
);

void PickableQuads_DisposeAll(void);

bool PickableQuads_GetPick(TQ3ViewObject view, TQ3Point2D point, TQ3Int32 *pickID);

void PickableQuads_Draw(TQ3ViewObject view);

#ifdef __cplusplus
}
#endif
