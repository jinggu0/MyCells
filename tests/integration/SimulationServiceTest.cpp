#include "../core/CatchCompat.h"

#include "../../src/services/SimulationService.h"
#include "../../src/storage/Database.h"
#include "../../src/storage/EventLogRepository.h"
#include "../../src/storage/CellRepository.h"
#include "../../src/storage/EnvironmentRepository.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace
{

std::string minimalInitSql()
{
    return R"SQL(
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS cell_profiles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    uuid TEXT NOT NULL UNIQUE,
    name TEXT NOT NULL,
    model_type TEXT NOT NULL,
    lineage_id TEXT,
    generation INTEGER NOT NULL DEFAULT 0,
    status TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS cell_snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    cell_id INTEGER NOT NULL UNIQUE,
    mass REAL NOT NULL,
    volume REAL NOT NULL,
    surface_area REAL NOT NULL,
    internal_nutrient REAL NOT NULL,
    stored_nutrient REAL NOT NULL,
    energy REAL NOT NULL,
    health REAL NOT NULL,
    stress REAL NOT NULL,
    membrane_integrity REAL NOT NULL,
    age_seconds REAL NOT NULL,
    division_readiness REAL NOT NULL,
    dormancy_state REAL NOT NULL DEFAULT 0,
    alive INTEGER NOT NULL DEFAULT 1,
    growth_phase TEXT,
    cell_cycle_progress REAL NOT NULL DEFAULT 0,
    last_simulated_at TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    FOREIGN KEY (cell_id) REFERENCES cell_profiles(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS environment_snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    cell_id INTEGER NOT NULL UNIQUE,
    nutrient_density REAL NOT NULL,
    oxygen_level REAL NOT NULL,
    temperature REAL NOT NULL,
    ph REAL NOT NULL,
    osmolarity REAL NOT NULL,
    toxin_level REAL NOT NULL,
    diffusion_rate REAL NOT NULL,
    micronutrient_level REAL DEFAULT 1,
    oxidative_stress_field REAL DEFAULT 0,
    mechanical_stress REAL DEFAULT 0,
    resource_gradient REAL DEFAULT 0,
    last_simulated_at TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    FOREIGN KEY (cell_id) REFERENCES cell_profiles(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS simulation_sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    session_uuid TEXT NOT NULL UNIQUE,
    cell_id INTEGER NOT NULL,
    started_at TEXT NOT NULL,
    ended_at TEXT,
    resume_from TEXT,
    simulated_until TEXT,
    offline_elapsed_seconds REAL NOT NULL DEFAULT 0,
    notes TEXT,
    FOREIGN KEY (cell_id) REFERENCES cell_profiles(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS event_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    cell_id INTEGER NOT NULL,
    session_id INTEGER,
    event_type TEXT NOT NULL,
    event_severity TEXT NOT NULL,
    event_time TEXT NOT NULL,
    sim_time_seconds REAL,
    title TEXT NOT NULL,
    description TEXT,
    payload_json TEXT,
    mass_before REAL,
    mass_after REAL,
    energy_before REAL,
    energy_after REAL,
    health_before REAL,
    health_after REAL,
    stress_before REAL,
    stress_after REAL,
    created_at TEXT NOT NULL,
    FOREIGN KEY (cell_id) REFERENCES cell_profiles(id) ON DELETE CASCADE,
    FOREIGN KEY (session_id) REFERENCES simulation_sessions(id) ON DELETE SET NULL
);

CREATE INDEX IF NOT EXISTS idx_event_logs_cell_id_event_time
    ON event_logs(cell_id, event_time DESC);

CREATE TABLE IF NOT EXISTS user_actions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    cell_id INTEGER NOT NULL,
    session_id INTEGER,
    action_type TEXT NOT NULL,
    action_time TEXT NOT NULL,
    payload_json TEXT,
    created_at TEXT NOT NULL,
    FOREIGN KEY (cell_id) REFERENCES cell_profiles(id) ON DELETE CASCADE,
    FOREIGN KEY (session_id) REFERENCES simulation_sessions(id) ON DELETE SET NULL
);

CREATE INDEX IF NOT EXISTS idx_user_actions_cell_id_action_time
    ON user_actions(cell_id, action_time DESC);
)SQL";
}

struct TempPaths
{
    fs::path dir;
    fs::path db_path;
    fs::path sql_path;
};

TempPaths makeTempPaths(const std::string& name)
{
    TempPaths p;
    p.dir = fs::temp_directory_path() / ("mycells_test_" + name);
    fs::create_directories(p.dir);
    p.db_path = p.dir / "test.db";
    p.sql_path = p.dir / "init.sql";

    std::ofstream ofs(p.sql_path);
    ofs << minimalInitSql();
    ofs.close();

    return p;
}

void cleanupTempPaths(const TempPaths& p)
{
    std::error_code ec;
    fs::remove_all(p.dir, ec);
}

} // namespace

TEST_CASE("SimulationService initializes default state")
{
    TempPaths p = makeTempPaths("init");
    try
    {
        acell::services::SimulationService sim(p.db_path.string());
        sim.initialize(p.sql_path.string(), "default-cell");

        REQUIRE(sim.cellId() > 0);
        REQUIRE(sim.cell().alive);
        REQUIRE(sim.cell().uuid == "default-cell");
        REQUIRE(sim.cell().mass > 0.0);
        REQUIRE(sim.environment().nutrient_density >= 0.0);
        REQUIRE_FALSE(sim.lastSimulatedAt().empty());
    }
    catch(...)
    {
        cleanupTempPaths(p);
        throw;
    }
    cleanupTempPaths(p);
}

TEST_CASE("SimulationService update persists state")
{
    TempPaths p = makeTempPaths("persist");
    try
    {
        {
            acell::services::SimulationService sim(p.db_path.string());
            sim.initialize(p.sql_path.string(), "default-cell");

            const double mass_before = sim.cell().mass;
            sim.update(60.0);
            sim.saveNow();

            REQUIRE(sim.cell().mass >= mass_before);
        }

        acell::storage::Database db(p.db_path.string());
        acell::storage::CellRepository cell_repo(db);

        auto loaded = cell_repo.loadSnapshotRecordByUuid("default-cell");
        REQUIRE(loaded.has_value());
        REQUIRE(loaded->cell.mass > 0.0);
        REQUIRE_FALSE(loaded->last_simulated_at.empty());
    }
    catch(...)
    {
        cleanupTempPaths(p);
        throw;
    }
    cleanupTempPaths(p);
}

TEST_CASE("SimulationService resumes offline growth on reopen")
{
    TempPaths p = makeTempPaths("offline_resume");
    try
    {
        double first_age = 0.0;
        double first_mass = 0.0;

        {
            acell::services::SimulationService sim(p.db_path.string());
            sim.initialize(p.sql_path.string(), "default-cell");
            sim.update(120.0);
            sim.saveNow();

            first_age = sim.cell().age_seconds;
            first_mass = sim.cell().mass;
        }

        // Move the timestamp into the past so initialize() applies offline fast-forward.
        {
            acell::storage::Database db(p.db_path.string());
            const std::string sql =
                "UPDATE cell_snapshots "
                "SET last_simulated_at = datetime(last_simulated_at, '-2 hours') "
                "WHERE cell_id = (SELECT id FROM cell_profiles WHERE uuid = 'default-cell');";
            db.exec(sql);
        }

        {
            acell::services::SimulationService sim(p.db_path.string());
            sim.initialize(p.sql_path.string(), "default-cell");

            REQUIRE(sim.cell().age_seconds > first_age);
            REQUIRE(sim.cell().mass >= first_mass);
        }

        {
            acell::storage::Database db(p.db_path.string());
            acell::storage::CellRepository cell_repo(db);
            acell::storage::EventLogRepository event_repo(db);

            auto cell = cell_repo.loadSnapshotRecordByUuid("default-cell");
            REQUIRE(cell.has_value());

            auto events = event_repo.loadRecent(cell->cell.id, 20);
            bool found_offline_summary = false;

            for(const auto& ev : events)
            {
                if(ev.event_type == "offline_growth_summary")
                {
                    found_offline_summary = true;
                    break;
                }
            }

            REQUIRE(found_offline_summary);
        }
    }
    catch(...)
    {
        cleanupTempPaths(p);
        throw;
    }
    cleanupTempPaths(p);
}

TEST_CASE("SimulationService user actions persist and affect state")
{
    TempPaths p = makeTempPaths("user_actions");
    try
    {
        acell::services::SimulationService sim(p.db_path.string());
        sim.initialize(p.sql_path.string(), "default-cell");

        const double nutrient_before = sim.environment().nutrient_density;
        const double temp_before = sim.environment().temperature;
        const double ph_before = sim.environment().ph;

        sim.feed(0.75);
        sim.setTemperature(temp_before + 2.0);
        sim.setPh(ph_before - 0.3);
        sim.reduceToxin(0.1);
        sim.renameCell("Renamed Cell");
        sim.saveNow();

        REQUIRE(sim.environment().nutrient_density > nutrient_before);
        REQUIRE(sim.environment().temperature == Approx(temp_before + 2.0));
        REQUIRE(sim.environment().ph == Approx(ph_before - 0.3));
        REQUIRE(sim.cell().name == "Renamed Cell");

        acell::storage::Database db(p.db_path.string());
        acell::storage::UserActionRepository action_repo(db);
        acell::storage::CellRepository cell_repo(db);
        acell::storage::EnvironmentRepository env_repo(db);

        auto cell_rec = cell_repo.loadSnapshotRecordByUuid("default-cell");
        REQUIRE(cell_rec.has_value());
        REQUIRE(cell_rec->cell.name == "Renamed Cell");

        auto env_rec = env_repo.loadSnapshotRecord(cell_rec->cell.id);
        REQUIRE(env_rec.has_value());
        REQUIRE(env_rec->env.nutrient_density > nutrient_before);
        REQUIRE(env_rec->env.temperature == Approx(temp_before + 2.0));
        REQUIRE(env_rec->env.ph == Approx(ph_before - 0.3));

        auto actions = action_repo.loadRecent(cell_rec->cell.id, 10);
        REQUIRE(actions.size() >= 5);
    }
    catch(...)
    {
        cleanupTempPaths(p);
        throw;
    }
    cleanupTempPaths(p);
}
