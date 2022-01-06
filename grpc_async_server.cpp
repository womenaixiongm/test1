//
// Created by c00467120 on 2021/4/30.
//

#include "grpc_async_server.h"

GrpcAsyncServer::GrpcAsyncServer(const std::string& address, const std::string& serverName, const int cqNum = std::thread::hardware_concurrency())
    : m_address(address),
      m_serverName(serverName),
      m_processor(new Processor),
      m_cqNum(cqNum),
      m_isStop(true)
{
}

GrpcAsyncServer::~GrpcAsyncServer()
{
}

bool GrpcAsyncServer::StartServer()
{
    if (m_processor->Init()) {
        return false;
    }
    if (!RunServer()) {
        m_processor->Stop();
        return false;
    }
    return true;
}

bool GrpcAsyncServer::RunServer()
{
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort(m_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&m_service);
    for (uint32_t i = 0; i < m_cqNum; ++i) {
        m_cqs.emplace_back(builder.AddCompletionQueue());
    }
    m_server = builder.BuildAndStart();
    if (!m_server) {
        return false;
    }
    for (uint32_t i = 0; i < m_cqNum; ++i) {
        // 启动线程开始监听请求
        m_requestThreads.emplace_back(new std::thread(&GrpcAsyncServerInf::HandleRequest, this, i));
    }
}

// 请求的异步处理函数
void GrpcAsyncServer::HandleRequest(const uint32_t i)
{
    auto &cq = m_cqs[i];

    auto callDataTemplate = GetCallDataTemplate();
    for (const auto &callData : callDataTemplate) {
        callData->CreateCallData(&m_serverDataPacks[i]);
    }

    void *tag = nullptr;
    bool ok = true;
    while (!m_isStop.load()) {
        if (!cq->Next(&tag, &ok)) {
            // 完成队列已经停止且完成队列已经清空则 Next 会返回 false
            break;
        }
        if (tag != nullptr) {
            static_cast<RequestCallDataType*>(tag)->Proceed(ok);
        }
    }
}
void GrpcAsyncServer::Proceed(bool cqNextStatus)
{
    if (!cqNextStatus) {
        // 如果会话状态失败，则释放资源
        delete this;
        return;
    }

    if (m_status == CREATE) {
        m_status = PROCESS;
        // 等待一个新请求（m_requestFunc 为一个 grpc 为 AsyncService 生成的 RequestXXX 成员方法），接受到新的请求，该会话将进入完成队列
        (m_callDataPack->m_service->*m_requestFunc)(m_context.get(), m_request.get(), m_responder.get(), m_callDataPack->m_cq,
        m_callDataPack->m_cq, this);
    } else if (m_status == PROCESS) {
        Process();
    } else {
        if (m_status != FINISH) {
            // 不应该进入到这里
        }
        delete this;
    }
}

// 执行业务处理
void GrpcAsyncServer::Process()
{
    // 创建一个新的 CallData 对象来处理一个新请求
    CreateCallData(m_callDataPack);

    // 生成回调方法
    auto callbackFunc = [=] {
        this->m_status = FINISH;
        this->m_responder->Finish(*(this->m_reply), grpc::Status::OK, this);
    };

    // 使用线程池来执行业务代码，此处转换不应该为空
    RequestProcessorType *requestProcessor = m_callDataPack->m_processor;
    auto addTaskResult = m_callDataPack->m_workExecutor->AddTask([=] {
        // Process 方法需保证在其（包括异步）调用的所有代码退出之前执行 callback 将请求置为 FINISH 状态
        requestProcessor->Process(this->m_context.get(), this->m_request.get(), this->m_reply.get(),
                                  this->m_requestFlag, callbackFunc);
    });
    if (!addTaskResult) {
        // 如果无法添加到线程池，此时认为线程池已经关闭，直接返回 UNAVAILABLE
        m_status = FINISH;
        m_responder->Finish(*(this->m_reply), grpc::Status(grpc::StatusCode::UNAVAILABLE, "Server Unavailable"),
                            this);
    }
}
// 服务停止后的清理，获取完成队列中的所有 CallData 并完成清理
void GrpcAsyncServer::Clean()
{
    for (uint32_t i = 0; i < m_cqNum; ++i) {
        auto &cq = m_cqs[i];

        void *tag = nullptr;
        bool ok = true;
        while (cq->Next(&tag, &ok)) {
            if (tag != nullptr) {
                auto requestCallData = static_cast<RequestCallDataType*>(tag);
                delete requestCallData;
            }
        }
    }
}
bool GrpcAsyncServer::StopServer()
{
    if (m_isStop.load()) {
        return;
    }
    // 设置状态
    m_isStop.store(true);
    // 关闭服务器，设置 1s 的超时时间
    auto timePoint = std::chrono::system_clock::now() + std::chrono::seconds(1);
    m_server->Shutdown(timePoint);

    /**
     * 请在 m_processor 的 Stop 方法中保证业务逻辑正常退出
     * 不管先停止请求处理线程，还是先停止业务处理逻辑都可能导致已经接收到的请求没有及时处理而退出服务
     **/
    if (!m_processor->Stop()) {
    }

    // 关闭业务线程池
    m_workExecutor->Stop();
    // 关闭完成队列
    for (uint32_t i = 0; i < m_cqNum; ++i) {
        m_cqs[i]->Shutdown();
        // 停止请求处理线程
        if (m_requestThreads[i]->joinable()) {
            m_requestThreads[i]->join();
        }
    }
    // 清理
    Clean();
}
