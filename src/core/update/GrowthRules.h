#pragma once

#include "../model/CellState.h"
#include "../model/EnvironmentState.h"
#include "../model/SimulationConfig.h"

namespace acell
{
namespace core
{
namespace growth
{

struct MetabolicSnapshot
{
    double temp_fitness = 1.0;
    double ph_fitness = 1.0;
    double osmo_fitness = 1.0;
    double toxin_fitness = 1.0;
    double oxygen_fitness = 1.0;

    double env_fitness = 1.0;
    double metabolic_efficiency = 1.0;
    double growth_capacity = 1.0;

    double surface_factor = 1.0;
    double diffusion_modifier = 1.0;
};

double gaussianFitness(double value, double optimal, double sigma);

double calcTempFitness(const EnvironmentState& env, const SimulationConfig& cfg);
double calcPhFitness(const EnvironmentState& env, const SimulationConfig& cfg);
double calcOsmoFitness(const EnvironmentState& env, const SimulationConfig& cfg);
double calcToxinFitness(const EnvironmentState& env);
double calcOxygenFitness(const EnvironmentState& env, const SimulationConfig& cfg);

double calcEnvFitness(const EnvironmentState& env, const SimulationConfig& cfg);
double calcMetabolicEfficiency(const CellState& cell, double env_fitness);
double calcSurfaceFactor(const CellState& cell);
double calcDiffusionModifier(const EnvironmentState& env, const SimulationConfig& cfg);

MetabolicSnapshot buildMetabolicSnapshot(const CellState& cell,
                                         const EnvironmentState& env,
                                         const SimulationConfig& cfg);

double calcUptakeRate(const CellState& cell,
                      const EnvironmentState& env,
                      const SimulationConfig& cfg,
                      const MetabolicSnapshot& ms);

double applyUptake(CellState& cell,
                   const EnvironmentState& env,
                   const SimulationConfig& cfg,
                   double dt,
                   const MetabolicSnapshot& ms);

double calcStorageRelease(const CellState& cell,
                          const SimulationConfig& cfg,
                          double dt);

double applyStorageRelease(CellState& cell,
                           const SimulationConfig& cfg,
                           double dt);

double calcEnergyProduction(const CellState& cell,
                            const SimulationConfig& cfg,
                            double dt,
                            const MetabolicSnapshot& ms);

double applyEnergyProduction(CellState& cell,
                             const SimulationConfig& cfg,
                             double dt,
                             const MetabolicSnapshot& ms);

double calcMaintenanceCost(const CellState& cell,
                           const SimulationConfig& cfg,
                           double dt,
                           bool dormant);

double applyMaintenance(CellState& cell,
                        const SimulationConfig& cfg,
                        double maintenance_cost);

double calcGrowthFlux(const CellState& cell,
                      const SimulationConfig& cfg,
                      double dt,
                      const MetabolicSnapshot& ms,
                      bool dormant);

double applyGrowth(CellState& cell,
                   const SimulationConfig& cfg,
                   double dt,
                   const MetabolicSnapshot& ms,
                   bool dormant);

void rebalanceStorage(CellState& cell,
                      const SimulationConfig& cfg,
                      double dt);

void updateVolumeAndSurface(CellState& cell,
                            const SimulationConfig& cfg);

void updateDivisionReadiness(CellState& cell,
                             const SimulationConfig& cfg,
                             double dt,
                             const MetabolicSnapshot& ms);

void updateGrowthPhase(CellState& cell);

} // namespace growth
} // namespace core
} // namespace acell