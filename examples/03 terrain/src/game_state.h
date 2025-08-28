#pragma once
//#include "terrain.h"

#include <mini/ini.h>
#include <devkit/io/asset_manager.h>

void setupAssetManager(dk::io::AssetManager& assets, mINI::INIStructure& ini);

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
