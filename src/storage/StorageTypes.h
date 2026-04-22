#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "../core/model/CellState.h"
#include "../core/model/EnvironmentState.h"

namespace acell
{
namespace storage
{

struct EventLogEntry
{
    std::int64_t id = 0;
    std::int64_t cell_id = 0;
    std::optional<std::int64_t> session_id;

    std::string event_type;
    std::string event_severity = "info";
    std::string event_time;
    double sim_time_seconds = 0.0;

    std::string title;
    std::string description;
    std::string payload_json;

    std::optional<double> mass_before;
    std::optional<double> mass_after;
    std::optional<double> energy_before;
    std::optional<double> energy_after;
    std::optional<double> health_before;
    std::optional<double> health_after;
    std::optional<double> stress_before;
    std::optional<double> stress_after;

    std::string created_at;
};

struct CellSnapshotRecord
{
    core::CellState cell;
    std::string last_simulated_at;
    std::string created_at;
    std::string updated_at;
};

struct EnvironmentSnapshotRecord
{
    core::EnvironmentState env;
    std::string last_simulated_at;
    std::string created_at;
    std::string updated_at;
};

struct UserActionEntry
{
    std::int64_t id = 0;
    std::int64_t cell_id = 0;
    std::optional<std::int64_t> session_id;

    std::string action_type;
    std::string action_time;
    std::string payload_json;
    std::string created_at;
};

} // namespace storage
} // namespace acell