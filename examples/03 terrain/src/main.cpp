#include "editor_client.h"

#include <devkit/gfx/texture.h>

void setupAssetManager(dk::io::AssetManager& assets, mINI::INIStructure& ini)
{
	// Define root folder
	assets.root(ini["data"]["path"]);

	// Register types
	assets.type<dk::gfx::Texture>({ "png", "jpg" },
		dk::gfx::Texture::load, std::nullopt, std::nullopt, dk::io::AssetManager::Async);
	assets.type<dk::gfx::ShaderSource>("glsl", dk::gfx::ShaderSource::load, &dk::gfx::ShaderSource::update);

	// Setup directories to watch
	assets.watch("/textures", true);
	//assets.watch("/textures/terrain", false);
	//assets.watch("/textures/ui"     , true);
	assets.watch("/shaders" , true);
}

int main()
{
	// Load ini
	auto ini = [] {
		mINI::INIFile file(dk::common::executable_path().parent_path() / "examples.ini");
		mINI::INIStructure ini;
		file.read(ini);
		return ini;
	}();

	// Set loglevel
	spdlog::set_level(spdlog::level::trace);

	// Initialize and run editor client
	EditorClient editor(ini);
	editor.run();

	return 0;
}
