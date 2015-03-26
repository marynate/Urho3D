#include "Precompiled.h"

#include "../Math/MathDefs.h"
#include "../AI/Steering.h"

namespace Urho3D
{
namespace Steering
{

Vector3 Seek(const Vector3& pos, const Vector3& dest)
{
	return (dest - pos).Normalized();
}

Vector3 ArriveAt(const Vector3& pos, const Vector3& target, float speed) //seek Scaled
{
	float desiredSpeed = speed * (target- pos).Length();
    return desiredSpeed * (Seek(pos, target)).Normalized();
}

Vector3 Intercept(const Vector3& pos, const Vector3& target, const Vector3& targetVelocity) {
	return Seek(pos, target + targetVelocity);
}

Vector3 Evade(const Vector3& pos, const Vector3& target, const Vector3& targetVelocity) {
	return Intercept(pos,target,targetVelocity) * -1;
}

float VectorAngle(const Vector3& a, const Vector3& b)
{
	return Urho3D::Acos(a.Normalized().DotProduct(b.Normalized()));
}

Vector3 Project(Vector3 in, Vector3 other) {
	float hyp = in.Length();
	float adjacent = hyp * Urho3D::Cos(VectorAngle(other,in));
	return adjacent * other.Normalized();
}

void Avoid(SteeredObject* obj, const Vector<SteeredObject*>& others)
{
	float closest = obj->velocity.Length();
	Vector3 avoid;
	bool doAvoid = false;
	for (unsigned int i = 0; i < others.Size(); ++i) {
		if (others[i] == obj)
			continue;
		Vector3 otherVec = others[i]->position - obj->position;

		Vector3 project = Project(otherVec, obj->velocity);
		Vector3 intersect = obj->position + project;
		Vector3 ortho = others[i]->position - intersect;

		if (ortho.Length() < closest && project.Length() < closest) {
			closest = project.Length();
			avoid = (obj->velocity.Length() * -1) * ortho.Normalized();
			doAvoid = true;
		}
	}

	if (doAvoid)
		obj->velocity = avoid;
}

void Separate(SteeredObject* obj, const Vector<SteeredObject*>& others, float scale) {
	Vector3 steering;
	for (unsigned int i = 0; i < others.Size(); ++i) {
		if (others[i] == obj)
			continue;
		steering += (obj->position - others[i]->position) * scale;
	}
	obj->velocity = steering;
}

void FacingCohesion(SteeredObject* obj, const Vector<SteeredObject*>& others) {
	Vector3 steering;
	for (unsigned int i = 0; i < others.Size(); ++i) {
		if (others[i] == obj)
			continue;
		steering += obj->velocity;
	}
	obj->velocity = Seek(obj->position, (1.0f / (others.Size()-1) * steering));
}

void PositionCohesion(SteeredObject* obj, const Vector<SteeredObject*>& others) {
	Vector3 steering;
	for (unsigned int i = 0; i < others.Size(); ++i) {
		if (others[i] == obj)
			continue;
		steering += obj->position;
	}
	obj->velocity = Seek(obj->position, (1.0f / (others.Size()-1) * steering));
}

}
}