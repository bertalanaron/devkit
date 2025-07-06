#pragma once
#include <devkit/gfx/vertex.h>
#include <devkit/gfx/uniforms.h>
#include <devkit/gfx/texture.h>
#include <devkit/gfx/vertex_buffer.h>
#include <devkit/common/properties.h>

#include <GL/glew.h>

namespace dk::gfx::properties {

enum class depth_test { disabled, enabled };
enum class depth_mask { enabled, disabled };

enum class depth_func { less, never, equal, lequal, greater, notequal, gequal, always };

enum class blend { disabled, enabled };
enum class blend_func_src_factor { one, zero, src_alpha, one_minus_src_alpha };
enum class blend_func_dst_factor { zero, one, src_alpha, one_minus_src_alpha };

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
	dk::gfx::properties::blend_func_dst_factor>;

}

namespace dk::gfx {

class ShaderSource {
public:
	enum Type { Fragment, Vertex, Geometry };

	ShaderSource(std::string&& source)
		: m_source(std::move(source))
		, m_updated(true)
	{ }

	// @brief Load shader source from file at given path
	static ShaderSource load(const std::string& path);

	// @brief Update shader source from file at given path
	void update(const std::string& path);

	// @brief Compile shader source as either fragment, vertex or geometry shader
	// @returns Handle to the shader
	std::optional<unsigned> compileAs(Type type);

	// @returns true when source changed
	bool updated() const;

	const std::string& source() const
	{ return m_source; }

private:
	std::string m_source;
	bool        m_updated;
};

}

namespace dk::gfx {

class Shader 
	: public details::gfx::ShaderProperyCollection<Shader>
{
private:
	using source_sptr_t     = std::shared_ptr<ShaderSource>;
	using opt_source_sptr_t = std::optional<std::shared_ptr<ShaderSource>>;

	class Layout {
	public:
		struct Element {
			Element(VertexBuffer& vb);
			Element(const std::pair<std::reference_wrapper<VertexBuffer>, int>& vb);
			
			VertexBuffer& vertexBuffer;
			int           divisor;
		};

	public:
		Layout(std::vector<Element>&&);

		void makeActive();

	private:
		std::vector<Element> m_elements;
	};

public:
	Shader(source_sptr_t vertexSource, source_sptr_t fragmentSource, opt_source_sptr_t opt_geometrySource = std::nullopt)
		: m_vertexSource(vertexSource)
		, m_fragmentSource(fragmentSource)
		, m_geometrySource(opt_geometrySource)
	{ }

	void makeActive();

	UniformCollection& uniforms() { return m_uniforms; }

	TextureUnit& textures() { return m_textures; }

	void uniformTexture(const std::string& uniform, Texture& texture);

	// @brief 
	// @param vertexBuffer - List of either vertexbuffer or { vertexbuffer, divisor }
	template <dk::common::OfList<std::reference_wrapper<VertexBuffer>, std::pair<VertexBuffer&, int>>... Ts>
	void layout(Ts... elements)
	{
		std::vector<Layout::Element> layoutElements = { std::move(Layout::Element(elements))... };
		m_layout = Layout(std::move(layoutElements));
	}

private:
	source_sptr_t     m_vertexSource;
	source_sptr_t     m_fragmentSource;
	opt_source_sptr_t m_geometrySource;

	UniformCollection     m_uniforms;
	TextureUnit           m_textures;
	std::optional<Layout> m_layout = {};

	unsigned m_program  = 0;
	unsigned m_vertex   = 0;
	unsigned m_fragment = 0;
	unsigned m_geometry = 0;

	// @brief Returns true if either source has changed
	bool sourceChangedOrUninitalized() const;

	bool compile(unsigned& vertex, unsigned& fragment, unsigned& geometry) const;

	void attachAndLink(std::optional<std::string> fragDataLocation = std::nullopt);

	void detach();
};

}

namespace details::gfx {

constexpr int toUnderlying(dk::gfx::ShaderSource::Type type);

}
