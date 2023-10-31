///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_TYPES_HPP
#define VGCODE_TYPES_HPP

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include <array>
#include <cstdint>

namespace libvgcode {

static constexpr float PI = 3.141592f;

using Vec3f = std::array<float, 3>;
//
// 4 by 4 square matrix with column-major order:
// | a[0] a[4] a[8]  a[12] |
// | a[1] a[5] a[9]  a[13] |
// | a[2] a[6] a[10] a[14] |
// | a[3] a[7] a[11] a[15] |
//
using Mat4x4f = std::array<float, 16>;
//
// [0] = red
// [1] = green
// [2] = blue
// values should belong to [0..1]
//
using Color = std::array<float, 3>;

// Alias for GCodeViewer::EViewType defined into GCodeViewer.hpp
enum class EViewType : uint8_t
{
    FeatureType,
    Height,
    Width,
    Speed,
    FanSpeed,
    Temperature,
    VolumetricFlowRate,
    LayerTimeLinear,
    LayerTimeLogarithmic,
    Tool,
    ColorPrint,
    COUNT
};

// Alias for EMoveType defined into GCodeProcessor.hpp
enum class EMoveType : uint8_t
{
    Noop,
    Retract,
    Unretract,
    Seam,
    ToolChange,
    ColorChange,
    PausePrint,
    CustomGCode,
    Travel,
    Wipe,
    Extrude,
    COUNT
};

// Alias for GCodeExtrusionRole defined into ExtrusionRole.hpp
enum class EGCodeExtrusionRole : uint8_t
{
	  None,
	  Perimeter,
	  ExternalPerimeter,
	  OverhangPerimeter,
	  InternalInfill,
	  SolidInfill,
	  TopSolidInfill,
	  Ironing,
	  BridgeInfill,
	  GapFill,
	  Skirt,
	  SupportMaterial,
	  SupportMaterialInterface,
	  WipeTower,
	  Custom,
    COUNT
};

// Alias for Preview::OptionType defined into GUI_Preview.hpp
enum class EOptionType : uint8_t
{
    Travels,
    Wipes,
    Retractions,
    Unretractions,
    Seams,
    ToolChanges,
    ColorChanges,
    PausePrints,
    CustomGCodes,
#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
    CenterOfGravity,
    Shells,
    ToolMarker,
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
    COUNT
};

// Alias for PrintEstimatedStatistics::ETimeMode defined into GCodeProcessor.hpp
enum class ETimeMode : uint8_t
{
    Normal,
    Stealth,
    COUNT
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_TYPES_HPP