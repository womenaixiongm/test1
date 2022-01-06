//
// Created by c00467120 on 2021/4/29.
//

#ifndef UNTITLED_GRPC_CLIENT_H
#define UNTITLED_GRPC_CLIENT_H

#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include "threadPool.h"

using ChannelType = std::shared_ptr<::grpc::Channel>;
using StubTypePtr = std::unique_ptr<ServiceType::Stub>;
using CqTypePtr = std::unique_ptr<::grpc::CompletionQueue>;
template<typename Request, typename Response>
using GrpcAsyncInf = GrpcClientResponseReader<Response>(StubType::*)(::grpc::ClientContext*, const Request&, ::grpc::CompletionQueue*);

struct StubAndCq {
    ServiceType::Stub *m_stub;
    ::grpc::CompletionQueue *m_cq;
};

struct GrpcClientParams {
    std::string m_serverName;
    std::string m_serverAddress;
    // 完成队列的数量，一个完成队列对应一个线程
    uint32_t m_cqNum{1};
    // 每个 cq 的 channel 数量
    uint32_t m_channelNumPerCq{1};
    // 每个channel 的 stub 数量
    uint32_t m_stubNumPerChannel{1};
    // keepalive 间隔
    uint32_t m_keepliveTimeMs{60000};
    // keepalive/TCP_USER_TIMEOUT timeout
    uint32_t m_keepliveTimeoutMs{500};
    // 尝试重连的最小间隔 ms
    uint32_t m_minBackoffMs{500};
    // 尝试重连的最大间隔 ms
    uint32_t m_maxBackoffMs{4000};
};
class GrpcClient {
public:
    bool Init(const GrpcClientParams & params);
    void ChechParmas();
    bool CreateChannels();
    bool CreateStubs();
    void CreateCqs();
    void AsyncComplete(int cqIndex);
    StubAndCq GetStubAndCq();
    void Stop();
    template<typename Request, typename Response>
    void AsyncGrpcSend(GrpcAsyncInf<Request, Response> func, StClientHandler<Request, Response> *handler);
private:
    GrpcClientParams m_params;
    uint32_t m_channelNum{1};
    uint32_t m_stubNum{1};
    std::vector<ChannelType> m_channels;
    std::vector<StubTypePtr> m_stubs;
    std::vector<CqTypePtr> m_cqs;
    ThreadPool *m_threadPool;
    std::atomic<uint32_t> m_stubIndex;
};


#endif //UNTITLED_GRPC_CLIENT_H

