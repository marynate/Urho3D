#include "Precompiled.h"

#include "../Graphics/Connector.h"

#include "../Core/Context.h"
#include "../Graphics/Camera.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/Material.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Scene.h"

namespace Urho3D
{
	extern const char* GEOMETRY_CATEGORY;

Connector::Connector(Context* context) :
	Drawable(context, DRAWABLE_GEOMETRY)
{
	geometry_ = (new Geometry(context));
	vertexBuffer_ = (new VertexBuffer(context));
	indexBuffer_ = (new IndexBuffer(context));

	geometry_->SetVertexBuffer(0, vertexBuffer_, MASK_POSITION | MASK_TEXCOORD1);
	geometry_->SetIndexBuffer(indexBuffer_);

	indexBuffer_->SetShadowed(false);

	batches_.Resize(1);
	batches_[0].geometry_ = geometry_;
	batches_[0].geometryType_ = GEOM_BILLBOARD;
	batches_[0].worldTransform_ = &transforms_[0];

	segments_ = 10;
	applyGravity_ = false;
	width_ = 1.0f;
	gravityTolerance_ = 0.5f;
	alignToView_ = false;
}

Connector::~Connector()
{

}

void Connector::RegisterObject(Context* context)
{
	context->RegisterFactory<Connector>(GEOMETRY_CATEGORY);

	MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
	ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
	ACCESSOR_ATTRIBUTE("Width", GetWidth, SetWidth, float, 1.0f, AM_DEFAULT);
	ACCESSOR_ATTRIBUTE("Segments", GetSegments, SetSegments, int, 10, AM_DEFAULT);
	ACCESSOR_ATTRIBUTE("Apply Gravity", ApplyGravity, SetApplyGravity, bool, false, AM_DEFAULT);
	ACCESSOR_ATTRIBUTE("Gravity Tolerance", GetGravityTolerance, SetGravityTolerance, float, 0.5f, AM_DEFAULT);
	ACCESSOR_ATTRIBUTE("Align To View", GetAlignToView, SetAlignToView, bool, false, AM_DEFAULT);
	ACCESSOR_ATTRIBUTE("Target Node", GetTargetNodeID, SetTargetNodeID, int, -1, AM_DEFAULT);
}

void Connector::Update(const FrameInfo &frame)
{
	Drawable::Update(frame);
	UpdateGeometry();
	OnWorldBoundingBoxUpdate();
}

void Connector::UpdateBatches(const FrameInfo& frame)
{
	distance_ = frame.camera_->GetDistance(GetWorldBoundingBox().Center());

	batches_[0].distance_ = distance_;
	batches_[0].numWorldTransforms_ = 2;

	// TailGenerator positioning
	transforms_[0] = Matrix3x4::IDENTITY;
	// TailGenerator rotation
	transforms_[1] = Matrix3x4(Vector3::ZERO, Quaternion(0, 0, 0), Vector3::ONE);
}

void Connector::UpdateGeometry(const FrameInfo& frame)
{
	if (bufferDirty_ || indexBuffer_->IsDataLost())
		UpdateBufferSize();

	if (bufferDirty_ || vertexBuffer_->IsDataLost())
		UpdateVertexBuffer(frame);
}

UpdateGeometryType Connector::GetUpdateGeometryType()
{
	if (bufferDirty_ || bufferDirty_
		|| vertexBuffer_->IsDataLost() || indexBuffer_->IsDataLost())
		return UPDATE_MAIN_THREAD;
	else
		return UPDATE_NONE;
}

void Connector::OnNodeSet(Node* node)
{
	Drawable::OnNodeSet(node);
}

void Connector::OnWorldBoundingBoxUpdate()
{
	//worldBoundingBox_.Define(-M_LARGE_VALUE, M_LARGE_VALUE);
	Vector3 myPos = GetNode()->GetWorldPosition();
	Vector3 otherPos = otherNode_.Expired() ? myPos : otherNode_->GetWorldPosition();

	worldBoundingBox_.Merge(myPos);
	worldBoundingBox_.Merge(otherPos);
}

void  Connector::DrawDebugGeometry(DebugRenderer *debug, bool depthTest)
{
	Drawable::DrawDebugGeometry(debug, depthTest);
	debug->AddNode(node_);
	if (!otherNode_.Expired())
	{
		SharedPtr<Node> nd = otherNode_.Lock();
		debug->AddLine(GetNode()->GetWorldPosition(), nd->GetWorldPosition(), Color(0, 1, 0), false);
	}
}

void Connector::SetWidth(float aValue)
{
	width_ = aValue;
	UpdateGeometry();
}

void Connector::SetAlignToView(bool aValue)
{
	alignToView_ = aValue;
	UpdateGeometry();
}

void Connector::SetSegments(int aValue)
{
	segments_ = aValue;
	UpdateGeometry();
}

void Connector::SetApplyGravity(bool aValue)
{
	applyGravity_ = aValue;
	UpdateGeometry();
}

void Connector::SetGravityTolerance(float aValue)
{
	gravityTolerance_ = aValue;
	UpdateGeometry();
}

void Connector::SetTargetNodeID(int aValue)
{
	targetNode_ = aValue;
	if (targetNode_ != -1)
	{
		otherNode_ = GetScene()->GetNode((unsigned int)targetNode_);
		if (!otherNode_.Expired())
		{
			UpdateGeometry();
		}
	}
}

void Connector::SetMaterialAttr(const ResourceRef& value)
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	SetMaterial(cache->GetResource<Material>(value.name_));
}

ResourceRef Connector::GetMaterialAttr() const
{
	return GetResourceRef(batches_[0].material_, Material::GetTypeStatic());
}

void Connector::SetMaterial(Material* material)
{
	batches_[0].material_ = material;
	MarkNetworkUpdate();
}

void Connector::UpdateGeometry()
{
	bufferDirty_ = true;
}

void Connector::UpdateBufferSize()
{
	unsigned segs = segments_ + 1;

	if (!segs)
		return;
	
	const int vertsPerSegment = 2;
	if (vertexBuffer_->GetVertexCount() != (segs * vertsPerSegment))
	{
		vertexBuffer_->SetSize((segs * vertsPerSegment), MASK_POSITION | MASK_TEXCOORD1, true);
	}
	if (indexBuffer_->GetIndexCount() != (segs * vertsPerSegment))
	{
		indexBuffer_->SetSize((segs* vertsPerSegment) + segs, false);
	}
	bufferDirty_ = true;
	
	// build the index buffer
	unsigned short* dest = (unsigned short*)indexBuffer_->Lock(0, (segs * vertsPerSegment), true);
	if (!dest)
		return;

	unsigned vertexIndex = 0;
	unsigned stripsLen = segs;
	while (stripsLen--)
	{
		dest[0] = vertexIndex;
		dest[1] = vertexIndex + 1;
		dest += 2;
		vertexIndex += 2;
	}

	indexBuffer_->Unlock();
	indexBuffer_->ClearDataLost();
}

void Connector::UpdateVertexBuffer(const FrameInfo& frame)
{
	if (segments_ < 1 || otherNode_.Expired())
		return;
	Vector<Vector3> points;
	Vector<Vector2> integrationList;
	SharedPtr<Node> otherNode = otherNode_.Lock();
	const float totalDistance = (otherNode->GetWorldPosition() - GetNode()->GetWorldPosition()).Length();
	const float distance = totalDistance / ((float)segments_);

	for (unsigned i = 0; i < segments_ + 1; ++i)
	{
		Vector3 vec = otherNode_->GetWorldPosition().Lerp(GetNode()->GetWorldPosition(), ((float)i)/((float)segments_));
		points.Push(vec);
		if (applyGravity_)
			integrationList.Push(Vector2(vec.x_, vec.y_));
	}

	if (integrationList.Size() > 2 && applyGravity_)
		Integrate(integrationList, distance);

	const Vector3 camViewDir = frame.camera_->GetNode()->GetDirection();
	const Vector3 camUp = frame.camera_->GetNode()->GetWorldUp();
	const Vector3 camPos = frame.camera_->GetNode()->GetWorldPosition();
	const Vector3 camRight = frame.camera_->GetNode()->GetWorldRight();

	Vector<ConnectorVertex> verts;
	ConnectorVertex v;
	for (unsigned i = 0; i < points.Size(); ++i)
	{
		if (i < integrationList.Size())
			points[i].y_ = integrationList[i].y_;
		
		Vector3 ptDir = (points[i] - camPos).Normalized();
		Vector3 up = Vector3::UP;
		if (alignToView_)
		{
			if (camUp.Angle(ptDir) > camRight.Angle(ptDir))
				up = camUp;
			else
				up = camRight;
		}

		v.uv_ = Vector2(1.0f, 0.0f);
		v.position_ = points[i] + up * width_;
		verts.Push(v);

		v.uv_ = Vector2(0.0f, 1.0f);
		v.position_ = points[i] - up * width_;
		verts.Push(v);
	}

	// copy new mesh to vertex buffer
	unsigned meshVertexCount = verts.Size();
	batches_[0].geometry_->SetDrawRange(TRIANGLE_STRIP, 0, meshVertexCount, false);
	// get pointer
	ConnectorVertex* dest = (ConnectorVertex*)vertexBuffer_->Lock(0, meshVertexCount, true);
	if (!dest)
		return;
	// copy to vertex buffer
	memcpy(dest, &verts[0], verts.Size() * sizeof(ConnectorVertex));

	vertexBuffer_->Unlock();
	vertexBuffer_->ClearDataLost();

	bufferDirty_ = false;
}

void Connector::Integrate(Vector<Vector2>& aList, float aSpacing)
{
	// 4 passes for integration
	const float dstSquared = aSpacing * aSpacing;
	for (unsigned i = 0; i < 8; ++i)
	{
		for (unsigned v = 1; v < aList.Size() - 1; ++v)
		{
			const Vector2& left = aList[v - 1];
			const Vector2& right = aList[v + 1];
			Vector2& point = aList[v];
			Vector2 tmpPoint = point;

			int unitDist = aList.Size() - v;
			unitDist = Min(unitDist, v);

			const float leftDist = (point - left).Length();
			const float rightDist = (point - right).Length();
			if (leftDist < aSpacing + (aSpacing * gravityTolerance_) || rightDist < aSpacing + (aSpacing * gravityTolerance_))
				point.y_ -= (((aSpacing + (aSpacing * gravityTolerance_)) - Max(leftDist, rightDist))) * 0.15f * unitDist;
		}
	}
}

}