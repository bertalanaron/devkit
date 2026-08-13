#pragma once
#include <devkit/common/utils.h>

namespace dk::concurrency {

enum class Signal { Shutdown };

template <typename T>
class TaskQueue {
public:
	void push(const T& data)
	{
		{
			auto _ = std::lock_guard(m_mut);
			m_queue.push_back(data);
		}
		m_taskCount.fetch_add(1, std::memory_order_relaxed);
		m_sem.release();
	}

	std::expected<T, Signal> pop(std::stop_token stopToken) 
	{
		while (!stopToken.stop_requested()) {
			if (m_shutdown) 
				break;

			if (m_sem.try_acquire_for(std::chrono::milliseconds(100))) {
				auto _ = std::lock_guard(m_mut);
				T result = std::move(m_queue.front());
				m_queue.pop_front();
				return result;
			}
		}
		return std::unexpected(Signal::Shutdown);
	}

	// Indicates that job execution finished
	void done()
	{
		m_taskCount.fetch_sub(1, std::memory_order_relaxed);
		if (m_taskCount.load(std::memory_order_relaxed) == 0)
		{
			std::lock_guard lock(m_cvMut);
			m_cv.notify_all();
		}
	}

	// Blocks until all worker thread finished execution
	void wait() 
	{
		std::unique_lock lock(m_cvMut);
		m_cv.wait(lock, [&] { return m_taskCount.load(std::memory_order_relaxed) == 0; });
	}

	void shutdown() 
	{  
		m_shutdown = true;
	}

	~TaskQueue() { shutdown(); }

private:
	std::counting_semaphore<> m_sem{ 0 };
	std::mutex                m_mut{};
	std::mutex                m_cvMut{};
	std::deque<T>             m_queue{};

	std::atomic_int           m_taskCount = 0;
	std::condition_variable   m_cv;

	std::atomic_bool          m_shutdown = false;
};

} // dk::concurrency
