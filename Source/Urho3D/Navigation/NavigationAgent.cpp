//
// Copyright (c) 2008-2015 the Urho3D project.
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

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Core/Variant.h"
#include "../Graphics/DebugRenderer.h"
#include "../IO/Log.h"
#include "../Navigation/DetourCrowdManager.h"
#include "../Navigation/NavigationAgent.h"
#include "../Navigation/NavigationEvents.h"
#include "../Scene/Scene.h"
#include "../Scene/Component.h"
#include "../Scene/Node.h"
#include "../Scene/Serializable.h"

#include "../ThirdParty/Detour/DetourCommon.h"
#include "../ThirdParty/Detour/DetourCrowd.h"

#include "../DebugNew.h"





namespace Urho3D
{

extern const char* NAVIGATION_CATEGORY;

static const float DEFAULT_AGENT_MAX_SPEED = 5.0f;
static const float DEFAULT_AGENT_MAX_ACCEL = 3.6f;
static const NavigationQuality DEFAULT_AGENT_AVOIDANCE_QUALITY = NAVIGATIONQUALITY_HIGH;
static const NavigationPushiness DEFAULT_AGENT_NAVIGATION_PUSHINESS = PUSHINESS_MEDIUM;


NavigationAgent::NavigationAgent(Context* context) :
    Component(context),
    inCrowd_(false),
    agentCrowdId_(-1),
    targetRef_(-1),
    updateNodePosition_(true),
    maxAccel_(DEFAULT_AGENT_MAX_ACCEL),
    maxSpeed_(DEFAULT_AGENT_MAX_SPEED),
    navQuality_(DEFAULT_AGENT_AVOIDANCE_QUALITY),
    navPushiness_(DEFAULT_AGENT_NAVIGATION_PUSHINESS)
{

}

NavigationAgent::~NavigationAgent()
{
}

void NavigationAgent::RegisterObject(Context* context)
{
    context->RegisterFactory<NavigationAgent>(NAVIGATION_CATEGORY);

    ACCESSOR_ATTRIBUTE("Max Accel", GetMaxAccel, SetMaxAccel, float, DEFAULT_AGENT_MAX_ACCEL, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE("Max Speed", GetMaxSpeed, SetMaxSpeed, float, DEFAULT_AGENT_MAX_SPEED, AM_DEFAULT);
}

void NavigationAgent::OnNodeSet(Node* node)
{
    if (node)
    {
        Scene* scene = GetScene();
        if (scene)
        {
            if (scene == node)
                LOGWARNING(GetTypeName() + " should not be created to the root scene node");		
                /// \todo error handling if no DetourCrowdManager component was created
                crowdManager_ = scene->GetOrCreateComponent<DetourCrowdManager>();
                // 
                //crowdManager_ = scene->GetOrCreateComponent<DetourCrowdManager>();
                //crowdManager_->AddAgent(this);

                AddAgentToCrowd();
        }
        else
            LOGERROR("Node is detached from scene, can not create navigation agent.");

        node->AddListener(this);
    }
}

void NavigationAgent::OnSetEnabled()
{
    bool enabled = IsEnabledEffective();

    if (enabled && !inCrowd_)
        AddAgentToCrowd();
    else if (!enabled && inCrowd_)
        RemoveAgentFromCrowd();
}

void NavigationAgent::AddAgentToCrowd()
{
    if (!crowdManager_)
        return;

    PROFILE(AddAgentToCrowd);

    if (agentCrowdId_ !=-1)
        RemoveAgentFromCrowd();
    else
    {
        inCrowd_ = true;
        agentCrowdId_ = crowdManager_->AddAgent(node_->GetPosition(), maxAccel_, maxSpeed_);
        if (agentCrowdId_ == -1)
        {
            inCrowd_ = false;
            LOGERROR("AddAgentToCrowd: Could not add agent to crowd!");
            return;
        }
        crowdManager_->AddAgentComponent(this);
        dtCrowdAgentParams params = crowdManager_->GetCrowd()->getEditableAgent(agentCrowdId_)->params;
        //  add this component as a userpointer in dtCrowdAgentParams ? 
        params.userData = this;
        crowdManager_->UpdateAgentNavigationQuality(agentCrowdId_, navQuality_);
        crowdManager_->UpdateAgentPushiness(agentCrowdId_, navPushiness_);
    }
}

void NavigationAgent::RemoveAgentFromCrowd()
{
    if (crowdManager_ && agentCrowdId_ != -1 && inCrowd_)
    {
        crowdManager_->RemoveAgentComponent(this);
        crowdManager_->RemoveAgent(agentCrowdId_);
        inCrowd_ = false;
    }

}

bool NavigationAgent::SetMoveTarget(const Vector3& position)
{
    if (crowdManager_ && inCrowd_)
    {
        targetPosition_ = position;
        return crowdManager_->SetAgentTarget(agentCrowdId_, position, targetRef_);
    }
    return false;
}

bool NavigationAgent::SetMoveVelocity(const Vector3& velocity)
{
    if (crowdManager_ && inCrowd_)
    {		
        return crowdManager_->SetMoveVelocity(agentCrowdId_, velocity);
    }
    return false;
}

void NavigationAgent::SetMaxSpeed(float speed)
{
    maxSpeed_=speed;
    if(crowdManager_ && inCrowd_)
    {
        crowdManager_->UpdateAgentMaxSpeed(agentCrowdId_, maxSpeed_);
    }
}

void NavigationAgent::SetMaxAccel(float accel)
{
    maxAccel_=accel;
    if(crowdManager_ && inCrowd_)
    {
        crowdManager_->UpdateAgentMaxAcceleration(agentCrowdId_, maxAccel_);
    }
}

void NavigationAgent::SetNavigationQuality(NavigationQuality val)
{
    navQuality_=val;
    if(crowdManager_ && inCrowd_)
    {
        crowdManager_->UpdateAgentNavigationQuality(agentCrowdId_, navQuality_);
    }
}

void NavigationAgent::SetNavigationPushiness(NavigationPushiness val)
{
    navPushiness_=val;
    if(crowdManager_ && inCrowd_)
    {
        crowdManager_->UpdateAgentPushiness(agentCrowdId_, navPushiness_);
    }
}

Vector3 NavigationAgent::GetPosition() const
{
    if (crowdManager_ && inCrowd_)
    {
        return crowdManager_->GetAgentPosition(agentCrowdId_);
    }
    return node_->GetPosition();// or return ZERO ??
}

Vector3 NavigationAgent::GetDesiredVelocity() const
{
    if (crowdManager_ && inCrowd_)
    {
        return crowdManager_->GetAgentDesiredVelocity(agentCrowdId_);
    }
    return Vector3::ZERO;
}

Vector3 NavigationAgent::GetActualVelocity() const
{
    if (crowdManager_ && inCrowd_)
    {
        return crowdManager_->GetAgentCurrentVelocity(agentCrowdId_);
    }
    return Vector3::ZERO;
}

const Vector3& NavigationAgent::GetTargetPosition() const
{
    return targetPosition_;
}

Urho3D::NavigationAgentState NavigationAgent::GetAgentState() const
{
    if (crowdManager_ && inCrowd_)
    {
        const dtCrowdAgent * agent = crowdManager_->GetCrowdAgent(agentCrowdId_);
        if (!agent || !agent->active)
            return NAV_AGENT_INVALID;
        return (NavigationAgentState)agent->state;
    }
    return NAV_AGENT_INVALID;


}

Urho3D::NavigationTargetState NavigationAgent::GetTargetState() const
{
    if (crowdManager_ && inCrowd_)
    {
        const dtCrowdAgent * agent = crowdManager_->GetCrowdAgent(agentCrowdId_);
        if (!agent || !agent->active)
            return NAV_AGENT_TARGET_NONE;

        // Determine if we've arrived at the target
        if (agent->targetState == DT_CROWDAGENT_TARGET_VALID)
        {
            if (agent->ncorners)
            {
                // Is the agent at the end of its path?
                const bool endOfPath = (agent->cornerFlags[agent->ncorners - 1] & DT_STRAIGHTPATH_END) ? true : false;
                if (endOfPath)
                {
                    // Within its own radius of the goal?
                    if (dtVdist2D(agent->npos, &agent->cornerVerts[(agent->ncorners - 1) * 3]) <= agent->params.radius)
                        return NAV_AGENT_TARGET_ARRIVED;

                } // End if reaching end

            } // End if has path

        } // End if valid
        // Return the underlying state.
        return (NavigationTargetState)agent->targetState;
    }
    return NAV_AGENT_TARGET_NONE;
}

void NavigationAgent::SetUpdateNodePosition(bool unodepos)
{
    updateNodePosition_ = unodepos;
}

bool NavigationAgent::GetUpdateNodePosition()
{
    return updateNodePosition_;
}

void NavigationAgent::OnNavigationAgentReposition(const Vector3& newPos)
{
    if(node_)
    {
        // Notify parent node of the reposition
        VariantMap map;
        map[NavigationAgentReposition::P_POSITION]=newPos;
        map[NavigationAgentReposition::P_VELOCITY]=GetActualVelocity();
        node_->SendEvent(E_NAVIGATION_AGENT_REPOSITION, map);
        
        if (updateNodePosition_)
        {
            node_->SetPosition(newPos);
        }
    }
}

void NavigationAgent::OnMarkedDirty(Node* node)
{

}


}
