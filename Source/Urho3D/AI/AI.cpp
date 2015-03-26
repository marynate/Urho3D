#include "Precompiled.h"

#include "../AI/AI.h"
#include "../Core/Context.h"
#include "../AI/Blackboard.h"

namespace Urho3D {
	void RegisterAILibrary(Context* context)
	{
		Blackboard::RegisterObject(context);
	}
}