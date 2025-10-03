#include <devkit/gfx/shader.h>
#include <devkit/common/properties.h>

#include <GL/glew.h>

constexpr int details::gfx::toUnderlying(dk::gfx::ShaderSource::Type type) 
{
	switch (type)
	{
	case dk::gfx::ShaderSource::Fragment: return 0x8B30;
	case dk::gfx::ShaderSource::Vertex:   return 0x8B31;
	case dk::gfx::ShaderSource::Geometry: return 0x8DD9;
	default:
		return -1;
	}
}

dk::gfx::ShaderSource dk::gfx::ShaderSource::load(const std::string& path)
{
    std::ifstream ifs(path);
    std::ostringstream contents;
    contents << ifs.rdbuf();
    ifs.close();

    return ShaderSource(std::move(contents.str()));
}

void dk::gfx::ShaderSource::update(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        spdlog::warn("Couldn't update shader source: {} not found.", path);
        return;
    }

    std::ostringstream contents;
    contents << ifs.rdbuf();
    ifs.close();

    spdlog::trace("Shader source updated: {}", path);
    m_source = contents.str();
    m_updated = true;
}

void writeShaderCompilationErrorInfo(unsigned int handle) {
    int logLen, written;
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);

    if (logLen > 0) {
        std::string log(logLen, '\0');
        glGetShaderInfoLog(handle, logLen, &written, &log[0]);
        spdlog::error("Shader log:\n{}", log);
    }
}

bool checkShaderCompilation(unsigned id, const char* source) {
    int OK;
    glGetShaderiv(id, GL_COMPILE_STATUS, &OK);
    if (!OK) {
        if (source)
            spdlog::error("{}", source);

        spdlog::error("Failed to compile shader!");
        writeShaderCompilationErrorInfo(id);
        return false;
    }
    spdlog::trace("Compiled shader source. id: {}", id);
    return true;
}

void dk::gfx::ShaderSource::compileAs(Type type)
{
    m_updated = false;
    if (m_type == Type::Unset)
        m_type = type;
    if (m_type != type) {
        spdlog::error("Trying to compile {} shader as {}", magic_enum::enum_name(m_type), magic_enum::enum_name(type));
        return;
    }

    // Create shader
    unsigned id = glCreateShader(details::gfx::toUnderlying(type));
    if (!id) {
        spdlog::error("Error creating shader source");
        std::terminate();
    }

    // Compile shader
    const char* source = m_source.c_str();
    glShaderSource(id, 1, (const GLchar**)&source, NULL);
    glCompileShader(id);

    // Do nothing if compilation failed
    if (!checkShaderCompilation(id, m_source.c_str())) {
        glDeleteShader(id);
        return;
    }

    ++m_version;
    m_handle = id;
}

void dk::gfx::ShaderSource::tryDetach(Type type, unsigned program)
{
    GLint count = 0;
    glGetProgramiv(program, GL_ATTACHED_SHADERS, &count);

    std::vector<GLuint> shaders(count);
    glGetAttachedShaders(program, count, nullptr, shaders.data());

    for (GLuint shader : shaders) {
        GLint type = 0;
        glGetShaderiv(shader, GL_SHADER_TYPE, &type);
        if (type == details::gfx::toUnderlying(m_type)) {
            glDetachShader(program, shader);
            return;
        }
    }
}

unsigned dk::gfx::ShaderSource::attach(Type type, unsigned program)
{
    if (m_updated)
    {
        compileAs(type);
        tryDetach(type, program);
        glAttachShader(program, m_handle);
    }
    return m_version;
}

bool dk::gfx::ShaderSource::updated() const
{
    return m_updated;
}

template <>
void details::common::setProperty(dk::gfx::Shader& shader, const dk::gfx::properties::depth_test& dt)
{
    if (dt == dk::gfx::properties::depth_test::enabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

template <>
void details::common::setProperty(dk::gfx::Shader& shader, const dk::gfx::properties::depth_mask& dm)
{
    if (dm == dk::gfx::properties::depth_mask::enabled)
        glDepthMask(GL_TRUE);
    else
        glDepthMask(GL_FALSE);
}

unsigned details::gfx::toUnderlying(dk::gfx::properties::depth_func df)
{
    switch (df)
    {
    case dk::gfx::properties::depth_func::never:    return GL_NEVER;
    case dk::gfx::properties::depth_func::less:     return GL_LESS;
    case dk::gfx::properties::depth_func::equal:    return GL_EQUAL;
    case dk::gfx::properties::depth_func::lequal:   return GL_LEQUAL;
    case dk::gfx::properties::depth_func::greater:  return GL_GREATER; 
    case dk::gfx::properties::depth_func::notequal: return GL_NOTEQUAL;
    case dk::gfx::properties::depth_func::gequal:   return GL_GEQUAL;
    case dk::gfx::properties::depth_func::always:   return GL_ALWAYS;
    default:
        break;
    }
}

template <>
void details::common::setProperty(dk::gfx::Shader& shader, const dk::gfx::properties::depth_func& df)
{
    glDepthFunc(details::gfx::toUnderlying(df));
}

template <>
void details::common::setProperty(dk::gfx::Shader& shader, const dk::gfx::properties::backface_culling& bc)
{
    if (bc == dk::gfx::properties::backface_culling::enabled)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
}

unsigned details::gfx::toUnderlying(dk::gfx::properties::blend_func_src_factor bfs)
{
    switch (bfs)
    {
    case dk::gfx::properties::blend_func_src_factor::one:                 return GL_ONE;
    case dk::gfx::properties::blend_func_src_factor::zero:                return GL_ZERO;
    case dk::gfx::properties::blend_func_src_factor::src_alpha:           return GL_ALPHA;
    case dk::gfx::properties::blend_func_src_factor::one_minus_src_alpha: return GL_ONE_MINUS_SRC_ALPHA;
    default: return 0;
    }
}

unsigned details::gfx::toUnderlying(dk::gfx::properties::blend_func_dst_factor bfs)
{
    switch (bfs)
    {
    case dk::gfx::properties::blend_func_dst_factor::zero:                return GL_ZERO;
    case dk::gfx::properties::blend_func_dst_factor::one:                 return GL_ONE;
    case dk::gfx::properties::blend_func_dst_factor::src_alpha:           return GL_ALPHA;
    case dk::gfx::properties::blend_func_dst_factor::one_minus_src_alpha: return GL_ONE_MINUS_SRC_ALPHA;
    default: return 0;
    }
}

template <>
void details::common::setProperty(dk::gfx::Shader& shader, const dk::gfx::properties::blend& b)
{
    if (b == dk::gfx::properties::blend::enabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
}

template <>
void details::common::setProperty(dk::gfx::Shader& shader, const dk::gfx::properties::blend_func_src_factor& srcf)
{
    GLint currentDst;
    glGetIntegerv(GL_BLEND_DST_RGB, &currentDst);
    glBlendFunc(details::gfx::toUnderlying(srcf), currentDst);
}

template <>
void details::common::setProperty(dk::gfx::Shader& shader, const dk::gfx::properties::blend_func_dst_factor& dstf)
{
    GLint currentSrc;
    glGetIntegerv(GL_BLEND_SRC_RGB, &currentSrc);
    glBlendFunc(currentSrc, details::gfx::toUnderlying(dstf));
}

void dk::gfx::Shader::makeActive()
{
    if (!m_program)
        m_program = glCreateProgram();
    // Compile shaders and link if compiled successfully
    compile();
    glUseProgram(m_program);
    
    callPropertySetters(true);

    // Set vertex layout
    int attributeIndex = 0;
    for (auto& layoutElement : m_layout)
    {
        // Bind api resource of buffer
        layoutElement.bind();
        // Set attribute pointers
        const auto nextIndex = layoutElement.attributes->makePointersActive(attributeIndex, layoutElement.attributesMask);
        // Set pointer divisors
        layoutElement.attributes->setPointerDivisors(layoutElement.divisor, attributeIndex, layoutElement.attributesMask);
        attributeIndex = nextIndex;
    }

    // Bind textures
    m_textures.makeActive();
    
    // Bind uniforms
    m_uniforms.makeActive(m_program);
}

void dk::gfx::Shader::uniformTexture(const std::string& uniform, Texture& texture)
{
    if (auto opt_slot = m_textures.getSlot(texture); opt_slot.has_value()) {
        // Texture is allready bound
        uniforms().set(uniform, opt_slot.value());
        return;
    }

    auto opt_slot = textures().emptySlot();
    if (!opt_slot.has_value())
    {
        // No empty slot is available
        spdlog::warn("[gfx] Shaders's texture unit has no available slot");
        return;
    }

    textures()[opt_slot.value()] = texture;
    uniforms().set(uniform, opt_slot.value());
}

void dk::gfx::Shader::compile()
{
    // Compile and attach sources
    unsigned vertexVersion   = m_vertexSource.lock()->attach(ShaderSource::Type::Vertex  , m_program);
    unsigned fragmentVersion = m_fragmentSource.lock()->attach(ShaderSource::Type::Fragment, m_program);
    unsigned geometryVersion = 0;
    if (m_geometrySource.has_value())
        geometryVersion = m_geometrySource.value().lock()->attach(ShaderSource::Type::Geometry, m_program);

    // Check whether source version changed
    bool shouldLink = false;
    shouldLink |= vertexVersion   != m_vertexVersion;
    shouldLink |= fragmentVersion != m_fragmentVersion;
    shouldLink |= geometryVersion != m_geometryVersion;
    m_vertexVersion   = vertexVersion;
    m_fragmentVersion = fragmentVersion;
    m_geometryVersion = geometryVersion;
    
    // Link if a source changed
    if (shouldLink)
        linkSources();
}

bool checkShaderLinking(unsigned int program) {
    int OK;
    glGetProgramiv(program, GL_LINK_STATUS, &OK);
    if (!OK) {
        spdlog::error("Failed to link shader program!");
        writeShaderCompilationErrorInfo(program);
        return false;
    }
    return true;
}

void dk::gfx::Shader::linkSources(std::optional<std::string> fragDataLocation)
{
    // Connect the fragmentColor to the frame buffer memory
    if (fragDataLocation.has_value())
        glBindFragDataLocation(m_program, 0, fragDataLocation.value().c_str());

    // Link shaders
    glLinkProgram(m_program);
    if (!checkShaderLinking(m_program))
        std::terminate();
}

//void dk::gfx::Shader::detach()
//{
//    if (m_vertex != 0)
//        glDetachShader(m_program, m_vertex);
//    if (m_fragment != 0)
//        glDetachShader(m_program, m_fragment);
//    if (m_geometry != 0)
//        glDetachShader(m_program, m_geometry);
//}

template <>
void details::common::setProperty(dk::gfx::Shader& shader, const dk::gfx::properties::sample_shading& sampleShading)
{
    if (!GLEW_ARB_sample_shading)
        return;

    if (sampleShading == dk::gfx::properties::sample_shading::enabled) {
        glEnable(GL_SAMPLE_SHADING);
        glMinSampleShading(1.0);
    }
    else
        glDisable(GL_SAMPLE_SHADING);
}
