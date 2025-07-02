#include <devkit/io/window.h>
#include <devkit/gfx/frame_buffer.h>
#include <devkit/algo/geometry.h>
#include <devkit/algo/draw.h>
#include <devkit/gfx/vertex_buffer.h>
#include <devkit/gfx/element_buffer.h>
#include <devkit/io/asset_manager.h>
#include <devkit/io/input_combination.h>
#include <devkit/gfx/shader.h>
#include <devkit/gfx/mesh.h>
#include <devkit/gfx/scene.h>
#include <devkit/gfx/camera.h>
#include <devkit/gfx/texture.h>
#include <devkit/gfx/font.h>
#include <devkit/gfx/vertex_sink.h>

#include <imgui.h>
#include <magic_enum.hpp>
#include <yaml-cpp/yaml.h>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <fstream>

template <typename E, typename C>
	requires(std::is_enum_v<E>)
void imguiPropertyPanel(C& container, dk::common::id_t<E>) {
	// Get enum names
	static std::array<const char*, magic_enum::enum_count<E>()> names = [] {
		std::array<const char*, magic_enum::enum_count<E>()> res{};
		for (int i = 0; i < magic_enum::enum_count<E>(); ++i)
			res[i] = magic_enum::enum_name(magic_enum::enum_cast<E>(i).value()).data();
		return res;
		}();

	// Get current value
	E current = container.property<E>();
	int currentInt = static_cast<int>(current);

	// Show combo box
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Text(magic_enum::enum_type_name<E>().data());
	ImGui::TableSetColumnIndex(1);
	std::string comboLabel = std::format("##{}_selector", magic_enum::enum_type_name<E>().data());
	ImGui::SetNextItemWidth(-1);
	if (ImGui::Combo(comboLabel.c_str(), &currentInt, names.data(), magic_enum::enum_count<E>()))
	{
		E en = magic_enum::enum_cast<E>(currentInt).value();
		container.property(en);
	}
}

template <typename C>
glm::ivec2 imguiPropertyPanel(C& container, const char* name, const glm::ivec2& _current) {
	glm::ivec2 current = _current;

	// Create array for ImGui input
	int values[2] = { current.x, current.y };

	// Show input for ivec2
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Text(name);
	ImGui::TableSetColumnIndex(1);

	std::string inputLabel = std::format("##{}_selector", name);
	ImGui::SetNextItemWidth(-1);
	if (ImGui::InputInt2(inputLabel.c_str(), values)) {
		current = glm::ivec2(values[0], values[1]);
	}
	return current;
}

template <typename C>
std::string imguiPropertyPanel(C& container, const char* name, const std::string& _current) {
	std::string current = _current;

	// Create a buffer for ImGui input (ImGui needs a char buffer)
	static char buffer[256];
	std::strncpy(buffer, current.c_str(), sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = '\0'; // Ensure null termination

	// Show input text box
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Text(name);
	ImGui::TableSetColumnIndex(1);

	std::string inputLabel = std::format("##{}_selector", name);
	ImGui::SetNextItemWidth(-1);
	if (ImGui::InputText(inputLabel.c_str(), buffer, sizeof(buffer))) {
		current = std::string(buffer);
	}
	return current;
}

template <typename E, typename C>
	requires(!std::is_enum_v<E> && requires{ typename E::type; })
void imguiPropertyPanel(C& container, dk::common::id_t<E>) {
	E current = container.property<E>();
	E en = E(imguiPropertyPanel<C>(container, details::common::propertyName<E>(), (typename E::type)(current)));
	container.property(en);
}

template <typename E, typename C>
void imguiPropertyPanel(C& container) {
	imguiPropertyPanel<E>(container, dk::common::id_t<E>{});
}

void imguiPropertiesPanel(auto& container, float labelColumnWidth) {
	if (ImGui::BeginTable("Properties", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed/*, labelColumnWidth*/);
		ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);

		container.foreachParam([](auto& c, auto id) { imguiPropertyPanel(c, id); });

		ImGui::EndTable();
	}
}




#define ABSOLUTE_RESOURCE_PATH "D:\\Projects\\_Sandbox\\cpp\\devkit tdd\\examples\\_common_data\\"

class Example01 {
public:
	void run()
	{
		while (m_window.beginFrame()) {
			// Clear backbuffer
			dk::gfx::backBuffer().clear(dk::gfx::FrameBuffer::ClearMask::Color, DK_COLOR(0x1e1e1eff));
			dk::gfx::backBuffer().clear(dk::gfx::FrameBuffer::ClearMask::Depth, DK_COLOR(0xffffffff));

			m_assetManager.loadFrom(ABSOLUTE_RESOURCE_PATH);

			showGUI();
			moveCamera();

			// Draw scene
			m_shaders["planet"]->uniforms() << m_ucCamera;
			auto& planetTexture = m_assetManager.get<dk::gfx::Texture>(assetPath("planet_texture"));
			m_shaders["planet"]->uniformTexture("u_texture", planetTexture);
			for (auto& mesh : m_assetManager.get<dk::gfx::Scene>(assetPath("planet_model")).meshes()) {
				m_shaders["planet"]->layout(std::ref(mesh.vertices()));
				dk::gfx::backBuffer().render(*m_shaders["planet"], mesh.indices(), dk::gfx::Primitive::Triangles);
			}

			// Bind uniforms and textures
			m_shaders["rgba"]->uniforms()     << m_ucCamera;
			m_colorSink->draw(*m_shaders["rgba"], dk::gfx::backBuffer());

			// Draw asteroids
			m_shaders["asteroid"]->uniforms() << m_ucCamera;
			auto& asteroidTexture = m_assetManager.get<dk::gfx::Texture>(assetPath("asteroid_texture"));
			m_shaders["asteroid"]->uniformTexture("u_texture", asteroidTexture);
			auto& asteroidMesh = m_assetManager.get<dk::gfx::Scene>(assetPath("asteroid_model")).meshes().at(0);
			m_shaders["asteroid"]->layout(std::ref(asteroidMesh.vertices()), std::make_pair(std::ref(*m_meteors), 1));
			// Rotate around y axis
			static float t = 0;
			t += (double)m_window.dt().count() / 80000000000;
			m_shaders["asteroid"]->uniforms().set("u_t", t);
			// Render
			dk::gfx::backBuffer().render(*m_shaders["asteroid"], asteroidMesh.indices(), dk::gfx::Primitive::Triangles, m_meteors->size());

			// Draw skybox
			m_shaders["skybox"]->uniforms()   << m_ucCamera;
			m_shaders["skybox"]->uniformTexture("u_skybox", *m_skybox);
			auto& unitCubeMesh = m_assetManager.get<dk::gfx::Scene>(ABSOLUTE_RESOURCE_PATH "models\\cube.obj").meshes().at(0);
			m_shaders["skybox"]->layout(std::ref(unitCubeMesh.vertices()));
			dk::gfx::backBuffer().render(*m_shaders["skybox"], unitCubeMesh.indices(), dk::gfx::Primitive::Triangles);

			m_window.endFrame();
		}

		auto& camera = m_assetManager.get<nlohmann::json>(ABSOLUTE_RESOURCE_PATH "camera.json");
		camera = nlohmann::json(m_camera);
		m_assetManager.saveAllIn(ABSOLUTE_RESOURCE_PATH);
	}

	void setup()
	{
		// Open window
		m_window.property(dk::io::properties::window::theme::dark);
		m_window.open();

		// Register asset types
		m_assetManager.type<dk::gfx::ShaderSource>("glsl", dk::gfx::ShaderSource::load, &dk::gfx::ShaderSource::update);
		m_assetManager.type<dk::gfx::Scene>("obj", dk::gfx::Scene::load, std::nullopt, std::nullopt, dk::io::AssetManager::Deferred);
		m_assetManager.type<dk::gfx::Texture>("png", dk::gfx::Texture::load);
		m_assetManager.type<YAML::Node>("yaml", YAML::LoadFile, [](YAML::Node& node, const std::string& path) { 
			node = YAML::LoadFile(path);
			spdlog::info("{}", YAML::Dump(node));
		});
		m_assetManager.type<dk::gfx::Font>("ttf", dk::gfx::Font::load);
		using JsonFStream = dk::io::FileStream<nlohmann::json>;
		m_assetManager.type<nlohmann::json>("json", JsonFStream::load, JsonFStream::update, JsonFStream::save);
		
		// Discover assets
		m_assetManager.loadFrom(ABSOLUTE_RESOURCE_PATH);

		// Create shaders
		m_shaders["rgba"] = std::make_unique<dk::gfx::Shader>(
			m_assetManager.getShared<dk::gfx::ShaderSource>(ABSOLUTE_RESOURCE_PATH "shaders\\rgba_vs.glsl"), 
			m_assetManager.getShared<dk::gfx::ShaderSource>(ABSOLUTE_RESOURCE_PATH "shaders\\rgba_fs.glsl"));
		m_shaders["planet"] = std::make_unique<dk::gfx::Shader>(
			m_assetManager.getShared<dk::gfx::ShaderSource>(ABSOLUTE_RESOURCE_PATH "shaders\\mesh_vs.glsl"), 
			m_assetManager.getShared<dk::gfx::ShaderSource>(ABSOLUTE_RESOURCE_PATH "shaders\\textured_fs.glsl"));
		m_shaders["planet"]->property(dk::gfx::properties::depth_test::enabled);
		m_shaders["asteroid"] = std::make_unique<dk::gfx::Shader>(
			m_assetManager.getShared<dk::gfx::ShaderSource>(ABSOLUTE_RESOURCE_PATH "shaders\\instanced_mesh_vs.glsl"), 
			m_assetManager.getShared<dk::gfx::ShaderSource>(ABSOLUTE_RESOURCE_PATH "shaders\\asteroid_fs.glsl"));
		m_shaders["asteroid"]->property(dk::gfx::properties::depth_test::enabled);
		m_shaders["skybox"] = std::make_unique<dk::gfx::Shader>(
			m_assetManager.getShared<dk::gfx::ShaderSource>(ABSOLUTE_RESOURCE_PATH "shaders\\skybox_vs.glsl"), 
			m_assetManager.getShared<dk::gfx::ShaderSource>(ABSOLUTE_RESOURCE_PATH "shaders\\skybox_fs.glsl"));
		m_shaders["skybox"]->property(dk::gfx::properties::depth_test::enabled);
		m_shaders["skybox"]->property(dk::gfx::properties::depth_func::lequal);

		// Load skybox
		m_skybox = std::make_unique<dk::gfx::Texture>(std::move(dk::gfx::Texture::loadCubeMap({
			ABSOLUTE_RESOURCE_PATH "textures\\skyboxes\\stars\\right.png",
			ABSOLUTE_RESOURCE_PATH "textures\\skyboxes\\stars\\left.png",
			ABSOLUTE_RESOURCE_PATH "textures\\skyboxes\\stars\\top.png",
			ABSOLUTE_RESOURCE_PATH "textures\\skyboxes\\stars\\bottom.png",
			ABSOLUTE_RESOURCE_PATH "textures\\skyboxes\\stars\\front.png",
			ABSOLUTE_RESOURCE_PATH "textures\\skyboxes\\stars\\back.png"
			})));
		m_skybox->property(dk::gfx::properties::min_filter::linear);

		// Bind camera
		m_ucCamera.bind("u_camera.VP",      [&]() -> glm::mat4   { return m_camera.P() * m_camera.V(); });
		m_ucCamera.bind("u_camera.position",  [&]() -> glm::vec3 { return m_camera.position; });
		m_ucCamera.bind("u_camera.direction", [&]() -> glm::vec3 { return m_camera.lookat - m_camera.position; });
		// Create camera asset if doesn't exist
		if (!m_assetManager.contains(ABSOLUTE_RESOURCE_PATH, "camera.json"))
			m_assetManager.create(ABSOLUTE_RESOURCE_PATH, "camera.json", nlohmann::json(dk::gfx::Camera(m_camera)));
		m_camera = m_assetManager.get<nlohmann::json>(ABSOLUTE_RESOURCE_PATH "camera.json");

		// Create meteor instances
		m_meteors = std::make_unique<dk::gfx::VertexBuffer>(std::move(dk::gfx::VertexBuffer::create<dk::gfx::Vertex<glm::mat4>>()));
		for (int i = 0; i < conf<int>("asteroid_count"); ++i)
			m_meteors->push_back(dk::gfx::Vertex(glm::mat4(1.0)));
		placeAsteroids(*m_meteors);

		// Create debug sink
		m_colorSink = std::make_unique<dk::gfx::VertexSink>(std::move(dk::gfx::VertexSink::create<dk::gfx::RGBAVertex>()));
		*m_colorSink 
			<< dk::gfx::draw(dk::geom::ray3(glm::vec3(10,10,0), glm::vec3(5,5,0) - glm::vec3(10,10,0)), dk::colors::yellow)
			//<< dk::gfx::draw(m_camera, dk::colors::lime)
			;
	}

private:
	dk::io::AssetManager m_assetManager;
	dk::io::Window       m_window;

	template <typename T>
	using asset_map_t = std::unordered_map<std::string, std::unique_ptr<T>>;
	
	dk::gfx::Camera              m_camera;
	dk::gfx::Camera::Orbit       m_orbit;
	dk::gfx::UniformCollection   m_ucCamera;
	asset_map_t<dk::gfx::Shader> m_shaders;

	std::unique_ptr<dk::gfx::Texture> m_skybox;

	std::unique_ptr<dk::gfx::VertexBuffer> m_meteors;

	std::unique_ptr<dk::gfx::VertexSink> m_colorSink;

	template <typename T>
	T conf(const std::string& path) const
	{
		auto& yaml = m_assetManager.get<YAML::Node>(ABSOLUTE_RESOURCE_PATH "assets.yaml");
		return yaml[path].as<T>();
	}

	std::string assetPath(const std::string& assetName) const
	{
		return conf<std::string>(assetName);
	}

	void showGUI()
	{
		ImGui::Begin("Properties");
		if (ImGui::CollapsingHeader("Backbuffer", ImGuiTreeNodeFlags_DefaultOpen))
			imguiPropertiesPanel(dk::gfx::backBuffer(), 40.f);
		if (ImGui::CollapsingHeader("Window", ImGuiTreeNodeFlags_DefaultOpen))
			imguiPropertiesPanel(m_window, 40.f);
		if (ImGui::CollapsingHeader("Globe Texture", ImGuiTreeNodeFlags_DefaultOpen))
			imguiPropertiesPanel(m_assetManager.get<dk::gfx::Texture>(assetPath("planet_texture")), 40.f);
		ImGui::DragFloat("##fov", &m_camera.fov, 0.01, 0, 3.1415);
		ImGui::End();
	}

	void moveCamera()
	{
		if (dk::io::inclusive(dk::io::key::d))
			m_orbit.tilt(m_camera, { -0.005, 0 });
		if (dk::io::inclusive(dk::io::key::a))
			m_orbit.tilt(m_camera, { 0.005, 0 });
		if (dk::io::inclusive(dk::io::key::w))
			m_orbit.tilt(m_camera, { 0, 0.005 });
		if (dk::io::inclusive(dk::io::key::s))
			m_orbit.tilt(m_camera, { 0, -0.005 });

		if (dk::io::inclusive(dk::io::key::j))
			m_orbit.zoom(m_camera, 0.999);
		if (dk::io::inclusive(dk::io::key::k))
			m_orbit.zoom(m_camera, 1.001);

		m_camera.asp = dk::gfx::backBuffer().aspectRatio();
	}

	void placeAsteroids(dk::gfx::VertexBuffer& asteroids) 
	{
		srand(0);
		static float t = 0;
		t += 0.0001;

		float radius = 40.0;
		float offset = 4.5f;
		for(unsigned int i = 0; i < conf<int>("asteroid_count"); i++)
		{
			glm::mat4 model = glm::mat4(1.0f);
			// 1. translation: displace along circle with 'radius' in range [-offset, offset]
			float angle = (float)i / (float)conf<int>("asteroid_count") * 280.0f + t;
			float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
			float x = sin(angle) * radius + displacement;
			displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
			float y = displacement * 0.2f; // keep height of field smaller compared to width of x and z
			displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
			float z = cos(angle) * radius + displacement;
			model = glm::translate(model, glm::vec3(x, y, z));

			// 2. scale: scale between 0.05 and 0.25f
			float scale = (rand() % 20) / 400.0f + 0.05;
			model = glm::scale(model, glm::vec3(scale));

			// 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
			float rotAngle = (rand() % 360);
			model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

			// 4. now add to list of matrices
			asteroids.set(i, dk::gfx::Vertex(model));
		}  
	}
};


int main(void) {
	spdlog::set_level(spdlog::level::trace);

	dk::common::ThreadDemuxContainer<dk::common::TypelessBuffer> tdc(
		[]{ return dk::common::TypelessBuffer(dk::common::id_t<glm::vec4>{}); });

	dk::common::TypelessBuffer other(dk::common::id_t<glm::vec4>{});
	other.push_back(dk::colors::blue);
	other.push_back(dk::colors::blue);
	other.push_back(dk::colors::blue);

	tdc.local().push_back(dk::colors::red);
	tdc.local().push_back(dk::colors::red);
	tdc.local().push_back(dk::colors::red);
	tdc.local().push_back(dk::colors::red);

	tdc.local().insert(tdc.local().end(), other.begin(), other.end());

	for (int i = 0; i < other.size(); ++i)
		tdc.local().at(i) = other.at(i);

	for (auto& [thread, buffer] : *tdc.global())
	{
		for (glm::vec4& v : buffer)
		{
			spdlog::info("{}", nlohmann::json(v).dump(0));
		}
	}

	Example01 app;
	app.setup();
	app.run();

	return 0;
}
