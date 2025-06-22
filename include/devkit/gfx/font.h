#pragma once
#include <devkit/gfx/common.h>
#include <devkit/gfx/texture.h>

namespace dk::gfx {

class Font {
private:
	class SizeInstance {
	public:
		SizeInstance(Font& font, float size, int atlasWidth, int atlasHeight);

		dk::gfx::Texture& texture();

	private:
		Font&            m_font;
		float            m_size;
		void*            m_packContext;
		dk::gfx::Texture m_texture;
	};

public:
	static Font load(const std::string& path);

	Texture& texture(int fontSize);

private:
	using instance_map_t = std::unordered_map<int, std::unique_ptr<SizeInstance>>;

	void*                      m_info;
	std::vector<unsigned char> m_ttfBuffer;
	instance_map_t             m_instances;
};

}
