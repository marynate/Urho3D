#include "Precompiled.h"

#include "../Graphics/AnimationController.h"
#include "../AI/Behaviors.h"
#include "../AI/Blackboard.h"
#include "../Scene/Component.h"
#include "../Scene/Node.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

bool VariantLess(const Variant& lhs, const Variant& rhs)
{
	if (lhs.GetType() == VAR_INT) {
		if (rhs.GetType() == VAR_INT) {
			return lhs.GetInt() < rhs.GetInt();
		} else if (rhs.GetType() == VAR_FLOAT) {
			return lhs.GetInt() < rhs.GetFloat();
		}
	} else if (lhs.GetType() == VAR_FLOAT) {
		if (rhs.GetType() == VAR_INT) {
			return lhs.GetFloat() < rhs.GetInt();
		} else if (rhs.GetType() == VAR_FLOAT) {
			return lhs.GetFloat() < rhs.GetFloat();
		}
	}
	return false;
}

bool VariantGreater(const Variant& lhs, const Variant& rhs)
{
	if (lhs.GetType() == VAR_INT) {
		if (rhs.GetType() == VAR_INT) {
			return lhs.GetInt() > rhs.GetInt();
		} else if (rhs.GetType() == VAR_FLOAT) {
			return lhs.GetInt() > rhs.GetFloat();
		}
	} else if (lhs.GetType() == VAR_FLOAT) {
		if (rhs.GetType() == VAR_INT) {
			return lhs.GetFloat() > rhs.GetInt();
		} else if (rhs.GetType() == VAR_FLOAT) {
			return lhs.GetFloat() > rhs.GetFloat();
		}
	}
	return false;
}

bool VariantLessEqual(const Variant& lhs, const Variant& rhs)
{
	if (lhs.GetType() == VAR_INT) {
		if (rhs.GetType() == VAR_INT) {
			return lhs.GetInt() <= rhs.GetInt();
		} else if (rhs.GetType() == VAR_FLOAT) {
			return lhs.GetInt() <= rhs.GetFloat();
		}
	} else if (lhs.GetType() == VAR_FLOAT) {
		if (rhs.GetType() == VAR_INT) {
			return lhs.GetFloat() <= rhs.GetInt();
		} else if (rhs.GetType() == VAR_FLOAT) {
			return lhs.GetFloat() <= rhs.GetFloat();
		}
	}
	return false;
}

bool VariantGreaterEqual(const Variant& lhs, const Variant& rhs)
{
	if (lhs.GetType() == VAR_INT) {
		if (rhs.GetType() == VAR_INT) {
			return lhs.GetInt() >= rhs.GetInt();
		} else if (rhs.GetType() == VAR_FLOAT) {
			return lhs.GetInt() >= rhs.GetFloat();
		}
	} else if (lhs.GetType() == VAR_FLOAT) {
		if (rhs.GetType() == VAR_INT) {
			return lhs.GetFloat() >= rhs.GetInt();
		} else if (rhs.GetType() == VAR_FLOAT) {
			return lhs.GetFloat() >= rhs.GetFloat();
		}
	}
	return false;
}

bool CompareVariants(const Variant& lhs, const Variant& rhs, ComparisonType comp) {
	switch (comp) {
		case NotEqual:
			return lhs != rhs;
		case Equal:
			return lhs == rhs;
		case Less:
			return VariantLess(lhs, rhs);
		case LessEqual:
			return VariantLessEqual(lhs,rhs);
		case Greater:
			return VariantGreater(lhs,rhs);
		case GreaterEqual:
			return VariantGreaterEqual(lhs,rhs);
		}
	return false;
}

NodeAttributeCondition::NodeAttributeCondition(String name)
{
	SetName(name);
	compare = Equal;
}

BehaviorResult NodeAttributeCondition::Resolve(BehaviorData& data)
{
	Node* node = data.node.Get();
	if (node != 0x0) {
		Variant v = node->GetAttribute(attrName);
		if (!v.IsEmpty()) {
			return CompareVariants(v,attrValue, compare) ? E_SUCCESS : E_FAILED;
		}
	}
	return E_FAILED;
}

BehaviorResult HasChildCondition::Resolve(BehaviorData& data)
{
	Node* ch = data.GetNode()->GetChild(childName);
	return ch != 0x0 ? E_SUCCESS : E_FAILED;
}

ChildRouteDecorator::ChildRouteDecorator(SharedPtr<Behavior> wrap) : Decorator(wrap)
{
	SetName("Route to Child");
}

BehaviorResult ChildRouteDecorator::Resolve(BehaviorData& data)
{
	Node* ch = data.GetNode()->GetChild(childName);
	if (ch != 0x0) {
		Node* me = data.GetNode();
		data.node = ch;
		BehaviorResult ret = wrapped_->Resolve(data);
		data.node = me;
		return ret;
	}
	return E_FAILED;
}

HasComponentCondition::HasComponentCondition(String name) {SetName(name);}

BehaviorResult HasComponentCondition::Resolve(BehaviorData& data)
{
	Component* c = data.GetNode()->GetComponent(component);
	return c != 0x0 ? E_SUCCESS : E_FAILED;
}

ComponentAttributeCondition::ComponentAttributeCondition(String name)
{
	SetName(name);
	compare = Equal;
}

BehaviorResult ComponentAttributeCondition::Resolve(BehaviorData& data)
{
	Component* c = data.GetNode()->GetComponent(component);
	if (c != 0x0) {
		Variant v = c->GetAttribute(attrName);
		if (!v.IsEmpty()) {
			return CompareVariants(v, value, compare) ? E_SUCCESS : E_FAILED;
		}
	}
	return E_FAILED;
}

BehaviorResult IsAnimatingCondition::Resolve(BehaviorData& data)
{
	AnimationController* ac = data.GetNode()->GetComponent<AnimationController>();
	if (ac != 0x0)
		return ac->IsPlaying(animationName) ? E_SUCCESS : E_FAILED;
	return E_FAILED;
}

ManageLockAction::ManageLockAction()
{
	SetName("Edit Lock");
	modify = Urho3D::M_MAX_INT;
	set = Urho3D::M_MAX_INT;
}

BehaviorResult ManageLockAction::Resolve(BehaviorData& data) 
{
	if (data.locks != 0x0) {
		if (modify != Urho3D::M_MAX_INT)
			data.locks->SetLock(target, data.locks->GetLock(target) + modify);
		if (set != Urho3D::M_MAX_INT)
			data.locks->SetLock(target, set);
	}
	return E_SUCCESS;
}

RouteToParentDecorator::RouteToParentDecorator(SharedPtr<Behavior> wrap) : Decorator(wrap)
{
	SetName("Parent");
}

BehaviorResult RouteToParentDecorator::Resolve(BehaviorData& data) 
{
	Node* parent = data.GetNode()->GetParent();
	if (parent != 0x0) {
		Node* me = data.GetNode();
		data.node = parent;
		BehaviorResult ret = wrapped_->Resolve(data);
		data.node = me;
		return ret;
	}
	return E_FAILED;
}

NodeVariableCondition::NodeVariableCondition(String name)
{
	SetName(name);
	compare = Equal;
}

BehaviorResult NodeVariableCondition::Resolve(BehaviorData& data)
{
	Variant v = data.GetNode()->GetVar(varName);
	if (!v.IsEmpty())
		return CompareVariants(v, value, compare) ? E_SUCCESS : E_FAILED;
	return E_FAILED;
}

BlackboardCondition::BlackboardCondition(String name)
{
	SetName(name);
	compare = Equal;
}

BehaviorResult BlackboardCondition::Resolve(BehaviorData& data)
{
	Node* blackNode = data.GetNode()->GetScene()->GetChild(blackboardNode);
	if (blackNode != 0x0) {
		Blackboard* bb = blackNode->GetComponent<Blackboard>();
		if (bb != 0x0) {
			Variant v = bb->GetData(data.GetNode()->GetWorldPosition(), key);
			if (!v.IsEmpty()) {
				return CompareVariants(v, value, compare) ? E_SUCCESS : E_FAILED;
			}
		}
	}
	return E_FAILED;
}

RouteToDecorator::RouteToDecorator(SharedPtr<Behavior> b) : Decorator(b)
{
	SetName("Route To");
}

BehaviorResult RouteToDecorator::Resolve(BehaviorData& data)
{
	Node* nd = data.GetNode()->GetScene()->GetChild(target, true);
	if (nd != 0x0) {
		Node* me = data.GetNode();
		data.node = nd;
		BehaviorResult res = wrapped_->Resolve(data);
		data.node = me;
		return res;
	}
	return E_FAILED;
}

}