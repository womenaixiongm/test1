//
// Created by c00467120 on 2021/4/30.
//

#ifndef UNTITLED_GRPC_ASYNC_SERVER_H
#define UNTITLED_GRPC_ASYNC_SERVER_H

#include <memory>
#include <iostream>
#include "threadPool.h"

template<typename Processor>
class GrpcAsyncServer {
public:

private:
    std::unique_ptr<Processor> m_processor;
    ThreadPool *m_threadPool;
    // 请求处理线程，线程数量等于完成队列数量
    std::vector<std::unique_ptr<std::thread>> m_requestThreads;
    // 完成队列，当一个请求到达/一个请求完成业务处理，可以通过 m_cq->Next 调用获取到请求的信息进行（异步）处理
    std::vector<std::unique_ptr<::grpc::ServerCompletionQueue>> m_cqs;
};


#endif //UNTITLED_GRPC_ASYNC_SERVER_H

