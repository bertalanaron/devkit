#include "terrain.h"

//Terrain::Terrain(AssetManager& assets, const glm::ivec2& size)
//	: m_heightmap(size, Channels::R, Format::Unsigned8)
//{ }

Terrain::Terrain(AssetManager& assets, const std::filesystem::path& heightmapPath)
	: m_heightmap(&assets.get<Texture2D>(heightmapPath))
	, m_shader(&assets.get<Shader>("/shaders/terrain.shader"))
{
	config(MaxHeight(16));
	config(HeightOffset(16));
	config(MinTessLevel(4));
	config(MaxTessLevel(64));
	config(TessMaxDistance(150));
	config(TessMinDistance(0));

	m_heightToNormal = PostProcessLayer("u_texture", assets.get<ShaderSource>("/shaders/displacement_to_normal.fs.glsl"));

	m_heightmap->config(Texture::MagFilter::Linear);
	m_heightmap->config(Texture::MinFilter::Linear);

	glm::dvec2 size(500, 200);
	unsigned rezX = 50;
	unsigned rezY = size.y / size.x * (double)rezX;
	for(unsigned i = 0; i <= rezX-1; i++)
	{
		for(unsigned j = 0; j <= rezY-1; j++)
		{
			m_vertices.modify().push_back(TerrainVertex{ 
				glm::vec3(-size.x/2.0f + size.x*i/(float)rezX, 0.0f, -size.y/2.0f + size.y*j/(float)rezY), 
				glm::vec2(i / (float)rezX, j / (float)rezY) });

			m_vertices.modify().push_back(TerrainVertex{ 
				glm::vec3(-size.x/2.0f + size.x*(i+1)/(float)rezX, 0.0f, -size.y/2.0f + size.y*j/(float)rezY), 
				glm::vec2((i+1) / (float)rezX, j / (float)rezY) });

			m_vertices.modify().push_back(TerrainVertex{ 
				glm::vec3(-size.x/2.0f + size.x*i/(float)rezX, 0.0f, -size.y/2.0f + size.y*(j+1)/(float)rezY), 
				glm::vec2(i / (float)rezX, (j+1) / (float)rezY) });

			m_vertices.modify().push_back(TerrainVertex{ 
				glm::vec3(-size.x/2.0f + size.x*(i+1)/(float)rezX, 0.0f, -size.y/2.0f + size.y*(j+1)/(float)rezY), 
				glm::vec2((i+1) / (float)rezX, (j+1) / (float)rezY) });
		}
	}
}

void Terrain::render(FrameBuffer& out, const Camera& camera)
{
	m_shader->layout(m_vertices);

	auto& normalmap = m_heightToNormal(*m_heightmap);

	config.for_each([this](const auto& prop) {
		if (!config.dirty(prop)) return;
		const std::string_view name = config.property_name(prop);
		if (name.find("u_") == name.npos) return;
		const std::string namestr(name.cbegin(), name.cend());
		m_shader->uniforms().set(namestr, config.property_value(prop));
	});
	config.reset_dirty();

	m_shader->uniforms().set("u_camera.view",       camera.V());
	m_shader->uniforms().set("u_camera.projection", camera.P());
	m_shader->uniforms().set("u_camera.position",   camera.position);
	m_shader->uniforms().set("u_camera.direction",  camera.lookat - camera.position);

	m_shader->uniforms().set("u_model", glm::identity<glm::mat4>());

	m_shader->uniformTexture("u_heightMap", heightmap());
	m_shader->uniformTexture("u_normalMap", normalmap);

	out.render(*m_shader, m_vertices, Primitive::Patches);
}
