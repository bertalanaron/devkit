#include "editor_client.h"

#include <devkit/gfx/texture.h>

void setupAssetManager(dk::io::AssetManager& assets, mINI::INIStructure& ini)
{
	assets.root(ini["data"]["path"]);

	assets.type<dk::gfx::Texture>("png", 
		dk::gfx::Texture::load, std::nullopt, std::nullopt, dk::io::AssetManager::Async);
	assets.watch("/textures", false);
	assets.watch("/textures/terrain", false);
	assets.watch("/textures/ui"     , true);
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
