#include "terrain.h"

Terrain::Terrain(AssetManager& assets, const glm::ivec2& size)
	: m_heightmap(size, Channels::R, Format::Unsigned8)
{ }

Terrain::Terrain(AssetManager& assets, const std::filesystem::path& heightmapPath)
	: m_heightmap(assets.root() / heightmapPath)
{
	ShaderDescriptor desc;
	desc.fragment = "/shaders/terrain_fs.glsl";

	m_shader = Shader(desc, std::bind(&AssetManager::get<ShaderSource>, assets));

}

void Terrain::render(FrameBuffer& out)
{

}
