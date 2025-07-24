#pragma once
#include <devkit/common/utils.h>

namespace dk::io {

// TODO: update shouldn't resolve deferred tasks
class AssetManager {
public:
	enum ExecutionPolicy { Sync, Async, Deferred };

private:
	using path_t = std::filesystem::path;

	template <typename T>
	using load_fn_t = std::function<std::shared_ptr<T>(const std::string&)>;

	template <typename T>
	using update_fn_t = std::function<void(T&,const std::string&)>;
	
	template <typename T>
	using save_fn_t = std::function<void(const T&,const std::string&)>;

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

		std::shared_ptr<T> getShared()
		{
			resolve();
			return std::get<std::shared_ptr<T>>(m_storage);
		}

		T& get()
		{
			resolve();
			auto& ptr = std::get<std::shared_ptr<T>>(m_storage);
			return *ptr;
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

	class Directory {
	private:
		using file_map_t = std::unordered_map<path_t, std::filesystem::file_time_type>;

	public:
		Directory() = default;
		Directory(const path_t& absolutePath)
			: m_path(absolutePath)
		{ 
			DK_ASSERT((absolutePath.is_absolute(), "Directory requires absolute path"));
		}

		void synchronize(AssetManager& owner) 
		{
			namespace fs = std::filesystem;

			std::unordered_set<path_t> notDeleted;

			// Iterate over files and init or update assets with matching handlers
			for (const auto& filePath : fs::recursive_directory_iterator(m_path)) {
				if (!fs::is_regular_file(filePath.path()))
					continue;

				// Get file path and write time
				path_t             filePathStr   = filePath.path();
				fs::file_time_type lastWriteTime = fs::last_write_time(filePath.path());
				// File wasn't deleted since last sync
				notDeleted.insert(filePathStr);

				// Create or update asset
				// Leave as is if file existed previously and hasn't changed
				auto assetIt = owner.m_assets.find(filePathStr);
				if (!owner.hasAsset(filePathStr)) { // New file
					if (owner.load(filePathStr)) {
						m_files.emplace(filePathStr, lastWriteTime);
						spdlog::trace("Found asset at: {}", filePathStr.string());
					}
				}
				else { // Not new
					const auto prevLastWriteIt = m_files.find(filePathStr);
					const bool changed         = prevLastWriteIt->second < lastWriteTime;
					if (changed) { // File changed
						spdlog::trace("Asset changed at: {}", filePathStr.string());
						prevLastWriteIt->second = lastWriteTime;
						owner.update(filePathStr);
					}
				}
			}

			// Erase deleted assets
			for (auto it = m_files.begin(); it != m_files.end();) {
				if (notDeleted.contains(it->first))
					++it;
				else {
					spdlog::trace("Asset deleted at: {}", it->first.string());
					it = m_files.erase(it);
				}
			}
		}

		void unload(AssetManager& owner)
		{
			// Erase assets from manager
			for (const auto& [path, _] : m_files) {
				auto it = owner.m_assets.find(path);
				if (it != owner.m_assets.end())
					owner.m_assets.erase(it);
			}
		}

		void saveAll(AssetManager& owner)
		{
			// Erase assets from manager
			for (auto& [path, writeTime] : m_files) {
				auto it = owner.m_assets.find(path);
				if (it != owner.m_assets.end()) {
					owner.save(path);
					writeTime = std::filesystem::last_write_time(path);
				}
			}
		}

		void addPath(const path_t& absolutePath)
		{
			DK_ASSERT((absolutePath.is_absolute(), "Directory requires absolute path"));
			std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(absolutePath);
			m_files.emplace(absolutePath, writeTime);
		}

		bool contains(const path_t& absolutePath) const
		{
			DK_ASSERT((absolutePath.is_absolute(), "Directory requires absolute path"));
			return m_files.contains(absolutePath);
		}

		const path_t& path() const
		{ return m_path; }

	private:
		path_t     m_path;
		file_map_t m_files;
	};

	/*class Directory2 {
	private:
		struct FileState {
			std::filesystem::file_time_type lastWriteTime;
		};

	public:
		void synchronize(AssetManager& manager) const
		{

		}

		bool contains(const path_t& absolutePath) const
		{ return m_files.contains(absolutePath); }

		const path_t& path() const
		{ return m_path; }

	private:
		path_t m_path;
		bool   m_isRecursive;

		std::unordered_set<path_t>            m_files;
		std::unordered_map<path_t, FileState> m_states;
	};*/

public:
	// @brief Register asset type and provide handlers
	template <typename T>
	void type(
		const path_t&              extension, 
		LoadFn<T>                  load, 
		std::optional<UpdateFn<T>> opt_update = std::nullopt, 
		std::optional<SaveFn<T>>   opt_save   = std::nullopt, 
		ExecutionPolicy            policy = Sync) 
	{
		m_extensionToType[tryAddDotToExtension(extension)] = typeid(T);
		m_assetTypes[typeid(T)] = std::make_unique<AssetType<T>>(load, opt_update, opt_save, policy);
	}

	void loadFrom(const path_t& path) 
	{
		const path_t absolutePath = std::filesystem::absolute(path);
		auto it = m_directories.find(absolutePath);
		if (it == m_directories.end())
			it = m_directories.emplace(absolutePath, absolutePath).first;
		it->second.synchronize(*this);
	}

	void unloadFrom(const path_t& path)
	{
		const path_t absolutePath = std::filesystem::absolute(path);
		auto it = m_directories.find(absolutePath);
		if (it != m_directories.end()) {
			it->second.unload(*this);
			m_directories.erase(it);
		}
	}

	void saveAllIn(const path_t& path)
	{
		const path_t absolutePath = std::filesystem::absolute(path);
		auto it = m_directories.find(absolutePath);
		if (it != m_directories.end()) {
			it->second.saveAll(*this);
		}
	}

	template <typename T>
	void create(const path_t& path, T&& object)
	{
		// Validate type
		if (!m_assetTypes.contains(typeid(std::decay_t<T>)))
			throw std::runtime_error("Unknown type");

		// Get containing directory
		const path_t absolutePath = std::filesystem::absolute(path);
		auto directoryIt = findContainingDirectory(absolutePath);

		// Create asset
		m_assets.emplace(absolutePath, std::make_unique<Asset<std::decay_t<T>>>(absolutePath, std::move(object)));
		save(absolutePath);
		directoryIt->second.addPath(absolutePath);
	}

	bool contains(const path_t& path) const
	{
		const path_t absolutePath = std::filesystem::absolute(path);
		auto it = findContainingDirectory(absolutePath);
		if (it == m_directories.end())
			return false;
		return it->second.contains(absolutePath);
	}

	template <typename T>
	std::shared_ptr<T> getShared(const path_t& path) 
	{
		const path_t absolutePath = std::filesystem::absolute(path);
		return dynamic_cast<Asset<T>*>(m_assets.at(absolutePath).get())->getShared();
	}

	template <typename T>
	T& get(const path_t& path) const
	{
		const path_t absolutePath = std::filesystem::absolute(path);
		return dynamic_cast<Asset<T>*>(m_assets.at(absolutePath).get())->get();
	}

private:
	std::unordered_map<path_t, std::unique_ptr<AssetBase>>              m_assets;
	std::unordered_map<path_t, std::optional<std::type_index>>          m_extensionToType;
	std::unordered_map<std::type_index, std::unique_ptr<AssetTypeBase>> m_assetTypes;
	std::unordered_map<path_t, Directory>                               m_directories;

private:
	bool hasAsset(const path_t& path) const
	{ return m_assets.contains(path); }

	// @brief Load object from path
	bool load(const path_t& absolutePath) 
	{
		DK_ASSERT((absolutePath.is_absolute(), "AssetManager::load requires absolute path"));
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
		DK_ASSERT((absolutePath.is_absolute(), "AssetManager::update requires absolute path"));
		const path_t extension = absolutePath.extension();
		auto typeOfExtensionIt = m_extensionToType.find(extension);
		if (typeOfExtensionIt == m_extensionToType.end())
			return;
		m_assetTypes[typeOfExtensionIt->second.value()]->update(m_assets.at(absolutePath).get(), absolutePath);
	}

public:
	// @brief Save object at path if save method was provided
	void save(const path_t& absolutePath)
	{
		DK_ASSERT((absolutePath.is_absolute(), "AssetManager::save requires absolute path"));
		const path_t extension = absolutePath.extension();
		auto typeOfExtensionIt = m_extensionToType.find(extension);
		if (typeOfExtensionIt == m_extensionToType.end())
			return;
		m_assetTypes[typeOfExtensionIt->second.value()]->save(m_assets.at(absolutePath).get(), absolutePath);
	}

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
	static bool isPathParentOf(const path_t& parent, const path_t& child) 
	{
		std::error_code ec;
		auto absParent = std::filesystem::weakly_canonical(parent, ec);
		auto absChild  = std::filesystem::weakly_canonical(child, ec);
		if (ec || absParent.empty() || absChild.empty())
			return false;
		auto relative = std::filesystem::relative(absChild, absParent, ec);
		if (ec || relative.empty())
			return false;
		return *relative.begin() != "..";
	}

	static path_t tryAddDotToExtension(const path_t& extension)
	{
		if (extension.empty())
			return extension;
		auto str = extension.string();
		if (str.front() == '.')
			return extension;
		return path_t("." + str);
	}

	auto findContainingDirectory(const path_t& absolutePath)
	{
		DK_ASSERT((absolutePath.is_absolute(), "AssetManager::findContainingDirectory requires absolute path"));
		for (auto it = m_directories.begin(); it != m_directories.end(); ++it) {
			if (isPathParentOf(it->second.path(), absolutePath))
				return it;
		}
		return m_directories.end();
	}

	auto findContainingDirectory(const path_t& absolutePath) const
	{
		DK_ASSERT((absolutePath.is_absolute(), "AssetManager::findContainingDirectory requires absolute path"));
		for (auto it = m_directories.cbegin(); it != m_directories.cend(); ++it) {
			if (isPathParentOf(it->second.path(), absolutePath))
				return it;
		}
		return m_directories.end();
	}
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
