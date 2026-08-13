#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

namespace Engine {

	/// Fixed worker pool used by animation skinning, pose eval, transforms, etc.
	/// OpenGL work must stay on the main thread — only CPU-side jobs go here.
	class ThreadPool {
	  public:
		/// workers == 0 → hardware_concurrency() - 1 (at least 1 if multi-core).
		explicit ThreadPool(unsigned workers = 0);
		~ThreadPool();

		ThreadPool(const ThreadPool&)            = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;

		void Start(unsigned workers = 0);
		void Shutdown();

		[[nodiscard]] bool     IsRunning() const { return !m_stop && !m_workers.empty(); }
		[[nodiscard]] unsigned WorkerCount() const { return static_cast<unsigned>(m_workers.size()); }

		/// Queue a job; returns a future for the result.
		template <class F, class... Args>
		auto Enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
		{
			using R = std::invoke_result_t<F, Args...>;
			auto task = std::make_shared<std::packaged_task<R()>>(
			    std::bind(std::forward<F>(f), std::forward<Args>(args)...));
			std::future<R> fut = task->get_future();
			{
				std::lock_guard lock(m_mutex);
				if (m_stop) {
					// Run inline if pool is down.
					(*task)();
					return fut;
				}
				m_tasks.emplace([task]() { (*task)(); });
			}
			m_cv.notify_one();
			return fut;
		}

		/// Parallel for over [0, count). Splits into range chunks; blocks until done.
		/// The calling thread also participates (no idle main thread wait).
		/// minPerTask: don't create a task smaller than this (reduces overhead).
		void ParallelFor(int count, int minPerTask, const std::function<void(int begin, int end)>& rangeFn);

		/// Convenience: one index at a time.
		void ParallelForIndex(int count, int minPerTask, const std::function<void(int)>& fn)
		{
			ParallelFor(count, minPerTask, [&](int b, int e) {
				for (int i = b; i < e; ++i) fn(i);
			});
		}

	  private:
		void WorkerLoop();
		/// Pop and run one queued task if any (used while waiting on ParallelFor).
		bool TryRunOneTask();

		std::vector<std::thread>          m_workers;
		std::queue<std::function<void()>> m_tasks;
		mutable std::mutex                m_mutex;
		std::condition_variable           m_cv;
		bool                              m_stop = true;
	};

} // namespace Engine
