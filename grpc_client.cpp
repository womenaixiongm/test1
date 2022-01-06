//
// Created by c00467120 on 2021/4/29.
//

#include "grpc_client.h"

#include "singleTon.h"

bool GrpcClient::Init(const GrpcClientParams & params)
{
    ChechParmas();
    if (CreateChannels() || CreateStubs()) {
        return false;
    }
    CreateCqs();

    m_threadPool = SingleTon<ThreadPool>::GetSingleTon();
    return true;
}

void GrpcClient::ChechParmas()
{
    m_params.m_cqNum = m_params.m_cqNum == 0 ? 1 : m_params.m_cqNum;
    m_params.m_channelNumPerCq = m_params.m_channelNumPerCq == 0 ? 1 : m_params.m_channelNumPerCq;
    m_params.m_stubNumPerChannel = m_params.m_stubNumPerChannel == 0 ? 1 : m_params.m_stubNumPerChannel;
    m_channelNum = m_params.m_channelNumPerCq * m_params.m_cqNum;
    m_stubNum = m_params.m_stubNumPerChannel * m_channelNum;
}

bool GrpcClient::CreateChannels()
{
    for (int i = 0; i < m_channelNum; ++i) {
        grpc::ChannelArguments channelArgs;
        channelArgs.SetInt(GRPC_ARG_USE_LOCAL_SUBCHANNEL_POOL, 1);
        channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, m_params.m_keepliveTimeMs);
        channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, m_params.m_keepliveTimeoutMs);
        channelArgs.SetInt(GRPC_ARG_MIN_RECONNECT_BACKOFF_MS, m_params.m_minBackoffMs);
        channelArgs.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, m_params.m_maxBackoffMs);
        auto channel = grpc::CreateCustomChannel(m_params.m_serverAddress,
                                                 grpc::InsecureChannelCredentials(), channelArgs);
        if (!channel) {
            return false;
        }
        m_channels.emplace_back(std::move(channel));
    }
    return true;
}
bool GrpcClient::CreateStubs()
{
    for (int i = 0; i < m_stubNum; ++i) {
        auto &channel = m_channels[i % m_channelNum];
        auto stub = ServiceType::NewStub(channel);
        if (!stub) {
            return false;
        }
        m_stubs.emplace_back(std::move(stub));
    }
    return true;
}

void GrpcClient::CreateCqs()
{
    for (int i = 0; i < m_params.m_cqNum; ++i) {
        m_cqs.emplace_back(new ::grpc::CompletionQueue);
    }
    for (uint32_t i = 0; i < m_params.m_cqNum; ++i) {
        m_cqThreads.emplace_back(std::thread(&GrpcClient::AsyncComplete, this, i));
    }
}

void GrpcClient::AsyncComplete(int cqIndex)
{
    auto &cq = m_cqs[cqIndex];
    void *tag = NULL;
    bool ok = false;
    while (cq->Next(&tag, &ok)) {
        if (tag == NULL) {
            continue;
        }
        auto context = static_cast<BaseGrpcClientContext*>(tag);
        context->m_endTime = GetTimeNow();
        m_threadPool->AddTaskToPool(context->GetCallback());
        delete context;
        context = nullptr;
    }
}
StubAndCq GrpcClient::GetStubAndCq()
{
    StubAndCq stubAndCq;
    auto stubIndex = m_stubNum == 1 ? 0 : (m_stubIndex++) % m_stubNum;
    stubAndCq.m_stub = m_stubs[stubIndex].get();
    auto cqIndex = stubIndex % m_params.m_cqNum;
    stubAndCq.m_cq = m_cqs[cqIndex].get();
    return stubAndCq;
}

void GrpcClient::Stop()
{
    for (auto &cq : m_cqs) {
        cq->Shutdown();
    }
    m_cqs.clear();

    for (auto &cqThread : m_cqThreads) {
        if (cqThread.joinable()) {
            cqThread.join();
        }
    }
    m_cqThreads.clear();
}

template<typename Request, typename Response>
void GrpcClient::AsyncGrpcSend(GrpcAsyncInf<Request, Response> func, StClientHandler<Request, Response> *handler)
{
    auto stubAndCq = GetStubAndCq();
    handler->sendTime; // = GetMicroSeconds();
    handler->reader = (stubAndCq.m_stub->*func)(&handler->context, *handler->request, stubAndCq.m_cq);
    handler->reader->Finish(handler->response, &handler->status, handler);
}
