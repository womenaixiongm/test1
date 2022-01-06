//
// Created by c00467120 on 2021/4/25.
//

#ifndef THREADPOOL_THREADPOOL_H
#define THREADPOOL_THREADPOOL_H
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace std;

class ThreadPool {
public:
    explicit ThreadPool(int threadNum = 8);
    ~ThreadPool();
    template<typename F, typename... Args>
    auto AddTaskToPool(F&& f, Args&&... args);
    void Stop();
private:
    vector<thread> m_threads;
    queue<function<void()>> m_tasks;
    mutex m_queueMutex;
    condition_variable m_condition;
    atomic<bool> m_stop;
};

template<typename F, class... Args>
auto ThreadPool::AddTaskToPool(F&& f, Args&&... args)
{
    auto packaged = bind(std::forward<F>(f), std::forward<Args>(args)...);
    if (m_stop) {
        return;
    }
    m_tasks.emplace(*packaged());
    m_condition.notify_one();
}

#endif //THREADPOOL_THREADPOOL_H

