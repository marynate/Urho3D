#pragma once

#include "../Scene/Component.h"
#include "../Core/Variant.h"
#include "../Math/Sphere.h"

namespace Urho3D
{

class BBQueryItem
{
public:
    StringHash name;
    Vector3 where;
    Variant value;
    
	BBQueryItem() {}

	BBQueryItem(Vector3& where, const Variant& v)
	{
		this->where = where;
		value =v;
	}

    BBQueryItem(Vector3& where, const StringHash& name, const Variant& v) {
        this->where = where;
        value = v; 
        this->name = name;
    }
};

class BBQueryResult {
public:
    Sphere where;
    Vector<BBQueryItem> results; 
};

/// Block-map spatial storage of data in the scene
class Blackboard : public Component
{
    OBJECT(Blackboard)
public:
    /// Construct.
    Blackboard(Context* context);
    /// Destruct.
    ~Blackboard();
    /// Register object factory.
    static void RegisterObject(Context* context);
 
    /// Draw our cell grid/resolution when debug rendering
    virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);
 
    /// Update the expirations of the variants
    void Update(float timeStep);
    
    /// Get cell data for serialization
    const VariantVector& GetCellDataAttr() const;
    /// Set cell data for deserialization
    void SetCellDataAttr(const VariantVector&);
    
    // Query for a specifically named value
    void QueryFor(BBQueryResult&, const StringHash& key);
    // Fetch all values in a region of space
    void Query(BBQueryResult&);
    
    /// Place data into the blackboard block-map (Y dimension is ignored)
    void PlaceData(const Vector3& where, const StringHash& key, Variant& data, float expiration = 0.0f);
    /// Place data in the blackboard block-map
    void PlaceData(const Vector2& where, const StringHash& key, Variant& data, float expiration = 0.0f);
    /// Get data from the blackboard block-map (the Y dimension is ignored)
    Variant GetData(const Vector3& where, const StringHash& key);
    /// Get data from the blackboard block-map
    Variant GetData(const Vector2& where, const StringHash& key);
    /// Checks whether we have a piece of data in the blackboard (Y dimension is ignored)
    bool ContainsData(const Vector3& where, const StringHash& key);
    /// Checks whether we have a piece of data in the blackboard
    bool ContainsData(const Vector2& where, const StringHash& key);
    
	const IntVector2& GetGridSizeAttr() const;
	const Vector2& GetAreaAttr() const;
	void SetGridSizeAttr(const IntVector2&);
	void SetAreaAttr(const Vector2&);

protected:
	void HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData);
	void UpdateEventSubscription();

private:
	class Cell;

    Vector3 getCellPosition(int x, int y) const;
    int toCoordX(float) const;
    int toCoordY(float) const;
    int toCoordZ(float) const;

    PODVector<Cell*> history_;
    void SetupBlockmap();
    
    Cell* cells_;
    int cellCt_;
    Vector2 area_;
    IntVector2 dimensions_;

	VariantVector emptyRet_;
};

}