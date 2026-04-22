#include "CellRepository.h"

#include "SqliteCompat.h"

namespace acell
{
namespace storage
{

namespace
{

core::CellStatus parseCellStatus(const std::string& s)
{
    if(s == "dormant") return core::CellStatus::Dormant;
    if(s == "dividing") return core::CellStatus::Dividing;
    if(s == "dead") return core::CellStatus::Dead;
    return core::CellStatus::Alive;
}

core::GrowthPhase parseGrowthPhase(const std::string& s)
{
    if(s == "active") return core::GrowthPhase::Active;
    if(s == "stressed") return core::GrowthPhase::Stressed;
    if(s == "dormant") return core::GrowthPhase::Dormant;
    if(s == "dying") return core::GrowthPhase::Dying;
    return core::GrowthPhase::Lag;
}

core::ModelType parseModelType(const std::string& s)
{
    if(s == "MSC-1") return core::ModelType::MSC1;
    return core::ModelType::MSC1;
}

core::CellState readJoinedCell(sqlite3_stmt* stmt)
{
    core::CellState cell;
    cell.id = columnInt64(stmt, 0);
    cell.uuid = columnText(stmt, 1);
    cell.name = columnText(stmt, 2);
    cell.model_type = parseModelType(columnText(stmt, 3));
    cell.generation = columnInt64(stmt, 4);
    cell.status = parseCellStatus(columnText(stmt, 5));

    cell.mass = columnDouble(stmt, 6);
    cell.volume = columnDouble(stmt, 7);
    cell.surface_area = columnDouble(stmt, 8);
    cell.internal_nutrient = columnDouble(stmt, 9);
    cell.stored_nutrient = columnDouble(stmt, 10);
    cell.energy = columnDouble(stmt, 11);
    cell.health = columnDouble(stmt, 12);
    cell.stress = columnDouble(stmt, 13);
    cell.membrane_integrity = columnDouble(stmt, 14);
    cell.age_seconds = columnDouble(stmt, 15);
    cell.division_readiness = columnDouble(stmt, 16);
    cell.dormancy_state = columnDouble(stmt, 17);
    cell.alive = columnInt64(stmt, 18) != 0;
    cell.growth_phase = parseGrowthPhase(columnText(stmt, 19));
    cell.cell_cycle_progress = columnDouble(stmt, 20);

    cell.normalize();
    return cell;
}

} // namespace

CellRepository::CellRepository(Database& db)
    : db_(db)
{
}

std::int64_t CellRepository::createProfile(const core::CellState& cell,
                                           const std::string& created_at,
                                           const std::string& updated_at)
{
    static const char* sql =
        "INSERT INTO cell_profiles "
        "(uuid, name, model_type, lineage_id, generation, status, created_at, updated_at) "
        "VALUES (?, ?, ?, NULL, ?, ?, ?, ?);";

    Statement stmt(db_.handle(), sql);
    bindText(stmt.get(), 1, cell.uuid);
    bindText(stmt.get(), 2, cell.name);
    bindText(stmt.get(), 3, core::toString(cell.model_type));
    bindInt64(stmt.get(), 4, cell.generation);
    bindText(stmt.get(), 5, core::toString(cell.status));
    bindText(stmt.get(), 6, created_at);
    bindText(stmt.get(), 7, updated_at);

    if(sqlite3_step(stmt.get()) != SQLITE_DONE)
    {
        throwSqliteError(db_.handle(), "createProfile");
    }

    return db_.lastInsertRowId();
}

void CellRepository::updateProfileMetadata(const core::CellState& cell,
                                           const std::string& updated_at)
{
    static const char* sql =
        "UPDATE cell_profiles "
        "SET name = ?, model_type = ?, generation = ?, status = ?, updated_at = ? "
        "WHERE id = ?;";

    Statement stmt(db_.handle(), sql);
    bindText(stmt.get(), 1, cell.name);
    bindText(stmt.get(), 2, core::toString(cell.model_type));
    bindInt64(stmt.get(), 3, cell.generation);
    bindText(stmt.get(), 4, core::toString(cell.status));
    bindText(stmt.get(), 5, updated_at);
    bindInt64(stmt.get(), 6, cell.id);

    if(sqlite3_step(stmt.get()) != SQLITE_DONE)
    {
        throwSqliteError(db_.handle(), "updateProfileMetadata");
    }
}

void CellRepository::saveSnapshot(std::int64_t cell_id,
                                  const core::CellState& cell,
                                  const std::string& last_simulated_at,
                                  const std::string& updated_at)
{
    static const char* sql =
        "INSERT INTO cell_snapshots ("
        "cell_id, mass, volume, surface_area, internal_nutrient, stored_nutrient, "
        "energy, health, stress, membrane_integrity, age_seconds, division_readiness, "
        "dormancy_state, alive, growth_phase, cell_cycle_progress, "
        "last_simulated_at, created_at, updated_at"
        ") VALUES ("
        "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?"
        ") "
        "ON CONFLICT(cell_id) DO UPDATE SET "
        "mass = excluded.mass, "
        "volume = excluded.volume, "
        "surface_area = excluded.surface_area, "
        "internal_nutrient = excluded.internal_nutrient, "
        "stored_nutrient = excluded.stored_nutrient, "
        "energy = excluded.energy, "
        "health = excluded.health, "
        "stress = excluded.stress, "
        "membrane_integrity = excluded.membrane_integrity, "
        "age_seconds = excluded.age_seconds, "
        "division_readiness = excluded.division_readiness, "
        "dormancy_state = excluded.dormancy_state, "
        "alive = excluded.alive, "
        "growth_phase = excluded.growth_phase, "
        "cell_cycle_progress = excluded.cell_cycle_progress, "
        "last_simulated_at = excluded.last_simulated_at, "
        "updated_at = excluded.updated_at;";

    Statement stmt(db_.handle(), sql);

    bindInt64(stmt.get(), 1, cell_id);
    bindDouble(stmt.get(), 2, cell.mass);
    bindDouble(stmt.get(), 3, cell.volume);
    bindDouble(stmt.get(), 4, cell.surface_area);
    bindDouble(stmt.get(), 5, cell.internal_nutrient);
    bindDouble(stmt.get(), 6, cell.stored_nutrient);
    bindDouble(stmt.get(), 7, cell.energy);
    bindDouble(stmt.get(), 8, cell.health);
    bindDouble(stmt.get(), 9, cell.stress);
    bindDouble(stmt.get(), 10, cell.membrane_integrity);
    bindDouble(stmt.get(), 11, cell.age_seconds);
    bindDouble(stmt.get(), 12, cell.division_readiness);
    bindDouble(stmt.get(), 13, cell.dormancy_state);
    bindInt64(stmt.get(), 14, cell.alive ? 1 : 0);
    bindText(stmt.get(), 15, core::toString(cell.growth_phase));
    bindDouble(stmt.get(), 16, cell.cell_cycle_progress);
    bindText(stmt.get(), 17, last_simulated_at);
    bindText(stmt.get(), 18, updated_at);
    bindText(stmt.get(), 19, updated_at);

    if(sqlite3_step(stmt.get()) != SQLITE_DONE)
    {
        throwSqliteError(db_.handle(), "saveSnapshot");
    }
}

std::optional<core::CellState> CellRepository::loadProfileAndSnapshot(std::int64_t cell_id)
{
    return loadJoinedByWhere("cp.id = ?", std::to_string(cell_id), true);
}

std::optional<core::CellState> CellRepository::loadProfileAndSnapshotByUuid(const std::string& uuid)
{
    return loadJoinedByWhere("cp.uuid = ?", uuid, false);
}

std::optional<CellSnapshotRecord> CellRepository::loadSnapshotRecord(std::int64_t cell_id)
{
    return loadRecordByWhere("cp.id = ?", std::to_string(cell_id), true);
}

std::optional<CellSnapshotRecord> CellRepository::loadSnapshotRecordByUuid(const std::string& uuid)
{
    return loadRecordByWhere("cp.uuid = ?", uuid, false);
}

std::optional<core::CellState> CellRepository::loadJoinedByWhere(const std::string& where_clause,
                                                                 const std::string& param,
                                                                 bool by_id)
{
    auto rec = loadRecordByWhere(where_clause, param, by_id);
    if(!rec.has_value())
    {
        return std::nullopt;
    }
    return rec->cell;
}

std::optional<CellSnapshotRecord> CellRepository::loadRecordByWhere(const std::string& where_clause,
                                                                    const std::string& param,
                                                                    bool by_id)
{
    const std::string sql =
        "SELECT "
        "cp.id, cp.uuid, cp.name, cp.model_type, cp.generation, cp.status, "
        "cs.mass, cs.volume, cs.surface_area, cs.internal_nutrient, cs.stored_nutrient, "
        "cs.energy, cs.health, cs.stress, cs.membrane_integrity, cs.age_seconds, "
        "cs.division_readiness, cs.dormancy_state, cs.alive, cs.growth_phase, "
        "cs.cell_cycle_progress, cs.last_simulated_at, cs.created_at, cs.updated_at "
        "FROM cell_profiles cp "
        "JOIN cell_snapshots cs ON cp.id = cs.cell_id "
        "WHERE " + where_clause + " "
        "LIMIT 1;";

    Statement stmt(db_.handle(), sql);

    if(by_id)
    {
        bindInt64(stmt.get(), 1, std::stoll(param));
    }
    else
    {
        bindText(stmt.get(), 1, param);
    }

    const int rc = sqlite3_step(stmt.get());
    if(rc == SQLITE_DONE)
    {
        return std::nullopt;
    }
    if(rc != SQLITE_ROW)
    {
        throwSqliteError(db_.handle(), "loadSnapshotRecord");
    }

    CellSnapshotRecord rec;
    rec.cell = readJoinedCell(stmt.get());
    rec.last_simulated_at = columnText(stmt.get(), 21);
    rec.created_at = columnText(stmt.get(), 22);
    rec.updated_at = columnText(stmt.get(), 23);
    return rec;
}

} // namespace storage
} // namespace acell
