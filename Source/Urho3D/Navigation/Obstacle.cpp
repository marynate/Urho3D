#include "Precompiled.h"

#include "../Navigation/Obstacle.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Scene/Scene.h"
#include "../Navigation/DynamicNavigationMesh.h"

namespace Urho3D
{
	extern const char* NAVIGATION_CATEGORY;

	Obstacle::Obstacle(Context* context) :
		Component(context),
		height_(5.0f),
		radius_(5.0f),
		obstacleId_(0)
	{

	}

	Obstacle::~Obstacle()
	{
		
	}

	void Obstacle::RegisterObject(Context* context)
	{
		context->RegisterFactory<Obstacle>(NAVIGATION_CATEGORY);
		COPY_BASE_ATTRIBUTES(Component);
		ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, float, 5.0f, AM_DEFAULT);
		ACCESSOR_ATTRIBUTE("Height", GetHeight, SetHeight, float, 5.0f, AM_DEFAULT);
	}

	void Obstacle::OnSetEnabled()
	{
		bool enabled = IsEnabledEffective();

		if (ownerMesh_)
		{
			if (enabled)
			{
				ownerMesh_.Get()->AddObstacle(this);
			}
			else
			{
				ownerMesh_.Get()->RemoveObstacle(this);
			}
		}
	}

	void Obstacle::SetHeight(float newHeight)
	{
		height_ = newHeight;
		if (ownerMesh_)
			ownerMesh_->ObstacleChanged(this);
	}

	void Obstacle::SetRadius(float newRadius)
	{
		radius_ = newRadius;
		if (ownerMesh_)
			ownerMesh_->ObstacleChanged(this);
	}

	void Obstacle::OnNodeSet(Node* node)
	{
		if (node)
		{
			if (GetScene() == node)
			{
				LOGWARNING(GetTypeName() + " should not be created to the root scene node");
				return;
			}
			if (ownerMesh_)
				return;
			ownerMesh_ = GetScene()->GetComponent<DynamicNavigationMesh>();
			if (ownerMesh_)
			{
				ownerMesh_.Get()->AddObstacle(this);
			}
		}
	}

	void Obstacle::OnMarkedDirty(Node* node)
	{
		if (ownerMesh_)
			ownerMesh_.Get()->ObstacleChanged(this);
	}
}