export module MultithreadUtils;

import std;

export struct MultithreadUtils
{
	static constexpr size_t m_uMaxThreadsCount = 32;

	static constexpr size_t GetCacheLineSize()
	{
		return std::hardware_destructive_interference_size;
	}

	static constexpr size_t GetMaxConcurrentThreadsCount()
	{
		return std::min(m_uMaxThreadsCount, static_cast<size_t>(std::thread::hardware_concurrency()));
	}
};