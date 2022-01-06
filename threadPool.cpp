//
// Created by c00467120 on 2021/4/25.
//
#include "threadPool.h"
#include <vector>
#include <iostream>

using namespace std;

ThreadPool::ThreadPool(int threadNum)
{
    if (threadNum < 0) {
        threadNum = 8;
    }
    for (int index = 0; index < threadNum; ++index) {
        m_threads.emplace_back(
                [this]{
                    while (true) {
                        unique_lock<mutex> lock(this->m_queueMutex);
                        this->m_condition.wait(lock, [this]{return this->m_stop || !this->m_tasks.empty();});
                        if (this->m_stop && m_tasks.empty()) {
                            return;
                        }
                        auto& task = m_tasks.front();
                        m_tasks.pop();
                        task;
                    }
                }
        );
    }
}
void ThreadPool::Stop()
{
    m_stop.store(true);
    m_condition.notify_all();
    for (std::thread &worker : m_threads) {
        worker.join();
    }
    m_threads.clear();
}

ThreadPool::~ThreadPool()
{
    Stop();
}
