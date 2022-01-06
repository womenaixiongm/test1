//
// Created by c00467120 on 2021/4/29.
//

#ifndef UNTITLED_GRPC_HANDLER_H
#define UNTITLED_GRPC_HANDLER_H

template<typename Request, typename Response>
struct GrpcClientHandler{
    int sendTime;
    grpc::Status status;
    grpc::ClientContext context;
    function<void()> callback;
    Request* request;
    Response* reponse;
    std::unique<grpc::ClientAsyncResponseReader<Response>> reader;
};
#endif //UNTITLED_GRPC_HANDLER_H

