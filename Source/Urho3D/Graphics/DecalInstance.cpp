#include "Precompiled.h"

#include "../Graphics/DecalInstance.h"

#include "../Core/Context.h"
#include "../Scene/Node.h"
#include "../Graphics/DecalSet.h"
#include "../Graphics/Octree.h"
#include "../Graphics/OctreeQuery.h"
#include "../Math/Ray.h"
#include "../Graphics/Drawable.h"
#include "../Scene/Scene.h"

namespace Urho3D {
	extern const char* GEOMETRY_CATEGORY;

	DecalInstance::DecalInstance(Context* context) : Component(context) 
	{
		targetNodeID_ = -1;
		uvsMax_ = Vector2(1,1);
		uvs_ = Vector2(0,0);
		decalScale_ = 0.5f;
		decal_ = -1;
		depth_ = 1;
	}

	DecalInstance::~DecalInstance() 
	{
	}

	void DecalInstance::RegisterObject(Context* context) 
	{
		context->RegisterFactory<DecalInstance>(GEOMETRY_CATEGORY);
		ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("Target Set Node", GetTargetNode, SetTargetNode, int, -1, AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("Decal Scale", GetDecalScale, SetDecalScale, float, 0.5f, AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("UV Min", GetDecalUVMin, SetDecalUVMin, Vector2, Vector2(0,0), AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("UV Max", GetDecalUVMax, SetDecalUVMax, Vector2, Vector2(1,1), AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("Depth Bias", GetDepthBias, SetDepthBias, float, 1, AM_DEFAULT);
	}

	void DecalInstance::OnSetEnabled() {
		if (IsEnabled())
			CheckDecal();
		else
			ClearDecal();
	}

	void DecalInstance::OnNodeSet(Node* node) 
	{
		Component::OnNodeSet(node);
		CheckDecal();
	}

	void DecalInstance::CheckDecal() 
	{
		ClearDecal();
		if (GetNode() != 0x0 && targetNodeID_ != -1) {
			Node* nd = GetNode()->GetScene()->GetNode(targetNodeID_);
			if (nd == 0x0)
				return;
			DecalSet* ds = nd->GetOrCreateComponent<DecalSet>();
			if (ds != 0x0) {
				PODVector<RayQueryResult> results;
				Drawable* hitDrawable = 0x0;
				Ray r(GetNode()->GetPosition(), GetNode()->GetDirection());
				Vector3 hitPos;
				RayOctreeQuery query(results, r, RAY_TRIANGLE, 500, DRAWABLE_GEOMETRY);
				nd->GetScene()->GetComponent<Octree>()->RaycastSingle(query);
				if (results.Size())
				{
					RayQueryResult& result = results[0];
					hitPos = result.position_;
					hitDrawable = result.drawable_;

					ds->AddDecal(hitDrawable, hitPos, GetNode()->GetRotation(), decalScale_, 1.0f, depth_, uvs_, uvsMax_);
					ds_ = ds;
					decal_ = ds_->decals_.Size()-1;
				}
			}
		}
	}

	void DecalInstance::ClearDecal() {
		if (decal_ != -1 && ds_ != 0x0) {
			List<Decal>::Iterator it = ds_->decals_.Begin();
			for (int i = 0; i < decal_; ++i)
				it++;
			ds_->decals_.Erase(it);
		}
		decal_ = -1;
		ds_ = 0x0;
	}
}