#include "dkv/dkv.h"

#include "dm_constants.h"
#include "hilog/log.h"

namespace dkv {
  using namespace std;
  using namespace OHOS::DistributedHardware;
  using namespace OHOS::DistributedKv;
  using namespace OHOS::HiviewDFX;
  namespace fs = std::filesystem;
  const string DKV::m_app_id = "dkv";
  const string DKV::m_store_id = "data";
  const string DKV::m_base_dir = "/data/service/el1/public/database/dkv";

  DKV::DKV(std::set<std::string> ips, int port, std::string my_ip, int weight)
      : m_device_manager(DeviceManager::GetInstance()),
        m_ips(ips),
        m_my_ip(my_ip),
        m_port(port),
        m_weight(weight) {
    auto result
        = m_device_manager.InitDeviceManager(m_app_id, std::make_shared<DefaultDmInitCallback>());
    if (result != DM_OK) {
      throw runtime_error("InitDeviceManager failed, error code: " + to_string(result));
    }

    DmDeviceInfo deviceInfo;
    result = m_device_manager.GetLocalDeviceInfo(m_app_id, deviceInfo);
    if (result != DM_OK) {
      throw runtime_error("GetLocalDeviceInfo failed, error code: " + to_string(result));
    }
    m_device_id = deviceInfo.deviceId;
    LOG(INFO) << m_device_id << endl;
    m_server_ptr
        = make_shared<weight_raft::WeightServer>(m_base_dir, ips, m_port, m_device_id, m_my_ip);
    m_server_ptr->start();
    // m_server_ptr->setWeight(m_weight);

    if (!fs::exists(m_base_dir)) {
      fs::create_directories(m_base_dir);
      fs::permissions(m_base_dir,
                      fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all);
    }
    string dir = m_base_dir + "/key";
    fs::create_directories(dir);
    fs::permissions(dir, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all);
    dir = m_base_dir + "/kvdb/backup";
    fs::create_directories(dir);
    fs::permissions(dir, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all);
    Options options = {.createIfMissing = true,
                       .encrypt = false,
                       .autoSync = false,
                       .kvStoreType = KvStoreType::DEVICE_COLLABORATION,
                       .baseDir = m_base_dir};
    options.area = EL1;
    options.securityLevel = NO_LABEL;

    OHOS::DistributedKv::Status status = m_kv_data_manager.GetSingleKvStore(
        options, AppId({m_app_id}), StoreId({m_store_id}), m_kv_store_ptr);
    if (status != OHOS::DistributedKv::Status::SUCCESS) {
      throw runtime_error("init failed");
    }
  }

  std::string DKV::get_master() { return m_server_ptr->get_leader_device_id(); }

  OHOS::DistributedKv::Status DKV::Get(const Key &key, Value &value) {
    DataQuery query;
    std::vector<std::string> keys = {key.ToString()};
    query.InKeys(keys);
    m_kv_store_ptr->Sync({get_master()}, SyncMode::PULL, query, nullptr);
    return m_kv_store_ptr->Get(key, value);
  }

  // StoreId DKV::GetStoreId() const { return m_kv_store_ptr->GetStoreId(); }

  OHOS::DistributedKv::Status DKV::Put(const Key &key, const Value &value) {
    m_kv_store_ptr->Put(key, value);
    DataQuery query;
    std::vector<std::string> keys = {key.ToString()};
    query.InKeys(keys);
    return m_kv_store_ptr->Sync({get_master()}, SyncMode::PUSH, query, nullptr);
  }
  // OHOS::DistributedKv::Status DKV::PutBatch(const std::vector<Entry> &entries) {
  //   m_kv_store_ptr->PutBatch(entries);
  //   std::vector<std::string> keys;
  //   keys.reserve(entries.size());
  //   for (auto &&entry : entries) {
  //     keys.push_back(entry.key);
  //   }
  //   DataQuery query;
  //   query.InKeys(keys);
  //   return m_kv_store_ptr->Sync({get_master()}, SyncMode::PUSH, query, nullptr);
  // }

  OHOS::DistributedKv::Status DKV::Delete(const Key &key) {
    m_kv_store_ptr->Delete(key);
    DataQuery query;
    std::vector<std::string> keys = {key.ToString()};
    query.InKeys(keys);
    return m_kv_store_ptr->Sync({get_master()}, SyncMode::PUSH, query, nullptr);
  }

  // OHOS::DistributedKv::Status DKV::DeleteBatch(const std::vector<Key> &keys) {
  //   m_kv_store_ptr->DeleteBatch(keys);
  //   DataQuery query;
  //   query.InKeys(keys);
  //   return m_kv_store_ptr->Sync({get_master()}, SyncMode::PUSH, query, nullptr);
  // }

  // OHOS::DistributedKv::Status DKV::StartTransaction() {
  //   return OHOS::DistributedKv::Status::NOT_SUPPORT;
  // }

  // OHOS::DistributedKv::Status DKV::Commit() {
  //   return OHOS::DistributedKv::Status::NOT_SUPPORT;
  // }

  // OHOS::DistributedKv::Status DKV::Rollback() {
  //   return OHOS::DistributedKv::Status::NOT_SUPPORT;
  // }

  // OHOS::DistributedKv::Status DKV::SubscribeKvStore(
  //     SubscribeType type,
  //     std::shared_ptr<DistributedKv::KvStoreObserver> observer) {
  //   return m_kv_store_ptr->SubscribeKvStore(type, observer);
  // }

  // OHOS::DistributedKv::Status DKV::UnSubscribeKvStore(
  //     SubscribeType type,
  //     std::shared_ptr<DistributedKv::KvStoreObserver> observer) {
  //   return m_kv_store_ptr->UnSubscribeKvStore(type, observer);
  // }

  // OHOS::DistributedKv::Status DKV::Backup(const std::string &file,
  //                                   const std::string &baseDir) {
  //   return m_kv_store_ptr->Backup(file, baseDir);
  // }

  // OHOS::DistributedKv::Status DKV::Restore(const std::string &file,
  //                                    const std::string &baseDir) {
  //   return m_kv_store_ptr->Restore(file, baseDir);
  // }

  // OHOS::DistributedKv::Status DKV::DeleteBackup(
  //     const std::vector<std::string> &files, const std::string &baseDir,
  //     map<std::string, DistributedKv::OHOS::DistributedKv::Status> &status) {
  //   return m_kv_store_ptr->DeleteBackup(files, baseDir, status);
  // }

}  // namespace dkv