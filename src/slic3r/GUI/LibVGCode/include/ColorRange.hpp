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

//
// Helper class to interpolate between colors defined in Ranges_Colors palette.
// Interpolation can be done linearly or logarithmically.
// Usage:
// 1) Define an instance of ColorRange of the desired type
//    ColorRange range(EColorRangeType::Linear);
// 2) Pass to the instance all the values needed to setup the range:
//    for (size_t i = 0; i < my_data.size(); ++i) {
//        range.update(my_data[i]);
//    }
// 3) Get the color at the desired value:
//    Color c = range.get_color_at(value);
//
class ColorRange
{
public:
    //
    // Constructor
    //
    explicit ColorRange(EColorRangeType type = EColorRangeType::Linear);

    //
    // Use the passed value to update the range
    //
    void update(float value);

    //
    // Reset the range
    // Call this method before reuse an instance of ColorRange
    //
    void reset();

    //
    // Return the type of this ColorRange
    //
    EColorRangeType get_type() const;

    //
    // Return the interpolated color at the given value
    // Value is clamped to the range
    //
    Color get_color_at(float value) const;

    //
    // Return the range of this ColorRange
    // [0] -> min
    // [1] -> max
    //
    const std::array<float, 2>& get_range() const;

    float get_step_size() const;
    std::vector<float> get_values() const;

    static const ColorRange Dummy_Color_Range;

private:
    EColorRangeType m_type{ EColorRangeType::Linear };

    //
    // [0] = min
    // [1] = max
    //
    std::array<float, 2> m_range{ FLT_MAX, -FLT_MAX };

    //
    // Count of different values passed to update()
    // 
    size_t m_count{ 0 };
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_COLORRANGE_HPP