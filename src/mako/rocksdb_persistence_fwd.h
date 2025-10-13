#ifndef MAKO_ROCKSDB_PERSISTENCE_FWD_H
#define MAKO_ROCKSDB_PERSISTENCE_FWD_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <string>

namespace mako {

// Forward declaration only
class RocksDBPersistence {
public:
    static RocksDBPersistence& getInstance();

    bool initialize(const std::string& db_path, size_t num_partitions, size_t num_threads = 4);
    void shutdown();

    std::future<bool> persistAsync(const char* data, size_t size,
                                   uint32_t shard_id, uint32_t partition_id,
                                   std::function<void(bool)> callback = nullptr);

    void setEpoch(uint32_t epoch);
};

} // namespace mako

#endif // MAKO_ROCKSDB_PERSISTENCE_FWD_H