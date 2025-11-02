#pragma once
#include <devkit/gfx/vertex.h>
#include <devkit/gfx/uniforms.h>
#include <devkit/gfx/texture.h>
#include <devkit/common/properties.h>

#include <GL/glew.h>

namespace dk::gfx {

class ShaderSource {
public:
	enum Type { Unset, Fragment, Vertex, Geometry, TessellationControl, TessellationEvaluation };
	
	ShaderSource(std::string source)
		: m_source(std::move(source))
		, m_updated(true)
	{ }

	// @brief Load shader source from file at given path
	static ShaderSource load(const std::string& path);

	static std::weak_ptr<ShaderSource> postProcessVertexSource();

	static std::weak_ptr<ShaderSource> passthoughTextureFragmentSource();

	// @brief Update shader source from file at given path
	void update(const std::string& path);

	// @returns Returns a version tag
	unsigned attach(Type type, unsigned program);

	// @returns true when source changed
	bool updated() const;

	const std::string& source() const
	{ return m_source; }

private:
	std::string m_source;
	bool        m_updated = true;
	unsigned    m_version = 0;

	Type        m_type   = Unset;
	unsigned    m_handle = 0;

	// @brief Compile shader source as either fragment, vertex or geometry shader
	// @returns Handle to the shader
	void compileAs(Type type);

	void tryDetach(Type type, unsigned program);
};

}

namespace dk::gfx {

struct ShaderDescriptor {
	std::optional<ShaderSource> fragment;
	std::optional<ShaderSource> vertex;
	std::optional<ShaderSource> geometry;
	std::optional<ShaderSource> tessellation_evaluation;
	std::optional<ShaderSource> tessellation_control;
};


terrain.shader
```yaml
vertex: >
	
""
```


class Shader {
public:
	struct LayoutElement {
		const VertexAttributes* attributes;
		unsigned                attributesMask; 
		std::function<void()>   bind;           // function to bind api resource of buffer
		unsigned                divisor = 0;
	};

private:
	using load_source_function_t = std::variant<
		std::function<ShaderSource&(const std::filesystem::path&)>, 
		std::function<ShaderSource&&(const std::filesystem::path&)>>;

public:
	Shader()                    = default;
	Shader(Shader&&)            = default;
	Shader& operator=(Shader&&) = default;

	Shader(const ShaderDescriptor&, load_source_function_t loader);

	void source(ShaderSource& source, ShaderSource::Type type);

	void source(std::pair<ShaderSource, ShaderSource::Type> source);

	void source(ShaderSource&& source, ShaderSource::Type type);
	
	auto source(ShaderSource::Type type) -> std::optional<std::reference_wrapper<ShaderSource>>;

private:
	using source_t = std::variant<std::monostate, ShaderSource, ShaderSource*>;
	constexpr inline static auto s_sourceTypeCount = magic_enum::enum_count<ShaderSource::Type>() - 1u; // -1 for Unset

	std::array<source_t, s_sourceTypeCount> m_sources;

	void makeActive();

	friend class FrameBuffer;
};

namespace shader_literals {

std::pair<ShaderSource, ShaderSource::Type> operator"" _vert(const char* cstr, size_t len)
{ return std::make_pair(ShaderSource(std::string(cstr)), ShaderSource::Vertex); }

std::pair<ShaderSource, ShaderSource::Type> operator"" _frag(const char* cstr, size_t len)
{ return std::make_pair(ShaderSource(std::string(cstr)), ShaderSource::Vertex); }

}

void f()
{
	Shader shader;

	using namespace shader_literals;

	shader.source(R"(
		#version 330
		as
	)"_vert);

	shader.source(R"(
		#version 330

		
	)"_frag);

	shader.source(*ShaderSource::passthoughTextureFragmentSource().lock(), ShaderSource::Fragment);

	auto vert = shader.source(ShaderSource::TessellationControl);
}

//
//class Shader {
//private:
//	using source_wptr_t     = std::weak_ptr<ShaderSource>;
//	using opt_source_wptr_t = std::optional<std::weak_ptr<ShaderSource>>;
//
//public:
//	struct LayoutElement {
//		const VertexAttributes* attributes;
//		unsigned                attributesMask; 
//		std::function<void()>   bind;           // function to bind api resource of buffer
//		unsigned                divisor = 0;
//	};
//
//public:
//	Shader(source_wptr_t vertexSource, source_wptr_t fragmentSource, opt_source_wptr_t opt_geometrySource = std::nullopt)
//		: m_vertexSource(vertexSource)
//		, m_fragmentSource(fragmentSource)
//		, m_geometrySource(opt_geometrySource)
//	{ }
//
//	void makeActive();
//
//	UniformCollection& uniforms() { return m_uniforms; }
//
//	TextureUnit& textures() { return m_textures; }
//
//	void uniformTexture(const std::string& uniform, Texture& texture);
//
//	// @brief Set vertex layout including per instance data
//	void layout(std::convertible_to<LayoutElement> auto&&... elements)
//	{ m_layout = { std::move((LayoutElement)elements)... }; }
//
//private:
//	source_wptr_t     m_vertexSource;
//	source_wptr_t     m_fragmentSource;
//	opt_source_wptr_t m_geometrySource;
//
//	UniformCollection          m_uniforms;
//	TextureUnit                m_textures;
//	std::vector<LayoutElement> m_layout = {};
//
//	unsigned m_vertexVersion   = 0;
//	unsigned m_fragmentVersion = 0;
//	unsigned m_geometryVersion = 0;
//
//	unsigned m_program  = 0;
//
//	void compile();
//
//	void linkSources(std::optional<std::string> fragDataLocation = std::nullopt);
//
//	//void detach();
//};

inline Shader::LayoutElement perInstance(Shader::LayoutElement&& element, int divisor = 1) 
{
	element.divisor = divisor;
	return element;
};

class ShaderCollection {
private:
	template <typename T>
	using uptr_t = std::unique_ptr<T>;

	template <typename T>
	using wptr_t = std::weak_ptr<T>;

	template <typename T>
	using opt_t = std::optional<T>;

public:
	Shader& operator[](const std::string& name)
	{ return *m_shaders.at(name); }

	void insert(const std::string& name, wptr_t<ShaderSource> vertex, wptr_t<ShaderSource> fragment, opt_t<wptr_t<ShaderSource>> geometry = std::nullopt)
	{
		m_shaders.insert({ name, std::make_unique<Shader>(vertex, fragment, geometry) });
	}

	void insert(const std::string& name, std::tuple<wptr_t<ShaderSource>, wptr_t<ShaderSource>> sources)
	{
		insert(name, std::get<0>(sources), std::get<1>(sources));
	}

	void insert(const std::string& name, std::tuple<wptr_t<ShaderSource>, wptr_t<ShaderSource>, wptr_t<ShaderSource>> sources)
	{
		insert(name, std::get<0>(sources), std::get<1>(sources), std::get<2>(sources));
	}

	bool contains(const std::string& name)
	{ return m_shaders.contains(name); }

private:
	std::unordered_map<std::string, uptr_t<Shader>> m_shaders;
};

}

namespace details::gfx {

constexpr int toUnderlying(dk::gfx::ShaderSource::Type type);

}
