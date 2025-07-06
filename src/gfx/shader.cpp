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

std::optional<unsigned> dk::gfx::ShaderSource::compileAs(Type type)
{
	m_updated = false;

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

    // Return compilation status
    if (!checkShaderCompilation(id, m_source.c_str()))
        return std::nullopt;

	return id;
}

bool dk::gfx::ShaderSource::updated() const
{
    return m_updated;
}

dk::gfx::Shader::Layout::Element::Element(VertexBuffer& vb)
    : vertexBuffer(vb)
    , divisor(0)
{ }

dk::gfx::Shader::Layout::Element::Element(const std::pair<std::reference_wrapper<VertexBuffer>, int>& element)
    : vertexBuffer(element.first)
    , divisor(element.second)
{ }

dk::gfx::Shader::Layout::Layout(std::vector<Element>&& elements)
    : m_elements(std::move(elements))
{ }

void dk::gfx::Shader::Layout::makeActive()
{
    unsigned pointerIndex = 0;
    for (auto& element : m_elements) {
        element.vertexBuffer.makeActive();

        // Set vertex attrib pointers
        unsigned nextPointerIndex = element.vertexBuffer.vertexAttributes()->makePointersActive(pointerIndex);
        // Set divisor
        element.vertexBuffer.vertexAttributes()->setPointerDivisors(element.divisor, pointerIndex);

        pointerIndex = nextPointerIndex;
    }
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
    if (sourceChangedOrUninitalized()) {
        unsigned vertex = 0, fragment = 0, geometry = 0;
        if (compile(vertex, fragment, geometry)) {
            // Compilation was successful
            detach();
            m_vertex   = vertex;
            m_fragment = fragment;
            m_geometry = geometry;
            attachAndLink();
        }
    }
    glUseProgram(m_program);
    
    callPropertySetters(true);

    // Set vertex layout
    if (m_layout.has_value())
        m_layout->makeActive();
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
        // No empty slot is available
        return;

    textures()[opt_slot.value()] = texture;
    uniforms().set(uniform, opt_slot.value());
}

bool dk::gfx::Shader::sourceChangedOrUninitalized() const
{
    return m_vertexSource->updated() || m_vertex == 0
        || m_fragmentSource->updated() || m_fragment == 0 
        || (m_geometrySource.has_value() && (m_geometrySource.value()->updated() || m_geometry == 0));
}

bool dk::gfx::Shader::compile(unsigned& vertex, unsigned& fragment, unsigned& geometry) const
{
    if (m_vertexSource->updated() || m_vertex == 0) {
        auto maybe_vert = m_vertexSource->compileAs(dk::gfx::ShaderSource::Vertex);
        if (!maybe_vert.has_value())
            return false;
        vertex = maybe_vert.value();
    }
    if (m_fragmentSource->updated() || m_fragment == 0) {
        auto maybe_frag = m_fragmentSource->compileAs(dk::gfx::ShaderSource::Fragment);
        if (!maybe_frag.has_value())
            return false;
        fragment = maybe_frag.value();
    }
    if (m_geometrySource.has_value() && (m_geometrySource.value()->updated() || m_geometry == 0)) {
        auto maybe_geom = m_geometrySource.value()->compileAs(dk::gfx::ShaderSource::Geometry);
        if (!maybe_geom.has_value())
            return false;
        geometry = maybe_geom.value();
    }
    return true;
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

void dk::gfx::Shader::attachAndLink(std::optional<std::string> fragDataLocation)
{
    // Attach shaders
    glAttachShader(m_program, m_vertex);
    glAttachShader(m_program, m_fragment);
    if (m_geometrySource.has_value())
        glAttachShader(m_program, m_geometry);

    // Connect the fragmentColor to the frame buffer memory
    if (fragDataLocation.has_value())
        glBindFragDataLocation(m_program, 0, fragDataLocation.value().c_str());

    // Link shaders
    glLinkProgram(m_program);
    if (!checkShaderLinking(m_program))
        std::terminate();
}

void dk::gfx::Shader::detach()
{
    if (m_vertex != 0)
        glDetachShader(m_program, m_vertex);
    if (m_fragment != 0)
        glDetachShader(m_program, m_fragment);
    if (m_geometry != 0)
        glDetachShader(m_program, m_geometry);
}
