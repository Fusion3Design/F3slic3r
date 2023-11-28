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

static constexpr float Default_Travel_Radius = 0.05f;
static constexpr float Default_Wipe_Radius   = 0.05f;

//
// Vector in 3 dimensions
// [0] -> x
// [1] -> y
// [2] -> z
// Used for positions, displacements and so on.
//
using Vec3 = std::array<float, 3>;

//
// 4x4 square matrix with elements in column-major order:
// | a[0] a[4] a[8]  a[12] |
// | a[1] a[5] a[9]  a[13] |
// | a[2] a[6] a[10] a[14] |
// | a[3] a[7] a[11] a[15] |
//
using Mat4x4 = std::array<float, 16>;

//
// RGB color
// [0] -> red
// [1] -> green
// [2] -> blue
//
using Color = std::array<uint8_t, 3>;

//
// View types
//
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

//
// Move types
//
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

//
// Extrusion roles
//
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

//
// Option types
//
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
#if !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
    CenterOfGravity,
    Shells,
    ToolMarker,
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
    COUNT
};

//
// Time modes
//
enum class ETimeMode : uint8_t
{
    Normal,
    Stealth,
    COUNT
};

static constexpr size_t Time_Modes_Count = static_cast<size_t>(ETimeMode::COUNT);

//
// Color range type
//
enum class EColorRangeType : uint8_t
{
    Linear,
    Logarithmic,
    COUNT
};

static constexpr size_t Color_Range_Types_Count = static_cast<size_t>(EColorRangeType::COUNT);

//
// Predefined colors
//
static const Color Dummy_Color{  64,  64,  64 };
static const Color Wipe_Color { 255, 255, 255 };

//
// Palette used to render moves by ranges
// EViewType: Height, Width, Speed, FanSpeed, Temperature, VolumetricFlowRate,
//            LayerTimeLinear, LayerTimeLogarithmic
//
static const std::vector<Color> Ranges_Colors{ {
    {  11,  44, 122 }, // bluish
    {  19,  89, 133 },
    {  28, 136, 145 },
    {   4, 214,  15 },
    { 170, 242,   0 },
    { 252, 249,   3 },
    { 245, 206,  10 },
    { 227, 136,  32 },
    { 209, 104,  48 },
    { 194,  82,  60 },
    { 148,  38,  22 }  // reddish
} };

//
// Palette used to render extrusion moves by extrusion roles
// EViewType: FeatureType
//
static const std::vector<Color> Extrusion_Roles_Colors{ {
    { 230, 179, 179 },   // None
    { 255, 230,  77 },   // Perimeter
    { 255, 125,  56 },   // ExternalPerimeter
    {  31,  31, 255 },   // OverhangPerimeter
    { 176,  48,  41 },   // InternalInfill
    { 150,  84, 204 },   // SolidInfill
    { 240,  64,  64 },   // TopSolidInfill
    { 255, 140, 105 },   // Ironing
    {  77, 128, 186 },   // BridgeInfill
    { 255, 255, 255 },   // GapFill
    {   0, 135, 110 },   // Skirt
    {   0, 255,   0 },   // SupportMaterial
    {   0, 128,   0 },   // SupportMaterialInterface
    { 179, 227, 171 },   // WipeTower
    {  94, 209, 148 }    // Custom
} };

//
// Palette used to render travel moves
// EViewType: FeatureType, Height, Width, FanSpeed, Temperature, VolumetricFlowRate,
//            LayerTimeLinear, LayerTimeLogarithmic
//
static const std::vector<Color> Travels_Colors{ {
    {  56,  72, 155 }, // Move
    {  29, 108,  26 }, // Extrude
    { 129,  16,   7 }  // Retract
} };

//
// Palette used to render options
//
static const std::map<EMoveType, Color> Options_Colors{ {
    { EMoveType::Retract,     { 205,  34, 214 } },
    { EMoveType::Unretract,   {  73, 173, 207 } },
    { EMoveType::Seam,        { 230, 230, 230 } },
    { EMoveType::ToolChange,  { 193, 190,  99 } },
    { EMoveType::ColorChange, { 218, 148, 139 } },
    { EMoveType::PausePrint,  {  82, 240, 131 } },
    { EMoveType::CustomGCode, { 226, 210,  67 } }
} };

//
// Mapping from EMoveType to EOptionType
//
extern EOptionType type_to_option(EMoveType type);

//
// Returns the linear interpolation between the two given colors
// at the given t.
// t is clamped in the range [0..1]
//
extern Color lerp(const Color& c1, const Color& c2, float t);

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_TYPES_HPP