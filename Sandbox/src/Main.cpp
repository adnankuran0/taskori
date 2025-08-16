#include "taskori/taskori.h"
#include <iostream>
#include <chrono>

int main()
{
    // Create job sytem with 4 worker threas
    taskori::JobSystem js(4);

    // Submit jobs
    js.Submit([] {
        std::cout << "Job 1 executed!" << std::endl;
        });

    js.Submit([] {
        std::cout << "Job 2 executed!" << std::endl;
        });

    // Wait all the jobs are done
    js.WaitAll();

    std::cout << "All the jobs are completed." << std::endl;

    // Wait and exit
    js.Shutdown();

}