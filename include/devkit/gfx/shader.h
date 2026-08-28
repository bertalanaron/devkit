#pragma once
#include <devkit/gfx/api_resources.h>
#include <devkit/gfx/vertex.h>
#include <devkit/gfx/uniforms.h>
#include <devkit/gfx/texture.h>
#include <devkit/common/properties.h>

#include <glad/glad.h>

namespace dk::gfx {

class ShaderSource {
public:
	enum Type { Unset = -1, Fragment = 0, Vertex, Geometry, TessellationControl, TessellationEvaluation };
	
	ShaderSource()                          = default;
	ShaderSource(ShaderSource&&)            = default;
	ShaderSource& operator=(ShaderSource&&) = default;

	ShaderSource(std::string source)
		: m_code(std::move(source))
		, m_updated(true)
	{ }

	// @brief Load shader source from file at given path
	static ShaderSource load(const std::string& path);

	static auto postProcessVertexSource() -> std::pair<dk::gfx::ShaderSource, dk::gfx::ShaderSource::Type>;

	static auto passthoughTextureFragmentSource() -> std::pair<dk::gfx::ShaderSource, dk::gfx::ShaderSource::Type>;

	// @brief Update shader source from file at given path
	void update(const std::string& path);

	// @returns Returns a version tag
	unsigned attach(Type type, unsigned program);

	// @returns true when source changed
	bool updated() const;

	const std::string& code() const
	{ return m_code; }

private:
	api::Shader m_apiHandle;

	std::string m_code;
	bool        m_updated = true;
	unsigned    m_version = 0;

	void tryDetach(Type type, unsigned program);
};

namespace api {
unsigned toUnderlying(ShaderSource::Type);
}

class Shader {
public:
	struct LayoutElement {
		const VertexAttributes* attributes;
		unsigned                attributesMask; 
		std::function<void()>   bind;           // function to bind api resource of buffer
		unsigned                divisor = 0;
	};

public:
	using PatchVertices = common::UniqueProperty<int, "PatchVertices">;

	class Config : DK_CONFIG_SPECIALIZATION(Shader,
		PatchVertices);

	Config config;

public:
	Shader()                    = default;
	Shader(Shader&&)            = default;
	Shader& operator=(Shader&&) = default;

	void source(ShaderSource& source, ShaderSource::Type type)
	{ m_sources.at((int)type) = source_t(std::ref(source)); }

	void source(ShaderSource&& source, ShaderSource::Type type)
	{ 
		std::unique_ptr<ShaderSource> ptr = std::make_unique<ShaderSource>(std::move(source));
		m_sources.at((int)type) = source_t(std::move(ptr)); 
	}

	void source(std::pair<ShaderSource, ShaderSource::Type> pair)
	{ source(std::move(pair.first), pair.second); }

	// @brief Get linked shader source with given type
	std::optional<std::reference_wrapper<ShaderSource>> source(ShaderSource::Type type);

	// @brief Get linked shader source with given type
	std::optional<std::reference_wrapper<const ShaderSource>> source(ShaderSource::Type type) const;

	// @brief Access uniforms of the shader
	UniformCollection& uniforms() { return m_uniforms; }
	
	// @brief Access texture unit of the shader
	TextureUnit& textures() { return m_textures; }

	// @brief Bind shader to a uniform and to the texture unit
	void uniformTexture(const std::string& uniform, Texture& texture);

	// @brief Set vertex layout including per instance data
	void layout(std::convertible_to<LayoutElement> auto&&... elements)
	{ m_layout = { std::move((LayoutElement)elements)... }; }

private:
	constexpr inline static auto s_sourceTypeCount = magic_enum::enum_count<ShaderSource::Type>() - 1u; // -1 for Unset
	
	struct source_t {
		using data_t = std::variant<std::monostate, std::reference_wrapper<ShaderSource>, std::unique_ptr<ShaderSource>>;

		source_t()                      = default;
		source_t(source_t&&)            = default;
		source_t& operator=(source_t&&) = default;

		source_t(data_t&& _data)
			: data(std::move(_data))
		{ }

		data_t data;
		int    version = 0;
	};

	using SourceArray = std::array<source_t, s_sourceTypeCount>;
	using Layout = std::vector<LayoutElement>;

	api::Program      m_apiHandle;
	SourceArray       m_sources;

	UniformCollection m_uniforms;
	TextureUnit       m_textures;
	Layout            m_layout = {};

	void makeActive();

	void compile();

	void linkSources(std::optional<std::string> fragDataLocation = std::nullopt);

	template <typename P>
	friend void setShaderProperty(Shader&, const P&);

	friend class FrameBuffer;
};

namespace shader_literals {

inline std::pair<ShaderSource, ShaderSource::Type> operator""_vs(const char* cstr, size_t len)
{ return std::make_pair(ShaderSource(std::string(cstr)), ShaderSource::Vertex); }

inline std::pair<ShaderSource, ShaderSource::Type> operator""_fs(const char* cstr, size_t len)
{ return std::make_pair(ShaderSource(std::string(cstr)), ShaderSource::Fragment); }

inline std::pair<ShaderSource, ShaderSource::Type> operator""_gs(const char* cstr, size_t len)
{ return std::make_pair(ShaderSource(std::string(cstr)), ShaderSource::Geometry); }

inline std::pair<ShaderSource, ShaderSource::Type> operator""_tcs(const char* cstr, size_t len)
{ return std::make_pair(ShaderSource(std::string(cstr)), ShaderSource::TessellationControl); }

inline std::pair<ShaderSource, ShaderSource::Type> operator""_tes(const char* cstr, size_t len)
{ return std::make_pair(ShaderSource(std::string(cstr)), ShaderSource::TessellationEvaluation); }

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
	using Source         = std::reference_wrapper<ShaderSource>;
	using OptionalSource = std::optional<Source>;

public:
	Shader& operator[](const std::string& name)
	{ return m_shaders.at(name); }

	void insert(const std::string& name, OptionalSource vertex = std::nullopt, OptionalSource fragment = std::nullopt, 
		OptionalSource geometry = std::nullopt, OptionalSource tessControl = std::nullopt, OptionalSource tessEvaluation = std::nullopt)
	{
		auto& shader = m_shaders[name];
		if (vertex)         shader.source(vertex.value()        , ShaderSource::Vertex);
		if (fragment)       shader.source(fragment.value()      , ShaderSource::Fragment);
		if (geometry)       shader.source(geometry.value()      , ShaderSource::Geometry);
		if (tessControl)    shader.source(tessControl.value()   , ShaderSource::TessellationControl);
		if (tessEvaluation) shader.source(tessEvaluation.value(), ShaderSource::TessellationEvaluation);
	}

	void insert(const std::string& name, std::array<Source, 2> sources)
	{ insert(name, sources[0], sources[1]); }

	void insert(const std::string& name, std::array<Source, 3> sources)
	{ insert(name, sources[0], sources[1], sources[2]); }

	bool contains(const std::string& name)
	{ return m_shaders.contains(name); }

private:
	std::unordered_map<std::string, Shader> m_shaders;
};

}

namespace YAML {

template<>
struct convert<dk::gfx::Shader::Config> {
	static Node encode(const dk::gfx::Shader::Config& cfg) {
		Node node;
		cfg.for_each([&node](const auto& param) {
			using Param = std::decay_t<decltype(param)>;
			using T     = std::conditional_t<std::is_enum_v<Param>, Param, typename Param::type>;

			const std::string_view CamelCaseName = dk::gfx::Shader::Config::property_name(param);
			node[dk::common::camel_to_snake(CamelCaseName)] = (const T&)param;
		});
		return node;
	}

	static bool decode(const Node& node, dk::gfx::Shader::Config& cfg) {
		cfg.for_each([&](const auto& param) {
			using Param = std::decay_t<decltype(param)>;
			using T     = std::conditional_t<std::is_enum_v<Param>, Param, typename Param::type>;

			const std::string_view CamelCaseName = dk::gfx::Shader::Config::property_name(param);
			const auto snake_case_name = dk::common::camel_to_snake(CamelCaseName);
			if (!node[snake_case_name])
				return;

			cfg.set<Param>(node[snake_case_name].as<T>());
		});
		return true;
	}
};

template<>
struct convert<dk::gfx::Shader> {
	static Node encode(const dk::gfx::Shader& shader) {
		Node node;

		// Config block
		node["config"] = shader.config;

		// Shader sources
		for (auto type : magic_enum::enum_values<dk::gfx::ShaderSource::Type>()) {
			if (type == dk::gfx::ShaderSource::Type::Unset)
				continue;

			auto name = dk::common::camel_to_snake(std::string(magic_enum::enum_name(type)));

			auto srcOpt = shader.source(type);
			if (srcOpt && !srcOpt->get().code().empty())
				node[name] = srcOpt->get().code();
		}

		return node;
	}

	static bool decode(const Node& node, dk::gfx::Shader& shader) {
		// Config
		if (node["config"])
			shader.config = node["config"].as<dk::gfx::Shader::Config>();

		// Shader sources
		for (auto type : magic_enum::enum_values<dk::gfx::ShaderSource::Type>()) {
			if (type == dk::gfx::ShaderSource::Type::Unset)
				continue;

			auto name = dk::common::camel_to_snake(std::string(magic_enum::enum_name(type)));

			if (node[name]) {
				const std::string code = node[name].as<std::string>();
				dk::gfx::ShaderSource src(code);
				shader.source(std::move(src), type);
			}
		}
		return true;
	}
};

} // namespace YAML

namespace dk::gfx {

inline void saveShaderYAML(const Shader& shader, const std::filesystem::path& path) {
	YAML::Node node = YAML::convert<Shader>::encode(shader);
	std::ofstream fout(path);
	fout << node; 
}

inline Shader loadShaderYAML(std::filesystem::path path) {
	YAML::Node node = YAML::LoadFile(path.string());
	return node.as<Shader>();
}

} // namespace dk::gfx
