#include <Ignis.h>
#include "RaceShared.h"

class CarController : public ignis::ScriptBehaviour
{
public:
	// Driving
	float DriveForce = 16000.0f;
	float BrakeForce = 14000.0f;
	float ReverseForce = 8000.0f;
	float MaxSpeed = 55.0f;

	// Steering
	float SteerSpeed = 1.35f;
	float SteerRampMin = 6.5f;
	float SteerReduceAt = 8.0f;

	// Grip / Drift
	float LateralDamping = 16.0f;
	float DriftDamping = 2.2f;
	float DriftYawBoost = 1.08f;
	float DriftBrakeFactor = 0.35f;

	// Camera offset
	float CamAccelPull = 2.0f;
	float CamBrakePush = 1.5f;
	float CamSmooth = 3.5f;

private:
	ignis::Entity m_cam_entity;
	bool          m_has_cam = false;
	float         m_cam_base_x = 0.0f;
	float         m_cam_offset = 0.0f;

	ignis::PhysicsBody* GetBody()
	{
		return GetEntity()
			.GetComponent<ignis::RigidBodyComponent>()
			.RuntimeBody.get();
	}

public:
	void OnCreate() override
	{
		GetEntity().ForEachChild([&](ignis::Entity child)
			{
				if (m_has_cam) return;
				if (child.GetComponent<ignis::TagComponent>().Tag == "Camera")
				{
					m_cam_entity = child;
					m_has_cam = true;
					m_cam_base_x = child.GetComponent<ignis::TransformComponent>()
						.Translation.x;
				}
			});
	}

	void OnUpdate(float dt) override
	{
		using namespace ignis;

		auto* body = GetBody();
		if (!body || !body->IsDynamic()) return;

		if (!RaceShared::CanDrive)
		{
			float k = glm::max(0.0f, 1.0f - dt * 10.0f);
			body->SetLinearVelocity(body->GetLinearVelocity() * k);
			body->SetAngularVelocity(body->GetAngularVelocity() * k);
			return;
		}

		const bool kW = Input::IsKeyPressed(KeyCode::W);
		const bool kS = Input::IsKeyPressed(KeyCode::S);
		const bool kA = Input::IsKeyPressed(KeyCode::A);
		const bool kD = Input::IsKeyPressed(KeyCode::D);
		const bool kSpace = Input::IsKeyPressed(KeyCode::Space);

		auto& tc = GetEntity().GetComponent<TransformComponent>();
		glm::quat rot = tc.GetRotationQuat();
		glm::vec3 fwd = rot * glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 rgt = rot * glm::vec3(0.0f, 0.0f, -1.0f);

		glm::vec3 vel = body->GetLinearVelocity();
		float     speed = glm::length(vel);
		float     fwdSpd = glm::dot(vel, fwd);

		// 1. Drive / brake / reverse
		if (kW && !kS)
		{
			if (fwdSpd < MaxSpeed)
				body->ApplyCentralForce(fwd * DriveForce);
		}
		else if (kS && !kW)
		{
			if (fwdSpd > 0.5f)
				body->ApplyCentralForce(-fwd * BrakeForce);
			else if (fwdSpd > -MaxSpeed * 0.35f)
				body->ApplyCentralForce(-fwd * ReverseForce);
		}

		// 2. Steering
		float steerInput = 0.0f;
		if (kA) steerInput = 1.0f;
		if (kD) steerInput = -1.0f;

		glm::vec3 angVel = body->GetAngularVelocity();
		if (glm::abs(steerInput) > 0.01f && speed > 0.3f)
		{
			float lowFactor = glm::clamp(speed / SteerRampMin, 0.0f, 1.0f);
			float highFactor = 1.0f - 0.45f * glm::clamp(
				(speed - SteerReduceAt) / (MaxSpeed - SteerReduceAt), 0.0f, 1.0f);
			float dir = (fwdSpd >= 0.0f) ? 1.0f : -1.0f;
			float targetYaw = steerInput * dir * SteerSpeed * lowFactor * highFactor;
			angVel.y = glm::mix(angVel.y, targetYaw, glm::clamp(dt * 12.0f, 0.0f, 1.0f));
		}
		else
		{
			angVel.y = glm::mix(angVel.y, 0.0f, glm::clamp(dt * 7.0f, 0.0f, 1.0f));
		}
		body->SetAngularVelocity(angVel);

		// 3. Lateral grip / drift
		float     grip = kSpace ? DriftDamping : LateralDamping;
		glm::vec3 latVel = rgt * glm::dot(vel, rgt);
		body->ApplyCentralForce(-latVel * grip * body->GetMass());

		// 4. Handbrake drift
		if (kSpace && speed > 2.0f)
		{
			glm::vec3 driftAng = body->GetAngularVelocity();
			driftAng.y += steerInput * 0.5f;
			float yawLimit = SteerSpeed * 1.45f;
			driftAng.y = glm::clamp(driftAng.y, -yawLimit, yawLimit);
			body->SetAngularVelocity(driftAng);
			body->ApplyCentralForce(-fwd * fwdSpd * DriftBrakeFactor * body->GetMass());
		}

		// 5. Camera dynamic offset
		if (m_has_cam)
		{
			float targetOffset = 0.0f;
			if (kW && !kS)           targetOffset = -CamAccelPull;
			else if (kS && fwdSpd > 0.5f) targetOffset = CamBrakePush;

			m_cam_offset += (targetOffset - m_cam_offset)
				* glm::clamp(dt * CamSmooth, 0.0f, 1.0f);

			auto& camTc = m_cam_entity.GetComponent<ignis::TransformComponent>();
			camTc.Translation.x = m_cam_base_x + m_cam_offset;
		}
	}
};

IGNIS_SCRIPT(CarController)