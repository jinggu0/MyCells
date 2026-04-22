#pragma once

#include "GrowthRules.h"
#include "StressRules.h"

namespace acell
{
namespace core
{

struct CellUpdateResult
{
    growth::MetabolicSnapshot metabolic;
    stress::StressSnapshot stress_snapshot;

    double uptake_amount = 0.0;
    double storage_release = 0.0;
    double energy_production = 0.0;
    double maintenance_cost = 0.0;
    double energy_deficit = 0.0;
    double growth_delta = 0.0;

    bool entered_dormancy = false;
    bool exited_dormancy = false;
    bool became_division_ready = false;
    bool became_dead = false;
};

class CellUpdater
{
public:
    static CellUpdateResult update(CellState& cell,
                                   const EnvironmentState& env,
                                   const SimulationConfig& cfg,
                                   double dt);

private:
    static void updateDormancy(CellState& cell,
                               const EnvironmentState& env,
                               const SimulationConfig& cfg,
                               double dt,
                               CellUpdateResult& result);
};

} // namespace core
} // namespace acell