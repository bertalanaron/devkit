#include <devkit/gfx/texture.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <GL/glew.h>

dk::gfx::Texture dk::gfx::Texture::load(const std::string& path)
{
    Texture texture;

    // Load file using stb_image
    unsigned char* pixels = stbi_load(path.c_str(), &texture.m_size.x, &texture.m_size.y, &texture.m_channels, 0);
    if (!pixels)
    {
        const char* error = stbi_failure_reason();
        spdlog::error("Failed to load texture from {}", path);
        spdlog::error("stb_image error: {}", error);
        return texture;
    }
    // Save buffer
    size_t buffSize = texture.m_size.x * texture.m_size.y * texture.m_channels;
    texture.m_opt_pixels = std::vector<uint8_t>(pixels, pixels + buffSize);
    std::visit([](auto& v) {
        spdlog::info(v.size());
        }, texture.m_opt_pixels.value());
    stbi_image_free(pixels);
    
    spdlog::trace("Loaded texture from {}", path);
    return texture;
}

dk::gfx::Texture dk::gfx::Texture::loadCubeMap(const std::array<std::string, 6>& paths)
{
    Texture texture;
    texture.m_type = Type::Cubemap;

    std::array<std::vector<uint8_t>, 6> faces;
    for (int i = 0; i < paths.size(); ++i) {
        // Load file using stb_image
        unsigned char* pixels = stbi_load(paths[i].c_str(), &texture.m_size.x, &texture.m_size.y, &texture.m_channels, 0);
        if (!pixels)
        {
            const char* error = stbi_failure_reason();
            spdlog::error("Failed to load texture from {}", paths[i]);
            spdlog::error("stb_image error: {}", error);
            return texture;
        }
        // Save buffer
        size_t buffSize = texture.m_size.x * texture.m_size.y * texture.m_channels;
        faces[i] = std::vector<uint8_t>(pixels, pixels + buffSize);
        stbi_image_free(pixels);
    }
    texture.m_opt_pixels.emplace(std::move(faces));

    spdlog::trace("Loaded cubemap from {}", paths[0]);
    return texture;
}

dk::gfx::Texture dk::gfx::Texture::create(unsigned width, unsigned height, std::vector<uint8_t>&& pixels, int channels)
{
    Texture texture;
    texture.m_size = { width, height };
    texture.m_channels = channels;
    texture.m_opt_pixels = std::move(pixels);
    return texture;
}

void dk::gfx::Texture::makeActive(int unit)
{
    init();

    // Bind texture
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(details::gfx::toUnderlying(m_type), m_handle);

    callPropertySetters(true);
}

unsigned int dk::gfx::Texture::handle() const
{
    return m_handle;
}

dk::gfx::Texture::Type dk::gfx::Texture::type() const
{
    return m_type;
}

int toInternalFormat(int channels)
{
    if (channels == 1) return GL_RED;
    if (channels == 2) return GL_RG;
    if (channels == 3) return GL_RGB;
    if (channels == 4) return GL_RGBA;
}

void dk::gfx::Texture::init()
{
    if (m_handle)
        return;

    // Generate texture
    glGenTextures(1, &m_handle);
    glBindTexture(details::gfx::toUnderlying(m_type), m_handle);

    // Set default filtering
    glTexParameteri(details::gfx::toUnderlying(m_type), GL_TEXTURE_MIN_FILTER, details::gfx::toUnderlying(property<dk::gfx::properties::min_filter>()));
    glTexParameteri(details::gfx::toUnderlying(m_type), GL_TEXTURE_MAG_FILTER, details::gfx::toUnderlying(property<dk::gfx::properties::mag_filter>()));

    if (!m_opt_pixels.has_value()) { 
        // Create empty texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(details::gfx::toUnderlying(m_type), 0, toInternalFormat(m_channels), (unsigned)m_size.x, (unsigned)m_size.y, 0, toInternalFormat(m_channels), GL_UNSIGNED_BYTE, nullptr);
        return;
    }
    std::visit(dk::common::overload {
        // Load normal texture
        [this](const std::vector<uint8_t>& pixels) {
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
            glTexImage2D(GL_TEXTURE_2D, 0, toInternalFormat(m_channels), (unsigned)m_size.x, (unsigned)m_size.y, 0, toInternalFormat(m_channels), GL_UNSIGNED_BYTE, pixels.data());
        },
        // Load cube map
        [this](const std::array<std::vector<uint8_t>, 6>& faces) {
            for (int i = 0; i < faces.size(); ++i) {
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                    0, toInternalFormat(m_channels), (unsigned)m_size.x, (unsigned)m_size.y, 0, toInternalFormat(m_channels), GL_UNSIGNED_BYTE, faces.at(i).data()
                );
            }
        }
    }, m_opt_pixels.value());
}

dk::gfx::TextureUnit::SlotRef::SlotRef(TextureUnit& unit, int slot)
    : m_unit(unit)
    , m_slot(slot)
{ }

void dk::gfx::TextureUnit::SlotRef::operator=(Texture & texture)
{
    m_unit.m_textures.at(m_slot) = texture;
}

dk::gfx::TextureUnit::TextureUnit()
{
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_slotCount);
    m_textures.resize(m_slotCount);
}

dk::gfx::TextureUnit::SlotRef dk::gfx::TextureUnit::operator[](int slot)
{
    return SlotRef(*this, slot);
}

std::optional<int> dk::gfx::TextureUnit::getSlot(Texture& texture)
{
    for (int i = 0; i < m_slotCount; ++i)
        if (m_textures[i].has_value()
            && std::addressof(m_textures[i].value().get()) == std::addressof(texture))
            return i;
    return {};
}

std::optional<int> dk::gfx::TextureUnit::emptySlot() const
{
    for (int i = 0; i < m_slotCount; ++i)
        if (!m_textures[i].has_value())
            return i;
    return {};
}

void dk::gfx::TextureUnit::clear()
{
    for (auto& texture : m_textures)
        texture = {};
}

size_t dk::gfx::TextureUnit::size() const
{
    return m_slotCount;
}

void dk::gfx::TextureUnit::makeActive()
{
    for (int i = 0; i < m_slotCount; ++i) {
        if (!m_textures[i].has_value())
            continue;
        m_textures[i].value().get().makeActive(i);
    }
}

template <>
void details::common::setProperty(dk::gfx::Texture& texture, const dk::gfx::properties::min_filter& min_filter)
{
    glTexParameteri(details::gfx::toUnderlying(texture.type()), GL_TEXTURE_MIN_FILTER, details::gfx::toUnderlying(min_filter));
}

template <>
void details::common::setProperty(dk::gfx::Texture& texture, const dk::gfx::properties::mag_filter& mag_filter)
{
    glTexParameteri(details::gfx::toUnderlying(texture.type()), GL_TEXTURE_MAG_FILTER, details::gfx::toUnderlying(mag_filter));
}

int details::gfx::toUnderlying(dk::gfx::properties::min_filter min_filter)
{
    switch (min_filter)
    {
    case dk::gfx::properties::min_filter::nearest: return GL_NEAREST;
    case dk::gfx::properties::min_filter::linear : return GL_LINEAR;
    default: return 0;
    }
}

int details::gfx::toUnderlying(dk::gfx::properties::mag_filter mag_filter)
{
    return toUnderlying(dk::gfx::properties::min_filter(mag_filter));
}

int details::gfx::toUnderlying(const dk::gfx::Texture::Type& type)
{
    switch (type)
    {
    case dk::gfx::Texture::Type::Normal : return GL_TEXTURE_2D;
    case dk::gfx::Texture::Type::Cubemap: return GL_TEXTURE_CUBE_MAP;
    default:
        return 0;
    }
}
