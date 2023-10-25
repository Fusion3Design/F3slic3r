//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "ViewRange.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include <assert.h>
#include <algorithm>

namespace libvgcode {

size_t ViewRange::get_current_min() const
{
		return m_current[0];
}

size_t ViewRange::get_current_max() const
{
		return m_current[1];
}

const std::array<size_t, 2>& ViewRange::get_current_range() const
{
		return m_current;
}

void ViewRange::set_current_range(size_t min, size_t max)
{
		assert(min <= max);
		m_current[0] = std::clamp(min, m_global[0], m_global[1]);
		m_current[1] = std::clamp(max, m_global[0], m_global[1]);
}

size_t ViewRange::get_global_min() const
{
		return m_global[0];
}

size_t ViewRange::get_global_max() const
{
		return m_global[1];
}

const std::array<size_t, 2>& ViewRange::get_global_range() const
{
		return m_global;
}

void ViewRange::set_global_range(size_t min, size_t max)
{
		assert(min <= max);
		m_global[0] = min;
		m_global[1] = max;
		m_current[0] = std::clamp(m_current[0], m_global[0], m_global[1]);
		m_current[1] = std::clamp(m_current[1], m_global[0], m_global[1]);
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
