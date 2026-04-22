#pragma once

#include <algorithm>
#include "CoreTypes.h"

namespace acell
{
namespace core
{

struct EnvironmentState
{
    double nutrient_density = 1.0;
    double oxygen_level = 1.0;
    double water_availability = 1.0;
    double micronutrient_level = 1.0;

    double temperature = 37.0;
    double ph = 7.2;
    double osmolarity = 1.0;
    double viscosity = 1.0;

    double toxin_level = 0.0;
    double oxidative_stress_field = 0.0;
    double radiation_level = 0.0;
    double mechanical_stress = 0.0;

    double resource_gradient = 0.0;
    double diffusion_rate = 1.0;
    double crowding_index = 0.0;
    double signal_field = 0.0;

    void normalize()
    {
        nutrient_density = std::max(nutrient_density, 0.0);
        oxygen_level = clamp01(oxygen_level);
        water_availability = clamp01(water_availability);
        micronutrient_level = clamp01(micronutrient_level);

        osmolarity = std::max(osmolarity, 0.0);
        viscosity = std::max(viscosity, 0.0);

        toxin_level = clamp01(toxin_level);
        oxidative_stress_field = clamp01(oxidative_stress_field);
        radiation_level = clamp01(radiation_level);
        mechanical_stress = clamp01(mechanical_stress);

        resource_gradient = clamp01(resource_gradient);
        diffusion_rate = std::max(diffusion_rate, 0.0);
        crowding_index = clamp01(crowding_index);
        signal_field = clamp01(signal_field);
    }
};

} // namespace core
} // namespace acell