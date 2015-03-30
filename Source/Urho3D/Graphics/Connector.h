#pragma once

#include "../Graphics/Drawable.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"

namespace Urho3D
{
	class Context;

	class Connector : public Drawable
	{
		OBJECT(Connector)

		struct URHO3D_API ConnectorVertex
		{
			Vector3 position_;
			Vector2  uv_;
		};
	public:
		Connector(Context*);
		~Connector();

		static void RegisterObject(Context*);

		virtual void Update(const FrameInfo &frame);

		/// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
		virtual void UpdateBatches(const FrameInfo& frame);
		/// Prepare geometry for rendering. Called from a worker thread if possible (no GPU update.)
		virtual void UpdateGeometry(const FrameInfo& frame);
		/// Return whether a geometry update is necessary, and if it can happen in a worker thread.
		virtual UpdateGeometryType GetUpdateGeometryType();

		virtual void  DrawDebugGeometry(DebugRenderer *debug, bool depthTest) override;

		float GetWidth() const { return width_; }
		void SetWidth(float);
		bool GetAlignToView() const { return alignToView_; }
		void SetAlignToView(bool);
		int GetSegments() const { return segments_; }
		void SetSegments(int);
		bool ApplyGravity() const { return applyGravity_; }
		void SetApplyGravity(bool);
		float GetGravityTolerance() const { return gravityTolerance_; }
		void SetGravityTolerance(float);
		int GetTargetNodeID() const { return targetNode_; }
		void SetTargetNodeID(int);

		/// Set material.
		void SetMaterial(Material* material);
		/// Set material attribute.
		void SetMaterialAttr(const ResourceRef& value);
		/// Return material attribute.
		ResourceRef GetMaterialAttr() const;

	protected:
		void UpdateGeometry();

		/// Handle node being assigned.
		virtual void OnNodeSet(Node* node);
		/// Recalculate the world-space bounding box.
		virtual void OnWorldBoundingBoxUpdate();

		/// Mark vertex buffer to need an update.
		void MarkPositionsDirty();

	private:
		float width_;
		int segments_;
		bool applyGravity_;
		float gravityTolerance_;
		bool alignToView_;
		int targetNode_;

		/// Resize Connector vertex and index buffers.
		void UpdateBufferSize();
		/// Rewrite TailGenerator vertex buffer.
		void UpdateVertexBuffer(const FrameInfo& frame);

		void Integrate(Vector<Vector2>& aPoints, float aSpacing);

		/// Geometry.
		SharedPtr<Geometry> geometry_;
		/// Vertex buffer.
		SharedPtr<VertexBuffer> vertexBuffer_;
		/// Index buffer.
		SharedPtr<IndexBuffer> indexBuffer_;
		/// Transform matrices for position and billboard orientation.
		Matrix3x4 transforms_[2];

		Vector3 bbmin;
		Vector3 bbmax;

		Vector3 lastOtherNode_;
		Vector3 lastMe_;
		Vector3 lastCamera_;
		Vector3 lastCameraDir_;

		WeakPtr<Node> otherNode_;

		/// Whether it's time to update
		bool bufferDirty_;
	};
}