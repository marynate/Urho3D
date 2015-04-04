
#include "Precompiled.h"

#include "../Navigation/DynamicNavigationMesh.h"

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Graphics/DebugRenderer.h"
#include "../IO/Log.h"
#include "../Math/BoundingBox.h"
#include "../Navigation/DetourDebugRenderer.h"
#include "../Navigation/NavBuildData.h"
#include "../Navigation/Obstacle.h"
#include "../Scene/Node.h"
#include "../Scene/SceneEvents.h"

#include <LZ4/lz4.h>
#include <cfloat>
#include <Detour/fastlz.h>
#include <Detour/DetourNavMesh.h>
#include <Detour/DetourNavMeshBuilder.h>
#include <Detour/DetourNavMeshQuery.h>
#include <Detour/DetourDebugDraw.h>
#include <Detour/DetourTileCache.h>
#include <Detour/DetourTileCacheBuilder.h>
#include <Recast/Recast.h>
#include <Recast/RecastAlloc.h>

namespace Urho3D
{
	extern const char* NAVIGATION_CATEGORY;

	static void DebugDrawTileCachePolyMesh(DebugRenderer* dd, const struct dtTileCachePolyMesh& lmesh,
		const float* orig, const float cs, const float ch)
	{
		if (!dd) return;

		const int nvp = lmesh.nvp;

		const int offs[2 * 4] = { -1, 0, 0, 1, 1, 0, 0, -1 };

		// Draw neighbours edges
		const Color coln = Color(0, 48/255.0f, 64/255.0f, 32/255.0f);
		Vector3 lastVert;
		for (int i = 0; i < lmesh.npolys; ++i)
		{
			const unsigned short* p = &lmesh.polys[i*nvp * 2];
			for (int j = 0; j < nvp; ++j)
			{
				if (p[j] == DT_TILECACHE_NULL_IDX) break;
				if (p[nvp + j] & 0x8000) continue;
				const int nj = (j + 1 >= nvp || p[j + 1] == DT_TILECACHE_NULL_IDX) ? 0 : j + 1;
				int vi[2] = { p[j], p[nj] };

				for (int k = 0; k < 2; ++k)
				{
					const unsigned short* v = &lmesh.verts[vi[k] * 3];
					const float x = orig[0] + v[0] * cs;
					const float y = orig[1] + (v[1] + 1)*ch + 0.1f;
					const float z = orig[2] + v[2] * cs;
					if (k == 0)
						lastVert = Vector3(x, y, z);
					else
						dd->AddLine(lastVert, Vector3(x, y, z), coln);
				}
			}
		}

		// Draw boundary edges
		const Color colb = Color(0, 48/255.0f, 64/255.0f, 220/255.0f);
		for (int i = 0; i < lmesh.npolys; ++i)
		{
			const unsigned short* p = &lmesh.polys[i*nvp * 2];
			for (int j = 0; j < nvp; ++j)
			{
				if (p[j] == DT_TILECACHE_NULL_IDX) break;
				if ((p[nvp + j] & 0x8000) == 0) continue;
				const int nj = (j + 1 >= nvp || p[j + 1] == DT_TILECACHE_NULL_IDX) ? 0 : j + 1;
				int vi[2] = { p[j], p[nj] };

				Color col = colb;
				if ((p[nvp + j] & 0xf) != 0xf)
				{
					const unsigned short* va = &lmesh.verts[vi[0] * 3];
					const unsigned short* vb = &lmesh.verts[vi[1] * 3];

					const float ax = orig[0] + va[0] * cs;
					const float ay = orig[1] + (va[1] + 1 + (i & 1))*ch;
					const float az = orig[2] + va[2] * cs;
					const float bx = orig[0] + vb[0] * cs;
					const float by = orig[1] + (vb[1] + 1 + (i & 1))*ch;
					const float bz = orig[2] + vb[2] * cs;

					const float cx = (ax + bx)*0.5f;
					const float cy = (ay + by)*0.5f;
					const float cz = (az + bz)*0.5f;

					int d = p[nvp + j] & 0xf;

					const float dx = cx + offs[d * 2 + 0] * 2 * cs;
					const float dy = cy;
					const float dz = cz + offs[d * 2 + 1] * 2 * cs;

					
					dd->AddLine(Vector3(cx, cy, cz), Vector3(dx, dy, dz), Color(1, 0, 0, 1));

					col = Color(1, 1, 1, 0.5f);
				}

				for (int k = 0; k < 2; ++k)
				{
					const unsigned short* v = &lmesh.verts[vi[k] * 3];
					const float x = orig[0] + v[0] * cs;
					const float y = orig[1] + (v[1] + 1)*ch + 0.1f;
					const float z = orig[2] + v[2] * cs;
					if (k == 0)
						lastVert = Vector3(x, y, z);
					else
						dd->AddLine(lastVert, Vector3(x, y, z), col);
				}
			}
		}
	}

	struct DynamicNavigationMesh::TileCacheData
	{
		unsigned char* data;
		int dataSize;
	};

	struct FastLZCompressor : public dtTileCacheCompressor
	{
		virtual int maxCompressedSize(const int bufferSize)
		{
			return (int)(bufferSize* 1.05f);
		}

		virtual dtStatus compress(const unsigned char* buffer, const int bufferSize,
			unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize)
		{
			//*compressedSize = LZ4_compress((const char*)buffer, (char*)compressed, bufferSize);
			*compressedSize = fastlz_compress((const void *const)buffer, bufferSize, compressed);
			return DT_SUCCESS;
		}

		virtual dtStatus decompress(const unsigned char* compressed, const int compressedSize,
			unsigned char* buffer, const int maxBufferSize, int* bufferSize)
		{
			//*bufferSize = LZ4_decompress_safe((const char*)compressed, (char*)buffer, compressedSize, maxBufferSize);
			*bufferSize = fastlz_decompress(compressed, compressedSize, buffer, maxBufferSize);
			return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
		}
	};

	struct MeshProcess : public dtTileCacheMeshProcess
	{
		inline MeshProcess()
		{
		}

		virtual void process(struct dtNavMeshCreateParams* params, unsigned char* polyAreas, unsigned short* polyFlags)
		{
			// Update poly flags from areas.
			for (int i = 0; i < params->polyCount; ++i)
			{
				polyFlags[i] = RC_WALKABLE_AREA;
			}
		}
	};


	struct LinearAllocator : public dtTileCacheAlloc
	{
		unsigned char* buffer;
		int capacity;
		int top;
		int high;

		LinearAllocator(const int cap) : buffer(0), capacity(0), top(0), high(0)
		{
			resize(cap);
		}

		~LinearAllocator()
		{
			dtFree(buffer);
		}

		void resize(const int cap)
		{
			if (buffer) dtFree(buffer);
			buffer = (unsigned char*)dtAlloc(cap, DT_ALLOC_PERM);
			capacity = cap;
		}

		virtual void reset()
		{
			high = Max(high, top);
			top = 0;
		}

		virtual void* alloc(const int size)
		{
			if (!buffer)
				return 0;
			if (top + size > capacity)
				return 0;
			unsigned char* mem = &buffer[top];
			top += size;
			return mem;
		}

		virtual void free(void* /*ptr*/)
		{
			// Empty
		}
	};

	/// caches decompressed views of the original mesh geometry to avoid decompressing every frame that debug is being drawn
	struct NavDebugDrawData
	{
		NavDebugDrawData() :
			layer_(0),
			lcset_(0),
			lmesh_(0),
			allocator_(0)
		{

		}

		~NavDebugDrawData()
		{
			if (layer_)
				dtFreeTileCacheLayer(allocator_, layer_);
			layer_ = 0;
			if (lcset_)
				dtFreeTileCacheContourSet(allocator_, lcset_);
			lcset_ = 0;
			if (lmesh_)
				dtFreeTileCachePolyMesh(allocator_, lmesh_);
			lmesh_ = 0;
		}

		dtTileCacheAlloc* allocator_;
		dtTileCacheLayer* layer_;
		dtTileCacheContourSet* lcset_;
		dtTileCachePolyMesh* lmesh_;
	};

	DynamicNavigationMesh::DynamicNavigationMesh(Context* context) :
		NavigationMesh(context),
		tileCache_(0),
		maxObstacles_(256)
	{
		tileSize_ = 64;
		partitionType_ = NAVMESH_PARTITION_MONOTONE;
		allocator_ = new LinearAllocator(32000); //32kb
		compressor_ = new FastLZCompressor();
		meshProcessor_ = new MeshProcess();
	}

	DynamicNavigationMesh::~DynamicNavigationMesh()
	{
		ReleaseNavigationMesh();
		delete allocator_;
		delete compressor_;
		delete meshProcessor_;
	}

	void DynamicNavigationMesh::RegisterObject(Context* context)
	{
		context->RegisterFactory<DynamicNavigationMesh>(NAVIGATION_CATEGORY);

		COPY_BASE_ATTRIBUTES(NavigationMesh);
	}

	void DynamicNavigationMesh::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
	{
		if (!debug || !navMesh_ || !node_)
			return;

		const Matrix3x4& worldTransform = node_->GetWorldTransform();

		const dtNavMesh* navMesh = navMesh_;

		struct TileCacheBuildContext
		{
			inline TileCacheBuildContext(struct dtTileCacheAlloc* a) : layer(0), lcset(0), lmesh(0), alloc(a) {}
			inline ~TileCacheBuildContext() { purge(); }
			void purge()
			{
				dtFreeTileCacheLayer(alloc, layer);
				layer = 0;
				dtFreeTileCacheContourSet(alloc, lcset);
				lcset = 0;
				dtFreeTileCachePolyMesh(alloc, lmesh);
				lmesh = 0;
			}
			struct dtTileCacheLayer* layer;
			struct dtTileCacheContourSet* lcset;
			struct dtTileCachePolyMesh* lmesh;
			struct dtTileCacheAlloc* alloc;
		};
		
		for (int z = 0; z < numTilesZ_; ++z)
		{
			for (int x = 0; x < numTilesX_; ++x)
			{
				dtCompressedTileRef tiles[128];
				const int ntiles = tileCache_->getTilesAt(x, z, tiles, 128);

				dtTileCacheAlloc* talloc = tileCache_->getAlloc();
				dtTileCacheCompressor* tcomp = tileCache_->getCompressor();
				const dtTileCacheParams* params = tileCache_->getParams();
		
				for (int i = 0; i < ntiles; ++i)
				{
					const dtCompressedTile* tile = tileCache_->getTileByRef(tiles[i]);

					talloc->reset();

					TileCacheBuildContext bc(allocator_);
					const int walkableClimbVx = (int)(params->walkableClimb / params->ch);
					dtStatus status;

					// Decompress tile layer data. 
					status = dtDecompressTileCacheLayer(allocator_, tcomp, tile->data, tile->dataSize, &bc.layer);
					if (dtStatusFailed(status))
						return;
					//duDebugDrawTileCacheLayerAreas(&dd, *bc.layer, params->cs, params->ch);
					// Build navmesh
					status = dtBuildTileCacheRegions(talloc, *bc.layer, walkableClimbVx);
					if (dtStatusFailed(status))
						return;
					//duDebugDrawTileCacheLayerRegions(&dd, *bc.layer, params->cs, params->ch);

					bc.lcset = dtAllocTileCacheContourSet(talloc);
					if (!bc.lcset)
						return;
					status = dtBuildTileCacheContours(talloc, *bc.layer, walkableClimbVx, params->maxSimplificationError, *bc.lcset);
					if (dtStatusFailed(status))
						return;
					//duDebugDrawTileCacheContours(&dd, *bc.lcset, tile->header->bmin, params->cs, params->ch);

					bc.lmesh = dtAllocTileCachePolyMesh(talloc);
					if (!bc.lmesh)
						return;
					status = dtBuildTileCachePolyMesh(talloc, *bc.lcset, *bc.lmesh);
					if (dtStatusFailed(status))
						return;

					DebugDrawTileCachePolyMesh(debug, *bc.lmesh, tile->header->bmin, params->cs, params->ch);
					//duDebugDrawTileCachePolyMesh(&dd, *bc.lmesh, tile->header->bmin, params->cs, params->ch);
				}
			}
		}
	}

	bool DynamicNavigationMesh::Build()
	{
		PROFILE(BuildNavigationMesh);

		// Release existing navigation data and zero the bounding box
		ReleaseNavigationMesh();

		if (!node_)
			return false;

		if (!node_->GetWorldScale().Equals(Vector3::ONE))
			LOGWARNING("Navigation mesh root node has scaling. Agent parameters may not work as intended");

		Vector<NavigationGeometryInfo> geometryList;
		CollectGeometries(geometryList);

		if (geometryList.Empty())
			return true; // Nothing to do

		// Build the combined bounding box
		for (unsigned i = 0; i < geometryList.Size(); ++i)
			boundingBox_.Merge(geometryList[i].boundingBox_);

		// Expand bounding box by padding
		boundingBox_.min_ -= padding_;
		boundingBox_.max_ += padding_;

		{
			PROFILE(BuildNavigationMesh);

			// Calculate number of tiles
			int gridW = 0, gridH = 0;
			float tileEdgeLength = (float)tileSize_ * cellSize_;
			rcCalcGridSize(&boundingBox_.min_.x_, &boundingBox_.max_.x_, cellSize_, &gridW, &gridH);
			numTilesX_ = (gridW + tileSize_ - 1) / tileSize_;
			numTilesZ_ = (gridH + tileSize_ - 1) / tileSize_;

			// Calculate max. number of tiles and polygons, 22 bits available to identify both tile & polygon within tile
			unsigned maxTiles = NextPowerOfTwo(numTilesX_ * numTilesZ_);
			unsigned tileBits = 0;
			unsigned temp = maxTiles;
			while (temp > 1)
			{
				temp >>= 1;
				++tileBits;
			}

			unsigned maxPolys = 1 << (22 - tileBits);

			dtNavMeshParams params;
			rcVcopy(params.orig, &boundingBox_.min_.x_);
			params.tileWidth = tileEdgeLength;
			params.tileHeight = tileEdgeLength;
			params.maxTiles = maxTiles;
			params.maxPolys = maxPolys;

			navMesh_ = dtAllocNavMesh();
			if (!navMesh_)
			{
				LOGERROR("Could not allocate navigation mesh");
				return false;
			}

			if (dtStatusFailed(navMesh_->init(&params)))
			{
				LOGERROR("Could not initialize navigation mesh");
				ReleaseNavigationMesh();
				return false;
			}

			dtTileCacheParams tileCacheParams;
			memset(&tileCacheParams, 0, sizeof(tileCacheParams));
			rcVcopy(tileCacheParams.orig, &boundingBox_.min_.x_);
			tileCacheParams.ch = cellHeight_;
			tileCacheParams.cs = cellSize_;
			tileCacheParams.width = tileSize_;
			tileCacheParams.height = tileSize_;
			tileCacheParams.maxSimplificationError = this->detailSampleMaxError_;
			tileCacheParams.maxTiles = numTilesX_ * numTilesZ_ * 8;
			tileCacheParams.maxObstacles = maxObstacles_;
			// Settings from NavigationMesh
			tileCacheParams.walkableClimb = agentMaxClimb_;
			tileCacheParams.walkableHeight = agentHeight_;
			tileCacheParams.walkableRadius = agentRadius_;

			tileCache_ = dtAllocTileCache();
			if (!tileCache_)
			{
				LOGERROR("Could not allocate tile cache");
				ReleaseNavigationMesh();
				return false;
			}

			if (dtStatusFailed(tileCache_->init(&tileCacheParams, allocator_, compressor_, meshProcessor_)))
			{
				LOGERROR("Could not initialize tile cache");
				ReleaseNavigationMesh();
				return false;
			}

			// Build each tile
			unsigned numTiles = 0;

			for (int z = 0; z < numTilesZ_; ++z)
			{
				for (int x = 0; x < numTilesX_; ++x)
				{
					TileCacheData tiles[128];
					int layerCt = BuildTile(geometryList, x, z, tiles);
					for (int i = 0; i < layerCt; ++i)
					{
						dtCompressedTileRef tileRef;
						int status = tileCache_->addTile(tiles[i].data, tiles[i].dataSize, DT_COMPRESSEDTILE_FREE_DATA, &tileRef);
						if (dtStatusFailed(status))
						{
							dtFree(tiles[i].data);
							tiles[i].data = 0x0;
						}
						else
							tileCache_->buildNavMeshTile(tileRef, navMesh_);
					}
					++numTiles;
				}
			}

			for (int z = 0; z < numTilesZ_; ++z)
			{
				for (int x = 0; x < numTilesX_; ++x)
				{
					//tileCache_->buildNavMeshTilesAt(x, z, navMesh_);
				}
			}
			//tileCache_->update(0, navMesh_);

			LOGDEBUG("Built navigation mesh with " + String(numTiles) + " tiles");
			return true;
		}
	}

	bool DynamicNavigationMesh::Build(const BoundingBox& boundingBox)
	{
		PROFILE(BuildPartialNavigationMesh);

		if (!node_)
			return false;

		if (!navMesh_)
		{
			LOGERROR("Navigation mesh must first be built fully before it can be partially rebuilt");
			return false;
		}

		if (!node_->GetWorldScale().Equals(Vector3::ONE))
			LOGWARNING("Navigation mesh root node has scaling. Agent parameters may not work as intended");

		BoundingBox localSpaceBox = boundingBox.Transformed(node_->GetWorldTransform().Inverse());

		float tileEdgeLength = (float)tileSize_ * cellSize_;

		Vector<NavigationGeometryInfo> geometryList;
		CollectGeometries(geometryList);

		int sx = Clamp((int)((localSpaceBox.min_.x_ - boundingBox_.min_.x_) / tileEdgeLength), 0, numTilesX_ - 1);
		int sz = Clamp((int)((localSpaceBox.min_.z_ - boundingBox_.min_.z_) / tileEdgeLength), 0, numTilesZ_ - 1);
		int ex = Clamp((int)((localSpaceBox.max_.x_ - boundingBox_.min_.x_) / tileEdgeLength), 0, numTilesX_ - 1);
		int ez = Clamp((int)((localSpaceBox.max_.z_ - boundingBox_.min_.z_) / tileEdgeLength), 0, numTilesZ_ - 1);

		unsigned numTiles = 0;

		for (int z = sz; z <= ez; ++z)
		{
			for (int x = sx; x <= ex; ++x)
			{
				dtCompressedTileRef existing[128];
				const int existingCt = tileCache_->getTilesAt(x, z, existing, 128);
				for (int i = 0; i < existingCt; ++i)
				{
					unsigned char* data = 0x0;
					if (!dtStatusFailed(tileCache_->removeTile(existing[i], &data, 0)) && data != 0x0)
						dtFree(data);
				}

				TileCacheData tiles[128];
				int layerCt = BuildTile(geometryList, x, z, tiles);
				for (int i = 0; i < layerCt; ++i)
				{
					dtCompressedTileRef tileRef;
					int status = tileCache_->addTile(tiles[i].data, tiles[i].dataSize, DT_COMPRESSEDTILE_FREE_DATA, &tileRef);
					if (dtStatusFailed(status))
					{
						dtFree(tiles[i].data);
						tiles[i].data = 0x0;
					}
					else
					{
						tileCache_->buildNavMeshTile(tileRef, navMesh_);
						++numTiles;
					}
				}
			}
		}

		for (int z = sz; z <= ez; ++z)
		{
			for (int x = sx; x <= ex; ++x)
			{
				//tileCache_->buildNavMeshTilesAt(x, z, navMesh_);
			}
		}
		//tileCache_->update(0, navMesh_);

		LOGDEBUG("Rebuilt " + String(numTiles) + " tiles of the navigation mesh");
		return true;
	}

	int DynamicNavigationMesh::BuildTile(Vector<NavigationGeometryInfo>& geometryList, int x, int z, TileCacheData* tiles)
	{
		PROFILE(BuildNavigationMeshTile);

		tileCache_->removeTile(navMesh_->getTileRefAt(x, z, 0), 0, 0);

		float tileEdgeLength = (float)tileSize_ * cellSize_;

		BoundingBox tileBoundingBox(Vector3(
			boundingBox_.min_.x_ + tileEdgeLength * (float)x,
			boundingBox_.min_.y_,
			boundingBox_.min_.z_ + tileEdgeLength * (float)z
			),
			Vector3(
			boundingBox_.min_.x_ + tileEdgeLength * (float)(x + 1),
			boundingBox_.max_.y_,
			boundingBox_.min_.z_ + tileEdgeLength * (float)(z + 1)
			));

		DynamicNavBuildData build(allocator_);

		rcConfig cfg;
		memset(&cfg, 0, sizeof cfg);
		cfg.cs = cellSize_;
		cfg.ch = cellHeight_;
		cfg.walkableSlopeAngle = agentMaxSlope_;
		cfg.walkableHeight = (int)ceilf(agentHeight_ / cfg.ch);
		cfg.walkableClimb = (int)floorf(agentMaxClimb_ / cfg.ch);
		cfg.walkableRadius = (int)ceilf(agentRadius_ / cfg.cs);
		cfg.maxEdgeLen = (int)(edgeMaxLength_ / cellSize_);
		cfg.maxSimplificationError = edgeMaxError_;
		cfg.minRegionArea = (int)sqrtf(regionMinSize_);
		cfg.mergeRegionArea = (int)sqrtf(regionMergeSize_);
		cfg.maxVertsPerPoly = 6;
		cfg.tileSize = tileSize_;
		cfg.borderSize = cfg.walkableRadius + 3; // Add padding
		cfg.width = cfg.tileSize + cfg.borderSize * 2;
		cfg.height = cfg.tileSize + cfg.borderSize * 2;
		cfg.detailSampleDist = detailSampleDistance_ < 0.9f ? 0.0f : cellSize_ * detailSampleDistance_;
		cfg.detailSampleMaxError = cellHeight_ * detailSampleMaxError_;

		rcVcopy(cfg.bmin, &tileBoundingBox.min_.x_);
		rcVcopy(cfg.bmax, &tileBoundingBox.max_.x_);
		cfg.bmin[0] -= cfg.borderSize * cfg.cs;
		cfg.bmin[2] -= cfg.borderSize * cfg.cs;
		cfg.bmax[0] += cfg.borderSize * cfg.cs;
		cfg.bmax[2] += cfg.borderSize * cfg.cs;

		BoundingBox expandedBox(*reinterpret_cast<Vector3*>(cfg.bmin), *reinterpret_cast<Vector3*>(cfg.bmax));
		GetTileGeometry(&build, geometryList, expandedBox);

		if (build.vertices_.Empty() || build.indices_.Empty())
			return 0; // Nothing to do

		build.heightField_ = rcAllocHeightfield();
		if (!build.heightField_)
		{
			LOGERROR("Could not allocate heightfield");
			return 0;
		}

		if (!rcCreateHeightfield(build.ctx_, *build.heightField_, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs,
			cfg.ch))
		{
			LOGERROR("Could not create heightfield");
			return 0;
		}

		unsigned numTriangles = build.indices_.Size() / 3;
		SharedArrayPtr<unsigned char> triAreas(new unsigned char[numTriangles]);
		memset(triAreas.Get(), 0, numTriangles);

		rcMarkWalkableTriangles(build.ctx_, cfg.walkableSlopeAngle, &build.vertices_[0].x_, build.vertices_.Size(),
			&build.indices_[0], numTriangles, triAreas.Get());
		rcRasterizeTriangles(build.ctx_, &build.vertices_[0].x_, build.vertices_.Size(), &build.indices_[0],
			triAreas.Get(), numTriangles, *build.heightField_, cfg.walkableClimb);
		rcFilterLowHangingWalkableObstacles(build.ctx_, cfg.walkableClimb, *build.heightField_);
		rcFilterLedgeSpans(build.ctx_, cfg.walkableHeight, cfg.walkableClimb, *build.heightField_);
		rcFilterWalkableLowHeightSpans(build.ctx_, cfg.walkableHeight, *build.heightField_);

		build.compactHeightField_ = rcAllocCompactHeightfield();
		if (!build.compactHeightField_)
		{
			LOGERROR("Could not allocate create compact heightfield");
			return 0;
		}
		if (!rcBuildCompactHeightfield(build.ctx_, cfg.walkableHeight, cfg.walkableClimb, *build.heightField_,
			*build.compactHeightField_))
		{
			LOGERROR("Could not build compact heightfield");
			return 0;
		}
		if (!rcErodeWalkableArea(build.ctx_, cfg.walkableRadius, *build.compactHeightField_))
		{
			LOGERROR("Could not erode compact heightfield");
			return 0;
		}

		// area volumes
		for (unsigned i = 0; i < build.navAreas_.Size(); ++i)
			rcMarkBoxArea(build.ctx_, &build.navAreas_[i].bounds_.min_.x_, &build.navAreas_[i].bounds_.max_.x_, build.navAreas_[i].areaID_, *build.compactHeightField_);

		if (this->partitionType_ == NAVMESH_PARTITION_WATERSHED)
		{
			if (!rcBuildDistanceField(build.ctx_, *build.compactHeightField_))
			{
				LOGERROR("Could not build distance field");
				return 0;
			}
			if (!rcBuildRegions(build.ctx_, *build.compactHeightField_, cfg.borderSize, cfg.minRegionArea,
				cfg.mergeRegionArea))
			{
				LOGERROR("Could not build regions");
				return 0;
			}
		}
		else
		{
			if (!rcBuildRegionsMonotone(build.ctx_, *build.compactHeightField_, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea))
			{
				LOGERROR("Could not build monotone regions");
				return 0;
			}
		}

		build.contourSet_ = dtAllocTileCacheContourSet(allocator_);
		if (!build.contourSet_)
		{
			LOGERROR("Could not allocate contour set");
			return 0;
		}
		
		build.heightFieldLayers_ = rcAllocHeightfieldLayerSet();
		if (!build.heightFieldLayers_)
		{
			LOGERROR("Could not allocate height field layer set");
			return 0;
		}

		if (!rcBuildHeightfieldLayers(build.ctx_, *build.compactHeightField_, cfg.borderSize, cfg.walkableHeight, *build.heightFieldLayers_))
		{
			LOGERROR("Could not build height field layers");
			return 0;
		}

		int retCt = 0;
		for (int i = 0; i < build.heightFieldLayers_->nlayers; ++i)
		{
			dtTileCacheLayerHeader header;
			header.magic = DT_TILECACHE_MAGIC;
			header.version = DT_TILECACHE_VERSION;
			header.tx = x;
			header.ty = z;
			header.tlayer = i;

			rcHeightfieldLayer* layer = &build.heightFieldLayers_->layers[0];

			// Tile info.
			rcVcopy(header.bmin, layer->bmin);
			rcVcopy(header.bmax, layer->bmax);
			header.width = (unsigned char)layer->width;
			header.height = (unsigned char)layer->height;
			header.minx = (unsigned char)layer->minx;
			header.maxx = (unsigned char)layer->maxx;
			header.miny = (unsigned char)layer->miny;
			header.maxy = (unsigned char)layer->maxy;
			header.hmin = (unsigned short)layer->hmin;
			header.hmax = (unsigned short)layer->hmax;

			if (dtStatusFailed(dtBuildTileCacheLayer(compressor_/*compressor*/, &header, layer->heights, layer->areas/*areas*/, layer->cons, &(tiles[retCt].data), &tiles[retCt].dataSize)))
			{
				LOGERROR("Failed to build tile cache layers");
				return 0;
			}
			else
				++retCt;
		}

		// Set polygon flags
		/// \todo Allow to define custom flags
		//for (int i = 0; i < build.polyMesh_->npolys; ++i)
		//{
		//	if (build.polyMesh_->areas[i] == RC_WALKABLE_AREA)
		//		build.polyMesh_->flags[i] = 0x1;
		//}

		unsigned char* navData = 0;
		int navDataSize = 0;

		return true;
	}

	void DynamicNavigationMesh::ReleaseNavigationMesh()
	{
		NavigationMesh::ReleaseNavigationMesh();
		ReleaseTileCache();
	}

	void DynamicNavigationMesh::ReleaseTileCache()
	{
		dtFreeTileCache(tileCache_);
		tileCache_ = 0;

		for (unsigned i = 0; i < tileCacheData_.Size(); ++i)
			delete tileCacheData_[i];
		tileCacheData_.Clear();
	}

	void DynamicNavigationMesh::OnNodeSet(Node* node)
	{
		// Subscribe to the scene subsystem update, which will trigger the tile cache to update the nav mesh
		if (node)
		{
			SubscribeToEvent(node, E_SCENESUBSYSTEMUPDATE, HANDLER(DynamicNavigationMesh, HandleSceneSubsystemUpdate));
		}
		else
		{
			UnsubscribeFromAllEvents();
		}
	}

	void DynamicNavigationMesh::AddObstacle(Obstacle* obstacle)
	{
		if (tileCache_)
		{
			float pos[3];
			Vector3 obsPos = obstacle->GetNode()->GetWorldPosition();
			rcVcopy(pos, &obsPos.x_);
			dtObstacleRef refHolder;
			if (dtStatusFailed(tileCache_->addObstacle(pos, obstacle->GetRadius(), obstacle->GetHeight(), &refHolder)))
			{
				LOGERROR("Failed to add obstacle");
				return;
			}
			obstacle->obstacleId_ = refHolder;
		}
	}

	void DynamicNavigationMesh::ObstacleChanged(Obstacle* obstacle)
	{
		if (tileCache_)
		{
			RemoveObstacle(obstacle);
			AddObstacle(obstacle);
		}
	}

	void DynamicNavigationMesh::RemoveObstacle(Obstacle* obstacle)
	{
		if (tileCache_ && obstacle->obstacleId_ > 0)
		{
			if (dtStatusFailed(tileCache_->removeObstacle(obstacle->obstacleId_)))
			{
				LOGERROR("Failed to remove obstacle");
				return;
			}
			obstacle->obstacleId_ = 0;
		}
	}

	void DynamicNavigationMesh::HandleSceneSubsystemUpdate(StringHash eventType, VariantMap& eventData)
	{
		using namespace SceneSubsystemUpdate;

		const float deltaTime = eventData[P_TIMESTEP].GetFloat();
		if (tileCache_ && navMesh_)
		{
			tileCache_->update(deltaTime, navMesh_);
		}
	}
}