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

#define LIBVGCODE_USE_EIGEN 0
#if LIBVGCODE_USE_EIGEN
#include <Eigen/Geometry>
#else
#include <map>
#include <array>
#endif // LIBVGCODE_USE_EIGEN

namespace libvgcode {

#if LIBVGCODE_USE_EIGEN
using Vec3f   = Eigen::Matrix<float, 3, 1, Eigen::DontAlign>;
using Mat4x4f = Eigen::Matrix<float, 4, 4, Eigen::DontAlign>;

static float dot(const Vec3f& v1, const Vec3f& v2)
{
    return v1.dot(v2);
}

static Vec3f normalize(const Vec3f& v)
{
    return v.normalized();
}

static float length(const Vec3f& v)
{
    return v.norm();
}
#else
using Vec3f   = std::array<float, 3>;
using Mat4x4f = std::array<float, 16>;

static float dot(const Vec3f& v1, const Vec3f& v2)
{
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

static Vec3f normalize(const Vec3f& v)
{
    const float length = std::sqrt(dot(v, v));
    assert(length > 0.0f);
    const float inv_length = 1.0f / length;
    return { v[0] * inv_length, v[1] * inv_length, v[2] * inv_length };
}

static float length(const Vec3f& v)
{
    return std::sqrt(dot(v, v));
}
#endif // LIBVGCODE_USE_EIGEN

static_assert(sizeof(Vec3f) == 3 * sizeof(float));

static Vec3f toVec3f(float value) { return { value, value, value }; };
static Vec3f toVec3f(float x, float y, float z) { return { x, y, z }; };

static bool operator == (const Vec3f& v1, const Vec3f& v2) {
    return v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2];
}

static bool operator != (const Vec3f& v1, const Vec3f& v2) {
    return v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2];
}

static Vec3f operator + (const Vec3f& v1, const Vec3f& v2) {
    return { v1[0] + v2[0], v1[1] + v2[1], v1[2] + v2[2] };
}

static Vec3f operator - (const Vec3f& v1, const Vec3f& v2) {
    return { v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2] };
}

static Vec3f operator * (float f, const Vec3f& v) {
    return { f * v[0], f * v[1], f * v[2] };
}

using Color = std::array<float, 3>;

static constexpr float PI = 3.141592f;

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
    ColorPrint
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
    Extrude
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
	  Custom
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
    CenterOfGravity,
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

static uint8_t valueof(EMoveType type)
{
    return static_cast<uint8_t>(type);
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_TYPES_HPP