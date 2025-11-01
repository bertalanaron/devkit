#include <devkit/gfx/texture.h>
#include <devkit/gfx/frame_buffer.h>
#include "context.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <GL/glew.h>

#include <imgui.h>

namespace dk::gfx::api {

constexpr GLenum toUnderlying(Channels channels)
{
    switch (channels) {
    case Channels::R:             return GL_RED;
    case Channels::RG:            return GL_RG;
    case Channels::RGB:           return GL_RGB;
    case Channels::RGBA:          return GL_RGBA;
    case Channels::BGR:           return GL_BGR;
    case Channels::BGRA:          return GL_BGRA;
    case Channels::Depth:         return GL_DEPTH_COMPONENT;
    case Channels::Stencil:       return GL_STENCIL_INDEX;
    case Channels::DepthStencil:  return GL_DEPTH_STENCIL;
    }
    throw std::invalid_argument("Unknown channel layout");
}

constexpr GLenum toUnderlying(Format format)
{
    switch (format) {
    case Format::Unsigned8:  return GL_UNSIGNED_BYTE;
    case Format::Signed8:    return GL_BYTE;
    case Format::Unsigned16: return GL_UNSIGNED_SHORT;
    case Format::Signed16:   return GL_SHORT;
    case Format::Unsigned32: return GL_UNSIGNED_INT;
    case Format::Signed32:   return GL_INT;
    case Format::Float16:    return GL_HALF_FLOAT;
    case Format::Float32:    return GL_FLOAT;
    case Format::Packed10A2: return GL_UNSIGNED_INT_2_10_10_10_REV;
    case Format::Packed11F_10F: return GL_UNSIGNED_INT_10F_11F_11F_REV;
    case Format::Depth16:    return GL_UNSIGNED_SHORT;
    case Format::Depth24:    return GL_UNSIGNED_INT;
    case Format::Depth32F:   return GL_FLOAT;
    case Format::Depth24Stencil8:  return GL_UNSIGNED_INT_24_8;
    case Format::Depth32FStencil8: return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
    default: break;
    }
    throw std::invalid_argument("Unknown format data type");
}

unsigned internalFormat(Channels channels, Format format)
{
    using C = Channels;
    using F = Format;

    switch (channels) {
    case C::R:
        switch (format) {
        case F::Unsigned8:  return GL_R8;
        case F::Signed8:    return GL_R8_SNORM;
        case F::Unsigned16: return GL_R16;
        case F::Signed16:   return GL_R16_SNORM;
        case F::Float16:    return GL_R16F;
        case F::Float32:    return GL_R32F;
        case F::Unsigned32: return GL_R32UI;
        case F::Signed32:   return GL_R32I;
        default: break;
        }
        break;
    case C::RG:
        switch (format) {
        case F::Unsigned8:  return GL_RG8;
        case F::Signed8:    return GL_RG8_SNORM;
        case F::Unsigned16: return GL_RG16;
        case F::Signed16:   return GL_RG16_SNORM;
        case F::Float16:    return GL_RG16F;
        case F::Float32:    return GL_RG32F;
        case F::Unsigned32: return GL_RG32UI;
        case F::Signed32:   return GL_RG32I;
        default: break;
        }
        break;
    case C::RGB:
        switch (format) {
        case F::Unsigned8:  return GL_RGB8;
        case F::Signed8:    return GL_RGB8_SNORM;
        case F::Unsigned16: return GL_RGB16;
        case F::Signed16:   return GL_RGB16_SNORM;
        case F::Float16:    return GL_RGB16F;
        case F::Float32:    return GL_RGB32F;
        case F::Unsigned32: return GL_RGB32UI;
        case F::Signed32:   return GL_RGB32I;
        case F::SRGB8:      return GL_SRGB8;
        case F::Packed11F_10F: return GL_R11F_G11F_B10F;
        default: break;
        }
        break;
    case C::RGBA:
        switch (format) {
        case F::Unsigned8:  return GL_RGBA8;
        case F::Signed8:    return GL_RGBA8_SNORM;
        case F::Unsigned16: return GL_RGBA16;
        case F::Signed16:   return GL_RGBA16_SNORM;
        case F::Float16:    return GL_RGBA16F;
        case F::Float32:    return GL_RGBA32F;
        case F::Unsigned32: return GL_RGBA32UI;
        case F::Signed32:   return GL_RGBA32I;
        case F::SRGB8:      return GL_SRGB8_ALPHA8;
        case F::Packed10A2: return GL_RGB10_A2;
        default: break;
        }
        break;
    case C::Depth:
        switch (format) {
        case F::Depth16:    return GL_DEPTH_COMPONENT16;
        case F::Depth24:    return GL_DEPTH_COMPONENT24;
        case F::Depth32F:   return GL_DEPTH_COMPONENT32F;
        default: break;
        }
        break;
    case C::Stencil:
        if (format == F::Unsigned8)
            return GL_STENCIL_INDEX8;
        break;
    case C::DepthStencil:
        switch (format) {
        case F::Depth24Stencil8:   return GL_DEPTH24_STENCIL8;
        case F::Depth32FStencil8:  return GL_DEPTH32F_STENCIL8;
        default: break;
        }
        break;
    default:
        break;
    }

    throw std::invalid_argument("Unsupported Channels + Format combination");
}

}

dk::gfx::Channels dk::gfx::channelsFromCount(int channels)
{
    if (channels == 1) return Channels::R;
    if (channels == 2) return Channels::RG;
    if (channels == 3) return Channels::RGB;
    if (channels == 4) return Channels::RGBA;
    throw std::invalid_argument("Unknown channel count");
}

int dk::gfx::channelCount(dk::gfx::Channels channels)
{
    switch (channels) {
    case Channels::R:             return 1;
    case Channels::RG:            return 2;
    case Channels::RGB:           return 3;
    case Channels::RGBA:          return 4;
    case Channels::BGR:           return 3;
    case Channels::BGRA:          return 4;
    case Channels::Depth:         return 1;
    case Channels::Stencil:       return 1;
    case Channels::DepthStencil:  return 2;
    }
    throw std::invalid_argument("Unknown channel layout");
}

unsigned dk::gfx::Texture::handle()
{
    if (m_type == api::TextureType::Unset)
        throw std::runtime_error(
            "cannot get handle of texture with unset type "
            "(don't use the default constructor of any texture type)");

    updateOrInitializeAndBind();

    // Return api handle
    return m_apiHandle.handle();
}

void dk::gfx::Texture::updateOrInitializeAndBind()
{
    m_apiHandle.bind(m_type);

    // Call initializer with handle
    if (m_initializer.has_value())
    {
        m_initializer.value()(m_apiHandle.handle(), this);
        m_initializer.reset();

        // Call property setters
        config.for_each([&](const auto& prop) {
            setTextureProperty(*this, prop);
        });
        config.reset_dirty();

        return;
    }
    
    // Call property setters
    config.for_each([&](const auto& prop) {
        if (!config.dirty(prop))
            return;
        spdlog::debug("Texture.{}={}", Config::property_name(prop), std::format("{}", prop));
        setTextureProperty(*this, prop);
    });
    config.reset_dirty();
}

void dk::gfx::Texture::bindToUnit(unsigned unit)
{
    // Attach to texture unit
    glActiveTexture(GL_TEXTURE0 + unit);
    updateOrInitializeAndBind();
}

dk::gfx::Texture2D::Texture2D(const glm::ivec2& size, Channels channels, Format format)
    : Texture(api::TextureType::Texture2D, channels, format)
    , m_size(size)
{
    if (std::max(size.x, size.y) >= dk::io::GlobalState::hardware().glMaxTextureSize)
        throw std::runtime_error("texture size exceeds hardware maximum");

    // Emplace initializer which sets up texture buffer
    m_initializer.emplace([=](unsigned handle, Texture* texture) {
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, api::internalFormat(channels, format), (unsigned)size.x, (unsigned)size.y, 0, 
            api::toUnderlying(channels), api::toUnderlying(format), nullptr);
    });
}

stbi_uc* loadTexture2DFromFile(const std::filesystem::path& path, glm::ivec2& size, dk::gfx::Channels& channels)
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

    channels = dk::gfx::channelsFromCount(channelCount);
    return pixels;
}

dk::gfx::Texture2D::Texture2D(const std::filesystem::path& path)
    : Texture(api::TextureType::Texture2D, Channels::R, Format::Unsigned8)
{
    auto pixels = loadTexture2DFromFile(path, m_size, m_channels);

    // Emplace initializer which parses image data from file upon resource initialization
    if (pixels)
        m_initializer.emplace([path=path,channels=api::toUnderlying(m_channels),pixels](unsigned handle, Texture* tex) {
            Texture2D* texture = dynamic_cast<Texture2D*>(tex);
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
            glTexImage2D(GL_TEXTURE_2D, 0, channels, (unsigned)texture->m_size.x, (unsigned)texture->m_size.y, 0, channels, GL_UNSIGNED_BYTE, pixels);

            stbi_image_free(pixels);
        });
}

dk::gfx::Texture2D dk::gfx::Texture2D::load(const std::string& path)
{
    Texture2D texture(path);
    return texture;
}

void dk::gfx::Texture2D::resize(const glm::ivec2& size)
{
    if (size == m_size)
        return;

    m_size = size;
    m_initializer.emplace([=](unsigned handle, Texture* tex) {
        auto texture = dynamic_cast<Texture2D*>(tex);

#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, api::internalFormat(texture->m_channels, texture->m_format), (unsigned)size.x, (unsigned)size.y, 
            0, api::toUnderlying(texture->m_channels), api::toUnderlying(texture->m_format), nullptr);
    });
}

void dk::gfx::Texture2D::setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level)
{
    updateOrInitializeAndBind();
    if (attachment == api::Attachment::Color0)
        glFramebufferTexture2D(GL_FRAMEBUFFER, api::toUnderlying(attachment) + colorIndex, GL_TEXTURE_2D, m_apiHandle.handle(), level);
    else
        glFramebufferTexture2D(GL_FRAMEBUFFER, api::toUnderlying(attachment), GL_TEXTURE_2D, m_apiHandle.handle(), level);
}

dk::gfx::MultisampledTexture2D::MultisampledTexture2D(const glm::ivec2& size, unsigned samples, Channels channels, Format format)
    : Texture(api::TextureType::MultisampledTexture2D, channels, format)
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
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, api::internalFormat(channels, format), (unsigned)size.x, (unsigned)size.y, GL_TRUE);
    });
}

void dk::gfx::MultisampledTexture2D::resize(const glm::ivec2& size)
{
    if (size == m_size)
        return;

    m_size = size;
    m_initializer.emplace([=](unsigned handle, Texture* texture) {
        auto tex = dynamic_cast<MultisampledTexture2D*>(texture);

#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, tex->m_samples, api::internalFormat(tex->m_channels, tex->m_format), 
            (unsigned)size.x, (unsigned)size.y, GL_TRUE);
    });
}

void dk::gfx::MultisampledTexture2D::setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level)
{
    updateOrInitializeAndBind();
    if (attachment == api::Attachment::Color0)
        glFramebufferTexture2D(GL_FRAMEBUFFER, api::toUnderlying(attachment) + colorIndex, GL_TEXTURE_2D_MULTISAMPLE, m_apiHandle.handle(), level);
    else
        glFramebufferTexture2D(GL_FRAMEBUFFER, api::toUnderlying(attachment), GL_TEXTURE_2D_MULTISAMPLE, m_apiHandle.handle(), level);
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
    : Texture(api::TextureType::Cubemap, Channels::R, Format::Unsigned8)
{
    // Emplace initializer which parses image data from files upon resource initialization
    m_initializer.emplace([paths=paths](unsigned handle, Texture* texture) {
        int        channels = 1;
        glm::ivec2 size = glm::ivec2(0, 0);
        loadCubemapFromFile(paths, handle, size, channels);
        ((Cubemap*)texture)->m_channels = channelsFromCount(channels);
        ((Cubemap*)texture)->m_size = size;
    });
}

dk::gfx::Texture2DArray::Texture2DArray(const glm::ivec2& size, int layers, Channels channels, Format format)
    : Texture(api::TextureType::Texture2DArray, channels, format)
    , m_size(size)
    , m_layers(layers)
{ 
    // Validate layer count
    if (layers < 1 || layers > io::GlobalState::hardware().glMaxTextureLayers)
        throw std::out_of_range("Layer count is out of range");

    // Emplace initializer which parses image data from files upon resource initialization
    m_initializer.emplace([=](unsigned handle, Texture* tex) {
        Texture2DArray* texture = dynamic_cast<Texture2DArray*>(tex);
        const auto size     = texture->m_size;
        const auto channels = texture->m_channels;
        const auto format   = texture->m_format;

        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, api::internalFormat(channels, format), size.x, size.y, texture->layers(), 
            0, api::toUnderlying(channels), api::toUnderlying(format), nullptr);
    });
}

std::vector<stbi_uc*> loadTexturesFromFiles(const std::vector<std::filesystem::path>& files, dk::gfx::Channels& channels, glm::ivec2& size)
{
    std::vector<stbi_uc*> results;
    int channelCount;
    for (int i = 0; i < files.size(); ++i)
    {
        glm::ivec2 currSize;
        int        currChannels;
        const auto pathStr = files.at(i).string();
        results.push_back(stbi_load(pathStr.c_str(), &currSize.x, &currSize.y, &currChannels, 0));

        if (i == 0)
        {
            size         = currSize;
            channelCount = currChannels;
            continue;
        }

        if (currSize != size)
            throw std::runtime_error("Files have mismatching sizes");
        if (currChannels != channelCount)
            throw std::runtime_error("Files have mismatching channels");
    }

    channels = dk::gfx::channelsFromCount(channelCount);
    return results;
}

dk::gfx::Texture2DArray::Texture2DArray(const std::vector<std::filesystem::path>& files)
    : Texture(api::TextureType::Texture2DArray, Channels::R, Format::Unsigned8)
{
    auto images = loadTexturesFromFiles(files, m_channels, m_size);
    m_layers = images.size();

    // Emplace initializer which uploads pixels to the gpu and frees pixel buffers
    m_initializer.emplace([images=std::move(images)](unsigned handle, Texture* tex) {
        Texture2DArray* texture = dynamic_cast<Texture2DArray*>(tex);
        const auto size     = texture->m_size;
        const auto channels = texture->m_channels;
        const auto format   = texture->m_format;

        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, api::internalFormat(channels, format), size.x, size.y, texture->layers(), 
            0, api::toUnderlying(channels), api::toUnderlying(format), nullptr);

        for (int i = 0; i < images.size(); ++i)
        {
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, size.x, size.y, 1,  
                api::toUnderlying(channels), GL_UNSIGNED_BYTE, images.at(i));

            stbi_image_free(images.at(i));
        }
    });
}

void dk::gfx::Texture2DArray::resize(const glm::ivec2& size)
{
    if (size == m_size)
        return;

    m_size = size;
    m_initializer.emplace([](unsigned handle, Texture* tex) {
        Texture2DArray* texture = dynamic_cast<Texture2DArray*>(tex);

        const auto size     = texture->m_size;
        const auto channels = texture->m_channels;
        const auto format   = texture->m_format;

        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, api::internalFormat(channels, format), size.x, size.y, texture->layers(), 
            0, api::toUnderlying(channels), api::toUnderlying(format), nullptr);
    });
}

dk::gfx::Texture2DArray::Layer dk::gfx::Texture2DArray::operator[](int layer)
{ 
    if (layer < 0 || layer >= m_layers)
        throw std::out_of_range("Layer index is out of range");
    return Layer(*this, layer);
}

void dk::gfx::Texture2DArray::Layer::setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level)
{
    m_array->updateOrInitializeAndBind();
    if (attachment == api::Attachment::Color0)
        glFramebufferTextureLayer(GL_FRAMEBUFFER, api::toUnderlying(attachment) + colorIndex, m_array->m_apiHandle.handle(), level, m_layerIdx);
    else
        glFramebufferTextureLayer(GL_FRAMEBUFFER, api::toUnderlying(attachment), m_array->m_apiHandle.handle(), level, m_layerIdx);
}

dk::gfx::RenderBuffer::RenderBuffer(const glm::ivec2& size, Channels channels, Format format, unsigned samples)
    : m_valid(true)
    , m_size(size)
    , m_samples(samples)
    , m_channels(channels)
    , m_format(format)
{
    m_initializer = [=](unsigned handle, RenderBuffer* renderbuffer) {
        if (m_samples == 1)
            glRenderbufferStorage(GL_RENDERBUFFER, api::internalFormat(renderbuffer->m_channels, renderbuffer->m_format), 
                renderbuffer->size().x, renderbuffer->size().y);
        else
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, renderbuffer->m_samples, 
                api::internalFormat(renderbuffer->m_channels, renderbuffer->m_format), renderbuffer->size().x, renderbuffer->size().y);
    };
}

void dk::gfx::RenderBuffer::resize(const glm::ivec2& size) {
    if (size == m_size)
        return;

    m_size = size;
    m_initializer = [=](unsigned handle, RenderBuffer* renderbuffer) {
        if (m_samples == 1)
            glRenderbufferStorage(GL_RENDERBUFFER, api::internalFormat(renderbuffer->m_channels, renderbuffer->m_format), 
                renderbuffer->size().x, renderbuffer->size().y);
        else
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, renderbuffer->m_samples, 
                api::internalFormat(renderbuffer->m_channels, renderbuffer->m_format), renderbuffer->size().x, renderbuffer->size().y);
    };
}

void dk::gfx::RenderBuffer::setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level)
{
    updateOrInitializeAndBind();
    if (attachment == api::Attachment::Color0)
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, api::toUnderlying(attachment) + colorIndex, GL_RENDERBUFFER, m_apiHandle.handle());
    else
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, api::toUnderlying(attachment), GL_RENDERBUFFER, m_apiHandle.handle());
}

void dk::gfx::RenderBuffer::updateOrInitializeAndBind()
{
    m_apiHandle.bind();

    // Call initializer with handle
    if (m_initializer.has_value())
    {
        m_initializer.value()(m_apiHandle.handle(), this);
        m_initializer.reset();
        return;
    }
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

template<>
void dk::gfx::setTextureProperty(dk::gfx::Texture& texture, const Texture::MinFilter& minFilter)
{
    const auto [underlying, isMipmap] = [&] {
        switch (minFilter)
        {
        case Texture::MinFilter::Nearest             : return std::make_pair(GL_NEAREST               , false);
        case Texture::MinFilter::Linear              : return std::make_pair(GL_LINEAR                , false);
        case Texture::MinFilter::LinearMipmapLinear  : return std::make_pair(GL_LINEAR_MIPMAP_LINEAR  , true);
        case Texture::MinFilter::LinearMipmapNearest : return std::make_pair(GL_LINEAR_MIPMAP_NEAREST , true);
        case Texture::MinFilter::NearestMipmapLinear : return std::make_pair(GL_NEAREST_MIPMAP_LINEAR , true);
        case Texture::MinFilter::NearestMipmapNearest: return std::make_pair(GL_NEAREST_MIPMAP_NEAREST, true);
        default: throw std::runtime_error("unknown min filter value");
        }
    }();

    glTexParameteri(dk::gfx::api::toUnderlying(texture.m_type), GL_TEXTURE_MIN_FILTER, underlying);
    if (isMipmap)
        glGenerateMipmap(dk::gfx::api::toUnderlying(texture.m_type));
}

template <>
void dk::gfx::setTextureProperty(dk::gfx::Texture& texture, const Texture::MagFilter& magFilter)
{
    const auto underlying = [&] {
        switch (magFilter)
        {
        case Texture::MagFilter::Nearest : return GL_NEAREST;
        case Texture::MagFilter::Linear  : return GL_LINEAR;
        default: throw std::runtime_error("unknown min filter value");
        }
    }();

    glTexParameteri(dk::gfx::api::toUnderlying(texture.m_type), GL_TEXTURE_MAG_FILTER, underlying);
}
