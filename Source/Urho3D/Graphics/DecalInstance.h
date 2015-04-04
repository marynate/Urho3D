#pragma once

#include "../Scene/Component.h"
#include "../Math/Vector2.h"

namespace Urho3D 
{
	struct Decal;
	class DecalSet;

	class DecalInstance : Component {
		OBJECT(DecalInstance);
		BASEOBJECT(DecalInstance);
	public:
		DecalInstance(Context*);
		~DecalInstance();

		static void RegisterObject(Context*);

		virtual void OnSetEnabled() override;

		int GetTargetNode() const {return targetNodeID_;}
		void SetTargetNode(int id) {targetNodeID_ = id; CheckDecal();}
		float GetDecalScale() const {return decalScale_;}
		void SetDecalScale(float val) {decalScale_ = val; CheckDecal();}
		const Vector2& GetDecalUVMin() const {return uvs_;}
		void SetDecalUVMin(const Vector2& uv) {uvs_ = uv; CheckDecal();}
		const Vector2& GetDecalUVMax() const {return uvsMax_;}
		void SetDecalUVMax(const Vector2& uv) {uvsMax_ = uv; CheckDecal();}
		float GetDepthBias() const {return depth_;}
		void SetDepthBias(float val) {depth_ = val; CheckDecal();}
	protected:
		virtual void OnNodeSet(Node*) override;
		void CheckDecal();
		void ClearDecal();

	private:
		int targetNodeID_;
		float decalScale_;
		Vector2 uvs_;
		Vector2 uvsMax_;
		int decal_;
		float depth_;
		DecalSet* ds_;
	};

}