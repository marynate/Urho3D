#pragma once

#include "../Container/HashMap.h"
#include "../Scene/Node.h"
#include "../Container/Ptr.h"
#include "../Math/StringHash.h"

namespace Urho3D {

enum BehaviorResult {
    E_FAILED,
    E_SUCCESS,
    E_RUNNING
};

class BehaviorLock : public RefCounted
{
    HashMap<StringHash,int> locks_;
public:
    int GetLock(const String& name);
    void SetLock(const String& name, int ct);
    void IncLock(const String& name);
    void DecLock(const String& name);
};

class Behavior;

class BehaviorData
{
public:
    WeakPtr<Node> node;
    WeakPtr<Behavior> running;
    Vector<WeakPtr<Behavior> > stack;
	WeakPtr<BehaviorLock> locks;

	BehaviorData() : running(0), node(0), locks(0) {
		locks = 0x0;

	}

	BehaviorLock* GetLocks() const { return locks.Get();}
	Node* GetNode() const {return node.Get();}
	void SetNode(Node* node) {this->node = node;}
	Behavior* GetBehavior() const {return running.Get();}
	void SetLocks(BehaviorLock* lck) {locks = WeakPtr<BehaviorLock>(lck);}
    
    String DumpText() const;
};

class Behavior : public RefCounted {
    String name;
public:

    virtual String GetName() const {return name;}
    void SetName(String str) {name = str;}

    virtual BehaviorResult Resolve(BehaviorData& data) = 0;
    virtual void Update(Node* node, float td) = 0;
    
private:
};

class Composite : public Behavior
{
protected:
    Vector< SharedPtr<Behavior> > behaviors_;
public:
	virtual ~Composite();

    void Update(Node* node, float td) override {}
    
    void AddBehavior(SharedPtr<Behavior>);
    void RemoveBehavior(SharedPtr<Behavior>);
};

//as long as actions return E_SUCCESS it will continue to execute actions
class Sequence : public Composite {
public:
	Sequence(String name) {SetName(name);}
    BehaviorResult Resolve(BehaviorData&) override;
};

//will continue when actions return E_FAILED, will return E_FAILED if no one returns E_RUNNING or E_SUCCESS
class Selector : public Composite {
public:
	Selector(String name) {SetName(name);}
    BehaviorResult Resolve(BehaviorData&) override;
};

//anyFail == false MEANS will return E_RUNNING until every child fails or everyone succeeds 
//anyFail == true MEANS will return the result of the first failure/success
class Parallel : public Composite {
	bool anyFail;
public:
	Parallel(String name) {SetName(name);}
    BehaviorResult Resolve(BehaviorData&) override;

	bool QuitOnFail() const {return anyFail;}
	void SetQuitOnAnyFailure(bool b) {anyFail = b;}
};

//Picks one at random
class Randomized : public Composite {
	int attempts_;
	bool attemptRunning_;
public:
	Randomized(String name) {
		SetName(name); 
		attempts_ = 1;
		attemptRunning_ = false;
	}
    BehaviorResult Resolve(BehaviorData&) override;

	int GetAttempts() const {return attempts_;}
	void SetAttempts(int ct) {attempts_ = ct;}
};

//wraps a single behavior, used to implement weird shit
class Decorator : public Behavior {
protected:
	SharedPtr<Behavior> wrapped_;
public:
    Decorator(SharedPtr<Behavior> wrap);

	BehaviorResult Resolve(BehaviorData&) override {return E_FAILED;}
    void Update(Node* node, float td) override {}

	Behavior* GetWrapped();
	void SetWrapped(Behavior*);
};

class InvertDecorator : public Decorator {
public:
    InvertDecorator(SharedPtr<Behavior> wrap);
    BehaviorResult Resolve(BehaviorData&) override;
};

class UntilSuccessDecorator : public Decorator {
public:
    UntilSuccessDecorator(SharedPtr<Behavior>);
    
    BehaviorResult Resolve(BehaviorData&) override;
};

class UntilFailDecorator : public Decorator {
public:
    UntilFailDecorator(SharedPtr<Behavior>);
    BehaviorResult Resolve(BehaviorData&) override;
};

class AlwaysSucceedDecorator : public Decorator {
public:
    AlwaysSucceedDecorator(SharedPtr<Behavior>);
    BehaviorResult Resolve(BehaviorData&) override;
};

class AlwaysFailDecorator : public Decorator {
public:
    AlwaysFailDecorator(SharedPtr<Behavior>);
    BehaviorResult Resolve(BehaviorData&) override;
};

class RouteDecorator : public Decorator {
public:
    RouteDecorator(SharedPtr<Behavior>);
    BehaviorResult Resolve(BehaviorData&) override;
};

class GateDecorator : public Decorator {
    int limit_;
	bool defaultSucceed;
    String target_;
public:
    GateDecorator(SharedPtr<Behavior>);
    BehaviorResult Resolve(BehaviorData&) override;
    
    bool IsOpen(BehaviorData&) const;
};

}