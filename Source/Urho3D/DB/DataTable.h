#pragma once

#include "../Core/Variant.h"
#include "../DB/DataRow.h"

namespace Urho3D
{

    class ColumnInfo
    {
    public:
        ColumnInfo(VariantType aType, const String& aName);
		ColumnInfo() {} //for STL
        
        VariantType type_;
        String name_;
    };

    class DataTable {
    public:
        DataTable(); //new empty datatable
        DataTable(Vector<ColumnInfo> aColumnTypes);
        
        void Read(const String& aFile);
        
        void ReadJSON(const String& aFile);
        void ReadXML(const String& aFile);
        
        /// returns a cache vector
        VariantVector& GetRowRaw(unsigned int row);
        DataRow* GetRow(unsigned int row);
        
        int Rows() const;
        int Columns() const;
        const String& GetColumnName(unsigned int aIdx) const;
        int GetColumnIdx(const String& aColumnName) const;
        
    private:
        Vector<VariantVector> rows_;
        Vector<VariantType> columns_;
        Vector<String> colNames_;
        VariantVector emptyRet_;
    };

};