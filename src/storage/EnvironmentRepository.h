#pragma once

#include "../core/model/EnvironmentState.h"
#include "Database.h"
#include "StorageTypes.h"

#include <optional>
#include <cstdint>
#include <string>

namespace acell
{
namespace storage
{

class EnvironmentRepository
{
public:
    explicit EnvironmentRepository(Database& db);

    void saveSnapshot(std::int64_t cell_id,
                      const core::EnvironmentState& env,
                      const std::string& last_simulated_at,
                      const std::string& updated_at);

    std::optional<core::EnvironmentState> loadSnapshot(std::int64_t cell_id);
    std::optional<EnvironmentSnapshotRecord> loadSnapshotRecord(std::int64_t cell_id);

private:
    Database& db_;
};

} // namespace storage
} // namespace acell