#include "ThreadPool.h"

#include <algorithm>
#include <chrono>
#include <vector>

namespace Engine {

	ThreadPool::ThreadPool(unsigned workers)
	{
		Start(workers);
	}

	ThreadPool::~ThreadPool()
	{
		Shutdown();
	}

	void ThreadPool::Start(unsigned workers)
	{
		Shutdown();

		if (workers == 0) {
			unsigned hc = std::thread::hardware_concurrency();
			if (hc == 0) hc = 4;
			// Leave one logical core for the main / render thread.
			workers = hc > 1 ? hc - 1 : 1;
		}

		m_stop = false;
		m_workers.reserve(workers);
		for (unsigned i = 0; i < workers; ++i) {
			m_workers.emplace_back([this]() { WorkerLoop(); });
		}
	}

	void ThreadPool::Shutdown()
	{
		{
			std::lock_guard lock(m_mutex);
			if (m_stop && m_workers.empty()) return;
			m_stop = true;
		}
		m_cv.notify_all();
		for (auto& t : m_workers) {
			if (t.joinable()) t.join();
		}
		m_workers.clear();
		std::queue<std::function<void()>> empty;
		{
			std::lock_guard lock(m_mutex);
			std::swap(m_tasks, empty);
		}
	}

	void ThreadPool::WorkerLoop()
	{
		for (;;) {
			std::function<void()> job;
			{
				std::unique_lock lock(m_mutex);
				m_cv.wait(lock, [this]() { return m_stop || !m_tasks.empty(); });
				if (m_stop && m_tasks.empty()) return;
				job = std::move(m_tasks.front());
				m_tasks.pop();
			}
			job();
		}
	}

	bool ThreadPool::TryRunOneTask()
	{
		std::function<void()> job;
		{
			std::lock_guard lock(m_mutex);
			if (m_tasks.empty()) return false;
			job = std::move(m_tasks.front());
			m_tasks.pop();
		}
		job();
		return true;
	}

	void ThreadPool::ParallelFor(int count, int minPerTask, const std::function<void(int begin, int end)>& rangeFn)
	{
		if (count <= 0) return;
		if (!rangeFn) return;

		minPerTask = std::max(1, minPerTask);

		// Serial fast path: tiny work or pool not running.
		if (m_workers.empty() || count <= minPerTask) {
			rangeFn(0, count);
			return;
		}

		const int workers   = static_cast<int>(m_workers.size());
		const int nChunks   = std::min(count, workers + 1); // include caller as a worker
		int       chunkSize = (count + nChunks - 1) / nChunks;
		chunkSize           = std::max(chunkSize, minPerTask);

		struct Range {
			int b, e;
		};
		std::vector<Range> ranges;
		ranges.reserve(static_cast<size_t>(nChunks));
		for (int b = 0; b < count; b += chunkSize) {
			ranges.push_back({b, std::min(b + chunkSize, count)});
		}

		if (ranges.size() == 1) {
			rangeFn(ranges[0].b, ranges[0].e);
			return;
		}

		// Last range runs on the calling thread; the rest go to the pool.
		// While waiting, the caller helps drain the queue — prevents deadlock
		// when ParallelFor is nested (e.g. skin entities → skin mesh parts).
		std::vector<std::future<void>> futures;
		futures.reserve(ranges.size() - 1);
		for (size_t i = 0; i + 1 < ranges.size(); ++i) {
			const int b = ranges[i].b;
			const int e = ranges[i].e;
			futures.push_back(Enqueue([&, b, e]() { rangeFn(b, e); }));
		}

		const Range& local = ranges.back();
		rangeFn(local.b, local.e);

		for (auto& f : futures) {
			for (;;) {
				if (f.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
					f.get();
					break;
				}
				// Help the pool instead of spinning idle (also unblocks nested ParallelFor).
				if (!TryRunOneTask()) {
					f.wait();
					f.get();
					break;
				}
			}
		}
	}

} // namespace Engine
