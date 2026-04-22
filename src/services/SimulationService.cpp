#include "SimulationService.h"

#include "../utils/TimeUtils.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace acell
{
namespace services
{

SimulationService::SimulationService(const std::string& db_path)
    : db_(db_path),
      cell_repo_(db_),
      env_repo_(db_),
      event_repo_(db_),
      action_repo_(db_)
{
    cfg_.normalize();
}

void SimulationService::initialize(const std::string& init_sql_path,
                                   const std::string& default_uuid)
{
    cell_uuid_ = default_uuid;

    initializeSchema(init_sql_path);
    ensureDefaultStateExists();
    loadStateOrThrow();
    resumeOfflineGrowth();

    initialized_ = true;
}

void SimulationService::initializeSchema(const std::string& init_sql_path)
{
    db_.exec(readTextFile(init_sql_path));
}

void SimulationService::ensureDefaultStateExists()
{
    auto existing = cell_repo_.loadSnapshotRecordByUuid(cell_uuid_);
    if(existing.has_value())
    {
        cell_id_ = existing->cell.id;
        return;
    }

    const std::string now = utils::nowIsoUtc();

    core::CellState cell;
    cell.uuid = cell_uuid_;
    cell.name = "My Cell";
    cell.model_type = core::ModelType::MSC1;
    cell.status = core::CellStatus::Alive;
    cell.growth_phase = core::GrowthPhase::Lag;
    cell.normalize();

    core::EnvironmentState env;
    env.normalize();

    db_.beginTransaction();
    try
    {
        cell_id_ = cell_repo_.createProfile(cell, now, now);
        cell.id = cell_id_;

        cell_repo_.saveSnapshot(cell_id_, cell, now, now);
        env_repo_.saveSnapshot(cell_id_, env, now, now);

        logSimpleEvent(
            "cell_created",
            "info",
            "Cell created",
            "Default cell and environment were created.",
            now
        );

        db_.commit();
    }
    catch(...)
    {
        db_.rollback();
        throw;
    }
}

void SimulationService::loadStateOrThrow()
{
    auto cell_rec = cell_repo_.loadSnapshotRecordByUuid(cell_uuid_);
    if(!cell_rec.has_value())
    {
        throw std::runtime_error("Failed to load cell snapshot record.");
    }

    cell_ = cell_rec->cell;
    cell_id_ = cell_.id;
    last_simulated_at_ = cell_rec->last_simulated_at;

    auto env_rec = env_repo_.loadSnapshotRecord(cell_id_);
    if(!env_rec.has_value())
    {
        throw std::runtime_error("Failed to load environment snapshot record.");
    }

    env_ = env_rec->env;
    cfg_.normalize();
}

void SimulationService::resumeOfflineGrowth()
{
    const std::string now = utils::nowIsoUtc();
    const double elapsed = utils::elapsedSecondsUtc(last_simulated_at_, now);

    if(elapsed <= 0.0)
    {
        last_simulated_at_ = now;
        return;
    }

    core::CellState before = cell_;

    core::FastForwardSummary summary =
        core::FastForwardEngine::run(cell_, env_, cfg_, elapsed);

    last_simulated_at_ = now;

    db_.beginTransaction();
    try
    {
        persistCurrentState(now);

        if(summary.simulated_elapsed_seconds > 0.0)
        {
            storage::EventLogEntry ev;
            ev.cell_id = cell_id_;
            ev.event_type = "offline_growth_summary";
            ev.event_severity = "info";
            ev.event_time = now;
            ev.sim_time_seconds = cell_.age_seconds;
            ev.title = "Offline growth applied";
            ev.description = "Cell state was advanced using offline fast-forward.";
            ev.payload_json =
                "{"
                "\"requested_elapsed_seconds\":" + std::to_string(summary.requested_elapsed_seconds) + ","
                "\"simulated_elapsed_seconds\":" + std::to_string(summary.simulated_elapsed_seconds) + ","
                "\"steps\":" + std::to_string(summary.step_count) + ","
                "\"growth_delta\":" + std::to_string(summary.total_growth_delta) + ","
                "\"terminated_by_death\":" + std::string(summary.terminated_by_death ? "true" : "false") +
                "}";
            ev.mass_before = before.mass;
            ev.mass_after = cell_.mass;
            ev.energy_before = before.energy;
            ev.energy_after = cell_.energy;
            ev.health_before = before.health;
            ev.health_after = cell_.health;
            ev.stress_before = before.stress;
            ev.stress_after = cell_.stress;
            ev.created_at = now;
            event_repo_.insert(ev);
        }

        db_.commit();
    }
    catch(...)
    {
        db_.rollback();
        throw;
    }
}

void SimulationService::update(double dt)
{
    if(!initialized_)
    {
        throw std::runtime_error("SimulationService::initialize must be called first.");
    }

    const std::string now = utils::nowIsoUtc();
    core::CellState before = cell_;

    core::CellUpdateResult result = core::CellUpdater::update(cell_, env_, cfg_, dt);
    last_simulated_at_ = now;

    db_.beginTransaction();
    try
    {
        persistCurrentState(now);
        logUpdateEvents(before, result, now);
        db_.commit();
    }
    catch(...)
    {
        db_.rollback();
        throw;
    }
}

void SimulationService::saveNow()
{
    const std::string now = utils::nowIsoUtc();
    last_simulated_at_ = now;

    db_.beginTransaction();
    try
    {
        persistCurrentState(now);
        db_.commit();
    }
    catch(...)
    {
        db_.rollback();
        throw;
    }
}

void SimulationService::feed(double amount)
{
    if(!initialized_)
    {
        throw std::runtime_error("SimulationService::initialize must be called first.");
    }

    amount = std::max(amount, 0.0);
    const std::string now = utils::nowIsoUtc();

    db_.beginTransaction();
    try
    {
        const double before = env_.nutrient_density;
        env_.nutrient_density += amount;
        env_.normalize();

        persistCurrentState(now);

        logUserAction(
            "feed",
            "{"
            "\"amount\":" + std::to_string(amount) + ","
            "\"nutrient_density_before\":" + std::to_string(before) + ","
            "\"nutrient_density_after\":" + std::to_string(env_.nutrient_density) +
            "}",
            now
        );

        logSimpleEvent(
            "user_feed_applied",
            "info",
            "Feed applied",
            "External nutrient density was increased by user action.",
            now
        );

        db_.commit();
    }
    catch(...)
    {
        db_.rollback();
        throw;
    }
}

void SimulationService::setTemperature(double temperature)
{
    if(!initialized_)
    {
        throw std::runtime_error("SimulationService::initialize must be called first.");
    }

    const std::string now = utils::nowIsoUtc();

    db_.beginTransaction();
    try
    {
        const double before = env_.temperature;
        env_.temperature = temperature;
        env_.normalize();

        persistCurrentState(now);

        logUserAction(
            "change_temperature",
            "{"
            "\"before\":" + std::to_string(before) + ","
            "\"after\":" + std::to_string(env_.temperature) +
            "}",
            now
        );

        logSimpleEvent(
            "user_environment_changed",
            "notice",
            "Temperature changed",
            "The environment temperature was changed by the user.",
            now
        );

        db_.commit();
    }
    catch(...)
    {
        db_.rollback();
        throw;
    }
}

void SimulationService::setPh(double ph)
{
    if(!initialized_)
    {
        throw std::runtime_error("SimulationService::initialize must be called first.");
    }

    const std::string now = utils::nowIsoUtc();

    db_.beginTransaction();
    try
    {
        const double before = env_.ph;
        env_.ph = ph;
        env_.normalize();

        persistCurrentState(now);

        logUserAction(
            "change_ph",
            "{"
            "\"before\":" + std::to_string(before) + ","
            "\"after\":" + std::to_string(env_.ph) +
            "}",
            now
        );

        logSimpleEvent(
            "user_environment_changed",
            "notice",
            "pH changed",
            "The environment pH was changed by the user.",
            now
        );

        db_.commit();
    }
    catch(...)
    {
        db_.rollback();
        throw;
    }
}

void SimulationService::reduceToxin(double amount)
{
    if(!initialized_)
    {
        throw std::runtime_error("SimulationService::initialize must be called first.");
    }

    amount = std::max(amount, 0.0);
    const std::string now = utils::nowIsoUtc();

    db_.beginTransaction();
    try
    {
        const double before = env_.toxin_level;
        env_.toxin_level = std::max(0.0, env_.toxin_level - amount);
        env_.normalize();

        persistCurrentState(now);

        logUserAction(
            "apply_toxin_reduction",
            "{"
            "\"amount\":" + std::to_string(amount) + ","
            "\"before\":" + std::to_string(before) + ","
            "\"after\":" + std::to_string(env_.toxin_level) +
            "}",
            now
        );

        logSimpleEvent(
            "user_intervention_applied",
            "info",
            "Toxin reduced",
            "The user reduced the environmental toxin level.",
            now
        );

        db_.commit();
    }
    catch(...)
    {
        db_.rollback();
        throw;
    }
}

void SimulationService::renameCell(const std::string& new_name)
{
    if(!initialized_)
    {
        throw std::runtime_error("SimulationService::initialize must be called first.");
    }

    if(new_name.empty())
    {
        return;
    }

    const std::string now = utils::nowIsoUtc();

    db_.beginTransaction();
    try
    {
        const std::string before = cell_.name;
        cell_.name = new_name;

        persistCurrentState(now);

        logUserAction(
            "rename_cell",
            "{"
            "\"before\":\"" + before + "\","
            "\"after\":\"" + cell_.name + "\""
            "}",
            now
        );

        logSimpleEvent(
            "cell_renamed",
            "info",
            "Cell renamed",
            "The user changed the cell name.",
            now
        );

        db_.commit();
    }
    catch(...)
    {
        db_.rollback();
        throw;
    }
}

const core::CellState& SimulationService::cell() const
{
    return cell_;
}

const core::EnvironmentState& SimulationService::environment() const
{
    return env_;
}

const core::SimulationConfig& SimulationService::config() const
{
    return cfg_;
}

core::CellState& SimulationService::mutableCell()
{
    return cell_;
}

core::EnvironmentState& SimulationService::mutableEnvironment()
{
    return env_;
}

core::SimulationConfig& SimulationService::mutableConfig()
{
    return cfg_;
}

std::int64_t SimulationService::cellId() const
{
    return cell_id_;
}

const std::string& SimulationService::lastSimulatedAt() const
{
    return last_simulated_at_;
}

void SimulationService::persistCurrentState(const std::string& now)
{
    cell_repo_.updateProfileMetadata(cell_, now);
    cell_repo_.saveSnapshot(cell_id_, cell_, last_simulated_at_, now);
    env_repo_.saveSnapshot(cell_id_, env_, last_simulated_at_, now);
}

void SimulationService::logUpdateEvents(const core::CellState& before,
                                        const core::CellUpdateResult& result,
                                        const std::string& event_time)
{
    if(result.entered_dormancy)
    {
        logSimpleEvent(
            "dormancy_entered",
            "notice",
            "Dormancy entered",
            "The cell entered dormancy due to stressful conditions.",
            event_time
        );
    }

    if(result.exited_dormancy)
    {
        logSimpleEvent(
            "dormancy_exited",
            "notice",
            "Dormancy exited",
            "The cell recovered enough to exit dormancy.",
            event_time
        );
    }

    if(result.became_division_ready)
    {
        logSimpleEvent(
            "division_ready",
            "info",
            "Division readiness reached",
            "The cell reached full division readiness.",
            event_time
        );
    }

    if(result.became_dead)
    {
        logSimpleEvent(
            "death",
            "critical",
            "Cell died",
            "The cell has died during simulation.",
            event_time
        );
    }

    if(cell_.stress - before.stress >= 0.05)
    {
        storage::EventLogEntry ev;
        ev.cell_id = cell_id_;
        ev.event_type = "stress_increase";
        ev.event_severity = cell_.stress >= 0.8 ? "warning" : "notice";
        ev.event_time = event_time;
        ev.sim_time_seconds = cell_.age_seconds;
        ev.title = "Stress increased";
        ev.description = "The cell experienced a notable increase in stress.";
        ev.payload_json =
            "{"
            "\"stress_delta\":" + std::to_string(cell_.stress - before.stress) +
            "}";
        ev.stress_before = before.stress;
        ev.stress_after = cell_.stress;
        ev.created_at = event_time;
        event_repo_.insert(ev);
    }

    if(cell_.mass - before.mass >= 0.02)
    {
        storage::EventLogEntry ev;
        ev.cell_id = cell_id_;
        ev.event_type = "growth_progress";
        ev.event_severity = "info";
        ev.event_time = event_time;
        ev.sim_time_seconds = cell_.age_seconds;
        ev.title = "Growth progressed";
        ev.description = "The cell mass increased during this update.";
        ev.payload_json =
            "{"
            "\"mass_delta\":" + std::to_string(cell_.mass - before.mass) + ","
            "\"energy_delta\":" + std::to_string(cell_.energy - before.energy) +
            "}";
        ev.mass_before = before.mass;
        ev.mass_after = cell_.mass;
        ev.energy_before = before.energy;
        ev.energy_after = cell_.energy;
        ev.created_at = event_time;
        event_repo_.insert(ev);
    }
}

void SimulationService::logSimpleEvent(const std::string& event_type,
                                       const std::string& severity,
                                       const std::string& title,
                                       const std::string& description,
                                       const std::string& event_time)
{
    storage::EventLogEntry ev;
    ev.cell_id = cell_id_;
    ev.event_type = event_type;
    ev.event_severity = severity;
    ev.event_time = event_time;
    ev.sim_time_seconds = cell_.age_seconds;
    ev.title = title;
    ev.description = description;
    ev.created_at = event_time;
    event_repo_.insert(ev);
}

void SimulationService::logUserAction(const std::string& action_type,
                                      const std::string& payload_json,
                                      const std::string& action_time)
{
    storage::UserActionEntry a;
    a.cell_id = cell_id_;
    a.action_type = action_type;
    a.action_time = action_time;
    a.payload_json = payload_json;
    a.created_at = action_time;
    action_repo_.insert(a);
}

std::string SimulationService::readTextFile(const std::string& path)
{
    std::ifstream ifs(path, std::ios::binary);
    if(!ifs)
    {
        throw std::runtime_error("Failed to open file: " + path);
    }

    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

std::vector<acell::storage::EventLogEntry> acell::services::SimulationService::loadRecentEvents(int limit)
{
    return event_repo_.loadRecent(cell_id_, limit);
}

std::vector<acell::storage::UserActionEntry> acell::services::SimulationService::loadRecentActions(int limit)
{
    return action_repo_.loadRecent(cell_id_, limit);
}

} // namespace services
} // namespace acell