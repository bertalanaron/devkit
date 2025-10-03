#pragma once
#include <devkit/gfx/vertex.h>
#include <devkit/gfx/uniforms.h>
#include <devkit/gfx/texture.h>
#include <devkit/common/properties.h>

#include <GL/glew.h>

namespace dk::gfx::properties {

enum class depth_test { disabled, enabled };
enum class depth_mask { enabled, disabled };

enum class depth_func { less, never, equal, lequal, greater, notequal, gequal, always };

enum class blend { disabled, enabled };
enum class blend_func_src_factor { one, zero, src_alpha, one_minus_src_alpha };
enum class blend_func_dst_factor { zero, one, src_alpha, one_minus_src_alpha };

enum class sample_shading { disabled, enabled };

}

namespace details::gfx {

unsigned toUnderlying(dk::gfx::properties::depth_func df); 

unsigned toUnderlying(dk::gfx::properties::blend_func_src_factor bfs); 
unsigned toUnderlying(dk::gfx::properties::blend_func_dst_factor bfd); 

template <typename D>
using ShaderProperyCollection = dk::common::DeferredPropertyCollection<D, 
	dk::gfx::properties::depth_test,
	dk::gfx::properties::depth_mask,
	dk::gfx::properties::depth_func,
	dk::gfx::properties::backface_culling,
	dk::gfx::properties::blend,
	dk::gfx::properties::blend_func_src_factor,
	dk::gfx::properties::blend_func_dst_factor,
	dk::gfx::properties::sample_shading>;

}

namespace dk::gfx {

class ShaderSource {
public:
	enum Type { Fragment, Vertex, Geometry, Unset };

	ShaderSource(std::string&& source)
		: m_source(std::move(source))
		, m_updated(true)
	{ }

	// @brief Load shader source from file at given path
	static ShaderSource load(const std::string& path);

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

class Shader 
	: public details::gfx::ShaderProperyCollection<Shader>
{
private:
	using source_wptr_t     = std::weak_ptr<ShaderSource>;
	using opt_source_wptr_t = std::optional<std::weak_ptr<ShaderSource>>;

public:
	struct LayoutElement {
		const VertexAttributes* attributes;
		unsigned                attributesMask; 
		std::function<void()>   bind;           // function to bind api resource of buffer
		unsigned                divisor = 0;
	};

public:
	Shader(source_wptr_t vertexSource, source_wptr_t fragmentSource, opt_source_wptr_t opt_geometrySource = std::nullopt)
		: m_vertexSource(vertexSource)
		, m_fragmentSource(fragmentSource)
		, m_geometrySource(opt_geometrySource)
	{ }

	void makeActive();

	UniformCollection& uniforms() { return m_uniforms; }

	TextureUnit& textures() { return m_textures; }

	void uniformTexture(const std::string& uniform, Texture& texture);

	// @brief Set vertex layout including per instance data
	void layout(std::convertible_to<LayoutElement> auto&&... elements)
	{ m_layout = { std::move((LayoutElement)elements)... }; }

private:
	source_wptr_t     m_vertexSource;
	source_wptr_t     m_fragmentSource;
	opt_source_wptr_t m_geometrySource;

	UniformCollection          m_uniforms;
	TextureUnit                m_textures;
	std::vector<LayoutElement> m_layout = {};

	unsigned m_vertexVersion   = 0;
	unsigned m_fragmentVersion = 0;
	unsigned m_geometryVersion = 0;

	unsigned m_program  = 0;

	void compile();

	void linkSources(std::optional<std::string> fragDataLocation = std::nullopt);

	//void detach();
};

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
