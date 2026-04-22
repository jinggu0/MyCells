#include "StressRules.h"
#include "GrowthRules.h"

#include <algorithm>
#include <cmath>

namespace acell
{
namespace core
{
namespace stress
{

double normalizedDeviation(double value, double optimal, double sigma)
{
    sigma = std::max(sigma, 1e-9);
    return std::abs(value - optimal) / sigma;
}

double calcTempDeviation(const EnvironmentState& env, const SimulationConfig& cfg)
{
    return normalizedDeviation(env.temperature,
                               cfg.physical.optimal_temperature,
                               cfg.physical.temp_tolerance_sigma);
}

double calcPhDeviation(const EnvironmentState& env, const SimulationConfig& cfg)
{
    return normalizedDeviation(env.ph,
                               cfg.physical.optimal_ph,
                               cfg.physical.ph_tolerance_sigma);
}

double calcOsmoDeviation(const EnvironmentState& env, const SimulationConfig& cfg)
{
    return normalizedDeviation(env.osmolarity,
                               cfg.physical.optimal_osmolarity,
                               cfg.physical.osmo_tolerance_sigma);
}

double calcStarvationFactor(const CellState& cell, const SimulationConfig& cfg)
{
    const double ref = std::max(cfg.metabolism.energy_target, 1e-9);
    return clamp01((ref - cell.energy) / ref);
}

double calcAgeFactor(const CellState& cell)
{
    const double ref = 86400.0 * 14.0;
    return clamp01(cell.age_seconds / ref);
}

StressSnapshot buildStressSnapshot(const CellState& cell,
                                   const EnvironmentState& env,
                                   const SimulationConfig& cfg)
{
    StressSnapshot ss;

    ss.temp_deviation = calcTempDeviation(env, cfg);
    ss.ph_deviation = calcPhDeviation(env, cfg);
    ss.osmo_deviation = calcOsmoDeviation(env, cfg);
    ss.starvation_factor = calcStarvationFactor(cell, cfg);

    ss.stress_up =
        cfg.stress.k_temp_stress * ss.temp_deviation +
        cfg.stress.k_ph_stress * ss.ph_deviation +
        cfg.stress.k_osmo_stress * ss.osmo_deviation +
        cfg.stress.k_toxin_stress * env.toxin_level +
        cfg.stress.k_starvation_stress * ss.starvation_factor +
        cfg.stress.k_membrane_stress * (1.0 - cell.membrane_integrity);

    const double env_fitness = growth::calcEnvFitness(env, cfg);
    const double recovery_factor =
        cell.health *
        env_fitness *
        clamp01(cell.energy / std::max(cfg.stress.energy_recovery_ref, 1e-9));

    ss.stress_down = cfg.stress.k_recovery * recovery_factor;

    ss.health_down =
        cfg.health.k_stress_damage * cell.stress +
        cfg.health.k_toxin_damage * env.toxin_level +
        cfg.health.k_age_damage * calcAgeFactor(cell);

    const double repair_factor =
        env_fitness *
        clamp01(cell.energy / std::max(cfg.metabolism.repair_energy_ref, 1e-9)) *
        cell.membrane_integrity;

    ss.health_up = cfg.health.k_repair * repair_factor;

    ss.membrane_down =
        cfg.membrane.k_mem_toxin * env.toxin_level +
        cfg.membrane.k_mem_osmo * ss.osmo_deviation +
        cfg.membrane.k_mem_age * calcAgeFactor(cell);

    ss.membrane_up =
        cfg.membrane.k_mem_repair *
        env_fitness *
        clamp01(cell.energy / std::max(cfg.metabolism.membrane_repair_ref, 1e-9));

    return ss;
}

void applyStress(CellState& cell,
                 const EnvironmentState& env,
                 const SimulationConfig& cfg,
                 double dt,
                 StressSnapshot* out_snapshot)
{
    StressSnapshot ss = buildStressSnapshot(cell, env, cfg);

    cell.stress += (ss.stress_up - ss.stress_down) * dt;
    cell.stress = clamp01(cell.stress);

    if(out_snapshot)
    {
        *out_snapshot = ss;
    }
}

void applyHealth(CellState& cell,
                 const EnvironmentState& env,
                 const SimulationConfig& cfg,
                 double dt,
                 StressSnapshot* out_snapshot)
{
    StressSnapshot ss = buildStressSnapshot(cell, env, cfg);

    cell.health += (ss.health_up - ss.health_down) * dt;
    cell.health = clamp01(cell.health);

    if(out_snapshot)
    {
        *out_snapshot = ss;
    }
}

void applyMembrane(CellState& cell,
                   const EnvironmentState& env,
                   const SimulationConfig& cfg,
                   double dt,
                   StressSnapshot* out_snapshot)
{
    StressSnapshot ss = buildStressSnapshot(cell, env, cfg);

    cell.membrane_integrity += (ss.membrane_up - ss.membrane_down) * dt;
    cell.membrane_integrity = clamp01(cell.membrane_integrity);

    if(out_snapshot)
    {
        *out_snapshot = ss;
    }
}

} // namespace stress
} // namespace core
} // namespace acell