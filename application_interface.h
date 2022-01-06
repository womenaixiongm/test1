//
// Created by c00467120 on 2021/5/4.
//

#ifndef UNTITLED_APPLICATION_INTERFACE_H
#define UNTITLED_APPLICATION_INTERFACE_H
#include <iostream>
#include <condition_variable>

using namespace std;

class ApplicationInterface {
public:
    bool Start(int argc, char **argv);
private:
    bool m_isStop = true;

    std::mutex m_mutex;

    std::condition_variable m_condition;

    // 定时任务，用来动态读取配置文件
    std::unique_ptr<MonitorConfigFileTask> m_monitorConfigFileTask;
};


#endif //UNTITLED_APPLICATION_INTERFACE_H

