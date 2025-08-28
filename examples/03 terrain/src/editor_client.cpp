#include "editor_client.h"

#include "terrain_editor.h"
#include "objects_editor.h"

void EditorClient::setup()
{
	setupAssetManager(m_assets, ini());

	// Setup hotkeys
	m_inputs.define("close", dk::io::key::esc);
	m_inputs.define("run"  , dk::io::key::f5);

	// Setup window
	m_window.property(dk::io::properties::window::theme::dark);
	m_window.property(dk::io::properties::window::size(1280, 720));
	m_window.property(dk::io::properties::window::default_dockspace::disabled);

	// Create edit strategies
	m_editStrategies.emplace_back(std::make_unique<TerrainEditStrategy>(this));
	m_editStrategies.emplace_back(std::make_unique<ObjectsEditStrategy>(this));
	for (auto& strategy : m_editStrategies)
		strategy->setup();
}

void EditorClient::showUI(const dk::io::Frame& frame)
{
	ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;
	ImGui::GetStyle().TabRounding = 0.f;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, 0xff1f1f1f);

	// Draw menu bar
	if (auto menuBar = ImGuiCustomMainMenuBar("MainMenuBar"); menuBar)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
		ImGui::PushStyleColor(ImGuiCol_Button        , 0x00);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered , 0xff4f4f4f);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive  , 0x00);

		ImGui::Button("File", ImVec2(48, 26)); ImGui::SameLine();
		ImGui::Button("Edit", ImVec2(48, 26)); ImGui::SameLine();
		ImGui::Button("View", ImVec2(48, 26)); ImGui::SameLine();

		// Draw game client controller floater
		const float gameClientControllFloaterWidth = 
			m_execution == Execution::Editing ? 22.f : 74.f;
		ImGui::SetCursorPosX(ImGui::GetMainViewport()->Size.x / 2.f - gameClientControllFloaterWidth / 2.f);
		if (m_execution == Execution::Editing)
		{
			// Draw run button
			ImGui::ImageButton("##run_game_client_button", 
				m_assets.get<dk::gfx::Texture>("/textures/ui/editor/run_btn.png").imguiTextureId(), 
				ImVec2(22, 22)); ImGui::SameLine();
		}
		else
		{
			// Draw stop button
			ImGui::ImageButton("##stop_game_client_button", 
				m_assets.get<dk::gfx::Texture>("/textures/ui/editor/stop_btn.png").imguiTextureId(), 
				ImVec2(22, 22)); ImGui::SameLine();
			// Draw restart button
			ImGui::ImageButton("##restart_game_client_button", 
				m_assets.get<dk::gfx::Texture>("/textures/ui/editor/restart_btn.png").imguiTextureId(), 
				ImVec2(22, 22)); ImGui::SameLine();
			// Draw pause button
			ImGui::ImageButton("##pause_game_client_button", 
				m_assets.get<dk::gfx::Texture>("/textures/ui/editor/pause_btn.png").imguiTextureId(), 
				ImVec2(22, 22)); ImGui::SameLine();
		}

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();
	}

	ImGui::DockSpaceOverViewport(m_window.imguiDockspaceID(), (const ImGuiViewport*)0,
		ImGuiDockNodeFlags_PassthruCentralNode);

	if (ImBeginClearInWindow("editor_strategy_options_window", 
		ImVec2(frame.viewport().offset().x, frame.viewport().offsetFromTop())))
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
		ImGuiMultiChoiceButtons multiChoice(m_selectedEditStrategyIndex, "editor_strategy_options");
		for (const auto& strategy : m_editStrategies)
			multiChoice.item([&] { return strategy->showToolbarIcon(); }, m_editStrategies.size());
		ImGui::PopStyleVar();
	}
	ImGui::End();

	// Show options panel of selected strategy
	if (ImGui::Begin("Editor Options"))
		m_editStrategies.at(m_selectedEditStrategyIndex)->showOptionsPanel();
	ImGui::End();

	ImGui::ShowDemoWindow();

	ImGui::PopStyleColor();
}

void EditorClient::updateWhileEditing(const dk::io::Frame& frame)
{
}

void EditorClient::updateWhileTesting(const dk::io::Frame& frame)
{
}
