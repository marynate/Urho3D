#pragma once

#include "../Core/Object.h"

namespace Urho3D
{

///AI subsystem
class AI : public Object
{
    OBJECT(AI);
public:
    AI(Context* context);
    virtual ~AI();
    


private:
    
};

void RegisterAILibrary(Context*);

}