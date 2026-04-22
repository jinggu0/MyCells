#include "EnvironmentRepository.h"

#include "SqliteCompat.h"

namespace acell
{
namespace storage
{

EnvironmentRepository::EnvironmentRepository(Database& db)
    : db_(db)
{
}

void EnvironmentRepository::saveSnapshot(std::int64_t cell_id,
                                         const core::EnvironmentState& env,
                                         const std::string& last_simulated_at,
                                         const std::string& updated_at)
{
    static const char* sql =
        "INSERT INTO environment_snapshots ("
        "cell_id, nutrient_density, oxygen_level, temperature, ph, osmolarity, "
        "toxin_level, diffusion_rate, micronutrient_level, oxidative_stress_field, "
        "mechanical_stress, resource_gradient, last_simulated_at, created_at, updated_at"
        ") VALUES ("
        "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?"
        ") "
        "ON CONFLICT(cell_id) DO UPDATE SET "
        "nutrient_density = excluded.nutrient_density, "
        "oxygen_level = excluded.oxygen_level, "
        "temperature = excluded.temperature, "
        "ph = excluded.ph, "
        "osmolarity = excluded.osmolarity, "
        "toxin_level = excluded.toxin_level, "
        "diffusion_rate = excluded.diffusion_rate, "
        "micronutrient_level = excluded.micronutrient_level, "
        "oxidative_stress_field = excluded.oxidative_stress_field, "
        "mechanical_stress = excluded.mechanical_stress, "
        "resource_gradient = excluded.resource_gradient, "
        "last_simulated_at = excluded.last_simulated_at, "
        "updated_at = excluded.updated_at;";

    Statement stmt(db_.handle(), sql);
    bindInt64(stmt.get(), 1, cell_id);
    bindDouble(stmt.get(), 2, env.nutrient_density);
    bindDouble(stmt.get(), 3, env.oxygen_level);
    bindDouble(stmt.get(), 4, env.temperature);
    bindDouble(stmt.get(), 5, env.ph);
    bindDouble(stmt.get(), 6, env.osmolarity);
    bindDouble(stmt.get(), 7, env.toxin_level);
    bindDouble(stmt.get(), 8, env.diffusion_rate);
    bindDouble(stmt.get(), 9, env.micronutrient_level);
    bindDouble(stmt.get(), 10, env.oxidative_stress_field);
    bindDouble(stmt.get(), 11, env.mechanical_stress);
    bindDouble(stmt.get(), 12, env.resource_gradient);
    bindText(stmt.get(), 13, last_simulated_at);
    bindText(stmt.get(), 14, updated_at);
    bindText(stmt.get(), 15, updated_at);

    if(sqlite3_step(stmt.get()) != SQLITE_DONE)
    {
        throwSqliteError(db_.handle(), "EnvironmentRepository::saveSnapshot");
    }
}

std::optional<core::EnvironmentState> EnvironmentRepository::loadSnapshot(std::int64_t cell_id)
{
    auto rec = loadSnapshotRecord(cell_id);
    if(!rec.has_value())
    {
        return std::nullopt;
    }
    return rec->env;
}

std::optional<EnvironmentSnapshotRecord> EnvironmentRepository::loadSnapshotRecord(std::int64_t cell_id)
{
    static const char* sql =
        "SELECT nutrient_density, oxygen_level, temperature, ph, osmolarity, "
        "toxin_level, diffusion_rate, micronutrient_level, oxidative_stress_field, "
        "mechanical_stress, resource_gradient, last_simulated_at, created_at, updated_at "
        "FROM environment_snapshots "
        "WHERE cell_id = ? "
        "LIMIT 1;";

    Statement stmt(db_.handle(), sql);
    bindInt64(stmt.get(), 1, cell_id);

    const int rc = sqlite3_step(stmt.get());
    if(rc == SQLITE_DONE)
    {
        return std::nullopt;
    }
    if(rc != SQLITE_ROW)
    {
        throwSqliteError(db_.handle(), "EnvironmentRepository::loadSnapshotRecord");
    }

    EnvironmentSnapshotRecord rec;
    rec.env.nutrient_density = columnDouble(stmt.get(), 0);
    rec.env.oxygen_level = columnDouble(stmt.get(), 1);
    rec.env.temperature = columnDouble(stmt.get(), 2);
    rec.env.ph = columnDouble(stmt.get(), 3);
    rec.env.osmolarity = columnDouble(stmt.get(), 4);
    rec.env.toxin_level = columnDouble(stmt.get(), 5);
    rec.env.diffusion_rate = columnDouble(stmt.get(), 6);
    rec.env.micronutrient_level = columnDouble(stmt.get(), 7);
    rec.env.oxidative_stress_field = columnDouble(stmt.get(), 8);
    rec.env.mechanical_stress = columnDouble(stmt.get(), 9);
    rec.env.resource_gradient = columnDouble(stmt.get(), 10);
    rec.last_simulated_at = columnText(stmt.get(), 11);
    rec.created_at = columnText(stmt.get(), 12);
    rec.updated_at = columnText(stmt.get(), 13);

    rec.env.normalize();
    return rec;
}

} // namespace storage
} // namespace acell
