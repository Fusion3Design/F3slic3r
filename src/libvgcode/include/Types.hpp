///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_TYPES_HPP
#define VGCODE_TYPES_HPP

#define ENABLE_COG_AND_TOOL_MARKERS 0

#include <array>
#include <vector>
#include <cstdint>

namespace libvgcode {

static constexpr float PI = 3.141592f;

static constexpr float Default_Travels_Radius = 0.1f;
static constexpr float Default_Wipes_Radius   = 0.1f;

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
// Axis aligned box in 3 dimensions
// [0] -> { min_x, min_y, min_z }
// [1] -> { max_x, max_y, max_z }
//
using AABox = std::array<Vec3, 2>;

//
// One dimensional natural numbers interval
// [0] -> min
// [1] -> max
//
using Interval = std::array<size_t, 2>;

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

static constexpr size_t View_Types_Count = static_cast<size_t>(EViewType::COUNT);

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

static constexpr size_t Move_Types_Count = static_cast<size_t>(EMoveType::COUNT);

//
// Travel move types
//
enum class ETravelMoveType : uint8_t
{
    Move,
    Extrude,
    Retract,
    COUNT
};

static constexpr size_t Travel_Move_Types_Count = static_cast<size_t>(ETravelMoveType::COUNT);

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

static constexpr size_t GCode_Extrusion_Roles_Count = static_cast<size_t>(EGCodeExtrusionRole::COUNT);

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
#if ENABLE_COG_AND_TOOL_MARKERS
    CenterOfGravity,
    ToolMarker,
#endif // ENABLE_COG_AND_TOOL_MARKERS
    COUNT
};

static constexpr size_t Option_Types_Count = static_cast<size_t>(EOptionType::COUNT);

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
// Color range types
//
enum class EColorRangeType : uint8_t
{
    Linear,
    Logarithmic,
    COUNT
};

static constexpr size_t Color_Range_Types_Count = static_cast<size_t>(EColorRangeType::COUNT);

//
// Bounding box types
//
enum class EBBoxType : uint8_t
{
    Full,
    Extrusion,
    ExtrusionNoCustom,
    COUNT
};

static constexpr size_t BBox_Types_Count = static_cast<size_t>(EBBoxType::COUNT);

//
// Predefined colors
//
static const Color Dummy_Color{  64,  64,  64 };
static const Color Wipe_Color { 255, 255,   0 };

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

#endif // VGCODE_TYPES_HPP