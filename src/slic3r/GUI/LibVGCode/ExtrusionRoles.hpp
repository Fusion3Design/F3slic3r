///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_EXTRUSION_ROLES_HPP
#define VGCODE_EXTRUSION_ROLES_HPP

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include "Types.hpp"

#include <map>

namespace libvgcode {

class ExtrusionRoles
{
public:
    struct Item
    {
        std::array<float, static_cast<size_t>(ETimeMode::COUNT)> times;
    };

    void add(EGCodeExtrusionRole role, const std::array<float, static_cast<size_t>(ETimeMode::COUNT)>& times);

    uint32_t get_roles_count() const;
    std::vector<EGCodeExtrusionRole> get_roles() const;
    float get_time(EGCodeExtrusionRole role, ETimeMode mode) const;

    void reset();

private:
    std::map<EGCodeExtrusionRole, Item> m_items;
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_EXTRUSION_ROLES_HPP
