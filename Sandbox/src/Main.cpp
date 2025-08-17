#include "taskori/taskori.h"
#include <iostream>
#include <chrono>

int main() 
{
    taskori::Scheduler sched(4);

    auto job1 = sched.Submit([] {
        std::cout << "Job 1 running\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }, 1);

    auto job2 = sched.Submit([] {
        std::cout << "Job 2 running\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }, 2, { job1 }); // depends on job 1

    auto job3 = sched.Submit([] {
        std::cout << "Job 3 running\n";
        }, 3, { job1, job2 });

    auto job4 = sched.Submit([] {
        std::cout << "Job 4 running\n";
        });

    sched.WaitAll();

    std::cout << "All jobs completed!\n";
    return 0;
}
