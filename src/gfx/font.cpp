#include <devkit/gfx/font.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define DK_TEXTU_NUM_CHARS 200

std::vector<unsigned char> loadTTF(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open TTF file");

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    std::vector<unsigned char> buffer(size);

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), size);

    return buffer;
}

dk::gfx::Texture textureFromTTF(unsigned char* ttfBuffer, int atlasWidth, int atlasHeight, float pixelHeight, void* packContext/*, dk::gfx::Font::Character characters[200]*/)
{
    std::vector<uint8_t> atlasBitmap(atlasWidth * atlasHeight);

    auto pc = reinterpret_cast<stbtt_pack_context*>(packContext);
    pc = new stbtt_pack_context();

    stbtt_PackBegin(pc, atlasBitmap.data(), atlasWidth, atlasHeight, 0, 1, nullptr);

    stbtt_packedchar packedChars[200];
    stbtt_PackFontRange(pc, ttfBuffer, 0, pixelHeight, 0, 200, packedChars);

    stbtt_PackEnd(pc);

    //stbtt_bakedchar bakedChars[96]; // ASCII 32..126
    //stbtt_BakeFontBitmap(ttfBuffer, 0, pixelHeight, atlasBitmap.data(), atlasWidth, atlasHeight, 32, 96, bakedChars);

    return dk::gfx::Texture::create(atlasWidth, atlasHeight, std::move(atlasBitmap), 1);
}

dk::gfx::Font::SizeInstance::SizeInstance(Font& font, float size, int atlasWidth, int atlasHeight)
    : m_font(font)
    , m_size(size)
    , m_packContext(nullptr)
    , m_texture(textureFromTTF(m_font.m_ttfBuffer.data(), atlasWidth, atlasHeight, size, m_packContext))
{ }

dk::gfx::Texture& dk::gfx::Font::SizeInstance::texture()
{
    return m_texture;
}

dk::gfx::Font dk::gfx::Font::load(const std::string& path)
{
    Font font;

    // Load file into buffer
    font.m_ttfBuffer = loadTTF(path);

    // Initialize font info
    font.m_info = new stbtt_fontinfo();
    auto& info = *reinterpret_cast<stbtt_fontinfo*>(font.m_info);
    stbtt_InitFont(&info, font.m_ttfBuffer.data(), 0);

    return font;
}

dk::gfx::Texture& dk::gfx::Font::texture(int fontSize)
{
    auto it = m_instances.find(fontSize);
    if (it == m_instances.end())
        it = m_instances.insert({ fontSize, std::make_unique<SizeInstance>(*this, fontSize, 2048, 2048) }).first;

    return it->second->texture();
}
