//
// Copyright (c) 2008-2014 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Precompiled.h"

#include "../Container/Vector.h"
#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Graphics/DebugRenderer.h"
#include "../IO/Log.h"
#include "../Navigation/DetourCrowdManager.h"
#include "../Navigation/NavigationMesh.h"
#include "../Navigation/NavigationAgent.h"
#include "../Scene/Component.h"
#include "../Scene/Node.h"
#include "../Scene/SceneEvents.h"

#include "../ThirdParty/Detour/DetourCrowd.h"

#include "../DebugNew.h"

namespace Urho3D
{
    extern const char* NAVIGATION_CATEGORY;

    static const int MAX_AGENTS = 128;

    Vector3 FloatToVec3(const float* f)
    {
        Vector3 v;
        v.x_ = f[0];
        v.y_ = f[1];
        v.z_ = f[2];
        return v;
    }

    DetourCrowdManager::DetourCrowdManager(Context* context) :
        Component(context),
        maxAgents_(MAX_AGENTS),
        crowd_(0),
        navigationMesh_(0),
        agentDebug_(NULL)
    {
    }

    DetourCrowdManager::~DetourCrowdManager()
    {
        dtFreeCrowd(crowd_);
        if (agentDebug_)
            delete agentDebug_;
    }

    void DetourCrowdManager::RegisterObject(Context* context)
    {
        context->RegisterFactory<DetourCrowdManager>(NAVIGATION_CATEGORY);
    }

    void DetourCrowdManager::SetNavigationMesh(NavigationMesh *navMesh)
    {
        navigationMesh_ = WeakPtr<NavigationMesh>(navMesh);
        if (navigationMesh_ && navigationMesh_->navMeshQuery_ == 0)
            navigationMesh_->InitializeQuery();
    }

    NavigationMesh* DetourCrowdManager::GetNavigationMesh()
    {
        return navigationMesh_.Get();
    }

    bool DetourCrowdManager::CreateCrowd()
    {
        /// \todo check if already created ?
        if(!navigationMesh_) return false;

        if (navigationMesh_.Expired())
            return false;
        if (navigationMesh_->navMesh_ == 0)
            return false;
        if (navigationMesh_->navMeshQuery_ == 0)
        if (!navigationMesh_->InitializeQuery())
            return false;
        if (crowd_ == 0)
            crowd_ = dtAllocCrowd();
        if (agentDebug_ == NULL)
        {
            agentDebug_ = new dtCrowdAgentDebugInfo();
        }

        // Initialize the crowd
        bool b = crowd_->init(maxAgents_, navigationMesh_->GetAgentRadius(), navigationMesh_->navMesh_);
        if (b == false)
        {
            LOGERROR("Could not initialize  DetourCrowd");
            return false;
        }

        // Make polygons with 'disabled' flag invalid.
		// \todo add add support for exlusion flags when there's a means to mark flags
        crowd_->getEditableFilter(0)->setExcludeFlags(0);

        // Setup local avoidance params to different qualities.
        dtObstacleAvoidanceParams params;
        memcpy(&params, crowd_->getObstacleAvoidanceParams(0), sizeof(dtObstacleAvoidanceParams));

        // Low (11)
        params.velBias = 0.5f;
        params.adaptiveDivs = 5;
        params.adaptiveRings = 2;
        params.adaptiveDepth = 1;
        crowd_->setObstacleAvoidanceParams(0, &params);

        // Medium (22)
        params.velBias = 0.5f;
        params.adaptiveDivs = 5;
        params.adaptiveRings = 2;
        params.adaptiveDepth = 2;
        crowd_->setObstacleAvoidanceParams(1, &params);

        // Good (45)
        params.velBias = 0.5f;
        params.adaptiveDivs = 7;
        params.adaptiveRings = 2;
        params.adaptiveDepth = 3;
        crowd_->setObstacleAvoidanceParams(2, &params);

        // High (66)
        params.velBias = 0.5f;
        params.adaptiveDivs = 7;
        params.adaptiveRings = 3;
        params.adaptiveDepth = 3;
        crowd_->setObstacleAvoidanceParams(3, &params);

        return true;
    }

    int DetourCrowdManager::AddAgent(const Vector3 &pos, float maxaccel, float maxSpeed)
    {
        if (crowd_ == 0 && navigationMesh_.Expired())
            return -1;
        dtCrowdAgentParams params;
        params.radius = navigationMesh_->GetAgentRadius();
        params.height = navigationMesh_->GetAgentHeight();
        params.maxAcceleration = maxaccel;
        params.maxSpeed = maxSpeed;
        params.collisionQueryRange = params.radius * 8.0f;
        params.pathOptimizationRange = params.radius * 30.0f;
        params.updateFlags = 0
            | DT_CROWD_ANTICIPATE_TURNS
            | DT_CROWD_OPTIMIZE_VIS
            | DT_CROWD_OPTIMIZE_TOPO
            | DT_CROWD_OBSTACLE_AVOIDANCE
            //| DT_CROWD_COLLISION_RESOLUTION
            //		| DT_CROWD_AGENT_PRIORITIES
            //		| DT_CROWD_SEPARATION
            ;
        params.obstacleAvoidanceType = 3;
        params.separationWeight = 2.0f;
        params.queryFilterType = 0;
        dtPolyRef polyRef;
        float nearestPos[3];
        dtStatus status = navigationMesh_->navMeshQuery_->findNearestPoly(
            pos.Data(),
            crowd_->getQueryExtents(),
            crowd_->getFilter(0),
            &polyRef,
            nearestPos);

        MarkNetworkUpdate();

        return crowd_->addAgent(nearestPos, &params);
    }

    void DetourCrowdManager::RemoveAgent(int agent)
    {
        if (crowd_ == 0)
            return;
        crowd_->removeAgent(agent);
        MarkNetworkUpdate();
    }

    void DetourCrowdManager::UpdateAgentNavigationQuality(int agent, NavigationQuality nq)
    {
        if (crowd_ == 0)
            return;

        dtCrowdAgentParams params = crowd_->getAgent(agent)->params;
        switch (nq)
        {
        case NAVIGATIONQUALITY_LOW:
        {
                                      params.updateFlags &= ~0
                                          & ~DT_CROWD_ANTICIPATE_TURNS
                                          & ~DT_CROWD_OPTIMIZE_VIS
                                          & ~DT_CROWD_OPTIMIZE_TOPO
                                          & ~DT_CROWD_OBSTACLE_AVOIDANCE
                                          ;
        }
            break;

        case NAVIGATIONQUALITY_MEDIUM:
        {
                                      params.updateFlags |= 0;
                                      params.updateFlags &= ~0
                                          & ~DT_CROWD_OBSTACLE_AVOIDANCE
                                          & ~DT_CROWD_ANTICIPATE_TURNS
                                          & ~DT_CROWD_OPTIMIZE_VIS
                                          & ~DT_CROWD_OPTIMIZE_TOPO
                                          ;
        }
            break;

        case NAVIGATIONQUALITY_HIGH:
        {
                                       params.obstacleAvoidanceType = 3;
                                       params.updateFlags |= 0
                                           | DT_CROWD_ANTICIPATE_TURNS
                                           | DT_CROWD_OPTIMIZE_VIS
                                           | DT_CROWD_OPTIMIZE_TOPO
                                           | DT_CROWD_OBSTACLE_AVOIDANCE
                                           ;
        }
            break;
        }

        crowd_->updateAgentParameters(agent, &params);

        MarkNetworkUpdate();
    }

    void DetourCrowdManager::UpdateAgentPushiness(int agent, NavigationPushiness pushiness)
    {
        if (crowd_ == 0)
            return;

        dtCrowdAgentParams params = crowd_->getAgent(agent)->params;
        switch (pushiness)
        {
        case PUSHINESS_LOW:
            params.separationWeight = 4.0f;
            params.collisionQueryRange = params.radius * 16.0f;
            break;

        case PUSHINESS_MEDIUM:
            params.separationWeight = 2.0f;
            params.collisionQueryRange = params.radius * 8.0f;
            break;

        case PUSHINESS_HIGH:
            params.separationWeight = 0.5f;
            params.collisionQueryRange = params.radius * 1.0f;
            break;
        }
        crowd_->updateAgentParameters(agent, &params);

        MarkNetworkUpdate();
    }

    void DetourCrowdManager::UpdateAgentMaxSpeed(int agent, float maxSpeed)
    {
        if (crowd_ == 0)
            return;
        dtCrowdAgentParams params = crowd_->getAgent(agent)->params;
        params.maxSpeed = maxSpeed;
        crowd_->updateAgentParameters(agent, &params);

        MarkNetworkUpdate();
    }

    void DetourCrowdManager::UpdateAgentMaxAcceleration(int agent, float accel)
    {
        if (crowd_ == 0)
            return;
        dtCrowdAgentParams params = crowd_->getAgent(agent)->params;
        params.maxAcceleration = accel;
        crowd_->updateAgentParameters(agent, &params);

        MarkNetworkUpdate();
    }

    bool DetourCrowdManager::SetAgentTarget(int agent, Vector3 target)
    {
        if (crowd_ == 0)
            return false;
        dtPolyRef polyRef;
        float nearestPos[3];
        dtStatus status = navigationMesh_->navMeshQuery_->findNearestPoly(
            target.Data(),
            crowd_->getQueryExtents(),
            crowd_->getFilter(0),
            &polyRef,
            nearestPos);

        if ((status & DT_FAILURE) == 0)
        {
            if (!crowd_->requestMoveTarget(agent, polyRef, nearestPos))
            {
                // TODO: Handle Failure (Couldn't request new target.)
                return false;
            }
        }
        else
        {
            // TODO: Handle failure (Couldn't find nearest polygon.)
            return false;
        }
        return true;
    }

    bool DetourCrowdManager::SetAgentTarget(int agent, Vector3 target, unsigned int & targetRef)
    {
        if (crowd_ == 0)
            return false;
        float nearestPos[3];
        dtStatus status = navigationMesh_->navMeshQuery_->findNearestPoly(
            target.Data(),
            crowd_->getQueryExtents(),
            crowd_->getFilter(0),
            &targetRef,
            nearestPos);

        if ((status & DT_FAILURE) == 0)
        {
            if (!crowd_->requestMoveTarget(agent, targetRef, nearestPos))
            {
                // TODO: Handle Failure (Couldn't request new target.)
                return false;
            }
        }
        else
        {
            // TODO: Handle failure (Couldn't find nearest polygon.)
            return false;
        }
        return true;
    }

    Vector3 DetourCrowdManager::GetAgentPosition(int agent)
    {
        if (crowd_ == 0)
            return Vector3();

        if (!crowd_->getAgent(agent) || !crowd_->getAgent(agent)->active)
            return Vector3();
        return FloatToVec3(crowd_->getAgent(agent)->npos);
    }

    Vector3 DetourCrowdManager::GetAgentCurrentVelocity(int agent)
    {
        if (crowd_ == 0)
            return Vector3();
        if (!crowd_->getAgent(agent) || !crowd_->getAgent(agent)->active)
            return Vector3();
        return FloatToVec3(crowd_->getAgent(agent)->vel);
    }

    Vector3 DetourCrowdManager::GetAgentDesiredVelocity(int agent)
    {
        if (crowd_ == 0)
            return Vector3();
        if (!crowd_->getAgent(agent) || !crowd_->getAgent(agent)->active)
            return Vector3();
        return FloatToVec3(crowd_->getAgent(agent)->dvel);
    }

    Vector3 DetourCrowdManager::GetClosestWalkablePosition(Vector3 pos)
    {
        if (crowd_ == 0)
            return Vector3();
        float closest[3];
        const static float extents[] = { 1.0f, 20.0f, 1.0f };
        dtPolyRef closestPoly;
        dtQueryFilter filter;
        dtStatus status = navigationMesh_->navMeshQuery_->findNearestPoly(
            pos.Data(),
            extents,
            &filter,
            &closestPoly,
            closest);
        return FloatToVec3(closest);
    }

    bool DetourCrowdManager::SetMoveVelocity(int agent, const Vector3 & velocity)
    {
        if (crowd_ == 0)
            return false;
        // Request that we navigate to this position.
        if (crowd_->getAgent(agent) && crowd_->getAgent(agent)->active)
            return crowd_->requestMoveVelocity(agent, velocity.Data());

        // Not a valid operation.
        return false;
    }

    void DetourCrowdManager::Update(float delta)
    {
        if (crowd_ == 0)
            return;

        crowd_->update(delta, agentDebug_);

        /// \todo  use this ?
        PODVector<NavigationAgent*>::Iterator it;
        for (it = agentComponents_.Begin(); it != agentComponents_.End(); it++)
        {
            (*it)->OnNavigationAgentReposition(FloatToVec3(crowd_->getAgent((*it)->GetAgentCrowdId())->npos));
        }
        /// or use events ?
        /// or use this
        /// but there is a problem if you dont use NavigationAgent components ...
        // 		dtCrowdAgent* _agents[MAX_AGENTS] = { NULL };
        // 		//memset(_agents, 0, MAX_AGENTS * sizeof(*_agents));
        // 		int count = crowd_->getActiveAgents(_agents, MAX_AGENTS);
        // 		for (int i = 0; i < count; i++)
        // 		{
        // 			if (_agents[i])
        // 			{
        // 				NavigationAgent* a = static_cast<NavigationAgent*>(_agents[i]->params.userData);
        // 				if (a)
        // 				{
        // 					a->OnNavigationAgentReposition(FloatToVec3(_agents[i]->npos));
        // 				}
        // 			}
        // 		}
    }

    const dtCrowdAgent * DetourCrowdManager::GetCrowdAgent(int agent)
    {
        if (crowd_ == 0)
            return NULL;
        return crowd_->getAgent(agent);
    }

    void DetourCrowdManager::AddAgentComponent(NavigationAgent* agent)
    {
        agentComponents_.Push(agent);
    }

    void DetourCrowdManager::RemoveAgentComponent(NavigationAgent* agent)
    {
        agentComponents_.Remove(agent);
    }

    void DetourCrowdManager::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
    {
        if (debug && navigationMesh_.NotNull())
        {
			//\todo draw path corridors?

            // Current position-to-target line
            for (int i = 0; i < crowd_->getAgentCount(); i++)
            {
                const dtCrowdAgent* ag = crowd_->getAgent(i);
                if (!ag->active)
                    continue;
   
                Color color(0.6f, 0.2f, 0.2f, 1.0f);
   
                // Render line to target:
                Vector3 pos1(ag->npos[0], ag->npos[1], ag->npos[2]);
                Vector3 pos2;
                for (int i = 0; i < ag->ncorners; ++i)
                {
                    pos2.x_ = ag->cornerVerts[i * 3];
                    pos2.y_ = ag->cornerVerts[i * 3 + 1];
                    pos2.z_ = ag->cornerVerts[i * 3 + 2];
                    debug->AddLine(pos1, pos2, color, depthTest);
                    pos1 = pos2;
                }
                pos2.x_ = ag->targetPos[0];
                pos2.y_ = ag->targetPos[1];
                pos2.z_ = ag->targetPos[2];
                debug->AddLine(pos1, pos2, color, depthTest);
   
                // Target circle
                debug->AddSphere(Sphere(pos2, 0.5f), color, depthTest);

				//\todo draw velocity info?
            }
        }
    }

    const PODVector<NavigationAgent*>& DetourCrowdManager::GetNavigationAgents() const
    {
        return agentComponents_;
    }

    dtCrowd * DetourCrowdManager::GetCrowd()
    {
        return crowd_;
    }

    void DetourCrowdManager::HandleSceneSubsystemUpdate(StringHash eventType, VariantMap& eventData)
    {
        using namespace SceneSubsystemUpdate;

        Update(eventData[P_TIMESTEP].GetFloat());
    }

    void DetourCrowdManager::OnNodeSet(Node* node)
    {
        // Subscribe to the scene subsystem update, which will trigger the crowd update step, and grab a reference
        // to the scene's NavigationMesh
        if (node)
        {			
            SubscribeToEvent(node, E_SCENESUBSYSTEMUPDATE, HANDLER(DetourCrowdManager, HandleSceneSubsystemUpdate));
            NavigationMesh *mesh=node->GetComponent<NavigationMesh>();
            if (mesh)
				SetNavigationMesh(mesh);
        }
    }

}