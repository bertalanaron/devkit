#include <devkit/io/frame.h>
#include <devkit/gfx/frame_buffer.h>
#include <devkit/gfx/texture.h>
#include <devkit/common/imgui_helpers.h>

using namespace dk::io;
using namespace dk::gfx;
using namespace dk::common;

class PostProcessLayer {
public:
	PostProcessLayer()                              = default;
	PostProcessLayer(PostProcessLayer&&)            = default;
	PostProcessLayer& operator=(PostProcessLayer&&) = default;

	PostProcessLayer(const std::string& textureUniform, ShaderSource& fragmentSource)
		: m_textureUniform(textureUniform)
	{ 
		m_shader.source(ShaderSource::postProcessVertexSource());
		m_shader.source(fragmentSource, dk::gfx::ShaderSource::Fragment);
		m_frameBuffer.color[0] = Texture2D(glm::ivec2(100, 100), Channels::RGB);
	}

	Texture2D& operator()(Texture2D& input)
	{
		auto& outTex = m_frameBuffer.color[0].get<Texture2D>();
		outTex.resize(input.size());
		m_shader.uniformTexture(m_textureUniform, input);
		m_frameBuffer.render(m_shader);
		return outTex;
	}

	Shader& shader() { return m_shader; }

	Texture2D& get() { return m_frameBuffer.color[0].get<Texture2D>(); }

private:
	std::string m_textureUniform;
	Shader      m_shader;
	FrameBuffer m_frameBuffer;
};

class Terrain {
public:
	using MaxHeight       = dk::common::UniqueProperty<float, "u_maxHeight">;
	using HeightOffset    = dk::common::UniqueProperty<float, "u_heightOffset">;
	using MinTessLevel    = dk::common::UniqueProperty<int  , "u_minTessLevel">;
	using MaxTessLevel    = dk::common::UniqueProperty<int  , "u_maxTessLevel">;
	using TessMinDistance = dk::common::UniqueProperty<float, "u_tessMinDistance">;
	using TessMaxDistance = dk::common::UniqueProperty<float, "u_tessMaxDistance">;

	class Config : DK_CONFIG_SPECIALIZATION(Terrain,
		MaxHeight, HeightOffset, MinTessLevel, MaxTessLevel, TessMinDistance, TessMaxDistance);

	Config config;

public:
	Terrain()                     = default;
	Terrain(Terrain&&)            = default;
	Terrain& operator=(Terrain&&) = default;

	Terrain(AssetManager& assets, const glm::ivec2& size);
	Terrain(AssetManager& assets, const std::filesystem::path& heightmapPath);

	void render(FrameBuffer& out, const Camera& camera);

	Texture2D& heightmap() { return *m_heightmap; }

private:
	using TerrainVertex = Vertex<glm::vec3, glm::vec2>;

	Texture2D*       m_heightmap;
	Shader*          m_shader;
	PostProcessLayer m_heightToNormal;
	VertexBuffer     m_vertices = dk::common::id<TerrainVertex>;
};

namespace dk::imgui_helpers {
DK_IMHELPER_SCALAR_CONFIG_MINMAXSTEP(Terrain::MaxHeight      , 0.1,  100, 0.1);
DK_IMHELPER_SCALAR_CONFIG_MINMAXSTEP(Terrain::HeightOffset   , -64,   64, 0.1);
DK_IMHELPER_SCALAR_CONFIG_MINMAXSTEP(Terrain::MinTessLevel   ,   0,   64, 0.1);
DK_IMHELPER_SCALAR_CONFIG_MINMAXSTEP(Terrain::MaxTessLevel   ,   0,   64, 0.1);
DK_IMHELPER_SCALAR_CONFIG_MINMAXSTEP(Terrain::TessMinDistance,   0, 1000, 0.1);
DK_IMHELPER_SCALAR_CONFIG_MINMAXSTEP(Terrain::TessMaxDistance,   0, 1000, 0.1);
}
