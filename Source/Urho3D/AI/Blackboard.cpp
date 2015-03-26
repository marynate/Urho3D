#include "Precompiled.h"

#include "../AI/Blackboard.h"
#include "../Core/Context.h"
#include "../Graphics/DebugRenderer.h"
#ifdef URHO3D_PHYSICS
	#include "../Physics/PhysicsEvents.h"
	#include "../Physics/PhysicsWorld.h"
#endif
#include "../Core/CoreEvents.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"

namespace Urho3D
{

extern const char* LOGIC_CATEGORY;

class Blackboard::Cell
{    
public:
    HashMap<StringHash,float> times_;
    VariantMap data_;
    
    void Update(float td) {
        //check for items that need to be removed
        HashMap<StringHash,float>::Iterator it = times_.Begin();
        for (; it != times_.End(); ++it) {
            it->second_ -= td;
            if (it->second_ <= 0) {
                data_.Erase(it->first_);
                it = times_.Erase(it);
            }
        }
    }
    
    void SaveInto(VariantMap& target) { //we add a variant map into this
        if (times_.Size() > 0) {
            VariantMap map = VariantMap();
            for (HashMap<StringHash,float>::ConstIterator cit = times_.Begin(); cit != times_.End(); ++cit)
                map[cit->first_] = cit->second_;
            target["times"] = map;
        }
        target["values"] = data_;
    }
    
    void ReadFrom(const VariantMap& input) { //we get the variant map we added during SaveInto
        VariantMap::ConstIterator cit = input.Find("times");
        if (cit != input.End()) {
            const VariantMap& times = cit->second_.GetVariantMap();
            VariantMap::ConstIterator ccit = times.Begin();
            for (; ccit != times.End(); ++ccit)
                times_[ccit->first_] = ccit->second_.GetFloat();
                
        }
		cit = input.Find("values");
		if (cit != input.End())
			data_ = cit->second_.GetVariantMap();
    }
};


Blackboard::Blackboard(Context* context) : 
    Component(context), 
    dimensions_(32,32),
    area_(1000.0f,1000.0f),
	cells_(0x0)
{
    SetupBlockmap();
}

Blackboard::~Blackboard()
{
    delete[] cells_;
}

void Blackboard::SetupBlockmap() 
{
    history_.Clear();
    if (cells_ != 0x0) {
        delete[] cells_;
        cells_ = 0x0;
    }
    
    cellCt_ = dimensions_.x_ * dimensions_.y_;
    if (cellCt_ > 0)
        cells_ = new Cell[cellCt_];
}

void Blackboard::RegisterObject(Context* context)
{
    context->RegisterFactory<Blackboard>(LOGIC_CATEGORY);
    
    ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE("Grid Size", GetGridSizeAttr, SetGridSizeAttr, IntVector2, IntVector2(32,32), AM_DEFAULT);
    ACCESSOR_ATTRIBUTE("Area", GetAreaAttr, SetAreaAttr, Vector2, Vector2(1000.0f,1000.0f), AM_DEFAULT);
    ACCESSOR_ATTRIBUTE("Cell Data", GetCellDataAttr, SetCellDataAttr, VariantVector, Variant::emptyVariantVector, AM_NOEDIT);
}

void Blackboard::UpdateEventSubscription()
{
	Scene* scene = GetScene();
	if (scene == 0x0)
		return; //??log an error?
#ifdef URHO3D_PHYSICS
	PhysicsWorld* world = scene->GetOrCreateComponent<PhysicsWorld>();
	if (world != 0x0)
		SubscribeToEvent(world, E_PHYSICSPRESTEP, HANDLER(Blackboard, HandlePhysicsPreStep));
#endif
}

#ifdef URHO3D_PHYSICS
void Blackboard::HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData) {
	using namespace SceneUpdate;

	const float timeDelta = eventData[P_TIMESTEP].GetFloat();
	if (cells_ != 0x0) {
		for (int i = 0; i < cellCt_; ++i)
			cells_[i].Update(timeDelta);
	}
}
#endif

void Blackboard::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    float x = 0 - area_.x_/2.0f;
    float y = 0 - area_.y_/2.0f;
    
    Color c(0.5f,0.5f,0.5f,0.7f);
    
    //TODO: ermmm .... how slow is this going to be?
    //X-grid
	for (float cx = x; cx <= area_.x_/2.0f; cx += area_.x_/dimensions_.x_) {
        debug->AddLine(Vector3(cx, 0, y), Vector3(cx, 0, y + area_.y_), c);
    }
    //Y-grid
    for (float cy = y; cy <= area_.y_/2.0f; cy += area_.y_/dimensions_.y_) {
        debug->AddLine(Vector3(x, 0, cy), Vector3(x + area_.x_, 0, cy), c);
    }
}

void Blackboard::Update(float timeStep)
{
	using namespace SceneUpdate;

	if (cells_ != 0x0) {
		for (int i = 0; i < cellCt_; ++i)
			cells_[i].Update(timeStep);
	}
}

// Query for specifically named values, we'll only get the values
void Blackboard::QueryFor(BBQueryResult& query, const StringHash& key)
{
    if (cells_ == 0x0)
        return; //no reason to query?
    Vector3& loc = query.where.center_;
    float radius = query.where.radius_; //square for simpler distance compare
    
    //cube coordinates
    const int left = toCoordX(loc.x_ - radius);
    const int right = toCoordX(loc.x_ + radius);
    const int bottom = toCoordY(loc.z_ - radius);
    const int top = toCoordY(loc.z_ + radius);
    
    for (int x = left; x <= right; ++x) {
        for (int y = bottom; y <= top; ++y) {
            Cell& cell = cells_[dimensions_.x_ * y + x];
            VariantMap::ConstIterator cit = cell.data_.Find(key);
            if (cit != cell.data_.End())
                query.results.Push(BBQueryItem(getCellPosition(x,y), key, cit->second_));
        }
    }
}

// Fetch all values in a region of space, we'll get an item for each cell that contains it's Variant map of data
void Blackboard::Query(BBQueryResult& query)
{
    if (cells_ == 0x0)
        return; //no reason to query?
    Vector3& loc = query.where.center_;
    float radius = query.where.radius_; //square for simpler distance compare
    
    //cube coordinates
    const int left = toCoordX(loc.x_ - radius);
    const int right = toCoordX(loc.x_ + radius);
    const int bottom = toCoordY(loc.z_ - radius);
    const int top = toCoordY(loc.z_ + radius);
    
    for (int x = left; x <= right; ++x) {
        for (int y = bottom; y <= top; ++y) {
            Cell& cell = cells_[dimensions_.x_ * y + x];
            query.results.Push(BBQueryItem(getCellPosition(x,y), cell.data_));
        }
    }
}

Vector3 Blackboard::getCellPosition(int x, int y) const
{
    //BottomLeft + (CellSize * CellPosition) + HalfCellSize <-- in order to be in the dead center of the cell
    const float dimX = area_.x_ / dimensions_.x_;
    //const float dimY = area_.y / dimensions_.y;
    const float dimZ = area_.y_ / dimensions_.y_;
    const float cx = (0-area_.x_ / 2.0f) + dimX * x + dimX/2.0f;
    //float y = (0-area_.y/2f) + dimY * x + dimY/2f;
    const float cz = (0-area_.y_ / 2.0f) + dimZ * y + dimZ/2.0f;
    
    return Vector3(cx, 0, cz);
}

int Blackboard::toCoordX(float in) const {
    return ((int)(in-area_.x_/2.0f)) % dimensions_.x_;
}

int Blackboard::toCoordY(float in) const {
    return ((int)(in-area_.y_/2.0f)) % dimensions_.y_;
}

int Blackboard::toCoordZ(float) const {
    return 0;
}

const IntVector2& Blackboard::GetGridSizeAttr() const
{
    return dimensions_;
}

void Blackboard::SetGridSizeAttr(const IntVector2& in)
{
    dimensions_ = in;
    SetupBlockmap();
}

const Vector2& Blackboard::GetAreaAttr() const
{
    return area_;
}

void Blackboard::SetAreaAttr(const Vector2& in)
{
    area_ = in; //does not affect the grid, just how we map coordinates to the grid
}

const VariantVector& Blackboard::GetCellDataAttr() const
{
    if (cells_ == 0x0) //??
		return Variant::emptyVariantVector;
    VariantVector vec;
    for (int i = 0; i < cellCt_; ++i) {
        VariantMap targetMap;
        cells_[cellCt_].SaveInto(targetMap);
        vec.Push(targetMap);
    }
    return vec;
}

void Blackboard::SetCellDataAttr(const VariantVector& vec)
{
    for (unsigned int i = 0; i < vec.Size(); ++i) {
        const VariantMap& map = vec[i].GetVariantMap();
        if (map.Size() > 0)
            cells_[i].ReadFrom(map);
    }
}

void Blackboard::PlaceData(const Vector2& where, const StringHash& key, Variant& data, float expiration)
{
    const int x = ((int)(where.x_-area_.x_/2.0f)) % dimensions_.x_;
    const int y = ((int)(where.y_-area_.y_/2.0f)) % dimensions_.y_;
    cells_[x*y].data_[key] = data;
    if (expiration > 0)
        cells_[x*y].times_[key] = expiration;
}

void Blackboard::PlaceData(const Vector3& where, const StringHash& key, Variant& data, float expiration)
{
    const int x = ((int)(where.x_-area_.x_/2.0f)) % dimensions_.x_;
    const int y = ((int)(where.y_-area_.y_/2.0f)) % dimensions_.y_;
    cells_[x*y].data_[key] = data;
    if (expiration > 0)
        cells_[x*y].times_[key] = expiration;
}

Variant Blackboard::GetData(const Vector2& where, const StringHash& key)
{
    const int x = ((int)(where.x_-area_.x_/2.0f)) % dimensions_.x_;
    const int y = ((int)(where.y_-area_.y_/2.0f)) % dimensions_.y_;
    VariantMap::ConstIterator cit = cells_[x*y].data_.Find(key);
    if (cit != cells_[dimensions_.x_ * y + x].data_.End())
        return cit->second_;
    return Variant();
}

Variant Blackboard::GetData(const Vector3& where, const StringHash& key)
{
    const int x = ((int)(where.x_-area_.x_/2.0f)) % dimensions_.x_;
    const int y = ((int)(where.y_-area_.y_/2.0f)) % dimensions_.y_;
    VariantMap::ConstIterator cit = cells_[x*y].data_.Find(key);
    if (cit != cells_[dimensions_.x_ * y + x].data_.End())
        return cit->second_;
    return Variant();
}

bool Blackboard::ContainsData(const Vector2& where, const StringHash& key)
{
    const int x = ((int)(where.x_-area_.x_/2.0f)) % dimensions_.x_;
    const int y = ((int)(where.y_-area_.y_/2.0f)) % dimensions_.y_;
    VariantMap::ConstIterator cit = cells_[dimensions_.x_ * y + x].data_.Find(key);
    return cit != cells_[dimensions_.x_ * y + x].data_.End();
}

bool Blackboard::ContainsData(const Vector3& where, const StringHash& key)
{
    const int x = ((int)(where.x_-area_.x_/2.0f)) % dimensions_.x_;
    const int y = ((int)(where.y_-area_.y_/2.0f)) % dimensions_.y_;
    VariantMap::ConstIterator cit = cells_[dimensions_.x_ * y + x].data_.Find(key);
    return cit != cells_[dimensions_.x_ * y + x].data_.End();
}

}