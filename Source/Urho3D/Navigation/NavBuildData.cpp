
#include "Precompiled.h"

#include "../Navigation/NavBuildData.h"

#include <Recast/Recast.h>
#include <Detour/DetourNavMesh.h>
#include <Detour/DetourNavMeshBuilder.h>
#include <Detour/DetourNavMeshQuery.h>
#include <Detour/DetourTileCache.h>
#include <Detour/DetourTileCacheBuilder.h>

namespace Urho3D
{
	/// Construct.
	NavBuildData::NavBuildData() :
		ctx_(new rcContext(false)),
		heightField_(0),
		compactHeightField_(0)
	{
		ctx_ = new rcContext(false);
	}

	NavBuildData::~NavBuildData()
	{
		if (ctx_)
			delete(ctx_);
		rcFreeHeightField(heightField_);
		rcFreeCompactHeightfield(compactHeightField_);

		ctx_ = 0;
		heightField_ = 0;
		compactHeightField_ = 0;
		
	}

	SimpleNavBuildData::SimpleNavBuildData() :
		NavBuildData(),
		contourSet_(0),
		polyMesh_(0),
		polyMeshDetail_(0)
	{

	}

	SimpleNavBuildData::~SimpleNavBuildData()
	{
		rcFreeContourSet(contourSet_);
		rcFreePolyMesh(polyMesh_);
		rcFreePolyMeshDetail(polyMeshDetail_);

		contourSet_ = 0;
		polyMesh_ = 0;
		polyMeshDetail_ = 0;
	}

	DynamicNavBuildData::DynamicNavBuildData(dtTileCacheAlloc* allocator) :
		contourSet_(0),
		heightFieldLayers_(0),
		polyMesh_(0),
		alloc_(allocator)
	{

	}

	DynamicNavBuildData::~DynamicNavBuildData()
	{
		if (contourSet_)
			dtFreeTileCacheContourSet(alloc_, contourSet_);
		if (polyMesh_)
			dtFreeTileCachePolyMesh(alloc_, polyMesh_);
		if (heightFieldLayers_)
			rcFreeHeightfieldLayerSet(heightFieldLayers_);

		contourSet_ = 0;
		polyMesh_ = 0;
		heightFieldLayers_ = 0;
	}
}