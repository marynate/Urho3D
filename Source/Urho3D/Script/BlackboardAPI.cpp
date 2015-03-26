#include "Precompiled.h"

#include "../Script/Addons.h"
#include <AngelScript/angelscript.h>
#include "../Script/APITemplates.h"
#include "../AI/Blackboard.h"
#include "../Math/StringHash.h"
#include "../Core/Variant.h"
#include "../Math/Vector3.h"

namespace Urho3D
{


CScriptArray* GetQueryList(BBQueryResult* result)
{
	return VectorToArray<BBQueryItem>(result->results, "Array<BBQueryItem>");
}

bool BBQuery(BBQueryResult& in, Blackboard* result)
{
	result->Query(in);
	return in.results.Size() > 0;
}

bool BBQueryFor(BBQueryResult& in, const StringHash& key, Blackboard* bb)
{
	bb->QueryFor(in, key);
	return in.results.Size() > 0;
}

void CreateQueryItem(BBQueryItem* it)
{
	new (it) BBQueryItem();
}

void CreateQueryResult(BBQueryResult* it)
{
	new (it) BBQueryResult();
}

void RegisterBlackboardAPI(asIScriptEngine* engine)
{
	engine->RegisterObjectType("BBQueryItem", sizeof(BBQueryItem), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CAK);
	engine->RegisterObjectBehaviour("BBQueryItem", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(CreateQueryItem), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectProperty("BBQueryItem", "StringHash name", offsetof(BBQueryItem, name));
	engine->RegisterObjectProperty("BBQueryItem", "Vector3 where", offsetof(BBQueryItem, where));
	engine->RegisterObjectProperty("BBQueryItem", "Variant value", offsetof(BBQueryItem, value));

	engine->RegisterObjectType("BBQueryResult", sizeof(BBQueryResult), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CAK);
	engine->RegisterObjectBehaviour("BBQueryResult", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(CreateQueryResult), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectProperty("BBQueryResult", "Sphere area", offsetof(BBQueryItem, where));
	engine->RegisterObjectMethod("BBQueryResult", "Array<BBQueryItem>@ get_results()", asFUNCTION(GetQueryList), asCALL_CDECL_OBJLAST);

    RegisterComponent<Blackboard>(engine, "Blackboard");
	engine->RegisterObjectMethod("Blackboard", "IntVector2 get_gridSize()", asMETHOD(Blackboard, GetGridSizeAttr), asCALL_THISCALL);
	engine->RegisterObjectMethod("Blackboard", "void set_gridSize(IntVector2)", asMETHOD(Blackboard, SetGridSizeAttr), asCALL_THISCALL);
	engine->RegisterObjectMethod("Blackboard", "Vector2 get_area()", asMETHOD(Blackboard, GetAreaAttr), asCALL_THISCALL);
	engine->RegisterObjectMethod("Blackboard", "void set_area(Vector2)", asMETHOD(Blackboard, SetAreaAttr), asCALL_THISCALL);
	engine->RegisterObjectMethod("Blackboard", "Variant GetData(Vector3&in, StringHash&in)", asMETHODPR(Blackboard, GetData, (const Vector3&, const StringHash&), Variant), asCALL_THISCALL);
	engine->RegisterObjectMethod("Blackboard", "Variant GetData(Vector2&in, StringHash&in)", asMETHODPR(Blackboard, GetData, (const Vector2&, const StringHash&), Variant), asCALL_THISCALL);
	engine->RegisterObjectMethod("Blackboard", "void SetData(Vector3&in,StringHash&in,Variant&,float=0)", asMETHODPR(Blackboard, PlaceData, (const Vector3&, const StringHash&,Variant&,float), void), asCALL_THISCALL);
	engine->RegisterObjectMethod("Blackboard", "void SetData(Vector2&in,StringHash&in,Variant&,float=0)", asMETHODPR(Blackboard, PlaceData, (const Vector2&, const StringHash&,Variant&,float), void), asCALL_THISCALL);
	engine->RegisterObjectMethod("Blackboard", "bool Contains(Vector3&in,StringHash&in)", asMETHODPR(Blackboard, ContainsData, (const Vector3&, const StringHash&), bool), asCALL_THISCALL);
	engine->RegisterObjectMethod("Blackboard", "bool Contains(Vector2&in,StringHash&in)", asMETHODPR(Blackboard, ContainsData, (const Vector2&, const StringHash&), bool), asCALL_THISCALL);
	engine->RegisterObjectMethod("Blackboard", "bool Query(BBQueryResult&)", asFUNCTION(BBQuery), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("Blackboard", "bool QueryFor(BBQueryResult&, StringHash&in)", asFUNCTION(BBQueryFor), asCALL_CDECL_OBJLAST);
}

}