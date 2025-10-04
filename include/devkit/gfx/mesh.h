#pragma once
#include <devkit/gfx/vertex.h>
#include <devkit/gfx/vertex_buffer.h>
#include <devkit/gfx/element_buffer.h>
#include <devkit/gfx/uniforms.h>
#include <devkit/io/asset_manager.h>
#include <devkit/gfx/shader.h>

namespace dk::gfx {

#define __DK_TABLE_TEXTURETYPE(F, ...)                           \
	/*                       Index | Type      | Uniform Name */ \
	F( __VA_ARGS__ __VA_OPT__(,) 0 , BaseColor , u_color       ) \
	F( __VA_ARGS__ __VA_OPT__(,) 1 , Diffuse   , u_diffuse     ) \
	F( __VA_ARGS__ __VA_OPT__(,) 2 , Normal    , u_normal      ) \
	F( __VA_ARGS__ __VA_OPT__(,) 3 , Specular  , u_specular    ) \
	F( __VA_ARGS__ __VA_OPT__(,) 4 , Height    , u_height      ) \
	F( __VA_ARGS__ __VA_OPT__(,) 5 , Emissive  , u_emissive    ) \
	F( __VA_ARGS__ __VA_OPT__(,) 6 , Metalness , u_metalness   ) \
	F( __VA_ARGS__ __VA_OPT__(,) 7 , Roughness , u_roughness   ) \
	/* end of table */

#pragma region MACRO_MAGIC
#define __DK_ENUMVALUE_TEXTURETYPE(index, name, ...) name = index, 
#define __DK_TEXTURETYPE_BINDUNIFORMS_ALL(index, name, uniformName)       \
	for (unsigned i = 0; i < (*this)[name].size(); ++i) {                 \
	    const std::string uName = std::format("{}[{}]", #uniformName, i); \
		shader.uniformTexture(uName, textureSource((*this)[name].at(i))); \
	}                                                                     \
	/* end of macro */
#define __DK_TEXTURETYPE_BINDUNIFORMS_FIRST(index, name, uniformName)            \
	if (!(*this)[name].empty())                                                  \
		shader.uniformTexture(#uniformName, textureSource((*this)[name].at(0))); \
	/* end of macro */
#pragma endregion

class Material {
public:
	enum TextureType { 
		__DK_TABLE_TEXTURETYPE(__DK_ENUMVALUE_TEXTURETYPE)
	};
	
	using TextureCollection = std::array<std::vector<std::filesystem::path>, magic_enum::enum_count<TextureType>()>;

public:
	explicit Material(TextureCollection&& textures)
		: m_textures(std::move(textures))
	{ }

	auto& operator[](TextureType type) { return m_textures.at(type); }
	const auto& operator[](TextureType type) const { return m_textures.at(type); }

	// @brief Uses default uniform names to bind material textures to a shader
	void bindAllTo(Shader& shader, std::function<Texture&(const std::filesystem::path&)> textureSource)
	{ __DK_TABLE_TEXTURETYPE(__DK_TEXTURETYPE_BINDUNIFORMS_ALL) }

	// @brief Uses default uniform names to bind the first material texture of each type to a shader
	void bindFirstTo(Shader& shader, std::function<Texture&(const std::filesystem::path&)> textureSource)
	{ __DK_TABLE_TEXTURETYPE(__DK_TEXTURETYPE_BINDUNIFORMS_FIRST) }

private:
	TextureCollection m_textures;
};

class Mesh {
public:
	Mesh(const Mesh&)       = default;
	Mesh(Mesh&&)            = default;
	Mesh& operator=(Mesh&&) = default;

	template <typename... VertArgs>
	Mesh(common::id_t<Vertex<VertArgs...>> vertTypeId)
		: vertices(vertTypeId)
	{ }

	Mesh(VertexFlags flags)
		: vertices(flags)
	{ }

	VertexBuffer            vertices;
	ElementBuffer           indices;
	std::optional<Material> material;

	operator Shader::LayoutElement() { return vertices; }
};

// @brief Ignores select vertex attributes of a mesh using a bitmask
class MeshMask {
public:
	MeshMask(Mesh& _mesh, VertexFlags target, VertexFlags original);

	operator Shader::LayoutElement()
	{
		Shader::LayoutElement layout = mesh;
		layout.attributesMask = m_mask;
		return layout;
	}

	Mesh&                    mesh;

	VertexBuffer&            vertices;
	ElementBuffer&           indices;
	std::optional<Material>& material;

private:
	uint64_t m_mask = 0;
};

}
