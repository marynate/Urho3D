//
// Copyright (c) 2008-2015 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Core/Object.h"

namespace Urho3D
{

	EVENT(E_NAVIGATION_AREA_REBUILT, NavigationAreaRebuilt)
	{
		PARAM(P_BOUNDSMIN, BoundsMin); // Vector3
		PARAM(P_BOUNDSMAX, BoundsMax); // Vector3
	}

	EVENT(E_NAVIGATION_AGENT_REPOSITION, NavigationAgentReposition)
	{
		PARAM(P_POSITION, Position); // Vector3
		PARAM(P_VELOCITY, Velocity); // Vector3
	}

	EVENT(E_NAVIGATION_OBSTACLE_ADDED, NavigationObstacleAdded)
	{
		PARAM(P_NODE, Node); // Node*
		PARAM(P_POSITION, Position); // Vector3
		PARAM(P_RADIUS, Radius); // float
	}

	EVENT(E_NAVIGATION_OBSTACLE_REMOVED, NavigationObstacleRemoved)
	{
		PARAM(P_POSITION, Position); // Vector3
	}
}