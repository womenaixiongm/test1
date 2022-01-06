//
// Created by c00467120 on 2021/5/5.
//

#ifndef UNTITLED_TIMER_TASK_INTERFACE_H
#define UNTITLED_TIMER_TASK_INTERFACE_H

#include <mutex>
#include <condition_variable>
using namespace std;

class TimerTaskInterface {
public:
    TimerTaskInterface() = delete;
    TimerTaskInterface(int delay);
    ~TimerTaskInterface();
    virtual bool IntervalTask() = 0;
    void Run();
    void Stop();
private:
    int m_delay;
    int m_intervalTime;
    mutex m_mutex;
    condition_variable m_condition;
    bool m_isStop;
};


#endif //UNTITLED_TIMER_TASK_INTERFACE_H

