#pragma once

#include <string>
#include <algorithm>
#include <cmath>

namespace acell
{
namespace core
{

inline double clamp01(double x)
{
    return std::max(0.0, std::min(1.0, x));
}

inline double clampMin(double x, double lo)
{
    return std::max(x, lo);
}

enum class ModelType
{
    MSC1
};

enum class CellStatus
{
    Alive,
    Dormant,
    Dividing,
    Dead
};

enum class GrowthPhase
{
    Lag,
    Active,
    Stressed,
    Dormant,
    Dying
};

inline std::string toString(ModelType t)
{
    switch(t)
    {
    case ModelType::MSC1:
        return "MSC-1";
    }
    return "MSC-1";
}

inline std::string toString(CellStatus s)
{
    switch(s)
    {
    case CellStatus::Alive:
        return "alive";
    case CellStatus::Dormant:
        return "dormant";
    case CellStatus::Dividing:
        return "dividing";
    case CellStatus::Dead:
        return "dead";
    }
    return "alive";
}

inline std::string toString(GrowthPhase p)
{
    switch(p)
    {
    case GrowthPhase::Lag:
        return "lag";
    case GrowthPhase::Active:
        return "active";
    case GrowthPhase::Stressed:
        return "stressed";
    case GrowthPhase::Dormant:
        return "dormant";
    case GrowthPhase::Dying:
        return "dying";
    }
    return "lag";
}

} // namespace core
} // namespace acell
