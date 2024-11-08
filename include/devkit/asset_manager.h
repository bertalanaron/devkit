#pragma once
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <any>
#include <future>
#include <expected>
#include <filesystem>

#include "devkit/util.h"
#include "log.h"

namespace NS_DEVKIT {

class AssetManager {
public:
	enum Policy { Sync, Defered, Async };
	enum class AssetStatus { OK = 0x00, INVALID_TYPE, INVALID_PATH };

private:
	using TypeID = const char*;
	using FTime = std::filesystem::file_time_type;

	template <typename Mthd>
	struct Handler {
		using Method = Mthd;

		template <typename T>
		Handler(Policy policy, std::function<T(const wchar_t*)> method)
			: m_policy(policy)
			, m_method(method)
			, m_type(typeid(T).name())
		{ }

		template <typename T>
		Handler(Policy policy, std::function<T&(const wchar_t*, std::any&)> method)
			: m_policy(policy)
			, m_method(method)
			, m_type(typeid(T).name())
		{ }

		auto operator()(const wchar_t* path, const auto& then, auto&... maybeAsset) const {
			TRACE("am.h()");
			auto method = transfrormMethod(path, then, maybeAsset...);

			if (m_policy == Async) {
				// Run asynchrounously
				return std::async(std::launch::async, method, path);
			}
			else if (m_policy == Defered) {
				// Run defered
				return std::async(std::launch::deferred, method, path);
			}
			else { // Synchrous case
				using ReturnType = decltype(method(path));
				std::promise<ReturnType> promise;
				try {
					promise.set_value(method(path));
				}
				catch (...) {
					promise.set_exception(std::current_exception());
				}
				return promise.get_future();
			}
		}

		TypeID type() const {
			return m_type;
		}

	private:
		Policy m_policy;
		Method m_method;
		TypeID m_type;

		auto transfrormMethod(const wchar_t* path, const auto& then, auto&... maybeAsset) const {
			if constexpr (sizeof...(maybeAsset) == 1)
				return [this, then, &maybeAsset...](const wchar_t* path) { 
				try { auto res = m_method(path, maybeAsset...); then(); return res; }
				catch(...) { then(); throw std::current_exception(); } };
			else 
				return [this, then](const wchar_t* path) { 
				try { auto res = m_method(path); then(); return res; }
				catch(...) { then(); throw std::current_exception(); } };
		}
	};

	using InitHandler = Handler<std::function<std::any(const wchar_t*)>>;
	using UpdateHandler = Handler<std::function<std::any(const wchar_t*, std::any&)>>;

	struct Asset {
		enum class State { OK = 0x00, WAITING, AVAILABLE };

		Asset() = default;

		void create(const InitHandler& handler, const wchar_t* path) {
			TRACE("am.a.c {}", (void*)this);
			m_type = handler.type();
			m_path = path;
			m_lastModified = std::filesystem::last_write_time(path);
			m_state = State::WAITING;

			auto callback = [this]() {
				TRACE("am.a init then {}", utf8(m_path));
				if (m_fut.valid())
					m_state = State::AVAILABLE;
				};
			m_fut = handler(m_path, std::move(callback));
		}

		template <typename T>
		T& get() {
			TRACE("am.a.g");
			if (m_state == State::WAITING)
				waitTillReady();

			localize();

			return std::any_cast<T&>(m_asset);
		}

		void update(const UpdateHandler& handler) {
			// Return if file hasn't been updated
			FTime writeTime = std::filesystem::last_write_time(m_path);
			if (writeTime == m_lastModified)
				return;

			TRACE("am.a.u");
			// If still initing or updating wait
			if (m_state == State::WAITING)
				waitTillReady();

			localize();

			// Call update handler
			m_state = State::WAITING;
			m_lastModified = writeTime;
			auto callback = [this]() {
				TRACE("am.a update then {}", utf8(m_path));
				if (m_fut.valid())
					m_state = State::AVAILABLE;
				};
			m_fut = handler(m_path, callback, m_asset);
		}

		TypeID type() const {
			return m_type;
		}

		bool ready() const {
			TRACE("am.a.r");
			using namespace std::chrono_literals;
			if (m_state != State::WAITING || !m_fut.valid())
				return false;
			return m_fut.wait_for(0ms) == std::future_status::ready;
		}

	private:
		TypeID					m_type;
		const wchar_t*			m_path;
		std::any				m_asset;
		std::future<std::any>	m_fut;
		FTime					m_lastModified;
		State					m_state;

		void waitTillReady() {
			TRACE("am.a.wtr {}", (void*)this);
			if (m_fut.valid())
				m_fut.wait();
			while (m_state == State::WAITING) { } // busy waiting for handler callback (provided in constructor)
		}

		void localize() {
			if (m_state == State::AVAILABLE) {
				m_asset = m_fut.get();
				m_state = State::OK;
			}
		}
	};

public:
	template <typename T>
	void attachInitHandler(std::wstring extension, T(*method)(const wchar_t*), Policy policy = Policy::Sync) {
		TRACE("am.aih {}", utf8(extension.c_str()));
		m_initHandlers.insert({ extension, InitHandler(policy, std::function<T(const wchar_t*)>(method)) });
	}

	void removeInitHandler(std::wstring extension) {
		TRACE("am.rih {}", utf8(extension.c_str()));
		m_initHandlers.erase(extension);
	}

	template <typename T>
	void attachUpdateHandler(std::wstring extension, void(T::*method)(const wchar_t*), Policy policy = Policy::Sync) {
		TRACE("am.auh {}", utf8(extension.c_str()));

		std::function<T&(const wchar_t*, std::any&)> transformed 
			= [method](const wchar_t* path, std::any& asset) -> T& {
			(std::any_cast<T&>(asset).*method)(path); return std::any_cast<T&>(asset); };
		m_updateHandlers.insert({ extension, UpdateHandler(policy, transformed) });
	}

	void removeUpdateHandler(std::wstring extension) {
		TRACE("am.ruh {}", utf8(extension.c_str()));
	}

	template <typename T>
	std::expected<std::reference_wrapper<T>, AssetStatus> getExp(const wchar_t* path) {
		TRACE("am.ge");
		auto it = m_assets.find(path);
		if (it == m_assets.end())
			return std::unexpected(AssetStatus::INVALID_PATH);

		auto& asset = it->second;
		if (asset.type() != typeid(T).name())
			return std::unexpected(AssetStatus::INVALID_TYPE);

		return asset.get<T>();
	}

	template <typename T>
	T& get(const wchar_t* path, auto... defaultVal) {
		TRACE("am.g");
		static_assert(sizeof...(defaultVal) < 2);
		static_assert((std::is_same_v<T, decltype(defaultVal)> && ...));

		if constexpr (sizeof...(defaultVal) == 1)
			return getExp<T>(path).value_or(defaultVal...);
		else
			return getExp<T>(path).value();
	}

	bool ready(const wchar_t* path) {
		TRACE("am.r");
		auto it = m_assets.find(path);
		if (it == m_assets.end())
			return false;
		return it->second.ready();
	}

	void synchronize(const wchar_t* path) {
		TRACE("am.s");

		std::vector<std::wstring> files = findFiles(path);
		for (auto& file : files)
			handleFile(file);
	}

private:
	std::unordered_map<std::wstring, InitHandler>	m_initHandlers{};
	std::unordered_map<std::wstring, UpdateHandler>	m_updateHandlers{};
	std::unordered_map<std::wstring, Asset>			m_assets{};

	void handleFile(const std::wstring& path) {
		TRACE("am.hf");
		auto ext = extension(path);

		auto assetIt = m_assets.find(path);
		if (assetIt == m_assets.end()) {
			auto handlerIt = m_initHandlers.find(ext);
			if (handlerIt == m_initHandlers.end())
				return;

			auto [iter, inserted] = m_assets.insert({path, {}});
			if (inserted)
				iter->second.create(handlerIt->second, iter->first.c_str());
		}
		else {
			auto handlerIt = m_updateHandlers.find(ext);
			if (handlerIt == m_updateHandlers.end())
				return;

			assetIt->second.update(handlerIt->second);
		}
	}

	static std::wstring extension(const std::wstring& path) {
		size_t pos = path.rfind('.');
		if (pos == std::wstring::npos)
			return L"";
		return path.substr(pos + 1);
	}

	static std::vector<std::wstring> findFiles(const wchar_t* path) {
		TRACE("am.ff");
		namespace fs = std::filesystem;

		std::vector<std::wstring> res{};
		try {
			for (const auto& entry : fs::recursive_directory_iterator(path)) {
				if (fs::is_regular_file(entry.path()))
					res.push_back(entry.path());
			}
		}
		catch (const fs::filesystem_error& e) {
			ERR("{}", e.what());
		}
		return res;
	}
};

}
