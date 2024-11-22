#pragma once
#include <functional>
#include <any>
#include <future>
#include <optional>
#include <expected>
#include <filesystem>
#include <semaphore>

// Enable mocking of filesystem
#ifndef ASSET_MANAGER_FILE_SYSTEM
#define ASSET_MANAGER_FILE_SYSTEM std::filesystem
#endif
namespace AssetManager_filesystem = ASSET_MANAGER_FILE_SYSTEM;

namespace NS_DEVKIT {

template <typename D, typename Iter>
class IteratorBase {
public:
	using iter = Iter;
	IteratorBase(Iter it, Iter end) 
		: m_it(it)
		, m_end(end) 
	{ }

	D& operator++() { 
		++m_it;
		return static_cast<D&>(*this); 
	}
	D operator++(int) { D newIt(m_it); return ++newIt; }

	bool operator==(const D& other) const { return m_it == other.m_it; }
	bool operator!=(const D& other) const { return m_it != other.m_it; }
protected:
	Iter m_it;
	Iter m_end;
};

class AssetManager {
public:
	enum class Execution { Sync = 0x0, Async = 0x1, Deferred = 0x2 };

	enum class AssetReturnStatus { Ok = 0x00, NotFound, TypeMismatch };

private:
	template <typename F>
	struct Extension {
		const wchar_t* extension;
		Execution	   policy;
		F&&			   func;
		Extension(const wchar_t* _extension, F&& _func, Execution _policy = Execution::Sync)
			: extension(_extension), func(std::forward<F&&>(_func)), policy(_policy) 
		{ }
	};

private:
	template <typename T>
	using PathToTMap = std::unordered_map<std::wstring, T>;

private:
	// Mostly same as std::any with the notable difference of 
	// allowing T to be a type with deleted copy constructor. 
	struct MoveOnlyAny {
		MoveOnlyAny(uintptr_t type) 
			: m_storage(nullptr)
			, m_type(type) 
		{ }
		MoveOnlyAny() { }
		MoveOnlyAny(MoveOnlyAny&& other) noexcept
			: m_storage(other.m_storage)
			, m_type(other.m_type)
		{
			other.m_storage = nullptr;
			other.m_type = reinterpret_cast<uintptr_t>(nullptr);
		}

		template <typename T, typename... Args>
		void emplace(Args&&... args) 
		{
			static_assert(std::move_constructible<T>, "Type T needs to be move constructable.");

			reset();
			m_storage = new char[sizeof(T)];
			new (m_storage) T(std::forward<Args&&>(args)...);
			m_type = reinterpret_cast<uintptr_t>(&typeid(std::decay_t<T>));
			m_destructor = [this] { get<T>().~T(); };
		}

		template <typename T>
		T& get() 
		{
			if (*typeInfo() != typeid(std::decay_t<T>))
				throw std::bad_any_cast{};
			return *reinterpret_cast<T*>(m_storage);
		}

		template <typename T>
		const T& get() const 
		{
			if (!is_type<T>())
				throw std::bad_any_cast{};
			return *reinterpret_cast<T*>(m_storage);
		}

		bool has_value() const {
			return m_storage != nullptr;
		}

		template <typename T>
		bool is_type() const {
			if (m_type == 0)
				return false;
			return *typeInfo() == typeid(std::decay_t<T>);
		}

		~MoveOnlyAny() { reset(); }
	private:
		void*	  m_storage  = nullptr;
		uintptr_t m_type	 = reinterpret_cast<uintptr_t>(nullptr);
		std::function<void()> m_destructor{};

		void reset() {
			if (!m_storage)
				return;

			m_destructor();
			delete m_storage;

			m_destructor = {};
			m_storage = nullptr;
			m_type = reinterpret_cast<uintptr_t>(nullptr);
		}

		const std::type_info* typeInfo() const {
			return reinterpret_cast<const type_info*>(m_type);
		}
	};

private:
	struct Initialize {
		Initialize(const Initialize&) = default;
		Initialize(Initialize&&) = default;
		template <typename Functor>
			requires(not std::is_same_v<std::decay_t<Functor>, Initialize>)
		Initialize(Functor&& func)
			: m_func{[func = std::forward<Functor&&>(func)](MoveOnlyAny& storage, const wchar_t* path) 
				{ storage.emplace<decltype(func(path))>(func(path)); }}
		{ }
		template <typename T>
		Initialize(T (*func)(const wchar_t*))
			: m_func([func](MoveOnlyAny& storage, const wchar_t* path) { storage.emplace<T>(func(path)); })
		{ }

		void operator()(MoveOnlyAny& storage, const wchar_t* path) {
			m_func(storage, path);
		}
	private:
		std::function<void(MoveOnlyAny&, const wchar_t*)> m_func;
	};

	struct Update {
		Update(const Update&) = default;
		Update(Update&&) = default;
		template <typename T, typename Ret>
		Update(std::function<Ret(T&, const wchar_t*)> func)
			: m_func{[func](MoveOnlyAny& storage, const wchar_t* path) 
				{ func(storage.get<T>(), path); }}
		{ }
		template <typename T>
		Update(auto (T::*func)(const wchar_t*))
			: m_func([func](MoveOnlyAny& storage, const wchar_t* path) { (storage.get<T>().*func)(path); })
		{ }

		void operator()(MoveOnlyAny& storage, const wchar_t* path) {
			m_func(storage, path);
		}
	private:
		std::function<void(MoveOnlyAny&, const wchar_t*)> m_func;
	};

	template <class T>
		requires(std::is_same_v<T, Initialize> || std::is_same_v<T, Update>)
	struct Handler {
		template <typename U>
		Handler(U&& func, Execution policy)
			: m_functor(std::forward<U&&>(func))
			, m_policy(policy)
		{ }

		std::future<void> operator()(MoveOnlyAny& storage, const wchar_t* path) {
			if (m_policy == Execution::Sync) {
				std::promise<void> promise;
				try { // Call functor and pass exceptions to future
					m_functor(storage, path);
				}
				catch (...) {
					promise.set_exception(std::current_exception());
				}
				return promise.get_future();
			}
			else { // Async or Deferred
				return std::async((std::launch)m_policy, m_functor, std::ref(storage), path);
			}
		}
	private:
		T		  m_functor;
		Execution m_policy;
	};

private:
	class Asset {
	public:
		Asset(MoveOnlyAny&& any)
			: m_storage(std::move(any))
			, m_fut(std::nullopt)
			, m_semaphore(1)
		{ }

		template <typename T>
		T& get() { 
			return m_storage.get<T>(); 
		}

		void tryResolve() {
			m_semaphore.acquire();

			if (!unresolved())
				return m_semaphore.release();

			m_fut.value().wait();
			m_fut.reset();

			m_semaphore.release();
		}

		bool unresolved() const {
			return m_fut.has_value() 
				&& std::future_status::ready != m_fut.value().wait_for(std::chrono::milliseconds(0));
		}

		template <typename T>
		bool is_type() const {
			return m_storage.is_type<T>();
		}

		bool has_value() const {
			return m_storage.has_value();
		}

		template<typename T>
		void handle(Handler<T>& handler, const wchar_t* path) {
			m_fut.emplace(handler(m_storage, path));
		}

	private:
		MoveOnlyAny						 m_storage;
		std::optional<std::future<void>> m_fut;
		std::binary_semaphore			 m_semaphore;
	};

private:
	template <typename T>
	struct AssetCollection {
		struct iterator : public IteratorBase<iterator, PathToTMap<Asset>::iterator> {
			using Base = IteratorBase<iterator, PathToTMap<Asset>::iterator>;
		public:
			iterator(Base::iter it, Base::iter end) : Base(it, end) {
				while (Base::m_it != Base::m_end && !Base::m_it->second.is_type<T>())
					++Base::m_it;
			}

			std::pair<const std::wstring&, T&> operator*() {
				// Resolve future if unresolved
				Base::m_it->second.tryResolve();

				return { Base::m_it->first, Base::m_it->second.get<T>() };
			}

			iterator& operator++() { 
				do { ++Base::m_it; } 
				while (Base::m_it != Base::m_end && !Base::m_it->second.is_type<T>()); 
				return *this; 
			}
		};

		AssetCollection(PathToTMap<Asset>& assets) : m_assets(assets) { }

		iterator begin() { return iterator(m_assets.begin(), m_assets.end()); }

		iterator end() { return iterator(m_assets.end(), m_assets.end()); }
	private:
		PathToTMap<Asset>& m_assets;
	};

private:
	class Directory {
	public:
		Directory(AssetManager& manager, const wchar_t* path)
			: m_assetManager(manager)
			, m_path(path)
		{ }

		template <typename T>
			requires(std::constructible_from<Initialize, T> && not std::constructible_from<Update, T>)
		void assing(const wchar_t* extension, T&& function, Execution policy = Execution::Sync) {
			uintptr_t typeInfo = reinterpret_cast<uintptr_t>(&typeid(std::decay_t<decltype(function(nullptr))>));
			m_typeInfos.insert({ extension, typeInfo });
			m_initHandlers.insert({ extension, Handler<Initialize>(std::forward<T&&>(function), policy)});
		}

		template <typename T>
			requires(std::constructible_from<Update, T>)
		void assing(const wchar_t* extension, T&& function, Execution policy = Execution::Sync) {
			m_updateHandlers.insert({ extension, Handler<Update>(std::forward<T&&>(function), policy)});
		}

		template <typename... Ts>
			requires((std::constructible_from<Initialize, Ts> || std::constructible_from<Update, Ts>) && ...)
		void assing(Extension<Ts>&&... extensionHandlers) {
			(assing(extensionHandlers.extension, std::forward<Ts&&>(extensionHandlers.func), extensionHandlers.policy), ...);
		}

		template <typename F>
		Extension<F> ext(const wchar_t* path, F&& func, Execution policy = Execution::Sync) {
			return m_assetManager.ext(path, std::forward<F&&>(func), policy);
		}

		template <typename T>
		std::expected<std::reference_wrapper<T>, AssetReturnStatus> get_exp(const wchar_t* path) {
			// Find asset or return NotFound
			auto assetIt = m_assets.find(path);
			if (assetIt == m_assets.end())
				return std::unexpected(AssetReturnStatus::NotFound);

			// Resolve future if unresolved
			assetIt->second.tryResolve();

			// Check for type mismatch
			if (!assetIt->second.is_type<T>())
				return std::unexpected(AssetReturnStatus::TypeMismatch);

			// Return asset
			return assetIt->second.get<T>();
		}

		template <typename T>
		T& get(const wchar_t* path) {
			return get_exp<T>(path).value();
		}

		// TODO: getAll<T>()
		template <typename T>
		AssetCollection<T> getAll() { return AssetCollection<T>(m_assets); }

		unsigned synchronize() {
			namespace fs = AssetManager_filesystem;
			++m_syncCount;
			unsigned synchronizedCount = 0;

			// Iterate over files and init or update assets with matching handlers
			for (const auto& filePath : fs::recursive_directory_iterator(m_path)) {
				if (!fs::is_regular_file(filePath.path()))
					continue;

				// Set sync stamp of asset to current sync count (used to find deleted files)
				auto assetStampIt = m_assetSyncStamps.find(filePath.path().wstring());
				if (assetStampIt != m_assetSyncStamps.end())
					assetStampIt->second = m_syncCount;

				Time lastWriteTime = fs::last_write_time(filePath.path());
				if (tryHandleFile(filePath.path().wstring(), lastWriteTime)) {
					// Asset initialized or updated successfully
					++synchronizedCount;
					continue;
				}
				continue;
			}

			// Handle deleted files
			auto assetIt = m_assets.begin();
			while (assetIt != m_assets.end()) {
				if (m_assetSyncStamps.at(assetIt->first) == m_syncCount) {
					++assetIt;
					continue; // Not delted
				}

				// Remove asset
				m_assetWriteTime.erase(assetIt->first);
				m_assetSyncStamps.erase(assetIt->first);
				assetIt = m_assets.erase(assetIt);
			}

			return synchronizedCount;
		}

	private:
		using Time = AssetManager_filesystem::file_time_type;

		AssetManager&					m_assetManager;
		std::wstring					m_path;
		unsigned						m_syncCount = 0;

		PathToTMap<Handler<Initialize>> m_initHandlers{};
		PathToTMap<Handler<Update>>	    m_updateHandlers{};
		PathToTMap<uintptr_t>			m_typeInfos{};

		PathToTMap<Asset>				m_assets{};
		PathToTMap<Time>				m_assetWriteTime{};
		PathToTMap<unsigned>			m_assetSyncStamps{};

	private:
		bool tryHandleFile(const std::wstring& path, Time writeTime) {
			auto ext = fileExtension(path);
			// Return if extension doesn't have init handler
			auto initHandlerIt = m_initHandlers.find(ext.data());
			if (initHandlerIt == m_initHandlers.end())
				return false;

			// Find or create asset storage
			auto assetIt = m_assets.find(path);
			if (assetIt == m_assets.end()) {
				uintptr_t type = m_typeInfos.at(ext.data());
				assetIt = m_assets.emplace(path, MoveOnlyAny{ type }).first;
			}

			// Asset has unresolved future
			if (assetIt->second.unresolved())
				return false;

			// Get path string with extended lifetime
			const auto& storedPath = assetIt->first;

			if (!assetIt->second.has_value()) {
				// Initialize asset and set write time
				m_assetWriteTime[storedPath] = writeTime;
				m_assetSyncStamps[storedPath] = m_syncCount;
				assetIt->second.handle(initHandlerIt->second, storedPath.c_str());
				return true;
			}
			else {
				// Return if update times match
				auto writeTimeIt = m_assetWriteTime.find(path);
				if (writeTimeIt == m_assetWriteTime.end() || writeTimeIt->second == writeTime)
					return false;

				// Find update handler and return if it doesn't exists
				auto updateHandlerIt = m_updateHandlers.find(ext.data());
				if (updateHandlerIt == m_updateHandlers.end())
					return false;

				// Update asset and write time
				m_assetWriteTime[storedPath] = writeTime;
				assetIt->second.handle(updateHandlerIt->second, storedPath.c_str());
				return true;
			}
		}
	};

public:
	template <typename... Fs>
		requires((std::constructible_from<Initialize, Fs> || std::constructible_from<Update, Fs>) && ...)
	Directory& directory(std::wstring&& path, Extension<Fs>&&... extensionHandlers) {
		// Find or create directory
		Directory* dir;
		auto it = m_directories.find(path);
		if (it != m_directories.end())
			dir = &it->second;
		else
			dir = &m_directories.insert({ std::move(path), { *this, path.c_str() }}).first->second;

		// Assign handlers and return
		dir->assing(std::forward<Extension<Fs>&&>(extensionHandlers)...);
		return *dir;
	}

	// Create: T(const wchar_t*)
	// Update: void(T&,const wchar_t*) or void T::(const wchar_t*)
	template <typename F>
	Extension<F> ext(const wchar_t* path, F&& func, Execution policy = Execution::Sync) {
		return Extension<F>{ path, std::forward<F&&>(func), policy };
	}

private:
	PathToTMap<Directory> m_directories{};

private:
	static const std::wstring_view fileExtension(const std::wstring& filePath) {
		size_t pos = filePath.rfind('.');
		if (pos == std::wstring::npos)
			return L"";
		return std::wstring_view(filePath.begin() + pos, filePath.end());
	}
};

}
