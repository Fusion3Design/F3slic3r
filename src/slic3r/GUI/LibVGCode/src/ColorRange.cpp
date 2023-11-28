//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "ColorRange.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include <algorithm>
#include <assert.h>

namespace libvgcode {

ColorRange::ColorRange(EColorRangeType type)
: m_type(type)
{
}

void ColorRange::update(float value)
{
    m_range[0] = std::min(m_range[0], value);
    m_range[1] = std::max(m_range[1], value);
}

void ColorRange::reset()
{
    m_range = { FLT_MAX, -FLT_MAX };
}

static float step_size(const std::array<float, 2>& range, EColorRangeType type)
{
    switch (type)
    {
    default:
    case EColorRangeType::Linear:
    {
        return (range[1] - range[0]) / (static_cast<float>(Ranges_Colors.size()) - 1.0f);
    }
    case EColorRangeType::Logarithmic:
    {
        return (range[0] != 0.0f) ? std::log(range[1] / range[0]) / (static_cast<float>(Ranges_Colors.size()) - 1.0f) : 0.0f;
    }
    }
}

EColorRangeType ColorRange::get_type() const
{
    return m_type;
}

Color ColorRange::get_color_at(float value) const
{
    // Input value scaled to the colors range
    float global_t = 0.0f;
    value = std::clamp(value, m_range[0], m_range[1]);
    const float step = step_size(m_range, m_type);
    if (step > 0.0f) {
        if (m_type == EColorRangeType::Logarithmic) {
            if (m_range[0] != 0.0f)
                global_t = log(value / m_range[0]) / step;
        }
        else
            global_t = (value - m_range[0]) / step;
    }

    const size_t color_max_idx = Ranges_Colors.size() - 1;

    // Compute the two colors just below (low) and above (high) the input value
    const size_t color_low_idx = std::clamp<size_t>(static_cast<size_t>(global_t), 0, color_max_idx);
    const size_t color_high_idx = std::clamp<size_t>(color_low_idx + 1, 0, color_max_idx);

    // Interpolate between the low and high colors to find exactly which color the input value should get
    return lerp(Ranges_Colors[color_low_idx], Ranges_Colors[color_high_idx], global_t - static_cast<float>(color_low_idx));
}

const std::array<float, 2>& ColorRange::get_range() const
{
    return m_range;
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

