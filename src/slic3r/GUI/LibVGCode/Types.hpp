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
#include <vector>
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

//
// Predefined colors
//
static const Color Dummy_Color{ 0.25f, 0.25f, 0.25f };
static const Color Wipe_Color{ 1.0f, 1.0f, 0.0f };
static const Color Default_Tool_Color{ 1.0f, 0.502f, 0.0f };

//
// Palette used to render moves by ranges
//
static const std::vector<Color> Ranges_Colors{ {
    { 0.043f, 0.173f, 0.478f }, // bluish
    { 0.075f, 0.349f, 0.522f },
    { 0.110f, 0.533f, 0.569f },
    { 0.016f, 0.839f, 0.059f },
    { 0.667f, 0.949f, 0.000f },
    { 0.988f, 0.975f, 0.012f },
    { 0.961f, 0.808f, 0.039f },
    { 0.890f, 0.533f, 0.125f },
    { 0.820f, 0.408f, 0.188f },
    { 0.761f, 0.322f, 0.235f },
    { 0.581f, 0.149f, 0.087f }  // reddish
} };

//
// Palette used to render extrusion moves by extrusion roles
//
static const std::vector<Color> Extrusion_Roles_Colors{ {
    { 0.90f, 0.70f, 0.70f },   // None
    { 1.00f, 0.90f, 0.30f },   // Perimeter
    { 1.00f, 0.49f, 0.22f },   // ExternalPerimeter
    { 0.12f, 0.12f, 1.00f },   // OverhangPerimeter
    { 0.69f, 0.19f, 0.16f },   // InternalInfill
    { 0.59f, 0.33f, 0.80f },   // SolidInfill
    { 0.94f, 0.25f, 0.25f },   // TopSolidInfill
    { 1.00f, 0.55f, 0.41f },   // Ironing
    { 0.30f, 0.50f, 0.73f },   // BridgeInfill
    { 1.00f, 1.00f, 1.00f },   // GapFill
    { 0.00f, 0.53f, 0.43f },   // Skirt
    { 0.00f, 1.00f, 0.00f },   // SupportMaterial
    { 0.00f, 0.50f, 0.00f },   // SupportMaterialInterface
    { 0.70f, 0.89f, 0.67f },   // WipeTower
    { 0.37f, 0.82f, 0.58f },   // Custom
} };

//
// Palette used to render travel moves
//
static const std::vector<Color> Travels_Colors{ {
    { 0.219f, 0.282f, 0.609f }, // Move
    { 0.112f, 0.422f, 0.103f }, // Extrude
    { 0.505f, 0.064f, 0.028f }  // Retract
} };

//
// Palette used to render option moves
//
static const std::map<EMoveType, Color> Options_Colors{ {
    { EMoveType::Retract,     { 0.803f, 0.135f, 0.839f } },
    { EMoveType::Unretract,   { 0.287f, 0.679f, 0.810f } },
    { EMoveType::Seam,        { 0.900f, 0.900f, 0.900f } },
    { EMoveType::ToolChange,  { 0.758f, 0.744f, 0.389f } },
    { EMoveType::ColorChange, { 0.856f, 0.582f, 0.546f } },
    { EMoveType::PausePrint,  { 0.322f, 0.942f, 0.512f } },
    { EMoveType::CustomGCode, { 0.886f, 0.825f, 0.262f } }
} };

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_TYPES_HPP