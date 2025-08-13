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

//
//class GameState {
//public:
//	Terrain terrain;
//
//	GameState();
//
//	std::unique_ptr<GameState> clone();
//
//	static GameState load(const std::filesystem::path& path);
//	void save(const std::filesystem::path& path);
//};
//
//
//class ClientBase {
//public:
//	ClientBase(dk::io::AssetManager& assets, std::unique_ptr<GameState>&& state)
//		: m_assets(assets)
//		, m_state(std::move(state))
//	{ 
//		setup();
//	}
//
//	virtual void update(const dk::io::Frame& frame) = 0;
//	virtual void render(const dk::io::Frame& frame) = 0;
//
//	virtual ~ClientBase()
//	{
//		teardown();
//	}
//
//protected:
//	dk::io::AssetManager&      m_assets;
//	dk::io::InputManager       m_inputs;
//	dk::io::Window             m_window;
//	View                       m_view;
//	std::unique_ptr<GameState> m_state;
//
//	virtual void setup()    = 0;
//	virtual void teardown() = 0;
//};
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
//class GameClient 
//	: public ClientBase
//{
//public:
//	using ClientBase::ClientBase;
//
//};
//
//class EditorClient 
//	: public ClientBase
//{
//public:
//	using ClientBase::ClientBase;
//
//	void update(const dk::io::Frame& frame) override
//	{
//		if (m_inputs.activated("close")
//			|| !m_window.isOpen())
//		{
//			ClientManager::removeClient(this);
//			return;
//		}
//
//		if (m_inputs.activated("load"))
//		{
//			auto optPath = openFile();
//			if (optPath)
//			{
//				auto newState    = GameState::load(openFile().value());
//				auto newStatePtr = std::make_unique<GameState>(std::move(newState));
//				m_state.swap(newStatePtr);
//			}
//		}
//
//		if (m_inputs.activated("save"))
//		{
//			auto optPath = openFile();
//			if (optPath)
//			{
//				m_state->save(saveFile().value());
//			}
//		}
//
//		if (m_inputs.activated("run"))
//		{
//			auto gameClient = std::make_unique<GameClient>(m_assets, std::move(m_state->clone()));
//			ClientManager::addClient(std::move(gameClient));
//		}
//	}
//
//private:
//	GameClient createGameClient();
//
//	std::optional<std::filesystem::path> openFile();
//	std::optional<std::filesystem::path> saveFile();
//};


class Application {
public:
	Application()
	{
		// Parse ini file
		m_ini = [] {
			mINI::INIFile file(dk::common::executable_path().parent_path() / "examples.ini");
			mINI::INIStructure ini;
			file.read(ini);
			return ini;
		}();

		// Setup asset manager directories
		m_assets.root(m_ini["data"]["path"]);
		m_assets.watch("/shaders"   , true);
		m_assets.watch("/textures"  , false);
		m_assets.watch("/textures/terrain", false);
		m_assets.watch("/fonts"     , true);
		m_assets.watch("/models/rts", true);
		// Setup types
		m_assets.type<dk::gfx::Texture>("png", dk::gfx::Texture::load, std::nullopt, std::nullopt, dk::io::AssetManager::Async);
		m_assets.type<dk::gfx::ShaderSource>("glsl", dk::gfx::ShaderSource::load, &dk::gfx::ShaderSource::update);
		m_assets.type<dk::gfx::Scene>("fbx", dk::gfx::Scene::load);
		// Preload assets
		m_assets.synchronize();

		auto& diagonal = m_assets.get<dk::gfx::Scene>("/models/rts/cliff_poc.fbx")["diagonal"];
		for (const auto& mesh : diagonal.meshes())
			spdlog::info("{}", mesh.get().vertices().size());

		// Setup window
		m_window.properties(
			dk::io::properties::window::title("Terrain"), 
			dk::io::properties::window::theme::dark, 
			//dk::io::properties::window::mouse_grab::enabled,
			dk::io::properties::window::size(1280, 720));

		// Setup inputs
		m_cameraController.inputs.define("tilt", dk::io::modkey::alt);
		m_cameraController.inputs.define("shift", dk::io::modkey::none + dk::io::button::right);
		m_inputs.define("quit", dk::io::key::esc);
		m_inputs.define("toggle_fullscreen", dk::io::key::f);
	}

	void run()
	{
		int msaa = (m_ini.has("graphics") && m_ini["graphics"].has("msaa") 
			? std::stoi(m_ini["graphics"]["msaa"]) 
			: 1);
		m_window.open(msaa);
		dk::gfx::backBuffer().property(dk::gfx::properties::multisampling::enabled);

		m_terrainEditor = std::make_unique<TerrainEditor>(m_assets);
		dk::dbg::store<float, "offset_from_vertex">() = .75;

		while (m_window.isOpen()) {
			const auto& frame = m_window.beginFrame();
			dk::gfx::backBuffer().clear(dk::gfx::FrameBuffer::ClearMask::Color | dk::gfx::FrameBuffer::ClearMask::Depth, dk::colors::gray);
			m_mainView.update(frame, m_cameraController);
			
			// Sync assets
			m_assets.synchronize();

			ImGui::ShowDemoWindow();

			if (ImGui::Begin("viewport"))
			{
				const auto text = std::format("offset.x={} offset.y={}", frame.viewport().offset().x, frame.viewport().offset().y);
				ImGui::Text(text.c_str());
				ImGui::End();
			}

			m_terrainEditor->update(m_terrain, m_mainView);
			m_terrain.navmesh().build(m_terrain);
			m_terrainEditor->render(m_terrain, dk::gfx::backBuffer(), m_assets, m_mainView);

			if (m_inputs.activated("quit"))
				m_window.close();
			if (m_inputs.activated("toggle_fullscreen"))
				m_window.property(dk::common::toggle(m_window.property<dk::io::properties::window::mode>()));


			m_window.endFrame();
		}
	}

private:
	mINI::INIStructure             m_ini;

	dk::io::AssetManager           m_assets;
	dk::io::InputManager           m_inputs;
	dk::io::Window                 m_window;
	View                           m_mainView;
	RTSCameraController            m_cameraController;

	Terrain                        m_terrain;
	std::unique_ptr<TerrainEditor> m_terrainEditor;
};

int main(void) {
	spdlog::set_level(spdlog::level::trace);

	Application application;
	application.run();

	return 0;
}
