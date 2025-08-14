#include <devkit/io/window.h>
#include <devkit/io/input_combination.h>
#include <devkit/gfx/scene.h>
#include <devkit/io/frame.h>
#include <devkit/io/file_dialog.h>

#include <mini/ini.h>

#include <imgui.h>

#include "view.h"
#include "rts_camera_controller.h"
#include "terrain.h"
#include "terrain_editor.h"


// A simple "segmented control" made of buttons. One is always selected.
bool SegmentedControl(const char* id, int* current, const std::vector<const char*>& labels)
{
	bool changed = false;
	ImGui::BeginGroup();
	ImGui::PushID(id);

	// Style setup: selected vs unselected states
	ImGuiStyle& style = ImGui::GetStyle();
	const ImVec4 colSelected      = ImGui::GetColorU32(ImGuiCol_ButtonActive) ? style.Colors[ImGuiCol_ButtonActive] : ImVec4(0.26f,0.59f,0.98f,1.0f);
	const ImVec4 colSelectedHover = style.Colors[ImGuiCol_ButtonHovered];
	const ImVec4 colUnsel         = style.Colors[ImGuiCol_Button];
	const ImVec4 colUnselHover    = style.Colors[ImGuiCol_ButtonHovered];

	for (int i = 0; i < (int)labels.size(); ++i)
	{
		ImGui::PushID(i);

		const bool isSelected = (i == *current);
		ImGui::PushStyleColor(ImGuiCol_Button,        isSelected ? colSelected      : colUnsel);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, isSelected ? colSelectedHover : colUnselHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  isSelected ? colSelected      : colUnsel);

		// Optional: tweak rounding to make it feel like a segmented control
		float r = style.FrameRounding;
		float left  = (i == 0) ? r : 0.0f;
		float right = (i == (int)labels.size() - 1) ? r : 0.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f); // keep button rect; we'll fake segment edges
		// Draw the button
		if (ImGui::Button(labels[i]))
		{
			if (!isSelected) { *current = i; changed = true; } // never deselect; always switch
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);

		if (i + 1 < (int)labels.size())
			ImGui::SameLine();

		ImGui::PopID();
	}

	ImGui::PopID();
	ImGui::EndGroup();
	return changed; // true if selection changed this frame
}


class GameState {
public:
	//Terrain terrain;

	GameState() = default;

	std::unique_ptr<GameState> clone()
	{
		return std::make_unique<GameState>(*this);
	}

	static GameState load(const std::filesystem::path& path);
	void save(const std::filesystem::path& path);
};

class ClientBase {
public:
	ClientBase(
		mINI::INIStructure			 ini,
		std::unique_ptr<GameState>&& state = std::make_unique<GameState>())
		: m_state(std::move(state))
		, m_ini(ini)
	{ }

	virtual void update(const dk::io::Frame& frame) { };
	virtual void render(const dk::io::Frame& frame) 
	{ 
		dk::gfx::backBuffer().clear(dk::gfx::FrameBuffer::ClearMask::Color, dk::colors::gray);
	};

	void stop()
	{ m_stopRequested = true; }

	bool stopRequested() const
	{
		return m_stopRequested || (m_initialized && !m_window.isOpen());
	}

	void run()
	{
		while (!stopRequested())
		{
			step();
		}
	}

	void step()
	{
		if (!m_initialized)
		{
			setup();
			m_window.open(std::stoi(ini_or("graphics", "msaa", "1")));
			m_initialized = true;
		}
		const auto& frame = m_window.beginFrame();
		update(frame);
		render(frame);
		m_window.endFrame();
	}

	virtual ~ClientBase()
	{
		if (m_window.isOpen())
			m_window.close();
		teardown();
	}

protected:
	dk::io::AssetManager       m_assets;
	dk::io::InputManager       m_inputs;
	View                       m_view;
	std::unique_ptr<GameState> m_state;
	dk::io::Window             m_window;
	
	virtual void setup()    { };
	virtual void teardown() { };

	auto& ini()
	{ return m_ini; }

	std::optional<std::string> ini(const std::string& label, const std::string& value)
	{
		if (!m_ini.has(label) || !m_ini[label].has(value))
			return std::nullopt;
		return m_ini[label][value];
	}

	std::string ini_or(
		const std::string& label, 
		const std::string& value, 
		const std::string& fallback)
	{
		const auto result = ini(label, value);
		if (result.has_value())
			return result.value();
		return fallback;
	}

private:
	bool m_stopRequested = false;
	bool m_initialized = false;

	mINI::INIStructure m_ini;
};

//
//class ClientManager
//	: private dk::common::SingletonBase<ClientManager>
//{
//public:
//	static void addClient(std::unique_ptr<ClientBase>&& client)
//	{
//		instance().m_clients.emplace(client.get(), std::move(client));
//	}
//
//	static void removeClient(const ClientBase* clientPtr)
//	{
//		instance().m_clients.erase(clientPtr);
//	}
//
//private:
//	std::unordered_map<const ClientBase*, std::unique_ptr<ClientBase>> m_clients;
//};
//


void setupAssetManager(dk::io::AssetManager& assets, mINI::INIStructure& ini)
{
	assets.root(ini["data"]["path"]);

	assets.type<dk::gfx::Texture>("png", 
		dk::gfx::Texture::load, std::nullopt, std::nullopt, dk::io::AssetManager::Async);
	assets.watch("/textures", false);
	assets.watch("/textures/terrain", false);
}

class GameClient 
	: public ClientBase
{
public:
	using ClientBase::ClientBase;

	void update(const dk::io::Frame& frame) override
	{
		if (m_inputs.activated("close"))
			stop();

		m_assets.synchronize();

		if (ImGui::Begin("Hi")) {
			for (auto [path, texture] : m_assets.each<dk::gfx::Texture>()) {
				auto imId = texture.imguiTextureId();
				ImGui::Image(imId, ImVec2(32, 32));
			} 
		} ImGui::End();
	}

	void render(const dk::io::Frame& frame) override
	{
		dk::gfx::backBuffer().clear(dk::gfx::FrameBuffer::ClearMask::Color, dk::colors::black);
	}

private:
	void setup() override
	{
		setupAssetManager(m_assets, ini());
		m_inputs.define("close", dk::io::key::x);
	}
};

class EditorClient 
	: public ClientBase
{
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

public:
	using ClientBase::ClientBase;

	void update(const dk::io::Frame& frame) override
	{
		// Handle close
		if (m_inputs.activated("close"))
			stop();

		ImGui::BeginDisabled(m_execution == Execution::Testing);

		// Execution in editing or testing state
		if (m_execution == Execution::Editing)
			updateWhileEditing(frame);
		if (m_execution == Execution::Testing)
			updateWhileEditing(frame);
			//updateWhileTesting(frame);

		ImGui::EndDisabled();

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

	std::vector<std::unique_ptr<EditStrategyBase>> m_editStrategies;

	void updateWhileEditing(const dk::io::Frame& frame);

	void updateWhileTesting(const dk::io::Frame& frame);

	void setup() override;
};

class EditorClient::TerrainEditStrategy
	: public EditorClient::EditStrategyBase
{
public:
	using EditStrategyBase::EditStrategyBase;

	void setup() override
	{
		editor().m_inputs.define("select_edit_strategy_terrain", 
			dk::io::modkey::alt + dk::io::key::_1);
	}
	bool showToolbarIcon() override
	{
		bool result = ImGui::Button("terrain");
		if (ImGui::IsItemHovered())
		{
			//const auto ic = editor().m_inputs.inputCombinationOf("select_edit_strategy_terrain");
			ImGui::SetTooltip("alt + 1");
		}
		return result;
	}
	void showOptionsPanel() override
	{
		ImGui::Text("Edit terrain");
	}
};

void EditorClient::setup()
{
	setupAssetManager(m_assets, ini());

	// Setup hotkeys
	m_inputs.define("close", dk::io::key::esc);
	m_inputs.define("run"  , dk::io::key::r);

	// Setup window
	m_window.property(dk::io::properties::window::theme::dark);
	m_window.property(dk::io::properties::window::size(1280, 720));

	// Create edit strategies
	m_editStrategies.emplace_back(std::make_unique<TerrainEditStrategy>(this));
}

void EditorClient::updateWhileEditing(const dk::io::Frame& frame)
{
	static int selected = 0;
	const auto imguiMainWindowPos = ImGui::GetMainViewport()->Pos;
	const auto imguiRelativePos = ImVec2(frame.viewport().offset().x, frame.viewport().offsetFromTop());
	ImGui::SetNextWindowPos(ImVec2(imguiMainWindowPos.x + imguiRelativePos.x, 
		                           imguiMainWindowPos.y + imguiRelativePos.y));
	if (ImGui::Begin("ContentWindow",
		nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize   |
		ImGuiWindowFlags_NoMove     |
		ImGuiWindowFlags_NoScrollbar|
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoSavedSettings))
	{
		SegmentedControl("toolbar_options", &selected, { "terrain", "texture", "decor" });
	}
	ImGui::End();

	ImGui::ShowDemoWindow();

	// Show toolbar
	if (ImGui::Begin("Toolbar"))
	{
		for (auto& strategy : m_editStrategies)
			strategy->showToolbarIcon();
	}
	ImGui::End();
}

void EditorClient::updateWhileTesting(const dk::io::Frame& frame)
{
}




int main()
{
	spdlog::set_level(spdlog::level::trace);

	auto ini = [] {
		mINI::INIFile file(dk::common::executable_path().parent_path() / "examples.ini");
		mINI::INIStructure ini;
		file.read(ini);
		return ini;
	}();

	EditorClient editor(ini);
	editor.run();

	return 0;
}

//class Application {
//public:
//	Application()
//	{
//		// Parse ini file
//		m_ini = [] {
//			mINI::INIFile file(dk::common::executable_path().parent_path() / "examples.ini");
//			mINI::INIStructure ini;
//			file.read(ini);
//			return ini;
//		}();
//
//		// Setup asset manager directories
//		m_assets.root(m_ini["data"]["path"]);
//		m_assets.watch("/shaders"   , true);
//		m_assets.watch("/textures"  , false);
//		m_assets.watch("/textures/terrain", false);
//		m_assets.watch("/fonts"     , true);
//		m_assets.watch("/models/rts", true);
//		// Setup types
//		m_assets.type<dk::gfx::Texture>("png", dk::gfx::Texture::load, std::nullopt, std::nullopt, dk::io::AssetManager::Async);
//		m_assets.type<dk::gfx::ShaderSource>("glsl", dk::gfx::ShaderSource::load, &dk::gfx::ShaderSource::update);
//		m_assets.type<dk::gfx::Scene>("fbx", dk::gfx::Scene::load);
//		// Preload assets
//		m_assets.synchronize();
//
//		auto& diagonal = m_assets.get<dk::gfx::Scene>("/models/rts/cliff_poc.fbx")["diagonal"];
//		for (const auto& mesh : diagonal.meshes())
//			spdlog::info("{}", mesh.get().vertices().size());
//
//		// Setup window
//		m_window.properties(
//			dk::io::properties::window::title("Terrain"), 
//			dk::io::properties::window::theme::dark, 
//			//dk::io::properties::window::mouse_grab::enabled,
//			dk::io::properties::window::size(1280, 720));
//
//		// Setup inputs
//		m_cameraController.inputs.define("tilt", dk::io::modkey::alt);
//		m_cameraController.inputs.define("shift", dk::io::modkey::none + dk::io::button::right);
//		m_inputs.define("quit", dk::io::key::esc);
//		m_inputs.define("toggle_fullscreen", dk::io::key::f);
//	}
//
//	void run()
//	{
//		int msaa = (m_ini.has("graphics") && m_ini["graphics"].has("msaa") 
//			? std::stoi(m_ini["graphics"]["msaa"]) 
//			: 1);
//		m_window.open(msaa);
//		dk::gfx::backBuffer().property(dk::gfx::properties::multisampling::enabled);
//
//		m_terrainEditor = std::make_unique<TerrainEditor>(m_assets);
//		dk::dbg::store<float, "offset_from_vertex">() = .75;
//
//		while (m_window.isOpen()) {
//			const auto& frame = m_window.beginFrame();
//			dk::gfx::backBuffer().clear(dk::gfx::FrameBuffer::ClearMask::Color | dk::gfx::FrameBuffer::ClearMask::Depth, dk::colors::gray);
//			m_mainView.update(frame, m_cameraController);
//			
//			// Sync assets
//			m_assets.synchronize();
//
//			ImGui::ShowDemoWindow();
//
//			if (ImGui::Begin("viewport"))
//			{
//				const auto text = std::format("offset.x={} offset.y={}", frame.viewport().offset().x, frame.viewport().offset().y);
//				ImGui::Text(text.c_str());
//				ImGui::End();
//			}
//
//			m_terrainEditor->update(m_terrain, m_mainView);
//			m_terrain.navmesh().build(m_terrain);
//			m_terrainEditor->render(m_terrain, dk::gfx::backBuffer(), m_assets, m_mainView);
//
//			if (m_inputs.activated("quit"))
//				m_window.close();
//			if (m_inputs.activated("toggle_fullscreen"))
//				m_window.property(dk::common::toggle(m_window.property<dk::io::properties::window::mode>()));
//
//
//			m_window.endFrame();
//		}
//	}
//
//private:
//	mINI::INIStructure             m_ini;
//
//	dk::io::AssetManager           m_assets;
//	dk::io::InputManager           m_inputs;
//	dk::io::Window                 m_window;
//	View                           m_mainView;
//	RTSCameraController            m_cameraController;
//
//	Terrain                        m_terrain;
//	std::unique_ptr<TerrainEditor> m_terrainEditor;
//};

//int main(void) {
//	spdlog::set_level(spdlog::level::trace);
//
//
//
//	Application application;
//	application.run();
//
//	return 0;
//}
