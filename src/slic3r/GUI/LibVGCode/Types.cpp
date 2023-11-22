//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "Types.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include <algorithm>

namespace libvgcode {

// mapping from EMoveType to EOptionType
EOptionType type_to_option(EMoveType type)
{
    switch (type)
    {
    case EMoveType::Retract:     { return EOptionType::Retractions; }
    case EMoveType::Unretract:   { return EOptionType::Unretractions; }
    case EMoveType::Seam:        { return EOptionType::Seams; }
    case EMoveType::ToolChange:  { return EOptionType::ToolChanges; }
    case EMoveType::ColorChange: { return EOptionType::ColorChanges; }
    case EMoveType::PausePrint:  { return EOptionType::PausePrints; }
    case EMoveType::CustomGCode: { return EOptionType::CustomGCodes; }
    default:                     { return EOptionType::COUNT; }
    }
}

Color lerp(const Color& c1, const Color& c2, float t)
{
    // It will be possible to replace this with std::lerp when using c++20
    t = std::clamp(t, 0.0f, 1.0f);
    const float one_minus_t = 1.0f - t;
    return { one_minus_t * c1[0] + t * c2[0], one_minus_t * c1[1] + t * c2[1], one_minus_t * c1[2] + t * c2[2] };
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
