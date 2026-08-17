#include <devkit/io/asset_manager.h>

#include <mini/ini.h>

#include <yaml-cpp/yaml.h>
#include <rfl/json.hpp>
#include <rfl/yaml.hpp>

namespace dk::io::assets {

template <>
rfl::Result<Meta::AssetMeta> Meta::read_metadata<Json>(std::istream& istream)
{
    return rfl::json::read<AssetMeta>(istream);
}

template <>
rfl::Result<Meta::AssetMeta> Meta::read_metadata<Yaml>(std::istream& istream)
{
    return rfl::yaml::read<AssetMeta>(istream);
}

const std::string& Meta::asset_type() const { return m_meta.asset_type; }

void detail::DependencyTracker::set_dependencies(
    const std::filesystem::path& asset_meta_path,
    std::vector<Dependency> dependencies)
{
    std::ranges::sort(dependencies, {}, &Dependency::path);
    const auto duplicate_dependencies = std::ranges::unique(dependencies, {}, &Dependency::path);
    dependencies.erase(duplicate_dependencies.begin(), duplicate_dependencies.end());

    std::scoped_lock lock(m_mutex);
    m_dependencies_by_asset_meta.insert_or_assign(asset_meta_path, std::move(dependencies));
}

std::vector<detail::DependencyTracker::Dependency> detail::DependencyTracker::dependencies_for(
    const std::filesystem::path& asset_meta_path) const
{
    std::scoped_lock lock(m_mutex);
    const auto it = m_dependencies_by_asset_meta.find(asset_meta_path);
    if (it == m_dependencies_by_asset_meta.end())
        return {};
    return it->second;
}

std::vector<std::pair<std::filesystem::path, std::vector<detail::DependencyTracker::Dependency>>>
detail::DependencyTracker::all_dependencies() const
{
    std::scoped_lock lock(m_mutex);
    return {m_dependencies_by_asset_meta.begin(), m_dependencies_by_asset_meta.end()};
}

detail::FactoryContextBase::FactoryContextBase(
    std::filesystem::path root,
    std::filesystem::path relative_path,
    std::string asset_name,
    std::shared_ptr<DependencyTracker> dependency_tracker)
    : root(std::move(root))
    , relative_path(std::move(relative_path))
    , asset_name(std::move(asset_name))
    , dependency_tracker(std::move(dependency_tracker))
{ }

void detail::FactoryContextBase::begin_dependency_watch()
{
    m_watched_dependencies.clear();
}

void detail::FactoryContextBase::watch_dependency(const std::filesystem::path& relative)
{
    const auto path = absolute_path(relative);

    std::error_code modification_error;
    const auto last_modified = std::filesystem::last_write_time(path, modification_error);
    if (modification_error)
        throw std::runtime_error("Could not stat asset dependency " + path.generic_string() + ": " + modification_error.message());

    std::error_code size_error;
    const auto file_size = std::filesystem::file_size(path, size_error);
    if (size_error)
        throw std::runtime_error("Could not read the size of asset dependency " + path.generic_string() + ": " + size_error.message());

    m_watched_dependencies.push_back({path, last_modified, file_size});
}

void detail::FactoryContextBase::publish_watched_dependencies()
{
    if (!dependency_tracker)
        return;

    dependency_tracker->set_dependencies(absolute_path(), std::move(m_watched_dependencies));
    m_watched_dependencies.clear();
}

InitializationContext::InitializationContext(detail::FactoryContextBase&& base, Meta&& meta, VirtualAssetManagerPtr&& virtual_asset_manager)
    : detail::FactoryContextBase(std::move(base))
    , meta(std::move(meta))
    , virtual_assets(std::move(virtual_asset_manager))
{ }

void InitializationContext::set_meta(Meta&& meta)
{
    this->meta = std::move(meta);
}

ModificationContext::ModificationContext(detail::FactoryContextBase&& base, Meta&& original_meta, Meta&& meta, VirtualAssetManagerPtr&& virtual_asset_manager)
    : detail::FactoryContextBase(std::move(base))
    , meta(std::move(meta))
    , virtual_assets(std::move(virtual_asset_manager))
{
    // TODO: original_meta
    (void)original_meta;
}

void ModificationContext::set_meta(Meta&& meta)
{
    this->meta = std::move(meta);
}

Manager::AbstractFactory::InitializationTask Manager::AbstractFactory::create_initialization_task()
{
    return m_initialization_task_factory();
}

Manager::AbstractFactory::ModificationTask Manager::AbstractFactory::create_modification_task()
{
    return m_modification_task_factory();
}

Manager::Storage::ReadyState::ReadyState(common::move_only_any&& asset, Meta&& meta, detail::FactoryContextBase&& context_base)
    : m_asset(std::move(asset))
    , m_meta(std::move(meta))
    , m_ctx_base(std::move(context_base))
{ }

Manager::Storage::ReadyState Manager::Storage::ReadyState::execute()
{
    return std::move(*this);
}

Manager::Storage::PendingModificationState Manager::Storage::ReadyState::set_meta(AbstractFactory& factory, Meta&& meta, std::unique_ptr<VirtualAssetManager>&& virtual_asset_manager)
{
    ModificationContext ctx(std::move(m_ctx_base), std::move(m_meta), std::forward<Meta>(meta), std::move(virtual_asset_manager));
    return PendingModificationState(std::move(m_asset), std::move(ctx), factory);
}

bool Manager::Storage::ReadyState::has_pending_task() { return false; }

const std::string& Manager::Storage::ReadyState::asset_type() const { return m_meta.asset_type(); }

Manager::Storage::PendingModificationState::PendingModificationState(common::move_only_any&& asset, ModificationContext&& ctx,
                                                                     AbstractFactory& factory)
    : m_asset(std::move(asset))
    , m_ctx(std::move(ctx))
    , m_task(factory.create_modification_task())
{ }

Manager::Storage::ReadyState Manager::Storage::PendingModificationState::execute()
{
    auto modification_fut = m_task.get_future();
    m_ctx.begin_dependency_watch();
    m_task(m_asset, m_ctx);
    modification_fut.get();
    m_ctx.publish_watched_dependencies();
    return ReadyState(std::move(m_asset),
                      std::move(m_ctx.meta),
                      std::move(static_cast<detail::FactoryContextBase&>(m_ctx)));
}

Manager::Storage::PendingModificationState Manager::Storage::PendingModificationState::set_meta(AbstractFactory& factory, Meta&& meta, std::unique_ptr<VirtualAssetManager>&&)
{
    m_ctx.set_meta(std::forward<Meta>(meta));
    m_task = factory.create_modification_task();
    return std::move(*this);
}

bool Manager::Storage::PendingModificationState::has_pending_task() { return true; }

const std::string& Manager::Storage::PendingModificationState::asset_type() const { return m_ctx.meta.asset_type(); }

Manager::Storage::PendingInitializationState::PendingInitializationState(InitializationContext&& ctx,
                                                                         AbstractFactory& factory)
    : m_ctx(std::move(ctx))
    , m_task(factory.create_initialization_task())
{ }

Manager::Storage::ReadyState Manager::Storage::PendingInitializationState::execute()
{
    auto asset_fut = m_task.get_future();
    m_ctx.begin_dependency_watch();
    m_task(m_ctx);
    auto asset = asset_fut.get();
    m_ctx.publish_watched_dependencies();
    return ReadyState(std::move(asset),
                      std::move(m_ctx.meta),
                      std::move(static_cast<detail::FactoryContextBase&>(m_ctx)));
}

Manager::Storage::PendingInitializationState Manager::Storage::PendingInitializationState::set_meta(AbstractFactory& factory, Meta&& meta, std::unique_ptr<VirtualAssetManager>&&)
{
    m_ctx.set_meta(std::forward<Meta>(meta));
    m_task = factory.create_initialization_task();
    return std::move(*this);
}

bool Manager::Storage::PendingInitializationState::has_pending_task() { return true; }

const std::string& Manager::Storage::PendingInitializationState::asset_type() const { return m_ctx.meta.asset_type(); }

Manager::Storage::EmptyState::EmptyState(detail::FactoryContextBase&& base)
    : m_ctx_base(std::move(base))
{ }

Manager::Storage::EmptyState Manager::Storage::EmptyState::execute()
{
    throw std::runtime_error("Trying to execute task before storage is in a valid state");
}

Manager::Storage::PendingInitializationState Manager::Storage::EmptyState::set_meta(AbstractFactory& factory, Meta&& meta, std::unique_ptr<VirtualAssetManager>&& virtual_asset_manager)
{
    InitializationContext ctx(std::move(m_ctx_base), std::forward<Meta>(meta), std::move(virtual_asset_manager));
    return PendingInitializationState(std::move(ctx), factory);
}

bool Manager::Storage::EmptyState::has_pending_task() { return false; }

const std::string& Manager::Storage::EmptyState::asset_type() const
{
    throw std::runtime_error("Trying to get asset type from an empty state.");
}

Manager::Storage::VirtualAssetState Manager::Storage::VirtualAssetState::execute()
{
    return std::move(*this);
}

Manager::Storage::VirtualAssetState Manager::Storage::VirtualAssetState::set_meta(
    AbstractFactory&,
    Meta&&,
    std::unique_ptr<VirtualAssetManager>&&)
{
    throw std::runtime_error("Trying to set metadata on a virtual asset.");
}

bool Manager::Storage::VirtualAssetState::has_pending_task() { return false; }

const std::string& Manager::Storage::VirtualAssetState::asset_type() const { return m_asset_type; }

void Manager::Storage::execute_pending_task()
{
    std::scoped_lock lock(m_mutex);
    execute_pending_task_unlocked();
}

bool Manager::Storage::has_pending_task() const
{
    std::scoped_lock lock(m_mutex);
    return has_pending_task_unlocked();
}

void Manager::Storage::execute_pending_task_unlocked()
{
    m_state = std::visit([](auto& current_state) -> State { return current_state.execute(); }, m_state);
}

bool Manager::Storage::has_pending_task_unlocked() const
{
    return std::visit([](const auto& current_state) { return current_state.has_pending_task(); }, m_state);
}

const std::string& Manager::Storage::asset_type() const
{
    std::scoped_lock lock(m_mutex);
    return std::visit([](const auto& current_state) -> const std::string& {
        return current_state.asset_type();
    }, m_state);
}

bool Manager::Storage::is_virtual_asset() const
{
    std::scoped_lock lock(m_mutex);
    return std::holds_alternative<VirtualAssetState>(m_state);
}

void Manager::Storage::set_meta(AbstractFactory& factory, Meta&& meta, std::unique_ptr<VirtualAssetManager>&& virtual_asset_manager)
{
    std::scoped_lock lock(m_mutex);
    m_state = std::visit([&](auto& current_state) -> State {
        return current_state.set_meta(factory, std::forward<Meta>(meta), std::move(virtual_asset_manager));
    }, m_state);
}

Manager::StorageCollection::WriteSession::WriteSession(StorageCollection& collection)
    : m_lock(collection.m_mutex)
    , m_collection(collection)
{ }

Manager::Storage& Manager::StorageCollection::WriteSession::get_or_insert(const std::filesystem::path& path,
                                                                          detail::FactoryContextBase&& context_base)
{
    auto [it, inserted] = m_collection.m_storages.try_emplace(
        path,
        std::make_shared<Storage>(std::move(context_base)));
    return *it->second;
}

Manager::StorageCollection::StorageCollection() = default;

Manager::StorageCollection::WriteSession Manager::StorageCollection::begin_write()
{
    return WriteSession(*this);
}

Manager::StorageCollection::VirtualAssetWriteSession::VirtualAssetWriteSession(
    StorageCollection& collection,
    const std::filesystem::path& parent_asset_name)
    : m_lock(collection.m_mutex)
    , m_collection(collection)
    , m_parent_asset_name(parent_asset_name.lexically_normal())
{ }

Manager::StorageCollection::VirtualAssetWriteSession::~VirtualAssetWriteSession()
{
    remove_outdated();
}

void Manager::StorageCollection::VirtualAssetWriteSession::mark_updated(
    const std::filesystem::path& relative_asset_name)
{
    m_updated_assets.insert((m_parent_asset_name / relative_asset_name).lexically_normal());
}

namespace {

bool is_strict_child_path(const std::filesystem::path& parent, const std::filesystem::path& child)
{
    auto parent_it = parent.begin();
    auto child_it = child.begin();

    for (; parent_it != parent.end(); ++parent_it, ++child_it) {
        if (child_it == child.end() || *child_it != *parent_it)
            return false;
    }

    return child_it != child.end();
}

}

void Manager::StorageCollection::VirtualAssetWriteSession::remove_outdated()
{
    if (m_removed_outdated)
        return;

    for (auto it = m_collection.m_storages.begin(); it != m_collection.m_storages.end();) {
        const auto asset_name = it->first.lexically_normal();
        if (is_strict_child_path(m_parent_asset_name, asset_name)
            && it->second->is_virtual_asset()
            && !m_updated_assets.contains(asset_name)) {
            it = m_collection.m_storages.erase(it);
            continue;
        }

        ++it;
    }

    m_removed_outdated = true;
}

Manager::StorageCollection::ReadSession::Iterator::Iterator(BaseIterator current, BaseIterator end, const Filter* filter)
    : m_current(current)
    , m_end(end)
    , m_filter(filter)
{
    skip_filtered();
}

Manager::StorageCollection::ReadSession::Iterator::value_type
Manager::StorageCollection::ReadSession::Iterator::operator*() const
{
    return {m_current->first, *m_current->second};
}

Manager::StorageCollection::ReadSession::Iterator&
Manager::StorageCollection::ReadSession::Iterator::operator++()
{
    ++m_current;
    skip_filtered();
    return *this;
}

Manager::StorageCollection::ReadSession::Iterator
Manager::StorageCollection::ReadSession::Iterator::operator++(int)
{
    auto previous = *this;
    ++(*this);
    return previous;
}

bool Manager::StorageCollection::ReadSession::Iterator::operator==(const Iterator& other) const
{
    return m_current == other.m_current;
}

bool Manager::StorageCollection::ReadSession::Iterator::operator!=(const Iterator& other) const
{
    return !(*this == other);
}

void Manager::StorageCollection::ReadSession::Iterator::skip_filtered()
{
    while (m_current != m_end && m_filter && *m_filter && !(*m_filter)(m_current->first, *m_current->second))
        ++m_current;
}

Manager::StorageCollection::ReadSession::ReadSession(StorageCollection& collection, Filter filter)
    : m_collection(collection)
    , m_filter(std::move(filter))
{
    std::shared_lock lock(collection.m_mutex);
    m_snapshot.reserve(collection.m_storages.size());
    for (auto& [asset_name, storage] : collection.m_storages) {
        if (!m_filter || m_filter(asset_name, *storage))
            m_snapshot.emplace_back(asset_name, storage);
    }
}

Manager::StorageCollection::ReadSession::Iterator Manager::StorageCollection::ReadSession::begin()
{
    return Iterator(m_snapshot.begin(), m_snapshot.end(), nullptr);
}

Manager::StorageCollection::ReadSession::Iterator Manager::StorageCollection::ReadSession::end()
{
    return Iterator(m_snapshot.end(), m_snapshot.end(), nullptr);
}

Manager::StorageCollection::ReadSession::Iterator
Manager::StorageCollection::ReadSession::find(const std::filesystem::path& path)
{
    const auto it = std::ranges::find_if(m_snapshot, [&](const auto& entry) {
        return entry.first == path;
    });
    return Iterator(it, m_snapshot.end(), nullptr);
}

Manager::StorageCollection::ReadSession Manager::StorageCollection::begin_read()
{
    return ReadSession(*this);
}

std::optional<std::reference_wrapper<Manager::Storage>> Manager::StorageCollection::get(const std::filesystem::path& path)
{
    std::shared_lock lock(m_mutex);
    const auto it = m_storages.find(path);
    if (it == m_storages.end())
        return std::nullopt;
    return *it->second;
}

std::optional<std::reference_wrapper<const Manager::Storage>> Manager::StorageCollection::get(const std::filesystem::path& path) const
{
    std::shared_lock lock(m_mutex);
    const auto it = m_storages.find(path);
    if (it == m_storages.end())
        return std::nullopt;
    return *it->second;
}

std::shared_ptr<Manager::Storage> Manager::StorageCollection::get_shared(const std::filesystem::path& path)
{
    std::shared_lock lock(m_mutex);
    const auto it = m_storages.find(path);
    if (it == m_storages.end())
        return {};
    return it->second;
}

Manager::AbstractFactoryCollection::AbstractFactoryCollection() = default;

bool Manager::AbstractFactoryCollection::has_type(const std::type_index& type_index) const
{
    std::scoped_lock lock(m_mutex);
    return m_types.left.find(type_index) != m_types.left.end();
}

bool Manager::AbstractFactoryCollection::has_type(const std::string& asset_type_string) const
{
    std::scoped_lock lock(m_mutex);
    return m_types.right.find(asset_type_string) != m_types.right.end();
}

const std::string& Manager::AbstractFactoryCollection::get_asset_type_string(const std::type_index& type_index) const
{
    std::scoped_lock lock(m_mutex);
    const auto it = m_types.left.find(type_index);
    if (it == m_types.left.end())
        throw std::runtime_error("No factory registered for requested asset type.");
    return it->second;
}

std::type_index Manager::AbstractFactoryCollection::get_type_index(const std::string& asset_type_string) const
{
    std::scoped_lock lock(m_mutex);
    const auto it = m_types.right.find(asset_type_string);
    if (it == m_types.right.end())
        throw std::runtime_error("No factory registered for asset type " + asset_type_string);
    return it->second;
}

Manager::AbstractFactory& Manager::AbstractFactoryCollection::get(const std::string& asset_type_string)
{
    std::scoped_lock lock(m_mutex);
    const auto type_it = m_types.right.find(asset_type_string);
    if (type_it == m_types.right.end())
        throw std::runtime_error("No factory registered for asset type " + asset_type_string);

    return m_factories.at(type_it->second);
}

Manager::FileSystemHandler::FileSystemHandler(
    const std::string& asset_meta_suffix,
    std::shared_ptr<detail::DependencyTracker> dependency_tracker)
    : m_asset_meta_suffix(asset_meta_suffix)
    , m_dependency_tracker(std::move(dependency_tracker))
{ }

void Manager::FileSystemHandler::execute_scan(const std::filesystem::path& root,
                                              std::shared_ptr<StorageCollection>& storages,
                                              AbstractFactoryCollection& factories,
                                              const MetaReader& read_meta)
{
    // A number of editors save by truncating and rewriting a file. During that
    // operation the file may have a valid stat result while its contents are
    // incomplete. Require edits to an already known file to remain unchanged
    // briefly before attempting to parse them.
    constexpr auto stability_delay = std::chrono::milliseconds(100);

    auto storage_write_session = storages->begin_write();
    std::unordered_set<std::filesystem::path> reloaded_meta_files;

    auto observe_known_file_change = [&](const detail::DependencyTracker::Dependency& dependency) -> std::optional<FileState> {
        const auto& path = dependency.path;

        std::error_code modification_error;
        const auto last_modified = std::filesystem::last_write_time(path, modification_error);
        if (modification_error) {
            spdlog::debug("Could not stat watched dependency {} while it is being updated: {}", path.string(), modification_error.message());
            return std::nullopt;
        }

        std::error_code size_error;
        const auto file_size = std::filesystem::file_size(path, size_error);
        if (size_error) {
            spdlog::debug("Could not read the size of watched dependency {} while it is being updated: {}", path.string(), size_error.message());
            return std::nullopt;
        }

        auto state_it = m_file_states.find(path);
        if (state_it == m_file_states.end()) {
            if (dependency.last_modified == last_modified && dependency.file_size == file_size) {
                m_file_states.insert_or_assign(path, FileState{last_modified, file_size, {}, {}, {}});
                return std::nullopt;
            }

            auto [inserted_it, inserted] = m_file_states.emplace(
                path,
                FileState{
                    dependency.last_modified,
                    dependency.file_size,
                    last_modified,
                    file_size,
                    std::chrono::steady_clock::now(),
                });
            (void)inserted_it;
            (void)inserted;
            return std::nullopt;
        }

        auto& state = state_it->second;
        if (state.last_modified == last_modified && state.file_size == file_size) {
            state.pending_last_modified.reset();
            state.pending_file_size.reset();
            return std::nullopt;
        }

        const bool same_pending_version = state.pending_last_modified == last_modified
                                       && state.pending_file_size == file_size;
        if (!same_pending_version) {
            state.pending_last_modified = last_modified;
            state.pending_file_size = file_size;
            state.pending_since = std::chrono::steady_clock::now();
            return std::nullopt;
        }

        if (std::chrono::steady_clock::now() - state.pending_since < stability_delay)
            return std::nullopt;

        return FileState{last_modified, file_size, {}, {}, {}};
    };

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (!is_asset_metafile(entry))
            continue;

        std::error_code modification_error;
        const auto last_modified = entry.last_write_time(modification_error);
        if (modification_error) {
            spdlog::debug("Could not stat asset metadata {} while it is being updated: {}", entry.path().string(), modification_error.message());
            continue;
        }

        std::error_code size_error;
        const auto file_size = entry.file_size(size_error);
        if (size_error) {
            spdlog::debug("Could not read the size of asset metadata {} while it is being updated: {}", entry.path().string(), size_error.message());
            continue;
        }

        auto state_it = m_file_states.find(entry.path());
        if (state_it != m_file_states.end()
            && state_it->second.last_modified == last_modified
            && state_it->second.file_size == file_size) {
            state_it->second.pending_last_modified.reset();
            state_it->second.pending_file_size.reset();
            continue;
        }

        if (state_it != m_file_states.end()) {
            auto& state = state_it->second;
            const bool same_pending_version = state.pending_last_modified == last_modified
                                           && state.pending_file_size == file_size;
            if (!same_pending_version) {
                state.pending_last_modified = last_modified;
                state.pending_file_size = file_size;
                state.pending_since = std::chrono::steady_clock::now();
                continue;
            }
            if (std::chrono::steady_clock::now() - state.pending_since < stability_delay)
                continue;
        }

        // Get asset name its path relative to root
        const auto relative_path = std::filesystem::relative(entry.path(), root);
        const auto asset_name    = generate_asset_name(relative_path);

        try {
            // Load metadata before changing storage state. Editors can expose a
            // truncated file briefly while replacing its contents.
            Meta meta = read_meta(entry.path());

            std::error_code final_modification_error;
            const auto final_last_modified = std::filesystem::last_write_time(entry.path(), final_modification_error);
            std::error_code final_size_error;
            const auto final_file_size = std::filesystem::file_size(entry.path(), final_size_error);
            if (final_modification_error || final_size_error
                || final_last_modified != last_modified || final_file_size != file_size) {
                spdlog::debug("Asset metadata {} changed while it was being read; retrying on the next scan", entry.path().string());
                continue;
            }

            auto& factory = factories.get(meta.asset_type());

            // Get or create storage only after a complete, stable metadata read.
            detail::FactoryContextBase context_base(root, relative_path, asset_name, m_dependency_tracker);
            auto& storage = storage_write_session.get_or_insert(asset_name, std::move(context_base));
            storage.set_meta(factory, std::move(meta), std::make_unique<VirtualAssetManager>(storages, asset_name));

            // Record only a version that parsed successfully and stayed stable.
            m_file_states.insert_or_assign(entry.path(), FileState{final_last_modified, final_file_size, {}, {}, {}});
            reloaded_meta_files.insert(entry.path());
        } catch (const std::exception& error) {
            std::error_code final_modification_error;
            const auto final_last_modified = std::filesystem::last_write_time(entry.path(), final_modification_error);
            std::error_code final_size_error;
            const auto final_file_size = std::filesystem::file_size(entry.path(), final_size_error);
            if (final_modification_error || final_size_error
                || final_last_modified != last_modified || final_file_size != file_size) {
                spdlog::debug("Could not read stable asset metadata {} because it changed during save: {}; retrying on the next scan",
                              entry.path().string(), error.what());
                continue;
            }

            // The file remained unchanged throughout both the debounce period
            // and the read, so this is a persistent error rather than evidence
            // of an in-progress save.
            throw;
        }
    }

    for (const auto& [asset_meta_path, dependencies] : m_dependency_tracker->all_dependencies()) {
        if (reloaded_meta_files.contains(asset_meta_path))
            continue;

        std::optional<std::pair<std::filesystem::path, FileState>> changed_dependency;
        for (const auto& dependency : dependencies) {
            if (const auto changed_state = observe_known_file_change(dependency)) {
                changed_dependency.emplace(dependency.path, *changed_state);
                break;
            }
        }

        if (!changed_dependency)
            continue;

        const auto relative_path = std::filesystem::relative(asset_meta_path, root);
        const auto asset_name    = generate_asset_name(relative_path);

        Meta meta = read_meta(asset_meta_path);
        auto& factory = factories.get(meta.asset_type());

        detail::FactoryContextBase context_base(root, relative_path, asset_name, m_dependency_tracker);
        auto& storage = storage_write_session.get_or_insert(asset_name, std::move(context_base));
        storage.set_meta(factory, std::move(meta), std::make_unique<VirtualAssetManager>(storages, asset_name));

        m_file_states.insert_or_assign(changed_dependency->first, changed_dependency->second);
    }
}

bool Manager::FileSystemHandler::is_asset_metafile(const std::filesystem::directory_entry& entry) const
{
    return entry.is_regular_file() && entry.path().filename().string().ends_with(m_asset_meta_suffix);
}

void Manager::root(const std::filesystem::path& path)
{
    m_root = path;
}

void Manager::root_via_ini(const std::filesystem::path& ini_path, const std::string& ini_namespace,
                           const std::string& ini_key)
{
    const mINI::INIFile file(ini_path.string());
    if (mINI::INIStructure ini; file.read(ini)) {
        if (ini.has(ini_namespace) && ini[ini_namespace].has(ini_key)) {
            root(ini[ini_namespace][ini_key]);
            return;
        }
        throw std::runtime_error("Could not find root in INI file");
    }
    throw std::runtime_error(std::format("Could not read INI file at {}", ini_path.string()));
}

const std::filesystem::path& Manager::root() const
{
    return m_root;
}

void Manager::scan_filesystem()
{
    m_file_system_handler.execute_scan(root(), m_storages, m_factories, m_meta_reader);
}

Manager::Storage& Manager::operator[](const std::filesystem::path& asset_name)
{
    auto storage = m_storages->get(asset_name);
    if (!storage)
        throw std::out_of_range("Asset is not loaded: " + asset_name.generic_string());
    return storage->get();
}

const Manager::Storage& Manager::operator[](const std::filesystem::path& asset_name) const
{
    auto storage = m_storages->get(asset_name);
    if (!storage)
        throw std::out_of_range("Asset is not loaded: " + asset_name.generic_string());
    return storage->get();
}

Manager::StorageCollection::ReadSession Manager::all()
{
    return m_storages->begin_read();
}

Manager::StorageCollection::ReadSession Manager::in_directory(const std::filesystem::path& path,
                                                              DirectoryIteration iteration)
{
    auto directory = path.lexically_normal();
    return StorageCollection::ReadSession(
        *m_storages,
        [directory = std::move(directory), iteration](const std::filesystem::path& asset_name, const Storage&) {
            auto asset_path = asset_name.lexically_normal();
            auto directory_it = directory.begin();
            auto asset_it = asset_path.begin();

            for (; directory_it != directory.end(); ++directory_it, ++asset_it) {
                if (asset_it == asset_path.end() || *asset_it != *directory_it)
                    return false;
            }

            if (asset_it == asset_path.end())
                return false;

            if (iteration == DirectoryIteration::Recursive)
                return true;

            ++asset_it;
            return asset_it == asset_path.end();
        });
}

Manager::StorageCollection::ReadSession Manager::of_type(const std::string& asset_type_string)
{
    return StorageCollection::ReadSession(
        *m_storages,
        [asset_type_string](const std::filesystem::path&, const Storage& storage) {
            return storage.asset_type() == asset_type_string;
        });
}

VirtualAssetManager::VirtualAssetManager(
    std::weak_ptr<Manager::StorageCollection> storages,
    const std::filesystem::path& parent_asset_name)
    : m_storages(std::move(storages))
    , m_parent_asset_name(parent_asset_name)
{ }

Manager::StorageCollection::VirtualAssetWriteSession VirtualAssetManager::begin_write()
{
    auto storages = m_storages.lock();
    if (!storages)
        throw std::runtime_error("Asset manager storage no longer exists.");

    return Manager::StorageCollection::VirtualAssetWriteSession(*storages, m_parent_asset_name);
}

}
