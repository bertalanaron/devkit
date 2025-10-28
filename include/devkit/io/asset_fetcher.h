#pragma once
#include <devkit/io/asset_manager.h>
#include <functional>
#include <filesystem>
#include <unordered_map>

namespace dk::io {

template <typename T>
class AssetFetcher {
public:
	using path_t = std::filesystem::path;
	using loader_fn_t = std::function<T(AssetFetcher&, const path_t&)>;

	// @brief Create a simple asset fetcher with a subdirectory root
	// @param manager Reference to the AssetManager
	// @param root Subdirectory path relative to the manager's root
	AssetFetcher(AssetManager& manager, const path_t& root)
		: assets(manager)
		, m_root(root.relative_path())
		, m_loader(std::nullopt)
	{ }

	// @brief Create an asset fetcher with a custom loader function
	// @param manager Reference to the AssetManager
	// @param root Subdirectory path relative to the manager's root
	// @param loader Custom function to build asset from multiple sources
	AssetFetcher(AssetManager& manager, const path_t& root, loader_fn_t loader)
		: assets(manager)
		, m_root(root.relative_path())
		, m_loader(std::move(loader))
	{ }

	// @brief Fetch asset by path using operator[]
	// @param path Path relative to the fetcher's root
	// @return Reference to the loaded asset
	T& operator[](const path_t& path)
	{
		const path_t fullPath = m_root / path.relative_path();
		
		// If custom loader is provided, use it
		if (m_loader.has_value()) {
			// Check if we already created this asset
			auto it = m_customAssets.find(fullPath);
			if (it != m_customAssets.end()) {
				return it->second;
			}
			
			// Create asset using custom loader and cache it
			T asset = m_loader.value()(*this, path);
			auto [inserted_it, success] = m_customAssets.emplace(fullPath, std::move(asset));
			return inserted_it->second;
		}
		
		// Otherwise, use the manager's get method
		return assets.get<T>(fullPath);
	}

	// @brief Get the root path of this fetcher
	// @return The root path relative to the manager's root
	const path_t& root() const
	{
		return m_root;
	}

	// Public reference to the underlying AssetManager
	AssetManager& assets;

private:
	path_t m_root;
	std::optional<loader_fn_t> m_loader;
	std::unordered_map<path_t, T> m_customAssets;
};

}
