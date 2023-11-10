///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_VIEWRANGE_HPP
#define VGCODE_VIEWRANGE_HPP

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include "Range.hpp"

namespace libvgcode {

class ViewRange
{
public:
		const std::array<uint32_t, 2>& get_current() const;
		void set_current(const Range& other);
		void set_current(const std::array<uint32_t, 2>& range);
		void set_current(uint32_t min, uint32_t max);

		const std::array<uint32_t, 2>& get_global() const;
		void set_global(const Range& other);
		void set_global(const std::array<uint32_t, 2>& range);
		void set_global(uint32_t min, uint32_t max);

		void reset();

private:
		Range m_current;
		Range m_global;
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_VIEWRANGE_HPP