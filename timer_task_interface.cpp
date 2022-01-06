//
// Created by c00467120 on 2021/5/5.
//

#include "timer_task_interface.h"
#include <chrono>

TimerTaskInterface::TimerTaskInterface(int delay)
{
    m_delay = delay;
}
TimerTaskInterface::~TimerTaskInterface()
{
    Stop();
}

void TimerTaskInterface::Run()
{
    if (m_delay > 0) {
        unique_lock<mutex> lock(m_mutex);
        m_condition.wait_for(lock, chrono::milliseconds(m_delay), [=]{return this->m_isStop;});
        if (m_isStop) {
            return;
        }
    }
    while (true) {
        if (IntervalTask()) {
            break;
        }
        unique_lock<mutex> lock(m_mutex);
        m_condition.wait_for(lock, chrono::milliseconds(m_intervalTime), [=]{return this->m_isStop;});
        if (m_isStop) {
            return;
        }
    }
}
void TimerTaskInterface::Stop()
{

}


