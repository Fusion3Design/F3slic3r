//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak, Vojtěch Bubník @bubnikv, Oleksandra Iushchenko @YuSanka
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "ViewerImpl.hpp"
#include "ViewRange.hpp"
#include "Settings.hpp"
#include "Shaders.hpp"
#include "OpenGLUtils.hpp"
#include "Utils.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
#include "libslic3r/GCode/GCodeProcessor.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/ImGuiWrapper.hpp"
//################################################################################################################################

#include <map>
#include <assert.h>
#include <exception>
#include <cstdio>
#include <string>

namespace libvgcode {

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
static EMoveType valueof(Slic3r::EMoveType type)
{
    return static_cast<EMoveType>(static_cast<uint8_t>(type));
}

static EGCodeExtrusionRole valueof(Slic3r::GCodeExtrusionRole role)
{
    return static_cast<EGCodeExtrusionRole>(static_cast<uint8_t>(role));
}

static Vec3f toVec3f(const Eigen::Matrix<float, 3, 1, Eigen::DontAlign>& v)
{
    return { v.x(), v.y(), v.z() };
}
//################################################################################################################################

static const float TRAVEL_RADIUS = 0.05f;
static const float WIPE_RADIUS = 0.05f;

template<class T, class O = T>
using IntegerOnly = std::enable_if_t<std::is_integral<T>::value, O>;

// Rounding up.
// 1.5 is rounded to 2
// 1.49 is rounded to 1
// 0.5 is rounded to 1,
// 0.49 is rounded to 0
// -0.5 is rounded to 0,
// -0.51 is rounded to -1,
// -1.5 is rounded to -1.
// -1.51 is rounded to -2.
// If input is not a valid float (it is infinity NaN or if it does not fit)
// the float to int conversion produces a max int on Intel and +-max int on ARM.
template<typename I>
inline IntegerOnly<I, I> fast_round_up(double a)
{
    // Why does Java Math.round(0.49999999999999994) return 1?
    // https://stackoverflow.com/questions/9902968/why-does-math-round0-49999999999999994-return-1
    return a == 0.49999999999999994 ? I(0) : I(floor(a + 0.5));
}

// Round to a bin with minimum two digits resolution.
// Equivalent to conversion to string with sprintf(buf, "%.2g", value) and conversion back to float, but faster.
static float round_to_bin(const float value)
{
//    assert(value >= 0);
    constexpr float const scale[5]     = { 100.f,  1000.f,  10000.f,  100000.f,  1000000.f };
    constexpr float const invscale[5]  = { 0.01f,  0.001f,  0.0001f,  0.00001f,  0.000001f };
    constexpr float const threshold[5] = { 0.095f, 0.0095f, 0.00095f, 0.000095f, 0.0000095f };
    // Scaling factor, pointer to the tables above.
    int                   i = 0;
    // While the scaling factor is not yet large enough to get two integer digits after scaling and rounding:
    for (; value < threshold[i] && i < 4; ++i);
    // At least on MSVC std::round() calls a complex function, which is pretty expensive.
    // our fast_round_up is much cheaper and it could be inlined.
//    return std::round(value * scale[i]) * invscale[i];
    double a = value * scale[i];
    assert(std::abs(a) < double(std::numeric_limits<int64_t>::max()));
    return fast_round_up<int64_t>(a) * invscale[i];
}

static int hex_digit_to_int(const char c) {
    return (c >= '0' && c <= '9') ? int(c - '0') :
           (c >= 'A' && c <= 'F') ? int(c - 'A') + 10 :
           (c >= 'a' && c <= 'f') ? int(c - 'a') + 10 : -1;
};

bool decode_color(const std::string& color_in, Color& color_out)
{
    constexpr const float INV_255 = 1.0f / 255.0f;

    color_out.fill(0.0f);
    if (color_in.size() == 7 && color_in.front() == '#') {
        const char* c = color_in.data() + 1;
        for (unsigned int i = 0; i < 3; ++i) {
            const int digit1 = hex_digit_to_int(*c++);
            const int digit2 = hex_digit_to_int(*c++);
            if (digit1 != -1 && digit2 != -1)
                color_out[i] = float(digit1 * 16 + digit2) * INV_255;
        }
    }
    else
        return false;

    assert(0.0f <= color_out[0] && color_out[0] <= 1.0f);
    assert(0.0f <= color_out[1] && color_out[1] <= 1.0f);
    assert(0.0f <= color_out[2] && color_out[2] <= 1.0f);
    return true;
}

bool decode_colors(const std::vector<std::string>& colors_in, std::vector<Color>& colors_out)
{
    colors_out = std::vector<Color>(colors_in.size());
    for (size_t i = 0; i < colors_in.size(); ++i) {
        if (!decode_color(colors_in[i], colors_out[i]))
            return false;
    }
    return true;
}

static Mat4x4f inverse(const Mat4x4f& m)
{
    // ref: https://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix

    std::array<float, 16> inv;

    inv[0] = m[5] * m[10] * m[15] -
             m[5] * m[11] * m[14] -
             m[9] * m[6] * m[15] +
             m[9] * m[7] * m[14] +
             m[13] * m[6] * m[11] -
             m[13] * m[7] * m[10];

    inv[4] = -m[4] * m[10] * m[15] +
             m[4] * m[11] * m[14] +
             m[8] * m[6] * m[15] -
             m[8] * m[7] * m[14] -
             m[12] * m[6] * m[11] +
             m[12] * m[7] * m[10];

    inv[8] = m[4] * m[9] * m[15] -
             m[4] * m[11] * m[13] -
             m[8] * m[5] * m[15] +
             m[8] * m[7] * m[13] +
             m[12] * m[5] * m[11] -
             m[12] * m[7] * m[9];

    inv[12] = -m[4] * m[9] * m[14] +
               m[4] * m[10] * m[13] +
               m[8] * m[5] * m[14] -
               m[8] * m[6] * m[13] -
               m[12] * m[5] * m[10] +
               m[12] * m[6] * m[9];

    inv[1] = -m[1] * m[10] * m[15] +
             m[1] * m[11] * m[14] +
             m[9] * m[2] * m[15] -
             m[9] * m[3] * m[14] -
             m[13] * m[2] * m[11] +
             m[13] * m[3] * m[10];

    inv[5] = m[0] * m[10] * m[15] -
             m[0] * m[11] * m[14] -
             m[8] * m[2] * m[15] +
             m[8] * m[3] * m[14] +
             m[12] * m[2] * m[11] -
             m[12] * m[3] * m[10];

    inv[9] = -m[0] * m[9] * m[15] +
             m[0] * m[11] * m[13] +
             m[8] * m[1] * m[15] -
             m[8] * m[3] * m[13] -
             m[12] * m[1] * m[11] +
             m[12] * m[3] * m[9];

    inv[13] = m[0] * m[9] * m[14] -
             m[0] * m[10] * m[13] -
             m[8] * m[1] * m[14] +
             m[8] * m[2] * m[13] +
             m[12] * m[1] * m[10] -
             m[12] * m[2] * m[9];

    inv[2] = m[1] * m[6] * m[15] -
             m[1] * m[7] * m[14] -
             m[5] * m[2] * m[15] +
             m[5] * m[3] * m[14] +
             m[13] * m[2] * m[7] -
             m[13] * m[3] * m[6];

    inv[6] = -m[0] * m[6] * m[15] +
             m[0] * m[7] * m[14] +
             m[4] * m[2] * m[15] -
             m[4] * m[3] * m[14] -
             m[12] * m[2] * m[7] +
             m[12] * m[3] * m[6];

    inv[10] = m[0] * m[5] * m[15] -
              m[0] * m[7] * m[13] -
              m[4] * m[1] * m[15] +
              m[4] * m[3] * m[13] +
              m[12] * m[1] * m[7] -
              m[12] * m[3] * m[5];

    inv[14] = -m[0] * m[5] * m[14] +
              m[0] * m[6] * m[13] +
              m[4] * m[1] * m[14] -
              m[4] * m[2] * m[13] -
              m[12] * m[1] * m[6] +
              m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] +
             m[1] * m[7] * m[10] +
             m[5] * m[2] * m[11] -
             m[5] * m[3] * m[10] -
             m[9] * m[2] * m[7] +
             m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] -
             m[0] * m[7] * m[10] -
             m[4] * m[2] * m[11] +
             m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] -
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] +
             m[0] * m[7] * m[9] +
             m[4] * m[1] * m[11] -
             m[4] * m[3] * m[9] -
             m[8] * m[1] * m[7] +
             m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] -
              m[0] * m[6] * m[9] -
              m[4] * m[1] * m[10] +
              m[4] * m[2] * m[9] +
              m[8] * m[1] * m[6] -
              m[8] * m[2] * m[5];

    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    assert(det != 0.0f);

    det = 1.0 / det;

    std::array<float, 16> ret = {};
    for (int i = 0; i < 16; ++i) {
        ret[i] = inv[i] * det;
    }

    return ret;
}

static std::string short_time(const std::string& time)
{
    // Parse the dhms time format.
    int days = 0;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    if (time.find('d') != std::string::npos)
        sscanf(time.c_str(), "%dd %dh %dm %ds", &days, &hours, &minutes, &seconds);
    else if (time.find('h') != std::string::npos)
        sscanf(time.c_str(), "%dh %dm %ds", &hours, &minutes, &seconds);
    else if (time.find('m') != std::string::npos)
        sscanf(time.c_str(), "%dm %ds", &minutes, &seconds);
    else if (time.find('s') != std::string::npos)
        sscanf(time.c_str(), "%ds", &seconds);

    // Round to full minutes.
    if (days + hours + minutes > 0 && seconds >= 30) {
        if (++minutes == 60) {
            minutes = 0;
            if (++hours == 24) {
                hours = 0;
                ++days;
            }
        }
    }

    // Format the dhm time
    char buffer[64];
    if (days > 0)
        sprintf(buffer, "%dd%dh%dm", days, hours, minutes);
    else if (hours > 0)
        sprintf(buffer, "%dh%dm", hours, minutes);
    else if (minutes > 0)
        sprintf(buffer, "%dm", minutes);
    else
        sprintf(buffer, "%ds", seconds);
    return buffer;
}

// Returns the given time is seconds in format DDd HHh MMm SSs
static std::string get_time_dhms(float time_in_secs)
{
    int days = (int)(time_in_secs / 86400.0f);
    time_in_secs -= (float)days * 86400.0f;
    int hours = (int)(time_in_secs / 3600.0f);
    time_in_secs -= (float)hours * 3600.0f;
    int minutes = (int)(time_in_secs / 60.0f);
    time_in_secs -= (float)minutes * 60.0f;

    char buffer[64];
    if (days > 0)
        sprintf(buffer, "%dd %dh %dm %ds", days, hours, minutes, (int)time_in_secs);
    else if (hours > 0)
        sprintf(buffer, "%dh %dm %ds", hours, minutes, (int)time_in_secs);
    else if (minutes > 0)
        sprintf(buffer, "%dm %ds", minutes, (int)time_in_secs);
    else
        sprintf(buffer, "%ds", (int)std::round(time_in_secs));

    return buffer;
}

std::string check_shader(GLuint handle)
{
    std::string ret;
    GLint params;
    glsafe(glGetShaderiv(handle, GL_COMPILE_STATUS, &params));
    if (params == GL_FALSE) {
        glsafe(glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &params));
        ret.resize(params);
        glsafe(glGetShaderInfoLog(handle, params, &params, ret.data()));
    }
    return ret;
}

std::string check_program(GLuint handle)
{
    std::string ret;
    GLint params;
    glsafe(glGetProgramiv(handle, GL_LINK_STATUS, &params));
    if (params == GL_FALSE) {
        glsafe(glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &params));
        ret.resize(params);
        glsafe(glGetProgramInfoLog(handle, params, &params, ret.data()));
    }
    return ret;
}

unsigned int init_shader(const std::string& shader_name, const char* vertex_shader, const char* fragment_shader)
{
    const GLuint vs_id = glCreateShader(GL_VERTEX_SHADER);
    glcheck();
    glsafe(glShaderSource(vs_id, 1, &vertex_shader, nullptr));
    glsafe(glCompileShader(vs_id));
    std::string res = check_shader(vs_id);
    if (!res.empty()) {
        glsafe(glDeleteShader(vs_id));
        throw std::runtime_error("LibVGCode: Unable to compile vertex shader:\n" + shader_name + "\n" + res + "\n");
    }

    const GLuint fs_id = glCreateShader(GL_FRAGMENT_SHADER);
    glcheck();
    glsafe(glShaderSource(fs_id, 1, &fragment_shader, nullptr));
    glsafe(glCompileShader(fs_id));
    res = check_shader(fs_id);
    if (!res.empty()) {
        glsafe(glDeleteShader(vs_id));
        glsafe(glDeleteShader(fs_id));
        throw std::runtime_error("LibVGCode: Unable to compile fragment shader:\n" + shader_name + "\n" + res + "\n");
    }

    const GLuint shader_id = glCreateProgram();
    glcheck();
    glsafe(glAttachShader(shader_id, vs_id));
    glsafe(glAttachShader(shader_id, fs_id));
    glsafe(glLinkProgram(shader_id));
    res = check_program(shader_id);
    if (!res.empty()) {
        glsafe(glDetachShader(shader_id, vs_id));
        glsafe(glDetachShader(shader_id, fs_id));
        glsafe(glDeleteShader(vs_id));
        glsafe(glDeleteShader(fs_id));
        glsafe(glDeleteProgram(shader_id));
        throw std::runtime_error("LibVGCode: Unable to link shader program:\n" + shader_name + "\n" + res + "\n");
    }

    glsafe(glDetachShader(shader_id, vs_id));
    glsafe(glDetachShader(shader_id, fs_id));
    glsafe(glDeleteShader(vs_id));
    glsafe(glDeleteShader(fs_id));
    return shader_id;
}

// mapping from EMoveType to EOptionType
static EOptionType type_to_option(EMoveType type) {
    switch (type)
    {
    case EMoveType::Retract:     { return EOptionType::Retractions; }
    case EMoveType::Unretract:   { return EOptionType::Unretractions; }
    case EMoveType::Seam:        { return EOptionType::Seams; }
    case EMoveType::ToolChange:  { return EOptionType::ToolChanges; }
    case EMoveType::ColorChange: { return EOptionType::ColorChanges; }
    case EMoveType::PausePrint:  { return EOptionType::PausePrints; }
    case EMoveType::CustomGCode: { return EOptionType::CustomGCodes; }
    default:                     { return EOptionType::COUNT; }
    }
}

ViewerImpl::~ViewerImpl()
{
    reset();
    if (m_options_shader_id != 0)
        glDeleteProgram(m_options_shader_id);
    if (m_segments_shader_id != 0)
        glDeleteProgram(m_segments_shader_id);
}

void ViewerImpl::init()
{
    if (m_segments_shader_id != 0)
        return;

    // segments shader
    m_segments_shader_id = init_shader("segments", Segments_Vertex_Shader, Segments_Fragment_Shader);

    m_uni_segments_view_matrix_id            = glGetUniformLocation(m_segments_shader_id, "view_matrix");
    m_uni_segments_projection_matrix_id      = glGetUniformLocation(m_segments_shader_id, "projection_matrix");
    m_uni_segments_camera_position_id        = glGetUniformLocation(m_segments_shader_id, "camera_position");
    m_uni_segments_positions_tex_id          = glGetUniformLocation(m_segments_shader_id, "positionsTex");
    m_uni_segments_height_width_angle_tex_id = glGetUniformLocation(m_segments_shader_id, "heightWidthAngleTex");
    m_uni_segments_colors_tex_id             = glGetUniformLocation(m_segments_shader_id, "colorsTex");
    m_uni_segments_segment_index_tex_id      = glGetUniformLocation(m_segments_shader_id, "segmentIndexTex");
    glcheck();
    assert(m_uni_segments_view_matrix_id != -1 &&
           m_uni_segments_projection_matrix_id != -1 &&
           m_uni_segments_camera_position_id != -1 &&
           m_uni_segments_positions_tex_id != -1 &&
           m_uni_segments_height_width_angle_tex_id != -1 &&
           m_uni_segments_colors_tex_id != -1 &&
           m_uni_segments_segment_index_tex_id != -1);

    m_segment_template.init();

    // options shader
    m_options_shader_id = init_shader("options", Options_Vertex_Shader, Options_Fragment_Shader);

    m_uni_options_view_matrix_id            = glGetUniformLocation(m_options_shader_id, "view_matrix");
    m_uni_options_projection_matrix_id      = glGetUniformLocation(m_options_shader_id, "projection_matrix");
    m_uni_options_positions_tex_id          = glGetUniformLocation(m_options_shader_id, "positionsTex");
    m_uni_options_height_width_angle_tex_id = glGetUniformLocation(m_options_shader_id, "heightWidthAngleTex");
    m_uni_options_colors_tex_id             = glGetUniformLocation(m_options_shader_id, "colorsTex");
    m_uni_options_segment_index_tex_id      = glGetUniformLocation(m_options_shader_id, "segmentIndexTex");
    glcheck();
    assert(m_uni_options_view_matrix_id != -1 &&
           m_uni_options_projection_matrix_id != -1 &&
           m_uni_options_positions_tex_id != -1 &&
           m_uni_options_height_width_angle_tex_id != -1 &&
           m_uni_options_colors_tex_id != -1 &&
           m_uni_options_segment_index_tex_id != -1);

    m_option_template.init(16);

#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
    // cog marker shader
    m_cog_marker_shader_id = init_shader("cog_marker", Cog_Marker_Vertex_Shader, Cog_Marker_Fragment_Shader);

    m_uni_cog_marker_world_center_position = glGetUniformLocation(m_cog_marker_shader_id, "world_center_position");
    m_uni_cog_marker_scale_factor          = glGetUniformLocation(m_cog_marker_shader_id, "scale_factor");
    m_uni_cog_marker_view_matrix           = glGetUniformLocation(m_cog_marker_shader_id, "view_matrix");
    m_uni_cog_marker_projection_matrix     = glGetUniformLocation(m_cog_marker_shader_id, "projection_matrix");
    glcheck();
    assert(m_uni_cog_marker_world_center_position != -1 &&
           m_uni_cog_marker_scale_factor != -1 &&
           m_uni_cog_marker_view_matrix != -1 &&
           m_uni_cog_marker_projection_matrix != -1);

    m_cog_marker.init(32, 1.0f);

    // tool marker shader
    m_tool_marker_shader_id = init_shader("tool_marker", Tool_Marker_Vertex_Shader, Tool_Marker_Fragment_Shader);

    m_uni_tool_marker_world_origin      = glGetUniformLocation(m_tool_marker_shader_id, "world_origin");
    m_uni_tool_marker_scale_factor      = glGetUniformLocation(m_tool_marker_shader_id, "scale_factor");
    m_uni_tool_marker_view_matrix       = glGetUniformLocation(m_tool_marker_shader_id, "view_matrix");
    m_uni_tool_marker_projection_matrix = glGetUniformLocation(m_tool_marker_shader_id, "projection_matrix");
    m_uni_tool_marker_color_base        = glGetUniformLocation(m_tool_marker_shader_id, "color_base");

    glcheck();
    assert(m_uni_tool_marker_world_origin != -1 &&
           m_uni_tool_marker_scale_factor != -1 &&
           m_uni_tool_marker_view_matrix != -1 &&
           m_uni_tool_marker_projection_matrix != -1 &&
           m_uni_tool_marker_color_base != -1);

    m_tool_marker.init(32, 2.0f, 4.0f, 1.0f, 8.0f);
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
}

void ViewerImpl::load(const Slic3r::GCodeProcessorResult& gcode_result, const std::vector<std::string>& str_tool_colors)
{
    if (m_settings.time_mode != ETimeMode::Normal) {
        const Slic3r::PrintEstimatedStatistics& stats = gcode_result.print_statistics;
        bool force_normal_mode = static_cast<size_t>(m_settings.time_mode) >= stats.modes.size();
        if (!force_normal_mode) {
            const float normal_time = stats.modes[static_cast<uint8_t>(ETimeMode::Normal)].time;
            const float mode_time = stats.modes[static_cast<uint8_t>(m_settings.time_mode)].time;
            force_normal_mode = mode_time == 0.0f ||
                                short_time(get_time_dhms(mode_time)) == short_time(get_time_dhms(normal_time)); // TO CHECK -> Is this necessary ?
        }
        if (force_normal_mode)
            m_settings.time_mode = ETimeMode::Normal;
    }

    m_tool_colors.clear();
    if (m_settings.view_type == EViewType::Tool && !gcode_result.extruder_colors.empty())
        // update tool colors from config stored in the gcode
        decode_colors(gcode_result.extruder_colors, m_tool_colors);
    else
        // update tool colors
        decode_colors(str_tool_colors, m_tool_colors);

    // ensure there are enough colors defined
    while (m_tool_colors.size() < std::max<size_t>(1, gcode_result.extruders_count)) {
        m_tool_colors.push_back(Default_Tool_Color);
    }

    static unsigned int last_result_id = 0;
    if (last_result_id == gcode_result.id)
        return;

    last_result_id = gcode_result.id;

#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
    m_cog_marker.reset();
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS

    reset();

    m_vertices_map.reserve(2 * gcode_result.moves.size());
    m_vertices.reserve(2 * gcode_result.moves.size());
    uint32_t seams_count = 0;
    for (size_t i = 1; i < gcode_result.moves.size(); ++i) {
        const Slic3r::GCodeProcessorResult::MoveVertex& curr = gcode_result.moves[i];
        const Slic3r::GCodeProcessorResult::MoveVertex& prev = gcode_result.moves[i - 1];
        const EMoveType curr_type = valueof(curr.type);
        const EGCodeExtrusionRole curr_role = valueof(curr.extrusion_role);

        if (curr_type == EMoveType::Seam)
           ++seams_count;

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
            extrusion_role = static_cast<EGCodeExtrusionRole>(curr.extrusion_role);

        float width;
        float height;
        switch (curr_type)
        {
        case EMoveType::Travel: { width = TRAVEL_RADIUS; height = TRAVEL_RADIUS; break; }
        case EMoveType::Wipe:   { width = WIPE_RADIUS;   height = WIPE_RADIUS;   break; }
        default:                { width = curr.width;    height = curr.height;   break; }
        }

        if (type_to_option(curr_type) == EOptionType::COUNT) {
            if (m_vertices.empty() || prev.type != curr.type) {
                // to be able to properly detect the start/end of a path we add a 'phantom' vertex equal to the current one with
                // the exception of the position
                const PathVertex vertex = { toVec3f(prev.position), height, width, curr.feedrate, curr.fan_speed,
                    curr.temperature, curr.volumetric_rate(), extrusion_role, curr_type,
                    static_cast<uint8_t>(curr.extruder_id), static_cast<uint8_t>(curr.cp_color_id),
                    static_cast<uint32_t>(curr.layer_id) };
                m_vertices_map.emplace_back(static_cast<uint32_t>(i) - seams_count);
                m_vertices.emplace_back(vertex);
                m_layers.update(static_cast<uint32_t>(curr.layer_id), static_cast<uint32_t>(m_vertices.size()));
            }
        }

        const PathVertex vertex = { toVec3f(curr.position), height, width, curr.feedrate, curr.fan_speed, curr.temperature,
            curr.volumetric_rate(), extrusion_role, curr_type, static_cast<uint8_t>(curr.extruder_id),
            static_cast<uint8_t>(curr.cp_color_id), static_cast<uint32_t>(curr.layer_id) };
        m_vertices_map.emplace_back(static_cast<uint32_t>(i) - seams_count);
        m_vertices.emplace_back(vertex);
        m_layers.update(static_cast<uint32_t>(curr.layer_id), static_cast<uint32_t>(m_vertices.size()));

#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
        // updates calculation for center of gravity
        if (curr_type == EMoveType::Extrude &&
            curr_role != EGCodeExtrusionRole::Skirt &&
            curr_role != EGCodeExtrusionRole::SupportMaterial &&
            curr_role != EGCodeExtrusionRole::SupportMaterialInterface &&
            curr_role != EGCodeExtrusionRole::WipeTower &&
            curr_role != EGCodeExtrusionRole::Custom) {
            const Vec3f curr_pos = toVec3f(curr.position);
            const Vec3f prev_pos = toVec3f(prev.position);
            m_cog_marker.update(0.5f * (curr_pos + prev_pos), curr.mm3_per_mm * length(curr_pos - prev_pos));
        }
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
    }
    m_vertices_map.shrink_to_fit();
    m_vertices.shrink_to_fit();

    assert(m_vertices_map.size() == m_vertices.size());

    m_valid_lines_bitset = BitSet<>(m_vertices.size());
    m_valid_lines_bitset.setAll();

    static constexpr const Vec3f ZERO = { 0.0f, 0.0f, 0.0f };

    // buffers to send to gpu
    std::vector<Vec3f> positions;
    std::vector<Vec3f> heights_widths_angles;
    positions.reserve(m_vertices.size());
    heights_widths_angles.reserve(m_vertices.size());
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        const PathVertex& v = m_vertices[i];
        const EMoveType move_type = v.type;
        const bool prev_line_valid = i > 0 && m_valid_lines_bitset[i - 1];
        const Vec3f prev_line = prev_line_valid ? v.position - m_vertices[i - 1].position : ZERO;
        const bool this_line_valid = i + 1 < m_vertices.size() &&
                                     m_vertices[i + 1].position != v.position &&
                                     m_vertices[i + 1].type == move_type &&
                                     move_type != EMoveType::Seam;
        const Vec3f this_line = this_line_valid ? m_vertices[i + 1].position - v.position : ZERO;

        if (this_line_valid) {
            // there is a valid path between point i and i+1.
        }
        else {
            // the connection is invalid, there should be no line rendered, ever
            m_valid_lines_bitset.reset(i);
        }

        Vec3f position = v.position;
        if (move_type == EMoveType::Extrude)
            // push down extrusion vertices by half height to render them at the right z
            position[2] -= 0.5 * v.height;
        positions.emplace_back(position);

        const float angle = atan2(prev_line[0] * this_line[1] - prev_line[1] * this_line[0], dot(prev_line, this_line));
        heights_widths_angles.push_back({ v.height, v.width, angle });
    }

    if (!positions.empty()) {
        int old_bound_texture = 0;
        glsafe(glGetIntegerv(GL_TEXTURE_BINDING_BUFFER, &old_bound_texture));

        // create and fill positions buffer
        glsafe(glGenBuffers(1, &m_positions_buf_id));
        glsafe(glBindBuffer(GL_TEXTURE_BUFFER, m_positions_buf_id));
        glsafe(glBufferData(GL_TEXTURE_BUFFER, positions.size() * sizeof(Vec3f), positions.data(), GL_STATIC_DRAW));
        glsafe(glGenTextures(1, &m_positions_tex_id));
        glsafe(glBindTexture(GL_TEXTURE_BUFFER, m_positions_tex_id));

        // create and fill height, width and angles buffer
        glsafe(glGenBuffers(1, &m_heights_widths_angles_buf_id));
        glsafe(glBindBuffer(GL_TEXTURE_BUFFER, m_heights_widths_angles_buf_id));
        glsafe(glBufferData(GL_TEXTURE_BUFFER, heights_widths_angles.size() * sizeof(Vec3f), heights_widths_angles.data(), GL_STATIC_DRAW));
        glsafe(glGenTextures(1, &m_heights_widths_angles_tex_id));
        glsafe(glBindTexture(GL_TEXTURE_BUFFER, m_heights_widths_angles_tex_id));

        // create (but do not fill) colors buffer (data is set in update_colors())
        glsafe(glGenBuffers(1, &m_colors_buf_id));
        glsafe(glBindBuffer(GL_TEXTURE_BUFFER, m_colors_buf_id));
        glsafe(glGenTextures(1, &m_colors_tex_id));
        glsafe(glBindTexture(GL_TEXTURE_BUFFER, m_colors_tex_id));

        // create (but do not fill) enabled segments buffer (data is set in update_enabled_entities())
        glsafe(glGenBuffers(1, &m_enabled_segments_buf_id));
        glsafe(glBindBuffer(GL_TEXTURE_BUFFER, m_enabled_segments_buf_id));
        glsafe(glGenTextures(1, &m_enabled_segments_tex_id));
        glsafe(glBindTexture(GL_TEXTURE_BUFFER, m_enabled_segments_tex_id));

        // create (but do not fill) enabled options buffer (data is set in update_enabled_entities())
        glsafe(glGenBuffers(1, &m_enabled_options_buf_id));
        glsafe(glBindBuffer(GL_TEXTURE_BUFFER, m_enabled_options_buf_id));
        glsafe(glGenTextures(1, &m_enabled_options_tex_id));
        glsafe(glBindTexture(GL_TEXTURE_BUFFER, m_enabled_options_tex_id));

        glsafe(glBindBuffer(GL_TEXTURE_BUFFER, 0));
        glsafe(glBindTexture(GL_TEXTURE_BUFFER, old_bound_texture));
    }

    for (uint8_t i = 0; i < static_cast<uint8_t>(ETimeMode::COUNT); ++i) {
        if (i < gcode_result.print_statistics.modes.size())
            m_layers_times[i] = gcode_result.print_statistics.modes[i].layers_times;
    }

    if (!m_layers.empty())
        set_layers_range(0, static_cast<uint32_t>(m_layers.size() - 1));
    update_view_global_range();
    m_settings.update_colors = true;
}

void ViewerImpl::update_enabled_entities()
{
    std::vector<uint32_t> enabled_segments;
    std::vector<uint32_t> enabled_options;
    std::array<uint32_t, 2> range = m_view_range.get_current_range();

    // when top layer only visualization is enabled, we need to render
    // all the toolpaths in the other layers as grayed, so extend the range
    // to contain them
    if (m_settings.top_layer_only_view)
        range[0] = m_view_range.get_global_range()[0];

    // to show the options at the current tool marker position we need to extend the range by one extra step
    if (m_vertices[range[1]].is_option())
        ++range[1];

    for (uint32_t i = range[0]; i < range[1]; ++i) {
        const PathVertex& v = m_vertices[i];

        if (!m_valid_lines_bitset[i] && !v.is_option())
            continue;
        if (v.is_travel()) {
            if (!m_settings.options_visibility.at(EOptionType::Travels))
                continue;
        }
        else if (v.is_wipe()) {
            if (!m_settings.options_visibility.at(EOptionType::Wipes))
                continue;
        }
        else if (v.is_option()) {
            if (!m_settings.options_visibility.at(type_to_option(v.type)))
                continue;
        }
        else if (v.is_extrusion()) {
            if (!m_settings.extrusion_roles_visibility.at(v.role))
                continue;
        }
        else
            continue;

        if (v.is_option())
            enabled_options.push_back(i);
        else
            enabled_segments.push_back(i);
    }

    m_enabled_segments_count = enabled_segments.size();
    m_enabled_options_count = enabled_options.size();

#if ENABLE_NEW_GCODE_VIEWER_DEBUG
    m_enabled_segments_range = (m_enabled_segments_count > 0) ?
        std::make_pair((uint32_t)enabled_segments.front(), (uint32_t)enabled_segments.back()) : std::make_pair((uint32_t)0, (uint32_t)0);
    m_enabled_options_range = (m_enabled_options_count > 0) ?
        std::make_pair((uint32_t)enabled_options.front(), (uint32_t)enabled_options.back()) : std::make_pair((uint32_t)0, (uint32_t)0);
#endif // ENABLE_NEW_GCODE_VIEWER_DEBUG

    // update gpu buffer for enabled segments
    assert(m_enabled_segments_buf_id > 0);
    glsafe(glBindBuffer(GL_TEXTURE_BUFFER, m_enabled_segments_buf_id));
    if (!enabled_segments.empty())
        glsafe(glBufferData(GL_TEXTURE_BUFFER, enabled_segments.size() * sizeof(uint32_t), enabled_segments.data(), GL_STATIC_DRAW));
    else
        glsafe(glBufferData(GL_TEXTURE_BUFFER, 0, nullptr, GL_STATIC_DRAW));

    // update gpu buffer for enabled options
    assert(m_enabled_options_buf_id > 0);
    glsafe(glBindBuffer(GL_TEXTURE_BUFFER, m_enabled_options_buf_id));
    if (!enabled_options.empty())
        glsafe(glBufferData(GL_TEXTURE_BUFFER, enabled_options.size() * sizeof(uint32_t), enabled_options.data(), GL_STATIC_DRAW));
    else
        glsafe(glBufferData(GL_TEXTURE_BUFFER, 0, nullptr, GL_STATIC_DRAW));

    glsafe(glBindBuffer(GL_TEXTURE_BUFFER, 0));
}

static float encode_color(const Color& color) {
    const int r = (int)(255.0f * color[0]);
    const int g = (int)(255.0f * color[1]);
    const int b = (int)(255.0f * color[2]);
    const int i_color = r << 16 | g << 8 | b;
    return float(i_color);
}

void ViewerImpl::update_colors()
{
    update_color_ranges();

    const uint32_t top_layer_id = m_settings.top_layer_only_view ? m_layers_range.get()[1] : 0;
    std::vector<float> colors(m_vertices.size());
    for (size_t i = 0; i < m_vertices.size(); i++) {
        colors[i] = (m_vertices[i].layer_id < top_layer_id) ?
            encode_color(Dummy_Color) : encode_color(select_color(m_vertices[i]));
    }

    // update gpu buffer for colors
    assert(m_colors_buf_id > 0);
    glsafe(glBindBuffer(GL_TEXTURE_BUFFER, m_colors_buf_id));
    glsafe(glBufferData(GL_TEXTURE_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW));
    glsafe(glBindBuffer(GL_TEXTURE_BUFFER, 0));
}

void ViewerImpl::render(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix)
{
    if (m_settings.update_view_global_range) {
        update_view_global_range();
        m_settings.update_view_global_range = false;
    }

    if (m_settings.update_enabled_entities) {
        update_enabled_entities();
        m_settings.update_enabled_entities = false;
    }

    if (m_settings.update_colors) {
        update_colors();
        m_settings.update_colors = false;
    }

    const Mat4x4f inv_view_matrix = inverse(view_matrix);
    const Vec3f camera_position = { inv_view_matrix[12], inv_view_matrix[13], inv_view_matrix[14] };

    render_segments(view_matrix, projection_matrix, camera_position);
    render_options(view_matrix, projection_matrix);
#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
    if (m_settings.options_visibility.at(EOptionType::ToolMarker))
        render_tool_marker(view_matrix, projection_matrix);
    if (m_settings.options_visibility.at(EOptionType::CenterOfGravity))
        render_cog_marker(view_matrix, projection_matrix);
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS

#if ENABLE_NEW_GCODE_VIEWER_DEBUG
    render_debug_window();
#endif // ENABLE_NEW_GCODE_VIEWER_DEBUG
}

EViewType ViewerImpl::get_view_type() const
{
    return m_settings.view_type;
}

ETimeMode ViewerImpl::get_time_mode() const
{
    return m_settings.time_mode;
}

const std::array<uint32_t, 2>& ViewerImpl::get_layers_range() const
{
    return m_layers_range.get();
}

bool ViewerImpl::is_top_layer_only_view() const
{
    return m_settings.top_layer_only_view;
}

bool ViewerImpl::is_option_visible(EOptionType type) const
{
    try
    {
        return m_settings.options_visibility.at(type);
    }
    catch (...)
    {
        return false;
    }
}

bool ViewerImpl::is_extrusion_role_visible(EGCodeExtrusionRole role) const
{
    try
    {
        return m_settings.extrusion_roles_visibility.at(role);
    }
    catch (...)
    {
        return false;
    }
}

void ViewerImpl::set_view_type(EViewType type)
{
    m_settings.view_type = type;
    m_settings.update_colors = true;
}

void ViewerImpl::set_time_mode(ETimeMode mode)
{
    m_settings.time_mode = mode;
    m_settings.update_colors = true;
}

void ViewerImpl::set_layers_range(const std::array<uint32_t, 2>& range)
{
    set_layers_range(range[0], range[1]);
}

void ViewerImpl::set_layers_range(uint32_t min, uint32_t max)
{
    m_layers_range.set(min, max);
    m_settings.update_view_global_range = true;
    m_settings.update_enabled_entities = true;
    m_settings.update_colors = true;
}

void ViewerImpl::set_top_layer_only_view(bool top_layer_only_view)
{
    m_settings.top_layer_only_view = top_layer_only_view;
    m_settings.update_colors = true;
}

void ViewerImpl::toggle_option_visibility(EOptionType type)
{
    try
    {
        bool& value = m_settings.options_visibility.at(type);
        value = !value;
        if (type == EOptionType::Travels)
            m_settings.update_view_global_range = true;
        m_settings.update_enabled_entities = true;
        m_settings.update_colors = true;
    }
    catch (...)
    {
        // do nothing;
    }
}

void ViewerImpl::toggle_extrusion_role_visibility(EGCodeExtrusionRole role)
{
    try
    {
        bool& value = m_settings.extrusion_roles_visibility.at(role);
        value = !value;
        m_settings.update_enabled_entities = true;
        m_settings.update_colors = true;
    }
    catch (...)
    {
        // do nothing;
    }
}

const std::array<uint32_t, 2>& ViewerImpl::get_view_current_range() const
{
    return m_view_range.get_current_range();
}

const std::array<uint32_t, 2>& ViewerImpl::get_view_global_range() const
{
    return m_view_range.get_global_range();
}

void ViewerImpl::set_view_current_range(uint32_t min, uint32_t max)
{
    uint32_t min_id = 0;
    for (size_t i = 0; i < m_vertices_map.size(); ++i) {
        if (m_vertices_map[i] < min)
            min_id = static_cast<uint32_t>(i);
        else
            break;
    }
    ++min_id;

    uint32_t max_id = min_id;
    if (max > min) {
        for (size_t i = static_cast<size_t>(min_id); i < m_vertices_map.size(); ++i) {
            if (m_vertices_map[i] < max)
                max_id = static_cast<uint32_t>(i);
            else
                break;
        }
        ++max_id;
    }

    // adjust the max id to take in account the 'phantom' vertices added in load()
    if (max_id < static_cast<uint32_t>(m_vertices_map.size() - 1) &&
        m_vertices[max_id + 1].type == m_vertices[max_id].type &&
        m_vertices_map[max_id + 1] == m_vertices_map[max_id])
        ++max_id;

    // we show the seams when the endpoint of a closed path is reached, so we need to increase the max id by one
    if (max_id < static_cast<uint32_t>(m_vertices.size() - 1) && m_vertices[max_id + 1].type == EMoveType::Seam)
        ++max_id;

    Range new_range;
    new_range.set(min_id, max_id);

    if (m_old_current_range != new_range) {
        m_view_range.set_current_range(new_range);
        m_old_current_range = new_range;
        m_settings.update_enabled_entities = true;
    }
}

uint32_t ViewerImpl::get_vertices_count() const
{
    return static_cast<uint32_t>(m_vertices.size());
}

PathVertex ViewerImpl::get_current_vertex() const
{
    return m_vertices[m_view_range.get_current_range()[1]];
}

PathVertex ViewerImpl::get_vertex_at(uint32_t id) const
{
    return (id < static_cast<uint32_t>(m_vertices.size())) ? m_vertices[id] : PathVertex();
}

const std::array<std::vector<float>, static_cast<size_t>(ETimeMode::COUNT)>& ViewerImpl::get_layers_times() const
{
    return m_layers_times;
}

#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
Vec3f ViewerImpl::get_cog_marker_position() const
{
    return m_cog_marker.get_position();
}

float ViewerImpl::get_cog_marker_scale_factor() const
{
    return m_cog_marker_scale_factor;
}

bool ViewerImpl::is_tool_marker_enabled() const
{
    return m_tool_marker.is_enabled();
}

const Vec3f& ViewerImpl::get_tool_marker_position() const
{
    return m_tool_marker.get_position();
}

float ViewerImpl::get_tool_marker_offset_z() const
{
    return m_tool_marker.get_offset_z();
}

float ViewerImpl::get_tool_marker_scale_factor() const
{
    return m_tool_marker_scale_factor;
}

const Color& ViewerImpl::get_tool_marker_color() const
{
    return m_tool_marker.get_color();
}

float ViewerImpl::get_tool_marker_alpha() const
{
    return m_tool_marker.get_alpha();
}

void ViewerImpl::set_cog_marker_scale_factor(float factor)
{
    m_cog_marker_scale_factor = std::max(factor, 0.001f);
}

void ViewerImpl::enable_tool_marker(bool value)
{
    m_tool_marker.enable(value);
}

void ViewerImpl::set_tool_marker_position(const Vec3f& position)
{
    m_tool_marker.set_position(position);
}

void ViewerImpl::set_tool_marker_offset_z(float offset_z)
{
    m_tool_marker.set_offset_z(offset_z);
}

void ViewerImpl::set_tool_marker_scale_factor(float factor)
{
    m_tool_marker_scale_factor = std::max(factor, 0.001f);
}

void ViewerImpl::set_tool_marker_color(const Color& color)
{
    m_tool_marker.set_color(color);
}

void ViewerImpl::set_tool_marker_alpha(float alpha)
{
    m_tool_marker.set_alpha(alpha);
}
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS

static void delete_textures(unsigned int& id)
{
    if (id != 0) {
        glsafe(glDeleteTextures(1, &id));
        id = 0;
    }
}

static void delete_buffers(unsigned int& id)
{
    if (id != 0) {
        glsafe(glDeleteBuffers(1, &id));
        id = 0;
    }
}

void ViewerImpl::reset()
{
    m_layers.reset();
    m_layers_range.reset();
    m_view_range.reset();
    m_old_current_range.reset();
    m_vertices.clear();
    m_vertices_map.clear();
    m_valid_lines_bitset.clear();

    m_layers_times = std::array<std::vector<float>, (uint8_t)ETimeMode::COUNT>();

    delete_textures(m_enabled_options_tex_id);
    delete_buffers(m_enabled_options_buf_id);

    delete_textures(m_enabled_segments_tex_id);
    delete_buffers(m_enabled_segments_buf_id);

    delete_textures(m_colors_tex_id);
    delete_buffers(m_colors_buf_id);

    delete_textures(m_heights_widths_angles_tex_id);
    delete_buffers(m_heights_widths_angles_buf_id);

    delete_textures(m_positions_tex_id);
    delete_buffers(m_positions_buf_id);
}

void ViewerImpl::update_view_global_range()
{
    const std::array<uint32_t, 2>& layers_range = m_layers_range.get();
    const bool travels_visible = m_settings.options_visibility.at(EOptionType::Travels);

    auto first_it = m_vertices.begin();
    while (first_it != m_vertices.end() && (
           first_it->layer_id < layers_range[0] ||
           first_it->is_option() ||
           (first_it->is_travel() && !travels_visible))) {
        ++first_it;
    }

    if (first_it == m_vertices.end())
        m_view_range.set_global_range(0, 0);
    else {
        if (travels_visible) {
            // if the global range starts with a travel move, extend it to the travel start
            while (first_it != m_vertices.begin() && first_it->is_travel()) {
                --first_it;
            }
        }

        auto last_it = first_it;
        while (last_it != m_vertices.end() && last_it->layer_id <= layers_range[1]) {
            ++last_it;
        }

        // remove trailing options, if any
        auto rev_first_it = std::make_reverse_iterator(first_it);
        auto rev_last_it = std::make_reverse_iterator(last_it);
        while (rev_last_it != rev_first_it && rev_last_it->is_option()) {
            ++rev_last_it;
        }

        last_it = rev_last_it.base();
        if (travels_visible) {
            // if the global range ends with a travel move, extend it to the travel end
            while (last_it != m_vertices.end() && last_it->is_travel()) {
                ++last_it;
            }
        }

        m_view_range.set_global_range(std::distance(m_vertices.begin(), first_it), std::distance(m_vertices.begin(), last_it));
    }
}

void ViewerImpl::update_color_ranges()
{
    m_width_range.reset();
    m_height_range.reset();
    m_speed_range.reset();
    m_fan_speed_range.reset();
    m_temperature_range.reset();
    m_volumetric_rate_range.reset();
    m_layer_time_range[0].reset(); // ColorRange::EType::Linear
    m_layer_time_range[1].reset(); // ColorRange::EType::Logarithmic

    for (size_t i = 0; i < m_vertices.size(); i++) {
        const PathVertex& v = m_vertices[i];
        if (v.is_extrusion()) {
            m_height_range.update(round_to_bin(v.height));
            if (!v.is_custom_gcode() || m_settings.extrusion_roles_visibility.at(EGCodeExtrusionRole::Custom)) {
                m_width_range.update(round_to_bin(v.width));
                m_volumetric_rate_range.update(round_to_bin(v.volumetric_rate));
            }
            m_fan_speed_range.update(v.fan_speed);
            m_temperature_range.update(v.temperature);
        }
        if ((v.is_travel() && m_settings.options_visibility.at(EOptionType::Travels)) || v.is_extrusion())
            m_speed_range.update(v.feedrate);
    }

    for (size_t i = 0; i < m_layers_times.size(); ++i) {
        for (const float time : m_layers_times[static_cast<uint8_t>(m_settings.time_mode)]) {
            m_layer_time_range[i].update(time);
        }
    }
}

Color ViewerImpl::select_color(const PathVertex& v) const
{
    if (v.type == EMoveType::Noop)
        return Dummy_Color;

    if (v.is_wipe())
        return Wipe_Color;

    if (v.is_option())
        return Options_Colors.at(v.type);

    const size_t role = static_cast<size_t>(v.role);
    switch (m_settings.view_type)
    {
    case EViewType::FeatureType:
    {
        assert((v.is_travel() && role < Travels_Colors.size()) || (v.is_extrusion() && role < Extrusion_Roles_Colors.size()));
        return v.is_travel() ? Travels_Colors[role] : Extrusion_Roles_Colors[role];
    }
    case EViewType::Height:
    {
        assert(!v.is_travel() || role < Travels_Colors.size());
        return v.is_travel() ? Travels_Colors[role] : m_height_range.get_color_at(v.height);
    }
    case EViewType::Width:
    {
        assert(!v.is_travel() || role < Travels_Colors.size());
        return v.is_travel() ? Travels_Colors[role] : m_width_range.get_color_at(v.width);
    }
    case EViewType::Speed:
    {
        return m_speed_range.get_color_at(v.feedrate);
    }
    case EViewType::FanSpeed:
    {
        assert(!v.is_travel() || role < Travels_Colors.size());
        return v.is_travel() ? Travels_Colors[role] : m_fan_speed_range.get_color_at(v.fan_speed);
    }
    case EViewType::Temperature:
    {
        assert(!v.is_travel() || role < Travels_Colors.size());
        return v.is_travel() ? Travels_Colors[role] : m_temperature_range.get_color_at(v.temperature);
    }
    case EViewType::VolumetricFlowRate:
    {
        assert(!v.is_travel() || role < Travels_Colors.size());
        return v.is_travel() ? Travels_Colors[role] : m_volumetric_rate_range.get_color_at(v.volumetric_rate);
    }
    case EViewType::LayerTimeLinear:
    {
        assert(!v.is_travel() || role < Travels_Colors.size());
        return v.is_travel() ? Travels_Colors[role] : m_layer_time_range[0].get_color_at(m_layers_times[static_cast<size_t>(m_settings.time_mode)][v.layer_id]);
    }
    case EViewType::LayerTimeLogarithmic:
    {
        assert(!v.is_travel() || role < Travels_Colors.size());
        return v.is_travel() ? Travels_Colors[role] : m_layer_time_range[1].get_color_at(m_layers_times[static_cast<size_t>(m_settings.time_mode)][v.layer_id]);
    }
    case EViewType::Tool:
    {
        assert(static_cast<size_t>(v.extruder_id) < m_tool_colors.size());
        return m_tool_colors[v.extruder_id];
    }
    case EViewType::ColorPrint:
    {
        return m_tool_colors[v.color_id % m_tool_colors.size()];
    }
    default: { break; }
    }

    return Dummy_Color;
}

void ViewerImpl::render_segments(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix, const Vec3f& camera_position)
{
    if (m_segments_shader_id == 0)
        return;

    int curr_active_texture = 0;
    glsafe(glGetIntegerv(GL_ACTIVE_TEXTURE, &curr_active_texture));
    int curr_bound_texture = 0;
    glsafe(glGetIntegerv(GL_TEXTURE_BINDING_BUFFER, &curr_bound_texture));
    int curr_shader;
    glsafe(glGetIntegerv(GL_CURRENT_PROGRAM, &curr_shader));
    const bool curr_cull_face = glIsEnabled(GL_CULL_FACE);
    glcheck();

    glsafe(glActiveTexture(GL_TEXTURE0));
    glsafe(glBindTexture(GL_TEXTURE_BUFFER, m_positions_tex_id));
    glsafe(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, m_positions_buf_id));

    glsafe(glActiveTexture(GL_TEXTURE1));
    glsafe(glBindTexture(GL_TEXTURE_BUFFER, m_heights_widths_angles_tex_id));
    glsafe(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, m_heights_widths_angles_buf_id));

    glsafe(glActiveTexture(GL_TEXTURE2));
    glsafe(glBindTexture(GL_TEXTURE_BUFFER, m_colors_tex_id));
    glsafe(glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, m_colors_buf_id));

    glsafe(glActiveTexture(GL_TEXTURE3));
    glsafe(glBindTexture(GL_TEXTURE_BUFFER, m_enabled_segments_tex_id));
    glsafe(glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, m_enabled_segments_buf_id));

    glsafe(glUseProgram(m_segments_shader_id));

    glsafe(glUniform1i(m_uni_segments_positions_tex_id, 0));
    glsafe(glUniform1i(m_uni_segments_height_width_angle_tex_id, 1));
    glsafe(glUniform1i(m_uni_segments_colors_tex_id, 2));
    glsafe(glUniform1i(m_uni_segments_segment_index_tex_id, 3));
    glsafe(glUniformMatrix4fv(m_uni_segments_view_matrix_id, 1, GL_FALSE, view_matrix.data()));
    glsafe(glUniformMatrix4fv(m_uni_segments_projection_matrix_id, 1, GL_FALSE, projection_matrix.data()));
    glsafe(glUniform3fv(m_uni_segments_camera_position_id, 1, camera_position.data()));

    glsafe(glDisable(GL_CULL_FACE));

    m_segment_template.render(m_enabled_segments_count);

    if (curr_cull_face)
        glsafe(glEnable(GL_CULL_FACE));

    glsafe(glUseProgram(curr_shader));
    glsafe(glBindTexture(GL_TEXTURE_BUFFER, curr_bound_texture));
    glsafe(glActiveTexture(curr_active_texture));
}

void ViewerImpl::render_options(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix)
{
    if (m_options_shader_id == 0)
        return;

    int curr_active_texture = 0;
    glsafe(glGetIntegerv(GL_ACTIVE_TEXTURE, &curr_active_texture));
    int curr_bound_texture = 0;
    glsafe(glGetIntegerv(GL_TEXTURE_BINDING_BUFFER, &curr_bound_texture));
    int curr_shader;
    glsafe(glGetIntegerv(GL_CURRENT_PROGRAM, &curr_shader));
    const bool curr_cull_face = glIsEnabled(GL_CULL_FACE);
    glcheck();

    glsafe(glActiveTexture(GL_TEXTURE0));
    glsafe(glBindTexture(GL_TEXTURE_BUFFER, m_positions_tex_id));
    glsafe(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, m_positions_buf_id));

    glsafe(glActiveTexture(GL_TEXTURE1));
    glsafe(glBindTexture(GL_TEXTURE_BUFFER, m_heights_widths_angles_tex_id));
    glsafe(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, m_heights_widths_angles_buf_id));

    glsafe(glActiveTexture(GL_TEXTURE2));
    glsafe(glBindTexture(GL_TEXTURE_BUFFER, m_colors_tex_id));
    glsafe(glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, m_colors_buf_id));

    glsafe(glActiveTexture(GL_TEXTURE3));
    glsafe(glBindTexture(GL_TEXTURE_BUFFER, m_enabled_options_tex_id));
    glsafe(glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, m_enabled_options_buf_id));

    glsafe(glEnable(GL_CULL_FACE));

    glsafe(glUseProgram(m_options_shader_id));

    glsafe(glUniform1i(m_uni_options_positions_tex_id, 0));
    glsafe(glUniform1i(m_uni_options_height_width_angle_tex_id, 1));
    glsafe(glUniform1i(m_uni_options_colors_tex_id, 2));
    glsafe(glUniform1i(m_uni_options_segment_index_tex_id, 3));
    glsafe(glUniformMatrix4fv(m_uni_options_view_matrix_id, 1, GL_FALSE, view_matrix.data()));
    glsafe(glUniformMatrix4fv(m_uni_options_projection_matrix_id, 1, GL_FALSE, projection_matrix.data()));

    m_option_template.render(m_enabled_options_count);

    if (!curr_cull_face)
        glsafe(glDisable(GL_CULL_FACE));

    glsafe(glUseProgram(curr_shader));
    glsafe(glBindTexture(GL_TEXTURE_BUFFER, curr_bound_texture));
    glsafe(glActiveTexture(curr_active_texture));
}

#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
void ViewerImpl::render_cog_marker(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix)
{
    if (m_cog_marker_shader_id == 0)
        return;

    int curr_shader;
    glsafe(glGetIntegerv(GL_CURRENT_PROGRAM, &curr_shader));
    const bool curr_cull_face = glIsEnabled(GL_CULL_FACE);
    const bool curr_depth_test = glIsEnabled(GL_DEPTH_TEST);
    glcheck();

    glsafe(glEnable(GL_CULL_FACE));
    glsafe(glDisable(GL_DEPTH_TEST));

    glsafe(glUseProgram(m_cog_marker_shader_id));

    glsafe(glUniform3fv(m_uni_cog_marker_world_center_position, 1, m_cog_marker.get_position().data()));
    glsafe(glUniform1f(m_uni_cog_marker_scale_factor, m_cog_marker_scale_factor));
    glsafe(glUniformMatrix4fv(m_uni_cog_marker_view_matrix, 1, GL_FALSE, view_matrix.data()));
    glsafe(glUniformMatrix4fv(m_uni_cog_marker_projection_matrix, 1, GL_FALSE, projection_matrix.data()));

    m_cog_marker.render();

    if (curr_depth_test)
        glsafe(glEnable(GL_DEPTH_TEST));
    if (!curr_cull_face)
        glsafe(glDisable(GL_CULL_FACE));

    glsafe(glUseProgram(curr_shader));
}

void ViewerImpl::render_tool_marker(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix)
{
    if (m_tool_marker_shader_id == 0)
        return;

    if (!m_tool_marker.is_enabled())
        return;

    int curr_shader;
    glsafe(glGetIntegerv(GL_CURRENT_PROGRAM, &curr_shader));
    const bool curr_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean curr_depth_mask;
    glsafe(glGetBooleanv(GL_DEPTH_WRITEMASK, &curr_depth_mask));
    const bool curr_blend = glIsEnabled(GL_BLEND);
    glcheck();
    int curr_blend_func;
    glsafe(glGetIntegerv(GL_BLEND_SRC_ALPHA, &curr_blend_func));

    glsafe(glDisable(GL_CULL_FACE));
    glsafe(glDepthMask(GL_FALSE));
    glsafe(glEnable(GL_BLEND));
    glsafe(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    glsafe(glUseProgram(m_tool_marker_shader_id));

    const Vec3f& origin = m_tool_marker.get_position();
    const Vec3f offset = { 0.0f, 0.0f, m_tool_marker.get_offset_z() };
    const Vec3f position = origin + offset;
    glsafe(glUniform3fv(m_uni_tool_marker_world_origin, 1, position.data()));
    glsafe(glUniform1f(m_uni_tool_marker_scale_factor, m_tool_marker_scale_factor));
    glsafe(glUniformMatrix4fv(m_uni_tool_marker_view_matrix, 1, GL_FALSE, view_matrix.data()));
    glsafe(glUniformMatrix4fv(m_uni_tool_marker_projection_matrix, 1, GL_FALSE, projection_matrix.data()));
    const Color& color = m_tool_marker.get_color();
    glsafe(glUniform4f(m_uni_tool_marker_color_base, color[0], color[1], color[2], m_tool_marker.get_alpha()));

    m_tool_marker.render();

    glsafe(glBlendFunc(GL_SRC_ALPHA, curr_blend_func));
    if (!curr_blend)
        glsafe(glDisable(GL_BLEND));
    if (curr_depth_mask == GL_TRUE)
        glsafe(glDepthMask(GL_TRUE));
    if (curr_cull_face)
        glsafe(glEnable(GL_CULL_FACE));

    glsafe(glUseProgram(curr_shader));
}
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS

#if ENABLE_NEW_GCODE_VIEWER_DEBUG
void ViewerImpl::render_debug_window()
{
    Slic3r::GUI::ImGuiWrapper& imgui = *Slic3r::GUI::wxGetApp().imgui();
    imgui.begin(std::string("LibVGCode Viewer Debug"), ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTable("Data", 2)) {

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "# vertices");
        ImGui::TableSetColumnIndex(1);
        imgui.text(std::to_string(m_vertices.size()));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "# enabled lines");
        ImGui::TableSetColumnIndex(1);
        imgui.text(std::to_string(m_enabled_segments_count) + " [" + std::to_string(m_enabled_segments_range.first) + "-" + std::to_string(m_enabled_segments_range.second) + "]");

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "# enabled options");
        ImGui::TableSetColumnIndex(1);
        imgui.text(std::to_string(m_enabled_options_count) + " [" + std::to_string(m_enabled_options_range.first) + "-" + std::to_string(m_enabled_options_range.second) + "]");

        ImGui::Separator();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "layers range");
        ImGui::TableSetColumnIndex(1);
        const std::array<uint32_t, 2>& layers_range = get_layers_range();
        imgui.text(std::to_string(layers_range[0]) + " - " + std::to_string(layers_range[1]));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "view range (current)");
        ImGui::TableSetColumnIndex(1);
        const std::array<uint32_t, 2>& current_view_range = get_view_current_range();
        imgui.text(std::to_string(current_view_range[0]) + " - " + std::to_string(current_view_range[1]));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "view range (global)");
        ImGui::TableSetColumnIndex(1);
        const std::array<uint32_t, 2>& global_view_range = get_view_global_range();
        imgui.text(std::to_string(global_view_range[0]) + " - " + std::to_string(global_view_range[1]));

        auto add_range_property_row = [&imgui](const std::string& label, const std::array<float, 2>& range) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, label);
            ImGui::TableSetColumnIndex(1);
            char buf[64];
            sprintf(buf, "%.3f - %.3f", range[0], range[1]);
            imgui.text(buf);
        };

        add_range_property_row("height range", m_height_range.get_range());
        add_range_property_row("width range", m_width_range.get_range());
        add_range_property_row("speed range", m_speed_range.get_range());
        add_range_property_row("fan speed range", m_fan_speed_range.get_range());
        add_range_property_row("temperature range", m_temperature_range.get_range());
        add_range_property_row("volumetric rate range", m_volumetric_rate_range.get_range());
        add_range_property_row("layer time linear range", m_layer_time_range[0].get_range());
        add_range_property_row("layer time logarithmic range", m_layer_time_range[1].get_range());

        ImGui::EndTable();

#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
        ImGui::Separator();

        if (ImGui::BeginTable("Cog", 2)) {

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "Cog marker scale factor");
            ImGui::TableSetColumnIndex(1);
            imgui.text(std::to_string(get_cog_marker_scale_factor()));

            ImGui::EndTable();
        }

        ImGui::Separator();

        if (ImGui::BeginTable("Tool", 2)) {

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "Tool marker scale factor");
            ImGui::TableSetColumnIndex(1);
            imgui.text(std::to_string(get_tool_marker_scale_factor()));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "Tool marker z offset");
            ImGui::TableSetColumnIndex(1);
            float tool_z_offset = get_tool_marker_offset_z();
            if (imgui.slider_float("##ToolZOffset", &tool_z_offset, 0.0f, 1.0f))
                set_tool_marker_offset_z(tool_z_offset);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "Tool marker color");
            ImGui::TableSetColumnIndex(1);
            Color color = get_tool_marker_color();
            if (ImGui::ColorPicker3("##ToolColor", color.data()))
                set_tool_marker_color(color);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "Tool marker alpha");
            ImGui::TableSetColumnIndex(1);
            float tool_alpha = get_tool_marker_alpha();
            if (imgui.slider_float("##ToolAlpha", &tool_alpha, 0.25f, 0.75f))
                set_tool_marker_alpha(tool_alpha);

            ImGui::EndTable();
        }
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
    }

    imgui.end();

/*
    auto to_string = [](EMoveType type) {
        switch (type)
        {
        case EMoveType::Noop:        { return "Noop"; }
        case EMoveType::Retract:     { return "Retract"; }
        case EMoveType::Unretract:   { return "Unretract"; }
        case EMoveType::Seam:        { return "Seam"; }
        case EMoveType::ToolChange:  { return "ToolChange"; }
        case EMoveType::ColorChange: { return "ColorChange"; }
        case EMoveType::PausePrint:  { return "PausePrint"; }
        case EMoveType::CustomGCode: { return "CustomGCode"; }
        case EMoveType::Travel:      { return "Travel"; }
        case EMoveType::Wipe:        { return "Wipe"; }
        case EMoveType::Extrude:     { return "Extrude"; }
        default:                     { return "Error"; }
        }
    };

    imgui.begin(std::string("LibVGCode Viewer Vertices"), ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    if (ImGui::BeginTable("VertexData", 4)) {
        uint32_t counter = 0;
        for (size_t i = 0; i < m_vertices.size(); ++i) {
            const PathVertex& v = m_vertices[i];
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            imgui.text_colored(m_valid_lines_bitset[i] ? Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT : Slic3r::GUI::ImGuiWrapper::COL_GREY_LIGHT, std::to_string(++counter));
            ImGui::TableSetColumnIndex(1);
            imgui.text(to_string(v.type));

            ImGui::TableSetColumnIndex(2);
            imgui.text(std::to_string(m_vertices_map[i]));

            ImGui::TableSetColumnIndex(3);
            imgui.text(std::to_string(v.position[0]) + ", " + std::to_string(v.position[1]) + ", " + std::to_string(v.position[2]));
        }

        ImGui::EndTable();
    }
    imgui.end();
*/
}
#endif // ENABLE_NEW_GCODE_VIEWER_DEBUG

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
