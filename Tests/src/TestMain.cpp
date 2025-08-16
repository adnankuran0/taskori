#include "taskori/taskori.h"
#include <gtest/gtest.h>
#include <atomic>
#include <vector>
#include <chrono>

using namespace taskori;

// Test: Submitted jobs are executed
TEST(SchedulerTest, BasicExecution) 
{
    Scheduler js(4);
    std::atomic<int> counter = 0;

    js.Submit([&] { counter++; });
    js.Submit([&] { counter++; });

    js.WaitAll();
    EXPECT_EQ(counter.load(), 2);
}

// Test: WaitAll blocks until jobs are finished
TEST(SchedulerTest, WaitAllBlocksUntilJobsDone) 
{
    Scheduler js(2);
    std::atomic<bool> jobFinished = false;

    js.Submit([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        jobFinished = true;
        });

    js.WaitAll();
    EXPECT_TRUE(jobFinished.load());
}

// Test: Execute multiple jobs in parallel
TEST(SchedulerTest, MultipleJobsExecution) 
{
    Scheduler js(4);
    std::atomic<int> counter = 0;
    int totalJobs = 100;

    for (int i = 0; i < totalJobs; i++) 
    {
        js.Submit([&counter] { counter++; });
    }

    js.WaitAll();
    EXPECT_EQ(counter.load(), totalJobs);
}

// Test: No jobs should execute after shutdown
TEST(SchedulerTest, NoJobsAfterShutdown) 
{
    Scheduler js(2);
    js.Shutdown();

    bool jobExecuted = false;
    js.Submit([&] { jobExecuted = true; });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(jobExecuted);
}

// Test: Thread-safe counter under heavy load
TEST(SchedulerTest, ThreadSafeCounter) 
{
    Scheduler js(4);
    std::atomic<int> counter = 0;
    int totalJobs = 1000;

    for (int i = 0; i < totalJobs; i++) 
    {
        js.Submit([&counter] { counter++; });
    }

    js.WaitAll();
    EXPECT_EQ(counter.load(), totalJobs);
}

// Test: WaitAll behaves correctly on empty queue
TEST(SchedulerTest, WaitAllWithNoJobs) 
{
    Scheduler js(2);
    EXPECT_NO_THROW(js.WaitAll());
}

int main(int argc, char** argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}