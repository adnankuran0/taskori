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
#include <memory>
#include <random>

namespace taskori {

class Scheduler 
{
public:
    using Job = std::function<void()>;

    struct JobEntry 
    {
        Job job;
        int priority = 0;
        std::atomic<int> remainingDeps{ 0 };
        std::vector<std::shared_ptr<JobEntry>> dependents;
        std::promise<void> promise;
        std::atomic<bool> finished{ false };
        std::mutex depMutex; // thread safe dependents
    };

    explicit Scheduler(unsigned int workerCount = std::thread::hardware_concurrency())
        : m_Stop(false), m_ActiveJobCount(0), m_WorkerCount(workerCount)
    {
        m_Queues.resize(workerCount);
        for (unsigned int i = 0; i < workerCount; i++)
            m_QueueMutexes.emplace_back(std::make_unique<std::mutex>());
        Start(workerCount);
    }

    ~Scheduler() 
    {
        Shutdown();
    }

    std::shared_ptr<JobEntry> Submit(Job job, int priority = 0,
        std::vector<std::shared_ptr<JobEntry>> deps = {}) noexcept
    {
        auto entry = std::make_shared<JobEntry>();
        entry->job = std::move(job);
        entry->priority = priority;
        entry->remainingDeps = static_cast<int>(deps.size());

        for (auto& dep : deps) 
        {
            std::lock_guard<std::mutex> lock(dep->depMutex);
            dep->dependents.push_back(entry);
        }

        if (entry->remainingDeps == 0)
            Enqueue(entry);

        return entry;
    }

    std::future<void> GetFuture(std::shared_ptr<JobEntry> entry) 
    {
        return entry->promise.get_future();
    }

    void WaitAll() noexcept 
    {
        std::unique_lock<std::mutex> lock(m_GlobalMutex);
        m_GlobalCondition.wait(lock, [this] 
            {
            return m_ActiveJobCount.load() == 0 && AllQueuesEmpty();
            });
    }

    void Shutdown() noexcept 
    {
        m_Stop = true;
        m_GlobalCondition.notify_all();

        for (auto& worker : m_Workers)
            if (worker.joinable())
                worker.join();

        m_Workers.clear();
    }

private:
    struct CompareJob 
    {
        bool operator()(const std::shared_ptr<JobEntry>& a,
            const std::shared_ptr<JobEntry>& b) const noexcept
        {
            return a->priority < b->priority;
        }
    };

    using QueueType = std::priority_queue<std::shared_ptr<JobEntry>,
        std::vector<std::shared_ptr<JobEntry>>,
        CompareJob>;

    void Start(unsigned int threadCount) noexcept 
    {
        for (unsigned int i = 0; i < threadCount; i++)
            m_Workers.emplace_back([this, i] { Worker(i); });
    }

    void Enqueue(std::shared_ptr<JobEntry> entry) noexcept 
    {
        static thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<size_t> dist(0, m_WorkerCount - 1);
        size_t idx = dist(rng);

        {
            std::lock_guard<std::mutex> lock(*m_QueueMutexes[idx]);
            m_Queues[idx].push(entry);
        }
        m_GlobalCondition.notify_one();
    }

    bool AllQueuesEmpty() 
    {
        for (auto& q : m_Queues)
            if (!q.empty()) return false;
        return true;
    }

    void Worker(size_t id) 
    {
        while (!m_Stop) 
        {
            std::shared_ptr<JobEntry> jobEntry;

            // Try local queue
            {
                std::lock_guard<std::mutex> lock(*m_QueueMutexes[id]);
                if (!m_Queues[id].empty()) 
                {
                    jobEntry = m_Queues[id].top();
                    m_Queues[id].pop();
                    m_ActiveJobCount.fetch_add(1, std::memory_order_relaxed);
                }
            }

            // Task stealing
            if (!jobEntry) 
            {
                for (size_t i = 0; i < m_WorkerCount; i++) 
                {
                    if (i == id) continue;
                    std::lock_guard<std::mutex> lock(*m_QueueMutexes[i]);
                    if (!m_Queues[i].empty()) 
                    {
                        jobEntry = m_Queues[i].top();
                        m_Queues[i].pop();
                        m_ActiveJobCount.fetch_add(1, std::memory_order_relaxed);
                        break;
                    }
                }
            }

            if (!jobEntry) 
            {
                std::unique_lock<std::mutex> lock(m_GlobalMutex);
                m_GlobalCondition.wait_for(lock, std::chrono::milliseconds(1));
                continue;
            }

            // Execute job
            jobEntry->job();

            if (!jobEntry->finished.exchange(true)) 
            {
                jobEntry->promise.set_value();
                // Trigger dependents
                std::lock_guard<std::mutex> depLock(jobEntry->depMutex);
                for (auto& dep : jobEntry->dependents) 
                {
                    if (--dep->remainingDeps == 0)
                        Enqueue(dep);
                }
            }

            m_ActiveJobCount.fetch_sub(1, std::memory_order_relaxed);
            m_GlobalCondition.notify_all();
        }
    }

    unsigned int m_WorkerCount;
    std::vector<std::thread> m_Workers;
    std::vector<QueueType> m_Queues;
    std::vector<std::unique_ptr<std::mutex>> m_QueueMutexes;
    std::mutex m_GlobalMutex;
    std::condition_variable m_GlobalCondition;
    std::atomic<int> m_ActiveJobCount{ 0 };
    std::atomic<bool> m_Stop;
};

} // namespace taskori

#endif // TASKORI_H
