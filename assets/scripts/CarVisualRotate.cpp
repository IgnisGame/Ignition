#include <Ignis.h>

class CarVisualRotate : public ignis::ScriptBehaviour
{
public:
	// Visual steering rotation (Y-axis turn)
	float SteerVisualMaxAngle = 12.0f;
	float SteerVisualSpeed = 2.0f;
	float SteerVisualReturnSpeed = 3.0f;

private:
	float m_current_steer_angle = 0.0f;

public:
	void OnCreate() override
	{
		// Initialize visual rotation to zero
		m_current_steer_angle = 0.0f;
	}

	void OnUpdate(float dt) override
	{
		using namespace ignis;

		// Get parent entity (the Car with physics body)
		Entity parent = GetEntity().GetParent();
		if (!parent)
			return;

		// Check if parent has a rigid body component
		if (!parent.HasComponent<RigidBodyComponent>())
			return;

		auto& parent_rb = parent.GetComponent<RigidBodyComponent>();
		auto* body = parent_rb.RuntimeBody.get();
		if (!body || !body->IsDynamic())
			return;

		// Get parent's velocity to check if car is moving
		glm::vec3 vel = body->GetLinearVelocity();
		float speed = glm::length(vel);

		// Read steering input
		const bool kA = Input::IsKeyPressed(KeyCode::A);
		const bool kD = Input::IsKeyPressed(KeyCode::D);

		float steerInput = 0.0f;
		if (kA) steerInput = 1.0f;
		if (kD) steerInput = -1.0f;

		// Calculate target visual steering angle
		float targetSteerAngle = 0.0f;
		if (speed > 1.0f)  // Only show visual steering when moving
		{
			targetSteerAngle = steerInput * SteerVisualMaxAngle;
		}

		// Smoothly interpolate to target angle
		float steerBlendSpeed = (glm::abs(targetSteerAngle) > glm::abs(m_current_steer_angle))
			? SteerVisualSpeed : SteerVisualReturnSpeed;
		m_current_steer_angle += (targetSteerAngle - m_current_steer_angle) * glm::clamp(dt * steerBlendSpeed, 0.0f, 1.0f);

		// Debug logging (remove after testing)
		static int frame_count = 0;
		if (++frame_count % 60 == 0 && glm::abs(steerInput) > 0.01f)
		{
			Log::CoreInfo("CarVisualRotate: steerInput={}, targetAngle={}, currentAngle={}, speed={}", 
				steerInput, targetSteerAngle, m_current_steer_angle, speed);
		}

		// Apply visual steering rotation as local Y-axis rotation
		auto& tc = GetEntity().GetComponent<TransformComponent>();
		tc.Rotation = glm::vec3(0.0f, m_current_steer_angle, 0.0f);
	}
};

IGNIS_SCRIPT(CarVisualRotate)