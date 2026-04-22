#pragma once

#include "../model/CellState.h"
#include "../model/EnvironmentState.h"
#include "../model/SimulationConfig.h"
#include "../update/CellUpdater.h"

#include <cstddef>

namespace acell
{
namespace core
{

struct FastForwardOptions
{
    bool precise = false;
    bool stop_on_death = true;
    double max_step_override_seconds = 0.0;
};

struct FastForwardSummary
{
    double requested_elapsed_seconds = 0.0;
    double simulated_elapsed_seconds = 0.0;
    double remaining_elapsed_seconds = 0.0;

    std::size_t step_count = 0;
    std::size_t entered_dormancy_count = 0;
    std::size_t exited_dormancy_count = 0;
    std::size_t division_ready_count = 0;
    std::size_t death_count = 0;

    double total_uptake = 0.0;
    double total_storage_release = 0.0;
    double total_energy_production = 0.0;
    double total_maintenance_cost = 0.0;
    double total_energy_deficit = 0.0;
    double total_growth_delta = 0.0;

    bool terminated_by_death = false;
};

class FastForwardEngine
{
public:
    static FastForwardSummary run(CellState& cell,
                                  const EnvironmentState& env,
                                  const SimulationConfig& cfg,
                                  double elapsed_seconds,
                                  const FastForwardOptions& options = {});

private:
    static double chooseStepSeconds(const CellState& cell,
                                    const SimulationConfig& cfg,
                                    double remaining_seconds,
                                    const FastForwardOptions& options);

    static bool needsFineStep(const CellState& cell);
};

} // namespace core
} // namespace acell