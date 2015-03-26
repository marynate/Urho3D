#include "Precompiled.h"

#include "IKController.h"

#include "../Core/Context.h"
#include "../Graphics/DebugRenderer.h"
#include "../Math/MathDefs.h"
#include "../Scene/Node.h"

namespace Urho3D
{

	extern const char* GEOMETRY_CATEGORY;

	class IKController::IKChain
	{
		friend class IKController;

		StringHash Name;
		WeakPtr<Node> StartNode;
		WeakPtr<Node> EndNode;
		WeakPtr<Node> MiddleNode;
		Vector3 Target;
		float Twist;
		bool Enabled;

		void Update()
		{
			Node* start = StartNode.Get();
			Node* mid = MiddleNode.Get();
			Node* end = EndNode.Get();

			if (start == 0x0 || mid == 0x0 || end == 0x0)
				return; //can't do anythings

			Vector3 poleVector = (start->GetWorldPosition() - end->GetWorldPosition()).Normalized();

			Quaternion startRot;
			Quaternion midRot;
			IKController::SolveTwoBoneIK(start->GetWorldPosition(), mid->GetWorldPosition(), end->GetWorldPosition(),
				Target, poleVector, Twist, startRot, midRot);

			// set the rotations of our joints
			start->SetWorldRotation(startRot);
			mid->SetWorldRotation(midRot);
		}
	};

IKController::IKController(Context* context) :
	LogicComponent(context)
{
}
IKController::~IKController()
{
	for (unsigned int i = 0; i < chains_.Size(); ++i)
		delete chains_[i];
	chains_.Clear();
}

void IKController::RegisterObject(Context* context)
{
	context->RegisterFactory<IKController>(GEOMETRY_CATEGORY);

	COPY_BASE_ATTRIBUTES(LogicComponent);
}

void IKController::FixedPostUpdate(float timeStep)
{
	if (IsEnabledEffective())
	{
		// update each active IK chain
		for (unsigned int i = 0; i < chains_.Size(); ++i)
		{
			if (chains_[i]->Enabled)
			{
				chains_[i]->Update();
			}
		}
	}
}

void IKController::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
	for (unsigned int i = 0; i < chains_.Size(); ++i)
	{
		if (chains_[i]->Enabled)
		{
			// draw the debug lines for indicating the state of the IK chains
			

			// draw the handle position
			debug->AddSphere(Sphere(chains_[i]->Target, 0.25f), Color(1.0f,0.0f,0.0f));
			debug->AddSphere(Sphere(chains_[i]->Target, 0.15f), Color(0.5f, 0.0f, 1.0f));

			// draw line from start -> end
			Node* sn = chains_[i]->StartNode.Get();
			Node* end = chains_[i]->EndNode.Get();
			Node* mid = chains_[i]->MiddleNode.Get();

			if (sn != 0x0)
				debug->AddNode(sn);
			if (end != 0x0)
				debug->AddNode(end);
			if (mid != 0x0)
				debug->AddNode(end);

			if (sn != 0x0 && end != 0x0)
				debug->AddLine(sn->GetWorldPosition(), end->GetWorldPosition(), Color(1.0f, 1.0f, 0.0f));
		}
	}
}

bool IKController::HasIKChain(const StringHash& aName) const
{
	for (unsigned int i = 0; i < chains_.Size(); ++i)
	{
		if (chains_[i]->Name == aName)
			return true;
	}
	return false;
}

void IKController::AddIKChain(const StringHash& aName, const String& aStart, const String& aMiddle, const String& aEnd)
{
	if (HasIKChain(aName))
		return;

	// Not found, add a new
	WeakPtr<Node> StartNode(GetNode()->GetChild(aStart, true));
	WeakPtr<Node> MiddleNode(GetNode()->GetChild(aMiddle, true));
	WeakPtr<Node> EndNode(GetNode()->GetChild(aEnd, true));
	

	if (!StartNode.Expired() && !EndNode.Expired() && !MiddleNode.Expired())
	{
		IKChain* chain = new IKChain();
		chain->StartNode = StartNode;
		chain->MiddleNode = MiddleNode;
		chain->EndNode = EndNode;
		chain->Enabled = false;
		chain->Twist = 0.0f;
		chains_.Push(chain);
	}
}

void IKController::RemoveIKChain(const StringHash& aName)
{
	for (unsigned int i = 0; i < chains_.Size(); ++i)
	{
		if (chains_[i]->Name == aName)
		{
			// do anything necessary
			//???

			//remove it now
			chains_.Erase(i);
			return;
		}
	}
}

void IKController::UpdateIKChainEffector(const StringHash& aName, const Vector3& aTargetPos)
{
	for (unsigned int i = 0; i < chains_.Size(); ++i)
	{
		if (chains_[i]->Name == aName)
		{
			chains_[i]->Target = aTargetPos;
			return;
		}
	}
}

void IKController::UpdateIKChainTwist(const StringHash& aName, float aTwist)
{
	for (unsigned int i = 0; i < chains_.Size(); ++i)
	{
		if (chains_[i]->Name == aName)
		{
			chains_[i]->Twist = aTwist;
			return;
		}
	}
}

void IKController::ToggleIKChain(const StringHash& aName, bool aEnable)
{
	for (unsigned int i = 0; i < chains_.Size(); ++i)
	{
		if (chains_[i]->Name == aName)
		{
			chains_[i]->Enabled = aEnable;
			return;
		}
	}
}

float IKController::GetIKChainTwist(const StringHash& aName) const
{
	for (unsigned int i = 0; i < chains_.Size(); ++i)
	{
		if (chains_[i]->Name == aName)
			return chains_[i]->Twist;
	}
	return 0.0f;
}
Vector3 IKController::GetIKChainEffector(const StringHash& aName) const
{
	for (unsigned int i = 0; i < chains_.Size(); ++i)
	{
		if (chains_[i]->Name == aName)
			return chains_[i]->Target;
	}
	return Vector3();

}
bool IKController::GetIKChainEnabled(const StringHash& aName) const
{
	for (unsigned int i = 0; i < chains_.Size(); ++i)
	{
		if (chains_[i]->Name == aName)
			return chains_[i]->Enabled;
	}
	return false;
}

bool IKController::VectorsParallel(const Vector3& lhs, const Vector3& rhs, float tolerance)
{
	Vector3 v1 = lhs.Normalized();
	Vector3 v2 = rhs.Normalized();
	float dp = v1.DotProduct(v2);
	return ((dp > 1.0f) ? (dp - 1.0f <= tolerance) : 1.0f - dp <= tolerance);
}

Vector3 IKController::RotateVector(const Vector3& v, const Quaternion& q)
{
	float rw = -q.x_ * v.x_ - q.y_ * v.y_ - q.z_ * v.z_;
	float rx = q.w_ * v.x_ + q.y_ * v.z_ - q.z_ * v.y_;
	float ry = q.w_ * v.y_ + q.z_ * v.x_ - q.x_ * v.z_;
	float rz = q.w_ * v.z_ + q.x_ * v.y_ - q.y_ * v.x_;
	return Vector3(-rw * q.x_ + rx * q.w_ - ry * q.z_ + rz * q.y_,
		-rw * q.y_ + ry * q.w_ - rz * q.x_ + rx * q.z_,
		-rw * q.z_ + rz * q.w_ - rx * q.y_ + ry * q.x_);
}

#define kFloatEpsilon           1.0e-5F
#define kEpsilon 1.0e-5
#define absoluteValue(x) ((x) < 0 ? (-(x)) : (x))

void IKController::SolveTwoBoneIK(const Vector3& startJointPos, const Vector3& midJointPos, const Vector3& effectorPos, const Vector3& handlePos, const Vector3& poleVector, float twistValue, Quaternion& qStart, Quaternion& qMid)
{
	// vector from startJoint to midJoint
	Vector3 vector1 = midJointPos - startJointPos;
	// vector from midJoint to effector
	Vector3 vector2 = effectorPos - midJointPos;
	// vector from startJoint to handle
	Vector3 vectorH = handlePos - startJointPos;
	// vector from startJoint to effector
	Vector3 vectorE = effectorPos - startJointPos;
	// lengths of those vectors
	float length1 = vector1.Length();
	float length2 = vector2.Length();
	float lengthH = vectorH.Length();
	// component of the vector1 orthogonal to the vectorE
	Vector3 vectorO =
		vector1 - vectorE*((vector1*vectorE) / (vectorE*vectorE));

	//////////////////////////////////////////////////////////////////
	// calculate q12 which solves for the midJoint rotation
	//////////////////////////////////////////////////////////////////
	// angle between vector1 and vector2
	float vectorAngle12 = vector1.Angle(vector2);
	// vector orthogonal to vector1 and 2
	Vector3 vectorCross12 = vector1.CrossProduct(vector2);
	float lengthHsquared = lengthH*lengthH;
	// angle for arm extension 
	float cos_theta =
		(lengthHsquared - length1*length1 - length2*length2)
		/ (2 * length1*length2);
	if (cos_theta > 1)
		cos_theta = 1;
	else if (cos_theta < -1)
		cos_theta = -1;
	float theta = acos(cos_theta);
	// quaternion for arm extension
	Quaternion q12(theta - vectorAngle12, vectorCross12);

	//////////////////////////////////////////////////////////////////
	// calculate qEH which solves for effector rotating onto the handle
	//////////////////////////////////////////////////////////////////
	// vector2 with quaternion q12 applied
	vector2 = IKController::RotateVector(vector2, q12); // vector2.rotateBy(q12);
	// vectorE with quaternion q12 applied
	vectorE = vector1 + vector2;
	// quaternion for rotating the effector onto the handle
	Quaternion qEH(vectorE, vectorH);

	//////////////////////////////////////////////////////////////////
	// calculate qNP which solves for the rotate plane
	//////////////////////////////////////////////////////////////////
	// vector1 with quaternion qEH applied
	vector1 = IKController::RotateVector(vector1, qEH); // vector1.rotateBy(qEH);
	if (IKController::VectorsParallel(vector1, vectorH, kFloatEpsilon))
		// singular case, use orthogonal component instead
		vector1 = IKController::RotateVector(vectorO, qEH); // vectorO.rotateBy(qEH);
	// quaternion for rotate plane
	Quaternion qNP;
	if (!IKController::VectorsParallel(poleVector, vectorH, kFloatEpsilon) && (lengthHsquared != 0)) {
		// component of vector1 orthogonal to vectorH
		Vector3 vectorN =
			vector1 - vectorH*((vector1*vectorH) / lengthHsquared);
		// component of pole vector orthogonal to vectorH
		Vector3 vectorP =
			poleVector - vectorH*((poleVector*vectorH) / lengthHsquared);
		float dotNP = vectorN.DotProduct(vectorP) / (vectorN.Length()*vectorP.Length());
		if (absoluteValue(dotNP + 1.0) < kEpsilon) {
			// singular case, rotate halfway around vectorH
			Quaternion qNP1(Urho3D::M_PI, vectorH);
			qNP = qNP1;
		}
		else {
			Quaternion qNP2(vectorN, vectorP);
			qNP = qNP2;
		}
	}

	//////////////////////////////////////////////////////////////////
	// calculate qTwist which adds the twist
	//////////////////////////////////////////////////////////////////
	Quaternion qTwist(twistValue, vectorH);

	// quaternion for the mid joint
	qMid = q12;
	// concatenate the quaternions for the start joint
	qStart = qEH*qNP*qTwist;
}

}