#include "taskori/taskori.h"
#include <gtest/gtest.h>
#include <atomic>
#include <vector>
#include <chrono>

using namespace taskori;

TEST(SchedulerTest, SingleJobExecution) 
{
    taskori::Scheduler sched(2);
    std::atomic<bool> executed{ false };

    auto job = sched.Submit([&]() { executed = true; });
    sched.WaitAll();

    EXPECT_TRUE(executed);
}

TEST(SchedulerTest, MultipleJobsExecution) 
{
    taskori::Scheduler sched(4);
    std::atomic<int> counter{ 0 };

    for (int i = 0; i < 10; ++i)
        sched.Submit([&]() { counter.fetch_add(1, std::memory_order_relaxed); });

    sched.WaitAll();
    EXPECT_EQ(counter.load(), 10);
}

TEST(SchedulerTest, JobDependencies) 
{
    taskori::Scheduler sched(3);
    std::vector<int> order;

    auto job1 = sched.Submit([&]() { order.push_back(1); });
    auto job2 = sched.Submit([&]() { order.push_back(2); }, 1, { job1 });
    auto job3 = sched.Submit([&]() { order.push_back(3); }, 1, { job1, job2 });

    sched.WaitAll();

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

TEST(SchedulerTest, JobPriorities) 
{
    taskori::Scheduler sched(2);
    std::vector<int> order;

    auto low = sched.Submit([&]() { order.push_back(1); }, 1);
    auto high = sched.Submit([&]() { order.push_back(2); }, 10);

    sched.WaitAll();

    // High priority should execute before low if available
    EXPECT_EQ(order[0], 2);
    EXPECT_EQ(order[1], 1);
}

TEST(SchedulerTest, WaitAllBlocksUntilJobsComplete) 
{
    taskori::Scheduler sched(2);
    std::atomic<bool> jobDone{ false };

    auto job = sched.Submit([&]() 
        {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        jobDone = true;
        });

    auto start = std::chrono::high_resolution_clock::now();
    sched.WaitAll();
    auto end = std::chrono::high_resolution_clock::now();

    EXPECT_TRUE(jobDone);
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 100);
}

TEST(SchedulerTest, ShutdownSafety) 
{
    taskori::Scheduler sched(4);

    for (int i = 0; i < 20; ++i)
        sched.Submit([]() { std::this_thread::sleep_for(std::chrono::milliseconds(10)); });

    sched.Shutdown(); // Should safely terminate all threads
    SUCCEED();
}

TEST(SchedulerTest, JobExceptionDoesNotCrash) 
{
    taskori::Scheduler sched(2);

    auto job = sched.Submit([]() { throw std::runtime_error("Test"); });

    // The scheduler itself shouldn't propagate exceptions
    EXPECT_NO_THROW(sched.WaitAll());
}

TEST(SchedulerTest, TaskStealing) 
{
    taskori::Scheduler sched(4);
    std::atomic<int> counter{ 0 };

    // Submit more jobs than threads to force stealing
    for (int i = 0; i < 20; ++i)
        sched.Submit([&]() { counter.fetch_add(1, std::memory_order_relaxed); });

    sched.WaitAll();
    EXPECT_EQ(counter.load(), 20);
}

TEST(SchedulerTest, HighLoadStressTest) 
{
    taskori::Scheduler sched(8);
    const int JOB_COUNT = 1000;
    std::atomic<int> counter{ 0 };

    for (int i = 0; i < JOB_COUNT; ++i)
        sched.Submit([&]() { counter.fetch_add(1, std::memory_order_relaxed); });

    sched.WaitAll();
    EXPECT_EQ(counter.load(), JOB_COUNT);
}

TEST(SchedulerTest, ComplexDependencyGraph) 
{
    taskori::Scheduler sched(4);
    std::vector<int> executed;

    auto j1 = sched.Submit([&]() { executed.push_back(1); });
    auto j2 = sched.Submit([&]() { executed.push_back(2); }, 1, { j1 });
    auto j3 = sched.Submit([&]() { executed.push_back(3); }, 1, { j1 });
    auto j4 = sched.Submit([&]() { executed.push_back(4); }, 1, { j2, j3 });

    sched.WaitAll();

    ASSERT_EQ(executed.size(), 4u);
    EXPECT_EQ(executed[0], 1);
    EXPECT_TRUE((executed[1] == 2 && executed[2] == 3) || (executed[1] == 3 && executed[2] == 2));
    EXPECT_EQ(executed[3], 4);
}

TEST(SchedulerTest, NestedJobs) 
{
    taskori::Scheduler sched(2);
    std::atomic<int> counter{ 0 };

    auto outer = sched.Submit([&]() 
        {
        counter.fetch_add(1, std::memory_order_relaxed);
        sched.Submit([&]() { counter.fetch_add(1, std::memory_order_relaxed); });
        });

    sched.WaitAll();
    EXPECT_EQ(counter.load(), 2);
}

int main(int argc, char** argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}