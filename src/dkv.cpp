#include "dkv/dkv.h"

#include <string>

#include "dm_constants.h"

namespace dkv {
  using namespace std;
  using namespace OHOS::DistributedHardware;
  using namespace OHOS::DistributedKv;
  namespace fs = std::filesystem;

  void DefaultKvStoreObserver::OnChange(
      const OHOS::DistributedKv::ChangeNotification &changeNotification) {
    LOG(INFO) << changeNotification.GetDeviceId().c_str();
    for (const auto &e : changeNotification.GetInsertEntries()) {
      auto status = m_dkv_ptr->m_kv_main_store_ptr->Put(
          e.key, string("after_compute: ") + e.value.ToString());
      if (status != OHOS::DistributedKv::Status::SUCCESS) {
        LOG(ERROR) << "GetInsertEntries: Put error " << status;
      } else {
        LOG(INFO) << "GetInsertEntries: Put OK";
      }
    }
    for (const auto &e : changeNotification.GetUpdateEntries()) {
      auto status = m_dkv_ptr->m_kv_main_store_ptr->Put(
          e.key, string("after_compute: ") + e.value.ToString());
      if (status != OHOS::DistributedKv::Status::SUCCESS) {
        LOG(ERROR) << "GetUpdateEntries: Put error " << status;
      } else {
        LOG(INFO) << "GetUpdateEntries: Put OK";
      }
    }
    for (const auto &e : changeNotification.GetDeleteEntries()) {
      auto status = m_dkv_ptr->m_kv_main_store_ptr->Delete(e.key);
      if (status != OHOS::DistributedKv::Status::SUCCESS) {
        LOG(ERROR) << "Delete error " << status;
      } else {
        LOG(INFO) << "Delete OK";
      }
    }

    for (const auto &weight_info : m_dkv_ptr->m_server_ptr->get_weights()) {
      auto status
          = m_dkv_ptr->m_kv_main_store_ptr->Sync({weight_info.network_id()}, SyncMode::PUSH, 0);
      if (status != OHOS::DistributedKv::Status::SUCCESS) {
        LOG(ERROR) << "main sync error " << status;
      } else {
        LOG(INFO) << "main sync OK";
      }
    }
  }

  const string DKV::m_app_id = "dkv";
  const string DKV::m_store_id = "data2";
  const string DKV::m_main_store_id = "main_data2";
  const string DKV::m_base_dir = "/data/service/el1/public/database/dkv";
  // const string DKV::m_main_base_dir = "/data/service/el1/public/database/main_dkv";
  void DKV::create_dir(std::string directory) {
    if (!fs::exists(directory)) {
      fs::create_directories(directory);
      fs::permissions(directory,
                      fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all);
    }
    string dir = directory + "/key";
    fs::create_directories(dir);
    fs::permissions(dir, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all);
    dir = directory + "/kvdb/backup";
    fs::create_directories(dir);
    fs::permissions(dir, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all);
  }

  DKV::DKV(std::set<std::string> ips, int port, std::string my_ip, int weight)
      : m_device_manager(DeviceManager::GetInstance()),
        m_ips(ips),
        m_my_ip(my_ip),
        m_port(port),
        m_weight(weight) {
    m_defaultKvStoreSyncCallback_ptr = make_shared<DefaultKvStoreSyncCallback>();
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
    m_network_id = deviceInfo.networkId;
    cout << "deviceInfo.networkId: " << m_network_id << endl;
    m_server_ptr
        = make_shared<weight_raft::WeightServer>(m_base_dir, ips, m_port, m_network_id, m_my_ip);
    m_server_ptr->start();

    create_dir(m_base_dir);
    Options options = {.createIfMissing = true,
                       .encrypt = false,
                       .autoSync = false,
                       .kvStoreType = KvStoreType::SINGLE_VERSION,
                       .baseDir = m_base_dir};
    options.area = EL1;
    options.securityLevel = SecurityLevel::S1;

    OHOS::DistributedKv::Status status = m_kv_data_manager.GetSingleKvStore(
        options, AppId({m_app_id}), StoreId({m_store_id}), m_kv_store_ptr);
    if (status != OHOS::DistributedKv::Status::SUCCESS) {
      throw runtime_error("init failed");
    }

    status = m_kv_store_ptr->SubscribeKvStore(SubscribeType::SUBSCRIBE_TYPE_ALL,
                                              make_shared<DefaultKvStoreObserver>(this));
    if (status != Status::SUCCESS) {
      throw runtime_error("subscribe failed");
    }

    // create_dir(m_main_base_dir);
    // Options main_options = {.createIfMissing = true,
    //                         .encrypt = false,
    //                         .autoSync = false,
    //                         .kvStoreType = KvStoreType::SINGLE_VERSION,
    //                         .baseDir = m_main_base_dir};
    // main_options.area = EL1;
    // main_options.securityLevel = SecurityLevel::S1;

    status = m_kv_data_manager.GetSingleKvStore(options, AppId({m_app_id}),
                                                StoreId({m_main_store_id}), m_kv_main_store_ptr);
    if (status != OHOS::DistributedKv::Status::SUCCESS) {
      throw runtime_error("init failed");
    }
    int retryCount = 3;
    for (int i = 0; i < retryCount; i++) {
      try {
        m_server_ptr->setWeight(m_weight);
        cout << "success" << endl;
        break;
      } catch (const std::runtime_error &e) {
        std::cerr << "Attempt " << (i + 1) << " failed: " << e.what() << std::endl;
        if (i < retryCount - 1) {
          std::this_thread::sleep_for(
              std::chrono::seconds(2));  // Sleep for 2 seconds before retrying
        }
      }
    }
  }

  std::string DKV::get_master() { return m_server_ptr->get_leader_network_id(); }

  OHOS::DistributedKv::Status DKV::Get(const Key &key, Value &value) {
    return m_kv_main_store_ptr->Get(key, value);
  }

  // StoreId DKV::GetStoreId() const { return m_kv_store_ptr->GetStoreId(); }

  OHOS::DistributedKv::Status DKV::Put(const Key &key, const Value &value) {
    OHOS::DistributedKv::Status status = m_kv_store_ptr->Put(key, value);
    if (status != OHOS::DistributedKv::Status::SUCCESS) {
      cout << "local put error" << status << endl;
      return status;
    }
    DataQuery query;
    std::vector<std::string> keys = {key.ToString()};
    query.InKeys(keys);
    return m_kv_store_ptr->Sync({get_master()}, SyncMode::PUSH, query,
                                m_defaultKvStoreSyncCallback_ptr);
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
    OHOS::DistributedKv::Status status = m_kv_store_ptr->Delete(key);
    if (status != OHOS::DistributedKv::Status::SUCCESS) {
      return status;
    }
    DataQuery query;
    std::vector<std::string> keys = {key.ToString()};
    query.InKeys(keys);
    return m_kv_store_ptr->Sync({get_master()}, SyncMode::PUSH, query,
                                m_defaultKvStoreSyncCallback_ptr);
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