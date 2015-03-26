#include "Precompiled.h"
#include "TailGenerator.h"

#include "../Core/Context.h"
#include "../IO/MemoryBuffer.h"
#include "../Scene/Node.h"
#include "../Container/Sort.h"
#include "../Resource/ResourceCache.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/Camera.h"
#include "../Graphics/OctreeQuery.h"
#include "../Graphics/Drawable.h"
#include "../Graphics/Material.h"
#include "../Graphics/DebugRenderer.h"




namespace Urho3D
{

	extern const char* GEOMETRY_CATEGORY;

	TailGenerator::TailGenerator(Context* context) :
		Drawable(context, DRAWABLE_GEOMETRY)
	{

		geometry_[0] = (new Geometry(context));
		vertexBuffer_[0] = (new VertexBuffer(context));
		indexBuffer_[0] = (new IndexBuffer(context));

		geometry_[1] = (new Geometry(context));
		vertexBuffer_[1] = (new VertexBuffer(context));
		indexBuffer_[1] = (new IndexBuffer(context));

		geometry_[0]->SetVertexBuffer(0, vertexBuffer_[0], MASK_POSITION | MASK_COLOR | MASK_TEXCOORD1);
		geometry_[0]->SetIndexBuffer(indexBuffer_[0]);

		geometry_[1]->SetVertexBuffer(0, vertexBuffer_[1], MASK_POSITION | MASK_COLOR | MASK_TEXCOORD1);
		geometry_[1]->SetIndexBuffer(indexBuffer_[1]);

		indexBuffer_[0]->SetShadowed(false);
		indexBuffer_[1]->SetShadowed(false);

		batches_.Resize(2);
		batches_[0].geometry_ = geometry_[0];
		batches_[0].geometryType_ = GEOM_BILLBOARD;
		batches_[0].worldTransform_ = &transforms_[0];

		batches_[1].geometry_ = geometry_[1];
		batches_[1].geometryType_ = GEOM_BILLBOARD;
		batches_[1].worldTransform_ = &transforms_[0];

		forceUpdateVertexBuffer_ = false;
		previousPosition_ = Vector3::ZERO;

		tailNum_ = 10;
		// for debug
		//ResourceCache* cache = GetSubsystem<ResourceCache>();
		//SetMaterial(cache->GetResource<Material>("Materials/TailGenerator.xml"));
		scale_ = 1.0f; // default side scale
		tailTipColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
		tailHeadColor = Color(1.0f, 1.0f, 1.0f, 1.0f);

		forceUpdateVertexBuffer_ = false;

		bbmax = Vector3::ZERO;
		bbmin = Vector3::ZERO;
		vertical_ = horizontal_ = true;
	}

	TailGenerator::~TailGenerator()
	{
	}

	void TailGenerator::RegisterObject(Context* context)
	{
		context->RegisterFactory<TailGenerator>(GEOMETRY_CATEGORY);

		COPY_BASE_ATTRIBUTES(Drawable);
		MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("Segments", GetNumTails, SetNumTails, unsigned int, 10, AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("Length", GetTailLength, SetTailLength, float, 1.0f, AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("Width", GetWidthScale, SetWidthScale, float, 1.0f, AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("Start Color", GetColorForHead, SetColorForHead, Color, Color(1.0f, 1.0f, 1.0f, 1.0f), AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("End Color", GetColorForTip, SetColorForTip, Color, Color(1.0f, 1.0f, 1.0f, 1.0f), AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("Draw Vertical", GetDrawVertical, SetDrawVertical, bool, true, AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("Draw Horizontal", GetDrawHorizontal, SetDrawHorizontal, bool, true, AM_DEFAULT);
	}


	void TailGenerator::ProcessRayQuery(const RayOctreeQuery& query, PODVector<RayQueryResult>& results)
	{
		// If no billboard-level testing, use the Drawable test
		if (query.level_ < RAY_TRIANGLE)
		{
			Drawable::ProcessRayQuery(query, results);
			return;
		}
	}



	void TailGenerator::Update(const FrameInfo &frame)
	{
		Drawable::Update(frame);
		UpdateTail();
	}

	void TailGenerator::UpdateTail()
	{
		//Drawable::Update(frame);

		Vector3 wordPosition = node_->GetWorldPosition();
		float path = (previousPosition_ - wordPosition).Length();

		if (path > tailLength_)
		{
			// ????? ????? ????
			Tail newPoint;
			newPoint.position = wordPosition;

			Vector3 forwardmotion = (previousPosition_ - wordPosition).Normalized();
			Vector3 rightmotion = forwardmotion.CrossProduct(Vector3::UP);
			rightmotion.Normalize();
			newPoint.worldRight = rightmotion;
			newPoint.forward = forwardmotion;

			//forceBuildMeshInWorkerThread_ = true;
			forceUpdateVertexBuffer_ = true;
			previousPosition_ = wordPosition;

			fullPointPath.Push(newPoint);	// ???? ????, ??? ????? ?? ??? ????? ?????? ??????????.
			//knots.Push(wordPosition);		// ??? ??????? ???????
		}

	}

	void  TailGenerator::DrawDebugGeometry(DebugRenderer *debug, bool depthTest)
	{
		Drawable::DrawDebugGeometry(debug, depthTest);

		debug->AddNode(node_);

		for (int i = 0; i < ((int)tails_.Size()) - 1; i++)
		{
			debug->AddLine(tails_[i].position, tails_[i + 1].position, Color(1, 1, 1).ToUInt(), false);

		}

	}


	void TailGenerator::UpdateBatches(const FrameInfo& frame)
	{

		distance_ = frame.camera_->GetDistance(GetWorldBoundingBox().Center());

		batches_[0].distance_ = distance_;
		batches_[0].numWorldTransforms_ = 2;

		if (batches_.Size() > 1)
		{
			batches_[1].distance_ = distance_;
			batches_[1].numWorldTransforms_ = 2;
		}

		// TailGenerator positioning
		transforms_[0] = Matrix3x4::IDENTITY;
		// TailGenerator rotation
		transforms_[1] = Matrix3x4(Vector3::ZERO, Quaternion(0, 0, 0), Vector3::ONE);
	}

	void TailGenerator::UpdateGeometry(const FrameInfo& frame)
	{
		if (bufferSizeDirty_ || indexBuffer_[0]->IsDataLost() || indexBuffer_[1]->IsDataLost())
			UpdateBufferSize();

		if (bufferDirty_ || vertexBuffer_[0]->IsDataLost() || vertexBuffer_[1]->IsDataLost() || forceUpdateVertexBuffer_)
			UpdateVertexBuffer(frame);
	}

	UpdateGeometryType TailGenerator::GetUpdateGeometryType()
	{
		if (bufferDirty_ || bufferSizeDirty_ || vertexBuffer_[0]->IsDataLost() || indexBuffer_[0]->IsDataLost() ||
			vertexBuffer_[1]->IsDataLost() || indexBuffer_[1]->IsDataLost() || forceUpdateVertexBuffer_)
			return UPDATE_MAIN_THREAD;
		else
			return UPDATE_NONE;

	}

	void TailGenerator::SetMaterial(Material* material)
	{
		batches_[0].material_ = material;
		batches_[1].material_ = material;

		MarkNetworkUpdate();
	}

	void TailGenerator::OnNodeSet(Node* node)
	{
		Drawable::OnNodeSet(node);
	}

	void TailGenerator::OnWorldBoundingBoxUpdate()
	{

		//worldBoundingBox_.Define(-M_LARGE_VALUE, M_LARGE_VALUE);
		worldBoundingBox_.Merge(bbmin);
		worldBoundingBox_.Merge(bbmax);
		worldBoundingBox_.Merge(node_->GetWorldPosition());

	}

	/// Resize TailGenerator vertex and index buffers.
	void TailGenerator::UpdateBufferSize()
	{
		unsigned numTails = tailNum_;

		if (!numTails)
			return;

		if (vertexBuffer_[0]->GetVertexCount() != (numTails * 2))
		{
			vertexBuffer_[0]->SetSize((numTails * 2), MASK_POSITION | MASK_COLOR | MASK_TEXCOORD1, true);
			vertexBuffer_[1]->SetSize((numTails * 2), MASK_POSITION | MASK_COLOR | MASK_TEXCOORD1, true);

		}
		if (indexBuffer_[0]->GetIndexCount() != (numTails * 2))
		{
			indexBuffer_[0]->SetSize((numTails * 2), false);
			indexBuffer_[1]->SetSize((numTails * 2), false);
		}

		bufferSizeDirty_ = false;
		bufferDirty_ = true;

		// Indices do not change for a given tail generator capacity
		unsigned short* dest = (unsigned short*)indexBuffer_[0]->Lock(0, (numTails * 2), true);
		if (!dest)
			return;

		unsigned vertexIndex = 0;
		unsigned stripsLen = numTails;

		while (stripsLen--)
		{

			dest[0] = vertexIndex;
			dest[1] = vertexIndex + 1;
			dest += 2;
			vertexIndex += 2;
		}

		indexBuffer_[0]->Unlock();
		indexBuffer_[0]->ClearDataLost();

		unsigned short* dest2 = (unsigned short*)indexBuffer_[1]->Lock(0, (numTails * 2), true);
		if (!dest2)
			return;

		vertexIndex = 0;
		stripsLen = numTails;

		while (stripsLen--)
		{
			dest2[0] = vertexIndex;
			dest2[1] = vertexIndex + 1;
			dest2 += 2;
			vertexIndex += 2;
		}

		indexBuffer_[1]->Unlock();
		indexBuffer_[1]->ClearDataLost();


	}

	/// Rewrite TailGenerator vertex buffer.
	void TailGenerator::UpdateVertexBuffer(const FrameInfo& frame)
	{
		unsigned fullPointPathSize = fullPointPath.Size();
		unsigned currentVisiblePathSize = tailNum_;

		// Clear previous mesh data
		tailMesh[0].Clear();
		tailMesh[1].Clear();

		// build tail

		// if tail path is short and nothing to draw, exit
		if (fullPointPathSize < 2) return;

		activeTails.Clear();

		unsigned min_i = fullPointPathSize < currentVisiblePathSize ? 0 : fullPointPathSize - currentVisiblePathSize;
		// Step 1 : collect actual point's info for build tail path
		for (unsigned i = min_i; i < fullPointPathSize - 1; i++)
		{
			activeTails.Push(fullPointPath[i]);

			Vector3 &p = fullPointPath[i].position;

			// Math BoundingBox based on actual point
			if (p.x_ < bbmin.x_) bbmin.x_ = p.x_;
			if (p.y_ < bbmin.y_) bbmin.y_ = p.y_;
			if (p.z_ < bbmin.z_) bbmin.z_ = p.z_;

			if (p.x_ > bbmax.x_) bbmax.x_ = p.x_;
			if (p.y_ > bbmax.y_) bbmax.y_ = p.y_;
			if (p.z_ > bbmax.z_) bbmax.z_ = p.z_;

		}

		if (activeTails.Size() < 2) return;

		Vector<Tail> &t = activeTails;

		// generate strips of tris
		TailVertex v;

		float mixFactor = 1.0f / activeTails.Size();


		// Forward part of tail (strip in xz plane)
		for (unsigned i = 0; i < activeTails.Size(); ++i)
		{
			Color c = tailTipColor.Lerp(tailHeadColor, mixFactor * i);
			v.color_ = c.ToUInt();
			v.uv_ = Vector2(1.0f, 0.0f);
			v.position_ = t[i].position + t[i].worldRight * scale_;
			tailMesh[0].Push(v);

			//v.color_ = c.ToUInt();
			v.uv_ = Vector2(0.0f, 1.0f);
			v.position_ = t[i].position - t[i].worldRight * scale_;
			tailMesh[0].Push(v);

		}

		// Upper part of tail (strip in xy-plane)
		for (unsigned i = 0; i < activeTails.Size(); ++i)
		{
			Color c = tailTipColor.Lerp(tailHeadColor, mixFactor * i);
			v.color_ = c.ToUInt();
			v.uv_ = Vector2(1.0f, 0.0f);
			Vector3 up = t[i].forward.CrossProduct(t[i].worldRight);
			up.Normalize();
			v.position_ = t[i].position + up * scale_;
			tailMesh[1].Push(v);

			//v.color_ = c.ToUInt();
			v.uv_ = Vector2(0.0f, 1.0f);
			v.position_ = t[i].position - up * scale_;
			tailMesh[1].Push(v);

		}


		// copy new mesh to vertex buffer
		unsigned meshVertexCount = tailMesh[0].Size();
		batches_[0].geometry_->SetDrawRange(TRIANGLE_STRIP, 0, meshVertexCount, false);
		// get pointer
		TailVertex* dest = (TailVertex*)vertexBuffer_[0]->Lock(0, meshVertexCount, true);
		if (!dest)
			return;
		// copy to vertex buffer
		memcpy(dest, &tailMesh[0][0], tailMesh[0].Size() * sizeof(TailVertex));

		vertexBuffer_[0]->Unlock();
		vertexBuffer_[0]->ClearDataLost();

		if (batches_.Size() > 1)
		{
			// copy new mesh to vertex buffer
			// get pointer
			batches_[1].geometry_->SetDrawRange(TRIANGLE_STRIP, 0, meshVertexCount, false);

		}
		TailVertex* dest2 = (TailVertex*)vertexBuffer_[1]->Lock(0, meshVertexCount, true);
		if (!dest2)
			return;
		// copy to vertex buffer
		memcpy(dest2, &tailMesh[1][0], tailMesh[1].Size() * sizeof(TailVertex));

		vertexBuffer_[1]->Unlock();
		vertexBuffer_[1]->ClearDataLost();

		bufferDirty_ = false;

		// unmark flag
		forceUpdateVertexBuffer_ = false;
	}

	void TailGenerator::SetDrawVertical(bool value)
	{
		vertical_ = value;
		SetupBatches();
	}

	void TailGenerator::SetDrawHorizontal(bool value)
	{
		horizontal_ = value;
		SetupBatches();
	}

	void TailGenerator::SetupBatches()
	{
		if (horizontal_ && vertical_)
		{
			if (batches_.Size() < 2)
			{
				batches_[0].geometry_ = geometry_[0];
				batches_[0].geometryType_ = GEOM_BILLBOARD;
				batches_[0].worldTransform_ = &transforms_[0];
				
				batches_.Push(SourceBatch());
				batches_[1].geometry_ = geometry_[1];
				batches_[1].material_ = batches_[0].material_;
				batches_[1].geometryType_ = GEOM_BILLBOARD;
				batches_[1].worldTransform_ = &transforms_[0];
			}
		}
		else if (horizontal_) {
			batches_.Resize(1);
			batches_[0].geometry_ = geometry_[0];
			batches_[0].geometryType_ = GEOM_BILLBOARD;
			batches_[0].worldTransform_ = &transforms_[0];
		}
		else if (vertical_)
		{
			batches_.Resize(1);
			batches_[0].geometry_ = geometry_[1];
			batches_[0].geometryType_ = GEOM_BILLBOARD;
			batches_[0].worldTransform_ = &transforms_[0];
		}
	}


	void TailGenerator::SetTailLength(float length)
	{

		tailLength_ = length;
	}

	float TailGenerator::GetTailLength() const
	{
		return tailLength_;
	}

	void TailGenerator::SetColorForTip(const Color& c)
	{
		tailTipColor = Color(c.r_, c.g_, c.b_, 0.0f);
	}

	void TailGenerator::SetColorForHead(const Color& c)
	{
		tailHeadColor = Color(c.r_, c.g_, c.b_, 1.0f);
	}

	void TailGenerator::SetNumTails(unsigned num)
	{
		// Prevent negative value being assigned from the editor
		if (num > M_MAX_INT)
			num = 0;

		if (num > MAX_TAILS)
			num = MAX_TAILS;

		bufferSizeDirty_ = true;
		tailNum_ = num;
	}

	unsigned TailGenerator::GetNumTails() const
	{
		return tailNum_;
	}

	void TailGenerator::MarkPositionsDirty()
	{
		Drawable::OnMarkedDirty(node_);
		bufferDirty_ = true;
	}

	void TailGenerator::SetMaterialAttr(const ResourceRef& value)
	{
		ResourceCache* cache = GetSubsystem<ResourceCache>();
		SetMaterial(cache->GetResource<Material>(value.name_));
	}

	ResourceRef TailGenerator::GetMaterialAttr() const
	{
		return GetResourceRef(batches_[0].material_, Material::GetTypeStatic());
	}

	void TailGenerator::SetWidthScale(float scale)
	{
		scale_ = scale;
	}

}