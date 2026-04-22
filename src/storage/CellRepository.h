#pragma once

#include "../core/model/CellState.h"
#include "Database.h"
#include "StorageTypes.h"

#include <optional>
#include <string>

namespace acell
{
namespace storage
{

class CellRepository
{
public:
    explicit CellRepository(Database& db);

    std::int64_t createProfile(const core::CellState& cell,
                               const std::string& created_at,
                               const std::string& updated_at);

    void updateProfileMetadata(const core::CellState& cell,
                               const std::string& updated_at);

    std::optional<core::CellState> loadProfileAndSnapshot(std::int64_t cell_id);
    std::optional<core::CellState> loadProfileAndSnapshotByUuid(const std::string& uuid);

    std::optional<CellSnapshotRecord> loadSnapshotRecord(std::int64_t cell_id);
    std::optional<CellSnapshotRecord> loadSnapshotRecordByUuid(const std::string& uuid);

    void saveSnapshot(std::int64_t cell_id,
                      const core::CellState& cell,
                      const std::string& last_simulated_at,
                      const std::string& updated_at);

private:
    Database& db_;

    std::optional<core::CellState> loadJoinedByWhere(const std::string& where_clause,
                                                     const std::string& param,
                                                     bool by_id);

    std::optional<CellSnapshotRecord> loadRecordByWhere(const std::string& where_clause,
                                                        const std::string& param,
                                                        bool by_id);
};

} // namespace storage
} // namespace acell