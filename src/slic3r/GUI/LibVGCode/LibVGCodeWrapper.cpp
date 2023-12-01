///|/ Copyright (c) Prusa Research 2020 - 2023 Enrico Turri @enricoturri1966
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "libslic3r/libslic3r.h"
#include "LibVGCodeWrapper.hpp"

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#if ENABLE_NEW_GCODE_VIEWER
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

namespace libvgcode {

Vec3 convert(const Slic3r::Vec3f& v)
{
    return { v.x(), v.y(), v.z() };
}

Slic3r::Vec3f convert(const Vec3& v)
{
    return { v[0], v[1], v[2] };
}

Mat4x4 convert(const Slic3r::Matrix4f& m)
{
    Mat4x4 ret;
    std::memcpy(ret.data(), m.data(), 16 * sizeof(float));
    return ret;
}

Slic3r::ColorRGBA convert(const Color& c)
{
    static const float inv_255 = 1.0f / 255.0f;
    return { c[0] * inv_255, c[1] * inv_255, c[2] * inv_255, 1.0f };
}

Color convert(const Slic3r::ColorRGBA& c)
{
    return { static_cast<uint8_t>(c.r() * 255.0f), static_cast<uint8_t>(c.g() * 255.0f), static_cast<uint8_t>(c.b() * 255.0f) };
}

Slic3r::GCodeExtrusionRole convert(EGCodeExtrusionRole role)
{
    switch (role)
    {
    case EGCodeExtrusionRole::None:                     { return Slic3r::GCodeExtrusionRole::None; }
    case EGCodeExtrusionRole::Perimeter:                { return Slic3r::GCodeExtrusionRole::Perimeter; }
    case EGCodeExtrusionRole::ExternalPerimeter:        { return Slic3r::GCodeExtrusionRole::ExternalPerimeter; }
    case EGCodeExtrusionRole::OverhangPerimeter:        { return Slic3r::GCodeExtrusionRole::OverhangPerimeter; }
    case EGCodeExtrusionRole::InternalInfill:           { return Slic3r::GCodeExtrusionRole::InternalInfill; }
    case EGCodeExtrusionRole::SolidInfill:              { return Slic3r::GCodeExtrusionRole::SolidInfill; }
    case EGCodeExtrusionRole::TopSolidInfill:           { return Slic3r::GCodeExtrusionRole::TopSolidInfill; }
    case EGCodeExtrusionRole::Ironing:                  { return Slic3r::GCodeExtrusionRole::Ironing; }
    case EGCodeExtrusionRole::BridgeInfill:             { return Slic3r::GCodeExtrusionRole::BridgeInfill; }
    case EGCodeExtrusionRole::GapFill:                  { return Slic3r::GCodeExtrusionRole::GapFill; }
    case EGCodeExtrusionRole::Skirt:                    { return Slic3r::GCodeExtrusionRole::Skirt; }
    case EGCodeExtrusionRole::SupportMaterial:          { return Slic3r::GCodeExtrusionRole::SupportMaterial; }
    case EGCodeExtrusionRole::SupportMaterialInterface: { return Slic3r::GCodeExtrusionRole::SupportMaterialInterface; }
    case EGCodeExtrusionRole::WipeTower:                { return Slic3r::GCodeExtrusionRole::WipeTower; }
    case EGCodeExtrusionRole::Custom:                   { return Slic3r::GCodeExtrusionRole::Custom; }
    default:                                            { return Slic3r::GCodeExtrusionRole::None; }
    }
}

EGCodeExtrusionRole convert(Slic3r::GCodeExtrusionRole role)
{
    switch (role)
    {
    case Slic3r::GCodeExtrusionRole::None:                     { return EGCodeExtrusionRole::None; }
    case Slic3r::GCodeExtrusionRole::Perimeter:                { return EGCodeExtrusionRole::Perimeter; }
    case Slic3r::GCodeExtrusionRole::ExternalPerimeter:        { return EGCodeExtrusionRole::ExternalPerimeter; }
    case Slic3r::GCodeExtrusionRole::OverhangPerimeter:        { return EGCodeExtrusionRole::OverhangPerimeter; }
    case Slic3r::GCodeExtrusionRole::InternalInfill:           { return EGCodeExtrusionRole::InternalInfill; }
    case Slic3r::GCodeExtrusionRole::SolidInfill:              { return EGCodeExtrusionRole::SolidInfill; }
    case Slic3r::GCodeExtrusionRole::TopSolidInfill:           { return EGCodeExtrusionRole::TopSolidInfill; }
    case Slic3r::GCodeExtrusionRole::Ironing:                  { return EGCodeExtrusionRole::Ironing; }
    case Slic3r::GCodeExtrusionRole::BridgeInfill:             { return EGCodeExtrusionRole::BridgeInfill; }
    case Slic3r::GCodeExtrusionRole::GapFill:                  { return EGCodeExtrusionRole::GapFill; }
    case Slic3r::GCodeExtrusionRole::Skirt:                    { return EGCodeExtrusionRole::Skirt; }
    case Slic3r::GCodeExtrusionRole::SupportMaterial:          { return EGCodeExtrusionRole::SupportMaterial; }
    case Slic3r::GCodeExtrusionRole::SupportMaterialInterface: { return EGCodeExtrusionRole::SupportMaterialInterface; }
    case Slic3r::GCodeExtrusionRole::WipeTower:                { return EGCodeExtrusionRole::WipeTower; }
    case Slic3r::GCodeExtrusionRole::Custom:                   { return EGCodeExtrusionRole::Custom; }
    default:                                                   { return EGCodeExtrusionRole::None; }
    }
}

EMoveType convert(Slic3r::EMoveType type)
{
    switch (type)
    {
    case Slic3r::EMoveType::Noop:         { return EMoveType::Noop; }
    case Slic3r::EMoveType::Retract:      { return EMoveType::Retract; }
    case Slic3r::EMoveType::Unretract:    { return EMoveType::Unretract; }
    case Slic3r::EMoveType::Seam:         { return EMoveType::Seam; }
    case Slic3r::EMoveType::Tool_change:  { return EMoveType::ToolChange; }
    case Slic3r::EMoveType::Color_change: { return EMoveType::ColorChange; }
    case Slic3r::EMoveType::Pause_Print:  { return EMoveType::PausePrint; }
    case Slic3r::EMoveType::Custom_GCode: { return EMoveType::CustomGCode; }
    case Slic3r::EMoveType::Travel:       { return EMoveType::Travel; }
    case Slic3r::EMoveType::Wipe:         { return EMoveType::Wipe; }
    case Slic3r::EMoveType::Extrude:      { return EMoveType::Extrude; }
    default:                              { return EMoveType::COUNT; }
    }
}

EOptionType convert(const Slic3r::GUI::Preview::OptionType& type)
{
    switch (type)
    {
    case Slic3r::GUI::Preview::OptionType::Travel:          { return EOptionType::Travels; }
    case Slic3r::GUI::Preview::OptionType::Wipe:            { return EOptionType::Wipes; }
    case Slic3r::GUI::Preview::OptionType::Retractions:     { return EOptionType::Retractions; }
    case Slic3r::GUI::Preview::OptionType::Unretractions:   { return EOptionType::Unretractions; }
    case Slic3r::GUI::Preview::OptionType::Seams:           { return EOptionType::Seams; }
    case Slic3r::GUI::Preview::OptionType::ToolChanges:     { return EOptionType::ToolChanges; }
    case Slic3r::GUI::Preview::OptionType::ColorChanges:    { return EOptionType::ColorChanges; }
    case Slic3r::GUI::Preview::OptionType::PausePrints:     { return EOptionType::PausePrints; }
    case Slic3r::GUI::Preview::OptionType::CustomGCodes:    { return EOptionType::CustomGCodes; }
#if ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
    case Slic3r::GUI::Preview::OptionType::CenterOfGravity: { return EOptionType::COUNT; }
    case Slic3r::GUI::Preview::OptionType::ToolMarker:      { return EOptionType::COUNT; }
#else
    case Slic3r::GUI::Preview::OptionType::CenterOfGravity: { return EOptionType::CenterOfGravity; }
    case Slic3r::GUI::Preview::OptionType::ToolMarker:      { return EOptionType::ToolMarker; }
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
    default:                                                { return EOptionType::COUNT; }
    }
}

ETimeMode convert(const Slic3r::PrintEstimatedStatistics::ETimeMode& mode)
{
    switch (mode)
    {
    case Slic3r::PrintEstimatedStatistics::ETimeMode::Normal:  { return ETimeMode::Normal; }
    case Slic3r::PrintEstimatedStatistics::ETimeMode::Stealth: { return ETimeMode::Stealth; }
    default:                                                   { return ETimeMode::COUNT; }
    }
}

Slic3r::PrintEstimatedStatistics::ETimeMode convert(const ETimeMode& mode)
{
    switch (mode)
    {
    case ETimeMode::Normal:  { return Slic3r::PrintEstimatedStatistics::ETimeMode::Normal; }
    case ETimeMode::Stealth: { return Slic3r::PrintEstimatedStatistics::ETimeMode::Stealth; }
    default:                 { return Slic3r::PrintEstimatedStatistics::ETimeMode::Count; }
    }
}

GCodeInputData convert(const Slic3r::GCodeProcessorResult& result)
{
    const std::vector<Slic3r::GCodeProcessorResult::MoveVertex>& moves = result.moves;
    GCodeInputData ret;
    ret.vertices.reserve(2 * moves.size());
    for (size_t i = 1; i < moves.size(); ++i) {
        const Slic3r::GCodeProcessorResult::MoveVertex& curr = moves[i];
        const Slic3r::GCodeProcessorResult::MoveVertex& prev = moves[i - 1];
        const EMoveType curr_type = convert(curr.type);

        EGCodeExtrusionRole extrusion_role;
        if (curr_type == EMoveType::Travel) {
            // for travel moves set the extrusion role
            // which will be used later to select the proper color
            if (curr.delta_extruder == 0.0f)
                extrusion_role = static_cast<EGCodeExtrusionRole>(0); // Move
            else if (curr.delta_extruder > 0.0f)
                extrusion_role = static_cast<EGCodeExtrusionRole>(1); // Extrude
            else
                extrusion_role = static_cast<EGCodeExtrusionRole>(2); // Retract
        }
        else
            extrusion_role = convert(curr.extrusion_role);

        float width;
        float height;
        switch (curr_type)
        {
        case EMoveType::Travel:
        {
            width = Default_Travel_Radius;
            height = Default_Travel_Radius;
            break;
        }
        case EMoveType::Wipe:
        {
            width = Default_Wipe_Radius;
            height = Default_Wipe_Radius;
            break;
        }
        default:
        {
            width = curr.width;
            height = curr.height;
            break;
        }
        }

        if (type_to_option(curr_type) == libvgcode::EOptionType::COUNT) {
            if (ret.vertices.empty() || prev.type != curr.type || prev.extrusion_role != curr.extrusion_role) {
                // to allow libvgcode to properly detect the start/end of a path we need to add a 'phantom' vertex
                // equal to the current one with the exception of the position, which should match the previous move position,
                // and the times, which are set to zero
#if ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
                const libvgcode::PathVertex vertex = { convert(prev.position), height, width, curr.feedrate, curr.fan_speed,
                    curr.temperature, curr.volumetric_rate(), extrusion_role, curr_type,
                    static_cast<uint32_t>(curr.gcode_id), static_cast<uint32_t>(curr.layer_id),
                    static_cast<uint8_t>(curr.extruder_id), static_cast<uint8_t>(curr.cp_color_id), { 0.0f, 0.0f } };
#else
                const libvgcode::PathVertex vertex = { convert(prev.position), height, width, curr.feedrate, curr.fan_speed,
                    curr.temperature, curr.volumetric_rate(), 0.0f, extrusion_role, curr_type,
                    static_cast<uint32_t>(curr.gcode_id), static_cast<uint32_t>(curr.layer_id),
                    static_cast<uint8_t>(curr.extruder_id), static_cast<uint8_t>(curr.cp_color_id), { 0.0f, 0.0f } };
#endif // ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
                ret.vertices.emplace_back(vertex);
            }
        }

#if ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
        const libvgcode::PathVertex vertex = { convert(curr.position), height, width, curr.feedrate, curr.fan_speed,
            curr.temperature, curr.volumetric_rate(), extrusion_role, curr_type,  static_cast<uint32_t>(curr.gcode_id),
            static_cast<uint32_t>(curr.layer_id), static_cast<uint8_t>(curr.extruder_id), static_cast<uint8_t>(curr.cp_color_id), curr.time };
#else
        const libvgcode::PathVertex vertex = { convert(curr.position), height, width, curr.feedrate, curr.fan_speed,
            curr.temperature, curr.volumetric_rate(), curr.mm3_per_mm * (curr.position - prev.position).norm(),
            extrusion_role, curr_type, static_cast<uint32_t>(curr.gcode_id), static_cast<uint32_t>(curr.layer_id),
            static_cast<uint8_t>(curr.extruder_id), static_cast<uint8_t>(curr.cp_color_id), curr.time };
#endif // ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
        ret.vertices.emplace_back(vertex);
    }
    ret.vertices.shrink_to_fit();

    ret.spiral_vase_mode = result.spiral_vase_mode;

    return ret;
}

} // namespace libvgcode

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // ENABLE_NEW_GCODE_VIEWER
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
