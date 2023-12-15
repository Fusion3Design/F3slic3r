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

const Interval& Range::get() const
{
    return m_range;
}

void Range::set(const Range& other)
{
    m_range = other.m_range;
}

void Range::set(const Interval& range)
{
    set(range[0], range[1]);
}

void Range::set(Interval::value_type min, Interval::value_type max)
{
    if (max < min)
        std::swap(min, max);
    m_range[0] = min;
    m_range[1] = max;
}

Interval::value_type Range::get_min() const
{
    return m_range[0];
}

void Range::set_min(Interval::value_type min)
{
    set(min, m_range[1]);
}

Interval::value_type Range::get_max() const
{
    return m_range[1];
}

void Range::set_max(Interval::value_type max)
{
    set(m_range[0], max);
}

void Range::clamp(Range& other)
{
    other.m_range[0] = std::clamp(other.m_range[0], m_range[0], m_range[1]);
    other.m_range[1] = std::clamp(other.m_range[1], m_range[0], m_range[1]);
}

void Range::reset()
{
    m_range = { 0, 0 };
}

bool Range::operator == (const Range& other) const
{
    return m_range == other.m_range;
}

bool Range::operator != (const Range& other) const
{
    return m_range != other.m_range;
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
