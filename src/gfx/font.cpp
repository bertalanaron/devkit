#include <devkit/gfx/font.h>

#include <devkit/algo/geometry.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

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

void dk::gfx::Font::Atlas::textureFromTTF()
{
    std::vector<uint8_t> atlasBitmap(m_atlasSize.x * m_atlasSize.y);

    auto packContext = reinterpret_cast<stbtt_pack_context*>(m_packContext);
    packContext = new stbtt_pack_context();

    stbtt_PackBegin(packContext, atlasBitmap.data(), m_atlasSize.x, m_atlasSize.y, 0, 1, nullptr);
    stbtt_PackFontRange(packContext, m_font.m_ttfBuffer.data(), 0, m_size, 0, DK_GFX_FONT_NUMCHARS, reinterpret_cast<stbtt_packedchar*>(m_chars));
    stbtt_PackEnd(packContext);

    m_texture = Texture::create(m_atlasSize.x, m_atlasSize.y, std::move(atlasBitmap), 1);
    m_texture.property(dk::gfx::properties::min_filter::linear);
    m_texture.property(dk::gfx::properties::mag_filter::linear);
}

dk::gfx::Font::Atlas::Atlas(Font& font, float size, int atlasWidth, int atlasHeight)
    : m_font(font)
    , m_size(size)
    , m_packContext(nullptr)
    , m_atlasSize(atlasWidth, atlasHeight)
{
    textureFromTTF();
}

dk::gfx::Texture& dk::gfx::Font::Atlas::texture()
{
    return m_texture;
}

dk::gfx::Font::Character dk::gfx::Font::Atlas::getCharacter(float& x, char c, const glm::vec4& color, const glm::mat4& transform) const
{
    const auto& glyph = m_chars[c];

    const float xpos = x + glyph.xoff;
    const float ypos = 0 + glyph.yoff;

    const float xpos2 = x + glyph.xoff2;
    const float ypos2 = 0 + glyph.yoff2;

    const float xuv = (float)glyph.uvMin.x / m_atlasSize.x;
    const float yuv = (float)glyph.uvMin.y / m_atlasSize.y;

    const float xuv2 = (float)glyph.uvMax.x / m_atlasSize.x;
    const float yuv2 = (float)glyph.uvMax.y / m_atlasSize.y;

    x += glyph.xadvance;
 
    return Character{
        CharVertex{ glm::vec3(transform * glm::vec4(xpos , ypos2, 0, 1)), glm::vec2(xuv , yuv2), color },
        CharVertex{ glm::vec3(transform * glm::vec4(xpos , ypos , 0, 1)), glm::vec2(xuv , yuv ), color },
        CharVertex{ glm::vec3(transform * glm::vec4(xpos2, ypos , 0, 1)), glm::vec2(xuv2, yuv ), color },
        CharVertex{ glm::vec3(transform * glm::vec4(xpos , ypos2, 0, 1)), glm::vec2(xuv , yuv2), color },
        CharVertex{ glm::vec3(transform * glm::vec4(xpos2, ypos , 0, 1)), glm::vec2(xuv2, yuv ), color },
        CharVertex{ glm::vec3(transform * glm::vec4(xpos2, ypos2, 0, 1)), glm::vec2(xuv2, yuv2), color }
    };
}

std::vector<dk::gfx::Font::CharVertex> dk::gfx::Font::Atlas::get(const std::string& text, const glm::vec4& color, const glm::mat4& transform) const
{
    std::vector<CharVertex> result;
    result.reserve(text.size() * 6);
    float x = 0;
    for (const auto c : text) {
        for (const auto& v : getCharacter(x, c, color, transform)) {
            result.push_back(v);
        }
    }
    return result;
}

dk::gfx::Font::Character dk::gfx::Font::Atlas::getCharacter(float& x, char c, const glm::vec4& color, TextTransfrom& transform) const
{
    const auto& glyph = m_chars[c];

    const float xpos = x + glyph.xoff;
    const float ypos = 0 + glyph.yoff;

    const float xpos2 = x + glyph.xoff2;
    const float ypos2 = 0 + glyph.yoff2;

    const float xuv = (float)glyph.uvMin.x / m_atlasSize.x;
    const float yuv = (float)glyph.uvMin.y / m_atlasSize.y;

    const float xuv2 = (float)glyph.uvMax.x / m_atlasSize.x;
    const float yuv2 = (float)glyph.uvMax.y / m_atlasSize.y;

    transform.bbox.include(glm::dvec2(xpos, ypos));
    transform.bbox.include(glm::dvec2(xpos2, ypos2));

    x += glyph.xadvance;

    return Character{
        CharVertex{ transform.position + xpos  * transform.right + ypos2 * transform.up, glm::vec2(xuv , yuv2), color },
        CharVertex{ transform.position + xpos  * transform.right + ypos  * transform.up, glm::vec2(xuv , yuv ), color },
        CharVertex{ transform.position + xpos2 * transform.right + ypos  * transform.up, glm::vec2(xuv2, yuv ), color },
        CharVertex{ transform.position + xpos  * transform.right + ypos2 * transform.up, glm::vec2(xuv , yuv2), color },
        CharVertex{ transform.position + xpos2 * transform.right + ypos  * transform.up, glm::vec2(xuv2, yuv ), color },
        CharVertex{ transform.position + xpos2 * transform.right + ypos2 * transform.up, glm::vec2(xuv2, yuv2), color }
    };
}

std::vector<dk::gfx::Font::CharVertex> dk::gfx::Font::Atlas::get(const std::string& text, const glm::vec4& color, const TextTransfrom& transform) const
{
    TextTransfrom transformCpy = transform;
    std::vector<CharVertex> result;
    result.reserve(text.size() * 6);
    float x = 0;
    // Get individual characters
    for (const auto c : text) {
        for (const auto& v : getCharacter(x, c, color, transformCpy))
            result.push_back(v);

    }
    const auto signedBboxSize = transformCpy.bbox.max - transformCpy.bbox.min;
    // Offset vertices based on bbox size
    for (auto& v : result) {
        v.elem<0>() += (-transform.offset.x * std::fabs((float)signedBboxSize.x) * transform.right);
        v.elem<0>() += (transform.offset.y * std::fabs((float)signedBboxSize.y) * transform.up);
    }

    return result;
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
    return mutAtlas(fontSize).texture();
}

const dk::gfx::Font::Atlas& dk::gfx::Font::atlas(int fontSize)
{
    return mutAtlas(fontSize);
}

dk::gfx::Font::Atlas& dk::gfx::Font::mutAtlas(int fontSize)
{
    auto it = m_instances.find(fontSize);
    if (it == m_instances.end())
        it = m_instances.insert({ fontSize, std::make_unique<Atlas>(*this, fontSize, 2048, 2048) }).first;
    return *it->second;
}

#include <imgui.h>

dk::gfx::Font::TextTransfrom dk::gfx::Font::TextTransfrom::billboard(const glm::vec3& position, const glm::vec2& offset, const Camera& camera, const glm::dvec3& up)
{
    static float hcScale = 0.00145999901f;
    if (ImGui::Begin("Billboard Props")) {
        ImGui::InputFloat("scale", &hcScale, 0.00001f);
        ImGui::End();
    }

    TextTransfrom result;
    const auto camInvDir = camera.position - camera.lookat;
    float scale = glm::length(camera.position - position);
    result.position = position;
    result.offset   = offset;
    result.right    = scale * hcScale * glm::normalize(glm::cross((glm::vec3)up, camInvDir));
    result.up       = scale * hcScale * glm::normalize(glm::cross(result.right , camInvDir));
    return result;
}
