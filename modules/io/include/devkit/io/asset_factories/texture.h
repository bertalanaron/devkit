#pragma once

#include <devkit/io/asset_manager.h>
#include <devkit/gfx/texture.h>

namespace dk::io::assets::factories {

class Texture2DFactory {
public:
    using Texture2D = dk::gfx::Texture2D;

    Texture2D initialize(InitializationContext& ctx)
    {
        const auto path = ctx.meta.parse<std::filesystem::path>();
        return Texture2D::load(ctx.absolute_path(path));
    }

    void modify(Texture2D& texture, ModificationContext& ctx)
    {
        const auto path = ctx.meta.parse<std::filesystem::path>();
        texture = std::move(Texture2D::load(ctx.absolute_path(path)));
    }
};

class CubemapFactory {
public:
    using Cubemap = dk::gfx::Cubemap;

    struct Paths {
        std::filesystem::path left;
        std::filesystem::path right;
        std::filesystem::path top;
        std::filesystem::path bottom;
        std::filesystem::path front;
        std::filesystem::path back;
    };

    Cubemap initialize(InitializationContext& ctx)
    {
        const auto paths = ctx.meta.parse<Paths>();
        return Cubemap({
            ctx.absolute_path(paths.right),
            ctx.absolute_path(paths.left),
            ctx.absolute_path(paths.top),
            ctx.absolute_path(paths.bottom),
            ctx.absolute_path(paths.front),
            ctx.absolute_path(paths.back)
        });
    }

    void modify(Cubemap& cubemap, ModificationContext& ctx)
    {
        const auto paths = ctx.meta.parse<Paths>();
        cubemap = std::move(Cubemap({
            ctx.absolute_path(paths.right),
            ctx.absolute_path(paths.left),
            ctx.absolute_path(paths.top),
            ctx.absolute_path(paths.bottom),
            ctx.absolute_path(paths.front),
            ctx.absolute_path(paths.back)
        }));
    }
};

}
