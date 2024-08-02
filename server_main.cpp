#include <brpc/server.h>
#include <gflags/gflags.h>

#include <iostream>
#include <set>
#include <string>

#include "accesstoken_kit.h"
#include "distributed_kv_data_manager.h"
#include "dkv/dkv.h"
#include "dkv/dkv_service.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "weight_raft/utils.h"

using namespace std;
using namespace weight_raft;
using namespace dkv;
using namespace OHOS::Security::AccessToken;

void setPermissions() {
  uint64_t tokenId;
  const char* perms[3];
  perms[0] = "ohos.permission.ACCESS_SERVICE_DM";
  perms[1] = "ohos.permission.DISTRIBUTED_DATASYNC";
  perms[2] = "ohos.permission.DISTRIBUTED_SOFTBUS_CENTER";
  NativeTokenInfoParams infoInstance = {
      .dcapsNum = 0,
      .permsNum = 3,
      .aclsNum = 0,
      .dcaps = NULL,
      .perms = perms,
      .acls = NULL,
      .processName = "test_kv",
      .aplStr = "system_basic",
  };
  tokenId = GetAccessTokenId(&infoInstance);
  SetSelfTokenID(tokenId);
  AccessTokenKit::ReloadNativeTokenInfo();
}
int main(int argc, char* argv[]) {
  if (argc != 4) {
    cout << "server <all_ipaddresses> <my_ipaddress> <myweight>" << endl;
    return -1;
  }
  setPermissions();

  int weight_raft_port = 1088;
  int kv_port = 1089;
  set<string> ips = parse_nodes(string(argv[1]));
  string my_ip = argv[2];
  int weight = stoi(argv[3]);
  DKV* dkv_ptr = new DKV(ips, weight_raft_port, my_ip, weight);
  brpc::Server server;

  dkv::DKVServiceImpl service(dkv_ptr);
  if (server.AddService(&service, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    LOG(ERROR) << "Fail to add service";
    return -1;
  }
  butil::EndPoint ep(butil::IP_ANY, kv_port);
  // Start the server
  brpc::ServerOptions options;
  if (server.Start(ep, &options) != 0) {
    LOG(ERROR) << "Fail to start DKVServer";
    return -1;
  }
  int count = 0;
  // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
  while (!brpc::IsAskedToQuit()) {
    sleep(2);
    cout << "." << std::flush;
    count++;
    if (count == 5) {
      cout << endl;
      cout << "running" << std::flush;
    }
  }
  return 0;
}