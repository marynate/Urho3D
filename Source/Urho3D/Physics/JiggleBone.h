#pragma once

#include "../Scene/LogicComponent.h"
#include "../Math/Vector3.h"

namespace Urho3D
{
	class Context;

	class JiggleBone : public LogicComponent
	{
		OBJECT(JiggleBone)
	public:
		JiggleBone(Context*);
		~JiggleBone();

		static void RegisterObject(Context*);

		virtual void DelayedStart() override;
		virtual void PostUpdate(float timeStep) override;

		virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

		float GetStiffness() const { return stiffness_; }
		float GetMass() const { return mass_; }
		float GetDamping() const { return damping_; }
		float GetGravity() const { return gravity_; }
		void SetStiffness(float aValue) { stiffness_ = aValue; }
		void SetMass(float aValue) { mass_ = aValue; }
		void SetDamping(float aValue) { damping_ = aValue; }
		void SetGravity(float aValue) { gravity_ = aValue; }

		bool GetSquashAndStretch() const { return squashAndStretch_; }
		float GetSideStretch() const { return sideStretch_; }
		float GetFrontStretch() const { return frontStretch_; }
		void SetSquashAndStretch(bool aValue) { squashAndStretch_ = aValue; }
		void SetSideStretch(float aValue) { sideStretch_ = aValue; }
		void SetFrontStretch(float aValue) { frontStretch_ = aValue; }

	private:

		Vector3 targetPos_;
		Vector3 dynamicPos_;
		Vector3 boneAxis_;
		float targetDistance_;

		// dynamics settings
		float stiffness_;
		float mass_;
		float damping_;
		float gravity_;

		// dynamics values
		Vector3 force_;
		Vector3 acc_;
		Vector3 vel_;

		// squashing
		bool squashAndStretch_;
		float sideStretch_;
		float frontStretch_;


	};
}