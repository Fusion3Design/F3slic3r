///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_PATHVERTEX_HPP
#define VGCODE_PATHVERTEX_HPP

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include "Types.hpp"

#include <cstdint>

namespace libvgcode {

struct PathVertex
{
    Vec3f position{ 0.0f, 0.0f, 0.0f };
    float height{ 0.0f };
    float width{ 0.0f };
    float feedrate{ 0.0f };
    float fan_speed{ 0.0f };
    float temperature{ 0.0f };
    float volumetric_rate{ 0.0f };
    EGCodeExtrusionRole role{ EGCodeExtrusionRole::None };
    EMoveType type{ EMoveType::Noop };
    uint8_t extruder_id{ 0 };
    uint8_t color_id{ 0 };
    uint32_t layer_id{ 0 };

    bool is_extrusion() const;
    bool is_travel() const;
    bool is_option() const;
    bool is_wipe() const;
    bool is_custom_gcode() const;
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_PATHVERTEX_HPP