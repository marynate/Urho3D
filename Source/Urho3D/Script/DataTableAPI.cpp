
#include "../Script/APITemplates.h"

#include "../DB/DataTable.h"

namespace Urho3D
{
	static void DTReadJSON(JSONFile* file, DataTable* datatable)
	{
		datatable->ReadJSON(file);
	}

	static void DTReadXML(XMLFile* file, DataTable* datatable)
	{
		datatable->ReadXML(file);
	}

	static CScriptArray* DTGetRow(DataTable* datatable, unsigned int rowIdx)
	{
		return VectorToArray<Variant>(datatable->GetRow(rowIdx), "Array<Variant>");
	}

	void RegisterDataTableAPI(asIScriptEngine* engine)
	{
		engine->RegisterObjectType("DataTable", sizeof(DataTable), asOBJ_REF);
		RegisterRefCounted<DataTable>(engine, "DataTable");

		engine->RegisterObjectMethod("DataTable", "void ReadJSON(JSONFile@+)", asFUNCTION(DTReadJSON), asCALL_CDECL_OBJLAST);
		engine->RegisterObjectMethod("DataTable", "void ReadXML(XMLFile@+)", asFUNCTION(DTReadXML), asCALL_CDECL_OBJLAST);
		engine->RegisterObjectMethod("DataTable", "Array<Variant>@ GetRow(uint)", asFUNCTION(DTGetRow), asCALL_CDECL_OBJFIRST);
		engine->RegisterObjectMethod("DataTable", "int get_rows()", asMETHOD(DataTable, Rows), asCALL_THISCALL);
		engine->RegisterObjectMethod("DataTable", "int get_columns()", asMETHOD(DataTable, Columns), asCALL_THISCALL);
		engine->RegisterObjectMethod("DataTable", "String GetColumnName(uint)", asMETHOD(DataTable, GetColumnName), asCALL_THISCALL);
		engine->RegisterObjectMethod("DataTable", "int GetColumnIdx(const String&in)", asMETHOD(DataTable, GetColumnIdx), asCALL_THISCALL);
	}
}