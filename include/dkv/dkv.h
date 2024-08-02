#ifndef DKV_H
#define DKV_H

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <shared_mutex>
#include <string>
#include <vector>

#include "device_manager.h"
#include "distributed_kv_data_manager.h"
#include "types.h"
#include "weight_raft/server.h"

namespace dkv {

  class DefaultDmInitCallback : public OHOS::DistributedHardware::DmInitCallback {
  public:
    DefaultDmInitCallback() : DmInitCallback() {}
    virtual ~DefaultDmInitCallback() override {}
    virtual void OnRemoteDied() override {}
  };

  class DKV {  // : public OHOS::DistributedKv::KvStore {
    OHOS::DistributedHardware::DeviceManager &m_device_manager;
    std::string m_device_id;
    const static std::string m_app_id;
    const static std::string m_store_id;
    const static std::string m_base_dir;

    std::shared_mutex m_masters_mutex;
    std::vector<std::string> m_masters;
    std::shared_ptr<OHOS::DistributedKv::SingleKvStore> m_kv_store_ptr = nullptr;
    std::shared_ptr<weight_raft::WeightServer> m_server_ptr = nullptr;
    std::set<std::string> m_ips;
    std::string m_my_ip;
    int m_port;
    int m_weight;

    OHOS::DistributedKv::DistributedKvDataManager m_kv_data_manager;

  public:
    DKV(std::set<std::string> ips, int port, std::string my_ip, int weight);
    std::string get_master();
    OHOS::DistributedKv::Status Get(const OHOS::DistributedKv::Key &key,
                                    OHOS::DistributedKv::Value &value);
    OHOS::DistributedKv::Status Put(const OHOS::DistributedKv::Key &key,
                                    const OHOS::DistributedKv::Value &value);  // override;

    // OHOS::DistributedKv::Status PutBatch(
    //     const std::vector<OHOS::DistributedKv::Entry> &entries) override;

    OHOS::DistributedKv::Status Delete(const OHOS::DistributedKv::Key &key);  // override;

    // OHOS::DistributedKv::Status DeleteBatch(
    //     const std::vector<OHOS::DistributedKv::Key> &keys) override;

    // OHOS::DistributedKv::Status StartTransaction() override;

    // OHOS::DistributedKv::Status Commit() override;

    // OHOS::DistributedKv::Status Rollback() override;

    // OHOS::DistributedKv::Status SubscribeKvStore(
    //     OHOS::DistributedKv::SubscribeType type,
    //     std::shared_ptr<OHOS::DistributedKv::KvStoreObserver> observer)
    //     override;

    // OHOS::DistributedKv::Status UnSubscribeKvStore(
    //     OHOS::DistributedKv::SubscribeType type,
    //     std::shared_ptr<OHOS::DistributedKv::KvStoreObserver> observer)
    //     override;

    // OHOS::DistributedKv::Status Backup(const std::string &file,
    //                                    const std::string &baseDir) override;

    // OHOS::DistributedKv::Status Restore(const std::string &file,
    //                                     const std::string &baseDir) override;

    // OHOS::DistributedKv::Status DeleteBackup(
    //     const std::vector<std::string> &files, const std::string &baseDir,
    //     std::map<std::string, OHOS::DistributedKv::Status>
    //         &OHOS::DistributedKv::status) override;
  };
}  // namespace dkv
#endif