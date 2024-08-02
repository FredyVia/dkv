#include <brpc/controller.h>
#include <butil/logging.h>
#include <gflags/gflags.h>

#include <string>

#include "brpc/server.h"
#include "dkv/dkv.h"
#include "dkv/dkv_service.h"
#include "dm_constants.h"
#include "types.h"
namespace dkv {
  using namespace OHOS;

  DKVServiceImpl::DKVServiceImpl(DKV* dkv_ptr) : m_dkv_ptr(dkv_ptr){};

  // Impelements service methods
  // 把数据通过RPC传给leader
  void DKVServiceImpl::setDKV(::google::protobuf::RpcController* controller,
                              const DKVRequest* request, DKVResponse* response,
                              ::google::protobuf::Closure* done) {
    // This object helps you to call done->Run() in RAII style. If you need
    // to process the request asynchronously, pass done_guard.release().
    brpc::ClosureGuard done_guard(done);
    DistributedKv::Status status = m_dkv_ptr->Put(request->data().key(), request->data().value());

    if (status == DistributedKv::Status::SUCCESS) {
      response->set_success(true);
    } else {
      response->set_success(false);
      response->set_failinfo(std::to_string(status));
    }
  }

  void DKVServiceImpl::getDKV(::google::protobuf::RpcController* controller,
                              const DKVRequest* request, DKVResponse* response,
                              ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    DistributedKv::Value value;
    DistributedKv::Status status = m_dkv_ptr->Get(request->data().key(), value);
    if (status == DistributedKv::Status::SUCCESS) {
      response->set_success(true);
      response->set_value(value.ToString());
    } else {
      response->set_success(false);
      response->set_value("-1");
    }
  }
}  // namespace dkv
