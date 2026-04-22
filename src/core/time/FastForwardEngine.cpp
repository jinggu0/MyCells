#include "FastForwardEngine.h"

#include <algorithm>
#include <cmath>

namespace acell
{
namespace core
{

bool FastForwardEngine::needsFineStep(const CellState& cell)
{
    return
        !cell.alive ||
        cell.status == CellStatus::Dividing ||
        cell.division_readiness >= 0.8 ||
        cell.health <= 0.25 ||
        cell.stress >= 0.75 ||
        cell.membrane_integrity <= 0.2;
}

double FastForwardEngine::chooseStepSeconds(const CellState& cell,
                                            const SimulationConfig& cfg,
                                            double remaining_seconds,
                                            const FastForwardOptions& options)
{
    const double tick = std::max(cfg.runtime.tick_seconds, 0.001);

    if(options.precise)
    {
        return std::min(remaining_seconds, tick);
    }

    if(options.max_step_override_seconds > 0.0)
    {
        return std::min(remaining_seconds,
                        std::max(options.max_step_override_seconds, tick));
    }

    if(needsFineStep(cell))
    {
        return std::min(remaining_seconds,
                        std::max(cfg.runtime.offline_chunk_short_seconds, tick));
    }

    if(remaining_seconds <= 3600.0)
    {
        return std::min(remaining_seconds,
                        std::max(cfg.runtime.offline_chunk_short_seconds, tick));
    }

    if(remaining_seconds <= 86400.0)
    {
        return std::min(remaining_seconds,
                        std::max(cfg.runtime.offline_chunk_medium_seconds, tick));
    }

    return std::min(remaining_seconds,
                    std::max(cfg.runtime.offline_chunk_long_seconds, tick));
}

FastForwardSummary FastForwardEngine::run(CellState& cell,
                                          const EnvironmentState& env_input,
                                          const SimulationConfig& cfg_input,
                                          double elapsed_seconds,
                                          const FastForwardOptions& options)
{
    FastForwardSummary summary;
    summary.requested_elapsed_seconds = std::max(elapsed_seconds, 0.0);

    if(summary.requested_elapsed_seconds <= 0.0)
    {
        return summary;
    }

    EnvironmentState env = env_input;
    SimulationConfig cfg = cfg_input;

    env.normalize();
    cfg.normalize();
    cell.normalize();

    double remaining = summary.requested_elapsed_seconds;

    while(remaining > 1e-9)
    {
        if(options.stop_on_death && (!cell.alive || cell.status == CellStatus::Dead))
        {
            summary.terminated_by_death = true;
            break;
        }

        const double outer_step = chooseStepSeconds(cell, cfg, remaining, options);
        if(outer_step <= 0.0)
        {
            break;
        }

        double local_remaining = outer_step;

        const double base_substep = std::max(cfg.runtime.tick_seconds, 0.001);

        while(local_remaining > 1e-9)
        {
            if(options.stop_on_death && (!cell.alive || cell.status == CellStatus::Dead))
            {
                summary.terminated_by_death = true;
                break;
            }

            const double substep = std::min(local_remaining, base_substep);
            CellUpdateResult r = CellUpdater::update(cell, env, cfg, substep);

            summary.step_count++;
            summary.simulated_elapsed_seconds += substep;
            local_remaining -= substep;
            remaining -= substep;

            summary.total_uptake += r.uptake_amount;
            summary.total_storage_release += r.storage_release;
            summary.total_energy_production += r.energy_production;
            summary.total_maintenance_cost += r.maintenance_cost;
            summary.total_energy_deficit += r.energy_deficit;
            summary.total_growth_delta += r.growth_delta;

            if(r.entered_dormancy) summary.entered_dormancy_count++;
            if(r.exited_dormancy) summary.exited_dormancy_count++;
            if(r.became_division_ready) summary.division_ready_count++;
            if(r.became_dead) summary.death_count++;

            if(r.became_dead && options.stop_on_death)
            {
                summary.terminated_by_death = true;
                break;
            }
        }

        if(summary.terminated_by_death)
        {
            break;
        }
    }

    summary.remaining_elapsed_seconds = std::max(remaining, 0.0);
    return summary;
}

} // namespace core
} // namespace acell