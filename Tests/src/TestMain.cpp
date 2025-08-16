#include "taskori/taskori.h"
#include <gtest/gtest.h>
#include <atomic>
#include <vector>
#include <chrono>

using namespace taskori;

// Test: Submitted jobs are executed
TEST(JobSystemTest, BasicExecution) 
{
    JobSystem js(4);
    std::atomic<int> counter = 0;

    js.Submit([&] { counter++; });
    js.Submit([&] { counter++; });

    js.WaitAll();
    EXPECT_EQ(counter.load(), 2);
}

// Test: WaitAll blocks until jobs are finished
TEST(JobSystemTest, WaitAllBlocksUntilJobsDone) 
{
    JobSystem js(2);
    std::atomic<bool> jobFinished = false;

    js.Submit([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        jobFinished = true;
        });

    js.WaitAll();
    EXPECT_TRUE(jobFinished.load());
}

// Test: Execute multiple jobs in parallel
TEST(JobSystemTest, MultipleJobsExecution) 
{
    JobSystem js(4);
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
TEST(JobSystemTest, NoJobsAfterShutdown) 
{
    JobSystem js(2);
    js.Shutdown();

    bool jobExecuted = false;
    js.Submit([&] { jobExecuted = true; });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(jobExecuted);
}

// Test: Thread-safe counter under heavy load
TEST(JobSystemTest, ThreadSafeCounter) 
{
    JobSystem js(4);
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
TEST(JobSystemTest, WaitAllWithNoJobs) 
{
    JobSystem js(2);
    EXPECT_NO_THROW(js.WaitAll());
}

int main(int argc, char** argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}