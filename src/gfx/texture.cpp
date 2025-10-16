#include <devkit/gfx/texture.h>
#include <devkit/gfx/frame_buffer.h>
#include "context.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <GL/glew.h>

#include <imgui.h>

unsigned dk::gfx::Texture::handle()
{
    if (m_type == api::TextureType::Unset)
        throw std::runtime_error(
            "cannot get handle of texture with unset type "
            "(don't use the default constructor of any texture type)");

    // Call initializer with handle
    if (m_initializer.has_value())
    {
        m_apiHandle.bind(m_type);
        m_initializer.value()(m_apiHandle.handle(), this);
        m_initializer.reset();
        callPropertySetters(true);
    }
    
    // Return api handle
    return m_apiHandle.handle();
}

void dk::gfx::Texture::bindToUnit(unsigned unit)
{
    // Attach to texture unit
    glActiveTexture(GL_TEXTURE0 + unit);
    m_apiHandle.bind(m_type);
    // Call initializer with handle
    if (m_initializer.has_value())
    {
        m_initializer.value()(m_apiHandle.handle(), this);
        m_initializer.reset();
        callPropertySetters(true);
    }

    callPropertySetters(false);
}

void dk::gfx::Texture1D::setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level)
{
    m_apiHandle.bind(m_type);
    if (attachment == api::Attachment::Color0)
        glFramebufferTexture1D(GL_FRAMEBUFFER, api::toUnderlying(attachment) + colorIndex, GL_TEXTURE_1D, m_apiHandle.handle(), level);
    else
        glFramebufferTexture1D(GL_FRAMEBUFFER, api::toUnderlying(attachment), GL_TEXTURE_1D, m_apiHandle.handle(), level);

    callPropertySetters(false);
}

dk::gfx::Texture2D::Texture2D(const glm::ivec2& size, Channels channels)
    : Texture(api::TextureType::Texture2D, channels)
    , m_size(size)
{
    if (std::max(size.x, size.y) >= dk::io::GlobalState::hardware().glMaxTextureSize)
        throw std::runtime_error("texture size exceeds hardware maximum");

    // Emplace initializer which sets up texture buffer
    m_initializer.emplace([&](unsigned handle, Texture* texture) {
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, api::internalFormat(channels), (unsigned)size.x, (unsigned)size.y, 0, api::toUnderlying(channels), GL_UNSIGNED_BYTE, nullptr);
    });
}

stbi_uc* loadTexture2DFromFile(const std::filesystem::path& path, glm::ivec2& size, unsigned& channels)
{
    // Load file using stb_image
    int channelCount = 1;
    auto pixels = stbi_load(path.string().c_str(), &size.x, &size.y, &channelCount, 0);
    if (!pixels)
    {
        const char* error = stbi_failure_reason();
        spdlog::error("Failed to load texture from {}", path.string());
        spdlog::error("stb_image error: {}", error);
        return nullptr;
    }

    // Get api enum for channels
    channels = [=]() {
        if (channelCount == 1) return GL_RED;
        if (channelCount == 2) return GL_RG;
        if (channelCount == 3) return GL_RGB;
        if (channelCount == 4) return GL_RGBA;
    }();

    return pixels;
}

dk::gfx::Channels channelCountToEnum(int channels)
{
    if (channels == 1) return dk::gfx::Channels::R;
    if (channels == 2) return dk::gfx::Channels::RG;
    if (channels == 3) return dk::gfx::Channels::RGB;
    if (channels == 4) return dk::gfx::Channels::RGBA;
    throw std::runtime_error("invalid channel count");
}

dk::gfx::Texture2D::Texture2D(const std::filesystem::path& path)
    : Texture(api::TextureType::Texture2D, Channels::R)
{
    unsigned channels = 0;
    auto pixels = loadTexture2DFromFile(path, m_size, channels);

    // Emplace initializer which parses image data from file upon resource initialization
    if (pixels)
        m_initializer.emplace([path=path,channels,pixels](unsigned handle, Texture* tex) {
            Texture2D* texture = dynamic_cast<Texture2D*>(tex);
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
            glTexImage2D(GL_TEXTURE_2D, 0, channels, (unsigned)texture->m_size.x, (unsigned)texture->m_size.y, 0, channels, GL_UNSIGNED_BYTE, pixels);

            stbi_image_free(pixels);
        });
}

dk::gfx::Texture2D dk::gfx::Texture2D::loadFromFileAndInitialize(const std::string& path)
{
    Texture2D texture(path);
    //texture.handle();
    return texture;
}

void dk::gfx::Texture2D::setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level)
{
    m_apiHandle.bind(m_type);
    if (attachment == api::Attachment::Color0)
        glFramebufferTexture2D(GL_FRAMEBUFFER, api::toUnderlying(attachment) + colorIndex, GL_TEXTURE_2D, m_apiHandle.handle(), level);
    else
        glFramebufferTexture2D(GL_FRAMEBUFFER, api::toUnderlying(attachment), GL_TEXTURE_2D, m_apiHandle.handle(), level);

    callPropertySetters(false);
}

dk::gfx::MultisampledTexture2D::MultisampledTexture2D(const glm::ivec2& size, unsigned samples, Channels channels)
    : Texture(api::TextureType::MultisampledTexture2D, channels)
    , m_size(size)
    , m_samples(samples)
{
    if (std::max(size.x, size.y) >= dk::io::GlobalState::hardware().glMaxTextureSize)
        throw std::runtime_error("texture size exceeds hardware maximum");

    // Emplace initializer which sets up texture buffer
    m_initializer.emplace([&](unsigned handle, Texture*) {
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, api::internalFormat(channels), (unsigned)size.x, (unsigned)size.y, GL_TRUE);
    });
}

void dk::gfx::MultisampledTexture2D::setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level)
{
    m_apiHandle.bind(m_type);
    if (attachment == api::Attachment::Color0)
        glFramebufferTexture2D(GL_FRAMEBUFFER, api::toUnderlying(attachment) + colorIndex, GL_TEXTURE_2D_MULTISAMPLE, m_apiHandle.handle(), level);
    else
        glFramebufferTexture2D(GL_FRAMEBUFFER, api::toUnderlying(attachment), GL_TEXTURE_2D_MULTISAMPLE, m_apiHandle.handle(), level);
    
    callPropertySetters(false);
}

void loadCubemapFromFile(
    const std::array<std::filesystem::path, 6>& paths, 
    unsigned                                    handle, 
    glm::ivec2&                                 size, 
    int&                                        channels)
{
    for (int i = 0; i < paths.size(); ++i) {
        glm::ivec2 faceSize;
        int        faceChannels;

        // Load file using stb_image
        unsigned char* pixels = stbi_load(paths[i].string().c_str(), &faceSize.x, &faceSize.y, &faceChannels, 0);
        if (!pixels)
        {
            const char* error = stbi_failure_reason();
            spdlog::error("Failed to load texture from {}", paths[i].string());
            spdlog::error("stb_image error: {}", error);
            continue;
        }

        // Verify size and channels
        if (i == 0)
        {
            size     = faceSize;
            channels = faceChannels;
        }
        else
        {
            if (size != faceSize || channels != faceChannels)
                throw std::runtime_error("found mismatching face size or channels");
        }

        // Get api enum for channels
        unsigned apiChannelsEnum = [=]() {
            if (channels == 1) return GL_RED;
            if (channels == 2) return GL_RG;
            if (channels == 3) return GL_RGB;
            if (channels == 4) return GL_RGBA;
        }();

#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        unsigned cubemapSlot = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        glTexImage2D(cubemapSlot, 0, apiChannelsEnum, (unsigned)size.x, (unsigned)size.y, 0, apiChannelsEnum, GL_UNSIGNED_BYTE, pixels);

        stbi_image_free(pixels);
    }
}

dk::gfx::Cubemap::Cubemap(const std::array<std::filesystem::path, 6>& paths)
    : Texture(api::TextureType::Cubemap, Channels::R)
{
    // Emplace initializer which parses image data from files upon resource initialization
    m_initializer.emplace([paths=paths](unsigned handle, Texture* texture) {
        int        channels = 1;
        glm::ivec2 size = glm::ivec2(0, 0);
        loadCubemapFromFile(paths, handle, size, channels);
        ((Cubemap*)texture)->m_channels = channelCountToEnum(channels);
        ((Cubemap*)texture)->m_size = size;
    });
}

void dk::gfx::Cubemap::setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level)
{
    throw std::runtime_error("cubemap cannot be attached as framebuffer output");
}

bool isMipMapMinFilter(const dk::gfx::properties::min_filter& min_filter)
{
    if (min_filter == dk::gfx::properties::min_filter::linear_mipmap_linear) return true;
    if (min_filter == dk::gfx::properties::min_filter::linear_mipmap_nearest) return true;
    if (min_filter == dk::gfx::properties::min_filter::nearest_mipmap_linear) return true;
    if (min_filter == dk::gfx::properties::min_filter::nearest_mipmap_nearest) return true;
    return false;
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
    m_slotCount = dk::io::GlobalState::hardware().glMaxTextureImageUnits;
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
        m_textures[i].value().get().bindToUnit(i);
    }
}

template <>
void details::common::setProperty(dk::gfx::Texture& texture, const dk::gfx::properties::min_filter& min_filter)
{
    glTexParameteri(dk::gfx::api::toUnderlying(texture.m_type), GL_TEXTURE_MIN_FILTER, details::gfx::toUnderlying(min_filter));
    if (texture.handle() && isMipMapMinFilter(min_filter)) {
        glGenerateMipmap(dk::gfx::api::toUnderlying(texture.m_type));
    }
}

template <>
void details::common::setProperty(dk::gfx::Texture& texture, const dk::gfx::properties::mag_filter& mag_filter)
{
    glTexParameteri(dk::gfx::api::toUnderlying(texture.m_type), GL_TEXTURE_MAG_FILTER, details::gfx::toUnderlying(mag_filter));
}

int details::gfx::toUnderlying(dk::gfx::properties::min_filter min_filter)
{
    switch (min_filter)
    {
    case dk::gfx::properties::min_filter::nearest                : return GL_NEAREST;
    case dk::gfx::properties::min_filter::linear                 : return GL_LINEAR;
    case dk::gfx::properties::min_filter::linear_mipmap_linear   : return GL_LINEAR_MIPMAP_LINEAR;
    case dk::gfx::properties::min_filter::linear_mipmap_nearest  : return GL_LINEAR_MIPMAP_NEAREST;
    case dk::gfx::properties::min_filter::nearest_mipmap_linear  : return GL_NEAREST_MIPMAP_LINEAR;
    case dk::gfx::properties::min_filter::nearest_mipmap_nearest : return GL_NEAREST_MIPMAP_NEAREST;
    default: return 0;
    }
}

int details::gfx::toUnderlying(dk::gfx::properties::mag_filter mag_filter)
{
    return toUnderlying(dk::gfx::properties::min_filter(mag_filter));
}
