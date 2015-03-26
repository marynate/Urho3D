#pragma once

#include "BehaviorTree.h"

namespace Urho3D
{
	enum ComparisonType {
		NotEqual,
		Equal,
		Less,
		Greater,
		LessEqual,
		GreaterEqual
	};

	//
	class HasChildCondition : public Behavior {
	public:
		virtual BehaviorResult Resolve(BehaviorData&) override;
		virtual void Update(Node* node, float td) override {}

		String childName;
	};

	/// reroutes behavior tree evaluation to go against a child node
	class ChildRouteDecorator : public Decorator {
	public:
		ChildRouteDecorator(SharedPtr<Behavior> wrap);

		virtual BehaviorResult Resolve(BehaviorData&) override;

		String childName;
	};

	class NodeAttributeCondition : public Behavior {
	public:
		String attrName;
		Variant attrValue;
		ComparisonType compare;

		NodeAttributeCondition(String name);

		BehaviorResult Resolve(BehaviorData&) override;
		virtual void Update(Node* node, float td) override {}
	};

	class HasComponentCondition : public  Behavior {
	public:
		HasComponentCondition(String name);

		virtual BehaviorResult Resolve(BehaviorData&) override;
		virtual void Update(Node* node, float td) override {}

		String component;
	};

	class ComponentAttributeCondition : public  Behavior {
	public:
		ComponentAttributeCondition(String name);

		BehaviorResult Resolve(BehaviorData&) override;
		virtual void Update(Node* node, float td) override {}

		ComparisonType compare;
		String component;
		String attrName;
		Variant value;
	};

	class BlackboardCondition : public  Behavior {
	public:
		BlackboardCondition(String name);

		BehaviorResult Resolve(BehaviorData&) override;
		virtual void Update(Node* node, float td) override {}

		String blackboardNode;
		String key;
		Variant value;
		ComparisonType compare;
	};

	class IsAnimatingCondition : public Behavior {
	public:
		virtual BehaviorResult Resolve(BehaviorData&) override;
		virtual void Update(Node* node, float td) override {}

		String animationName;
	};

	class ManageLockAction : public Behavior {
	public:
		ManageLockAction();

		virtual BehaviorResult Resolve(BehaviorData&) override;
		virtual void Update(Node* node, float td) override {}

		String target;
		int modify;
		int set;
	};

	class RouteToParentDecorator : public Decorator {
	public:
		RouteToParentDecorator(SharedPtr<Behavior>);

		virtual BehaviorResult Resolve(BehaviorData&) override;

	};

	class NodeVariableCondition : public Behavior {
	public:
		NodeVariableCondition(String name);

		virtual BehaviorResult Resolve(BehaviorData&) override;
		virtual void Update(Node* node, float td) override {}

		ComparisonType compare;
		String varName;
		Variant value;
	};

	class RouteToDecorator : public Decorator {
	public:
		RouteToDecorator(SharedPtr<Behavior>);

		virtual BehaviorResult Resolve(BehaviorData&) override;
		virtual void Update(Node* node, float td) override {}

		String target;
	};
}