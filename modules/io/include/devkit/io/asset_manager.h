#pragma once
#include <devkit/common/utils.h>
#include <devkit/common/move_only_any.h>

#include <boost/bimap.hpp>
#include <rfl.hpp>

namespace dk::io::assets {

struct Json {};
struct Yaml {};

enum class DirectoryIteration {
    Shallow,
    Recursive,
};

// Reads and exposes metadata stored next to an asset.
class Meta {
private:
    struct AssetMeta {
        std::string  asset_type;
        rfl::Generic asset_data;
    };

    template <typename Format>
    static rfl::Result<AssetMeta> read_metadata(std::istream& istream);

public:
    template <typename Format>
    Meta(const std::filesystem::path& absolute_path, Format);

    const std::string& asset_type() const;

    template <typename T>
    T parse() const
    {
        auto parsed = rfl::from_generic<T>(m_meta.asset_data);
        if (!parsed.has_value())
            throw std::runtime_error("Failed to parse asset metadata when loading " + m_absolute_path.string() + ": " + parsed.error().what());
        return rfl::from_generic<T>(m_meta.asset_data).value();
    }

private:
    AssetMeta             m_meta;
    std::filesystem::path m_absolute_path;
};

template <typename T>
class Asset;

namespace detail {

class DependencyTracker {
public:
    struct Dependency {
        std::filesystem::path           path;
        std::filesystem::file_time_type last_modified;
        std::uintmax_t                  file_size;
    };

    void set_dependencies(const std::filesystem::path& asset_meta_path,
                          std::vector<Dependency> dependencies);

    std::vector<Dependency> dependencies_for(const std::filesystem::path& asset_meta_path) const;

    std::vector<std::pair<std::filesystem::path, std::vector<Dependency>>> all_dependencies() const;

private:
    std::unordered_map<std::filesystem::path, std::vector<Dependency>> m_dependencies_by_asset_meta;
    mutable std::mutex                                                 m_mutex;
};

// Shared path/name data passed to asset factory contexts.
struct FactoryContextBase {
    FactoryContextBase() = default;

    FactoryContextBase(std::filesystem::path root,
                       std::filesystem::path relative_path,
                       std::string asset_name,
                       std::shared_ptr<DependencyTracker> dependency_tracker);

    std::filesystem::path root;
    std::filesystem::path relative_path;
    std::string           asset_name;
    std::shared_ptr<DependencyTracker> dependency_tracker;

    auto absolute_path() const { return root / relative_path; }

    auto absolute_path(const std::filesystem::path& relative) const
    { return std::filesystem::canonical((root / relative_path).parent_path() / relative).lexically_normal(); }

    void begin_dependency_watch();

    void watch_dependency(const std::filesystem::path& relative);

    void publish_watched_dependencies();

private:
    std::vector<DependencyTracker::Dependency> m_watched_dependencies;
};

} // namespace detail

class VirtualAssetManager;

// Passed to factories while creating a new asset instance.
class InitializationContext : public detail::FactoryContextBase {
private:
    using VirtualAssetManagerPtr = std::unique_ptr<VirtualAssetManager>;

public:
    InitializationContext(detail::FactoryContextBase&& base, Meta&& meta, VirtualAssetManagerPtr&& virtual_asset_manager);

    void set_meta(Meta&& meta);

    Meta                   meta;
    VirtualAssetManagerPtr virtual_assets;
};

// Passed to factories while updating an existing asset instance.
class ModificationContext : public detail::FactoryContextBase {
private:
    using VirtualAssetManagerPtr = std::unique_ptr<VirtualAssetManager>;

public:
    ModificationContext(detail::FactoryContextBase&& base, Meta&& original_meta, Meta&& meta, VirtualAssetManagerPtr&& virtual_asset_manager);

    void set_meta(Meta&& meta);

    Meta                   meta;
    VirtualAssetManagerPtr virtual_assets;
};

template <typename T>
class PODFactory {
public:
    T initialize(InitializationContext& ctx)
    {
        return ctx.meta.template parse<T>();
    }

    void modify(T& asset, ModificationContext& ctx)
    {
        asset = ctx.meta.template parse<T>();
    }
};

class Manager {
private:
    using MetaReader = std::function<Meta(const std::filesystem::path&)>;

    // Handles asset factories by creating type-erased tasks for asset initialization and modification.
    class AbstractFactory {
    public:
        using InitializationTask = std::packaged_task<common::move_only_any(InitializationContext&)>;
        using ModificationTask   = std::packaged_task<void(common::move_only_any&, ModificationContext&)>;

        template <typename Factory>
        AbstractFactory(Factory&& factory);

        InitializationTask create_initialization_task();

        ModificationTask create_modification_task();

    private:
        std::function<InitializationTask()> m_initialization_task_factory;
        std::function<ModificationTask()>   m_modification_task_factory;
    };

    class FileSystemHandler;

    class Storage {
    private:
        class PendingModificationState;

        // Holds a loaded asset with no pending work.
        class ReadyState {
        public:
            ReadyState(common::move_only_any&& asset, Meta&& meta, detail::FactoryContextBase&& context_base);

            ReadyState execute();
            PendingModificationState set_meta(AbstractFactory&, Meta&& meta, std::unique_ptr<VirtualAssetManager>&& virtual_asset_manager);
            static bool has_pending_task();
            const std::string& asset_type() const;

            template <typename T>
            T& get() { return m_asset.get<T>(); }

        private:
            common::move_only_any      m_asset;
            Meta                       m_meta;
            detail::FactoryContextBase m_ctx_base;
        };

        // Holds a loaded asset and a queued modification task.
        class PendingModificationState {
        public:
            PendingModificationState(common::move_only_any&& asset, ModificationContext&& ctx, AbstractFactory& factory);

            ReadyState execute();
            PendingModificationState set_meta(AbstractFactory& factory, Meta&& meta, std::unique_ptr<VirtualAssetManager>&& virtual_asset_manager);
            static bool has_pending_task();
            const std::string& asset_type() const;

            template <typename T>
            T& get() { return m_asset.get<T>(); }

        private:
            common::move_only_any                          m_asset;
            ModificationContext               m_ctx;
            AbstractFactory::ModificationTask m_task;
        };

        // Holds metadata and a queued initialization task.
        class PendingInitializationState {
        public:
            PendingInitializationState(InitializationContext&& ctx, AbstractFactory& factory);

            ReadyState execute();
            PendingInitializationState set_meta(AbstractFactory& factory, Meta&& meta, std::unique_ptr<VirtualAssetManager>&& virtual_asset_manager);
            static bool has_pending_task();
            const std::string& asset_type() const;

            template <typename T>
            T& get() { throw std::runtime_error("Trying to get asset from a pending initialization state."); }

        private:
            InitializationContext               m_ctx;
            AbstractFactory::InitializationTask m_task;
        };

        // Placeholder before metadata has been discovered.
        class EmptyState {
        public:
            explicit EmptyState(detail::FactoryContextBase&& base);

            EmptyState execute();
            PendingInitializationState set_meta(AbstractFactory&, Meta&& meta, std::unique_ptr<VirtualAssetManager>&& virtual_asset_manager);
            static bool has_pending_task();
            const std::string& asset_type() const;

            template <typename T>
            T& get() { throw std::runtime_error("Trying to get asset from an empty state."); }

        private:
            detail::FactoryContextBase m_ctx_base;
        };

        class VirtualAssetState {
        public:
            template <typename T>
            explicit VirtualAssetState(const std::string& asset_type, T* asset);

            VirtualAssetState execute();
            VirtualAssetState set_meta(AbstractFactory&, Meta&& meta, std::unique_ptr<VirtualAssetManager>&& virtual_asset_manager);
            static bool has_pending_task();
            const std::string& asset_type() const;

            template <typename T>
            T& get() { return m_asset.get<std::reference_wrapper<T>>().get(); }

        private:
            std::string m_asset_type;
            common::move_only_any    m_asset;
        };

        using State = std::variant<EmptyState, PendingInitializationState, PendingModificationState, ReadyState, VirtualAssetState>;

    public:
        explicit Storage(detail::FactoryContextBase&& context_base)
            : m_state(std::in_place_type<EmptyState>, std::move(context_base))
        { }

        // Creates storage for a virtual asset
        template <typename T>
        explicit Storage(const std::string& asset_type, T* asset)
            : m_state(std::in_place_type<VirtualAssetState>, asset_type, asset)
        { }

        void execute_pending_task();

        [[nodiscard]]
        bool has_pending_task() const;

        [[nodiscard]]
        const std::string& asset_type() const;

        [[nodiscard]]
        bool is_virtual_asset() const;

        template <typename T>
        T& as()
        {
            std::scoped_lock lock(m_mutex);
            if (has_pending_task_unlocked())
                execute_pending_task_unlocked();
            return std::visit([](auto& current_state) -> T& { return current_state.template get<T>(); }, m_state);
        }

    private:
        void execute_pending_task_unlocked();

        [[nodiscard]]
        bool has_pending_task_unlocked() const;

        void set_meta(AbstractFactory& factory, Meta&& meta, std::unique_ptr<VirtualAssetManager>&& virtual_asset_manager);

    private:
        State              m_state;
        mutable std::mutex m_mutex;

        friend class Manager::FileSystemHandler;
    };

    class AbstractFactoryCollection {
    public:
        AbstractFactoryCollection();

        bool has_type(const std::type_index& type_index) const;

        bool has_type(const std::string& asset_type_string) const;

        template <typename AssetType>
        bool has_type() const
        {
            return has_type(std::type_index(typeid(AssetType)));
        }

        const std::string& get_asset_type_string(const std::type_index& type_index) const;

        template <typename AssetType>
        const std::string& get_asset_type_string() const
        {
            return get_asset_type_string(std::type_index(typeid(AssetType)));
        }

        std::type_index get_type_index(const std::string& asset_type_string) const;

        AbstractFactory& get(const std::string& asset_type_string);

        template <typename Factory>
        void insert(const std::string& asset_type_string, Factory&& factory)
        {
            using StoredFactory = std::decay_t<Factory>;
            using AssetType = std::decay_t<decltype(std::declval<StoredFactory&>().initialize(std::declval<InitializationContext&>()))>;

            std::scoped_lock lock(m_mutex);
            const auto type_index = std::type_index(typeid(AssetType));

            const auto type_it = m_types.left.find(type_index);
            if (type_it != m_types.left.end()) {
                if (type_it->second != asset_type_string)
                    throw std::runtime_error("Asset type is already registered as " + type_it->second);
                throw std::runtime_error("Asset factory is already registered for asset type " + asset_type_string);
            }

            const auto string_it = m_types.right.find(asset_type_string);
            if (string_it != m_types.right.end())
                throw std::runtime_error("Asset type string is already registered: " + asset_type_string);

            m_types.insert({type_index, asset_type_string});
            m_factories.emplace(type_index, AbstractFactory(std::forward<Factory>(factory)));
        }

    private:
        boost::bimap<std::type_index, std::string>           m_types;
        std::unordered_map<std::type_index, AbstractFactory> m_factories;
        mutable std::mutex                                   m_mutex; // For creating read/write locks
    };

    // Owns all asset storages and guards concurrent access.
    class StorageCollection {
    public:
        StorageCollection();

        class WriteSession {
        public:
            explicit WriteSession(StorageCollection& collection);

            Storage& get_or_insert(const std::filesystem::path& path, detail::FactoryContextBase&& context_base);

        private:
            std::unique_lock<std::shared_mutex> m_lock;
            StorageCollection&                  m_collection;
        };

        class VirtualAssetWriteSession {
        public:
            explicit VirtualAssetWriteSession(StorageCollection& collection, const std::filesystem::path& parent_asset_name);
            ~VirtualAssetWriteSession();

            VirtualAssetWriteSession(const VirtualAssetWriteSession&) = delete;
            VirtualAssetWriteSession& operator=(const VirtualAssetWriteSession&) = delete;
            VirtualAssetWriteSession(VirtualAssetWriteSession&&) = delete;
            VirtualAssetWriteSession& operator=(VirtualAssetWriteSession&&) = delete;

            template <typename T>
            void create_or_update(const std::string& asset_type_string, const std::filesystem::path& relative_asset_name,
                                  T& asset);

            void mark_updated(const std::filesystem::path& relative_asset_name);

            void remove_outdated();

        private:
            std::unique_lock<std::shared_mutex>       m_lock;
            StorageCollection&                        m_collection;
            std::unordered_set<std::filesystem::path> m_updated_assets;
            std::filesystem::path                     m_parent_asset_name;
            bool                                      m_removed_outdated = false;
        };

        class ReadSession {
        public:
            using Filter = std::function<bool(const std::filesystem::path&, const Storage&)>;

            // Iterates over storages, skipping entries rejected by the filter.
            class Iterator {
            public:
                using Entry = std::pair<std::filesystem::path, std::shared_ptr<Storage>>;
                using BaseIterator = std::vector<Entry>::iterator;
                using difference_type = std::ptrdiff_t;
                using value_type = std::pair<const std::filesystem::path&, Storage&>;
                using iterator_category = std::forward_iterator_tag;

                Iterator(BaseIterator current, BaseIterator end, const Filter* filter);

                value_type operator*() const;
                Iterator& operator++();
                Iterator operator++(int);
                bool operator==(const Iterator& other) const;
                bool operator!=(const Iterator& other) const;

            private:
                void skip_filtered();

                BaseIterator   m_current;
                BaseIterator   m_end;
                const Filter*  m_filter;
            };

            explicit ReadSession(StorageCollection& collection, Filter filter = {});

            Iterator begin();
            Iterator end();
            Iterator find(const std::filesystem::path& path);

        private:
            StorageCollection& m_collection;
            Filter             m_filter;
            std::vector<std::pair<std::filesystem::path, std::shared_ptr<Storage>>> m_snapshot;
        };

        WriteSession begin_write();

        ReadSession begin_read();

        std::optional<std::reference_wrapper<Storage>> get(const std::filesystem::path& path);

        std::optional<std::reference_wrapper<const Storage>> get(const std::filesystem::path& path) const;

        std::shared_ptr<Storage> get_shared(const std::filesystem::path& path);

    private:
        std::unordered_map<std::filesystem::path, std::shared_ptr<Storage>> m_storages;
        mutable std::shared_mutex                                          m_mutex;
    };

    class FileSystemHandler {
    private:
        struct FileState {
            std::filesystem::file_time_type last_modified;
            std::uintmax_t                  file_size;
            std::optional<std::filesystem::file_time_type> pending_last_modified;
            std::optional<std::uintmax_t>                  pending_file_size;
            std::chrono::steady_clock::time_point          pending_since;
        };

    public:
        FileSystemHandler(const std::string& asset_meta_suffix,
                          std::shared_ptr<detail::DependencyTracker> dependency_tracker);

        // Scans asset metadata files and updates storage state.
        void execute_scan(const std::filesystem::path& root,
                          std::shared_ptr<StorageCollection>& storages,
                          AbstractFactoryCollection& factories,
                          const MetaReader& read_meta);

    private:
        std::unordered_map<std::filesystem::path, FileState> m_file_states;
        std::string                                          m_asset_meta_suffix;
        std::shared_ptr<detail::DependencyTracker>           m_dependency_tracker;

        bool is_asset_metafile(const std::filesystem::directory_entry& entry) const;

        std::string generate_asset_name(const std::filesystem::path& relative_path) const
        {
            auto asset_name = relative_path.generic_string();
            asset_name.resize(asset_name.length() - m_asset_meta_suffix.length());
            if (asset_name.empty())
                throw std::runtime_error("Implicitly named asset is invalid at asset root.");
            if (asset_name.back() == '/')
                asset_name.pop_back();
            return asset_name;
        }
    };

public:
    // Creates a manager rooted at the executable location by default.
    template <typename Format>
    Manager(Format format, const std::string& asset_suffix)
        : m_root(dk::common::executable_path())
        , m_asset_suffix(asset_suffix)
        , m_meta_reader([](const std::filesystem::path& path) {
            return Meta(path, std::decay_t<Format>{});
        })
        , m_storages(std::make_shared<StorageCollection>())
        , m_dependency_tracker(std::make_shared<detail::DependencyTracker>())
        , m_file_system_handler(asset_suffix, m_dependency_tracker)
    {
        (void)format;
    }

    void root(const std::filesystem::path& path);

    void root_via_ini(const std::filesystem::path& ini_path, const std::string& ini_namespace,
                      const std::string& ini_key);

    // Returns current root set by user or the executable location by default.
    [[nodiscard]]
    const std::filesystem::path& root() const;

    template <typename Factory>
    void register_factory(const std::string& asset_type_string, Factory&& factory)
    {
        m_factories.insert(asset_type_string, std::forward<Factory>(factory));
    }

    template <typename AssetType>
    bool has_registered_factory_for_type() const
    {
        return m_factories.template has_type<AssetType>();
    }

    void scan_filesystem();

    // Returns storage for an asset name discovered during scanning.
    Storage& operator[](const std::filesystem::path& asset_name);

    const Storage& operator[](const std::filesystem::path& asset_name) const;

    StorageCollection::ReadSession all();

    StorageCollection::ReadSession in_directory(const std::filesystem::path& path,
                                                DirectoryIteration iteration = DirectoryIteration::Shallow);

    StorageCollection::ReadSession of_type(const std::string& asset_type_string);

    template <typename T>
    StorageCollection::ReadSession of_type()
    {
        return of_type(m_factories.template get_asset_type_string<T>());
    }

private:
    std::filesystem::path m_root;
    const std::string     m_asset_suffix;
    MetaReader            m_meta_reader;

    AbstractFactoryCollection          m_factories;
    std::shared_ptr<StorageCollection> m_storages;
    std::shared_ptr<detail::DependencyTracker> m_dependency_tracker;
    FileSystemHandler                  m_file_system_handler;

    friend class VirtualAssetManager;
    template <typename T> friend class Asset;
};

template <typename T>
class Asset {
private:
    class Resolved {
    public:
        Resolved(const std::filesystem::path& asset_name, Manager& manager)
        {
            m_storage = manager.m_storages->get_shared(asset_name);
            if (!m_storage)
                throw std::out_of_range("Asset is not loaded: " + asset_name.generic_string());

            m_asset = &m_storage->template as<T>();
        }

        Resolved(const Resolved&)            = delete;
        Resolved& operator=(const Resolved&) = delete;
        Resolved(Resolved&&)                 = default;
        Resolved& operator=(Resolved&&)      = delete;

        T& operator*() { return *m_asset; }
        const T& operator*() const { return *m_asset; }

        T* operator->() { return m_asset; }
        const T* operator->() const { return m_asset; }

    private:
        std::shared_ptr<Manager::Storage> m_storage;
        T*                                m_asset = nullptr;
    };

public:
    explicit Asset(std::filesystem::path asset_name)
        : m_asset_name(std::move(asset_name))
    { }

    Resolved lock(Manager& manager) { return Resolved(m_asset_name, manager); }

private:
    std::filesystem::path m_asset_name;
};

class VirtualAssetManager {
public:
    VirtualAssetManager(std::weak_ptr<Manager::StorageCollection> storages, const std::filesystem::path& parent_asset_name);

    Manager::StorageCollection::VirtualAssetWriteSession begin_write();

private:
    std::weak_ptr<Manager::StorageCollection> m_storages;
    std::filesystem::path                     m_parent_asset_name;

    friend class InitializationContext;
};

template<typename Format>
Meta::Meta(const std::filesystem::path &absolute_path, Format)
    : m_absolute_path(absolute_path)
{
    std::ifstream ifs(absolute_path);
    if (!ifs.is_open())
        throw std::runtime_error("Could not open file");
    auto metadata_result = read_metadata<Format>(ifs);
    if (!metadata_result.has_value())
    {
        throw std::runtime_error("Could not parse asset metadata in " + absolute_path.string() + ": " + metadata_result.error().what());
    }
    m_meta = metadata_result.value();
}

template<typename Factory>
Manager::AbstractFactory::AbstractFactory(Factory&& factory) {
    using StoredFactory = std::decay_t<Factory>;
    using AssetType = std::decay_t<decltype(std::declval<StoredFactory&>().initialize(std::declval<InitializationContext&>()))>;

    auto factory_ptr = std::make_shared<StoredFactory>(std::forward<Factory>(factory));

    m_initialization_task_factory = [factory_ptr] {
        return InitializationTask([factory_ptr](InitializationContext& ctx) -> common::move_only_any {
            return factory_ptr->initialize(ctx);
        });
    };

    m_modification_task_factory = [factory_ptr] {
        return ModificationTask([factory_ptr](common::move_only_any& asset, ModificationContext& ctx) {
            if constexpr (requires(StoredFactory& factory, AssetType& typed_asset, ModificationContext& mod_ctx) {
                factory.modify(typed_asset, mod_ctx);
            }) {
                factory_ptr->modify(asset.get<AssetType>(), ctx);
            }
        });
    };
}

template <typename T>
Manager::Storage::VirtualAssetState::VirtualAssetState(const std::string& asset_type, T* asset)
    : m_asset_type(asset_type)
    , m_asset(std::ref(*asset))
{ }

template <typename T>
void Manager::StorageCollection::VirtualAssetWriteSession::create_or_update(
    const std::string& asset_type_string,
    const std::filesystem::path& relative_asset_name,
    T& asset)
{
    const auto asset_name = (m_parent_asset_name / relative_asset_name).lexically_normal();
    m_collection.m_storages.insert_or_assign(asset_name, std::make_shared<Storage>(asset_type_string, std::addressof(asset)));
    mark_updated(relative_asset_name);
}

}
