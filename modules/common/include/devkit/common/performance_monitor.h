#pragma once
#include <devkit/common/utils.h>

namespace dk::dbg {

class PerformanceMonitor 
	: private dk::common::SingletonBase<PerformanceMonitor>
{
private:
	using Timepoint = std::chrono::system_clock::time_point;

	struct Sample {
		Timepoint                start;
		std::optional<Timepoint> end;
	};

	struct Process {
	public:
		Process() = default;
		Process(int samples)
			: m_samples(samples)
		{ }

		Process operator+=(Sample&& sample)
		{

		}

	private:
		size_t              m_sampleCount   = 0;
		size_t              m_samplePointer = 0;
		std::vector<Sample> m_samples;

		std::chrono::duration<std::chrono::seconds> m_frequency;	
		std::chrono::duration<std::chrono::seconds> m_duration;	
	};

public:
	static void begin(const std::string& process, int samples)
	{
		auto it = instance().m_processes.find(process);
		if (it == instance().m_processes.end())
			it = instance().m_processes.insert(std::make_pair(process, Process(samples))).first;
		it->second += Sample(std::chrono::system_clock::now());
	}

	static void end(const std::string& process);

	template <typename Duration>
		requires(std::chrono::_Is_duration_v<Duration>)
	static long long duration(const std::string& process)
	{

	}

	template <typename Duration>
		requires(std::chrono::_Is_duration_v<Duration>)
	static long double frequency(const std::string& process)
	{

	}

private:
	std::unordered_map<std::string, Process> m_processes;

};

} // dk::dbg
