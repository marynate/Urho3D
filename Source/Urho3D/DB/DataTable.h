#pragma once

#include "../Core/Variant.h"

namespace Urho3D
{
	class JSONFile;
	class XMLFile;

    class DataTable : public RefCounted {
    public:
        DataTable(); //new empty datatable
        
        void ReadJSON(JSONFile* aFile);
        void ReadXML(XMLFile* aFile);
        
        /// returns a cache vector
        VariantVector& GetRow(unsigned int row);
        
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