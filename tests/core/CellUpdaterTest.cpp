#include "CatchCompat.h"

#include "../../src/core/model/CellState.h"
#include "../../src/core/model/EnvironmentState.h"
#include "../../src/core/model/SimulationConfig.h"
#include "../../src/core/update/CellUpdater.h"

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

TEST_CASE("Cell grows under nutrient-rich stable conditions")
{
    CellState cell = makeDefaultCell();
    EnvironmentState env = makeGoodEnvironment();
    SimulationConfig cfg;

    const double initial_mass = cell.mass;
    const double initial_energy = cell.energy;

    for(int i=0; i<600; i++)
    {
        CellUpdater::update(cell, env, cfg, cfg.runtime.tick_seconds);
        if(!cell.alive)
        {
            break;
        }
    }

    REQUIRE(cell.alive);
    REQUIRE(cell.mass > initial_mass);
    REQUIRE(cell.energy >= 0.0);
    REQUIRE(cell.health > 0.0);
    REQUIRE(cell.stress < 1.0);
    REQUIRE(cell.mass > initial_mass + 0.01);
    REQUIRE(cell.energy != initial_energy);
}

TEST_CASE("Harsh starvation and toxicity damage the cell")
{
    CellState cell = makeDefaultCell();
    EnvironmentState env = makeGoodEnvironment();
    SimulationConfig cfg;

    cell.internal_nutrient = 0.0;
    cell.stored_nutrient = 0.0;
    cell.energy = 0.02;
    cell.health = 0.8;

    env.nutrient_density = 0.0;
    env.oxygen_level = 0.1;
    env.toxin_level = 1.0;
    env.temperature = 45.0;
    env.ph = 5.2;
    env.osmolarity = 1.5;

    const double initial_health = cell.health;

    for(int i=0; i<400; i++)
    {
        CellUpdater::update(cell, env, cfg, cfg.runtime.tick_seconds);
        if(!cell.alive)
        {
            break;
        }
    }

    REQUIRE(cell.health < initial_health);
    REQUIRE(cell.stress > 0.0);
    REQUIRE(cell.membrane_integrity < 1.0);
}

TEST_CASE("Good conditions increase division readiness over time")
{
    CellState cell = makeDefaultCell();
    EnvironmentState env = makeGoodEnvironment();
    SimulationConfig cfg;

    cell.mass = 2.2;
    cell.volume = 2.2;
    cell.surface_area = 1.7;
    cell.energy = 1.2;
    cell.health = 1.0;
    cell.stress = 0.0;
    cell.membrane_integrity = 1.0;
    cell.division_readiness = 0.0;

    const double initial_readiness = cell.division_readiness;

    for(int i=0; i<1200; i++)
    {
        CellUpdater::update(cell, env, cfg, cfg.runtime.tick_seconds);
        if(!cell.alive)
        {
            break;
        }
    }

    REQUIRE(cell.alive);
    REQUIRE(cell.division_readiness >= initial_readiness);
    REQUIRE(cell.division_readiness > 0.0);
}

TEST_CASE("Severe long-term deprivation can kill the cell")
{
    CellState cell = makeDefaultCell();
    EnvironmentState env = makeGoodEnvironment();
    SimulationConfig cfg;

    cell.internal_nutrient = 0.0;
    cell.stored_nutrient = 0.0;
    cell.energy = 0.0;
    cell.health = 0.2;
    cell.membrane_integrity = 0.2;

    env.nutrient_density = 0.0;
    env.toxin_level = 1.0;
    env.temperature = 48.0;
    env.ph = 4.8;
    env.osmolarity = 1.7;

    for(int i=0; i<5000; i++)
    {
        CellUpdater::update(cell, env, cfg, cfg.runtime.tick_seconds);
        if(!cell.alive)
        {
            break;
        }
    }

    REQUIRE_FALSE(cell.alive);
    REQUIRE(cell.status == CellStatus::Dead);
}
