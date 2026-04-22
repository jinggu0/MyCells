#pragma once

#include <string>
#include <cstdint>
#include "CoreTypes.h"

namespace acell
{
namespace core
{

struct CellState
{
    std::int64_t id = 0;
    std::string uuid;
    std::string name = "MSC-1";
    ModelType model_type = ModelType::MSC1;

    CellStatus status = CellStatus::Alive;
    GrowthPhase growth_phase = GrowthPhase::Lag;

    double mass = 1.0;
    double volume = 1.0;
    double surface_area = 1.0;
    double shape_index = 0.0;

    double internal_nutrient = 0.5;
    double stored_nutrient = 0.5;
    double energy = 0.5;

    double health = 1.0;
    double stress = 0.0;
    double membrane_integrity = 1.0;

    double age_seconds = 0.0;
    double division_readiness = 0.0;
    double cell_cycle_progress = 0.0;
    double dormancy_state = 0.0;

    std::int64_t generation = 0;
    bool alive = true;

    void normalize()
    {
        mass = clampMin(mass, 0.0);
        volume = clampMin(volume, 0.0);
        surface_area = clampMin(surface_area, 0.0);
        shape_index = clamp01(shape_index);

        internal_nutrient = clampMin(internal_nutrient, 0.0);
        stored_nutrient = clampMin(stored_nutrient, 0.0);
        energy = clampMin(energy, 0.0);

        health = clamp01(health);
        stress = clamp01(stress);
        membrane_integrity = clamp01(membrane_integrity);

        age_seconds = clampMin(age_seconds, 0.0);
        division_readiness = clamp01(division_readiness);
        cell_cycle_progress = clamp01(cell_cycle_progress);
        dormancy_state = clamp01(dormancy_state);

        if(!alive || health <= 0.0 || membrane_integrity <= 0.0)
        {
            alive = false;
            status = CellStatus::Dead;
        }
    }

    bool canGrow(double growth_energy_threshold,
        double health_min_for_growth,
        double stress_max_for_growth) const
    {
        return alive
            && status != CellStatus::Dead
            && status != CellStatus::Dividing
            && division_readiness < 1.0
            && energy > growth_energy_threshold
            && health > health_min_for_growth
            && stress < stress_max_for_growth
            && dormancy_state < 0.8;
    }

    bool canRepair(double repair_energy_ref) const
    {
        return alive
            && status != CellStatus::Dead
            && energy > repair_energy_ref
            && health > 0.0;
    }

    bool isDormant() const
    {
        return dormancy_state >= 0.5 || status == CellStatus::Dormant;
    }

    bool canDivide(double division_mass_threshold,
                   double division_energy_threshold,
                   double division_health_threshold,
                   double division_stress_threshold,
                   double division_membrane_threshold) const
    {
        return alive
            && status != CellStatus::Dead
            && mass >= division_mass_threshold
            && energy >= division_energy_threshold
            && health >= division_health_threshold
            && stress <= division_stress_threshold
            && membrane_integrity >= division_membrane_threshold
            && division_readiness >= 1.0;
    }

    void markDead()
    {
        alive = false;
        status = CellStatus::Dead;
        growth_phase = GrowthPhase::Dying;
        energy = 0.0;
        division_readiness = 0.0;
    }
};

} // namespace core
} // namespace acell