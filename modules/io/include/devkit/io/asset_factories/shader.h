#include <devkit/io/asset_manager.h>
#include <devkit/gfx/shader.h>

namespace dk::io::assets::factories {

class ShaderFactory {
public:
    using Shader       = dk::gfx::Shader;
    using ShaderSource = dk::gfx::ShaderSource;

    struct Config {
        std::optional<int> patch_vertices;
    };

    struct inplace {
        std::optional<Config> config;
        std::optional<std::string> vertex;
        std::optional<std::string> fragment;
        std::optional<std::string> geometry;
        std::optional<std::string> tessellation_control;
        std::optional<std::string> tessellation_evaluation;
    };

    struct external {
        std::optional<Config> config;
        std::optional<std::filesystem::path> vertex;
        std::optional<std::filesystem::path> fragment;
        std::optional<std::filesystem::path> geometry;
        std::optional<std::filesystem::path> tessellation_control;
        std::optional<std::filesystem::path> tessellation_evaluation;
    };

    using ParseData = rfl::TaggedUnion<"source_location", inplace, external>;

    Shader initialize(InitializationContext& ctx)
    {
        auto parse_data = ctx.meta.parse<ParseData>();
        dk::gfx::Shader shader;
        handle_sources(shader, parse_data, ctx);
        return shader;
    }

    void modify(Shader& shader, ModificationContext& ctx)
    {
        auto parse_data = ctx.meta.parse<ParseData>();
        handle_sources(shader, parse_data, ctx);
    }

private:
    static void handle_sources(Shader& shader, const auto& variant_sources, const auto& ctx)
    {
        auto to_string = dk::common::overload{
            [&](const std::string& shader_source_str) { return shader_source_str; },
            [&](const std::filesystem::path& shader_source_path) {
                const auto absolute_path = ctx.absolute_path(shader_source_path);
                std::ifstream file(absolute_path, std::ios::in);
                if (!file.is_open())
                    throw std::runtime_error("Could not open shader source file: " + absolute_path.generic_string());
                std::stringstream buffer;
                buffer << file.rdbuf();
                return buffer.str();
            }
        };

        auto bind_source = [&](const auto& source, const ShaderSource::Type type) -> std::optional<std::monostate> {
            shader.source(ShaderSource(to_string(source)), type);
            return std::nullopt;
        };

        auto bind_sources = [&](const auto& sources_impl) {
            sources_impl.vertex.and_then(std::bind_back(bind_source, ShaderSource::Vertex));
            sources_impl.fragment.and_then(std::bind_back(bind_source, ShaderSource::Fragment));
            sources_impl.geometry.and_then(std::bind_back(bind_source, ShaderSource::Geometry));
            sources_impl.tessellation_control.and_then(std::bind_back(bind_source, ShaderSource::TessellationControl));
            sources_impl.tessellation_evaluation.and_then(std::bind_back(bind_source, ShaderSource::TessellationEvaluation));
        };

        rfl::visit(bind_sources, variant_sources);
    }
};

}
