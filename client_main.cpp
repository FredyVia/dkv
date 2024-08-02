#include <brpc/channel.h>
#include <brpc/controller.h>
#include <butil/logging.h>
#include <gflags/gflags.h>

#include <cstddef>

#include "dkv/dkv_service.pb.h"

DEFINE_string(key, "", "Insert data to this device");
DEFINE_string(value, "", "Insert  data");
DEFINE_string(server, "127.0.0.1:1089", "IP Address of kv server");
DEFINE_string(command, "get",
              "Defaut get the data of this server.");  // get set

using namespace std;
using namespace brpc;
using namespace dkv;
using namespace gflags;

int main(int argc, char *argv[]) {
  ParseCommandLineFlags(&argc, &argv, true);
  brpc::Channel channel;
  brpc::Controller cntl;

  brpc::ChannelOptions options;
  if (channel.Init(FLAGS_server.c_str(), nullptr) != 0) {
    LOG(ERROR) << "Fail to initialize channel";
    return -1;
  }
  DKVService_Stub stub(&channel);

  DKVRequest request;
  DKVResponse response;
  DKVData data;

  data.set_key(FLAGS_key);
  data.set_value(FLAGS_value);
  request.mutable_data()->set_key(FLAGS_key);
  request.mutable_data()->set_value(FLAGS_value);
  if (FLAGS_command == "get") {
    stub.getDKV(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
      throw runtime_error("Can't get the dkv value");
    }
    if (!response.success()) {
      cout << "Fail to response get\n";
      throw runtime_error("dkv get logic error");
    }
    cout << "get the value: " << response.value() << endl;

  } else if (FLAGS_command == "set") {
    stub.setDKV(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
      throw runtime_error("Can't get the dkv value");
    }
    if (!response.success()) {
      cout << "Fail to response set\n";
      throw runtime_error("dkv set logic error");
    }
    cout << "setDKV success\n";
  }
  return 0;
}