#pragma once
#include <devkit/gfx/common.h>
#include <devkit/gfx/texture.h>
#include <devkit/gfx/vertex.h>
#include <devkit/algo/geometry.h>
#include <devkit/gfx/camera.h>

#ifndef DK_GFX_FONT_NUMCHARS
#define DK_GFX_FONT_NUMCHARS 128
#endif

namespace dk::gfx {

class Font {
public:
	using CharVertex = Vertex<glm::vec3, glm::vec2, glm::vec4>;
	using Character  = std::array<CharVertex, 6>;

	struct TextTransfrom {
		glm::vec3 position = glm::vec3(0, 0, 0);
		glm::vec3 up       = geom::axis::Y;
		glm::vec3 right    = geom::axis::X;

		glm::vec2   offset; 
		geom::bbox2 bbox;

		static TextTransfrom billboard(const glm::vec3& position, const glm::vec2& offset, const Camera& camera, const glm::dvec3& up);
	};

private:
	struct PackedCharData {
		glm::lowp_u16vec2 uvMin;
		glm::lowp_u16vec2 uvMax;
		float xoff,yoff,xadvance;
		float xoff2,yoff2;
	};

	class Atlas {
	public:
		Atlas(Font& font, float size, int atlasWidth, int atlasHeight);

		dk::gfx::Texture& texture();

		Character getCharacter(float& x, char c, const glm::vec4& color, const glm::mat4& transform) const;
		std::vector<CharVertex> get(const std::string& text, const glm::vec4& color, const glm::mat4& transform) const;

		Character getCharacter(float& x, char c, const glm::vec4& color, TextTransfrom& transform) const;
		std::vector<CharVertex> get(const std::string& text, const glm::vec4& color, const TextTransfrom& transform) const;

	private:
		Font&              m_font;
		float              m_size;
		void*              m_packContext = nullptr;
		dk::gfx::Texture2D m_texture;
		PackedCharData     m_chars[DK_GFX_FONT_NUMCHARS];
		glm::ivec2         m_atlasSize;

		void textureFromTTF();
	};

public:
	static Font load(const std::string& path);

	Texture& texture(int fontSize);

	const Atlas& atlas(int fontSize);

private:
	using instances_t = std::unordered_map<int, std::unique_ptr<Atlas>>;

	void*                      m_info = nullptr;
	std::vector<unsigned char> m_ttfBuffer;
	instances_t                m_instances;

	Atlas& mutAtlas(int fontSize);
};

}
