#pragma once

#include "../AI/SteeredObject.h"
#include "../Container/Vector.h"
#include "../Math/Vector3.h"

namespace Urho3D
{
	namespace Steering {
		
		Vector3 Seek(const Vector3& pos, const Vector3& dest);
		Vector3 ArriveAt(const Vector3& pos, const Vector3& target, float speed); //seek Scaled
		Vector3 Intercept(const Vector3& pos, const Vector3& target, const Vector3& targetVelocity);
		Vector3 Evade(const Vector3& pos, const Vector3& target, const Vector3& targetVelocity);
		
		void Avoid(SteeredObject* obj, const Vector<SteeredObject*>& others);
		void Separate(SteeredObject* obj, const Vector<SteeredObject*>& others, float scale);
		void FacingCohesion(SteeredObject* obj, const Vector<SteeredObject*>& others);
		void PositionCohesion(SteeredObject* obj, const Vector<SteeredObject*>& others);
	}
}