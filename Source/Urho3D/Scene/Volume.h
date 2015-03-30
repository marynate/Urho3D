#pragma once

#include "../Scene/Component.h"
#include "../Graphics/Drawable.h"
#include "../Container/Str.h"
#include "../Math/Matrix3.h"
#include "../Math/Matrix3x4.h"
#include "../Math/Matrix4.h"
#include "../Math/BoundingBox.h"

namespace Urho3D {

	class Context;

	class Volume : public Drawable {
		OBJECT(Volume);
	public:
		Volume(Context*);
		~Volume();

		static void RegisterObject(Context*);

		/// Visualize the component as debug geometry.
		virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

		/// Check whether a point is inside.
		bool ContainsPoint(const Vector3& point) const;
		bool ContainsBounds(const BoundingBox& bounds) const;
		bool ContainsSphere(const Sphere& sphere) const;
		BoundingBox GetBounds() const { return boundingBox_; }
		void SetBounds(const BoundingBox& bnds) { boundingBox_ = bnds; }

	protected:
		virtual void OnMarkedDirty(Node*) override;
		virtual void OnWorldBoundingBoxUpdate();
	private:
		/// Cached inverse world transform matrix.
		mutable Matrix3x4 inverseWorld_;
		/// Inverse transform dirty flag.
		mutable bool inverseWorldDirty_;
		const Matrix3x4& GetInverseWorldTransform() const;
	};

}