#pragma once

#include "../Container/RefCounted.h"
#include "../Math/Vector3.h"

namespace Urho3D
{

	class SteeredObject : public RefCounted {
	public:
		Vector3 position;
		Vector3 velocity;
		float radius;

		void Seek(const Vector3& target);
		void ArriveAt(const Vector3& target); //seek Scaled
		void Intercept(SteeredObject*);
		void Evade(SteeredObject*);

		void Avoid(const Vector<SteeredObject*>& others);
		void Separate(const Vector<SteeredObject*>& others);
		void FacingCohesion(const Vector<SteeredObject*>& others);
		void PositionCohesion(const Vector<SteeredObject*>& others);
	};
}