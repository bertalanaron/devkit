#include <fstream>
#include <filesystem>

#include <devkit/io/asset_manager.h>
#include <devkit/io/asset_fetcher.h>
#include <gtest/gtest.h>


// Simple test asset types
struct Texture2D {
	std::string path;
	int width = 0;
	int height = 0;

	static Texture2D load(const char* path) {
		return Texture2D{ .path = path, .width = 256, .height = 256 };
	}

	Texture2D() = default;
};

struct ShaderSource {
	std::string source;

	static ShaderSource load(const char* path) {
		std::ifstream ifs(path);
		if (!ifs.is_open())
			throw std::runtime_error("Could not open file: " + std::string(path));

		std::stringstream buffer;
		buffer << ifs.rdbuf();

		return ShaderSource{ .source = buffer.str() };
	}
};

struct Shader {
	std::string vertexSource;
	std::string fragmentSource;

	Shader() = default;
	
	Shader(std::tuple<std::weak_ptr<ShaderSource>, std::weak_ptr<ShaderSource>> sources) {
		auto [vs_weak, fs_weak] = sources;
		if (auto vs = vs_weak.lock()) {
			vertexSource = vs->source;
		}
		if (auto fs = fs_weak.lock()) {
			fragmentSource = fs->source;
		}
	}
};

void clearDirectory(const std::string& path) {
	namespace fs = std::filesystem;
	if (!fs::exists(path)) return;
	for (const auto& entry : fs::directory_iterator(path)) {
		fs::remove_all(entry.path());
	}
}

void createFileWithTextContent(const std::string& path, const std::string& value) {
	std::filesystem::create_directories(std::filesystem::path(path).parent_path());

	std::ofstream ofs(path);
	if (!ofs.is_open())
		throw std::runtime_error("Could not create file: " + path);
	ofs << value;
}


namespace io_tests {

TEST(AssetFetcher, simple_usage) {
	using namespace dk;
	namespace fs = std::filesystem;

	// Setup test directory
	clearDirectory("test_fetcher/");
	createFileWithTextContent("test_fetcher/textures/globe.png", "texture_data");

	// Create asset manager and register types
	io::AssetManager assetManager;
	assetManager.root("test_fetcher/", true);
	assetManager.type<Texture2D>("png", Texture2D::load);
	assetManager.synchronize();

	// Create a simple proxy whose only purpose is to reduce boilerplate
	io::AssetFetcher<Texture2D> textures(assetManager, "/textures");
	
	// Fetch asset using the proxy
	auto& globeTexture = textures["globe.png"];
	
	// Verify it loaded correctly
	ASSERT_EQ(globeTexture.width, 256);
	ASSERT_EQ(globeTexture.height, 256);
	ASSERT_TRUE(globeTexture.path.find("globe.png") != std::string::npos);
}

TEST(AssetFetcher, custom_loader) {
	using namespace dk;
	namespace fs = std::filesystem;

	// Setup test directory with shader files
	clearDirectory("test_fetcher_shader/");
	createFileWithTextContent("test_fetcher_shader/shaders/terrain_vs.glsl", "vertex shader code");
	createFileWithTextContent("test_fetcher_shader/shaders/terrain_fs.glsl", "fragment shader code");

	// Create asset manager and register types
	io::AssetManager assetManager;
	assetManager.root("test_fetcher_shader/", true);
	assetManager.type<ShaderSource>("glsl", ShaderSource::load);
	assetManager.synchronize();

	// Create a proxy which builds from multiple already loaded assets
	io::AssetFetcher<Shader> shaders(assetManager, "/shaders", 
		[](auto& fetcher, const std::filesystem::path& path) {
			const auto vs_path = fetcher.root() / std::format("{}_vs.glsl", path.string());
			const auto fs_path = fetcher.root() / std::format("{}_fs.glsl", path.string());
			return Shader(fetcher.assets.getMultipleWeak<ShaderSource>(vs_path, fs_path));
		});
	
	// Fetch shader using the proxy
	auto& shader = shaders["terrain"];
	
	// Verify it loaded correctly from multiple sources
	ASSERT_EQ(shader.vertexSource, "vertex shader code");
	ASSERT_EQ(shader.fragmentSource, "fragment shader code");
}

TEST(AssetFetcher, root_path) {
	using namespace dk;

	io::AssetManager assetManager;
	assetManager.root("test/", false);

	io::AssetFetcher<Texture2D> textures(assetManager, "/textures");
	
	// Verify root path
	ASSERT_EQ(textures.root().string(), "textures");
}

}
