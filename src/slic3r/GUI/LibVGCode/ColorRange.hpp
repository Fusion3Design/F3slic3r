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

#include "Types.hpp"

namespace libvgcode {

// Alias for GCodeViewer::Extrusion::Range
class ColorRange
{
public:
    enum class EType : unsigned char
    {
        Linear,
        Logarithmic,
        COUNT
    };

    explicit ColorRange(EType type = EType::Linear);

    void update(float value);
    void reset();

    EType get_type() const;
    Color get_color_at(float value) const;
    unsigned int get_count() const;

//################################################################################################################################
    // Debug
    const std::array<float, 2>& get_range() const;
//################################################################################################################################

private:
    EType m_type{ EType::Linear };
    // [0] = min, [1] = max
    std::array<float, 2> m_range{ FLT_MAX, -FLT_MAX };
    // updates counter
    unsigned int m_count{ 0 };
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_COLORRANGE_HPP