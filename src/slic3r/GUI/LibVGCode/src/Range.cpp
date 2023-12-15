//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "Range.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include <algorithm>

namespace libvgcode {

void Range::set(Interval::value_type min, Interval::value_type max)
{
    if (max < min)
        std::swap(min, max);
    m_range[0] = min;
    m_range[1] = max;
}

void Range::clamp(Range& other)
{
    other.m_range[0] = std::clamp(other.m_range[0], m_range[0], m_range[1]);
    other.m_range[1] = std::clamp(other.m_range[1], m_range[0], m_range[1]);
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
