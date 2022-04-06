#include "logger/logger.h"
#include <thread>
#include <vector>
#include <climits>

constexpr auto kThreadNum = 4;

void ThreadFunc(int thread_id)
{
    for(int j = 0; j < INT_MAX; ++j)
        white::LOG_DEBUG("This is the ", thread_id, "th thread test: ", j);
    white::LOG_INFO("thread: ", thread_id, " exit");
}

int main()
{
    white::LOG_INIT("./logger_test.log", white::kLogLevelDebug);
    std::vector<std::thread> threads;

    printf("Multithreading Stressing Test Start");
    fflush(stdout);
    for(int i = 0; i < kThreadNum; ++i)
        threads.push_back(std::thread{ThreadFunc, i});
    for(auto &t : threads)
        t.join();
    printf("Multithreading Stressing Test Finish");
}