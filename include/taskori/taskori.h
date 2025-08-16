#ifndef TASKORI_H
#define TASKORI_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <vector>
#include <atomic>

namespace taskori {

class JobSystem
{
public:
	using Job = std::function<void()>;

	/// <summary>
	/// Create worker threads
	/// </summary>
	/// <param name="workerCount">Number of the worker threads to be created</param>
	explicit JobSystem(unsigned int workerCount = std::thread::hardware_concurrency())
		: m_Stop(false), m_ActiveJobCount(0), m_WorkerCount(workerCount)
	{
		Start(m_WorkerCount);
	}

	~JobSystem()
	{
		Shutdown();
	}

	/// <summary>
	/// Submit a job to the system
	/// </summary>
	/// <param name="job">Function to worker execute</param>
	inline void Submit(Job job) noexcept
	{
		{
			std::unique_lock<std::mutex> lock(m_QueueMutex);
			m_JobQueue.push(std::move(job));
		}
		m_Condition.notify_one();
	}

	/// <summary>
	/// Wait until all submitted jobs are done
	/// </summary>
	inline void WaitAll() noexcept
	{
		std::unique_lock<std::mutex> lock(m_QueueMutex);
		m_Condition.wait(lock, [this] {
			return m_JobQueue.empty() && m_ActiveJobCount.load() == 0; });
	}



	/// <summary>
	/// Stop all workers and clean up
	/// </summary>
	inline void Shutdown() noexcept
	{
		{
			std::unique_lock<std::mutex> lock(m_QueueMutex);
			m_Stop = true;
		}
		m_Condition.notify_all();

		for (auto& worker : m_Workers)
		{
			if (worker.joinable())
				worker.join();
		}
		m_Workers.clear();
	}

private:
	inline void Start(unsigned int workerCount)
	{
		for (unsigned int i = 0; i < workerCount; i++)
		{
			m_Workers.emplace_back([this] { Work(); });
		}
	}

	void Work() noexcept
	{
		while (true)
		{
			Job job;
			{
				std::unique_lock<std::mutex> lock(m_QueueMutex);
				m_Condition.wait(lock, [this] { return !m_JobQueue.empty() || m_Stop; });
				if (m_Stop && m_JobQueue.empty())
					return;

				job = std::move(m_JobQueue.front());
				m_JobQueue.pop();
			}
			m_ActiveJobCount++;
			job();
			m_ActiveJobCount--;
			m_Condition.notify_all();
		}
	}

	unsigned int m_WorkerCount;
	std::queue<Job> m_JobQueue;
	std::vector<std::thread> m_Workers;
	std::mutex m_QueueMutex;
	std::condition_variable m_Condition;
	std::atomic<bool> m_Stop;
	std::atomic<int> m_ActiveJobCount;
};

} // namespace taskori 

#endif // !TASKORI_H




