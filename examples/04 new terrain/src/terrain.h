#include <devkit/io/frame.h>
#include <devkit/gfx/frame_buffer.h>
#include <devkit/gfx/texture.h>

using namespace dk::io;
using namespace dk::gfx;

class Terrain {
public:
	Terrain()                     = default;
	Terrain(Terrain&&)            = default;
	Terrain& operator=(Terrain&&) = default;

	Terrain(AssetManager& assets, const glm::ivec2& size);
	Terrain(AssetManager& assets, const std::filesystem::path& heightmapPath);

	void render(FrameBuffer& out);

	Texture2D& heightmap() { return m_heightmap; }

private:
	Texture2D m_heightmap;
	
	Shader    m_shader;
};
