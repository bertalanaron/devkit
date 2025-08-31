#pragma once
#include <devkit/common/utils.h>

namespace dk::io {

// TODO: update shouldn't resolve deferred tasks
class AssetManager {
public:
	enum ExecutionPolicy { Sync, Async, Deferred };

private:
	using path_t = std::filesystem::path;

	template <typename K, typename T>
	using map_t = std::unordered_map<K, T>;

	template <typename T>
	using set_t = std::unordered_set<T>;

	template <typename T>
	using uptr_t = std::unique_ptr<T>;

	template <typename T>
	using opt_t = std::optional<T>;

	static path_t absolute(const path_t& path)
	{
		return std::filesystem::weakly_canonical(std::filesystem::absolute(path));
	}

private:
	class GenericHandler {
	private:
		template <typename T>
		using load_fn_t = std::function<std::shared_ptr<T>(const std::string&)>;

		template <typename T>
		using update_fn_t = std::function<void(T&,const std::string&)>;

		template <typename T>
		using save_fn_t = std::function<void(const T&,const std::string&)>;

	public:
		template <typename T>
		struct LoadFn {
		public:
			std::shared_ptr<T> operator()(const path_t& path) const
			{ return m_value(path.string()); }

			LoadFn(auto fnc) 
				: m_value(std::move(normalizeLoad<T>(fnc))) 
			{ }
		private:
			load_fn_t<T> m_value;
		};

		template <typename T>
		struct UpdateFn {
		public:
			void operator()(T& object, const path_t& path) const
			{ m_value(object, path.string()); }

			UpdateFn(auto fnc) 
				: m_value(normalizeUpdate<T>(fnc)) 
			{ }
		private:
			update_fn_t<T> m_value;
		};

		template <typename T>
		struct SaveFn {
		public:
			void operator()(const T& object, const path_t& path) const
			{ return m_value(object, path.string()); }

			SaveFn(auto fnc) 
				: m_value(normalizeSave<T>(fnc)) 
			{ }
		private:
			save_fn_t<T> m_value;
		};

	private:
		// Normalize possible path types to std::filesystem::path
		template <typename P>
		static P convertPath(const std::string& p);
		template <>
		static path_t convertPath<path_t>(const std::string& p) 
		{ return p; }
		template <>
		static const std::string& convertPath<const std::string&>(const std::string& p) 
		{ return p; }
		template <>
		static const char* convertPath<const char*>(const std::string& p) 
		{ return p.c_str(); }

		// Normalize possible return types of load functions
		template<typename T>
		static std::shared_ptr<T> normalizeReturn(T* ptr) 
		{ return std::shared_ptr<T>(ptr); }
		template<typename T>
		static std::shared_ptr<T> normalizeReturn(std::shared_ptr<T>&& ptr) 
		{ return ptr; }
		template<typename T>
		static std::shared_ptr<T> normalizeReturn(T&& obj) 
		{ return std::make_shared<T>(std::move(obj)); }

		// Convert possible load function signitures to a uniform type
		template <typename T>
		static load_fn_t<T> normalizeLoad(auto fnc)
		{
			using Ret  = dk::common::function_traits<decltype(fnc)>::result_type;
			using Arg0 = dk::common::function_traits<decltype(fnc)>::template arg<0>::type;

			return [fnc](const std::string& path) { return normalizeReturn<T>(std::move(fnc(convertPath<Arg0>(path)))); };
		}

		// Convert possible update function signitures to a uniform type
		template <typename T>
		static update_fn_t<T> normalizeUpdate(auto fnc) 
		{
			using FT = dk::common::function_traits<decltype(fnc)>;
			using Path = FT::template arg<FT::arity - 1>::type;

			return [fnc](T& object, const std::string& path) {
				std::apply(fnc, std::make_tuple(std::ref(object), convertPath<Path>(path)));
			};
		}

		// Convert possible save function signitures to a uniform type
		template <typename T>
		static save_fn_t<T> normalizeSave(auto fnc) 
		{
			using FT = dk::common::function_traits<decltype(fnc)>;
			using Path = FT::template arg<FT::arity - 1>::type;

			return [fnc](const T& object, const std::string& path) {
				std::apply(fnc, std::make_tuple(std::ref(object), convertPath<Path>(path)));
			};
		}

	private:
		struct AssetBase {
			path_t m_path;

			AssetBase(const path_t& absolutePath)
				: m_path(absolutePath)
			{
				DK_ASSERT((absolutePath.is_absolute(), "AssetBase requires absolute path"));
			}

			virtual ~AssetBase() { }
		};

		struct AssetTypeBase {
			ExecutionPolicy m_policy;

			AssetTypeBase(ExecutionPolicy policy)
				: m_policy(policy)
			{ }

			virtual std::unique_ptr<AssetBase> load(const path_t&) const = 0;
			virtual void update(AssetBase*, const path_t&)         const = 0;
			virtual void save(AssetBase*, const path_t&)           const = 0;
		};

	public:
		template <typename T>
		struct Asset : public AssetBase {
		private:
			using object_storage_t = std::variant<std::shared_ptr<T>, std::future<std::shared_ptr<T>>>;
			using opt_task_t       = std::optional<std::packaged_task<std::shared_ptr<T>(const path_t&)>>;
			using opt_worker_t     = std::optional<std::jthread>;

		public:
			Asset(const path_t& absolutePath, object_storage_t&& storage, opt_task_t&& task, opt_worker_t&& worker)
				: AssetBase(absolutePath)
				, m_storage(std::move(storage))
				, m_task(std::move(task))
				, m_worker(std::move(worker))
				, m_lock(1)
			{ }

			Asset(const path_t& absolutePath, T&& object)
				: AssetBase(absolutePath)
				, m_storage(std::make_shared<T>(std::move(object)))
				, m_lock(1)
			{ }

			T& get()
			{
				resolve();
				auto& ptr = std::get<std::shared_ptr<T>>(m_storage);
				return *ptr;
			}

			std::shared_ptr<T> getShared()
			{
				resolve();
				return std::get<std::shared_ptr<T>>(m_storage);
			}

			std::weak_ptr<T> getWeak()
			{
				resolve();
				return std::get<std::shared_ptr<T>>(m_storage);
			}

		private:
			object_storage_t      m_storage;
			opt_task_t            m_task;
			opt_worker_t          m_worker;
			std::binary_semaphore m_lock;

			void resolve()
			{
				m_lock.acquire();
				std::visit(dk::common::overload{
					[&](const std::shared_ptr<T>&) { /* Sync */ },
					[&](std::future<std::shared_ptr<T>>& fut) { 
						if (m_task.has_value())
							// Deferred
							m_task.value()(m_path);
						m_storage = fut.get();
					},
					}, m_storage);
				m_lock.release();
			}
		};

	private:
		template <typename T>
		class AssetType : public AssetTypeBase {
		public:
			std::unique_ptr<AssetBase> load(const path_t& path) const override
			{
				std::unique_ptr<AssetBase> asset;

				if (m_policy == Sync) {
					asset = std::make_unique<Asset<T>>(path, m_load(path), std::nullopt, std::nullopt);
				}
				else if (m_policy == Deferred) {
					// Create task
					std::packaged_task<std::shared_ptr<T>(const path_t&)> task(m_load);
					asset = std::make_unique<Asset<T>>(path, std::move(task.get_future()), std::move(task), std::nullopt);
				} 
				else { // Async
					// Create task
					std::packaged_task<std::shared_ptr<T>(const path_t&)> task(m_load);
					// Start worker thread
					auto future = task.get_future();
					std::jthread worker(std::move(task), path);
					asset = std::make_unique<Asset<T>>(path, std::move(future), std::nullopt, std::move(worker));
				}
				return std::move(asset);
			}

			void update(AssetBase* asset, const path_t& path) const override
			{
				if (!m_update.has_value())
					return;
				Asset<T>* cast_asset = dynamic_cast<Asset<T>*>(asset);
				m_update.value()(cast_asset->get(), path);
			}

			void save(AssetBase* asset, const path_t& path) const override
			{
				if (!m_save.has_value())
					return;
				Asset<T>* cast_asset = dynamic_cast<Asset<T>*>(asset);
				m_save.value()(cast_asset->get(), path);
			}

			AssetType(LoadFn<T> load, std::optional<UpdateFn<T>> opt_update, std::optional<SaveFn<T>> opt_save, ExecutionPolicy policy)
				: AssetTypeBase(policy)
				, m_load(load)
				, m_update(opt_update)
				, m_save(opt_save)
			{ }

		private:
			LoadFn<T>                  m_load;
			std::optional<UpdateFn<T>> m_update;
			std::optional<SaveFn<T>>   m_save;
		};

	public:
		// @brief Register asset type and provide handlers
		template <typename T>
		void type(
			const std::vector<path_t>& extensions, 
			LoadFn<T>                  load, 
			std::optional<UpdateFn<T>> opt_update = std::nullopt, 
			std::optional<SaveFn<T>>   opt_save   = std::nullopt, 
			ExecutionPolicy            policy = Sync) 
		{
			// Create LUT of extensions
			for (const auto& extension : extensions)
				m_extensionToType[tryAddDotToExtension(extension)] = typeid(T);
			
			m_assetTypes[typeid(T)] = std::make_unique<AssetType<T>>(load, opt_update, opt_save, policy);
			m_typeNames[typeid(T)]  = typeid(T).name();
		}

		template <typename T>
		void typeName(const std::string& name)
		{
			m_typeNames.at(typeid(T)) = name;
		}

		void typeName(const std::type_index& type, const std::string& name)
		{
			m_typeNames.at(type) = name;
		}

		template <typename T>
		const std::string& typeName()
		{
			return m_typeNames.at(typeid(T));
		}

		const std::string& typeName(const std::type_index& type)
		{
			return m_typeNames.at(type);
		}

		std::optional<std::type_index> typeOf(const path_t& extension)
		{
			auto it = m_extensionToType.find(extension);
			if (it == m_extensionToType.end())
				return std::nullopt;
			return it->second;
		}

		// @brief Load object from path
		bool load(const path_t& absolutePath) 
		{
			DK_ASSERT((absolutePath.is_absolute(), "GenericHandler::load requires absolute path"));
			const path_t extension = absolutePath.extension();
			auto typeOfExtensionIt = m_extensionToType.find(extension);
			if (typeOfExtensionIt == m_extensionToType.end())
				return false;
			m_assets[absolutePath] = m_assetTypes[typeOfExtensionIt->second.value()]->load(absolutePath);
			return true;
		}

		// @brief Update object at path if save method was provided
		void update(const path_t& absolutePath)
		{
			DK_ASSERT((absolutePath.is_absolute(), "GenericHandler::update requires absolute path"));
			const path_t extension = absolutePath.extension();
			auto typeOfExtensionIt = m_extensionToType.find(extension);
			if (typeOfExtensionIt == m_extensionToType.end())
				return;
			m_assetTypes[typeOfExtensionIt->second.value()]->update(m_assets.at(absolutePath).get(), absolutePath);
		}

		// @brief Save object at path if save method was provided
		void save(const path_t& absolutePath)
		{
			DK_ASSERT((absolutePath.is_absolute(), "GenericHandler::save requires absolute path"));
			const path_t extension = absolutePath.extension();
			auto typeOfExtensionIt = m_extensionToType.find(extension);
			if (typeOfExtensionIt == m_extensionToType.end())
				return;
			m_assetTypes[typeOfExtensionIt->second.value()]->save(m_assets.at(absolutePath).get(), absolutePath);
		}

		void unload(const path_t& absolutePath)
		{
			m_assets.erase(absolutePath);
		}

		template <typename T>
		void create(const path_t& absolutePath, T&& object)
		{
			// Validate type
			if (!m_assetTypes.contains(typeid(T)))
				throw std::runtime_error("Unknown type");
			m_assets.emplace(absolutePath, std::make_unique<Asset<T>>(absolutePath, std::move(object)));
			save(absolutePath);
		}

	public:
		bool contains(const path_t& absolutePath) const
		{ return m_assets.contains(absolutePath); }

		template <typename T>
		Asset<T>* get(const path_t& absolutePath) const
		{
			DK_ASSERT((absolutePath.is_absolute(), "GenericHandler::get requires absolute path"));
			return dynamic_cast<Asset<T>*>(m_assets.at(absolutePath).get());
		}

		template <typename T>
		auto each() 
		{
			using namespace std::ranges;

			return m_assets
				| views::filter([&](const auto& pair) {
					return dynamic_cast<Asset<T>*>(pair.second.get()) != nullptr;
				})
				| views::transform([&](const auto& pair) {
					return std::make_pair(pair.first, std::ref(dynamic_cast<Asset<T>*>(pair.second.get())->get()));
				});
		}

		template <typename T>
		auto ceach() const
		{
			using namespace std::ranges;
			return m_assets
				| views::filter([&](const auto& pair) {
					return dynamic_cast<Asset<T>*>(pair.second.get()) != nullptr;
				})
				| views::transform([&](const auto& pair) {
					return std::make_pair(pair.first, std::cref(dynamic_cast<Asset<T>*>(pair.second.get())->get()));
				});
		}

	private:
		map_t<path_t         , uptr_t<AssetBase>>      m_assets;
		map_t<path_t         , opt_t<std::type_index>> m_extensionToType;
		map_t<std::type_index, uptr_t<AssetTypeBase>>  m_assetTypes;
		map_t<std::type_index, std::string>            m_typeNames;

	private:
		static path_t tryAddDotToExtension(const path_t& extension)
		{
			if (extension.empty())
				return extension;
			auto str = extension.string();
			if (str.front() == '.')
				return extension;
			return path_t("." + str);
		}
	};

	class Directory {
	private:
		struct FileState {
			long long int                   tag = 0;
			std::filesystem::file_time_type lastWriteTime;
		};

	public:
		Directory()                   = default;
		Directory(const path_t& path, bool isRecursive)
			: m_path(path)
			, m_isRecursive(isRecursive)
		{ }

		void synchronize(const path_t& root, GenericHandler& handler)
		{
			++m_syncTag;

			// Iterate over contained directory entries, optionally recursively
			if (m_isRecursive) {
				for (const auto& entry : std::filesystem::recursive_directory_iterator(m_path))
					handleDirectoryEntry(entry, root, handler);
			}
			else {
				for (const auto& entry : std::filesystem::directory_iterator(m_path))
					handleDirectoryEntry(entry, root, handler);
			}

			// Handle removed files
			for (auto it = m_states.begin(); it != m_states.end(); ) {
				if (it->second.tag != m_syncTag)
				{
					handler.unload(it->first);
					const auto opt_assetType = handler.typeOf(it->first.extension());
					spdlog::trace("{} removed \"{}\"", 
						handler.typeName(opt_assetType.value()), 
						std::filesystem::relative(it->first, root).string());
					it = m_states.erase(it);
				}
				else
					++it;
			}

			// Synchronize subdirectories
			for (auto& subdir : m_subdirectories)
				subdir.second.synchronize(root, handler);
		}

		void addSubdirectory(const path_t& path, bool isRecursive)
		{
			if (m_isRecursive) 
				throw std::runtime_error("Can't add subdirectory to recursive directory");
			if (m_subdirectories.contains(path))
				return;

			for (auto& subdir : m_subdirectories) {
				// Parent of allready existing subdir
				if (common::fs::is_parent(subdir.second.m_path, path)) {
					// Place directory between this, and subdir in the graph
					if (isRecursive)
						throw std::runtime_error("Can't add recursive directory above existing directory");
					Directory newDir(path, isRecursive);
					auto node = m_subdirectories.extract(subdir.second.m_path);
					newDir.m_subdirectories.insert(std::move(node));
					m_subdirectories.emplace(path, std::move(newDir));
					return;
				}
				// Child of subdir
				if (common::fs::is_parent(path, subdir.second.m_path)) {
					subdir.second.addSubdirectory(path, isRecursive);
					return;
				}
			}

			Directory newDir(path, isRecursive);
			m_subdirectories.emplace(path, std::move(newDir));
		}

		void removeSubdirectory(const path_t& path, GenericHandler& handler)
		{
			auto it = m_subdirectories.find(path);
			if (it != m_subdirectories.end()) {
				it->second.close(handler);
				m_subdirectories.erase(it);
			}
		}

		void close(GenericHandler& handler)
		{
			for (const auto& file : m_files)
				handler.unload(file);
			for (auto& [_, subdir] : m_subdirectories)
				subdir.close(handler);
		}

		// Called when files are changed by the asset manager e.g.: create()
		void updateFileState(const path_t& absolutePath)
		{
			auto it = m_states.find(absolutePath);
			if (it == m_states.end())
				it = m_states.insert({ absolutePath, FileState() }).first;
			updateFileState(it, absolutePath);
		}

		const path_t& path() const
		{ return m_path; }

	private:
		path_t m_path;
		bool   m_isRecursive;

		set_t<path_t>            m_files;
		map_t<path_t, FileState> m_states;
		map_t<path_t, Directory> m_subdirectories;

		long long int m_syncTag = 0;

		void handleDirectoryEntry(const std::filesystem::directory_entry& entry, const path_t& root, GenericHandler& handler)
		{
			if (!std::filesystem::is_regular_file(entry.path()))
				return;

			// Get file path and write time
			path_t                          path          = entry.path();
			path_t                          absolutePath  = absolute(path);
			std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(path);

			// Create or update asset
			// Leave as is if file existed previously and hasn't changed
			const auto opt_assetType = handler.typeOf(path.extension());
			if (!opt_assetType.has_value())
				return;
			if (!handler.contains(absolutePath)) { // New file
				if (handler.load(absolutePath)) {
					m_files.emplace(absolutePath);
					updateFileState(absolutePath);
					spdlog::trace("{} found \"{}\"", 
						handler.typeName(opt_assetType.value()), 
						std::filesystem::relative(path, root).string());
				}
			}
			else { // Not new
				const auto prevLastWrite = m_states.at(absolutePath).lastWriteTime;
				const bool changed       = prevLastWrite < lastWriteTime;
				if (changed) { // File changed
					handler.update(absolutePath);
					spdlog::trace("{} changed \"{}\"", 
						handler.typeName(opt_assetType.value()), 
						std::filesystem::relative(path, root).string());
				}
				updateFileState(absolutePath);
			}
		}

		void updateFileState(map_t<path_t, FileState>::iterator it, const path_t& absolutePath)
		{
			it->second.lastWriteTime = std::filesystem::last_write_time(absolutePath);
			it->second.tag = m_syncTag;
		}
	};

public:
	// @brief Register asset type and provide handlers
	template <typename T>
	void type(
		const path_t&                              extension, 
		GenericHandler::LoadFn<T>                  load, 
		std::optional<GenericHandler::UpdateFn<T>> opt_update = std::nullopt, 
		std::optional<GenericHandler::SaveFn<T>>   opt_save   = std::nullopt, 
		ExecutionPolicy                            policy = Sync) 
	{
		m_handler.type({ extension }, load, opt_update, opt_save, policy);
	}

	// @brief Register asset type and provide handlers
	template <typename T>
	void type(
		std::initializer_list<path_t>         extensions,
		GenericHandler::LoadFn<T>                  load, 
		std::optional<GenericHandler::UpdateFn<T>> opt_update = std::nullopt, 
		std::optional<GenericHandler::SaveFn<T>>   opt_save   = std::nullopt, 
		ExecutionPolicy                            policy = Sync) 
	{
		m_handler.type(extensions, load, opt_update, opt_save, policy);
	}

	template <typename T>
	void typeName(const std::string& name) 
	{ m_handler.typeName<T>(name); }

	void synchronize()
	{
		m_rootdir->synchronize(root(), m_handler);
	}

	void root(const path_t& path, bool isRecursive = false)
	{
		if (m_rootdir) {
			m_rootdir->close(m_handler);
			m_rootdir.reset();
		}
		m_rootdir = std::make_unique<Directory>(absolute(path), isRecursive);
	}

	const path_t& root() const
	{ return m_rootdir->path(); }

	path_t relativeToRoot(const path_t& path) const
	{ return std::filesystem::relative(path, root()); }

	void watch(const path_t& path, bool isRecursive = false)
	{
		const path_t absolutePath = m_rootdir->path() / path.relative_path();
		m_rootdir->addSubdirectory(absolutePath, isRecursive);
	}

	void stopWatching(const path_t& path)
	{
		const path_t absolutePath = m_rootdir->path() / path.relative_path();
		m_rootdir->removeSubdirectory(absolutePath, m_handler);
	}

	bool contains(const path_t& path) const
	{
		const path_t absolutePath = m_rootdir->path() / path.relative_path();
		return m_handler.contains(absolutePath);
	}

	template <typename T>
	T& get(const path_t& path) const
	{
		const path_t absolutePath = m_rootdir->path() / path.relative_path();
		return m_handler.get<T>(absolutePath)->get();
	}

	template <typename T>
	auto getMultiple(const std::convertible_to<path_t> auto&... paths)
	{
		return std::make_tuple(std::ref(get<T>(paths))...);
	}

	template <typename T>
	std::shared_ptr<T> getShared(const path_t& path) 
	{
		const path_t absolutePath = m_rootdir->path() / path.relative_path();
		return m_handler.get<T>(absolutePath)->getShared();
	}

	template <typename T>
	std::weak_ptr<T> getWeak(const path_t& path) 
	{
		const path_t absolutePath = m_rootdir->path() / path.relative_path();
		return m_handler.get<T>(absolutePath)->getWeak();
	}

	template <typename T>
	auto getMultipleWeak(const std::convertible_to<path_t> auto&... paths)
	{
		return std::make_tuple(getWeak<T>(paths)...);
	}

	template <typename T>
	auto each()
	{ return m_handler.each<T>(); }

	template <typename T>
	auto each(const path_t& directory)
	{ 
		return each<T>()
			| std::ranges::views::filter([&,dir=directory](const auto& pair) {
				const path_t absolutePath = m_rootdir->path() / dir.relative_path();
				return dk::common::fs::is_parent(pair.first, absolutePath);
			}); 
	}

	template <typename T>
	auto ceach() const
	{ return m_handler.ceach<T>(); }

	template <typename T>
	auto ceach(const path_t& directory) const
	{ 
		return ceach<T>()
			| std::ranges::views::filter([&](const auto& pair) {
				const auto relativePath = relativeToRoot(pair.first).lexically_normal().string();
				return relativePath.contains(directory.string());
			}); 
	}

	template <typename T>
	void create(const path_t& path, T&& object)
	{
		const path_t absolutePath = m_rootdir->path() / path.relative_path();
		m_handler.create<T>(absolutePath, std::move(object));
		m_rootdir->updateFileState(absolutePath);
	}

private:
	GenericHandler    m_handler;
	uptr_t<Directory> m_rootdir;

private:
};

template <typename T>
struct FileStream {
	static T load(const std::string& path)
	{
		std::ifstream ifs(path);
		if (!ifs.is_open())
			throw std::runtime_error("Couldn't open file");
		T result;
		result << ifs;
		return result;
	}
	static void update(T& object, const std::string& path)
	{
		std::ifstream ifs(path);
		if (!ifs.is_open())
			throw std::runtime_error("Couldn't open file");
		T result;
		result << ifs;
		object = result;
	}
	static void save(const T& object, const std::string& path)
	{
		std::ofstream ofs(path);
		if (!ofs.is_open())
			throw std::runtime_error("Couldn't open file");
		ofs << object;
	}
};

}
