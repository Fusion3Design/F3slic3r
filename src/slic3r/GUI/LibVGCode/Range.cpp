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

const std::array<uint32_t, 2>& Range::get() const
{
		return m_range;
}

void Range::set(const std::array<uint32_t, 2>& range)
{
		set(range[0], range[1]);
}

void Range::set(uint32_t min, uint32_t max)
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
