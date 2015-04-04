#pragma once

#include "../Container/Ptr.h"
#include "../Scene/Component.h"

namespace Urho3D
{
	class DynamicNavigationMesh;

	class Obstacle : public Component
	{
		OBJECT(Obstacle)
		friend class DynamicNavigationMesh;

	public:
		Obstacle(Context*);
		virtual ~Obstacle();

		static void RegisterObject(Context*);

		/// Updates the owning mesh when enabled status has changed
		virtual void OnSetEnabled() override;

		/// Gets the height of this obstacle
		float GetHeight() const { return height_; }
		/// Sets the height of this obstacle
		void SetHeight(float);
		/// Gets the blocking radius of this obstacle
		float GetRadius() const { return radius_; }
		/// Sets the blocking radius of this obstacle
		void SetRadius(float);

		unsigned GetObstacleID() const { return obstacleId_; }

	protected:
		/// Handle node being assigned.
		virtual void OnNodeSet(Node* node) override;
		/// \todo Handle node transform being dirtied.
		virtual void OnMarkedDirty(Node* node) override;

	private:
		/// radius of this obstacle
		float radius_;
		/// height of this obstacle, extends 1/2 height below and 1/2 height above the owning node's position
		float height_;

		/// id received from tile cache
		unsigned obstacleId_;
		/// pointer to the navigation mesh we belong to
		WeakPtr<DynamicNavigationMesh> ownerMesh_;
	};
}