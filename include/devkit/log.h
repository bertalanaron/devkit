#pragma once
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <codecvt>
#include <mutex>

enum class LogLevel : unsigned { Error, Debug, Trace };

unsigned& logLevel() {
	static LogLevel level = LogLevel::Debug;
	return (unsigned&)level;
}

void logLevel(LogLevel level) {
	logLevel() = (unsigned)level;
}

std::ostream*& logStream() {
	static std::ostream* os = &std::cout;
	return os;
}

void logStream(std::ostream* os) {
	logStream() = os;
}

inline std::mutex g_logMut;
inline int g_nextThreadID = 0;
inline std::unordered_map<int, int> g_threadToShort{};

#ifdef DEBUG_THREADS
#define _LOG_THREAD_ID(os, level, format, ...)									\
	int threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());	\
	int shortThreadID = 0;														\
	auto threadIDIt = g_threadToShort.find(threadID);							\
	if (threadIDIt == g_threadToShort.end()) {									\
		shortThreadID = g_nextThreadID;											\
		g_threadToShort[threadID] = g_nextThreadID++;							\
	} else																		\
		shortThreadID = threadIDIt->second;										\
	std::print(os, "[{}]", shortThreadID);
#else
#define _LOG_THREAD_ID(...)
#endif

#define LOG(os, level, format, ...)																			\
	do {																									\
		if ((unsigned)level <= logLevel()) {																\
			std::lock_guard<std::mutex> guard(g_logMut);													\
			_LOG_THREAD_ID(os, level, format, __VA_ARGS__)													\
			auto now = std::chrono::system_clock::now();													\
			std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);								\
			std::tm now_tm = *std::localtime(&now_time_t);													\
			auto milliseconds																				\
				= std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;		\
			os << std::put_time(&now_tm, "[%Y-%m-%d %H:%M:%S");												\
			os << '.' << std::setw(3) << std::setfill('0') << milliseconds.count();							\
			std::print(os, format __VA_OPT__(,) __VA_ARGS__);												\
		}																									\
	} while (false)

#define ERR(format, ...)	LOG(*logStream(), LogLevel::Error,   "] Error " format "\n", __VA_ARGS__)

#ifdef _DEBUG
#define DBG(format, ...)	LOG(*logStream(), LogLevel::Debug,   "] Debug " format "\n", __VA_ARGS__)
#define TRACE(format, ...)	LOG(*logStream(), LogLevel::Trace,   "] Trace " format "\n", __VA_ARGS__)
#else
#define DBG(...)
#define TRACE(...)
#endif

std::string utf8(const wchar_t* wcstr) {
	std::wstring wstr(wcstr);
	std::string narrow;
	for (wchar_t wc : wstr) {
		if (wc <= 0x7F) {
			narrow.push_back(static_cast<char>(wc));
		}
		else if (wc <= 0x7FF) {
			narrow.push_back(static_cast<char>(0xC0 | ((wc >> 6) & 0x1F)));
			narrow.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
		}
		else if (wc <= 0xFFFF) {
			narrow.push_back(static_cast<char>(0xE0 | ((wc >> 12) & 0x0F)));
			narrow.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
			narrow.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
		}
		else {
			narrow.push_back(static_cast<char>(0xF0 | ((wc >> 18) & 0x07)));
			narrow.push_back(static_cast<char>(0x80 | ((wc >> 12) & 0x3F)));
			narrow.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
			narrow.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
		}
	}

	return narrow;
}
