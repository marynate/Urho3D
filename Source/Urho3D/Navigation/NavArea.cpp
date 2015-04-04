
#include "Precompiled.h"
#include "../Core/Context.h"
#include "../Graphics/DebugRenderer.h"
#include "../Scene/Component.h"
#include "../Scene/Node.h"
#include "../Navigation/NavArea.h"

namespace Urho3D
{
	static const Vector3 DEFAULT_BOUNDING_BOX_MIN(-10.0f, -10.0f, -10.0f);
	static const Vector3 DEFAULT_BOUNDING_BOX_MAX(10.0f, 10.0f, 10.0f);

	extern const char* NAVIGATION_CATEGORY;

	NavArea::NavArea(Context* context) :
		Component(context),
		flags_(0),
		areaType_(0),
		boundingBox_(DEFAULT_BOUNDING_BOX_MIN, DEFAULT_BOUNDING_BOX_MAX)
	{

	}

	NavArea::~NavArea()
	{

	}
	
	void NavArea::RegisterObject(Context* context)
	{
		context->RegisterFactory<NavArea>();

		COPY_BASE_ATTRIBUTES(Component);
		ATTRIBUTE("Bounding Box Min", Vector3, boundingBox_.min_, DEFAULT_BOUNDING_BOX_MIN, AM_DEFAULT);
		ATTRIBUTE("Bounding Box Max", Vector3, boundingBox_.max_, DEFAULT_BOUNDING_BOX_MAX, AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("Flags Mask", GetFlags, SetFlags, unsigned, 0, AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("Area Type", GetAreaType, SetAreaType, unsigned, 0, AM_DEFAULT);
	}

	void NavArea::SetAreaType(unsigned newType)
	{
		areaType_ = newType;
	}

	void NavArea::SetFlags(unsigned newFlags)
	{
		flags_ = newFlags;
	}

	BoundingBox NavArea::GetTransformedBounds() const
	{
		return boundingBox_.Transformed(node_->GetWorldTransform());
	}

	void NavArea::DrawDebugGeometry(DebugRenderer* debug, bool depthTest) {
		if (debug && IsEnabledEffective())
			debug->AddBoundingBox(boundingBox_, node_->GetWorldTransform(), Color::GREEN, depthTest);
	}

	const Matrix3x4& NavArea::GetInverseWorldTransform() const
	{
		if (inverseWorldDirty_)
		{
			inverseWorld_ = node_ ? node_->GetWorldTransform().Inverse() : Matrix3x4::IDENTITY;
			inverseWorldDirty_ = false;
		}

		return inverseWorld_;
	}

	void NavArea::OnMarkedDirty(Node* node)
	{
		Component::OnMarkedDirty(node);
		inverseWorldDirty_ = true;
	}
}