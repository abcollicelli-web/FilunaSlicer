// Filuna Slicer OEM candidate — Bambu Studio speed interpolation primitives.
// Source-derived from bambulab/BambuStudio src/libslic3r/GCode.cpp,
// GCode::mapping_speed / GCode::get_speed_coor_x (Git blob
// 31cfddddeef4a0f972e8c60b42bf0d24d35017e6).
// Upstream copyright remains with its respective holders.
// GPL/AGPL provenance must be carried with any resulting distribution.
// This is a VERIFIED MATH CANDIDATE ONLY, not linked to the slicing pipeline.
// Do not treat as module loaded, hardware PASS, or OEM equivalent G-code.
#pragma once

#include <cmath>
#include <limits>

namespace Slic3r::Filuna::BambuA1 {

// Bambu Studio GCode::mapping_speed: if dist<=0 return 0;
// m_smooth_coefficient * pow(dist,2). No global printer config here.
inline double mapping_speed(double distance, double smooth_coefficient)
{
    if (distance <= 0.0)
        return 0.0;
    return smooth_coefficient * std::pow(distance, 2.0);
}

// Bambu Studio GCode::get_speed_coor_x: sqrt(speed / coefficient).
// The original code assumes a positive coefficient; fail safely when the
// upstream precondition is not met instead of dividing by zero.
inline double inverse_speed_distance(double speed, double smooth_coefficient)
{
    if (smooth_coefficient <= 0.0 || speed < 0.0)
        return std::numeric_limits<double>::quiet_NaN();
    return std::sqrt(speed / smooth_coefficient);
}

} // namespace Slic3r::Filuna::BambuA1
