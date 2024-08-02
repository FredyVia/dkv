#ifndef DKV_SERVICE_H
#define DKV_SERVICE_H

#include "dkv/dkv.h"
#include "dkv/dkv_service.pb.h"

namespace dkv {

  class DKVServiceImpl : public DKVService {
    DKV* m_dkv_ptr;

  public:
    DKVServiceImpl(DKV* dkv_ptr);

    // Impelements service methods
    // 把数据通过RPC传给leader
    void setDKV(::google::protobuf::RpcController* controller, const DKVRequest* request,
                DKVResponse* response, ::google::protobuf::Closure* done);

    void getDKV(::google::protobuf::RpcController* controller, const DKVRequest* request,
                DKVResponse* response, ::google::protobuf::Closure* done);
  };
}  // namespace dkv
#endif