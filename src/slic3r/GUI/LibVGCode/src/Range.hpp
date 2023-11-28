///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_RANGE_HPP
#define VGCODE_RANGE_HPP

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include <array>
#include <cstdint>

namespace libvgcode {

class Range
{
public:
		const std::array<uint32_t, 2>& get() const;
		void set(const Range& other);
		void set(const std::array<uint32_t, 2>& range);
		void set(uint32_t min, uint32_t max);

		uint32_t get_min() const;
		void set_min(uint32_t min);

		uint32_t get_max() const;
		void set_max(uint32_t max);

		// clamp the given range to stay inside this range
		void clamp(Range& other);
		void reset();

		bool operator == (const Range& other) const;
		bool operator != (const Range& other) const;

private:
		//
		// [0] = min
	  // [1] = max
		//
		std::array<uint32_t, 2> m_range{ 0, 0 };
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_RANGE_HPP