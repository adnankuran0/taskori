# Taskori – Lightweight C++ Job Scheduler

Taskori is a single-header, dependency-aware, thread-safe job scheduler for C++. It is designed to provide easy-to-use parallel task management for applications such as game engines, simulations, or any multithreaded workloads.

## Features

Single-header, minimal dependencies (<thread>, <mutex>, <queue>, <atomic>, <future>).

Supports job priorities.

Handles job dependencies.

Thread-safe task stealing for load balancing.

Safe integration with std::future to get job results.

Works with multiple worker threads.

## Installation

Simply include the header:
```cpp
#include "taskori.h"
```

To get the project up and running, you'll need to follow the setup instructions for your platform.

#### For Windows
1. Navigate to the `Scripts` folder in the project directory.
2. Run the Setup-Windows.bat batch file.

#### For Linux

1. Navigate to the `Scripts` folder in the project directory.

2. Make the `Setup-Linux.sh` script executable:
   
   ```bash
   chmod +x Setup-Linux.sh

3. Run the script to install necessary dependencies and set up the project:
   ```bash
   ./Setup-Linux.sh

No external dependencies are required besides the C++ Standard Library (C++17 recommended).

## Basic Usage
```cpp
#include "taskori.h"
#include <iostream>
#include <chrono>

int main() {
    // Create a scheduler with 4 worker threads
    taskori::Scheduler sched(4);

    // Submit jobs
    auto job1 = sched.Submit([] { std::cout << "Job 1 running\n"; });
    auto job2 = sched.Submit([] { std::cout << "Job 2 running\n"; }, 1, {job1}); // dependent on job1
    auto job3 = sched.Submit([] { std::cout << "Job 3 running\n"; }, 1, {job1, job2});

    // Wait for all jobs to complete
    sched.WaitAll();

    std::cout << "All jobs completed!\n";
}
```

Output (order respects dependencies):

```
Job 1 running
Job 2 running
Job 3 running
All jobs completed!
```
