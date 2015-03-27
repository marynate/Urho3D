#include "Precompiled.h"

#include "../Core/Variant.h"
#include "../DB/DataTable.h"
#include "../Core/StringUtils.h"
#include "../Container/Str.h"
#include "../Resource/JSONFile.h"
#include "../Resource/XMLFile.h"

namespace Urho3D
{
   
void DataTable::ReadJSON(JSONFile* aFile)
{
	JSONValue root = aFile->GetRoot();
	JSONValue types = root.GetChild("Types");
	JSONValue values = root.GetChild("Values");

	for (unsigned int i = 0; i < types.GetSize(); ++i)
	{
		JSONValue column = types.GetChild(i);
		String colName = column.GetString(0);
		String typeName = column.GetString(1);
		colNames_.Push(colName);
		VariantType varType;
		if (typeName.Compare("bool", false))
			varType = VAR_BOOL;
		else if (typeName.Compare("int", false))
			varType = VAR_INT;
		else if (typeName.Compare("float", false))
			varType = VAR_FLOAT;
		else if (typeName.Compare("string", false))
			varType = VAR_STRING;
		else if (typeName.Compare("color", false))
			varType = VAR_COLOR;

		columns_.Push(varType);
	}

	for (unsigned int i = 0; i < values.GetSize(); ++i)
	{
		JSONValue row = values.GetChild(i);
		VariantVector vec;
		for (unsigned int col = 0; col < row.GetSize(); ++i)
		{
			switch (columns_[col])
			{
			case VariantType::VAR_BOOL:
				vec.Push(Variant(row.GetBool(col)));
				break;
			case VariantType::VAR_INT:
				vec.Push(Variant(row.GetInt(col)));
				break;
			case VariantType::VAR_STRING:
				vec.Push(Variant(row.GetString(col)));
				break;
			case VariantType::VAR_FLOAT:
				vec.Push(Variant(row.GetFloat(col)));
				break;
			case VariantType::VAR_COLOR:
				vec.Push(Variant(row.GetColor(col)));
				break;
			}
		}
		rows_.Push(vec);
	}
}

void DataTable::ReadXML(XMLFile* aFile)
{
	XMLElement root = aFile->GetRoot();
	XMLElement types = root.GetChild("Columns");
	XMLElement type = types.GetChild("Column");
	while (type.NotNull())
	{
		String colName = type.GetAttributeCString("name");
		String typeName = type.GetAttributeCString("type");
		colNames_.Push(colName);
		VariantType varType;
		if (typeName.Compare("bool", false))
			varType = VAR_BOOL;
		else if (typeName.Compare("int", false))
			varType = VAR_INT;
		else if (typeName.Compare("float", false))
			varType = VAR_FLOAT;
		else if (typeName.Compare("string", false))
			varType = VAR_STRING;
		else if (typeName.Compare("color", false))
			varType = VAR_COLOR;
		columns_.Push(varType);

		type = type.GetNext("Column");
	}


	XMLElement values = root.GetChild("Rows");
	XMLElement row = values.GetChild("Row");
	while (row.NotNull())
	{
		VariantVector vec;
		XMLElement column = row.GetChild("Value");
		int i = 0;
		while (column.NotNull())
		{
			switch (columns_[i])
			{
			case VariantType::VAR_BOOL:
				vec.Push(Variant(column.GetBool("value")));
				break;
			case VariantType::VAR_INT:
				vec.Push(Variant(column.GetInt("value")));
				break;
			case VariantType::VAR_STRING:
				vec.Push(Variant(column.GetAttribute("value")));
				break;
			case VariantType::VAR_FLOAT:
				vec.Push(Variant(column.GetFloat("value")));
				break;
			case VariantType::VAR_COLOR:
				vec.Push(Variant(column.GetColor("value")));
				break;
			}

			rows_.Push(vec);
			column = column.GetNext("Value");
			++i;
		}
		row = row.GetNext("Row");
	}
}

/// returns a cache vector
VariantVector& DataTable::GetRow(unsigned int row)
{
    if (row < rows_.Size())
        return rows_[row];
    return emptyRet_;
}

int DataTable::Rows() const
{
    return rows_.Size();
}

int DataTable::Columns() const
{
    return columns_.Size();
}

const String& DataTable::GetColumnName(unsigned int aIdx) const
{
    if (aIdx < colNames_.Size())
        return colNames_[aIdx];
    return "";
}

int DataTable::GetColumnIdx(const String& aColumnName) const
{
    for (unsigned int i = 0; i < colNames_.Size(); ++i) 
	{
        if (colNames_[i] == aColumnName)
            return (int)i;
    }
    return -1;
}

};