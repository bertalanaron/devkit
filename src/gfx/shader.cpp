#include <devkit/gfx/shader.h>
#include <devkit/common/properties.h>

#include <glad/glad.h>

dk::gfx::api::ShaderType toApi(dk::gfx::ShaderSource::Type type)
{
    using namespace dk::gfx;
    switch(type) {
    case ShaderSource::Type::Vertex                : return api::ShaderType::Vertex;
    case ShaderSource::Type::Fragment              : return api::ShaderType::Fragment;
    case ShaderSource::Type::Geometry              : return api::ShaderType::Geometry;
    case ShaderSource::Type::TessellationControl   : return api::ShaderType::TessellationControl;
    case ShaderSource::Type::TessellationEvaluation: return api::ShaderType::TessellationEvaluation;
    }
    throw std::invalid_argument("Unknown shader type");
}

dk::gfx::ShaderSource dk::gfx::ShaderSource::load(const std::string& path)
{
    std::ifstream ifs(path);
    std::ostringstream contents;
    contents << ifs.rdbuf();
    ifs.close();

    return ShaderSource(std::move(contents.str()));
}

std::pair<dk::gfx::ShaderSource, dk::gfx::ShaderSource::Type> dk::gfx::ShaderSource::postProcessVertexSource()
{
    using namespace shader_literals;

    return R"(
        #version 330 core

        // Fullscreen triangle positions (in a VBO or generated in the shader)
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aTexCoord;

        out vec2 UV;

        void main()
        {
            UV = aTexCoord;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )"_vs;
}

std::pair<dk::gfx::ShaderSource, dk::gfx::ShaderSource::Type> dk::gfx::ShaderSource::passthoughTextureFragmentSource()
{
    using namespace shader_literals;

    return R"(
        #version 330 core

        in vec2 UV;
        out vec4 FragColor;

        uniform sampler2D u_texture;

        void main()
        {
            FragColor = texture(u_texture, UV);
        }
    )"_fs;
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
    m_code = contents.str();
    m_updated = true;
}

void dk::gfx::ShaderSource::tryDetach(Type type_, unsigned program)
{
    GLint count = 0;
    glGetProgramiv(program, GL_ATTACHED_SHADERS, &count);

    std::vector<GLuint> shaders(count);
    glGetAttachedShaders(program, count, nullptr, shaders.data());

    for (GLuint shader : shaders) {
        GLint type = 0;
        glGetShaderiv(shader, GL_SHADER_TYPE, &type);
        if (type == api::toUnderlying(toApi(type_))) {
            glDetachShader(program, shader);
            return;
        }
    }
}

unsigned dk::gfx::ShaderSource::attach(Type type, unsigned program)
{
    if (m_updated)
    {
        m_updated = false;
        m_apiHandle.compile(toApi(type), m_code);
        ++m_version;
    }
    tryDetach(type, program);
    glAttachShader(program, m_apiHandle.handle(toApi(type)));
    return m_version;
}

bool dk::gfx::ShaderSource::updated() const
{
    return m_updated;
}

std::optional<std::reference_wrapper<dk::gfx::ShaderSource>> dk::gfx::Shader::source(ShaderSource::Type type)
{
    using Ret = std::optional<std::reference_wrapper<dk::gfx::ShaderSource>>;
    return std::visit(common::overload {
        [](std::monostate) -> Ret { return std::nullopt; },
        [](std::unique_ptr<ShaderSource>& ptr) -> Ret {
            return *ptr.get();
        },
        [](std::reference_wrapper<ShaderSource> src) -> Ret {
            return src.get();
        }
    }, m_sources.at(type).data);
}

std::optional<std::reference_wrapper<const dk::gfx::ShaderSource>> dk::gfx::Shader::source(ShaderSource::Type type) const
{
    using Ret = std::optional<std::reference_wrapper<const dk::gfx::ShaderSource>>;
    return std::visit(common::overload {
        [](std::monostate) -> Ret { return std::nullopt; },
        [](const std::unique_ptr<ShaderSource>& ptr) -> Ret {
            return *ptr.get();
        },
        [](std::reference_wrapper<const ShaderSource> src) -> Ret {
            return src.get();
        }
    }, m_sources.at(type).data);
}

void dk::gfx::Shader::makeActive()
{
    auto program = m_apiHandle.handle();
    compile();
    m_apiHandle.bind();

    // Call property setters for changed params
    config.for_each([&](const auto& param) {
        if (!config.dirty(param))
            return;
        setShaderProperty(*this, param);
    });

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
    m_uniforms.makeActive(program);
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
    auto program = m_apiHandle.handle();
    bool shouldLink = false;

    // Compile and attach each bound source
    for (auto type : magic_enum::enum_values<ShaderSource::Type>()) 
    {
        // Skip Unset and unbound shader types
        if (type == ShaderSource::Unset)
            continue;
        if (!source(type).has_value())
            continue;

        // Compile and attach sources
        unsigned version = source(type).value().get().attach(type, program);

        // Check whether source version changed
        shouldLink |= (version != m_sources.at(type).version);
    }

    // Link if a source changed
    if (shouldLink)
        linkSources();
}

bool checkShaderLinking(unsigned int program) {
    int OK;
    glGetProgramiv(program, GL_LINK_STATUS, &OK);
    if (!OK) {
        spdlog::error("Failed to link shader program!");
        dk::gfx::api::writeShaderCompilationErrorInfo(program);
        return false;
    }
    return true;
}

void dk::gfx::Shader::linkSources(std::optional<std::string> fragDataLocation)
{
    auto program = m_apiHandle.handle();

    // Connect the fragmentColor to the frame buffer memory
    if (fragDataLocation.has_value())
        glBindFragDataLocation(program, 0, fragDataLocation.value().c_str());

    // Link shaders
    glLinkProgram(program);
    if (!checkShaderLinking(program))
        throw std::runtime_error("Failed to link program");
}

template<>
void dk::gfx::setShaderProperty(Shader&, const Shader::PatchVertices& patchVertices)
{
    glPatchParameteri(GL_PATCH_VERTICES, patchVertices.value);
}
