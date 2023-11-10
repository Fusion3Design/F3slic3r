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

namespace libvgcode {

const std::array<uint32_t, 2>& ViewRange::get_current_range() const
{
		return m_current.get();
}

void ViewRange::set_current_range(const Range& other)
{
		set_current_range(other.get());
}

void ViewRange::set_current_range(const std::array<uint32_t, 2>& range)
{
		set_current_range(range[0], range[1]);
}

void ViewRange::set_current_range(uint32_t min, uint32_t max)
{
		m_current.set(min, max);
		// force the current range to stay inside the modified global range
		m_global.clamp(m_current);
}

const std::array<uint32_t, 2>& ViewRange::get_global_range() const
{
		return m_global.get();
}

void ViewRange::set_global_range(const Range& other)
{
		set_global_range(other.get());
}

void ViewRange::set_global_range(const std::array<uint32_t, 2>& range)
{
		set_global_range(range[0], range[1]);
}

void ViewRange::set_global_range(uint32_t min, uint32_t max)
{
		// is the global range being extended ?
		const bool new_max = max > m_global.get()[1];
		m_global.set(min, max);
		// force the current range to stay inside the modified global range
		m_global.clamp(m_current);
		if (new_max)
				// force the current range to fill the extended global range
				m_current.set(m_current.get()[0], max);
}

void ViewRange::reset()
{
		m_current.reset();
		m_global.reset();
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
