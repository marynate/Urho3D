#include "Precompiled.h"

#include "../Script/Addons.h"
#include "../Script/APITemplates.h"
#include <AngelScript/angelscript.h>
#include "../AI/SteeredObject.h"

namespace Urho3D
{
	void DoAvoid(CScriptArray* others, SteeredObject* obj)
	{
		obj->Avoid(ArrayToVector<SteeredObject*>(others));
	}

	void DoSeparate(CScriptArray* others, SteeredObject* obj)
	{
		obj->Separate(ArrayToVector<SteeredObject*>(others));
	}

	void DoFaceCohesion(CScriptArray* others, SteeredObject* obj)
	{
		obj->FacingCohesion(ArrayToVector<SteeredObject*>(others));
	}

	void DoPosCohesion(CScriptArray* others, SteeredObject* obj)
	{
		obj->PositionCohesion(ArrayToVector<SteeredObject*>(others));
	}


	void RegisterSteeringAPI(asIScriptEngine* engine) {
		RegisterRefCounted<SteeredObject>(engine, "SteeredObject");
		RegisterSimpleConstruct<SteeredObject>(engine, "SteeredObject");
		engine->RegisterObjectType("SteeredObject", sizeof(SteeredObject), asOBJ_REF);
		engine->RegisterObjectProperty("SteeredObject", "Vector3 position", offsetof(SteeredObject, position));
		engine->RegisterObjectProperty("SteeredObject", "Vector3 velocity", offsetof(SteeredObject, velocity));
		engine->RegisterObjectProperty("SteeredObject", "float radius", offsetof(SteeredObject, radius));
		engine->RegisterObjectMethod("SteeredObject", "void Seek(Vector3&in)", asMETHOD(SteeredObject, Seek), asCALL_THISCALL);
		engine->RegisterObjectMethod("SteeredObject", "void ArriveAt(Vector3&in)", asMETHOD(SteeredObject, ArriveAt), asCALL_THISCALL);
		engine->RegisterObjectMethod("SteeredObject", "void Intercept(SteeredObject@)", asMETHOD(SteeredObject, Intercept), asCALL_THISCALL);
		engine->RegisterObjectMethod("SteeredObject", "void Evade(SteeredObject@)", asMETHOD(SteeredObject, Evade), asCALL_THISCALL);
		engine->RegisterObjectMethod("SteeredObject", "void Avoid(Array<SteeredObject@>@+)", asFUNCTION(DoAvoid), asCALL_CDECL_OBJLAST);
		engine->RegisterObjectMethod("SteeredObject", "void Separate(Array<SteeredObject@>@+)", asFUNCTION(DoSeparate), asCALL_CDECL_OBJLAST);
		engine->RegisterObjectMethod("SteeredObject", "void FacingCohesion(Array<SteeredObject@>@+)", asFUNCTION(DoFaceCohesion), asCALL_CDECL_OBJLAST);
		engine->RegisterObjectMethod("SteeredObject", "void PositionCohesion(Array<SteeredObject@>@+)", asFUNCTION(DoPosCohesion), asCALL_CDECL_OBJLAST);
	}
}