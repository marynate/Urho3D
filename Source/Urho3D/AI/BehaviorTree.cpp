#include "Precompiled.h"

#include "BehaviorTree.h"

namespace Urho3D
{

String BehaviorData::DumpText() const
{
    String ret = "";
    for (unsigned int i = 0; i < stack.Size(); ++i) {
        SharedPtr<Behavior> lck = stack[i].Lock();
        if (lck.Get()) {
            ret += lck->GetName();
        }
        if (i < stack.Size() - 1)
            ret += " -> ";
    }
    return ret;
}

Composite::~Composite()
{
	behaviors_.Clear();
}

void Composite::AddBehavior(SharedPtr<Behavior> behavior)
{
    behaviors_.Push(behavior);
}

void Composite::RemoveBehavior(SharedPtr<Behavior> behavior)
{
	Vector<SharedPtr<Behavior> >::Iterator it = behaviors_.Find(behavior);
	if (it != behaviors_.End())
		behaviors_.Erase(it);
}

int BehaviorLock::GetLock(const String& name) {
    HashMap<StringHash,int>::ConstIterator cit = locks_.Find(name);
    if (cit != locks_.End())
        return cit->second_;
    return 0;
}

void BehaviorLock::SetLock(const String& name, int ct) {
    HashMap<StringHash,int>::Iterator it = locks_.Find(name);
    if (it != locks_.End())
        it->second_ = ct;
    else
        locks_[name] = ct;
}

void BehaviorLock::IncLock(const String& name) {
    HashMap<StringHash,int>::Iterator it = locks_.Find(name);
    if (it != locks_.End())
        it->second_ += 1;
    else
        locks_[name] = 1;
}

void BehaviorLock::DecLock(const String& name) {
    HashMap<StringHash,int>::Iterator it = locks_.Find(name);
    if (it != locks_.End())
        it->second_ -= 1;
}

BehaviorResult Sequence::Resolve(BehaviorData& data)
{   
    Vector<SharedPtr<Behavior> >::Iterator it = behaviors_.Find(data.running.Lock());
    if (it == behaviors_.End())
        it = behaviors_.Begin();
    for (; it != behaviors_.End(); ++it) {
        data.stack.Push(WeakPtr<Behavior>(*it));
        
        BehaviorResult res = (*it)->Resolve(data);
        if (res == E_FAILED) {
            data.stack.Pop();
            return res;
        }
        if (res == E_RUNNING) {
            data.running = *it; //don't mess with the stack
            return res;
        }
        data.stack.Pop();
    }
    return E_SUCCESS; //everyone passed
}

BehaviorResult Selector::Resolve(BehaviorData& data)
{
    Vector<SharedPtr<Behavior> >::Iterator it = behaviors_.Find(data.running.Lock());
    if (it == behaviors_.End())
        it = behaviors_.Begin();
    for (; it != behaviors_.End(); ++it) {
        data.stack.Push(WeakPtr<Behavior>(*it));
        BehaviorResult res = (*it)->Resolve(data);
        if (res == E_SUCCESS) {
            data.stack.Pop();
            return res;
        }
        if (res == E_RUNNING) {
            data.running = *it;
            return res;
        }
        data.stack.Pop();
    }
    return E_FAILED; //everyone failed
}

BehaviorResult Parallel::Resolve(BehaviorData& data)
{
    int failCt = 0;
    int successCt = 0;
    for (unsigned int i = 0; i < behaviors_.Size(); ++i) {
        BehaviorResult res = behaviors_[i]->Resolve(data);
        data.stack.Push(WeakPtr<Behavior>(behaviors_[i]));
        
        if (res == E_FAILED && !anyFail)
            failCt++;
        else if (res == E_FAILED) {
            data.stack.Pop();
            return E_FAILED;
        }
        else if (res == E_SUCCESS && !anyFail)
            successCt++;
        else if (res == E_SUCCESS) {
            data.stack.Pop();
            return E_SUCCESS;
        }
        else if (res == E_RUNNING) {
            //TODO
        }
    }
    if (failCt == behaviors_.Size())
        return E_FAILED;
    else if (successCt == behaviors_.Size())
        return E_SUCCESS;
    return E_RUNNING;
}

BehaviorResult Randomized::Resolve(BehaviorData& data) 
{
    if (behaviors_.Size() == 0)
        return E_FAILED;
	int tries = 0;

	//we'll potentially try a few times to find an E_SUCCESS or E_RUNNING, unless attemptRunning_ is true
	//in which case we'll keep trying until we get an E_RUNNING, otherwise we've E_FAILED
	do {
		
		int sub = Urho3D::Random(0, behaviors_.Size()-1);
		BehaviorResult res = behaviors_[sub]->Resolve(data);
		if (res == E_RUNNING) { //push onto the stack if we're running
			data.stack.Push(WeakPtr<Behavior>(behaviors_[sub]));
			return res;
		} else if (res == E_SUCCESS && !attemptRunning_)
			return res;
		++tries;

	} while (tries < attempts_);

	return E_FAILED;
}

Decorator::Decorator(SharedPtr<Behavior> wrapped) {
	wrapped_ = wrapped;
}

Behavior* Decorator::GetWrapped()
{
	return wrapped_.Get();
}
void Decorator::SetWrapped(Behavior* ptr)
{
	wrapped_ = ptr;
}

InvertDecorator::InvertDecorator(SharedPtr<Behavior> wrap) : Decorator(wrap)
{
	SetName("Invert");
}

BehaviorResult InvertDecorator::Resolve(BehaviorData& data)
{
    BehaviorResult base = wrapped_->Resolve(data);
    if (base == E_FAILED)
        return E_SUCCESS;
    else if (base == E_SUCCESS)
        return E_SUCCESS;
    return base;
}

UntilSuccessDecorator::UntilSuccessDecorator(SharedPtr<Behavior> wrap) : Decorator(wrap)
{
	SetName("UntilSuccess");
}

BehaviorResult UntilSuccessDecorator::Resolve(BehaviorData& data)
{
    BehaviorResult base = wrapped_->Resolve(data);
    if (base == E_FAILED)
        return E_RUNNING;
    return base;
}

UntilFailDecorator::UntilFailDecorator(SharedPtr<Behavior> wrap) : Decorator(wrap)
{
	SetName("UntilFail");
}

BehaviorResult UntilFailDecorator::Resolve(BehaviorData& data)
{
    BehaviorResult base = wrapped_->Resolve(data);
    if (base == E_SUCCESS)
        return E_RUNNING;
    return base;
}

AlwaysSucceedDecorator::AlwaysSucceedDecorator(SharedPtr<Behavior> wrap) : Decorator(wrap)
{
	SetName("AlwaysSucceed");
}

BehaviorResult AlwaysSucceedDecorator::Resolve(BehaviorData& data)
{
    wrapped_->Resolve(data);
    return E_SUCCESS;
}

AlwaysFailDecorator::AlwaysFailDecorator(SharedPtr<Behavior> wrap) : Decorator(wrap)
{
	SetName("AlwaysFail");
}

BehaviorResult AlwaysFailDecorator::Resolve(BehaviorData& data)
{
    wrapped_->Resolve(data);
    return E_FAILED;
}

RouteDecorator::RouteDecorator(SharedPtr<Behavior> wrap) : Decorator(wrap)
{
	SetName("RouteTree");
}

BehaviorResult RouteDecorator::Resolve(BehaviorData& data) {
    return wrapped_->Resolve(data);
}

GateDecorator::GateDecorator(SharedPtr<Behavior> wrap) : Decorator(wrap)
{
	SetName("Invert");
}

bool GateDecorator::IsOpen(BehaviorData& data) const {
	if (data.locks != 0x0)
		return data.locks->GetLock(target_) < limit_;
	return false;
}

BehaviorResult GateDecorator::Resolve(BehaviorData& data) {
    if (IsOpen(data))
        return wrapped_->Resolve(data);
    if (defaultSucceed)
        return E_SUCCESS;
    return E_FAILED;
}
}