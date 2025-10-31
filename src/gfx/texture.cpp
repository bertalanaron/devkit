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

void dk::gfx::Texture1D::setAsTarget(api::Attachment attachment, unsigned colorIndex, unsigned level)
{
    updateOrInitializeAndBind();
    if (attachment == api::Attachment::Color0)
        glFramebufferTexture1D(GL_FRAMEBUFFER, api::toUnderlying(attachment) + colorIndex, GL_TEXTURE_1D, m_apiHandle.handle(), level);
    else
        glFramebufferTexture1D(GL_FRAMEBUFFER, api::toUnderlying(attachment), GL_TEXTURE_1D, m_apiHandle.handle(), level);
}

dk::gfx::Texture2D::Texture2D(const glm::ivec2& size, Channels channels)
    : Texture(api::TextureType::Texture2D, channels)
    , m_size(size)
{
    if (std::max(size.x, size.y) >= dk::io::GlobalState::hardware().glMaxTextureSize)
        throw std::runtime_error("texture size exceeds hardware maximum");

    // Emplace initializer which sets up texture buffer
    m_initializer.emplace([=](unsigned handle, Texture* texture) {
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, api::internalFormat(channels), (unsigned)size.x, (unsigned)size.y, 0, api::toUnderlying(channels), GL_UNSIGNED_BYTE, nullptr);
    });
}

dk::gfx::Channels channelCountToEnum(int channels)
{
    if (channels == 1) return dk::gfx::Channels::R;
    if (channels == 2) return dk::gfx::Channels::RG;
    if (channels == 3) return dk::gfx::Channels::RGB;
    if (channels == 4) return dk::gfx::Channels::RGBA;
    throw std::runtime_error("invalid channel count");
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

    channels = channelCountToEnum(channelCount);
    return pixels;
}

dk::gfx::Texture2D::Texture2D(const std::filesystem::path& path)
    : Texture(api::TextureType::Texture2D, Channels::R)
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

dk::gfx::Texture2D dk::gfx::Texture2D::loadFromFileAndInitialize(const std::string& path)
{
    Texture2D texture(path);
    //texture.handle();
    return texture;
}

void dk::gfx::Texture2D::resize(const glm::ivec2& size)
{
    if (size == m_size)
        return;

    m_size = size;
    m_initializer.emplace([channels=m_channels,size=size](unsigned handle, Texture* texture) {
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, api::internalFormat(channels), (unsigned)size.x, (unsigned)size.y, 0, api::toUnderlying(channels), GL_UNSIGNED_BYTE, nullptr);
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

void dk::gfx::MultisampledTexture2D::resize(const glm::ivec2& size)
{
    if (size == m_size)
        return;

    m_size = size;
    m_initializer.emplace([channels=m_channels,size=size,samples=m_samples](unsigned handle, Texture* texture) {
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, api::internalFormat(channels), (unsigned)size.x, (unsigned)size.y, GL_TRUE);
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

dk::gfx::Texture2DArray::Texture2DArray(const glm::ivec2& size, int layers, Channels channels)
    : Texture(api::TextureType::Texture2DArray, channels)
    , m_size(size)
    , m_layers(layers)
{ 
    // Validate layer count
    if (layers < 1 || layers > io::GlobalState::hardware().glMaxTextureLayers)
        throw std::out_of_range("Layer count is out of range");

    // Emplace initializer which parses image data from files upon resource initialization
    m_initializer.emplace([=](unsigned handle, Texture* texture) {
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, api::internalFormat(channels), size.x, size.y, layers);
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

    channels = channelCountToEnum(channelCount);
    return results;
}

dk::gfx::Texture2DArray::Texture2DArray(const std::vector<std::filesystem::path>& files)
    : Texture(api::TextureType::Texture2DArray, Channels::R)
{
    auto images = loadTexturesFromFiles(files, m_channels, m_size);
    m_layers = images.size();

    // Emplace initializer which uploads pixels to the gpu and frees pixel buffers
    m_initializer.emplace([images=std::move(images)](unsigned handle, Texture* tex) {
        Texture2DArray* texture = dynamic_cast<Texture2DArray*>(tex);

        const auto size     = texture->m_size;
        const auto channels = texture->m_channels;
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, api::internalFormat(channels), size.x, size.y, texture->layers());

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
    m_initializer.emplace([channels=m_channels,size=size](unsigned handle, Texture* texture) {
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, api::internalFormat(channels), size.x, size.y, dynamic_cast<Texture2DArray*>(texture)->layers());
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

dk::gfx::RenderBuffer::RenderBuffer(const glm::ivec2& size, Channels channels, unsigned samples)
    : m_valid(true)
    , m_size(size)
    , m_samples(samples)
    , m_channels(channels)
{
    m_initializer = [=](unsigned handle, RenderBuffer* renderbuffer) {
        if (m_samples == 1)
            glRenderbufferStorage(GL_RENDERBUFFER, api::internalFormat(renderbuffer->m_channels), 
                renderbuffer->size().x, renderbuffer->size().y);
        else
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, renderbuffer->m_samples, api::internalFormat(renderbuffer->m_channels), 
                renderbuffer->size().x, renderbuffer->size().y);
    };
}

void dk::gfx::RenderBuffer::resize(const glm::ivec2& size) {
    if (size == m_size)
        return;

    m_size = size;
    m_initializer = [=](unsigned handle, RenderBuffer* renderbuffer) {
        if (m_samples == 1)
            glRenderbufferStorage(GL_RENDERBUFFER, api::internalFormat(renderbuffer->m_channels), 
                renderbuffer->size().x, renderbuffer->size().y);
        else
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, renderbuffer->m_samples, api::internalFormat(renderbuffer->m_channels), 
                renderbuffer->size().x, renderbuffer->size().y);
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
