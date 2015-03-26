#include "Precompiled.h"


#include <AngelScript/angelscript.h>
#include "../Script/APITemplates.h"
#include "../Core/Context.h"
#include "../AI/Behaviors.h"
#include "../AI/BehaviorTree.h"
#include "../Script/Script.h"
#include "../Script/ScriptFile.h"
#include "../Container/Str.h"


namespace Urho3D {

template <class T> void BehaviorConstruct(String name)
{
	return new T(name);
}

template <class T> void Behavior_Constructor(asIScriptEngine* engine, const char* className)
{
	String declFactoryWithName(String(className) + "@+ f(const String&in)");
    engine->RegisterObjectBehaviour(className, asBEHAVE_FACTORY, declFactoryWithName.CString(), asFUNCTION(BehaviorConstruct<T>), asCALL_CDECL);
}

class BehaviorWrapper : public Behavior {
    asIScriptObject* obj;
    asIScriptFunction* resolve_;
    asIScriptFunction* update_;
    ScriptFile* file_;
public:
    BehaviorWrapper(asIScriptObject* obj, ScriptFile* file) {
        this->obj = obj;
        file_ = file;
        resolve_ = obj->GetObjectType()->GetMethodByName("Resolve");
        update_ = obj->GetObjectType()->GetMethodByName("Update");
        
		
		Script* script = file_->GetContext()->GetSubsystem<Script>();
		asIScriptContext* context = script->GetScriptFileContext();
		asIScriptFunction* func = obj->GetObjectType()->GetMethodByName("GetName");

		if (context->Prepare(func) >= 0) {
			context->SetObject(obj);
			file_->SetParameters(context, func, Variant::emptyVariantVector);
			script->IncScriptNestingLevel();
			context->Execute();
			SetName(*(String*)context->GetAddressOfReturnValue());

			context->Unprepare();
			script->DecScriptNestingLevel();

		}
    }
    
    void Update(Node* node, float td) {
        VariantVector params;
        params.Push(Variant(node));
        params.Push(Variant(td));
        file_->Execute(obj, update_, params);
    }
    
    BehaviorResult Resolve(BehaviorData& data) {
		VariantVector args;
		args.Push(Variant(&data));
		Script* script = file_->GetContext()->GetSubsystem<Script>();
		asIScriptContext* context = script->GetScriptFileContext();
		
		if (context->Prepare(resolve_) >= 0) {
			context->SetObject(obj);
			file_->SetParameters(context, resolve_, args);
			script->IncScriptNestingLevel();
			BehaviorResult res = E_FAILED;
			int error = context->Execute();
			if (error >= 0) {
				res = (BehaviorResult)context->GetReturnDWord();
				context->Unprepare();
				script->DecScriptNestingLevel();
			}
			return res;
			
		}
		return E_FAILED;
    }
};

void AddScriptBehavior(asIScriptObject* obj, Composite* tree) {
	BehaviorWrapper* wrapper = new BehaviorWrapper(obj, GetScriptContextFile());
    obj->SetUserData(wrapper);
    tree->AddBehavior(SharedPtr<Behavior>(wrapper));
}

void RemoveScriptBehavior(asIScriptObject* obj, Composite* tree) {
    tree->RemoveBehavior(SharedPtr<Behavior>((Behavior*)obj->GetUserData()));
}

void ConstructBehaviorData(BehaviorData* data)
{
	new (data) BehaviorData();
}

void DestructBehaviorData(BehaviorData* data)
{
	//??delete data;
}

void ConstructBehaviorLock(BehaviorLock* lock)
{
	new (lock) BehaviorLock();
}

void ConstructSequence(const String& name, Sequence* seq)
{
    new (seq) Sequence(name);
}

void ConstructParallel(const String& name, Parallel* p)
{
    new (p) Parallel(name);
}

void ConstructSelector(const String& name, Selector* sel)
{
    new (sel) Selector(name);
}

void ConstructRandomized(const String& name, Randomized* sel)
{
    new (sel) Randomized(name);
}

void RegisterBehaviorData(asIScriptEngine* engine)
{
    engine->RegisterObjectMethod("BehaviorData", "Node@+ get_node()", asMETHOD(BehaviorData, GetNode), asCALL_THISCALL);
	engine->RegisterObjectMethod("BehaviorData", "void set_node(Node@+)", asMETHOD(BehaviorData, SetNode), asCALL_THISCALL);
    engine->RegisterObjectMethod("BehaviorData", "Behavior@+ get_behavior()", asMETHOD(BehaviorData, GetBehavior), asCALL_THISCALL);
    engine->RegisterObjectMethod("BehaviorData", "String get_dump()", asMETHOD(BehaviorData, DumpText), asCALL_THISCALL);
	engine->RegisterObjectMethod("BehaviorData", "BehaviorLock@+ get_locks()", asMETHOD(BehaviorData, GetLocks), asCALL_THISCALL);
	engine->RegisterObjectMethod("BehaviorData", "void set_locks(BehaviorLock@+)", asMETHOD(BehaviorData, SetLocks), asCALL_THISCALL);
}

void RegisterBehaviorLock(asIScriptEngine* engine)
{
    engine->RegisterObjectMethod("BehaviorLock", "void SetLock(String,int)", asMETHOD(BehaviorLock, SetLock), asCALL_THISCALL);
    engine->RegisterObjectMethod("BehaviorLock", "void DecLock(String&in)", asMETHOD(BehaviorLock, DecLock), asCALL_THISCALL);
    engine->RegisterObjectMethod("BehaviorLock", "void IncLock(String&in)", asMETHOD(BehaviorLock, IncLock), asCALL_THISCALL);
    engine->RegisterObjectMethod("BehaviorLock", "void ClearLock(String&in)", asMETHOD(BehaviorLock, IncLock), asCALL_THISCALL);
}

template <class T> void RegisterComposite(asIScriptEngine* engine, const char* className)
{
	RegisterRefCounted<T>(engine, className);
	RegisterSimpleNamedConstruct<T>(engine, className);
	RegisterSubclass<Composite,T>(engine, "Composite", className);
    engine->RegisterObjectMethod(className,"void AddBehavior(Behavior@+)", asMETHOD(T, AddBehavior), asCALL_THISCALL);
    engine->RegisterObjectMethod(className,"void RemoveBehavior(Behavior@+)", asMETHOD(T, RemoveBehavior), asCALL_THISCALL);
    engine->RegisterObjectMethod(className, "BehaviorResult Resolve(BehaviorData&)", asMETHOD(T, Resolve), asCALL_THISCALL);
    engine->RegisterObjectMethod(className, "void AddBehavior(ScriptBehavior@+)", asFUNCTION(AddScriptBehavior), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(className, "void RemoveBehavior(ScriptBehavior@+)", asFUNCTION(RemoveScriptBehavior), asCALL_CDECL_OBJLAST);
}

void RegisterBehaviorTree(asIScriptEngine* engine)
{
	engine->RegisterObjectMethod("Parallel", "bool get_quitOnFail()", asMETHOD(Parallel, QuitOnFail), asCALL_THISCALL);
	engine->RegisterObjectMethod("Parallel", "void set_quitOnFail(bool)", asMETHOD(Parallel, SetQuitOnAnyFailure), asCALL_THISCALL);
}

void RegisterBehavior(asIScriptEngine* engine)
{
//Register types so that declaration parsing can succeed
	engine->RegisterEnum("BehaviorResult");
    engine->RegisterEnumValue("BehaviorResult","E_FAILED",0);
    engine->RegisterEnumValue("BehaviorResult","E_SUCCESS",1);
    engine->RegisterEnumValue("BehaviorResult","E_RUNNING",2);

	engine->RegisterObjectType("Behavior", sizeof(Behavior), asOBJ_REF);
	RegisterRefCounted<Behavior>(engine, "Behavior");
	
	engine->RegisterObjectType("BehaviorData", sizeof(BehaviorData), asOBJ_VALUE);
	engine->RegisterObjectBehaviour("BehaviorData", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructBehaviorData), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("BehaviorData", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructBehaviorData), asCALL_CDECL_OBJLAST);

	engine->RegisterObjectType("BehaviorLock", sizeof(BehaviorLock), asOBJ_REF);
	RegisterSimpleConstruct<BehaviorLock>(engine, "BehaviorLock");
	RegisterRefCounted<BehaviorLock>(engine, "BehaviorLock");

	engine->RegisterObjectType("Decorator",sizeof(Decorator), asOBJ_REF);
	RegisterRefCounted<Decorator>(engine, "Decorator");

	engine->RegisterInterface("ScriptBehavior");
    engine->RegisterInterfaceMethod("ScriptBehavior","String GetName()");
    engine->RegisterInterfaceMethod("ScriptBehavior","BehaviorResult Resolve(BehaviorData&)");
    engine->RegisterInterfaceMethod("ScriptBehavior","void Update(Node@,float)");

	engine->RegisterObjectType("Composite",sizeof(Composite), asOBJ_REF);
	RegisterRefCounted<Composite>(engine, "Composite");
	
	RegisterComposite<Sequence>(engine, "Sequence");
	RegisterComposite<Selector>(engine, "Selector");
	RegisterComposite<Parallel>(engine, "Parallel");
	RegisterComposite<Randomized>(engine, "Randomized");


//Behavior
    engine->RegisterObjectMethod("Behavior","BehaviorResult Resolve(BehaviorData&)", asMETHOD(Behavior, Resolve), asCALL_THISCALL);
    engine->RegisterObjectMethod("Behavior", "void Update(Node@,float)", asMETHOD(Behavior, Update), asCALL_THISCALL);
	engine->RegisterObjectMethod("Behavior", "String get_name()", asMETHOD(Behavior, GetName), asCALL_THISCALL);
	engine->RegisterObjectMethod("Behavior", "void set_name(String&in)", asMETHOD(Behavior,SetName), asCALL_THISCALL);

	
	RegisterSubclass<Decorator,Behavior>(engine, "Decorator", "Behavior");
	engine->RegisterObjectMethod("Decorator", "Behavior@+ get_wrapping()", asMETHOD(Decorator, GetWrapped), asCALL_THISCALL);
	engine->RegisterObjectMethod("Decorator", "void set_wrapping(Behavior@+)", asMETHOD(Decorator, SetWrapped), asCALL_THISCALL);
}

template <class T, class U> void RegisterBehaviorType(asIScriptEngine* engine, const char* name, const char* baseClass)
{
	RegisterRefCounted<T>(engine, name);
	RegisterSubclass<U,T>(engine, name, baseClass);
	engine->RegisterObjectMethod(name, "String get_name()", asMETHOD(T, GetName), asCALL_THISCALL);
	engine->RegisterObjectMethod(name, "BehaviorResult Resolve(BehaviorData&)", asMETHOD(T, Resolve), asCALL_THISCALL);
	engine->RegisterObjectMethod(name, "void Update(Node@+,float)",asMETHOD(T,Update), asCALL_THISCALL);
}

void CreateInvertDeco(Behavior* wrap, InvertDecorator* deco)
{
	new (deco) InvertDecorator(SharedPtr<Behavior>(wrap));
}

void CreateAlwaysFailDeco(Behavior* wrap, AlwaysFailDecorator* deco)
{
	new (deco) AlwaysFailDecorator(SharedPtr<Behavior>(wrap));
}

void CreateAlwaysSucceedDeco(Behavior* wrap, AlwaysSucceedDecorator* deco)
{
	new (deco) AlwaysSucceedDecorator(SharedPtr<Behavior>(wrap));
}

void CreateUntilFailDeco(Behavior* wrap, UntilFailDecorator* deco)
{
	new (deco) UntilFailDecorator(SharedPtr<Behavior>(wrap));
}

void CreateUntilSuccessDeco(Behavior* wrap, UntilSuccessDecorator* deco)
{
	new (deco) UntilSuccessDecorator(SharedPtr<Behavior>(wrap));
}

void CreateRouteTreeDeco(Behavior* wrap, RouteDecorator* deco)
{
	new (deco) RouteDecorator(SharedPtr<Behavior>(wrap));
}

template <class T> T* SimpleDecoConstruct(Behavior* b)
{
	T* ret = new T(SharedPtr<Behavior>(b));
	ret->AddRef();
	return ret;
}

template <class T> void DecoConstruct(asIScriptEngine* engine, const char* name)
{
	String declFactory(String(name) + "@+ f(Behavior@)");
	engine->RegisterObjectBehaviour(name, asBEHAVE_FACTORY, declFactory.CString(), asFUNCTION(SimpleDecoConstruct<T>), asCALL_CDECL);
}

void RegisterDecorators(asIScriptEngine* engine)
{
	RegisterBehaviorType<InvertDecorator,Decorator>(engine, "InvertDeco", "Decorator");
	RegisterSubclass<Behavior,InvertDecorator>(engine,"Behavior","InvertDeco");
	DecoConstruct<InvertDecorator>(engine,"InvertDeco");

	RegisterBehaviorType<AlwaysFailDecorator,Decorator>(engine, "AlwaysFailDeco", "Decorator");
	RegisterSubclass<Behavior,AlwaysFailDecorator>(engine,"Behavior","AlwaysFailDeco");
	DecoConstruct<AlwaysFailDecorator>(engine,"AlwaysFailDeco");

	RegisterBehaviorType<AlwaysSucceedDecorator,Decorator>(engine, "AlwaysSucceedDeco", "Decorator");
	RegisterSubclass<Behavior,AlwaysSucceedDecorator>(engine,"Behavior","AlwaysSucceedDeco");
	DecoConstruct<AlwaysSucceedDecorator>(engine, "AlwaysSucceedDeco");

	RegisterBehaviorType<UntilFailDecorator,Decorator>(engine, "UntilFailDeco", "Decorator");
	RegisterSubclass<Behavior,UntilFailDecorator>(engine,"Behavior","UntilFailDeco");
	DecoConstruct<UntilFailDecorator>(engine, "UntilFailDeco");

	RegisterBehaviorType<UntilSuccessDecorator,Decorator>(engine, "UntilSuccessDeco", "Decorator");
	RegisterSubclass<Behavior,UntilSuccessDecorator>(engine,"Behavior","UntilSuccessDeco");
	DecoConstruct<UntilSuccessDecorator>(engine, "UntilSuccessDeco");

	RegisterBehaviorType<RouteDecorator,Decorator>(engine, "RouteTree", "Decorator");
	RegisterSubclass<Behavior,RouteDecorator>(engine,"Behavior","RouteTree");
	DecoConstruct<RouteDecorator>(engine, "RouteTree");
}

void CreateChildRoute(Behavior* b, ChildRouteDecorator* deco)
{
	new (deco) ChildRouteDecorator(SharedPtr<Behavior>(b));
}

void CreateParentRoute(Behavior* b, RouteToParentDecorator* deco)
{
	new (deco) RouteToParentDecorator(SharedPtr<Behavior>(b));
}

void CreateRouteTo(Behavior* b, RouteToDecorator* deco)
{
	new (deco) RouteToDecorator(SharedPtr<Behavior>(b));
}

void RegisterBehaviors(asIScriptEngine* engine)
{
	engine->RegisterEnum("ComparisonType");
	engine->RegisterEnumValue("ComparisonType", "NotEqual", ComparisonType::NotEqual);
	engine->RegisterEnumValue("ComparisonType", "Equal", ComparisonType::Equal);
	engine->RegisterEnumValue("ComparisonType", "Less", ComparisonType::Less);
	engine->RegisterEnumValue("ComparisonType", "LessEqual", ComparisonType::LessEqual);
	engine->RegisterEnumValue("ComparisonType", "Greater", ComparisonType::Greater);
	engine->RegisterEnumValue("ComparisonType", "GreaterEqual", ComparisonType::GreaterEqual);

	RegisterBehaviorType<HasChildCondition,Behavior>(engine, "HasChildCond", "Behavior");
	RegisterSimpleConstruct<HasChildCondition>(engine, "HasChildCond");
	engine->RegisterObjectProperty("HasChildCond", "String childName", offsetof(HasChildCondition, childName));

	RegisterBehaviorType<ChildRouteDecorator, Decorator>(engine, "ChildRoute", "Decorator");
	RegisterSubclass<Behavior,ChildRouteDecorator>(engine,"Behavior","ChildRoute");
	DecoConstruct<ChildRouteDecorator>(engine, "ChildRoute");
	engine->RegisterObjectProperty("ChildRoute", "String childName", offsetof(ChildRouteDecorator, childName));

	RegisterBehaviorType<HasComponentCondition, Behavior>(engine, "HasComponentCond", "Behavior");
	RegisterSimpleNamedConstruct<HasComponentCondition>(engine, "HasComponentCond");
	engine->RegisterObjectProperty("HasComponentCond", "String component", offsetof(HasComponentCondition, component));

	RegisterBehaviorType<NodeAttributeCondition, Behavior>(engine, "NodeAttrCond", "Behavior");
	RegisterSimpleNamedConstruct<NodeAttributeCondition>(engine, "NodeAttrCond");
	engine->RegisterObjectProperty("NodeAttrCond", "Variant value", offsetof(NodeAttributeCondition, attrValue));
	engine->RegisterObjectProperty("NodeAttrCond", "String field", offsetof(NodeAttributeCondition, attrName));
	engine->RegisterObjectProperty("NodeAttrCond", "ComparisonType compare", offsetof(NodeAttributeCondition, compare));

	RegisterBehaviorType<ComponentAttributeCondition, Behavior>(engine, "ComponentAttrCond", "Behavior");
	RegisterSimpleNamedConstruct<ComponentAttributeCondition>(engine, "ComponentAttrCond");
	engine->RegisterObjectProperty("ComponentAttrCond", "String component", offsetof(ComponentAttributeCondition, component));
	engine->RegisterObjectProperty("ComponentAttrCond", "Variant value", offsetof(ComponentAttributeCondition, value));
	engine->RegisterObjectProperty("ComponentAttrCond", "String field", offsetof(ComponentAttributeCondition, attrName));
	engine->RegisterObjectProperty("ComponentAttrCond", "ComparisonType compare", offsetof(ComponentAttributeCondition, compare));

	RegisterBehaviorType<BlackboardCondition, Behavior>(engine, "BlackboardCond", "Behavior");
	RegisterSimpleNamedConstruct<BlackboardCondition>(engine, "BlackboardCond");
	engine->RegisterObjectProperty("BlackboardCond", "String node", offsetof(BlackboardCondition, blackboardNode));
	engine->RegisterObjectProperty("BlackboardCond", "Variant value", offsetof(BlackboardCondition, value));
	engine->RegisterObjectProperty("BlackboardCond", "String field", offsetof(BlackboardCondition, key));
	engine->RegisterObjectProperty("BlackboardCond", "ComparisonType compare", offsetof(BlackboardCondition, compare));

	RegisterBehaviorType<IsAnimatingCondition, Behavior>(engine, "AnimationCond", "Behavior");
	RegisterSimpleConstruct<IsAnimatingCondition>(engine, "AnimationCond");
	engine->RegisterObjectProperty("AnimationCond", "String animation", offsetof(IsAnimatingCondition, animationName));

	RegisterBehaviorType<ManageLockAction, Behavior>(engine, "ManageLock", "Behavior");
	RegisterSimpleConstruct<ManageLockAction>(engine, "ManageLock");
	engine->RegisterObjectProperty("ManageLock", "int modify", offsetof(ManageLockAction, modify));
	engine->RegisterObjectProperty("ManageLock", "int value", offsetof(ManageLockAction, set));
	engine->RegisterObjectProperty("ManageLock", "String field", offsetof(ManageLockAction, target));

	RegisterBehaviorType<NodeVariableCondition, Behavior>(engine, "NodeVarCond", "Behavior");
	RegisterSimpleNamedConstruct<NodeVariableCondition>(engine, "NodeVarCond");
	engine->RegisterObjectProperty("NodeVarCond", "Variant value", offsetof(NodeVariableCondition, value));
	engine->RegisterObjectProperty("NodeVarCond", "String field", offsetof(NodeVariableCondition, varName));
	engine->RegisterObjectProperty("NodeVarCond", "ComparisonType compare", offsetof(NodeVariableCondition, compare));

	RegisterBehaviorType<RouteToParentDecorator, Decorator>(engine,"ParentRoute", "Decorator");
	RegisterSubclass<Behavior,RouteToParentDecorator>(engine,"Behavior","ParentRoute");
	DecoConstruct<RouteToParentDecorator>(engine,"ParentRoute");

	RegisterBehaviorType<RouteToDecorator, Decorator>(engine, "RouteTo", "Decorator");
	RegisterSubclass<Behavior,RouteToDecorator>(engine,"Behavior","RouteTo");
	engine->RegisterObjectProperty("RouteTo", "String target", offsetof(RouteToDecorator, target));
	DecoConstruct<RouteToDecorator>(engine,"RouteTo");
}

void RegisterBehaviorAPI(asIScriptEngine* engine) {
	RegisterBehavior(engine);
    RegisterBehaviorData(engine);
    RegisterBehaviorTree(engine);
	RegisterDecorators(engine);
	RegisterBehaviors(engine);
}

}