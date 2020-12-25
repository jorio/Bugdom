#include <vector>
#include <Quesa.h>
#include <QuesaGeometry.h>
#include <QuesaGroup.h>
#include <QuesaStyle.h>
#include "bugdom.h"

struct PickableQuad
{
	std::vector<TQ3Vertex3D>	vertices;
	TQ3DisplayGroupObject		object;

	PickableQuad(TQ3Point3D coord, float width, float height, TQ3Int32 pickID);

	PickableQuad(const PickableQuad&) = delete;

	PickableQuad(PickableQuad&&) noexcept;

	~PickableQuad()
	{
		if (object)
		{
			Q3Object_Dispose(object);
			object = nullptr;
		}
	}
};

static std::vector<PickableQuad> gPickableObjects;

PickableQuad::PickableQuad(PickableQuad && other) noexcept
	: vertices(std::move(other.vertices))
	, object(other.object)
{
	other.object = nullptr;
}

PickableQuad::PickableQuad(
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

	vertices.push_back({{left,	top,	coord.z}, nullptr});
	vertices.push_back({{right,	top,	coord.z}, nullptr});
	vertices.push_back({{right,	bottom,	coord.z}, nullptr});
	vertices.push_back({{left,	bottom,	coord.z}, nullptr});

	TQ3PolygonData polygonData =
	{
		.numVertices			= 4,
		.vertices				= vertices.data(),
		.polygonAttributeSet	= nullptr,
	};

	TQ3DisplayGroupObject	dgo		= Q3DisplayGroup_New();
	TQ3GeometryObject		poly	= Q3Polygon_New(&polygonData);
	TQ3StyleObject			pick	= Q3PickIDStyle_New(pickID);
	Q3Group_AddObject(dgo, pick);
	Q3Group_AddObject(dgo, poly);
	Q3Object_Dispose(pick);
	Q3Object_Dispose(poly);

	this->object = dgo;
}

void PickableQuads_NewQuad(
		TQ3Point3D	coord,
		float		width,
		float		height,
		TQ3Int32	pickID
)
{
	gPickableObjects.emplace_back(coord, width, height, pickID);
}

void PickableQuads_DisposeAll()
{
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
		for (auto& pickable : gPickableObjects)
		{
			Q3Object_Submit(pickable.object, view);
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

void PickableQuads_Draw(TQ3ViewObject view)
{
	for (const auto& pickable : gPickableObjects)
	{
		std::vector vertices = pickable.vertices;
		vertices.push_back(vertices[0]);	// copy first vertex to loop polyline

		TQ3PolyLineData pld;
		pld.vertices = vertices.data();
		pld.numVertices = vertices.size();
		pld.polyLineAttributeSet = nullptr;
		pld.segmentAttributeSet = nullptr;
		Q3PolyLine_Submit(&pld, view);
	}
}

