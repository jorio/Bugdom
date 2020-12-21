#include <vector>
#include <Quesa.h>
#include <QuesaGeometry.h>
#include <QuesaGroup.h>
#include <QuesaStyle.h>
#include <Headers/MyPCH_Normal.pch>

static std::vector<TQ3DisplayGroupObject> gPickableObjects;

void PickableQuads_NewQuad(
		TQ3Point3D	coord,
		float		width,
		float		height,
		TQ3Int32	pickID
		)
{
	float left		= coord.x - width/2.0f;
	float right		= coord.x + width/2.0f;
	float top		= coord.y + height/2.0f;
	float bottom	= coord.y - height/2.0f;

	TQ3PolygonData pickableQuad;

	TQ3Vertex3D pickableQuadVertices[4] = {
			{{left,		top,	coord.z}, nullptr},
			{{right,	top,	coord.z}, nullptr},
			{{right,	bottom,	coord.z}, nullptr},
			{{left,		bottom,	coord.z}, nullptr},
	};

	pickableQuad.numVertices = 4;
	pickableQuad.vertices = pickableQuadVertices;
	pickableQuad.polygonAttributeSet = nullptr;

	TQ3DisplayGroupObject	dgo		= Q3DisplayGroup_New();
	TQ3StyleObject			pick	= Q3PickIDStyle_New(pickID);
	TQ3GeometryObject		poly	= Q3Polygon_New(&pickableQuad);
	Q3Group_AddObject(dgo, pick);
	Q3Group_AddObject(dgo, poly);

	gPickableObjects.push_back(dgo);

	Q3Object_Dispose(pick);
	Q3Object_Dispose(poly);
}

void PickableQuads_DisposeAll()
{
	for (auto object : gPickableObjects)
	{
		Q3Object_Dispose(object);
	}

	gPickableObjects.clear();
}

bool PickableQuads_GetPick(TQ3ViewObject view, TQ3Point2D point, TQ3Int32 *pickID)
{
	TQ3PickObject			pickObject;
	TQ3WindowPointPickData	pickSpec;
	TQ3Uns32				numHits;
	TQ3Status				status;

	//--------------------
	// CREATE PICK OBJECT

	pickSpec.data.sort				= kQ3PickSortNearToFar;		// set sort type
	pickSpec.data.mask				= kQ3PickDetailMaskPickID	// what do I want to get back?
									| kQ3PickDetailMaskXYZ;
	pickSpec.data.numHitsToReturn	= 1;				// only get 1 hit
	pickSpec.point					= point;			// set window point to pick
	pickSpec.vertexTolerance		= 4.0f;
	pickSpec.edgeTolerance			= 4.0f;

	pickObject = Q3WindowPointPick_New(&pickSpec);
	GAME_ASSERT(pickObject);

	//------------------
	// PICK SUBMIT LOOP

	status = Q3View_StartPicking(view, pickObject);
	GAME_ASSERT(status);
	do
	{
		for (auto object : gPickableObjects)
		{
			Q3Object_Submit(object, view);
		}
	} while (Q3View_EndPicking(view) == kQ3ViewStatusRetraverse);

	//-------------------------------
	// SEE WHETHER ANY HITS OCCURRED

	bool hitAnything = false;

	if (Q3Pick_GetNumHits(pickObject, &numHits) != kQ3Failure &&
		numHits > 0)
	{
		// Process the hit.
		hitAnything = true;
		Q3Pick_GetPickDetailData(pickObject, 0, kQ3PickDetailMaskPickID, pickID);	// get pickID
	}

	Q3Object_Dispose(pickObject);
	return hitAnything;
}

