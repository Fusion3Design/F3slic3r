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

static constexpr float DEFAULT_TRAVELS_RADIUS = 0.1f;
static constexpr float DEFAULT_WIPES_RADIUS   = 0.1f;

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

static constexpr size_t VIEW_TYPES_COUNT = static_cast<size_t>(EViewType::COUNT);

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

static constexpr size_t MOVE_TYPES_COUNT = static_cast<size_t>(EMoveType::COUNT);

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

static constexpr size_t GCODE_EXTRUSION_ROLES_COUNT = static_cast<size_t>(EGCodeExtrusionRole::COUNT);

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

static constexpr size_t OPTION_TYPES_COUNT = static_cast<size_t>(EOptionType::COUNT);

//
// Time modes
//
enum class ETimeMode : uint8_t
{
    Normal,
    Stealth,
    COUNT
};

static constexpr size_t TIME_MODES_COUNT = static_cast<size_t>(ETimeMode::COUNT);

//
// Color range types
//
enum class EColorRangeType : uint8_t
{
    Linear,
    Logarithmic,
    COUNT
};

static constexpr size_t COLOR_RANGE_TYPES_COUNT = static_cast<size_t>(EColorRangeType::COUNT);

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

static constexpr size_t BBOX_TYPES_COUNT = static_cast<size_t>(EBBoxType::COUNT);

//
// Predefined colors
//
static const Color DUMMY_COLOR{  64,  64,  64 };

//
// Palette used to render moves by ranges
// EViewType: Height, Width, Speed, FanSpeed, Temperature, VolumetricFlowRate,
//            LayerTimeLinear, LayerTimeLogarithmic
//
static const std::vector<Color> RANGES_COLORS{ {
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
extern EOptionType move_type_to_option(EMoveType type);

//
// Returns the linear interpolation between the two given colors
// at the given t.
// t is clamped in the range [0..1]
//
extern Color lerp(const Color& c1, const Color& c2, float t);

} // namespace libvgcode

#endif // VGCODE_TYPES_HPP