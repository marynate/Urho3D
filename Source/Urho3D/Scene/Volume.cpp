#include "Precompiled.h"

#include "../Scene/Volume.h"

#include "../Graphics/DebugRenderer.h"
#include "../Scene/Node.h"
#include "../Core/Context.h"

namespace Urho3D {

static const Vector3 DEFAULT_BOUNDING_BOX_MIN(-10.0f, -10.0f, -10.0f);
static const Vector3 DEFAULT_BOUNDING_BOX_MAX(10.0f, 10.0f, 10.0f);

extern const char* LOGIC_CATEGORY;

Volume::Volume(Context* ctx) : 
	Drawable(ctx, DRAWABLE_ANY) {
	inverseWorldDirty_ = true;
	boundingBox_ = BoundingBox(DEFAULT_BOUNDING_BOX_MIN, DEFAULT_BOUNDING_BOX_MAX);
}

Volume::~Volume() {
}

void Volume::RegisterObject(Context* context) {
	context->RegisterFactory<Volume>(LOGIC_CATEGORY);

	ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
	COPY_BASE_ATTRIBUTES(Drawable);
	ATTRIBUTE("Bounding Box Min", Vector3, boundingBox_.min_, DEFAULT_BOUNDING_BOX_MIN, AM_DEFAULT);
    ATTRIBUTE("Bounding Box Max", Vector3, boundingBox_.max_, DEFAULT_BOUNDING_BOX_MAX, AM_DEFAULT);
}

void Volume::DrawDebugGeometry(DebugRenderer* debug, bool depthTest) {
	if (debug && IsEnabledEffective())
        debug->AddBoundingBox(boundingBox_, node_->GetWorldTransform(), Color::GREEN, depthTest);
}

const Matrix3x4& Volume::GetInverseWorldTransform() const
{
    if (inverseWorldDirty_)
    {
        inverseWorld_ = node_ ? node_->GetWorldTransform().Inverse() : Matrix3x4::IDENTITY;
        inverseWorldDirty_ = false;
    }

    return inverseWorld_;
}

void Volume::OnWorldBoundingBoxUpdate()
{
    worldBoundingBox_ = boundingBox_.Transformed(node_->GetWorldTransform());
}

void Volume::OnMarkedDirty(Node* node)
{
	Drawable::OnMarkedDirty(node);
	inverseWorldDirty_ = true;
}

bool Volume::ContainsPoint(const Vector3& point) const {
	// Use an oriented bounding box test
    Vector3 localPoint(GetInverseWorldTransform() * point);
    return boundingBox_.IsInside(localPoint) != OUTSIDE;
}

bool Volume::ContainsBounds(const BoundingBox& bounds) const {
	BoundingBox bnds(bounds);
	bnds.Transform(GetInverseWorldTransform());
	return boundingBox_.IsInside(bnds) != OUTSIDE;
}

bool Volume::ContainsSphere(const Sphere& sphere) const {
	Sphere s(sphere);
	s.center_ = GetInverseWorldTransform() * s.center_;
	return boundingBox_.IsInside(s) != OUTSIDE;
}

}