#include "Precompiled.h"

#include "../Physics/JiggleBone.h"

#include "../Core/Context.h"
#include "../Graphics/DebugRenderer.h"
#include "../Scene/Node.h"

/*
Port of http://wiki.unity3d.com/index.php?title=JiggleBone
to Urho3D
*/

namespace Urho3D
{

extern const char* PHYSICS_CATEGORY;

JiggleBone::JiggleBone(Context* context) :
	LogicComponent(context)
{
	SetUpdateEventMask(USE_POSTUPDATE);
	// Bone settings
	boneAxis_ = Vector3(0, 0, 1);
	targetDistance_ = 2.0f;

	// Dynamics settings
	stiffness_ = 0.1f;
	mass_ = 0.9f;
	damping_ = 0.75f;
	gravity_ = 0.75f;

	// Squash and stretch variables
	squashAndStretch_ = true;
	sideStretch_ = 0.15f;
	frontStretch_ = 0.2f;
}

JiggleBone::~JiggleBone()
{

}

void JiggleBone::RegisterObject(Context* context)
{
	context->RegisterFactory<JiggleBone>(PHYSICS_CATEGORY);

	COPY_BASE_ATTRIBUTES(LogicComponent);
	ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
	ATTRIBUTE("Bone Axis", Vector3, boneAxis_, Vector3(0, 0, 1), AM_DEFAULT);

	ATTRIBUTE("Stiffness", float, stiffness_, 0.1f, AM_DEFAULT);
	ATTRIBUTE("Mass", float, mass_, 0.9f, AM_DEFAULT);
	ATTRIBUTE("Damping", float, damping_, 0.75f, AM_DEFAULT);
	ATTRIBUTE("Gravity", float, gravity_, 0.75f, AM_DEFAULT);

	ATTRIBUTE("Allow Stretch", bool, squashAndStretch_, true, AM_DEFAULT);
	ATTRIBUTE("Side Stretch", float, sideStretch_, 0.15f, AM_DEFAULT);
	ATTRIBUTE("Front Stretch", float, frontStretch_, 0.15f, AM_DEFAULT);
}

void JiggleBone::DelayedStart()
{
	targetPos_ = GetNode()->GetWorldPosition() + GetNode()->GetWorldDirection() * targetDistance_;
	dynamicPos_ = targetPos_;
}

void JiggleBone::PostUpdate(float timeStep)
{
	if (!IsEnabledEffective())
		return;
	// Update forwardVector and upVector
	Vector3 forwardVector = GetNode()->GetWorldDirection() * targetDistance_;
	Vector3 upVector = GetNode()->GetWorldUp();

	// Calculate target position
	Vector3 targetPos = GetNode()->GetWorldPosition() + forwardVector;

	// Calculate force, acceleration, and velocity per X, Y and Z
	force_.x_ = (targetPos.x_ - dynamicPos_.x_) * stiffness_;
	acc_.x_ = force_.x_ / mass_;
	vel_.x_ += acc_.x_ * (1 - damping_);

	force_.y_ = (targetPos.y_ - dynamicPos_.y_) * stiffness_;
	force_.y_ -= gravity_ / 10; // Add some gravity
	acc_.y_ = force_.y_ / mass_;
	vel_.y_ += acc_.y_ * (1 - damping_);

	force_.z_ = (targetPos.z_ - dynamicPos_.z_) * stiffness_;
	acc_.z_ = force_.z_ / mass_;
	vel_.z_ += acc_.z_ * (1 - damping_);

	// Update dynamic postion
	dynamicPos_ += vel_ + force_;

	// Set bone rotation to look at dynamicPos
	GetNode()->LookAt(dynamicPos_, upVector);

	// ==================================================
	// Squash and Stretch section
	// ==================================================
	if (squashAndStretch_){
		// Create a vector from target position to dynamic position
		// We will measure the magnitude of the vector to determine
		// how much squash and stretch we will apply
		Vector3 dynamicVec = dynamicPos_ - targetPos;

		// Get the magnitude of the vector
		float stretchMag = dynamicVec.Length();

		// Here we determine the amount of squash and stretch based on stretchMag
		// and the direction the Bone Axis is pointed in. Ideally there should be
		// a vector with two values at 0 and one at 1. Like Vector3(0,0,1)
		// for the 0 values, we assume those are the sides, and 1 is the direction
		// the bone is facing
		float xStretch;
		if (boneAxis_.x_ == 0)
			xStretch = 1 + (-stretchMag * sideStretch_);
		else 
			xStretch = 1 + (stretchMag * frontStretch_);

		float yStretch;
		if (boneAxis_.y_ == 0)
			yStretch = 1 + (-stretchMag * sideStretch_);
		else 
			yStretch = 1 + (stretchMag * frontStretch_);

		float zStretch;
		if (boneAxis_.z_ == 0)
			zStretch = 1 + (-stretchMag * sideStretch_);
		else 
			zStretch = 1 + (stretchMag * frontStretch_);

		// Set the bone scale
		GetNode()->SetScale(Vector3(xStretch, yStretch, zStretch));
	}
}

void JiggleBone::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
	if (IsEnabledEffective())
	{
		// draw forward line
		debug->AddLine(GetNode()->GetWorldPosition(), 
			GetNode()->GetWorldPosition() + GetNode()->GetWorldDirection() * targetDistance_, 
			Color(0, 0, 1), false);
		// draw the up vector
		debug->AddLine(GetNode()->GetWorldPosition(),
			GetNode()->GetWorldPosition() + GetNode()->GetWorldUp() * (targetDistance_ * 0.5f),
			Color(0, 1, 0), false);
		// draw the target position
		debug->AddLine(targetPos_, 
			Vector3::UP * (targetDistance_ * 0.2f), 
			Color(1, 1, 0), false);
		// draw the dynamic position
		debug->AddLine(dynamicPos_,
			Vector3::UP * (targetDistance_ * 0.2f),
			Color(1, 0, 0), false);
	}
}

}