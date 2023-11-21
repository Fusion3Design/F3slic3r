//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "Types.hpp"

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

} // namespace libvgcode
