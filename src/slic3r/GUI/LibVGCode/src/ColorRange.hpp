///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_COLORRANGE_HPP
#define VGCODE_COLORRANGE_HPP

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include "../include/Types.hpp"

namespace libvgcode {

class ColorRange
{
public:
    explicit ColorRange(EColorRangeType type = EColorRangeType::Linear);

    void update(float value);
    void reset();

    EColorRangeType get_type() const;
    Color get_color_at(float value) const;

    const std::array<float, 2>& get_range() const;

private:
    EColorRangeType m_type{ EColorRangeType::Linear };

    //
    // [0] = min
    // [1] = max
    //
    std::array<float, 2> m_range{ FLT_MAX, -FLT_MAX };
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_COLORRANGE_HPP