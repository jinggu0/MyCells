#include "CellUpdater.h"

#include <algorithm>

namespace acell
{
namespace core
{

void CellUpdater::updateDormancy(CellState& cell,
                                 const EnvironmentState& env,
                                 const SimulationConfig& cfg,
                                 double dt,
                                 CellUpdateResult& result)
{
    const bool want_enter =
        env.nutrient_density <= cfg.dormancy.nutrient_low_threshold &&
        cell.energy <= cfg.dormancy.energy_low_threshold &&
        cell.stress >= cfg.dormancy.stress_high_threshold &&
        cell.alive;

    const bool want_exit =
        env.nutrient_density > cfg.dormancy.nutrient_low_threshold * 1.5 &&
        cell.energy > cfg.dormancy.energy_low_threshold * 1.5 &&
        cell.stress < cfg.dormancy.stress_high_threshold * 0.7 &&
        cell.alive;

    const double before = cell.dormancy_state;

    if(want_enter)
    {
        cell.dormancy_state += cfg.dormancy.dormancy_enter_rate * dt;
    }
    else if(want_exit)
    {
        cell.dormancy_state -= cfg.dormancy.dormancy_exit_rate * dt;
    }

    cell.dormancy_state = clamp01(cell.dormancy_state);

    if(before < 0.5 && cell.dormancy_state >= 0.5)
    {
        result.entered_dormancy = true;
        cell.status = CellStatus::Dormant;
    }

    if(before >= 0.5 && cell.dormancy_state < 0.5)
    {
        result.exited_dormancy = true;
        if(cell.alive)
        {
            cell.status = CellStatus::Alive;
        }
    }
}

CellUpdateResult CellUpdater::update(CellState& cell,
                                     const EnvironmentState& env_input,
                                     const SimulationConfig& cfg_input,
                                     double dt)
{
    CellUpdateResult result;

    EnvironmentState env = env_input;
    SimulationConfig cfg = cfg_input;

    env.normalize();
    cfg.normalize();
    cell.normalize();

    if(!cell.alive || cell.status == CellStatus::Dead)
    {
        cell.markDead();
        result.became_dead = true;
        return result;
    }

    const bool was_division_ready = (cell.division_readiness >= 1.0);

    result.metabolic = growth::buildMetabolicSnapshot(cell, env, cfg);

    result.uptake_amount = growth::applyUptake(cell, env, cfg, dt, result.metabolic);
    result.storage_release = growth::applyStorageRelease(cell, cfg, dt);
    result.energy_production = growth::applyEnergyProduction(cell, cfg, dt, result.metabolic);

    const bool dormant_before_maintenance = cell.isDormant();
    result.maintenance_cost = growth::calcMaintenanceCost(cell, cfg, dt, dormant_before_maintenance);
    result.energy_deficit = growth::applyMaintenance(cell, cfg, result.maintenance_cost);

    stress::applyStress(cell, env, cfg, dt, &result.stress_snapshot);
    stress::applyMembrane(cell, env, cfg, dt, &result.stress_snapshot);
    stress::applyHealth(cell, env, cfg, dt, &result.stress_snapshot);

    updateDormancy(cell, env, cfg, dt, result);

    result.metabolic = growth::buildMetabolicSnapshot(cell, env, cfg);

    result.growth_delta = growth::applyGrowth(cell, cfg, dt, result.metabolic, cell.isDormant());
    growth::rebalanceStorage(cell, cfg, dt);
    growth::updateDivisionReadiness(cell, cfg, dt, result.metabolic);
    growth::updateGrowthPhase(cell);

    cell.cell_cycle_progress = cell.division_readiness;
    cell.age_seconds += std::max(dt, 0.0);

    if(cell.canDivide(cfg.division.division_mass_threshold,
                      cfg.division.division_energy_threshold,
                      cfg.division.division_health_threshold,
                      cfg.division.division_stress_threshold,
                      cfg.division.division_membrane_threshold))
    {
        cell.status = CellStatus::Dividing;
    }

    if(cell.health <= 0.0 || cell.membrane_integrity <= cfg.membrane.membrane_death_threshold)
    {
        cell.markDead();
    }

    cell.normalize();

    result.became_division_ready = (!was_division_ready && cell.division_readiness >= 1.0);
    result.became_dead = (!cell.alive || cell.status == CellStatus::Dead);

    return result;
}

} // namespace core
} // namespace acell