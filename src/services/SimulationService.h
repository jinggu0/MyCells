#pragma once

#include "../core/model/CellState.h"
#include "../core/model/EnvironmentState.h"
#include "../core/model/SimulationConfig.h"
#include "../core/time/FastForwardEngine.h"

#include "../storage/Database.h"
#include "../storage/CellRepository.h"
#include "../storage/EnvironmentRepository.h"
#include "../storage/EventLogRepository.h"
#include "../storage/UserActionRepository.h"
#include "../storage/StorageTypes.h"

#include <string>
#include <vector>

namespace acell
{
namespace services
{

class SimulationService
{
public:
    explicit SimulationService(const std::string& db_path);

    void initialize(const std::string& init_sql_path,
                    const std::string& default_uuid = "default-cell");

    void update(double dt);
    void saveNow();

    void feed(double amount);
    void setTemperature(double temperature);
    void setPh(double ph);
    void reduceToxin(double amount);
    void renameCell(const std::string& new_name);
    void resetSimulation(bool clearHistory = true);

    std::vector<storage::EventLogEntry> loadRecentEvents(int limit);
    std::vector<storage::UserActionEntry> loadRecentActions(int limit);

    const core::CellState& cell() const;
    const core::EnvironmentState& environment() const;
    const core::SimulationConfig& config() const;

    core::CellState& mutableCell();
    core::EnvironmentState& mutableEnvironment();
    core::SimulationConfig& mutableConfig();

    std::int64_t cellId() const;
    const std::string& lastSimulatedAt() const;

private:
    storage::Database db_;
    storage::CellRepository cell_repo_;
    storage::EnvironmentRepository env_repo_;
    storage::EventLogRepository event_repo_;
    storage::UserActionRepository action_repo_;

    std::int64_t cell_id_ = 0;
    std::string cell_uuid_;

    core::CellState cell_;
    core::EnvironmentState env_;
    core::SimulationConfig cfg_;

    std::string last_simulated_at_;
    bool initialized_ = false;

private:
    void initializeSchema(const std::string& init_sql_path);
    void ensureDefaultStateExists();
    void loadStateOrThrow();
    void resumeOfflineGrowth();

    void persistCurrentState(const std::string& now);
    void logUpdateEvents(const core::CellState& before,
                         const core::CellUpdateResult& result,
                         const std::string& event_time);

    void logSimpleEvent(const std::string& event_type,
                        const std::string& severity,
                        const std::string& title,
                        const std::string& description,
                        const std::string& event_time);

    void logUserAction(const std::string& action_type,
                       const std::string& payload_json,
                       const std::string& action_time);

    static std::string readTextFile(const std::string& path);
};

} // namespace services
} // namespace acell