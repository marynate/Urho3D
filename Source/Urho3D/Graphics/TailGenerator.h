#pragma once

#include "Drawable.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"


namespace Urho3D
{
	/// Vertex struct for tail  
	struct URHO3D_API TailVertex
	{
		Vector3 position_;
		unsigned color_;
		Vector2  uv_;
	};

	/// One billboard in the billboard set.
	struct URHO3D_API Tail
	{
		/// Position.
		Vector3 position;
		Vector3 worldRight;
		Vector3 forward;
	};

	static const unsigned MAX_TAILS = 65536 / 6;

	/// Custom component that creates a tail
	class URHO3D_API TailGenerator : public Drawable
	{
		OBJECT(TailGenerator);

	public:
		/// Construct.
		TailGenerator(Context* context);
		/// Destruct.
		virtual ~TailGenerator();
		/// Register object factory.
		static void RegisterObject(Context* context);

		/// Process octree raycast. May be called from a worker thread.
		virtual void ProcessRayQuery(const RayOctreeQuery& query, PODVector<RayQueryResult>& results);

		void UpdateTail();
		virtual void Update(const FrameInfo &frame);

		/// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
		virtual void UpdateBatches(const FrameInfo& frame);
		/// Prepare geometry for rendering. Called from a worker thread if possible (no GPU update.)
		virtual void UpdateGeometry(const FrameInfo& frame);
		/// Return whether a geometry update is necessary, and if it can happen in a worker thread.
		virtual UpdateGeometryType GetUpdateGeometryType();

		virtual void  DrawDebugGeometry(DebugRenderer *debug, bool depthTest);

		/// Set material.
		void SetMaterial(Material* material);
		/// Set tail segment length
		void SetTailLength(float length);
		/// Get tail segment length
		float GetTailLength() const;
		/// Set count segments of all tail 
		void SetNumTails(unsigned num);
		/// Get count segments of all tail 
		unsigned GetNumTails() const;
		/// Get width scale of the tail
		float GetWidthScale() const { return scale_; }
		/// Set width scale of the tail
		void SetWidthScale(float scale);
		/// Get color of the tip end of the tail
		const Color& GetColorForTip() const { return tailTipColor; }
		/// Set vertex blended color for tip of all tail. The alpha-value of new color resets by default to zero.
		void SetColorForTip(const Color& c);
		/// Get color of the beginning of the tail
		const Color& GetColorForHead() const { return tailHeadColor; }
		// Set vertex blended color for head of all tail. The alpha-value of new color resets by default to one.
		void SetColorForHead(const Color& c);
		/// Set material attribute.
		void SetMaterialAttr(const ResourceRef& value);
		/// Return material attribute.
		ResourceRef GetMaterialAttr() const;
		///
		bool GetDrawVertical() const { return vertical_; }
		bool GetDrawHorizontal() const { return horizontal_; }
		bool GetMatchNodeOrientation() const { return matchNode_; }
		void SetDrawVertical(bool value);
		void SetDrawHorizontal(bool value);
		void SetMatchNodeOrientation(bool value);

	protected:
		void SetupBatches();

		/// Handle node being assigned.
		virtual void OnNodeSet(Node* node);
		/// Recalculate the world-space bounding box.
		virtual void OnWorldBoundingBoxUpdate();

		/// Mark vertex buffer to need an update.
		void MarkPositionsDirty();

		/// Tails.
		PODVector<Tail> tails_;
		PODVector<Tail> fullPointPath;

	private:
		/// Resize TailGenerator vertex and index buffers.
		void UpdateBufferSize();
		/// Rewrite TailGenerator vertex buffer.
		void UpdateVertexBuffer(const FrameInfo& frame);

		/// Geometry.
		SharedPtr<Geometry> geometry_;
		/// Vertex buffer.
		SharedPtr<VertexBuffer> vertexBuffer_;
		/// Index buffer.
		SharedPtr<IndexBuffer> indexBuffer_;
		/// Transform matrices for position and billboard orientation.
		Matrix3x4 transforms_[2];

		/// Buffers need resize flag.
		bool bufferSizeDirty_;
		/// Vertex buffer needs rewrite flag.
		bool bufferDirty_;
		/// render the vertical strip
		bool vertical_;
		/// render the horizontal strip
		bool horizontal_;
		/// match Up/Right vectors to the node's orientation
		bool matchNode_;
		/// Previous position of tail
		Vector3 previousPosition_;
		float tailLength_;
		unsigned tailNum_;
		float scale_;

		bool forceUpdateVertexBuffer_;

		Vector<TailVertex> tailMesh;
		Vector<Tail> activeTails;

		Color tailTipColor;
		Color tailHeadColor;

		Vector3 bbmin;
		Vector3 bbmax;


	};
}