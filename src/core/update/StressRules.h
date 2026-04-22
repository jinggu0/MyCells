#pragma once

#include "../model/CellState.h"
#include "../model/EnvironmentState.h"
#include "../model/SimulationConfig.h"

namespace acell
{
namespace core
{
namespace stress
{

struct StressSnapshot
{
    double temp_deviation = 0.0;
    double ph_deviation = 0.0;
    double osmo_deviation = 0.0;
    double starvation_factor = 0.0;

    double stress_up = 0.0;
    double stress_down = 0.0;

    double health_down = 0.0;
    double health_up = 0.0;

    double membrane_down = 0.0;
    double membrane_up = 0.0;
};

double normalizedDeviation(double value, double optimal, double sigma);

double calcTempDeviation(const EnvironmentState& env, const SimulationConfig& cfg);
double calcPhDeviation(const EnvironmentState& env, const SimulationConfig& cfg);
double calcOsmoDeviation(const EnvironmentState& env, const SimulationConfig& cfg);
double calcStarvationFactor(const CellState& cell, const SimulationConfig& cfg);
double calcAgeFactor(const CellState& cell);

StressSnapshot buildStressSnapshot(const CellState& cell,
                                   const EnvironmentState& env,
                                   const SimulationConfig& cfg);

void applyStress(CellState& cell,
                 const EnvironmentState& env,
                 const SimulationConfig& cfg,
                 double dt,
                 StressSnapshot* out_snapshot = nullptr);

void applyHealth(CellState& cell,
                 const EnvironmentState& env,
                 const SimulationConfig& cfg,
                 double dt,
                 StressSnapshot* out_snapshot = nullptr);

void applyMembrane(CellState& cell,
                   const EnvironmentState& env,
                   const SimulationConfig& cfg,
                   double dt,
                   StressSnapshot* out_snapshot = nullptr);

} // namespace stress
} // namespace core
} // namespace acell