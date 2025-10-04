#include <devkit/common.h>
#include <devkit/io.h>
#include <devkit/gfx.h>
#include <devkit/algo.h>
#include <devkit/io/frame.h>

#include <mini/ini.h>

#include <imgui.h>

//class RTSExample {
//public:
//	void setup()
//	{
//		while (!std::filesystem::exists(std::filesystem::absolute(m_dataPath).concat("/.assets")))
//		{
//			// Get data path
//			auto filters = dk::io::FileDialog::makeFilters(dk::io::FileDialog::filter_t{"Assets Location", "assets"});
//			auto opt_dataPath = dk::io::FileDialog::openFile("", filters);
//			if (!opt_dataPath.has_value())
//				exit(0);
//			m_dataPath = opt_dataPath.value().parent_path();
//		}
//
//		// Setup asset types
//		m_assetManager.type<dk::gfx::ShaderSource>("glsl", dk::gfx::ShaderSource::load, &dk::gfx::ShaderSource::update);
//		m_assetManager.type<dk::gfx::Font>("ttf", dk::gfx::Font::load);
//
//		// Setup window
//		m_window.properties(
//			dk::io::properties::window::title("RTS Example"), 
//            dk::io::properties::window::theme::dark, 
//			dk::io::properties::window::mouse_grab::enabled,
//			dk::io::properties::window::size(1280, 720));
//		m_window.open(4);
//		dk::gfx::backBuffer().property(dk::gfx::properties::multisampling::enabled);
//
//		// Setup user inputs
//		m_inputManager.define("tilt" , dk::io::modkey::alt);
//		m_inputManager.define("toggle_fullscreen" , dk::io::key::f);
//
//		// Create shaders
//		m_assets.(m_dataPath.string());
//		m_shaders["rgba"] = std::make_unique<dk::gfx::Shader>(
//			m_assetManager.getShared<dk::gfx::ShaderSource>(m_dataPath.string() + "\\shaders\\rgba_vs.glsl"), 
//			m_assetManager.getShared<dk::gfx::ShaderSource>(m_dataPath.string() + "\\shaders\\rgba_fs.glsl"));
//		m_shaders["text"] = std::make_unique<dk::gfx::Shader>(
//			m_assetManager.getShared<dk::gfx::ShaderSource>(m_dataPath.string() + "\\shaders\\text_vs.glsl"), 
//			m_assetManager.getShared<dk::gfx::ShaderSource>(m_dataPath.string() + "\\shaders\\text_fs.glsl"));
//		m_shaders["text"]->property(dk::gfx::properties::depth_test::enabled);
//		m_shaders["text"]->property(dk::gfx::properties::blend::enabled);
//		m_shaders["text"]->property(dk::gfx::properties::blend_func_src_factor::src_alpha);
//		m_shaders["text"]->property(dk::gfx::properties::blend_func_dst_factor::one_minus_src_alpha);
//
//		// Setup camera
//		m_ucCamera.bind("u_camera.VP",        [&]() -> glm::mat4 { return m_camera.P() * m_camera.V(); });
//		m_ucCamera.bind("u_camera.position",  [&]() -> glm::vec3 { return m_camera.position; });
//		m_ucCamera.bind("u_camera.direction", [&]() -> glm::vec3 { return m_camera.lookat - m_camera.position; });
//	}
//
//	void run()
//	{
//		while (m_window.beginFrame()) {
//			// Clear backbuffer
//			dk::gfx::backBuffer().clear(dk::gfx::FrameBuffer::ClearMask::Color | dk::gfx::FrameBuffer::ClearMask::Depth, dk::colors::black);
//
//			moveCamera();
//
//			m_shaders["rgba"]->uniforms() << m_ucCamera;
//			auto drawer = dk::gfx::drawer2d(dk::geom::plane::Y(), dk::geom::axis::X);
//			m_dbgVertexSink << dk::gfx::draw(dk::geom::ray3({}, dk::geom::axis::X), dk::colors::red)
//			                << dk::gfx::draw(dk::geom::ray3({}, dk::geom::axis::Y), dk::colors::lime)
//			                << dk::gfx::draw(dk::geom::ray3({}, dk::geom::axis::Z), dk::colors::blue);
//			m_dbgVertexSink << drawer(dk::geom::circle2{ glm::dvec2(0, 0), 2.0 }, dk::colors::dodgerBlue)
//				            << drawer(dk::geom::circle2{ glm::dvec2(3, 4), 2.0 }, dk::colors::dodgerBlue);
//
//			auto cursorProjection = [&] { 
//				auto v3 = dk::geom::intersection(m_camera.castRay(m_window.cursorN()), drawer.plane);
//				return -glm::dvec2(v3.x, v3.z);
//			}();
//			static auto circle = dk::geom::circle2{ glm::vec2(0,0), 0.8};
//			auto edge   = dk::geom::edge2{ glm::dvec2(3, 3), glm::dvec2(5,7) };
//			auto force  = cursorProjection - circle.center;
//			auto opposingForceFactor = glm::max(0., circle.radius - glm::length(circle.center - dk::geom::closestPoint(edge, circle.center)));
//			auto opposingForce = glm::normalize(circle.center - dk::geom::closestPoint(edge, circle.center));
//			circle.center += dk::geom::clampLength(force, 0.0, circle.radius - 0.001) + opposingForce * opposingForceFactor;
//			m_dbgVertexSink << drawer(circle, circle.intersects(edge) ? dk::colors::maroon : dk::colors::fuchsia)
//				            << drawer(edge, dk::colors::white)
//				            << drawer(dk::geom::ray2{ circle.center, opposingForce * opposingForceFactor }, dk::colors::red);
//
//			m_dbgVertexSink.flush(*m_shaders["rgba"], dk::gfx::backBuffer());
//
//			// Draw and render text
//			m_shaders["text"]->uniforms() << m_ucCamera;
//			auto&     font     = m_assetManager.get<dk::gfx::Font>(m_dataPath.string() + "\\fonts\\times.ttf");
//			static int fontSize = 12;
//			if (ImGui::Begin("Font Props")) {
//				ImGui::InputInt("font-size", &fontSize);
//				ImGui::End();
//			}
//			m_shaders["text"]->uniformTexture("u_atlas", font.texture(fontSize));
//			const auto textTransform = dk::gfx::Font::TextTransfrom::billboard(glm::vec3(0, 1.2, 0), glm::vec2(.5, 0), m_camera, dk::geom::axis::Y);
//			m_textSink.push_back(dk::gfx::Primitive::Triangles, 
//				font.atlas(fontSize).get(std::format("{}", circle.intersects(edge)), dk::colors::white, textTransform));
//			m_textSink.flush(*m_shaders["text"], dk::gfx::backBuffer());
//
//			// Toggle fullscreen with the f key
//			if (m_inputManager.activated("toggle_fullscreen"))
//				m_window.property(dk::common::toggle(m_window.property<dk::io::properties::window::mode>()));
//			// Close window with the esc key
//			if (dk::io::key::esc) m_window.close();
//
//			m_window.endFrame(); 
//		}
//	}
//
//	RTSExample()
//		: m_dbgVertexSink(dk::gfx::VertexSink::create<dk::gfx::RGBAVertex>())
//		, m_textSink(dk::gfx::VertexSink::create<dk::gfx::Font::CharVertex>())
//	{ }
//
//private:
//	dk::io::Window       m_window;
//	dk::io::AssetManager m_assetManager;
//	dk::io::InputManager m_inputManager;
//
//	std::filesystem::path m_dataPath = "../../../examples/_common_data";
//
//	using shaders_t = std::unordered_map<std::string, std::unique_ptr<dk::gfx::Shader>>;
//
//	dk::gfx::VertexSink        m_dbgVertexSink;
//	dk::gfx::VertexSink        m_textSink;
//	dk::gfx::Camera            m_camera;
//	dk::gfx::UniformCollection m_ucCamera;
//	shaders_t                  m_shaders;
//
//	void moveCamera() 
//	{
//		// Tilt
//		if (m_inputManager.active("tilt") && !dk::io::button::middle)
//			dk::gfx::Camera::Orbit::tilt(m_camera, m_window.cursorDeltaP() * glm::vec2(.004, .004));
//
//		// Shift
//		const double shiftRate = 2.;
//		static int   disableShift = 0;
//		// When the cursor is at the edge of the screen
//		glm::dvec2 edgeDirection(0, 0);
//		if (m_window.cursorP().x == 0) edgeDirection.x =  1.;
//		if (m_window.cursorP().y == 0) edgeDirection.y = -1.;
//		if (m_window.cursorP().x == m_window.property<dk::io::properties::window::size>().x - 1) edgeDirection.x = -1.;
//		if (m_window.cursorP().y == m_window.property<dk::io::properties::window::size>().y - 1) edgeDirection.y =  1.;
//		const glm::dvec3 right   = glm::normalize(glm::cross(dk::geom::axis::Y, glm::dvec3(m_camera.lookat - m_camera.position)));
//		const glm::dvec3 forward = glm::normalize(glm::cross(dk::geom::axis::Y, right));
//		const glm::dvec3 direction = right * (double)edgeDirection.x + forward * (double)edgeDirection.y;
//		glm::dvec3 shift = direction * m_window.dtSeconds() * shiftRate * (double)glm::length(m_camera.lookat - m_camera.position);
//		// With the middle mouse button
//		if (dk::io::button::middle) {
//			auto cursorProjection = dk::geom::intersection(m_camera.castRay(m_window.cursorN()), dk::geom::plane::Y());
//			auto prevCursorProjection = dk::geom::intersection(m_camera.castRay(m_window.cursorN() - m_window.cursorDeltaN()), dk::geom::plane::Y());
//			shift = prevCursorProjection - cursorProjection;
//			// Wrap cursor
//			disableShift = m_window.wrapOutOfBoundsCursor(true) ? 3 : disableShift;
//		}
//		// Apply shift
//		if (disableShift = std::max(0, disableShift - 1); !disableShift)
//			dk::gfx::Camera::Orbit::shift(m_camera, shift);
//
//		// Zoom
//		if (dk::io::wheel::up && !dk::io::button::middle)
//			dk::gfx::Camera::Orbit::zoom(m_camera, 0.9);
//		if (dk::io::wheel::down && !dk::io::button::middle)
//			dk::gfx::Camera::Orbit::zoom(m_camera, 1.1);
//
//		// Set camera aspect ratio
//		m_camera.asp = dk::gfx::backBuffer().aspectRatio();
//	}
//};

struct XXX : public std::enable_shared_from_this<XXX> {
	int y;
};

class AlgoTester {
public:
	void setup()
	{
		// Parse ini file
		auto ini = [] {
 			mINI::INIFile file(dk::common::executable_path().parent_path() / "examples.ini");
			mINI::INIStructure ini;
			file.read(ini);
			return ini;
		}();

		// Setup asset manager directories
		m_assets.root(ini["data"]["path"]);
		m_assets.watch("/shaders" , true);
		m_assets.watch("/textures", true);
		m_assets.watch("/fonts"   , true);
		m_assets.stopWatching("/textures");
		// Setup asset types
		m_assets.type<dk::gfx::ShaderSource>("glsl", dk::gfx::ShaderSource::load, &dk::gfx::ShaderSource::update);
		m_assets.type<dk::gfx::Texture>("png", dk::gfx::Texture::load);
		m_assets.type<dk::gfx::Font>("ttf", dk::gfx::Font::load);
		m_assets.typeName<dk::gfx::ShaderSource>("ShaderSource");
		m_assets.typeName<dk::gfx::Texture>("Texture");
		m_assets.typeName<dk::gfx::Font>("Font");
		// Load assets
		m_assets.synchronize();

		// Setup window
		m_window.properties(
			dk::io::properties::window::title("Algo Tester"), 
			dk::io::properties::window::theme::dark, 
			dk::io::properties::window::size(1280, 720));
		m_window.open(4);
		dk::gfx::backBuffer().property(dk::gfx::properties::multisampling::enabled);

		// Setup user inputs
		m_inputManager.define("shift"             , dk::io::modkey::none + dk::io::button::left);
		m_inputManager.define("toggle_fullscreen" , dk::io::key::f);
		m_inputManager.define("create_edge"       , dk::io::modkey::none + dk::io::button::right);

		// Create shaders
		m_shaders.insert("rgba", m_assets.getMultipleWeak<dk::gfx::ShaderSource>("/shaders/rgba_vs.glsl", "/shaders/rgba_fs.glsl"));
		m_shaders["rgba"].property(dk::gfx::properties::depth_test::enabled);
		m_shaders.insert("text", m_assets.getMultipleWeak<dk::gfx::ShaderSource>("/shaders/text_vs.glsl", "/shaders/text_fs.glsl"));
		m_shaders["text"].property(dk::gfx::properties::depth_test::enabled);
		m_shaders["text"].property(dk::gfx::properties::blend::enabled);
		m_shaders["text"].property(dk::gfx::properties::blend_func_src_factor::src_alpha);
		m_shaders["text"].property(dk::gfx::properties::blend_func_dst_factor::one_minus_src_alpha);

		// Setup camera
		m_camera.lookat   = glm::vec3(0,0,-.1);
		m_camera.position = glm::vec3(0,10,0);
		m_ucCamera.bind("u_camera.VP",        [&]() -> glm::mat4 { return m_camera.P() * m_camera.V(); });
		m_ucCamera.bind("u_camera.position",  [&]() -> glm::vec3 { return m_camera.position; });
		m_ucCamera.bind("u_camera.direction", [&]() -> glm::vec3 { return m_camera.lookat - m_camera.position; });
	}

	void run()
	{
		dk::algo::Funnel             funnel(glm::dvec2(0,0), dk::geom::edge2{ glm::dvec2(0,1), glm::dvec2(0,1) });
		std::vector<dk::geom::edge2> edges;
		dk::geom::edge2              currentEdge;

		while (m_window.isOpen()) {
			const auto& frame = m_window.beginFrame();
			m_assets.synchronize();
			dk::gfx::backBuffer().clear(dk::gfx::FrameBuffer::ClearMask::Color | dk::gfx::FrameBuffer::ClearMask::Depth, DK_COLOR(0x333333ff));
			moveCamera(frame);

			m_shaders["rgba"].uniforms() << m_ucCamera;
			auto drawer2d = dk::gfx::drawer2d(dk::geom::plane::Y(), -dk::geom::axis::X);
			m_dbgVertexSink << dk::gfx::draw(dk::geom::ray3({}, dk::geom::axis::X), dk::colors::red)
				            << dk::gfx::draw(dk::geom::ray3({}, dk::geom::axis::Y), dk::colors::lime)
				            << dk::gfx::draw(dk::geom::ray3({}, dk::geom::axis::Z), dk::colors::blue);

			if (m_inputManager.activated("create_edge"))
				currentEdge[0] = dk::geom::xz(dk::geom::intersection(m_camera.castRay(frame.cursorN()), dk::geom::plane::Y()));
			if (m_inputManager.active("create_edge")) {
				currentEdge[1] = dk::geom::xz(dk::geom::intersection(m_camera.castRay(frame.cursorN()), dk::geom::plane::Y()));
				m_dbgVertexSink << drawer2d(currentEdge, dk::colors::maroon);
			}
			if (m_inputManager.deactivated("create_edge")) {
				funnel.appendPortal(currentEdge);
				edges.push_back(dk::geom::edge2{currentEdge[0], currentEdge[1]});
			}
			funnel.evaluateContainedPaths();
			for (const auto& edge : edges)
				m_dbgVertexSink << drawer2d(edge, dk::colors::aqua, dk::colors::blue);
			m_dbgVertexSink << drawer2d(funnel, dk::colors::red, dk::colors::orange, dk::colors::orange, dk::colors::maroon);

			// Toggle fullscreen with the f key
			if (m_inputManager.activated("toggle_fullscreen"))
				m_window.property(dk::common::toggle(m_window.property<dk::io::properties::window::mode>()));
			// Close window with the esc key
			if (dk::io::key::esc) m_window.close();

			m_dbgVertexSink.flush(m_shaders["rgba"], dk::gfx::backBuffer());

			m_window.endFrame();
		}
	}

	AlgoTester()
		: m_dbgVertexSink(dk::common::id<dk::gfx::RGBAVertex>)
		, m_textSink(dk::common::id<dk::gfx::Font::CharVertex>)
	{ 
		dk::dbg::store<dk::gfx::VertexSink*, "funnel_dbg">() = &m_dbgVertexSink;
	}

private:
	dk::io::Window       m_window;
	dk::io::AssetManager m_assets;
	dk::io::InputManager m_inputManager;

	std::filesystem::path m_dataPath = "../../../examples/_common_data";

	using shaders_t = std::unordered_map<std::string, std::unique_ptr<dk::gfx::Shader>>;

	dk::gfx::VertexSink        m_dbgVertexSink;
	dk::gfx::VertexSink        m_textSink;
	dk::gfx::Camera            m_camera;
	dk::gfx::UniformCollection m_ucCamera;
	//shaders_t                  m_shaders;
	dk::gfx::ShaderCollection  m_shaders;

	void moveCamera(const dk::io::Frame& frame)
	{
		if (m_inputManager.active("shift"))
		{
			auto cursorProjection     = dk::geom::intersection(m_camera.castRay(frame.cursorN()), dk::geom::plane::Y());
			auto prevCursorProjection = dk::geom::intersection(m_camera.castRay(frame.cursorN() - frame.cursorDeltaN()), dk::geom::plane::Y());
			dk::gfx::Camera::Orbit::shift(m_camera, prevCursorProjection - cursorProjection);
		}

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
	spdlog::set_level(spdlog::level::trace);

	AlgoTester rts;
	rts.setup();
	rts.run();

	return 0;
}

//#include <queue>
//
//class Navmesh {
//public:
//	std::optional<const dk::geom::polygon2*> polygonAtPoint(const glm::dvec2& point) const;
//
//	std::optional<const dk::geom::polygon2*> polygonOfEdge(const dk::geom::edge2& edge) const;
//
//	unsigned polygonLevel(const dk::geom::polygon2* polygon) const;
//};
//
//class Route {};
//
//// Definitions:
////	- trivial path -> dst and start points are both contained in the same lvl 2 or under region
////
////  - lvl0 -> has no neighbouring nodes
////  - lvl1 -> has exactly 1 neighbouring node
////  - lvl2 -> has 2 or more neighbouring nodes
////  - lvl3 -> has 3 or more lvl2 neighbours
//
//class Context {
//public:
//	std::optional<Route> findPath(const glm::dvec2& start)
//	{
//		// Setup and optionally find path in lvl less than 2 neighbouring nodes
//		bool lvl3DstNodesDiscovered = false;
//		auto optRoute = bootstrap(lvl3DstNodesDiscovered, start);
//		if (!lvl3DstNodesDiscovered)
//			return optRoute;
//
//	}
//
//private:
//	template <typename K, typename T>
//	using map = std::unordered_map<K, T>;
//
//	template <typename T>
//	using set = std::unordered_set<T>;
//
//	using Vertex = glm::dvec2;
//	using Node   = const dk::geom::polygon2*;
//	using Edge   = dk::geom::edge2;
//	using Funnel = dk::geom::Funnel;
//
//	struct Access {
//		Edge     edge;
//		uint16_t index;
//	};
//
//	struct FrontierElemCost {
//		double fMinCost;
//		double hCost;
//	};
//
//	using FrontierElem = std::pair<Access, FrontierElemCost>;
//	using Frontier     = std::priority_queue<FrontierElem>;
//
//private:
//	// @returns The trivial route if it exists
//	std::optional<Route> bootstrap(bool& shouldContinue, const Vertex& start)
//	{
//		// Setup destination
//		m_destination = start;
//		auto optDstNode = m_navmesh.polygonAtPoint(start);
//		if (!optDstNode.has_value())
//			return std::nullopt;
//		m_destinationNode = optDstNode.value();
//
//		// Handle context reuse
//		if (m_dirty) {
//			if (m_lvl3DstNodes.empty() || trivialPathExists())
//				// Either: reachable area contains no lvl3 nodes, or dst is included in the trivial neighbourhood
//				return findTrivialPath();
//			// Update costs according to new heuristic cost
//			reorderFrontier();
//		}
//
//		// Find lvl3 nodes reachable from the dst
//		discoverLvl3DstNodes();
//		if (m_lvl3DstNodes.empty())
//			// There are no reachable lvl3 nodes
//			return findTrivialPath();
//		shouldContinue = true;
//		return std::nullopt;
//	}
//	
//	// @brief Adjust frontier order based on new h costs
//	void reorderFrontier();
//
//	bool trivialPathExists() const;
//
//	std::optional<Route> findTrivialPath();
//
//	void discoverLvl3DstNodes();
//
//	std::vector<Access> neighboursThrough(const Access& from) const
//	{
//		const auto node = m_navmesh.polygonOfEdge(from.edge);
//		if (!node.has_value())
//			return {};
//		return neighboursOf(node.value());
//	}
//
//	std::vector<Access> neighboursOf(Node node) const;
//
//private:
//	Navmesh& m_navmesh;
//	bool     m_dirty;
//
//	Vertex m_start;
//	Node   m_startNode;
//
//	Vertex m_destination;
//	Node   m_destinationNode;
//
//	set<Node>         m_lvl3DstNodes;
//	map<Node, Funnel> m_lvl3DstNodeFunnels;
//	Frontier          m_frontier;
//};
