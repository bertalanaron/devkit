#pragma once
#include "game_state.h"
#include "game_client.h"
#include "ui_utils.h"

class EditorClient 
	: public ClientBase
{
private:
	template <typename T>
	using uptr_t = std::unique_ptr<T>;

public:
	enum class Execution {
		Editing, Testing
	};

	class EditStrategyBase {
	public:
		EditStrategyBase(EditorClient* editor)
			: m_editor(editor)
		{ }

		virtual void setup() { }
		virtual void update(const dk::io::Frame& frame) { }
		virtual bool showToolbarIcon() = 0;
		virtual void showOptionsPanel() { }

	protected:
		EditorClient& editor()
		{ return *m_editor; }
	private:
		EditorClient* m_editor;
	};

	class TerrainEditStrategy;
	class ObjectsEditStrategy;

public:
	using ClientBase::ClientBase;

	void update(const dk::io::Frame& frame) override
	{
		// Handle close
		if (m_inputs.activated("close"))
			stop();

		m_assets.synchronize();

		// Disable ui when testing
		ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.2);
		ImGui::BeginDisabled(m_execution == Execution::Testing);

		// Execution in editing or testing state
		showUI(frame);
		if (m_execution == Execution::Editing)
			updateWhileEditing(frame);
		if (m_execution == Execution::Testing)
			updateWhileTesting(frame);

		ImGui::EndDisabled();
		ImGui::PopStyleVar();

		// Handle game instance creation and update
		if (m_inputs.activated("run"))
		{
			m_gameClient.emplace(ini(), m_state->clone());
			m_execution = Execution::Testing;
		}
		if (m_gameClient)
		{
			if (m_gameClient->stopRequested())
			{
				m_gameClient.reset();
				m_execution = Execution::Editing;
			}
			else
				m_gameClient->step();
			frame.makeCurrent();
		}
	}

private:
	std::optional<GameClient> m_gameClient;
	Execution                 m_execution = Execution::Editing;

	int                                   m_selectedEditStrategyIndex = 0;
	std::vector<uptr_t<EditStrategyBase>> m_editStrategies;

	void updateWhileEditing(const dk::io::Frame& frame);

	void updateWhileTesting(const dk::io::Frame& frame);

	void showUI(const dk::io::Frame& frame);

	void setup() override;
};
