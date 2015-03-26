#include "Precompiled.h"

#include "../Core/Variant.h"
#include "../DB/DataTable.h"

namespace Urho3D
{

ColumnInfo::ColumnInfo(VariantType aType, const String& aName)
{
    type_ = aType;
    name_ = aName;
}

DataTable::DataTable(Vector<ColumnInfo> aColumnTypes)
{
    for (unsigned int i = 0; i < aColumnTypes.Size(); ++i) {
        columns_.Push(aColumnTypes[i].type_);
        colNames_.Push(aColumnTypes[i].name_);
    }
    //TODO column names
}
    
void DataTable::Read(const String& aFile)
{
}

/// returns a cache vector
VariantVector& DataTable::GetRowRaw(unsigned int row)
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
    for (unsigned int i = 0; i < colNames_.Size(); ++i) {
        if (colNames_[i] == aColumnName)
            return (int)i;
    }
    return -1;
}

};