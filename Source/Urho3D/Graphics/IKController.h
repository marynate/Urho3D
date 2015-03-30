
#include "../Scene/LogicComponent.h"

#include "../Container/Vector.h"
#include "../Container/Str.h"
#include "../Math/Quaternion.h"
#include "../Math/Vector3.h"


namespace Urho3D
{
	class URHO3D_API IKController : public LogicComponent
	{
		OBJECT(IKController);

		class IKChain;
	public:
		IKController(Context*);
		virtual ~IKController();

		static void RegisterObject(Context*);

		virtual void PostUpdate(float timeStep) override;

		virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

		bool HasIKChain(const StringHash& aName) const;
		void AddIKChain(const StringHash& aName, const String& aStart, const String& aMiddle, const String& aEnd);
		void RemoveIKChain(const StringHash& aName);
		void UpdateIKChainEffector(const StringHash& aName, const Vector3&);
		void UpdateIKChainTwist(const StringHash& aName, float aTwist);
		void ToggleIKChain(const StringHash& aName, bool aEnable);
		float GetIKChainTwist(const StringHash&) const;
		Vector3 GetIKChainEffector(const StringHash&) const;
		bool GetIKChainEnabled(const StringHash&) const;

		static void SolveTwoBoneIK(const Vector3& startJointPos, const Vector3& midJointPos, const Vector3& effectorPos, const Vector3& handlePos, const Vector3& poleVector, float twistValue, Quaternion& qStart, Quaternion& qMid);

	private:
		Vector<IKChain*> chains_;

		static bool VectorsParallel(const Vector3& lhs, const Vector3& rhs, float tolerance);
		static Vector3 RotateVector(const Vector3& vec, const Quaternion& quat);
	};
}