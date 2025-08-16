#ifndef TASKORI_H
#define TASKORI_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <vector>
#include <atomic>
#include <future>

//TODO:
//Queue → priority queue / multiple queues
//Job → dependency graph + optional priority
//Workers → affinity / frame - sliced update
//Submit → future or callback support
//Monitor → job profiling and debugging

namespace taskori {

class Scheduler
{
public:
	using Job = std::function<void()>;

	struct JobEntry
	{
		Job job;
		int priority = 0; // 0=low 1=medium 2=high
		std::promise<void> promise;
	};

	/// <summary>
	/// Create worker threads
	/// </summary>
	/// <param name="workerCount">Number of the worker threads to be created</param>
	explicit Scheduler(unsigned int workerCount = std::thread::hardware_concurrency())
		: m_Stop(false), m_ActiveJobCount(0), m_WorkerCount(workerCount)
	{
		Start(m_WorkerCount);
	}

	~Scheduler()
	{
		Shutdown();
	}

	/// <summary>
	/// Submit a job to the system
	/// </summary>
	/// <param name="job">Function to worker execute</param>
	inline std::future<void> Submit(Job job, int priority = 0) noexcept
	{
		std::promise<void> p;
		auto future = p.get_future();
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_JobQueue.push(JobEntry{ std::move(job), priority, std::move(p) });
		}
		m_Condition.notify_one();
		return future;
	}

	/// <summary>
	/// Wait until all submitted jobs are done
	/// </summary>
	inline void WaitAll() noexcept
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_Condition.wait(lock, [this] {
			return m_JobQueue.empty() && m_ActiveJobCount.load() == 0; });
	}



	/// <summary>
	/// Stop all workers and clean up
	/// </summary>
	inline void Shutdown() noexcept
	{
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
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
	struct CompareJob
	{
		bool operator()(const JobEntry& a, const JobEntry& b)
		{
			return a.priority < b.priority;
		}
	};

	inline void Start(unsigned int workerCount)
	{
		for (unsigned int i = 0; i < workerCount; i++)
		{
			m_Workers.emplace_back([this] { Work(); });
		}
	}

	void Work() 
	{
		while (true) 
		{
			JobEntry jobEntry;
			{
				std::unique_lock<std::mutex> lock(m_Mutex);
				m_Condition.wait(lock, [this] 
					{
					return !m_JobQueue.empty() || m_Stop;
					});

				if (m_Stop && m_JobQueue.empty())
					return;

				jobEntry = std::move(const_cast<JobEntry&>(m_JobQueue.top()));
				m_JobQueue.pop();
				m_ActiveJobCount++;
			}

			jobEntry.job();
			jobEntry.promise.set_value(); // signal future completion

			m_ActiveJobCount--;
			m_Condition.notify_all();
		}
	}

	unsigned int m_WorkerCount;
	std::priority_queue<JobEntry, std::vector<JobEntry>, CompareJob> m_JobQueue;
	std::vector<std::thread> m_Workers;
	std::mutex m_Mutex;
	std::condition_variable m_Condition;
	std::atomic<bool> m_Stop;
	std::atomic<int> m_ActiveJobCount;
};

} // namespace taskori 

#endif // !TASKORI_H




