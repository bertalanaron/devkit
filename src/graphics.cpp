#include <algorithm>
#include <fstream>

#include "devkit/graphics.h"
#include "devkit/log.h"
#include "graphics_includes.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

using namespace NS_DEVKIT;

struct OpenGLVertexBufferImpl {
    GLuint id = 0;
    GLuint vao = 0;

    OpenGLVertexBufferImpl() 
    { 
        glGenBuffers(1, &id); 

        GLint currentVAO;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
        if (0 != currentVAO)
            vao = currentVAO;
        else
            glGenVertexArrays(1, &vao);
    }

    void bind() 
    {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, id);
    }
    
    ~OpenGLVertexBufferImpl() { glDeleteBuffers(1, &id); }
};

struct VertexBufferBase::Impl {
    OpenGLVertexBufferImpl m_glVertexBuffer;

    std::vector<GLType>    m_types;
    size_t                 m_vertexSize;
    std::vector<std::byte> m_data;
    size_t                 m_count;

    bool                   m_inited = false;
    bool                   m_changed = false;
    int	    	           m_minUpdatedIndex = 0;
    int	    	           m_maxUpdatedIndex = 0;

    Impl(size_t size, std::vector<GLType>&& types, uintptr_t data)
        : m_glVertexBuffer()
        , m_types(types)
        , m_vertexSize(0)
        , m_data()
        , m_count(size)
    {
        std::for_each(types.begin(), types.end(), [&](const GLType& type) { m_vertexSize += type.size; });
        m_data.resize(size * m_vertexSize);

        if (data)
            std::memcpy(m_data.data(), reinterpret_cast<std::byte*>(data), m_data.size());

        m_minUpdatedIndex = std::numeric_limits<int>::max();
        m_maxUpdatedIndex = -1;
    }

    void bind() { 
        m_glVertexBuffer.bind();

        if (!m_inited) {
            m_inited = true;
            glBufferData(GL_ARRAY_BUFFER, m_vertexSize * m_count, m_data.data(), GL_STATIC_DRAW);
        }
        else
            update();

        enableVertexAttribPointers();
    }

    void vertexUpdated(size_t index) {
        if (m_minUpdatedIndex > (int)index)
            m_minUpdatedIndex = (int)index;
        if (m_maxUpdatedIndex < (int)index)
            m_maxUpdatedIndex = (int)index;
        m_changed = true;
    }

private:
    void enableVertexAttribPointers() const {
        std::size_t sizeSoFar = 0;
        GLuint index = 0;
        for (const auto& type : m_types) {
            // Enable attrib pointer
            glVertexAttribPointer(index, type.count, type.type, GL_FALSE, m_vertexSize, (void*)sizeSoFar);
            glEnableVertexAttribArray(index);

            // Move pointer
            sizeSoFar += type.size;
            ++index;
        }
    }

    void update() {
        if (!m_changed)
            return;

        int updateBeg = m_minUpdatedIndex * m_vertexSize;
        int updateSize = (m_maxUpdatedIndex - m_minUpdatedIndex + 1) * m_vertexSize;
        if (updateSize < 1) {
            m_changed = false;
            return;
        }

        void* dataBeg = &(m_data.data())[m_minUpdatedIndex * m_vertexSize];

        // Update vertices inide range
        glBindVertexArray(m_glVertexBuffer.vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer.id);
        glBufferSubData(GL_ARRAY_BUFFER, updateBeg, updateSize, dataBeg);

        m_changed = false;
        m_minUpdatedIndex = std::numeric_limits<int>::infinity();
        m_maxUpdatedIndex = -1;
    }
};

VertexBufferBase::VertexBufferBase(size_t size, std::vector<GLType>&& types, uintptr_t data)
    : m_impl(std::make_unique<Impl>(size, std::forward<std::vector<GLType>&&>(types), data))
{ }

void VertexBufferBase::bind() const {
    m_impl->bind();
}

void VertexBufferBase::draw(Primitive primitive) const {
    bind();
    glDrawArrays((GLenum)primitive, 0, m_impl->m_count);
}

void VertexBufferBase::draw(Primitive primitive, Shader& shader) const {
    shader.use();
    draw(primitive);
}

VertexBufferBase::~VertexBufferBase() { }

uintptr_t VertexBufferBase::at_impl(size_t index) const
{
    return reinterpret_cast<uintptr_t>(&m_impl->m_data.at(index * m_impl->m_vertexSize));
}


void writeShaderCompilationErrorInfo(unsigned int handle) {
    int logLen, written;
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);

    if (logLen > 0) {
        std::string log(logLen, '\0');
        glGetShaderInfoLog(handle, logLen, &written, &log[0]);
        ERR("Shader log:\n{}", log);
    }
}

bool checkShaderCompilation(unsigned id, const char* source) {
    int OK;
    glGetShaderiv(id, GL_COMPILE_STATUS, &OK);
    if (!OK) {
        if (source)
            ERR("{}", source);

        ERR("Failed to compile shader!");
        writeShaderCompilationErrorInfo(id);
        return false;
    }
    DBG("Compiled shader source");
    return true;
}

ShaderSource::ShaderSource(std::string&& source)
    : m_source(source)
    , m_updated(true)
{ }

ShaderSource ShaderSource::loadFromFile(const wchar_t* path)
{
    std::ifstream ifs(path);
    std::ostringstream contents;
    contents << ifs.rdbuf();
    ifs.close();
    
    return ShaderSource(std::move(contents.str()));
}

void ShaderSource::updateFromFile(const wchar_t* path)
{
    std::ifstream ifs(path);
    std::ostringstream contents;
    contents << ifs.rdbuf();
    ifs.close();

    m_source = contents.str();
    m_updated = true;
}

bool ShaderSource::compile(unsigned type, unsigned& id)
{
    m_updated = false;

    // Create shader
    id = glCreateShader(type);
    if (!id) {
        ERR("Error creating shader source");
        exit(1);
    }

    // Compile shader
    const char* source = m_source.c_str();
    glShaderSource(id, 1, (const GLchar**)&source, NULL);
    glCompileShader(id);

    // Return compilation status
    return checkShaderCompilation(id, m_source.c_str());
}

bool NS_DEVKIT::ShaderSource::updated() const
{
    return m_updated;
}

struct OpenGLShaderImpl {
    GLuint id;

    OpenGLShaderImpl() { id = glCreateProgram(); }

    ~OpenGLShaderImpl() { glDeleteProgram(id); }
};

bool checkShaderLinking(unsigned int program) {
    int OK;
    glGetProgramiv(program, GL_LINK_STATUS, &OK);
    if (!OK) {
        ERR("Failed to link shader program!");
        writeShaderCompilationErrorInfo(program);
        return false;
    }
    return true;
}

struct Shader::Impl {
    Shader&          m_shader;
    OpenGLShaderImpl m_glProgram;

    GLuint           m_vertexSourceId   = 0, m_prevVertexSourceId   = 0; 
    GLuint           m_fragmentSourceId = 0, m_prevFragmentSourceId = 0;
    GLuint           m_geometrySourceId = 0, m_prevGeometrySourceId = 0;

    template <typename T>
    class StaticUniformSource;

    template <typename T>
    class DynamicUniformSource;

    void insertOrUpdatedUniform(const std::string& name, std::unique_ptr<IUniformSource>&& uniform);
    
    Impl(Shader& shader) 
        : m_shader(shader) 
    { }

    void use() {
        compile();
        glUseProgram(m_glProgram.id);

        // Bind uniforms
        for (auto& [uniformName, uniformSource] : m_shader.m_uniforms) {
            uniformSource->set(m_shader, uniformName);
        }
    }

private:
    void compile() 
    {
        // Compile shader sources
        bool success = true;
        bool updatedAny = false;
        if (m_shader.m_vertexSource.updated()) {
            updatedAny = true;
            if (!m_shader.m_vertexSource.compile(GL_VERTEX_SHADER, m_vertexSourceId))
            { success = false; m_vertexSourceId = m_prevVertexSourceId; }
        }
        if (m_shader.m_fragmentSource.updated()) {
            updatedAny = true;
            if (!m_shader.m_fragmentSource.compile(GL_FRAGMENT_SHADER, m_fragmentSourceId))
            { success = false; m_fragmentSourceId = m_prevFragmentSourceId; }
        }
        if (m_shader.m_geometrySource.has_value()
            && m_shader.m_geometrySource.value().get().updated()) {
            updatedAny = true;
            if (!m_shader.m_geometrySource.value().get().compile(GL_GEOMETRY_SHADER, m_geometrySourceId))
            { success = false; m_geometrySourceId = m_prevGeometrySourceId; }
        }

        // Attach and link shaders if compilation succeded
        if (updatedAny && success)
            attachAndLinkShaders();
    }

    void attachAndLinkShaders() {
        // Detach previously attached shaders, if any
        if (m_prevVertexSourceId)
            glDetachShader(m_glProgram.id, m_prevVertexSourceId);
        if (m_prevFragmentSourceId)
            glDetachShader(m_glProgram.id, m_prevFragmentSourceId);
        if (m_prevGeometrySourceId 
            && m_shader.m_geometrySource.has_value())
            glDetachShader(m_glProgram.id, m_prevGeometrySourceId);
        m_prevVertexSourceId = m_vertexSourceId;
        m_prevFragmentSourceId = m_fragmentSourceId;
        m_prevGeometrySourceId = m_geometrySourceId;

        // Attach shaders
        glAttachShader(m_glProgram.id, m_vertexSourceId);
        glAttachShader(m_glProgram.id, m_fragmentSourceId);
        if (m_shader.m_geometrySource.has_value())
            glAttachShader(m_glProgram.id, m_geometrySourceId);

        // Connect the fragmentColor to the frame buffer memory
        glBindFragDataLocation(m_glProgram.id, 0, "outColor");

        // program packaging
        glLinkProgram(m_glProgram.id);
        if (!checkShaderLinking(m_glProgram.id))
            exit(1);
    }
};

Shader::Shader(ShaderSource& vertexSource, ShaderSource& fragmentSource, OptionalShaderRef geometrySource)
    : m_vertexSource(vertexSource)
    , m_fragmentSource(fragmentSource)
    , m_geometrySource(geometrySource)
    , m_impl(std::make_shared<Impl>(*this))
{ }

Shader::Shader(const Shader& other)
    : m_vertexSource(other.m_vertexSource)
    , m_fragmentSource(other.m_fragmentSource)
    , m_geometrySource(other.m_geometrySource)
    , m_impl(other.m_impl)
{
    for (const auto& [name, uniform] : other.m_uniforms) {
        m_uniforms.insert({ name, uniform->clone() });
    }
}

GLint getLocation(const Shader& shader, const std::string& name) {
    return glGetUniformLocation(shader.id(), name.c_str());
}

void setUniform(const Shader& shader, const std::string& name, const int& value) {
    glUniform1i(getLocation(shader, name), value);
}

void setUniform(const Shader& shader, const std::string& name, const float& value) {
    glUniform1f(getLocation(shader, name), value);
}

void setUniform(const Shader& shader, const std::string& name, const double& value) {
    glUniform1d(getLocation(shader, name), value);
}

void setUniform(const Shader& shader, const std::string& name, const glm::vec2& value) {
    glUniform2fv(getLocation(shader, name), 1, (float*)&value);
}

void setUniform(const Shader& shader, const std::string& name, const glm::vec3& value) {
    glUniform3fv(getLocation(shader, name), 1, (float*)&value);
}

void setUniform(const Shader& shader, const std::string& name, const glm::vec4& value) {
    glUniform4fv(getLocation(shader, name), 1, (float*)&value);
}

void setUniform(const Shader& shader, const std::string& name, const Texture& texture) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.id());
    glUniform1i(getLocation(shader, name), 0);
}

struct CameraValues {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 position;
    glm::vec3 direction;
};

void setUniform(const Shader& shader, const std::string& name, const CameraValues& camera) {
    glm::mat4 vp = camera.projection * camera.view;
    glUniformMatrix4fv(getLocation(shader, name + ".VP"), 1, GL_TRUE, (float*)&vp[0]);
    glUniform3fv(getLocation(shader, name + ".position"), 1, (float*)&camera.position);
    glUniform3fv(getLocation(shader, name + ".direction"), 1, (float*)&camera.direction);
}

template <typename T>
class Shader::Impl::StaticUniformSource : public Shader::IUniformSource {
public:
    StaticUniformSource(const T& data)
        : m_data(data)
    { }

    virtual void set(const Shader& shader, const std::string& name) const override {
        ::setUniform(shader, name, m_data);
    }

    virtual std::unique_ptr<Shader::IUniformSource> clone() const override {
        return std::make_unique<StaticUniformSource>(*this);
    }
private:
    T m_data;
};

void Shader::Impl::insertOrUpdatedUniform(const std::string& name, std::unique_ptr<IUniformSource>&& uniform) {
    auto it = m_shader.m_uniforms.find(name);
    if (it == m_shader.m_uniforms.end())
        m_shader.m_uniforms.insert({ name, std::move(uniform) });
    else
        m_shader.m_uniforms[name] = std::move(uniform);
}

void Shader::setUniform(const std::string& name, const int& value)
{
    m_impl->insertOrUpdatedUniform(name, std::make_unique<Impl::StaticUniformSource<int>>(value));
}

void Shader::setUniform(const std::string& name, const double& value)
{
    m_impl->insertOrUpdatedUniform(name, std::make_unique<Impl::StaticUniformSource<double>>(value));
}

void Shader::setUniform(const std::string& name, const float& value)
{
    m_impl->insertOrUpdatedUniform(name, std::make_unique<Impl::StaticUniformSource<float>>(value));
}

void Shader::setUniform(const std::string& name, const glm::vec2& value)
{
    m_impl->insertOrUpdatedUniform(name, std::make_unique<Impl::StaticUniformSource<glm::vec2>>(value));
}

void Shader::setUniform(const std::string& name, const glm::vec3& value)
{
    m_impl->insertOrUpdatedUniform(name, std::make_unique<Impl::StaticUniformSource<glm::vec3>>(value));
}

void Shader::setUniform(const std::string& name, const glm::vec4& value)
{
    m_impl->insertOrUpdatedUniform(name, std::make_unique<Impl::StaticUniformSource<glm::vec4>>(value));
}

void Shader::setUniform(const std::string& name, const Texture& value)
{
    m_impl->insertOrUpdatedUniform(name, std::make_unique<Impl::StaticUniformSource<Texture>>(value));
}

void Shader::setUniform(const std::string& name, CameraController& value) {
    CameraValues camera{ 
        value.viewMatrix(), 
        value.projectionMatrix(), 
        value.camera().position, 
        glm::normalize(value.camera().position - value.camera().lookat)
    };
    m_impl->insertOrUpdatedUniform(name, std::make_unique<Impl::StaticUniformSource<CameraValues>>(camera));
}

void Shader::use()
{
    m_impl->use();
}

unsigned Shader::id() const
{
    return m_impl->m_glProgram.id;
}

Shader::~Shader() { }


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
    pushViewportSize(size);

    // Clear buffer
    if (m_impl->m_properties.doClear) {
        const auto& clearColor = m_impl->m_properties.clearColor;
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    return true;
}

void FrameBuffer::endFrame()
{
    m_impl->unbind();
    popViewportSize();
}

glm::u32vec2 FrameBuffer::frameSize() const
{
    return m_impl->m_texture.size();
}

void* FrameBuffer::context()
{
    return m_impl->m_glFrameBuffer.glContext;
}

FrameBuffer::Properties& FrameBuffer::properties() {
    return m_impl->m_properties;
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
    stbi_set_flip_vertically_on_load(true);
    unsigned char* pixels = stbi_load(utf8(path).c_str(), &width, &height, &channels, 0);
    DBG("Loaded texture from {}", utf8(path));
    return Texture(pixels, { width, height });
}

void Texture::update(const wchar_t* path) 
{
    *this = load(path);
}

Texture::~Texture() { }


Camera& CameraController::camera() 
{
    m_accessed = true;
    return m_currentCamera;
}

const Camera& CameraController::camera() const 
{
    return m_currentCamera;
}

const glm::mat4& CameraController::viewMatrix() 
{
    update();
    return m_viewMatrix;
}

const glm::mat4& CameraController::projectionMatrix() 
{
    update();
    return m_projectionMatrix;
}

void CameraController::update() 
{
    if (!m_accessed || m_currentCamera == m_previousCamera)
        return;

    m_accessed = false;
    m_previousCamera = m_currentCamera;
    const auto& camera = m_currentCamera;

    // Update view matrix
    m_viewMatrix = glm::lookAt(camera.position, camera.lookat, camera.vup);

    // Update projection matrix
    if (camera.projection == Camera::Projection::Perspective)
        m_projectionMatrix = glm::perspective(camera.fov, camera.asp, camera.np, camera.fp);

    else { // Orhtographic
        float size = glm::length(camera.lookat - camera.position);
        float left = -camera.asp * size;
        float right = camera.asp * size;
        float bottom = -size;
        float top = size;
        m_projectionMatrix = glm::ortho(left, right, bottom, top, camera.np, camera.fp);
    }
}

void CameraControllerOrbit::tilt(CameraController& controller, const glm::vec2& delta)
{
    glm::vec3 offset = controller.camera().position - controller.camera().lookat;

    glm::vec3 axis = controller.camera().vup;
    glm::quat rotation = glm::angleAxis(delta.x, glm::normalize(axis));
    offset = offset * rotation;

    axis = glm::cross(axis, offset);
    rotation = glm::angleAxis(delta.y, glm::normalize(axis));
    offset = offset * rotation;

    controller.camera().position = controller.camera().lookat + offset;
}

void CameraControllerOrbit::shift(CameraController& controller, const glm::vec2& delta)
{
    glm::vec3 direction = glm::normalize(controller.camera().lookat - controller.camera().position);
    glm::vec3 right = glm::normalize(glm::cross(direction, controller.camera().vup));
    glm::vec3 up = glm::cross(direction, right);
    controller.camera().position += right * delta.x + up * delta.y;
    controller.camera().lookat += right * delta.x + up * delta.y;
}

void CameraControllerOrbit::zoom(CameraController& controller, float delta) {
    glm::vec3 offset = controller.camera().position - controller.camera().lookat;
    controller.camera().position = controller.camera().lookat + offset * delta;
}

void CameraController::handle(const CameraControllHandler& handler, const glm::vec2 tiltShift, float zoomAmount) {
    if (handler.tiltWith.active())
        tilt(tiltShift * handler.tiltMultiplier);

    if (handler.shiftWith.active()) {
        glm::vec2 shiftMul = glm::length(camera().position - camera().lookat) * handler.shiftMultiplier;
        shift(tiltShift * shiftMul);
    }

    if (handler.zoomInWith.active())
        zoom(1.f + zoomAmount * handler.zoomMultiplier);
    else if (handler.zoomOutWith.active())
        zoom(1.f - zoomAmount * handler.zoomMultiplier);
}

void DebugLines::flus()
{

}
