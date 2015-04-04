#pragma once

#include "../Container/Vector.h"
#include "../Math/BoundingBox.h"
#include "../Math/Vector3.h"

class rcContext;
struct rcHeightfield;
struct rcCompactHeightfield;
struct rcContourSet;
struct rcPolyMesh;
struct rcPolyMeshDetail;

struct rcHeightfieldLayerSet;
struct dtTileCacheContourSet;
struct dtTileCachePolyMesh;
struct dtTileCacheAlloc;

namespace Urho3D
{
	struct URHO3D_API NavAreaStub
	{
		BoundingBox bounds_;
		unsigned char areaID_;
	};

	struct URHO3D_API NavBuildData
	{
		NavBuildData();
		virtual ~NavBuildData();

		/// World-space bounding box of the navigation mesh tile.
		BoundingBox worldBoundingBox_;
		/// Vertices from geometries.
		PODVector<Vector3> vertices_;
		/// Triangle indices from geometries.
		PODVector<int> indices_;
		/// Offmesh connection vertices.
		PODVector<Vector3> offMeshVertices_;
		/// Offmesh connection radii.
		PODVector<float> offMeshRadii_;
		/// Offmesh connection flags.
		PODVector<unsigned short> offMeshFlags_;
		/// Offmesh connection areas.
		PODVector<unsigned char> offMeshAreas_;
		/// Offmesh connection direction.
		PODVector<unsigned char> offMeshDir_;
		/// Recast context.
		rcContext* ctx_;
		/// Recast heightfield
		rcHeightfield* heightField_;
		/// Recast compact heightfield.
		rcCompactHeightfield* compactHeightField_;
		/// Pretransformed navigation areas, no correlation to the geometry above
		PODVector<NavAreaStub> navAreas_;
	};

	struct SimpleNavBuildData : public NavBuildData
	{
		SimpleNavBuildData();
		virtual ~SimpleNavBuildData();

		/// Recast contour set.
		rcContourSet* contourSet_;
		/// Recast poly mesh.
		rcPolyMesh* polyMesh_;
		/// Recast detail poly mesh.
		rcPolyMeshDetail* polyMeshDetail_;
	};

	struct DynamicNavBuildData : public NavBuildData
	{
		DynamicNavBuildData(dtTileCacheAlloc* alloc);
		virtual ~DynamicNavBuildData();

		dtTileCacheContourSet* contourSet_;
		dtTileCachePolyMesh* polyMesh_;
		/// Recast heightfield layer set
		rcHeightfieldLayerSet* heightFieldLayers_;
		dtTileCacheAlloc* alloc_;
	};
}