#include "GrowthRules.h"

#include <algorithm>
#include <cmath>

namespace acell
{
namespace core
{
namespace growth
{

namespace
{

double sphereSurfaceAreaFromVolume(double volume)
{
    if(volume <= 0.0)
    {
        return 0.0;
    }

    const double pi = std::acos(-1.0);
    const double k = std::pow(36.0 * pi, 1.0 / 3.0);
    return k * std::pow(volume, 2.0 / 3.0);
}

} // namespace

double gaussianFitness(double value, double optimal, double sigma)
{
    sigma = std::max(sigma, 1e-9);
    const double z = (value - optimal) / sigma;
    return std::exp(-0.5 * z * z);
}

double calcTempFitness(const EnvironmentState& env, const SimulationConfig& cfg)
{
    return gaussianFitness(env.temperature,
                           cfg.physical.optimal_temperature,
                           cfg.physical.temp_tolerance_sigma);
}

double calcPhFitness(const EnvironmentState& env, const SimulationConfig& cfg)
{
    return gaussianFitness(env.ph,
                           cfg.physical.optimal_ph,
                           cfg.physical.ph_tolerance_sigma);
}

double calcOsmoFitness(const EnvironmentState& env, const SimulationConfig& cfg)
{
    return gaussianFitness(env.osmolarity,
                           cfg.physical.optimal_osmolarity,
                           cfg.physical.osmo_tolerance_sigma);
}

double calcToxinFitness(const EnvironmentState& env)
{
    return clamp01(1.0 - env.toxin_level);
}

double calcOxygenFitness(const EnvironmentState& env, const SimulationConfig& cfg)
{
    const double ref = std::max(cfg.physical.optimal_oxygen, 1e-9);
    return clamp01(env.oxygen_level / ref);
}

double calcEnvFitness(const EnvironmentState& env, const SimulationConfig& cfg)
{
    const double t = calcTempFitness(env, cfg);
    const double p = calcPhFitness(env, cfg);
    const double o = calcOsmoFitness(env, cfg);
    const double x = calcToxinFitness(env);
    const double g = calcOxygenFitness(env, cfg);

    return clamp01(t * p * o * x * g);
}

double calcMetabolicEfficiency(const CellState& cell, double env_fitness)
{
    return clamp01(env_fitness * cell.health * cell.membrane_integrity * (1.0 - cell.stress));
}

double calcSurfaceFactor(const CellState& cell)
{
    return std::max(std::pow(std::max(cell.volume, 0.0), 2.0 / 3.0), 0.0);
}

double calcDiffusionModifier(const EnvironmentState& env, const SimulationConfig& cfg)
{
    const double ref = std::max(cfg.uptake.diffusion_reference, 1e-9);
    return clamp01(env.diffusion_rate / ref);
}

MetabolicSnapshot buildMetabolicSnapshot(const CellState& cell,
                                         const EnvironmentState& env,
                                         const SimulationConfig& cfg)
{
    MetabolicSnapshot ms;
    ms.temp_fitness = calcTempFitness(env, cfg);
    ms.ph_fitness = calcPhFitness(env, cfg);
    ms.osmo_fitness = calcOsmoFitness(env, cfg);
    ms.toxin_fitness = calcToxinFitness(env);
    ms.oxygen_fitness = calcOxygenFitness(env, cfg);

    ms.env_fitness = clamp01(ms.temp_fitness
                           * ms.ph_fitness
                           * ms.osmo_fitness
                           * ms.toxin_fitness
                           * ms.oxygen_fitness);

    ms.metabolic_efficiency = calcMetabolicEfficiency(cell, ms.env_fitness);
    ms.growth_capacity = ms.metabolic_efficiency;
    ms.surface_factor = calcSurfaceFactor(cell);
    ms.diffusion_modifier = calcDiffusionModifier(env, cfg);
    return ms;
}

double calcUptakeRate(const CellState& cell,
                      const EnvironmentState& env,
                      const SimulationConfig& cfg,
                      const MetabolicSnapshot& ms)
{
    return cfg.uptake.k_uptake
         * env.nutrient_density
         * cell.membrane_integrity
         * ms.surface_factor
         * ms.metabolic_efficiency
         * ms.diffusion_modifier;
}

double applyUptake(CellState& cell,
                   const EnvironmentState& env,
                   const SimulationConfig& cfg,
                   double dt,
                   const MetabolicSnapshot& ms)
{
    const double uptake = std::max(calcUptakeRate(cell, env, cfg, ms) * dt, 0.0);
    cell.internal_nutrient += uptake;
    return uptake;
}

double calcStorageRelease(const CellState& cell,
                          const SimulationConfig& cfg,
                          double dt)
{
    const double target = std::max(cfg.metabolism.energy_target, 1e-9);
    const double need_factor = clamp01((target - cell.energy) / target);
    const double releasable = cfg.storage.k_storage_release * need_factor * dt;
    return std::min(cell.stored_nutrient, std::max(releasable, 0.0));
}

double applyStorageRelease(CellState& cell,
                           const SimulationConfig& cfg,
                           double dt)
{
    const double released = calcStorageRelease(cell, cfg, dt);
    cell.stored_nutrient -= released;
    cell.internal_nutrient += released;
    return released;
}

double calcEnergyProduction(const CellState& cell,
                            const SimulationConfig& cfg,
                            double dt,
                            const MetabolicSnapshot& ms)
{
    const double cap = cfg.metabolism.k_catabolism * ms.metabolic_efficiency * dt;
    return std::min(cell.internal_nutrient, std::max(cap, 0.0));
}

double applyEnergyProduction(CellState& cell,
                             const SimulationConfig& cfg,
                             double dt,
                             const MetabolicSnapshot& ms)
{
    const double production = calcEnergyProduction(cell, cfg, dt, ms);
    cell.internal_nutrient -= production;
    cell.energy += production * cfg.metabolism.yield_energy;
    return production;
}

double calcMaintenanceCost(const CellState& cell,
                           const SimulationConfig& cfg,
                           double dt,
                           bool dormant)
{
    double maintenance = (cfg.maintenance.k_base
                        + cfg.maintenance.k_mass * cell.mass
                        + cfg.maintenance.k_stress * cell.stress) * dt;

    if(dormant)
    {
        maintenance *= cfg.dormancy.dormancy_maintenance_scale;
    }

    return std::max(maintenance, 0.0);
}

double applyMaintenance(CellState& cell,
                        const SimulationConfig& cfg,
                        double maintenance_cost)
{
    cell.energy -= maintenance_cost;

    if(cell.energy >= 0.0)
    {
        return 0.0;
    }

    const double deficit = -cell.energy;
    cell.energy = 0.0;
    cell.health -= cfg.maintenance.k_energy_deficit_damage * deficit;
    cell.stress += cfg.maintenance.k_energy_deficit_stress * deficit;
    return deficit;
}

double calcGrowthFlux(const CellState& cell,
                      const SimulationConfig& cfg,
                      double dt,
                      const MetabolicSnapshot& ms,
                      bool dormant)
{
    if(!cell.canGrow(cfg.growth.growth_energy_threshold,
                     cfg.growth.health_min_for_growth,
                     cfg.growth.stress_max_for_growth))
    {
        return 0.0;
    }

    const double surplus_energy = std::max(0.0, cell.energy - cfg.growth.growth_energy_threshold);
    double growth_inhibition = 1.0 - clamp01(cell.stress);
    double flux = cfg.growth.k_growth * surplus_energy * ms.metabolic_efficiency * growth_inhibition;

    if(dormant)
    {
        flux *= cfg.dormancy.dormancy_growth_scale;
    }

    return std::max(flux * dt, 0.0);
}

double applyGrowth(CellState& cell,
                   const SimulationConfig& cfg,
                   double dt,
                   const MetabolicSnapshot& ms,
                   bool dormant)
{
    const double dm = calcGrowthFlux(cell, cfg, dt, ms, dormant);
    const double energy_cost = dm * cfg.growth.growth_energy_cost;

    if(energy_cost > cell.energy)
    {
        const double safe_dm = cell.energy / std::max(cfg.growth.growth_energy_cost, 1e-9);
        cell.mass += safe_dm;
        cell.energy = 0.0;
        updateVolumeAndSurface(cell, cfg);
        return safe_dm;
    }

    cell.mass += dm;
    cell.energy -= energy_cost;
    updateVolumeAndSurface(cell, cfg);
    return dm;
}

void rebalanceStorage(CellState& cell,
                      const SimulationConfig& cfg,
                      double dt)
{
    if(cell.internal_nutrient <= cfg.uptake.nutrient_soft_cap)
    {
        return;
    }

    const double extra = cell.internal_nutrient - cfg.uptake.nutrient_soft_cap;
    const double amount = std::min(extra, cfg.storage.k_storage_fill * dt);

    cell.internal_nutrient -= amount;
    cell.stored_nutrient += amount;
}

void updateVolumeAndSurface(CellState& cell,
                            const SimulationConfig& cfg)
{
    const double density = std::max(cfg.physical.density_cell, 1e-9);
    cell.volume = cell.mass / density;
    cell.surface_area = sphereSurfaceAreaFromVolume(cell.volume);
}

void updateDivisionReadiness(CellState& cell,
                             const SimulationConfig& cfg,
                             double dt,
                             const MetabolicSnapshot& ms)
{
    const bool good =
        cell.mass >= cfg.division.division_mass_threshold &&
        cell.energy >= cfg.division.division_energy_threshold &&
        cell.health >= cfg.division.division_health_threshold &&
        cell.stress <= cfg.division.division_stress_threshold &&
        cell.membrane_integrity >= cfg.division.division_membrane_threshold &&
        cell.alive;

    if(good)
    {
        const double prep_factor =
            clamp01((cell.mass / std::max(cfg.division.division_mass_threshold, 1e-9)) - 1.0) *
            ms.env_fitness *
            cell.health *
            (1.0 - cell.stress);

        cell.division_readiness += cfg.division.k_division_prep * prep_factor * dt;
    }
    else
    {
        double bad = 0.0;

        if(cell.mass < cfg.division.division_mass_threshold) bad += 0.2;
        if(cell.energy < cfg.division.division_energy_threshold) bad += 0.2;
        if(cell.health < cfg.division.division_health_threshold) bad += 0.2;
        if(cell.stress > cfg.division.division_stress_threshold) bad += 0.2;
        if(cell.membrane_integrity < cfg.division.division_membrane_threshold) bad += 0.2;

        cell.division_readiness -= cfg.division.k_division_decay * bad * dt;
    }

    cell.division_readiness = clamp01(cell.division_readiness);
}

void updateGrowthPhase(CellState& cell)
{
    if(!cell.alive || cell.status == CellStatus::Dead)
    {
        cell.growth_phase = GrowthPhase::Dying;
        cell.status = CellStatus::Dead;
        return;
    }

    if(cell.isDormant())
    {
        cell.growth_phase = GrowthPhase::Dormant;
        cell.status = CellStatus::Dormant;
        return;
    }

    if(cell.division_readiness >= 1.0)
    {
        cell.status = CellStatus::Dividing;
    }
    else
    {
        cell.status = CellStatus::Alive;
    }

    if(cell.health <= 0.15 || cell.membrane_integrity <= 0.1)
    {
        cell.growth_phase = GrowthPhase::Dying;
        return;
    }

    if(cell.stress >= 0.6)
    {
        cell.growth_phase = GrowthPhase::Stressed;
        return;
    }

    if(cell.mass <= 1.05)
    {
        cell.growth_phase = GrowthPhase::Lag;
        return;
    }

    cell.growth_phase = GrowthPhase::Active;
}

} // namespace growth
} // namespace core
} // namespace acell