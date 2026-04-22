#pragma once

#include <algorithm>
#include "CoreTypes.h"

namespace acell
{
namespace core
{

struct SimulationConfig
{
    struct Runtime
    {
        double tick_seconds = 1.0;
        double ui_frame_seconds = 1.0 / 30.0;
        double snapshot_interval_seconds = 10.0;
        double series_sample_interval_seconds = 30.0;

        double offline_chunk_short_seconds = 60.0;
        double offline_chunk_medium_seconds = 300.0;
        double offline_chunk_long_seconds = 1800.0;
    } runtime;

    struct CellPhysical
    {
        double density_cell = 1.0;
        double optimal_temperature = 37.0;
        double optimal_ph = 7.2;
        double optimal_osmolarity = 1.0;
        double optimal_oxygen = 1.0;

        double temp_tolerance_sigma = 6.0;
        double ph_tolerance_sigma = 0.7;
        double osmo_tolerance_sigma = 0.25;
    } physical;

    struct Uptake
    {
        double k_uptake = 0.08;
        double diffusion_reference = 1.0;
        double nutrient_soft_cap = 2.0;
    } uptake;

    struct Metabolism
    {
        double k_catabolism = 0.12;
        double yield_energy = 1.0;
        double energy_target = 1.0;
        double repair_energy_ref = 0.3;
        double membrane_repair_ref = 0.25;
    } metabolism;

    struct Storage
    {
        double k_storage_release = 0.08;
        double k_storage_fill = 0.05;
    } storage;

    struct Maintenance
    {
        double k_base = 0.01;
        double k_mass = 0.005;
        double k_stress = 0.01;
        double k_energy_deficit_damage = 0.05;
        double k_energy_deficit_stress = 0.08;
    } maintenance;

    struct Growth
    {
        double growth_energy_threshold = 0.25;
        double health_min_for_growth = 0.3;
        double stress_max_for_growth = 0.7;

        double k_growth = 0.03;
        double growth_energy_cost = 0.5;
    } growth;

    struct Stress
    {
        double k_temp_stress = 0.03;
        double k_ph_stress = 0.03;
        double k_osmo_stress = 0.04;
        double k_toxin_stress = 0.05;
        double k_starvation_stress = 0.07;
        double k_membrane_stress = 0.04;

        double k_recovery = 0.02;
        double energy_recovery_ref = 0.4;
    } stress;

    struct Health
    {
        double k_stress_damage = 0.015;
        double k_toxin_damage = 0.02;
        double k_age_damage = 0.000001;

        double k_repair = 0.01;
    } health;

    struct Membrane
    {
        double k_mem_toxin = 0.02;
        double k_mem_osmo = 0.02;
        double k_mem_age = 0.000001;
        double k_mem_repair = 0.015;

        double membrane_death_threshold = 0.05;
    } membrane;

    struct Division
    {
        double division_mass_threshold = 2.0;
        double division_energy_threshold = 0.6;
        double division_health_threshold = 0.7;
        double division_stress_threshold = 0.35;
        double division_membrane_threshold = 0.75;

        double k_division_prep = 0.02;
        double k_division_decay = 0.03;
    } division;

    struct Dormancy
    {
        double nutrient_low_threshold = 0.15;
        double energy_low_threshold = 0.12;
        double stress_high_threshold = 0.75;

        double dormancy_enter_rate = 0.05;
        double dormancy_exit_rate = 0.03;
        double dormancy_maintenance_scale = 0.3;
        double dormancy_growth_scale = 0.0;
    } dormancy;

    void normalize()
    {
        runtime.tick_seconds = std::max(runtime.tick_seconds, 0.001);
        runtime.ui_frame_seconds = std::max(runtime.ui_frame_seconds, 0.001);
        runtime.snapshot_interval_seconds = std::max(runtime.snapshot_interval_seconds, runtime.tick_seconds);
        runtime.series_sample_interval_seconds = std::max(runtime.series_sample_interval_seconds, runtime.tick_seconds);

        runtime.offline_chunk_short_seconds = std::max(runtime.offline_chunk_short_seconds, runtime.tick_seconds);
        runtime.offline_chunk_medium_seconds = std::max(runtime.offline_chunk_medium_seconds, runtime.tick_seconds);
        runtime.offline_chunk_long_seconds = std::max(runtime.offline_chunk_long_seconds, runtime.tick_seconds);

        uptake.k_uptake = std::max(uptake.k_uptake, 0.0);
        uptake.diffusion_reference = std::max(uptake.diffusion_reference, 0.000001);
        uptake.nutrient_soft_cap = std::max(uptake.nutrient_soft_cap, 0.0);

        metabolism.k_catabolism = std::max(metabolism.k_catabolism, 0.0);
        metabolism.yield_energy = std::max(metabolism.yield_energy, 0.0);
        metabolism.energy_target = std::max(metabolism.energy_target, 0.000001);
        metabolism.repair_energy_ref = std::max(metabolism.repair_energy_ref, 0.000001);
        metabolism.membrane_repair_ref = std::max(metabolism.membrane_repair_ref, 0.000001);

        storage.k_storage_release = std::max(storage.k_storage_release, 0.0);
        storage.k_storage_fill = std::max(storage.k_storage_fill, 0.0);

        maintenance.k_base = std::max(maintenance.k_base, 0.0);
        maintenance.k_mass = std::max(maintenance.k_mass, 0.0);
        maintenance.k_stress = std::max(maintenance.k_stress, 0.0);
        maintenance.k_energy_deficit_damage = std::max(maintenance.k_energy_deficit_damage, 0.0);
        maintenance.k_energy_deficit_stress = std::max(maintenance.k_energy_deficit_stress, 0.0);

        growth.growth_energy_threshold = std::max(growth.growth_energy_threshold, 0.0);
        growth.health_min_for_growth = clamp01(growth.health_min_for_growth);
        growth.stress_max_for_growth = clamp01(growth.stress_max_for_growth);
        growth.k_growth = std::max(growth.k_growth, 0.0);
        growth.growth_energy_cost = std::max(growth.growth_energy_cost, 0.0);

        stress.k_temp_stress = std::max(stress.k_temp_stress, 0.0);
        stress.k_ph_stress = std::max(stress.k_ph_stress, 0.0);
        stress.k_osmo_stress = std::max(stress.k_osmo_stress, 0.0);
        stress.k_toxin_stress = std::max(stress.k_toxin_stress, 0.0);
        stress.k_starvation_stress = std::max(stress.k_starvation_stress, 0.0);
        stress.k_membrane_stress = std::max(stress.k_membrane_stress, 0.0);
        stress.k_recovery = std::max(stress.k_recovery, 0.0);
        stress.energy_recovery_ref = std::max(stress.energy_recovery_ref, 0.000001);

        health.k_stress_damage = std::max(health.k_stress_damage, 0.0);
        health.k_toxin_damage = std::max(health.k_toxin_damage, 0.0);
        health.k_age_damage = std::max(health.k_age_damage, 0.0);
        health.k_repair = std::max(health.k_repair, 0.0);

        membrane.k_mem_toxin = std::max(membrane.k_mem_toxin, 0.0);
        membrane.k_mem_osmo = std::max(membrane.k_mem_osmo, 0.0);
        membrane.k_mem_age = std::max(membrane.k_mem_age, 0.0);
        membrane.k_mem_repair = std::max(membrane.k_mem_repair, 0.0);
        membrane.membrane_death_threshold = clamp01(membrane.membrane_death_threshold);

        division.division_mass_threshold = std::max(division.division_mass_threshold, 0.0);
        division.division_energy_threshold = std::max(division.division_energy_threshold, 0.0);
        division.division_health_threshold = clamp01(division.division_health_threshold);
        division.division_stress_threshold = clamp01(division.division_stress_threshold);
        division.division_membrane_threshold = clamp01(division.division_membrane_threshold);
        division.k_division_prep = std::max(division.k_division_prep, 0.0);
        division.k_division_decay = std::max(division.k_division_decay, 0.0);

        dormancy.nutrient_low_threshold = std::max(dormancy.nutrient_low_threshold, 0.0);
        dormancy.energy_low_threshold = std::max(dormancy.energy_low_threshold, 0.0);
        dormancy.stress_high_threshold = clamp01(dormancy.stress_high_threshold);
        dormancy.dormancy_enter_rate = std::max(dormancy.dormancy_enter_rate, 0.0);
        dormancy.dormancy_exit_rate = std::max(dormancy.dormancy_exit_rate, 0.0);
        dormancy.dormancy_maintenance_scale = std::max(dormancy.dormancy_maintenance_scale, 0.0);
        dormancy.dormancy_growth_scale = std::max(dormancy.dormancy_growth_scale, 0.0);
    }
};

} // namespace core
} // namespace acell