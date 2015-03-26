#include "Precompiled.h"

#include "SteeredObject.h"
#include "Steering.h"

namespace Urho3D
{

void SteeredObject::Seek(const Vector3& target) 
{
	velocity = Steering::Seek(position, target);
}

void SteeredObject::ArriveAt(const Vector3& target) 
{ //seek Scaled
	velocity = Steering::ArriveAt(position, target, velocity.Length());
}

void SteeredObject::Intercept(SteeredObject* other)
{
	velocity = Steering::Intercept(position, other->position, other->velocity);
}

void SteeredObject::Evade(SteeredObject* other)
{
	velocity = Steering::Evade(position, other->position, other->velocity);
}

void SteeredObject::Avoid(const Vector<SteeredObject*>& others)
{
	Steering::Avoid(this, others);
}

void SteeredObject::Separate(const Vector<SteeredObject*>& others)
{
	Steering::Separate(this, others, 1);
}

void SteeredObject::FacingCohesion(const Vector<SteeredObject*>& others)
{
	Steering::FacingCohesion(this, others);
}

void SteeredObject::PositionCohesion(const Vector<SteeredObject*>& others)
{
	Steering::PositionCohesion(this, others);
}

}