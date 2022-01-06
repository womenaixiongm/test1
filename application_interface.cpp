//
// Created by c00467120 on 2021/5/4.
//

#include "application_interface.h"

// 信号处理函数
void ApplicationInterface::GraceStop(int signal)
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_isStop = true;
    }
    m_condition.notify_one();
}

void ApplicationInterface::SetGraceStop()
{
    g_graceCallBackFunc = [=](int signal) { this->GraceStop(signal); };

    // 屏蔽 SIGCHLD、SIGPIPE
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    // 处理 SIGINT、SIGTERM
    struct sigaction action;
    action.sa_handler = [](int signal) { g_graceCallBackFunc(signal); };
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGINT, &action, nullptr) != 0 || sigaction(SIGTERM, &action, nullptr) != 0) {
        // 信号处理函数设置失败不认为程序启动失败
    }
}



bool ApplicationInterface::Start(int argc, char **argv)
{
    g_appName = std::move(appName);

    // 先初始化自有日志系统
    dsp::common::LogManager::Instance()->InitLogger(g_appName);

    // 读取命令行参数，先设置退出函数
    g_defaultGflagsExitFunc = google::gflags_exitfunc;
    google::gflags_exitfunc = &GflagsExitFunc;
    google::ParseCommandLineFlags(&argc, &argv, false);
    // 初始化成功之后若配置文件修改导致错误不再调用默认的方法退出程序
    g_defaultGflagsExitFunc = nullptr;

    // 初始化日志和监控
    google::InitGoogleLogging(argv[0]);
    ::common::File::AddDirRecursively("../logs");
    dsp::common::LogManager::Instance()->InitLogHandler();
    dsp::common::InitAlarmHcm(nullptr);
    dsp::common::SysRunLogTask::Instance()->Init(g_appName);

    // 启动一个线程动态监控配置文件，可以使用信号的方式避免 detach
    m_monitorConfigFileTask.reset(new MonitorConfigFileTask(FLAGS_dynamic_monitor_file, argc, argv));
    if (!m_monitorConfigFileTask->Init()) {
        // 无法动态监控配置文件只打印日志，不认为启动失败
        LOG(WARNING) << "Failed to Monitor Config File";
    }

    // 初始化应用，InitApplication 返回后，应用程序将处于工作状态
    if (!InitApplication()) {
        Stop();
        return false;
    }

    // 写入 pid 到文件
    std::ofstream("./pid") << getpid() << std::endl;

    // mutex 将避免指令重排，这里不需要加锁保护
    m_isStop = false;

    // 设置信号捕获函数
    SetGraceStop();

    // 等待退出信号，避免轮询
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [=] { return this->m_isStop; });
    }

    // 停止应用程序
    Stop();
}
