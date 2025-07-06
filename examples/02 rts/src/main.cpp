#include <devkit/common.h>
#include <devkit/io.h>
#include <devkit/gfx.h>
#include <devkit/algo.h>

class RTSExample {
public:
	void setup()
	{
		while (!std::filesystem::exists(std::filesystem::absolute(m_dataPath).concat("/.assets")))
		{
			// Get data path
			auto filters = dk::io::FileDialog::makeFilters(dk::io::FileDialog::filter_t{"Assets Location", "assets"});
			auto opt_dataPath = dk::io::FileDialog::openFile("", filters);
			if (!opt_dataPath.has_value())
				exit(0);
			m_dataPath = opt_dataPath.value().parent_path();
		}

		// Setup asset types
		m_assetManager.type<dk::gfx::ShaderSource>("glsl", dk::gfx::ShaderSource::load, &dk::gfx::ShaderSource::update);

		// Setup window
		m_window.properties(
			dk::io::properties::window::title("RTS Example"), 
            dk::io::properties::window::theme::dark, 
			dk::io::properties::window::mouse_grab::enabled,
			dk::io::properties::window::size(1280, 720));
		m_window.open();

		// Setup user inputs
		m_inputManager.define("tilt" , dk::io::modkey::alt);
		m_inputManager.define("toggle_fullscreen" , dk::io::key::f);

		// Create shaders
		m_assetManager.loadFrom(m_dataPath.string());
		m_shaders["rgba"] = std::make_unique<dk::gfx::Shader>(
			m_assetManager.getShared<dk::gfx::ShaderSource>(m_dataPath.string() + "\\shaders\\rgba_vs.glsl"), 
			m_assetManager.getShared<dk::gfx::ShaderSource>(m_dataPath.string() + "\\shaders\\rgba_fs.glsl"));

		// Setup camera
		m_ucCamera.bind("u_camera.VP",        [&]() -> glm::mat4 { return m_camera.P() * m_camera.V(); });
		m_ucCamera.bind("u_camera.position",  [&]() -> glm::vec3 { return m_camera.position; });
		m_ucCamera.bind("u_camera.direction", [&]() -> glm::vec3 { return m_camera.lookat - m_camera.position; });
	}

	void run()
	{
		while (m_window.beginFrame()) {
			// Clear backbuffer
			dk::gfx::backBuffer().clear(dk::gfx::FrameBuffer::ClearMask::Color | dk::gfx::FrameBuffer::ClearMask::Depth, dk::colors::black);

			moveCamera();

			m_shaders["rgba"]->uniforms() << m_ucCamera;
			auto drawer = dk::gfx::drawer2d(dk::geom::plane::Y(), dk::geom::axis::X);
			m_dbgVertexSink << dk::gfx::draw(dk::geom::ray3({}, dk::geom::axis::X), dk::colors::red)
			                << dk::gfx::draw(dk::geom::ray3({}, dk::geom::axis::Y), dk::colors::lime)
			                << dk::gfx::draw(dk::geom::ray3({}, dk::geom::axis::Z), dk::colors::blue);
			m_dbgVertexSink << drawer(dk::geom::circle2{ glm::dvec2(0, 0), 2.0 }, dk::colors::dodgerBlue);

			auto cursorProjection = [&] { 
				auto v3 = dk::geom::intersection(m_camera.castRay(m_window.cursorN()), drawer.plane);
				return -glm::dvec2(v3.x, v3.z);
			}();
			m_dbgVertexSink << drawer(dk::geom::edge2{cursorProjection, glm::dvec2(0,0)}, dk::colors::orangeRed)
					        << drawer(dk::geom::circle2{ cursorProjection, 1.0 }, dk::colors::maroon);

			//m_dbgVertexSink << dk::gfx::draw(
			//	dk::geom::circle2{ glm::dvec2(0, 0), 2.0 }, 20, dk::colors::dodgerBlue, dk::geom::plane::Y() + 0.2, dk::geom::axis::X);

			m_dbgVertexSink.flush(*m_shaders["rgba"], dk::gfx::backBuffer());

			// Toggle fullscreen with the f key
			if (m_inputManager.activated("toggle_fullscreen"))
				m_window.property(dk::common::toggle(m_window.property<dk::io::properties::window::mode>()));
			// Close window with the esc key
			if (dk::io::key::esc) m_window.close();

			m_window.endFrame();
		}
	}

	RTSExample()
		: m_dbgVertexSink(dk::gfx::VertexSink::create<dk::gfx::RGBAVertex>())
	{ }

private:
	dk::io::Window       m_window;
	dk::io::AssetManager m_assetManager;
	dk::io::InputManager m_inputManager;

	std::filesystem::path m_dataPath = "../../../examples/_common_data";

	using shaders_t = std::unordered_map<std::string, std::unique_ptr<dk::gfx::Shader>>;

	dk::gfx::VertexSink        m_dbgVertexSink;
	dk::gfx::Camera            m_camera;
	dk::gfx::UniformCollection m_ucCamera;
	shaders_t                  m_shaders;

	void moveCamera() 
	{
		// Tilt
		if (m_inputManager.active("tilt") && !dk::io::button::middle)
			dk::gfx::Camera::Orbit::tilt(m_camera, m_window.cursorDeltaP() * glm::vec2(.004, .004));

		// Shift
		const double shiftRate = 2.;
		// When the cursor is at the edge of the screen
		glm::dvec2 edgeDirection(0, 0);
		if (m_window.cursorP().x == 0) edgeDirection.x =  1.;
		if (m_window.cursorP().y == 0) edgeDirection.y = -1.;
		if (m_window.cursorP().x == m_window.property<dk::io::properties::window::size>().x - 1) edgeDirection.x = -1.;
		if (m_window.cursorP().y == m_window.property<dk::io::properties::window::size>().y - 1) edgeDirection.y =  1.;
		const glm::dvec3 right   = glm::normalize(glm::cross(dk::geom::axis::Y, glm::dvec3(m_camera.lookat - m_camera.position)));
		const glm::dvec3 forward = glm::normalize(glm::cross(dk::geom::axis::Y, right));
		const glm::dvec3 direction = right * (double)edgeDirection.x + forward * (double)edgeDirection.y;
		glm::dvec3 shift = direction * m_window.dtSeconds() * shiftRate * (double)glm::length(m_camera.lookat - m_camera.position);
		// With the middle mouse button
		if (dk::io::button::middle) {
			auto cursorProjection = dk::geom::intersection(m_camera.castRay(m_window.cursorN()), dk::geom::plane::Y());
			auto prevCursorProjection = dk::geom::intersection(m_camera.castRay(m_window.cursorN() - m_window.cursorDeltaN()), dk::geom::plane::Y());
			shift = prevCursorProjection - cursorProjection;
		}
		// Apply shift
		dk::gfx::Camera::Orbit::shift(m_camera, shift);

		// Zoom
		if (dk::io::wheel::up && !dk::io::button::middle)
			dk::gfx::Camera::Orbit::zoom(m_camera, 0.9);
		if (dk::io::wheel::down && !dk::io::button::middle)
			dk::gfx::Camera::Orbit::zoom(m_camera, 1.1);

		// Set camera aspect ratio
		m_camera.asp = dk::gfx::backBuffer().aspectRatio();
	}
};

int main(void) {
	RTSExample rts;
	rts.setup();
	rts.run();

	return 0;
}
