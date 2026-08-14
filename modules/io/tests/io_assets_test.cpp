#include <devkit/io/asset_manager.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <future>
#include <iterator>
#include <memory>
#include <set>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <unistd.h>

using namespace dk;

namespace {

struct DummyAsset {
    std::string           asset_name;
    std::filesystem::path relative_path;
    std::string           contents;
};

struct FactoryCounters {
    int initialized = 0;
    int modified    = 0;
};

std::string read_file(const std::filesystem::path& path)
{
    std::ifstream stream(path);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

struct DummyFactory {
    std::shared_ptr<FactoryCounters> counters;

    DummyAsset initialize(io::assets::InitializationContext& ctx)
    {
        ++counters->initialized;
        return DummyAsset{
            .asset_name = ctx.asset_name,
            .relative_path = ctx.relative_path,
            .contents = read_file(ctx.absolute_path()),
        };
    }

    void modify(DummyAsset& asset, io::assets::ModificationContext& ctx)
    {
        ++counters->modified;
        asset.asset_name = ctx.asset_name;
        asset.relative_path = ctx.relative_path;
        asset.contents = read_file(ctx.absolute_path());
    }
};

struct OtherAsset {
    std::string asset_name;
};

struct OtherFactory {
    OtherAsset initialize(io::assets::InitializationContext& ctx)
    {
        return OtherAsset{.asset_name = ctx.asset_name};
    }
};

struct NonCopyableAsset {
    NonCopyableAsset(std::string asset_name, std::string contents)
        : asset_name(std::move(asset_name))
        , contents(std::move(contents))
    { }

    NonCopyableAsset(const NonCopyableAsset&)            = delete;
    NonCopyableAsset& operator=(const NonCopyableAsset&) = delete;
    NonCopyableAsset(NonCopyableAsset&&) noexcept        = default;
    NonCopyableAsset& operator=(NonCopyableAsset&&) noexcept = default;

    std::string asset_name;
    std::string contents;
};

static_assert(!std::is_copy_constructible_v<NonCopyableAsset>);
static_assert(!std::is_copy_assignable_v<NonCopyableAsset>);
static_assert(std::is_move_constructible_v<NonCopyableAsset>);

struct NonCopyableFactory {
    std::shared_ptr<FactoryCounters> counters;

    NonCopyableAsset initialize(io::assets::InitializationContext& ctx)
    {
        ++counters->initialized;
        return NonCopyableAsset(ctx.asset_name, read_file(ctx.absolute_path()));
    }

    void modify(NonCopyableAsset& asset, io::assets::ModificationContext& ctx)
    {
        ++counters->modified;
        asset.asset_name = ctx.asset_name;
        asset.contents = read_file(ctx.absolute_path());
    }
};

struct TaggedAsset {
    std::string factory_tag;
};

struct TaggedFactory {
    std::string tag;

    TaggedAsset initialize(io::assets::InitializationContext&)
    {
        return TaggedAsset{.factory_tag = tag};
    }
};

struct Config {
    std::string title;
    int         max_count;
    bool        enabled;
};

struct BlockingAsset {
    std::string asset_name;
};

struct BlockingFactoryControl {
    std::atomic<int> initialized = 0;
    std::promise<void> initialize_entered;
    std::promise<void> release_initialize;
};

struct BlockingFactory {
    std::shared_ptr<BlockingFactoryControl> control;

    BlockingAsset initialize(io::assets::InitializationContext& ctx)
    {
        ++control->initialized;
        control->initialize_entered.set_value();
        control->release_initialize.get_future().wait();
        return BlockingAsset{.asset_name = ctx.asset_name};
    }
};

struct MeshAsset {
    std::string name;
};

struct MaterialAsset {
    std::string name;
};

struct SceneAsset {
    std::vector<MeshAsset>     meshes;
    std::vector<MaterialAsset> materials;
};

struct SceneFactoryCounters {
    int initialized = 0;
    int modified    = 0;
};

struct SceneFactory {
    std::shared_ptr<SceneFactoryCounters> counters;

    std::shared_ptr<SceneAsset> initialize(io::assets::InitializationContext& ctx)
    {
        ++counters->initialized;

        auto scene = std::make_shared<SceneAsset>();
        populate_scene(*scene, read_file(ctx.absolute_path()));
        publish_virtual_assets(*ctx.virtual_assets, *scene);
        return scene;
    }

    void modify(std::shared_ptr<SceneAsset>& scene, io::assets::ModificationContext& ctx)
    {
        ++counters->modified;

        populate_scene(*scene, read_file(ctx.absolute_path()));
        publish_virtual_assets(*ctx.virtual_assets, *scene);
    }

private:
    static void populate_scene(SceneAsset& scene, const std::string& metadata)
    {
        scene.meshes.clear();
        scene.materials.clear();

        scene.meshes.push_back(MeshAsset{.name = "mars-mesh"});
        if (metadata.contains("with_moon"))
            scene.meshes.push_back(MeshAsset{.name = "moon-mesh"});

        if (!metadata.contains("without_material"))
            scene.materials.push_back(MaterialAsset{.name = "mars-material"});
    }

    static void publish_virtual_assets(io::assets::VirtualAssetManager& virtual_assets, SceneAsset& scene)
    {
        auto write_session = virtual_assets.begin_write();

        for (std::size_t i = 0; i < scene.meshes.size(); ++i)
            write_session.create_or_update("mesh", "mars/meshes/" + std::to_string(i), scene.meshes[i]);

        for (std::size_t i = 0; i < scene.materials.size(); ++i)
            write_session.create_or_update("material", "mars/materials/" + std::to_string(i), scene.materials[i]);
    }
};

class Assets2Test : public testing::Test {
protected:
    void SetUp() override
    {
        const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
        root = std::filesystem::temp_directory_path() / ("asset-manager-assets2-test-" + std::to_string(unique));
        std::filesystem::create_directories(root);
    }

    void TearDown() override
    {
        std::filesystem::remove_all(root);
    }

    void write_text(const std::filesystem::path& relative_path, const std::string& contents) const
    {
        const auto path = root / relative_path;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream stream(path);
        stream << contents;
    }

    void write_asset(const std::filesystem::path& relative_path, const std::string& value) const
    {
        write_asset_of_type(relative_path, "dummy", value);
    }

    void write_asset_of_type(const std::filesystem::path& relative_path,
                             const std::string&           asset_type,
                             const std::string&           value) const
    {
        write_text(relative_path, "asset_type: " + asset_type + "\nasset_data: " + value + "\n");
    }

    void write_config_asset(const std::filesystem::path& relative_path,
                            const std::string&           title,
                            int                          max_count,
                            bool                         enabled) const
    {
        write_text(relative_path,
                   "asset_type: config\n"
                   "asset_data:\n"
                   "  title: " + title + "\n"
                   "  max_count: " + std::to_string(max_count) + "\n"
                   "  enabled: " + (enabled ? "true" : "false") + "\n");
    }

    void write_json_asset_of_type(const std::filesystem::path& relative_path,
                                  const std::string&           asset_type,
                                  const std::string&           value) const
    {
        write_text(relative_path,
                   "{\n"
                   "  \"asset_type\": \"" + asset_type + "\",\n"
                   "  \"asset_data\": \"" + value + "\"\n"
                   "}\n");
    }

    std::filesystem::path root;
};

} // namespace

TEST_F(Assets2Test, ReadsRootFromIniAndScansAssetsWithoutInitializingThem)
{
    write_asset("items/one.asset.yaml", "initial");
    write_text("assets.ini", "[assets]\nroot=" + root.string() + "\n");

    const auto counters = std::make_shared<FactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("dummy", DummyFactory{counters});

    EXPECT_TRUE(assets.has_registered_factory_for_type<DummyAsset>());

    assets.root_via_ini(root / "assets.ini", "assets", "root");
    assets.scan_filesystem();

    EXPECT_EQ(assets.root(), root);
    EXPECT_EQ(counters->initialized, 0);

    auto& storage = assets["items/one"];
    EXPECT_TRUE(storage.has_pending_task());

    auto& asset = storage.as<DummyAsset>();
    EXPECT_EQ(counters->initialized, 1);
    EXPECT_FALSE(storage.has_pending_task());
    EXPECT_EQ(asset.asset_name, "items/one");
    EXPECT_EQ(asset.relative_path.generic_string(), "items/one.asset.yaml");
    EXPECT_NE(asset.contents.find("asset_data: initial"), std::string::npos);
}

TEST_F(Assets2Test, ReScanningUnchangedFilesDoesNotScheduleModification)
{
    write_asset("items/one.asset.yaml", "initial");

    const auto counters = std::make_shared<FactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("dummy", DummyFactory{counters});
    assets.root(root);

    assets.scan_filesystem();
    auto& storage = assets["items/one"];
    std::ignore = storage.as<DummyAsset>();

    assets.scan_filesystem();

    EXPECT_FALSE(storage.has_pending_task());
    EXPECT_EQ(counters->initialized, 1);
    EXPECT_EQ(counters->modified, 0);
}

TEST_F(Assets2Test, ReScanningChangedFileSchedulesLazyModification)
{
    const auto asset_path = std::filesystem::path("items/one.asset.yaml");
    write_asset(asset_path, "initial");

    const auto counters = std::make_shared<FactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("dummy", DummyFactory{counters});
    assets.root(root);

    assets.scan_filesystem();
    auto& storage = assets["items/one"];

    auto& asset = storage.as<DummyAsset>();
    EXPECT_NE(asset.contents.find("asset_data: initial"), std::string::npos);

    write_asset(asset_path, "changed");
    const auto absolute_asset_path = root / asset_path;
    std::filesystem::last_write_time(
        absolute_asset_path,
        std::filesystem::last_write_time(absolute_asset_path) + std::chrono::seconds(2));

    assets.scan_filesystem();

    EXPECT_TRUE(storage.has_pending_task());
    EXPECT_EQ(counters->modified, 0);

    auto& modified_asset = storage.as<DummyAsset>();
    EXPECT_EQ(&modified_asset, &asset);
    EXPECT_FALSE(storage.has_pending_task());
    EXPECT_EQ(counters->initialized, 1);
    EXPECT_EQ(counters->modified, 1);
    EXPECT_NE(modified_asset.contents.find("asset_data: changed"), std::string::npos);
}

TEST_F(Assets2Test, InitializesAndModifiesNonCopyableAssets)
{
    const auto asset_path = std::filesystem::path("items/one.asset.yaml");
    write_asset_of_type(asset_path, "non_copyable", "initial");

    const auto counters = std::make_shared<FactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("non_copyable", NonCopyableFactory{counters});
    assets.root(root);

    EXPECT_TRUE(assets.has_registered_factory_for_type<NonCopyableAsset>());

    assets.scan_filesystem();

    auto& storage = assets["items/one"];
    auto& asset = storage.as<NonCopyableAsset>();

    EXPECT_FALSE(storage.has_pending_task());
    EXPECT_EQ(counters->initialized, 1);
    EXPECT_EQ(asset.asset_name, "items/one");
    EXPECT_NE(asset.contents.find("asset_data: initial"), std::string::npos);

    write_asset_of_type(asset_path, "non_copyable", "changed");
    const auto absolute_asset_path = root / asset_path;
    std::filesystem::last_write_time(
        absolute_asset_path,
        std::filesystem::last_write_time(absolute_asset_path) + std::chrono::seconds(2));

    assets.scan_filesystem();

    EXPECT_TRUE(storage.has_pending_task());

    auto& modified_asset = storage.as<NonCopyableAsset>();
    EXPECT_EQ(&modified_asset, &asset);
    EXPECT_FALSE(storage.has_pending_task());
    EXPECT_EQ(counters->initialized, 1);
    EXPECT_EQ(counters->modified, 1);
    EXPECT_EQ(modified_asset.asset_name, "items/one");
    EXPECT_NE(modified_asset.contents.find("asset_data: changed"), std::string::npos);
}

TEST_F(Assets2Test, ScanThrowsWhenMetadataReferencesUnregisteredFactory)
{
    write_asset("items/one.asset.yaml", "initial");

    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.root(root);

    EXPECT_THROW(assets.scan_filesystem(), std::runtime_error);
}

TEST_F(Assets2Test, RegisteringDuplicateAssetTypeStringForDifferentCppTypeThrows)
{
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("duplicate", DummyFactory{std::make_shared<FactoryCounters>()});

    EXPECT_THROW(assets.register_factory("duplicate", OtherFactory{}), std::runtime_error);
}

TEST_F(Assets2Test, RegisteringSameCppTypeForDifferentAssetTypeStringThrows)
{
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("tagged_a", TaggedFactory{.tag = "a"});

    EXPECT_THROW(assets.register_factory("tagged_b", TaggedFactory{.tag = "b"}), std::runtime_error);
}

TEST_F(Assets2Test, PODFactoryInitializesAssetFromMetadataAssetData)
{
    write_config_asset("config/app.asset.yaml", "app-config", 42, true);

    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("config", io::assets::PODFactory<Config>());
    assets.root(root);

    EXPECT_TRUE(assets.has_registered_factory_for_type<Config>());

    assets.scan_filesystem();

    auto& storage = assets["config/app"];
    EXPECT_TRUE(storage.has_pending_task());

    const auto& config = storage.as<Config>();
    EXPECT_FALSE(storage.has_pending_task());
    EXPECT_EQ(config.title, "app-config");
    EXPECT_EQ(config.max_count, 42);
    EXPECT_TRUE(config.enabled);
}

TEST_F(Assets2Test, PODFactoryUpdatesAssetFromModifiedMetadataAssetData)
{
    const auto config_path = std::filesystem::path("config/app.asset.yaml");
    write_config_asset(config_path, "app-config", 42, true);

    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("config", io::assets::PODFactory<Config>());
    assets.root(root);

    assets.scan_filesystem();
    auto& config = assets["config/app"].as<Config>();
    ASSERT_EQ(config.title, "app-config");

    write_config_asset(config_path, "updated-config", 7, false);
    const auto absolute_config_path = root / config_path;
    std::filesystem::last_write_time(
        absolute_config_path,
        std::filesystem::last_write_time(absolute_config_path) + std::chrono::seconds(2));

    assets.scan_filesystem();

    auto& updated_config = assets["config/app"].as<Config>();
    EXPECT_EQ(&updated_config, &config);
    EXPECT_EQ(updated_config.title, "updated-config");
    EXPECT_EQ(updated_config.max_count, 7);
    EXPECT_FALSE(updated_config.enabled);
}

TEST_F(Assets2Test, ParsesJsonAssetMetadata)
{
    write_json_asset_of_type("items/one.asset.json", "dummy", "from-json");

    const auto counters = std::make_shared<FactoryCounters>();
    io::assets::Manager assets(io::assets::Json{}, ".asset.json");
    assets.register_factory("dummy", DummyFactory{counters});
    assets.root(root);

    assets.scan_filesystem();

    auto& storage = assets["items/one"];
    EXPECT_TRUE(storage.has_pending_task());

    const auto& asset = storage.as<DummyAsset>();
    EXPECT_EQ(counters->initialized, 1);
    EXPECT_FALSE(storage.has_pending_task());
    EXPECT_EQ(asset.asset_name, "items/one");
    EXPECT_EQ(asset.relative_path.generic_string(), "items/one.asset.json");
    EXPECT_NE(asset.contents.find("\"asset_data\": \"from-json\""), std::string::npos);
}

TEST_F(Assets2Test, JsonManagerRejectsYamlAssetMetadata)
{
    write_asset("items/one.asset.json", "yaml-in-json-file");

    io::assets::Manager assets(io::assets::Json{}, ".asset.json");
    assets.register_factory("dummy", DummyFactory{std::make_shared<FactoryCounters>()});
    assets.root(root);

    EXPECT_THROW(assets.scan_filesystem(), std::runtime_error);
}

TEST_F(Assets2Test, UnnamedAssetMetadataInDirectoryUsesDirectoryAsAssetName)
{
    write_asset("shaders/default/.asset.yaml", "default");

    const auto counters = std::make_shared<FactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("dummy", DummyFactory{counters});
    assets.root(root);

    assets.scan_filesystem();

    auto& storage = assets["shaders/default"];
    EXPECT_TRUE(storage.has_pending_task());

    const auto& asset = storage.as<DummyAsset>();
    EXPECT_EQ(counters->initialized, 1);
    EXPECT_EQ(asset.asset_name, "shaders/default");
    EXPECT_EQ(asset.relative_path.generic_string(), "shaders/default/.asset.yaml");
    EXPECT_NE(asset.contents.find("asset_data: default"), std::string::npos);
}

TEST_F(Assets2Test, AssetHandleResolvesTypedAssetAndInitializesItLazily)
{
    write_asset("items/one.asset.yaml", "initial");

    const auto counters = std::make_shared<FactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("dummy", DummyFactory{counters});
    assets.root(root);

    io::assets::Asset<DummyAsset> asset_handle("items/one");

    assets.scan_filesystem();
    EXPECT_EQ(counters->initialized, 0);

    auto resolved = asset_handle.lock(assets);

    EXPECT_EQ(counters->initialized, 1);
    EXPECT_EQ(resolved->asset_name, "items/one");
    EXPECT_EQ((*resolved).relative_path.generic_string(), "items/one.asset.yaml");
    EXPECT_NE(resolved->contents.find("asset_data: initial"), std::string::npos);
}

TEST_F(Assets2Test, AssetHandleThrowsWhenAssetNameIsNotLoaded)
{
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("dummy", DummyFactory{std::make_shared<FactoryCounters>()});
    assets.root(root);

    io::assets::Asset<DummyAsset> asset_handle("items/missing");

    EXPECT_THROW((void)asset_handle.lock(assets), std::out_of_range);
}

TEST_F(Assets2Test, AssetHandleThrowsWhenResolvedWithWrongType)
{
    write_asset("items/one.asset.yaml", "initial");

    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("dummy", DummyFactory{std::make_shared<FactoryCounters>()});
    assets.root(root);

    assets.scan_filesystem();

    io::assets::Asset<OtherAsset> asset_handle("items/one");

    EXPECT_THROW((void)asset_handle.lock(assets), std::bad_any_cast);
}

TEST_F(Assets2Test, ConcurrentLazyResolutionOfSamePendingAssetIsSafe)
{
    write_asset_of_type("items/one.asset.yaml", "blocking", "initial");

    const auto control = std::make_shared<BlockingFactoryControl>();
    auto initialize_entered = control->initialize_entered.get_future();

    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("blocking", BlockingFactory{control});
    assets.root(root);
    assets.scan_filesystem();

    auto& storage = assets["items/one"];

    BlockingAsset* first_asset = nullptr;
    BlockingAsset* second_asset = nullptr;
    std::exception_ptr first_exception;
    std::exception_ptr second_exception;
    std::promise<void> second_started;
    auto second_started_future = second_started.get_future();

    std::thread first([&] {
        try {
            first_asset = &storage.as<BlockingAsset>();
        } catch (...) {
            first_exception = std::current_exception();
        }
    });

    ASSERT_EQ(initialize_entered.wait_for(std::chrono::seconds(1)), std::future_status::ready);

    std::thread second([&] {
        try {
            second_started.set_value();
            second_asset = &storage.as<BlockingAsset>();
        } catch (...) {
            second_exception = std::current_exception();
        }
    });

    ASSERT_EQ(second_started_future.wait_for(std::chrono::seconds(1)), std::future_status::ready);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    control->release_initialize.set_value();

    first.join();
    second.join();

    EXPECT_EQ(first_exception, nullptr);
    EXPECT_EQ(second_exception, nullptr);
    EXPECT_EQ(control->initialized, 1);
    ASSERT_NE(first_asset, nullptr);
    ASSERT_NE(second_asset, nullptr);
    EXPECT_EQ(first_asset, second_asset);
    EXPECT_EQ(first_asset->asset_name, "items/one");
}

TEST_F(Assets2Test, IteratingAndInitializingAssetThatCreatesVirtualAssetsDoesNotDeadlock)
{
    write_asset_of_type("scenes/planets/.asset.yaml", "scene", "planets");
    const auto asset_root = root;

    ASSERT_EXIT({
        std::signal(SIGALRM, [](int) {
            std::_Exit(2);
        });
        ::alarm(1);

        const auto counters = std::make_shared<SceneFactoryCounters>();
        io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
        assets.register_factory("scene", SceneFactory{counters});
        assets.root(asset_root);
        assets.scan_filesystem();

        for (auto&& [asset_name, storage] : assets.all()) {
            if (asset_name == std::filesystem::path("scenes/planets"))
                std::ignore = storage.as<std::shared_ptr<SceneAsset>>();
        }

        ::alarm(0);
        std::_Exit(0);
    }, testing::ExitedWithCode(0), "");
}

TEST_F(Assets2Test, AssetInitializationCanCreateVirtualAssetsUnderItsAssetName)
{
    write_asset_of_type("scenes/planets/.asset.yaml", "scene", "planets");

    const auto counters = std::make_shared<SceneFactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("scene", SceneFactory{counters});
    assets.root(root);

    assets.scan_filesystem();

    EXPECT_THROW((void)assets["scenes/planets/mars/meshes/0"], std::out_of_range);

    auto& scene_storage = assets["scenes/planets"];
    const auto& scene = scene_storage.as<std::shared_ptr<SceneAsset>>();

    EXPECT_EQ(counters->initialized, 1);
    ASSERT_EQ(scene->meshes.size(), 1);
    ASSERT_EQ(scene->materials.size(), 1);

    auto& mesh = assets["scenes/planets/mars/meshes/0"].as<MeshAsset>();
    auto& material = assets["scenes/planets/mars/materials/0"].as<MaterialAsset>();

    EXPECT_EQ(&mesh, &scene->meshes[0]);
    EXPECT_EQ(&material, &scene->materials[0]);
    EXPECT_EQ(mesh.name, "mars-mesh");
    EXPECT_EQ(material.name, "mars-material");
    EXPECT_FALSE(assets["scenes/planets/mars/meshes/0"].has_pending_task());
}

TEST_F(Assets2Test, AssetHandleCanResolveVirtualAssets)
{
    write_asset_of_type("scenes/planets/.asset.yaml", "scene", "planets");

    const auto counters = std::make_shared<SceneFactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("scene", SceneFactory{counters});
    assets.root(root);

    assets.scan_filesystem();
    std::ignore = assets["scenes/planets"].as<std::shared_ptr<SceneAsset>>();

    io::assets::Asset<MeshAsset> mesh_handle("scenes/planets/mars/meshes/0");
    auto resolved_mesh = mesh_handle.lock(assets);

    EXPECT_EQ(resolved_mesh->name, "mars-mesh");
}

TEST_F(Assets2Test, VirtualAssetsAreIncludedInIterationAndCanBeFilteredByType)
{
    write_asset_of_type("scenes/planets/.asset.yaml", "scene", "planets");

    const auto counters = std::make_shared<SceneFactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("scene", SceneFactory{counters});
    assets.root(root);

    assets.scan_filesystem();
    std::ignore = assets["scenes/planets"].as<std::shared_ptr<SceneAsset>>();

    std::set<std::string> all_names;
    for (auto&& [asset_name, storage] : assets.all()) {
        all_names.insert(asset_name.generic_string());
        EXPECT_FALSE(storage.has_pending_task());
    }

    EXPECT_EQ(all_names, (std::set<std::string>{
                             "scenes/planets",
                             "scenes/planets/mars/materials/0",
                             "scenes/planets/mars/meshes/0",
                         }));

    std::set<std::string> mesh_names;
    for (auto&& [asset_name, storage] : assets.of_type("mesh")) {
        mesh_names.insert(asset_name.generic_string());
        EXPECT_EQ(storage.as<MeshAsset>().name, "mars-mesh");
    }

    EXPECT_EQ(mesh_names, (std::set<std::string>{
                              "scenes/planets/mars/meshes/0",
                          }));
}

TEST_F(Assets2Test, AssetModificationUpdatesAndRemovesOutdatedVirtualAssets)
{
    const auto scene_path = std::filesystem::path("scenes/planets/.asset.yaml");
    write_asset_of_type(scene_path, "scene", "planets");

    const auto counters = std::make_shared<SceneFactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("scene", SceneFactory{counters});
    assets.root(root);

    assets.scan_filesystem();
    const auto& scene = assets["scenes/planets"].as<std::shared_ptr<SceneAsset>>();
    ASSERT_EQ(scene->meshes.size(), 1);
    ASSERT_EQ(scene->materials.size(), 1);

    write_asset_of_type(scene_path, "scene", "with_moon without_material");
    const auto absolute_scene_path = root / scene_path;
    std::filesystem::last_write_time(
        absolute_scene_path,
        std::filesystem::last_write_time(absolute_scene_path) + std::chrono::seconds(2));

    assets.scan_filesystem();
    assets["scenes/planets"].execute_pending_task();

    EXPECT_EQ(counters->initialized, 1);
    EXPECT_EQ(counters->modified, 1);
    ASSERT_EQ(scene->meshes.size(), 2);
    EXPECT_TRUE(scene->materials.empty());

    auto& mars_mesh = assets["scenes/planets/mars/meshes/0"].as<MeshAsset>();
    auto& moon_mesh = assets["scenes/planets/mars/meshes/1"].as<MeshAsset>();

    EXPECT_EQ(&mars_mesh, &scene->meshes[0]);
    EXPECT_EQ(&moon_mesh, &scene->meshes[1]);
    EXPECT_EQ(mars_mesh.name, "mars-mesh");
    EXPECT_EQ(moon_mesh.name, "moon-mesh");
    EXPECT_THROW((void)assets["scenes/planets/mars/materials/0"], std::out_of_range);
}

TEST_F(Assets2Test, AllIterationYieldsAssetNameAndStoragePairs)
{
    write_asset("shaders/default.asset.yaml", "default");
    write_asset("shaders/primitive.asset.yaml", "primitive");
    write_asset("textures/diffuse.asset.yaml", "diffuse");

    const auto counters = std::make_shared<FactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("dummy", DummyFactory{counters});
    assets.root(root);

    assets.scan_filesystem();

    std::set<std::string> iterated_names;
    for (auto&& [asset_name, storage] : assets.all()) {
        iterated_names.insert(asset_name.generic_string());
        EXPECT_TRUE(storage.has_pending_task());

        const auto& asset = storage.as<DummyAsset>();
        EXPECT_EQ(asset.asset_name, asset_name.generic_string());
    }

    EXPECT_EQ(iterated_names, (std::set<std::string>{
                                  "shaders/default",
                                  "shaders/primitive",
                                  "textures/diffuse",
                              }));
    EXPECT_EQ(counters->initialized, 3);
}

TEST_F(Assets2Test, DirectoryIterationYieldsOnlyShallowAssetNameAndStoragePairsByDefault)
{
    write_asset("shaders/default.asset.yaml", "default");
    write_asset("shaders/nested/primitive.asset.yaml", "primitive");
    write_asset("textures/diffuse.asset.yaml", "diffuse");

    const auto counters = std::make_shared<FactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("dummy", DummyFactory{counters});
    assets.root(root);

    assets.scan_filesystem();

    std::set<std::string> iterated_names;
    for (auto&& [asset_name, storage] : assets.in_directory("shaders")) {
        iterated_names.insert(asset_name.generic_string());
        EXPECT_TRUE(storage.has_pending_task());
    }

    EXPECT_EQ(iterated_names, (std::set<std::string>{
                                  "shaders/default",
                              }));
    EXPECT_EQ(counters->initialized, 0);
}

TEST_F(Assets2Test, DirectoryIterationCanYieldRecursiveAssetNameAndStoragePairs)
{
    write_asset("shaders/default.asset.yaml", "default");
    write_asset("shaders/nested/primitive.asset.yaml", "primitive");
    write_asset("textures/diffuse.asset.yaml", "diffuse");

    const auto counters = std::make_shared<FactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("dummy", DummyFactory{counters});
    assets.root(root);

    assets.scan_filesystem();

    std::set<std::string> iterated_names;
    for (auto&& [asset_name, storage] : assets.in_directory("shaders", io::assets::DirectoryIteration::Recursive)) {
        iterated_names.insert(asset_name.generic_string());
        EXPECT_TRUE(storage.has_pending_task());
    }

    EXPECT_EQ(iterated_names, (std::set<std::string>{
                                  "shaders/default",
                                  "shaders/nested/primitive",
                              }));
    EXPECT_EQ(counters->initialized, 0);
}

TEST_F(Assets2Test, StringTypeIterationYieldsOnlyAssetNameAndStoragePairsForThatAssetType)
{
    write_asset_of_type("shaders/default.asset.yaml", "dummy", "default");
    write_asset_of_type("shaders/primitive.asset.yaml", "dummy", "primitive");
    write_asset_of_type("textures/diffuse.asset.yaml", "other", "diffuse");

    const auto counters = std::make_shared<FactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("dummy", DummyFactory{counters});
    assets.register_factory("other", OtherFactory{});
    assets.root(root);

    assets.scan_filesystem();

    std::set<std::string> iterated_names;
    for (auto&& [asset_name, storage] : assets.of_type("dummy")) {
        iterated_names.insert(asset_name.generic_string());

        const auto& asset = storage.as<DummyAsset>();
        EXPECT_EQ(asset.asset_name, asset_name.generic_string());
    }

    EXPECT_EQ(iterated_names, (std::set<std::string>{
                                  "shaders/default",
                                  "shaders/primitive",
                              }));
    EXPECT_EQ(counters->initialized, 2);
}

TEST_F(Assets2Test, TypedIterationYieldsOnlyAssetNameAndStoragePairsForThatAssetType)
{
    write_asset_of_type("shaders/default.asset.yaml", "dummy", "default");
    write_asset_of_type("shaders/primitive.asset.yaml", "dummy", "primitive");
    write_asset_of_type("textures/diffuse.asset.yaml", "other", "diffuse");

    const auto counters = std::make_shared<FactoryCounters>();
    io::assets::Manager assets(io::assets::Yaml{}, ".asset.yaml");
    assets.register_factory("dummy", DummyFactory{counters});
    assets.register_factory("other", OtherFactory{});
    assets.root(root);

    assets.scan_filesystem();

    std::set<std::string> iterated_names;
    for (auto&& [asset_name, storage] : assets.of_type<DummyAsset>()) {
        iterated_names.insert(asset_name.generic_string());

        const auto& asset = storage.as<DummyAsset>();
        EXPECT_EQ(asset.asset_name, asset_name.generic_string());
    }

    EXPECT_EQ(iterated_names, (std::set<std::string>{
                                  "shaders/default",
                                  "shaders/primitive",
                              }));
    EXPECT_EQ(counters->initialized, 2);
}
