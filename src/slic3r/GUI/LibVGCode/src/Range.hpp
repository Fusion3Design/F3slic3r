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

#include "../include/Types.hpp"

namespace libvgcode {

class Range
{
public:
    const Interval& get() const;
    void set(const Range& other);
    void set(const Interval& range);
    void set(Interval::value_type min, Interval::value_type max);
    
    Interval::value_type get_min() const;
    void set_min(Interval::value_type min);
    
    Interval::value_type get_max() const;
    void set_max(Interval::value_type max);
    
    // clamp the given range to stay inside this range
    void clamp(Range& other);
    void reset();
    
    bool operator == (const Range& other) const;
    bool operator != (const Range& other) const;

private:
    Interval m_range{ 0, 0 };
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_RANGE_HPP