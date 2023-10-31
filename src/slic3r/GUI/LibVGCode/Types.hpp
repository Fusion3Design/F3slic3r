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

namespace libvgcode {

static constexpr float PI = 3.141592f;

using Vec3f   = std::array<float, 3>;
// 4 by 4 square matrix with column-major order
using Mat4x4f = std::array<float, 16>;
using Color   = std::array<float, 3>;

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
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
    Shells,
    ToolMarker,
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