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
		std::shared_ptr<T> operator()(const std::string& path) const
		{ return m_value(path); }

		LoadFn(auto fnc) 
			: m_value(std::move(normalizeLoad<T>(fnc))) 
		{ }
	private:
		load_fn_t<T> m_value;
	};

	template <typename T>
	struct UpdateFn {
	public:
		void operator()(T& object, const std::string& path) const
		{ m_value(object, path); }

		UpdateFn(auto fnc) 
			: m_value(normalizeUpdate<T>(fnc)) 
		{ }
	private:
		update_fn_t<T> m_value;
	};

	template <typename T>
	struct SaveFn {
	public:
		void operator()(const T& object, const std::string& path) const
		{ return m_value(object, path); }

		SaveFn(auto fnc) 
			: m_value(normalizeSave<T>(fnc)) 
		{ }
	private:
		save_fn_t<T> m_value;
	};

	struct AssetBase {
		std::string m_path;

		AssetBase(const std::string& path)
			: m_path(path)
		{ }

		virtual ~AssetBase() { }
	};

	struct AssetTypeBase {
		ExecutionPolicy m_policy;

		AssetTypeBase(ExecutionPolicy policy)
			: m_policy(policy)
		{ }

		virtual std::unique_ptr<AssetBase> load(const std::string&) const = 0;
		virtual void update(AssetBase*, const std::string&)         const = 0;
		virtual void save(AssetBase*, const std::string&)           const = 0;
	};

	template <typename T>
	struct Asset : public AssetBase {
	private:
		using object_storage_t = std::variant<std::shared_ptr<T>, std::future<std::shared_ptr<T>>>;
		using opt_task_t       = std::optional<std::packaged_task<std::shared_ptr<T>(const std::string&)>>;
		using opt_worker_t     = std::optional<std::jthread>;

	public:
		Asset(const std::string& path, object_storage_t&& storage, opt_task_t&& task, opt_worker_t&& worker)
			: AssetBase(path)
			, m_storage(std::move(storage))
			, m_task(std::move(task))
			, m_worker(std::move(worker))
			, m_lock(1)
		{ }

		Asset(const std::string& path, T&& object)
			: AssetBase(path)
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
		std::unique_ptr<AssetBase> load(const std::string& path) const override
		{
			std::unique_ptr<AssetBase> asset;

			if (m_policy == Sync) {
				asset = std::make_unique<Asset<T>>(path, m_load(path), std::nullopt, std::nullopt);
			}
			else if (m_policy == Deferred) {
				// Create task
				std::packaged_task<std::shared_ptr<T>(const std::string&)> task(m_load);
				asset = std::make_unique<Asset<T>>(path, std::move(task.get_future()), std::move(task), std::nullopt);
			} 
			else { // Async
				// Create task
				std::packaged_task<std::shared_ptr<T>(const std::string&)> task(m_load);
				// Start worker thread
				auto future = task.get_future();
				std::jthread worker(std::move(task), path);
				asset = std::make_unique<Asset<T>>(path, std::move(future), std::nullopt, std::move(worker));
			}
			return std::move(asset);
		}

		void update(AssetBase* asset, const std::string& path) const override
		{
			if (!m_update.has_value())
				return;
			Asset<T>* cast_asset = dynamic_cast<Asset<T>*>(asset);
			m_update.value()(cast_asset->get(), path);
		}
		
		void save(AssetBase* asset, const std::string& path) const override
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
		using file_map_t = std::unordered_map<std::string, std::filesystem::file_time_type>;

	public:
		Directory() = default;
		Directory(const std::string& path)
			: m_path(path)
		{ }

		void synchronize(AssetManager& owner) 
		{
			namespace fs = std::filesystem;

			std::unordered_set<std::string> notDeleted;

			// Iterate over files and init or update assets with matching handlers
			for (const auto& filePath : fs::recursive_directory_iterator(m_path)) {
				if (!fs::is_regular_file(filePath.path()))
					continue;

				// Get file path and write time
				std::string        filePathStr   = filePath.path().string();
				fs::file_time_type lastWriteTime = fs::last_write_time(filePath.path());
				// File wasn't deleted since last sync
				notDeleted.insert(filePathStr);

				// Create or update asset
				// Leave as is if file existed previously and hasn't changed
				auto assetIt = owner.m_assets.find(filePathStr);
				if (!owner.hasAsset(filePathStr)) { // New file
					if (owner.load(filePathStr)) {
						m_files.emplace(filePathStr, lastWriteTime);
						spdlog::trace("Found asset at: {}", filePathStr);
					}
				}
				else { // Not new
					const auto prevLastWriteIt = m_files.find(filePathStr);
					const bool changed         = prevLastWriteIt->second < lastWriteTime;
					if (changed) { // File changed
						spdlog::trace("Asset changed at: {}", filePathStr);
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
					spdlog::trace("Asset deleted at: {}", it->first);
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

		void addPath(const std::string& path)
		{
			std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(path);
			m_files.emplace(path, writeTime);
		}

		bool contains(const std::string& path) const
		{
			return m_files.contains(path);
		}

	private:
		std::string m_path;
		file_map_t  m_files;
	};

public:
	// @brief Register asset type and provide handlers
	template <typename T>
	void type(
		const std::string&         extension, 
		LoadFn<T>                  load, 
		std::optional<UpdateFn<T>> opt_update = std::nullopt, 
		std::optional<SaveFn<T>>   opt_save   = std::nullopt, 
		ExecutionPolicy            policy = Sync) 
	{
		m_extensionToType[extension] = typeid(T);
		m_assetTypes[typeid(T)] = std::make_unique<AssetType<T>>(load, opt_update, opt_save, policy);
	}

	void loadFrom(const std::string& path) 
	{
		auto it = m_directories.find(path);
		if (it == m_directories.end())
			it = m_directories.emplace(path, path).first;
		it->second.synchronize(*this);
	}

	void unloadFrom(const std::string& path)
	{
		auto it = m_directories.find(path);
		if (it != m_directories.end()) {
			it->second.unload(*this);
			m_directories.erase(it);
		}
	}

	void saveAllIn(const std::string& path)
	{
		auto it = m_directories.find(path);
		if (it != m_directories.end()) {
			it->second.saveAll(*this);
		}
	}

	template <typename T>
	void create(const std::string& directory, const std::string& name, T&& object)
	{
		if (!m_assetTypes.contains(typeid(std::decay_t<T>)))
			throw std::runtime_error("Unknown type");
		std::string path = directory + name;
		m_assets.emplace(path, std::make_unique<Asset<std::decay_t<T>>>(path, std::move(object)));
		save(path);
		m_directories.at(directory).addPath(path);
	}

	bool contains(const std::string& directory, const std::string& name) const
	{
		auto it = m_directories.find(directory);
		if (it == m_directories.end())
			return false;
		return m_directories.at(directory).contains(directory + name);
	}

	template <typename T>
	std::shared_ptr<T> getShared(const std::string& path) 
	{
		return dynamic_cast<Asset<T>*>(m_assets.at(path).get())->getShared();
	}

	template <typename T>
	T& get(const std::string& path) const
	{
		return dynamic_cast<Asset<T>*>(m_assets.at(path).get())->get();
	}

private:
	std::unordered_map<std::string, std::unique_ptr<AssetBase>>         m_assets;
	std::unordered_map<std::string, std::optional<std::type_index>>     m_extensionToType;
	std::unordered_map<std::type_index, std::unique_ptr<AssetTypeBase>> m_assetTypes;
	std::unordered_map<std::string, Directory>                          m_directories;

private:
	// @brief data/hello.txt -> txt
	static std::string fileExtension(const std::string& filePath) 
	{
		size_t pos = filePath.rfind('.');
		if (pos == std::string::npos)
			return "";
		return std::string(filePath.begin() + pos + 1, filePath.end());
	}

	bool hasAsset(const std::string& path) const
	{ return m_assets.contains(path); }

	// @brief Load object from path
	bool load(const std::string& path) 
	{
		const std::string extension = fileExtension(path);
		auto typeOfExtensionIt = m_extensionToType.find(extension);
		if (typeOfExtensionIt == m_extensionToType.end())
			return false;
		m_assets[path] = m_assetTypes[typeOfExtensionIt->second.value()]->load(path);
		return true;
	}

	// @brief Update object at path if save method was provided
	void update(const std::string& path)
	{
		const std::string extension = fileExtension(path);
		auto typeOfExtensionIt = m_extensionToType.find(extension);
		if (typeOfExtensionIt == m_extensionToType.end())
			return;
		m_assetTypes[typeOfExtensionIt->second.value()]->update(m_assets.at(path).get(), path);
	}

public:
	// @brief Save object at path if save method was provided
	void save(const std::string& path)
	{
		const std::string extension = fileExtension(path);
		auto typeOfExtensionIt = m_extensionToType.find(extension);
		if (typeOfExtensionIt == m_extensionToType.end())
			return;
		m_assetTypes[typeOfExtensionIt->second.value()]->save(m_assets.at(path).get(), path);
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
