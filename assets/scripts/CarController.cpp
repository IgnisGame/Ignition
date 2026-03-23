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

	// Engine SFX range
	float EngineSfxMinVolume = 1.0f;
	float EngineSfxMaxVolume = 10.0f;
	float EngineSfxMinPitch = 0.1f;
	float EngineSfxMaxPitch = 1.0f;

	float DriftSfxThreshold = 1.5f;
	float DriftSfxLatSpeedMax = 18.0f;
	float DriftSfxMinVolume = 1.0f;
	float DriftSfxMaxVolume = 7.0f;
	float DriftSfxMinPitch = 0.7f;
	float DriftSfxMaxPitch = 1.0f;

private:
	ignis::Entity m_cam_entity;
	bool          m_has_cam = false;
	float         m_cam_base_x = 0.0f;
	float         m_cam_offset = 0.0f;

	ignis::UUID   m_engine_sfx_id = ignis::UUID::Invalid;
	bool          m_has_engine_sfx = false;

	ignis::UUID   m_drift_sfx_id = ignis::UUID::Invalid;
	bool          m_has_drift_sfx = false;

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
				const auto& tag = child.GetComponent<ignis::TagComponent>().Tag;

				if (!m_has_cam && tag == "Camera")
				{
					m_cam_entity = child;
					m_has_cam = true;
					m_cam_base_x = child.GetComponent<ignis::TransformComponent>()
						.Translation.x;
				}

				if (!m_has_engine_sfx && tag == "EngineSFX")
				{
					m_engine_sfx_id = child.GetID();
					m_has_engine_sfx = true;
				}

				if (!m_has_drift_sfx && tag == "DriftSFX")
				{
					m_drift_sfx_id = child.GetID();
					m_has_drift_sfx = true;
				}
			});

		if (m_has_engine_sfx)
		{
			auto* audio = GetScene()->GetAudioSystem();
			audio->SetVolume(m_engine_sfx_id, EngineSfxMinVolume);
			audio->SetPitch(m_engine_sfx_id, EngineSfxMinPitch);
		}
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

			auto* audio = GetScene()->GetAudioSystem();
			if (m_has_engine_sfx && audio->IsPlaying(m_engine_sfx_id))
				audio->Stop(m_engine_sfx_id);
			if (m_has_drift_sfx && audio->IsPlaying(m_drift_sfx_id))
				audio->Stop(m_drift_sfx_id);
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

		auto* audio = GetScene()->GetAudioSystem();

		// 6. Engine SFX 
		{
			float t = glm::clamp(speed / MaxSpeed, 0.0f, 1.0f);
			float volume = glm::mix(EngineSfxMinVolume, EngineSfxMaxVolume, t);
			float pitch = glm::mix(EngineSfxMinPitch, EngineSfxMaxPitch, t);

			audio->SetVolume(m_engine_sfx_id, volume);
			audio->SetPitch(m_engine_sfx_id, pitch);

			if (t > 0.01)
			{
				if (!audio->IsPlaying(m_engine_sfx_id)) audio->Play(m_engine_sfx_id);
			}
			else
			{
				if (audio->IsPlaying(m_engine_sfx_id)) audio->Stop(m_engine_sfx_id);
			}
		}

		// 7. Drift SFX
		if (m_has_drift_sfx)
		{
			float latSpeed = glm::length(latVel);

			if (latSpeed > DriftSfxThreshold)
			{
				float t = glm::clamp(
					(latSpeed - DriftSfxThreshold) /
					(DriftSfxLatSpeedMax - DriftSfxThreshold),
					0.0f, 1.0f);

				audio->SetVolume(m_drift_sfx_id,
					glm::mix(DriftSfxMinVolume, DriftSfxMaxVolume, t));
				audio->SetPitch(m_drift_sfx_id,
					glm::mix(DriftSfxMinPitch, DriftSfxMaxPitch, t));

				if (!audio->IsPlaying(m_drift_sfx_id))
					audio->Play(m_drift_sfx_id);
			}
			else
			{
				if (audio->IsPlaying(m_drift_sfx_id))
					audio->Stop(m_drift_sfx_id);
			}
		}
	}
};

IGNIS_SCRIPT(CarController)