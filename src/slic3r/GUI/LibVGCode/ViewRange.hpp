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

#include <array>

namespace libvgcode {

class ViewRange
{
public:
		size_t get_current_min() const;
		size_t get_current_max() const;

		const std::array<size_t, 2>& get_current_range() const;
		void set_current_range(size_t min, size_t max);

		size_t get_global_min() const;
		size_t get_global_max() const;

		const std::array<size_t, 2>& get_global_range() const;
		void set_global_range(size_t min, size_t max);

private:
		std::array<size_t, 2> m_current{ 0, 0 };
		std::array<size_t, 2> m_global{ 0, 0 };
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_VIEWRANGE_HPP