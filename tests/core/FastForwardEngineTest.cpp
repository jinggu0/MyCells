#include "CatchCompat.h"

#include <cmath>

#include "../../src/core/model/CellState.h"
#include "../../src/core/model/EnvironmentState.h"
#include "../../src/core/model/SimulationConfig.h"
#include "../../src/core/update/CellUpdater.h"
#include "../../src/core/time/FastForwardEngine.h"

namespace acell
{
namespace core
{
namespace
{

CellState makeDefaultCell()
{
    CellState cell;
    cell.mass = 1.0;
    cell.volume = 1.0;
    cell.surface_area = 1.0;
    cell.internal_nutrient = 0.5;
    cell.stored_nutrient = 0.5;
    cell.energy = 0.5;
    cell.health = 1.0;
    cell.stress = 0.0;
    cell.membrane_integrity = 1.0;
    cell.alive = true;
    cell.normalize();
    return cell;
}

EnvironmentState makeGoodEnvironment()
{
    EnvironmentState env;
    env.nutrient_density = 2.0;
    env.oxygen_level = 1.0;
    env.temperature = 37.0;
    env.ph = 7.2;
    env.osmolarity = 1.0;
    env.toxin_level = 0.0;
    env.diffusion_rate = 1.0;
    env.normalize();
    return env;
}

} // namespace
} // namespace core
} // namespace acell

using namespace acell::core;

TEST_CASE("Precise fast-forward matches repeated tick updates")
{
    CellState manual_cell = makeDefaultCell();
    CellState ff_cell = manual_cell;

    EnvironmentState env = makeGoodEnvironment();
    SimulationConfig cfg;

    const double elapsed = 300.0;

    for(int i=0; i<300; i++)
    {
        CellUpdater::update(manual_cell, env, cfg, cfg.runtime.tick_seconds);
    }

    FastForwardOptions opt;
    opt.precise = true;

    FastForwardSummary summary = FastForwardEngine::run(ff_cell, env, cfg, elapsed, opt);

    REQUIRE(summary.simulated_elapsed_seconds == 300.0);
    REQUIRE(std::abs(ff_cell.mass - manual_cell.mass) < 1e-9);
    REQUIRE(std::abs(ff_cell.energy - manual_cell.energy) < 1e-9);
    REQUIRE(std::abs(ff_cell.health - manual_cell.health) < 1e-9);
    REQUIRE(std::abs(ff_cell.stress - manual_cell.stress) < 1e-9);
    REQUIRE(std::abs(ff_cell.membrane_integrity - manual_cell.membrane_integrity) < 1e-9);
    REQUIRE(std::abs(ff_cell.division_readiness - manual_cell.division_readiness) < 1e-9);
}

TEST_CASE("Adaptive fast-forward advances the cell state")
{
    CellState cell = makeDefaultCell();
    EnvironmentState env = makeGoodEnvironment();
    SimulationConfig cfg;

    const double initial_mass = cell.mass;

    FastForwardSummary summary = FastForwardEngine::run(cell, env, cfg, 7200.0);

    REQUIRE(summary.simulated_elapsed_seconds > 0.0);
    REQUIRE(summary.step_count > 0);
    REQUIRE(cell.mass > initial_mass);
    REQUIRE(cell.alive);
}

TEST_CASE("Fast-forward can terminate early on death")
{
    CellState cell = makeDefaultCell();
    EnvironmentState env = makeGoodEnvironment();
    SimulationConfig cfg;

    cell.internal_nutrient = 0.0;
    cell.stored_nutrient = 0.0;
    cell.energy = 0.0;
    cell.health = 0.1;
    cell.membrane_integrity = 0.1;

    env.nutrient_density = 0.0;
    env.toxin_level = 1.0;
    env.temperature = 50.0;
    env.ph = 4.5;
    env.osmolarity = 1.8;

    FastForwardSummary summary = FastForwardEngine::run(cell, env, cfg, 86400.0);

    REQUIRE(summary.terminated_by_death);
    REQUIRE(summary.death_count >= 1);
    REQUIRE_FALSE(cell.alive);
    REQUIRE(summary.simulated_elapsed_seconds <= 86400.0);
}

TEST_CASE("Fast-forward summary aggregates major quantities")
{
    CellState cell = makeDefaultCell();
    EnvironmentState env = makeGoodEnvironment();
    SimulationConfig cfg;

    FastForwardOptions opt;
    opt.precise = false;
    opt.max_step_override_seconds = 60.0;

    FastForwardSummary summary = FastForwardEngine::run(cell, env, cfg, 3600.0, opt);

    REQUIRE(summary.total_uptake >= 0.0);
    REQUIRE(summary.total_energy_production >= 0.0);
    REQUIRE(summary.total_maintenance_cost >= 0.0);
    REQUIRE(summary.total_growth_delta >= 0.0);
    REQUIRE(summary.remaining_elapsed_seconds >= 0.0);
}
