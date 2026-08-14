#pragma once

#include <devkit/gfx/scene.h>
#include <devkit/io/asset_manager.h>

namespace dk::io::assets::factories {

class SceneFactory {
public:
    using Scene = dk::gfx::Scene;

    Scene initialize(InitializationContext& ctx)
    {
        const auto path = ctx.meta.parse<std::filesystem::path>();
        auto scene = Scene::load(ctx.absolute_path(path));

        auto virtual_assets = ctx.virtual_assets->begin_write();
        int i = 0;
        for (auto& mesh_factory : scene.meshFactories())
        {
            virtual_assets.create_or_update("mesh", "meshes/" + std::to_string(i), mesh_factory);
            ++i;
        }

        return std::move(scene);
    }
};

}
