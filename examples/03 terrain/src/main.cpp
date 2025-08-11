#include <devkit/io/window.h>
#include <devkit/io/input_combination.h>
#include <devkit/gfx/scene.h>
#include <devkit/io/frame.h>

#include <mini/ini.h>

#include <imgui.h>

#include "terrain.h"
#include "terrain_editor.h"

// TODO: 
//class View {
//public:
//	void update(auto& cameraController)
//	{
//		cameraController(m_camera);
//		m_uc.set("u_camera.VP"       , m_camera.P() * m_camera.V());
//		m_uc.set("u_camera.position" , m_camera.position);
//		m_uc.set("u_camera.direction", m_camera.lookat - m_camera.position);
//		m_uc.set("u_viewport.size"   , m_viewport.size());
//		m_uc.set("u_viewport.cursor" , m_viewport.cursorN());
//	}
//
//	glm::dvec3 cursor(const dk::geom::plane& plane)
//	{
//		return dk::geom::intersection(m_camera.castRay(m_viewport.cursorN()), plane);
//	}
//
//private:
//	dk::gfx::Viewport*         m_viewport;
//	dk::gfx::Camera            m_camera;
//	dk::gfx::UniformCollection m_uc;
//};

class UIView {
public:
	void update(auto& cameraController)
	{
		cameraController(m_camera);

		m_uc.set("u_camera.VP"       , m_camera.P() * m_camera.V());
		m_uc.set("u_camera.position" , m_camera.position);
		m_uc.set("u_camera.direction", m_camera.lookat - m_camera.position);

		m_uc.set("u_viewport.size"   , (glm::vec2)m_window.property<dk::io::properties::window::size>());
		m_uc.set("u_viewport.cursor" , m_window.cursorN());
	}

	glm::dvec3 cursor(const dk::geom::plane& plane) const
	{
		return dk::geom::intersection(m_camera.castRay(m_window.cursorN()), plane);
	}

private:
	dk::io::Window&            m_window;
	dk::gfx::Camera            m_camera;
	dk::gfx::UniformCollection m_uc;
};

class RtsCamera {
public:
	RtsCamera()
	{
		m_camera.position = glm::vec3(-2, 5, -2);
	}

	void update(dk::io::Window& window, dk::io::InputManager& inputs) 
	{
		// Tilt
		if (inputs.active("tilt") && !dk::io::button::middle)
			dk::gfx::Camera::Orbit::tilt(m_camera, window.cursorDeltaP() * glm::vec2(.004, .004));
	
		// Shift
		const double shiftRate = 2.;
		static int   disableShift = 0;
		// When the cursor is at the edge of the screen
		glm::dvec2 edgeDirection(0, 0);
		if (window.cursorP().x == 0) edgeDirection.x =  1.;
		if (window.cursorP().y == 0) edgeDirection.y = -1.;
		if (window.cursorP().x == window.property<dk::io::properties::window::size>().x - 1) edgeDirection.x = -1.;
		if (window.cursorP().y == window.property<dk::io::properties::window::size>().y - 1) edgeDirection.y =  1.;
		const glm::dvec3 right   = glm::normalize(glm::cross(dk::geom::axis::Y, glm::dvec3(m_camera.lookat - m_camera.position)));
		const glm::dvec3 forward = glm::normalize(glm::cross(dk::geom::axis::Y, right));
		const glm::dvec3 direction = right * (double)edgeDirection.x + forward * (double)edgeDirection.y;
		glm::dvec3 shift = direction * window.dtSeconds() * shiftRate * (double)glm::length(m_camera.lookat - m_camera.position);
		// With the middle mouse button
		if (dk::io::button::middle) {
			auto cursorProjection = dk::geom::intersection(m_camera.castRay(window.cursorN()), dk::geom::plane::Y());
			auto prevCursorProjection = dk::geom::intersection(m_camera.castRay(window.cursorN() - window.cursorDeltaN()), dk::geom::plane::Y());
			shift = prevCursorProjection - cursorProjection;
			// Wrap cursor
			disableShift = window.wrapOutOfBoundsCursor(true) ? 3 : disableShift;
		}
		// Apply shift
		if (disableShift = std::max(0, disableShift - 1); !disableShift)
			dk::gfx::Camera::Orbit::shift(m_camera, shift);
	
		// Zoom
		if (dk::io::wheel::up && !dk::io::button::middle)
			dk::gfx::Camera::Orbit::zoom(m_camera, 0.9);
		if (dk::io::wheel::down && !dk::io::button::middle)
			dk::gfx::Camera::Orbit::zoom(m_camera, 1.1);
	
		// Set camera aspect ratio
		m_camera.asp = dk::gfx::backBuffer().aspectRatio();
	}

	const auto& camera() const
	{ return m_camera; }

private:
	dk::gfx::Camera            m_camera;
	dk::gfx::UniformCollection m_uc;
};

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
			dk::io::properties::window::mouse_grab::enabled,
			dk::io::properties::window::size(1280, 720));

		// Setup inputs
		m_inputs.define("tilt", dk::io::modkey::alt);
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
			m_camera.update(m_window, m_inputs);
			
			// Sync assets
			m_assets.synchronize();

			m_terrainEditor->update(m_terrain, m_camera.camera(), m_window);
			m_terrain.navmesh().build(m_terrain);
			m_terrainEditor->render(m_terrain, dk::gfx::backBuffer(), m_assets);

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
	RtsCamera                      m_camera;

	Terrain                        m_terrain;
	std::unique_ptr<TerrainEditor> m_terrainEditor;
};

int main(void) {
	Application application;
	application.run();

	return 0;
}
