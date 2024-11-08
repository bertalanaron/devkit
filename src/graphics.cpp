#include "devkit/graphics.h"
#include "devkit/log.h"
#include "graphics_includes.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

using namespace NS_DEVKIT;

struct OpenGLFrameBufferImpl {
    GLuint        frameBufferId  = 0;
    GLuint        renderBufferId = 0;

    SDL_GLContext glContext      = nullptr;

    OpenGLFrameBufferImpl() {
        glContext = currentGlContext();
        glGenFramebuffers (1, &frameBufferId );
        glGenRenderbuffers(1, &renderBufferId);
    }

    ~OpenGLFrameBufferImpl() {
        glDeleteFramebuffers (1, &frameBufferId );
        glDeleteRenderbuffers(1, &renderBufferId);
    }
};

class FrameBuffer::Impl {
public:
    Texture&                m_texture;
    OpenGLFrameBufferImpl   m_glFrameBuffer;
    GLint                   m_fboIdBeforeBind = 0;

    Properties              m_properties;


    Impl(Texture& texture, Properties properties) 
        : m_texture(texture)
        , m_glFrameBuffer()
        , m_properties(properties)
    {
        // Save current fbo, rbo, and texture
        GLint currentFbo = 0, currentRbo = 0, currentTexture = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFbo);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &currentRbo);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);

        // Attach texture to frame buffer
        glBindTexture(GL_TEXTURE_2D, texture.id());
        glBindFramebuffer(GL_FRAMEBUFFER, m_glFrameBuffer.frameBufferId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.id(), 0);

        // Setup render buffer
        glBindRenderbuffer(GL_RENDERBUFFER, m_glFrameBuffer.renderBufferId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, texture.size().x, texture.size().y);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_glFrameBuffer.renderBufferId);

        // Check for errors
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            ERR("Framebuffer: Framebuffer is not complete!");
        else
            DBG("Framebuffer: Created successfully {{{},{}}}", m_glFrameBuffer.frameBufferId, m_glFrameBuffer.renderBufferId);

        // Bind originaly bound objects
        glBindFramebuffer(GL_FRAMEBUFFER, currentFbo);
        glBindTexture(GL_TEXTURE_2D, currentTexture);
        glBindRenderbuffer(GL_RENDERBUFFER, currentRbo);
    }

    void bind() {
        if (m_glFrameBuffer.glContext != currentGlContext())
            throw std::runtime_error("Framebuffer was created in a different context");

        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_fboIdBeforeBind);
        glBindFramebuffer(GL_FRAMEBUFFER, m_glFrameBuffer.frameBufferId);
    }

    void unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboIdBeforeBind);
    }
};

FrameBuffer::FrameBuffer(Texture& texture)
    : m_impl(std::make_unique<Impl>(texture, std::move(Properties{})))
{ }

FrameBuffer::FrameBuffer(Texture & texture, Properties properties)
    : m_impl(std::make_unique<Impl>(texture, std::move(properties)))
{ }

bool FrameBuffer::beginFrame()
{
    m_impl->bind();

    // Set viewport
    const auto size = m_impl->m_texture.size();
    pushViewport({ 0, 0 }, size);

    // Clear buffer
    const glm::u8vec4 clearColor = m_impl->m_properties.clearColor;
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return true;
}

void FrameBuffer::endFrame()
{
    m_impl->unbind();
    popViewport();
}

FrameBuffer::~FrameBuffer() { }

struct OpenGLTextureImpl {
    GLuint          id = 0;

    SDL_GLContext   glContext = nullptr;

    OpenGLTextureImpl() { 
        glContext = currentGlContext();
        glGenTextures(1, &id); 
    }
    ~OpenGLTextureImpl() { glDeleteTextures(1, &id); }
};

class Texture::Impl {
public:
    Properties          m_properties;
    OpenGLTextureImpl   m_openglImpl;

    Impl(Properties properties)
        : m_properties(std::move(properties))
        , m_openglImpl()
    {
        allocate();
    }

    Impl(Properties properties, unsigned char* pixels)
        : m_properties(std::move(properties))
        , m_openglImpl()
    {
        allocate(pixels);
    }

    void bind() {
        if (m_openglImpl.glContext != currentGlContext())
            throw std::runtime_error("Texture was created in a different gl context.");

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, m_openglImpl.id);

        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)m_properties.minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)m_properties.magFilter);
    }

    void allocate(unsigned char* pixels = nullptr) {
        // Bind texture
        GLint currentTexture = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);
        glBindTexture(GL_TEXTURE_2D, m_openglImpl.id);

        // Allocate texture
        auto size = m_properties.size;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, currentTexture);
    }
};

Texture::Texture()
    : m_impl(std::make_unique<Impl>(Properties{}))
{ }

Texture::Texture(Texture::Properties properties)
    : m_impl(std::make_unique<Impl>(std::move(properties)))
{ }

Texture::Texture(unsigned char* pixels, glm::i32vec2 size)
    : m_impl(std::make_unique<Impl>(std::move(Properties{ .size = size }), pixels))
{ }

uint32_t Texture::id() const
{
    return m_impl->m_openglImpl.id;
}

glm::u32vec2 Texture::size() const
{
    return m_impl->m_properties.size;
}

void Texture::bind() {
    m_impl->bind();
}

unsigned char* Texture::getPixelBuffer() {
    const auto& size = m_impl->m_properties.size;
    GLubyte* pixels = new GLubyte[size.x * size.y * 4];
    
    bind();
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    
    return pixels;
}

void Texture::save(const wchar_t* path)
{
    auto pixels = getPixelBuffer();
    const auto& size = m_impl->m_properties.size;
    stbi_write_png(utf8(path).c_str(), size.x, size.y, 4, pixels, size.x * 4);
    delete[] pixels;
}

Texture Texture::load(const wchar_t* path) 
{
    int width, height, channels;
    unsigned char* pixels = stbi_load(utf8(path).c_str(), &width, &height, &channels, 0);
    DBG("Loaded texture from {}", utf8(path));
    return Texture(pixels, { width, height });
}

Texture::~Texture() { }
