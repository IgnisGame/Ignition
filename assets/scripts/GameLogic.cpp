#include <Ignis.h>
#include "RaceShared.h"
#include <cstdio>
#include <cmath>
#include <string>

static ignis::Entity FindByTag(ignis::Scene* scene, const std::string& tag)
{
	auto view = scene->GetAllEntitiesWith<ignis::TagComponent>();
	for (auto e : view)
	{
		ignis::Entity entity(e, scene);
		if (entity.GetComponent<ignis::TagComponent>().Tag == tag)
			return entity;
	}
	return {};
}

static std::string FormatTime(float t)
{
	if (t < 0.0f) t = 0.0f;
	int min = static_cast<int>(t) / 60;
	int sec = static_cast<int>(t) % 60;
	int cs = static_cast<int>((t - std::floor(t)) * 100.0f);
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%02d:%02d.%02d", min, sec, cs);
	return buf;
}

static void SetPanelVisible(ignis::Entity panel, bool visible)
{
	if (!panel.IsValid()) return;
	if (panel.HasComponent<ignis::CanvasComponent>())
		panel.GetComponent<ignis::CanvasComponent>().Visible = visible;
}

static void SetText(ignis::Entity e, const std::string& text)
{
	if (!e.IsValid()) return;
	if (e.HasComponent<ignis::UITextComponent>())
		e.GetComponent<ignis::UITextComponent>().Text = text;
}

static int TagToCheckpointIndex(const std::string& tag)
{
	if (tag == "Checkpoint1") return 0;
	if (tag == "Checkpoint2") return 1;
	if (tag == "Checkpoint3") return 2;
	if (tag == "Checkpoint4") return 3;
	if (tag == "Finish")      return 4;
	return -1;
}

class GameManager : public ignis::ScriptBehaviour
{
public:
	float CountdownDuration = 3.0f;
	float SpeedScale = 3.6f;
	float CarMaxSpeed = 55.0f;

	void NotifyCheckpoint(int index)
	{
		if (m_state != State::Racing) return;
		if (index != m_next_cp) return;

		m_next_cp++;

		if (index < 4)
		{
			char buf[32];
			std::snprintf(buf, sizeof(buf), "CP  %d / 4", m_next_cp);
			SetText(m_txt_checkpoint, buf);
		}
		else
		{
			SetText(m_txt_checkpoint, "FINISH!");
			TransitionTo(State::Finished);
		}
	}

	void OnStartClicked() { if (m_state == State::MainMenu) TransitionTo(State::Countdown); }
	void OnResumeClicked() { if (m_state == State::Paused)   TransitionTo(State::Racing); }
	void OnRestartClicked() { Restart(); }
	void OnMainMenuClicked() { TransitionTo(State::MainMenu); }

	void OnCreate() override
	{
		m_car = FindByTag(GetScene(), "Car");
		if (m_car.IsValid())
		{
			auto& tc = m_car.GetComponent<ignis::TransformComponent>();
			m_car_start_pos = tc.Translation;
			m_car_start_rot = tc.Rotation;
		}

		m_panel_start = FindByTag(GetScene(), "StartMenuPanel");
		m_panel_hud = FindByTag(GetScene(), "HUDPanel");
		m_panel_pause = FindByTag(GetScene(), "PausePanel");
		m_panel_finish = FindByTag(GetScene(), "FinishPanel");

		m_txt_countdown = FindByTag(GetScene(), "CountdownText");
		m_txt_timer = FindByTag(GetScene(), "TimerText");
		m_txt_checkpoint = FindByTag(GetScene(), "CheckpointText");
		m_gauge_speed = FindByTag(GetScene(), "SpeedGauge");
		m_txt_speed = FindByTag(GetScene(), "SpeedText");
		m_txt_final_time = FindByTag(GetScene(), "FinalTimeText");

		if (m_gauge_speed.IsValid() &&
			m_gauge_speed.HasComponent<ignis::ProgressBarComponent>())
		{
			auto& bar = m_gauge_speed.GetComponent<ignis::ProgressBarComponent>();
			bar.MinValue = 0.0f;
			bar.MaxValue = CarMaxSpeed * SpeedScale;
		}

		TransitionTo(State::MainMenu);
	}

	void OnUpdate(float dt) override
	{
		switch (m_state)
		{
		case State::MainMenu:
			break;
		case State::Countdown:
			TickCountdown(dt);
			break;
		case State::Racing:
			m_race_time += dt;
			UpdateHUD();
			CheckPauseInput();
			break;
		case State::Paused:
			ZeroCarVelocity();
			CheckPauseInput();
			break;
		case State::Finished:
			break;
		}
	}

private:
	enum class State { MainMenu, Countdown, Racing, Paused, Finished };
	State m_state = State::MainMenu;

	float     m_countdown = 0.0f;
	float     m_race_time = 0.0f;
	int       m_next_cp = 0;
	bool      m_escape_prev = false;

	glm::vec3 m_car_start_pos = glm::vec3(0.0f);
	glm::vec3 m_car_start_rot = glm::vec3(0.0f);

	ignis::Entity m_car;
	ignis::Entity m_panel_start;
	ignis::Entity m_panel_hud;
	ignis::Entity m_panel_pause;
	ignis::Entity m_panel_finish;
	ignis::Entity m_txt_countdown;
	ignis::Entity m_txt_timer;
	ignis::Entity m_txt_checkpoint;
	ignis::Entity m_gauge_speed;
	ignis::Entity m_txt_speed;
	ignis::Entity m_txt_final_time;

	State     m_prev_state = State::MainMenu;
	glm::vec3 m_saved_linear_vel = glm::vec3(0.0f);
	glm::vec3 m_saved_angular_vel = glm::vec3(0.0f);

	ignis::PhysicsBody* CarBody()
	{
		if (!m_car.IsValid()) return nullptr;
		if (!m_car.HasComponent<ignis::RigidBodyComponent>()) return nullptr;
		return m_car.GetComponent<ignis::RigidBodyComponent>().RuntimeBody.get();
	}

	void ZeroCarVelocity()
	{
		if (auto* body = CarBody())
		{
			body->SetLinearVelocity(glm::vec3(0.0f));
			body->SetAngularVelocity(glm::vec3(0.0f));
		}
	}

	void HideAllPanels()
	{
		SetPanelVisible(m_panel_start, false);
		SetPanelVisible(m_panel_hud, false);
		SetPanelVisible(m_panel_pause, false);
		SetPanelVisible(m_panel_finish, false);
	}

	void ResetCarTransform()
	{
		if (auto* body = CarBody())
		{
			body->SetLinearVelocity(glm::vec3(0.0f));
			body->SetAngularVelocity(glm::vec3(0.0f));
			body->SetPosition(m_car_start_pos);
			body->SetRotation(glm::quat(glm::radians(m_car_start_rot)));
		}
		if (m_car.IsValid())
		{
			auto& tc = m_car.GetComponent<ignis::TransformComponent>();
			tc.Translation = m_car_start_pos;
			tc.Rotation = m_car_start_rot;
		}
	}

	void TransitionTo(State next)
	{
		m_prev_state = m_state;
		RaceShared::CanDrive = false;
		m_state = next;
		HideAllPanels();

		switch (next)
		{
		case State::MainMenu:
			SetPanelVisible(m_panel_start, true);
			break;

		case State::Countdown:
			ResetCarTransform();
			SetPanelVisible(m_panel_hud, true);
			m_countdown = CountdownDuration;
			m_race_time = 0.0f;
			m_next_cp = 0;
			SetText(m_txt_countdown, "3");
			SetText(m_txt_timer, "00:00.00");
			SetText(m_txt_checkpoint, "CP  0 / 4");
			break;

		case State::Racing:
			SetPanelVisible(m_panel_hud, true);
			SetText(m_txt_countdown, "");
			RaceShared::CanDrive = true;
			if (m_prev_state == State::Paused)
			{
				if (auto* body = CarBody())
				{
					body->SetLinearVelocity(m_saved_linear_vel);
					body->SetAngularVelocity(m_saved_angular_vel);
				}
			}
			break;

		case State::Paused:
			if (auto* body = CarBody())
			{
				m_saved_linear_vel = body->GetLinearVelocity();
				m_saved_angular_vel = body->GetAngularVelocity();
			}
			SetPanelVisible(m_panel_hud, true);
			SetPanelVisible(m_panel_pause, true);
			ZeroCarVelocity();
			break;

		case State::Finished:
			SetPanelVisible(m_panel_hud, true);
			SetPanelVisible(m_panel_finish, true);
			SetText(m_txt_final_time, FormatTime(m_race_time));
			break;
		}
	}

	void TickCountdown(float dt)
	{
		m_countdown -= dt;

		if (m_countdown > 2.0f)  SetText(m_txt_countdown, "3");
		else if (m_countdown > 1.0f)  SetText(m_txt_countdown, "2");
		else if (m_countdown > 0.0f)  SetText(m_txt_countdown, "1");
		else if (m_countdown > -0.6f) SetText(m_txt_countdown, "GO!");
		else                          TransitionTo(State::Racing);
	}

	void UpdateHUD()
	{
		SetText(m_txt_timer, FormatTime(m_race_time));

		float displaySpeed = 0.0f;
		if (auto* body = CarBody())
			displaySpeed = glm::length(body->GetLinearVelocity()) * SpeedScale;

		if (m_gauge_speed.IsValid() &&
			m_gauge_speed.HasComponent<ignis::ProgressBarComponent>())
		{
			m_gauge_speed.GetComponent<ignis::ProgressBarComponent>().Value = displaySpeed;
		}

		if (m_txt_speed.IsValid())
		{
			char buf[32];
			std::snprintf(buf, sizeof(buf), "%.0f km/h", displaySpeed);
			SetText(m_txt_speed, buf);
		}
	}

	void CheckPauseInput()
	{
		bool escNow = ignis::Input::IsKeyPressed(ignis::KeyCode::Escape);
		if (escNow && !m_escape_prev)
		{
			if (m_state == State::Racing) TransitionTo(State::Paused);
			else if (m_state == State::Paused) TransitionTo(State::Racing);
		}
		m_escape_prev = escNow;
	}

	void Restart()
	{
		ResetCarTransform();
		TransitionTo(State::Countdown);
	}
};

class CheckpointTrigger : public ignis::ScriptBehaviour
{
public:
	void OnCreate() override
	{
		const std::string& tag = GetEntity()
			.GetComponent<ignis::TagComponent>().Tag;

		m_index = TagToCheckpointIndex(tag);
	}

	void OnTriggerEnter(ignis::Entity other) override
	{
		if (m_index == -1) return;
		if (!other.IsValid()) return;
		if (!other.HasComponent<ignis::TagComponent>()) return;
		if (other.GetComponent<ignis::TagComponent>().Tag != "Car") return;

		ignis::Entity gmEntity = FindByTag(GetScene(), "GameManager");
		if (!gmEntity.IsValid()) return;

		auto* script = GetScene()->GetRuntimeScript(gmEntity.GetID());
		auto* gmScript = dynamic_cast<GameManager*>(script);
		if (gmScript)
			gmScript->NotifyCheckpoint(m_index);
	}

private:
	int m_index = -1;
};

#define FIND_GM()                                                              \
    auto _gm = FindByTag(GetScene(), "GameManager");                           \
    if (!_gm.IsValid()) return;                                                \
    auto* _s = dynamic_cast<GameManager*>(                                     \
        GetScene()->GetRuntimeScript(_gm.GetID()));                            \
    if (!_s) return

class StartButtonHandler : public ignis::ScriptBehaviour
{
public:
	void OnPointerClick(int btn) override
	{
		if (btn != 0) return;
		FIND_GM();
		_s->OnStartClicked();
	}
};

class ResumeButtonHandler : public ignis::ScriptBehaviour
{
public:
	void OnPointerClick(int btn) override
	{
		if (btn != 0) return;
		FIND_GM();
		_s->OnResumeClicked();
	}
};

class RestartButtonHandler : public ignis::ScriptBehaviour
{
public:
	void OnPointerClick(int btn) override
	{
		if (btn != 0) return;
		FIND_GM();
		_s->OnRestartClicked();
	}
};

class MainMenuButtonHandler : public ignis::ScriptBehaviour
{
public:
	void OnPointerClick(int btn) override
	{
		if (btn != 0) return;
		FIND_GM();
		_s->OnMainMenuClicked();
	}
};

#undef FIND_GM

IGNIS_SCRIPT(GameManager)
IGNIS_SCRIPT(CheckpointTrigger)
IGNIS_SCRIPT(StartButtonHandler)
IGNIS_SCRIPT(ResumeButtonHandler)
IGNIS_SCRIPT(RestartButtonHandler)
IGNIS_SCRIPT(MainMenuButtonHandler)