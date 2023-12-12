//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak, Vojtěch Bubník @bubnikv, Oleksandra Iushchenko @YuSanka
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "ViewerImpl.hpp"
#include "../include/GCodeInputData.hpp"
#include "Shaders.hpp"
#include "OpenGLUtils.hpp"
#include "Utils.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include <map>
#include <assert.h>
#include <exception>
#include <cstdio>
#include <string>
#include <algorithm>

namespace libvgcode {

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

static Mat4x4 inverse(const Mat4x4& m)
{
    // ref: https://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix

    Mat4x4 inv;

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

ViewerImpl::~ViewerImpl()
{
    reset();
    if (m_options_shader_id != 0)
        glsafe(glDeleteProgram(m_options_shader_id));
    if (m_segments_shader_id != 0)
        glsafe(glDeleteProgram(m_segments_shader_id));
}

void ViewerImpl::init()
{
    if (m_initialized)
        return;

    const bool is_valid_opengl_version = check_opengl_version();
    assert(is_valid_opengl_version);
    if (!is_valid_opengl_version)
        throw std::runtime_error("LibVGCode requires an active OpenGL context based on OpenGL 3.2 or higher:\n");

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

#if !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
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
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS

    m_initialized = true;
}

void ViewerImpl::reset()
{
    m_layers.reset();
    m_view_range.reset();
    m_extrusion_roles.reset();
    m_travels_time = { 0.0f, 0.0f };
    m_used_extruders_ids.clear();
    m_vertices.clear();
    m_valid_lines_bitset.clear();
#if !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
    m_cog_marker.reset();
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS

    m_enabled_segments_count = 0;
    m_enabled_options_count = 0;

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

void ViewerImpl::load(GCodeInputData&& gcode_data)
{
    if (gcode_data.vertices.empty())
        return;

    m_loading = true;

    m_vertices = std::move(gcode_data.vertices);
    m_settings.spiral_vase_mode = gcode_data.spiral_vase_mode;

    m_used_extruders_ids.reserve(m_vertices.size());

    for (size_t i = 0; i < m_vertices.size(); ++i) {
        const PathVertex& v = m_vertices[i];
        m_layers.update(v, static_cast<uint32_t>(i));
        if (v.type == EMoveType::Travel) {
            for (size_t j = 0; j < Time_Modes_Count; ++j) {
                m_travels_time[j] += v.times[j];
            }
        }
        else
            m_extrusion_roles.add(v.role, v.times);

        if (v.type == EMoveType::Extrude)
            m_used_extruders_ids.emplace_back(v.extruder_id);

        if (i > 0) {
#if !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
            // updates calculation for center of gravity
            if (v.type == EMoveType::Extrude &&
                v.role != EGCodeExtrusionRole::Skirt &&
                v.role != EGCodeExtrusionRole::SupportMaterial &&
                v.role != EGCodeExtrusionRole::SupportMaterialInterface &&
                v.role != EGCodeExtrusionRole::WipeTower &&
                v.role != EGCodeExtrusionRole::Custom) {
                m_cog_marker.update(0.5f * (v.position + m_vertices[i - 1].position), v.weight);
            }
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
        }
    }

    if (!m_layers.empty())
        m_layers.set_view_range(0, static_cast<uint32_t>(m_layers.count()) - 1);

    std::sort(m_used_extruders_ids.begin(), m_used_extruders_ids.end());
    m_used_extruders_ids.erase(std::unique(m_used_extruders_ids.begin(), m_used_extruders_ids.end()), m_used_extruders_ids.end());
    m_used_extruders_ids.shrink_to_fit();

    // reset segments visibility bitset
    m_valid_lines_bitset = BitSet<>(m_vertices.size());
    m_valid_lines_bitset.setAll();

    static constexpr const Vec3 ZERO = { 0.0f, 0.0f, 0.0f };

    // buffers to send to gpu
    std::vector<Vec3> positions;
    std::vector<Vec3> heights_widths_angles;
    positions.reserve(m_vertices.size());
    heights_widths_angles.reserve(m_vertices.size());
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        const PathVertex& v = m_vertices[i];
        const EMoveType move_type = v.type;
        const bool prev_line_valid = i > 0 && m_valid_lines_bitset[i - 1];
        const Vec3 prev_line = prev_line_valid ? v.position - m_vertices[i - 1].position : ZERO;
        const bool this_line_valid = i + 1 < m_vertices.size() &&
                                     m_vertices[i + 1].position != v.position &&
                                     m_vertices[i + 1].type == move_type &&
                                     move_type != EMoveType::Seam;
        const Vec3 this_line = this_line_valid ? m_vertices[i + 1].position - v.position : ZERO;

        if (this_line_valid) {
            // there is a valid path between point i and i+1.
        }
        else {
            // the connection is invalid, there should be no line rendered, ever
            m_valid_lines_bitset.reset(i);
        }

        Vec3 position = v.position;
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
        glsafe(glBufferData(GL_TEXTURE_BUFFER, positions.size() * sizeof(Vec3), positions.data(), GL_STATIC_DRAW));
        glsafe(glGenTextures(1, &m_positions_tex_id));
        glsafe(glBindTexture(GL_TEXTURE_BUFFER, m_positions_tex_id));

        // create and fill height, width and angles buffer
        glsafe(glGenBuffers(1, &m_heights_widths_angles_buf_id));
        glsafe(glBindBuffer(GL_TEXTURE_BUFFER, m_heights_widths_angles_buf_id));
        glsafe(glBufferData(GL_TEXTURE_BUFFER, heights_widths_angles.size() * sizeof(Vec3), heights_widths_angles.data(), GL_STATIC_DRAW));
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

    update_view_full_range();
    m_view_range.set_visible(m_view_range.get_enabled());
    update_enabled_entities();
    update_colors();

    m_loading = false;
}

void ViewerImpl::update_enabled_entities()
{
    std::vector<uint32_t> enabled_segments;
    std::vector<uint32_t> enabled_options;
    std::array<uint32_t, 2> range = m_view_range.get_visible();

    // when top layer only visualization is enabled, we need to render
    // all the toolpaths in the other layers as grayed, so extend the range
    // to contain them
    if (m_settings.top_layer_only_view_range)
        range[0] = m_view_range.get_full()[0];

    // to show the options at the current tool marker position we need to extend the range by one extra step
    if (m_vertices[range[1]].is_option() && range[1] < static_cast<uint32_t>(m_vertices.size()) - 1)
        ++range[1];

    if (m_settings.spiral_vase_mode) {
        // when spiral vase mode is enabled and only one layer is shown, extend the range by one step
        const std::array<uint32_t, 2>& layers_range = m_layers.get_view_range();
        if (layers_range[0] > 0 && layers_range[0] == layers_range[1])
            --range[0];
    }

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

    if (m_enabled_segments_count > 0)
        m_enabled_segments_range.set(enabled_segments.front(), enabled_segments.back());
    else
        m_enabled_segments_range.reset();
    if (m_enabled_options_count > 0)
        m_enabled_options_range.set(enabled_options.front(), enabled_options.back());
    else
        m_enabled_options_range.reset();

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

    m_settings.update_enabled_entities = false;
}

static float encode_color(const Color& color) {
    const int r = static_cast<int>(color[0]);
    const int g = static_cast<int>(color[1]);
    const int b = static_cast<int>(color[2]);
    const int i_color = r << 16 | g << 8 | b;
    return static_cast<float>(i_color);
}

void ViewerImpl::update_colors()
{
    update_color_ranges();

    const uint32_t top_layer_id = m_settings.top_layer_only_view_range ? m_layers.get_view_range()[1] : 0;
    const bool color_top_layer_only = m_view_range.get_full()[1] != m_view_range.get_visible()[1];
    std::vector<float> colors(m_vertices.size());
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        colors[i] = (color_top_layer_only && m_vertices[i].layer_id < top_layer_id &&
                     (!m_settings.spiral_vase_mode || i != m_view_range.get_enabled()[0])) ?
            encode_color(Dummy_Color) : encode_color(select_color(m_vertices[i]));
    }

    // update gpu buffer for colors
    assert(m_colors_buf_id > 0);
    glsafe(glBindBuffer(GL_TEXTURE_BUFFER, m_colors_buf_id));
    glsafe(glBufferData(GL_TEXTURE_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW));
    glsafe(glBindBuffer(GL_TEXTURE_BUFFER, 0));

    m_settings.update_colors = false;
}

void ViewerImpl::render(const Mat4x4& view_matrix, const Mat4x4& projection_matrix)
{
    // ensure that the render does take place while loading the data
    if (m_loading)
        return;

    if (m_settings.update_view_full_range)
        update_view_full_range();

    if (m_settings.update_enabled_entities)
        update_enabled_entities();

    if (m_settings.update_colors)
        update_colors();

    const Mat4x4 inv_view_matrix = inverse(view_matrix);
    const Vec3 camera_position = { inv_view_matrix[12], inv_view_matrix[13], inv_view_matrix[14] };
    render_segments(view_matrix, projection_matrix, camera_position);
    render_options(view_matrix, projection_matrix);

#if !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
    if (m_settings.options_visibility.at(EOptionType::ToolMarker))
        render_tool_marker(view_matrix, projection_matrix);
    if (m_settings.options_visibility.at(EOptionType::CenterOfGravity))
        render_cog_marker(view_matrix, projection_matrix);
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
}

EViewType ViewerImpl::get_view_type() const
{
    return m_settings.view_type;
}

void ViewerImpl::set_view_type(EViewType type)
{
    m_settings.view_type = type;
    m_settings.update_colors = true;
}

ETimeMode ViewerImpl::get_time_mode() const
{
    return m_settings.time_mode;
}

void ViewerImpl::set_time_mode(ETimeMode mode)
{
    m_settings.time_mode = mode;
    m_settings.update_colors = true;
}

const std::array<uint32_t, 2>& ViewerImpl::get_layers_view_range() const
{
    return m_layers.get_view_range();
}

void ViewerImpl::set_layers_view_range(const std::array<uint32_t, 2>& range)
{
    set_layers_view_range(range[0], range[1]);
}

void ViewerImpl::set_layers_view_range(uint32_t min, uint32_t max)
{
    m_layers.set_view_range(min, max);
    // force immediate update of the full range
    update_view_full_range();
    m_view_range.set_visible(m_view_range.get_enabled());
    m_settings.update_enabled_entities = true;
    m_settings.update_colors = true;
}

bool ViewerImpl::is_top_layer_only_view_range() const
{
    return m_settings.top_layer_only_view_range;
}

void ViewerImpl::set_top_layer_only_view_range(bool top_layer_only_view_range)
{
    m_settings.top_layer_only_view_range = top_layer_only_view_range;
    m_settings.update_colors = true;
}

size_t ViewerImpl::get_layers_count() const
{
    return m_layers.count();
}

float ViewerImpl::get_layer_z(size_t layer_id) const
{
    return m_layers.get_layer_z(layer_id);
}

std::vector<float> ViewerImpl::get_layers_zs() const
{
    return m_layers.get_zs();
}

size_t ViewerImpl::get_layer_id_at(float z) const
{
    return m_layers.get_layer_id_at(z);
}

size_t ViewerImpl::get_used_extruders_count() const
{
    return m_used_extruders_ids.size();
}

const std::vector<uint8_t>& ViewerImpl::get_used_extruders_ids() const
{
    return m_used_extruders_ids;
}

AABox ViewerImpl::get_bounding_box(EBBoxType type) const
{
    assert(type < EBBoxType::COUNT);
    Vec3 min = { FLT_MAX, FLT_MAX, FLT_MAX };
    Vec3 max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    for (const PathVertex& v : m_vertices) {
        if (type != EBBoxType::Full && (v.type != EMoveType::Extrude || v.width == 0.0f || v.height == 0.0f))
            continue;
        else if (type == EBBoxType::ExtrusionNoCustom && v.role == EGCodeExtrusionRole::Custom)
            continue;

        for (int j = 0; j < 3; ++j) {
            min[j] = std::min(min[j], v.position[j]);
            max[j] = std::max(max[j], v.position[j]);
        }
    }
    return { min, max };
}

bool ViewerImpl::is_option_visible(EOptionType type) const
{
    try {
        return m_settings.options_visibility.at(type);
    }
    catch (...) {
        return false;
    }
}

void ViewerImpl::toggle_option_visibility(EOptionType type)
{
    try {
        bool& value = m_settings.options_visibility.at(type);
        value = !value;
        update_view_full_range();
        m_settings.update_enabled_entities = true;
        m_settings.update_colors = true;
    }
    catch (...) {
        // do nothing;
    }
}

bool ViewerImpl::is_extrusion_role_visible(EGCodeExtrusionRole role) const
{
    try {
        return m_settings.extrusion_roles_visibility.at(role);
    }
    catch (...) {
        return false;
    }
}

void ViewerImpl::toggle_extrusion_role_visibility(EGCodeExtrusionRole role)
{
    try {
        bool& value = m_settings.extrusion_roles_visibility.at(role);
        value = !value;
        update_view_full_range();
        m_settings.update_enabled_entities = true;
        m_settings.update_colors = true;
    }
    catch (...) {
        // do nothing;
    }
}

const std::array<uint32_t, 2>& ViewerImpl::get_view_full_range() const
{
    return m_view_range.get_full();
}

const std::array<uint32_t, 2>& ViewerImpl::get_view_enabled_range() const
{
    return m_view_range.get_enabled();
}

const std::array<uint32_t, 2>& ViewerImpl::get_view_visible_range() const
{
    return m_view_range.get_visible();
}

void ViewerImpl::set_view_visible_range(uint32_t min, uint32_t max)
{
    // force update of the full range, to avoid clamping the visible range with full old values
    // when calling m_view_range.set_visible()
    update_view_full_range();
    m_view_range.set_visible(min, max);
    update_enabled_entities();
    m_settings.update_colors = true;
}

size_t ViewerImpl::get_vertices_count() const
{
    return m_vertices.size();
}

PathVertex ViewerImpl::get_current_vertex() const
{
    return m_vertices[m_view_range.get_visible()[1]];
}

PathVertex ViewerImpl::get_vertex_at(size_t id) const
{
    return (id < m_vertices.size()) ? m_vertices[id] : PathVertex();
}

size_t ViewerImpl::get_enabled_segments_count() const
{
    return m_enabled_segments_count;
}

const std::array<uint32_t, 2>& ViewerImpl::get_enabled_segments_range() const
{
    return m_enabled_segments_range.get();
}

size_t ViewerImpl::get_enabled_options_count() const
{
    return m_enabled_options_count;
}

const std::array<uint32_t, 2>& ViewerImpl::get_enabled_options_range() const
{
    return m_enabled_options_range.get();
}

std::vector<EGCodeExtrusionRole> ViewerImpl::get_extrusion_roles() const
{
    return m_extrusion_roles.get_roles();
}

float ViewerImpl::get_extrusion_role_time(EGCodeExtrusionRole role) const
{
    return m_extrusion_roles.get_time(role, m_settings.time_mode);
}

size_t ViewerImpl::get_extrusion_roles_count() const
{
    return m_extrusion_roles.get_roles_count();
}

float ViewerImpl::get_extrusion_role_time(EGCodeExtrusionRole role, ETimeMode mode) const
{
    return m_extrusion_roles.get_time(role, mode);
}

float ViewerImpl::get_travels_time() const
{
    return get_travels_time(m_settings.time_mode);
}

float ViewerImpl::get_travels_time(ETimeMode mode) const
{
    return (mode < ETimeMode::COUNT) ? m_travels_time[static_cast<size_t>(mode)] : 0.0f;
}

std::vector<float> ViewerImpl::get_layers_times() const
{
    return get_layers_times(m_settings.time_mode);
}

std::vector<float> ViewerImpl::get_layers_times(ETimeMode mode) const
{
    return m_layers.get_times(mode);
}

size_t ViewerImpl::get_tool_colors_count() const
{
    return m_tool_colors.size();
}

const std::vector<Color>& ViewerImpl::get_tool_colors() const
{
    return m_tool_colors;
}

void ViewerImpl::set_tool_colors(const std::vector<Color>& colors)
{
    m_tool_colors = colors;
    m_settings.update_colors = true;
}

const Color& ViewerImpl::get_extrusion_role_color(EGCodeExtrusionRole role) const
{
    auto it = m_extrusion_roles_colors.find(role);
    return (it != m_extrusion_roles_colors.end()) ? m_extrusion_roles_colors.at(role) : Dummy_Color;
}

void ViewerImpl::set_extrusion_role_color(EGCodeExtrusionRole role, const Color& color)
{
    auto it = m_extrusion_roles_colors.find(role);
    if (it != m_extrusion_roles_colors.end())
        m_extrusion_roles_colors[role] = color;
}

void ViewerImpl::reset_default_extrusion_roles_colors()
{
    m_extrusion_roles_colors = Default_Extrusion_Roles_Colors;
}

const Color& ViewerImpl::get_option_color(EOptionType type) const
{
    auto it = m_options_colors.find(type);
    return (it != m_options_colors.end()) ?  m_options_colors.at(type) : Dummy_Color;
}

void ViewerImpl::set_option_color(EOptionType type, const Color& color)
{
    auto it = m_options_colors.find(type);
    if (it != m_options_colors.end())
        m_options_colors[type] = color;
}

void ViewerImpl::reset_default_options_colors()
{
    m_options_colors = Default_Options_Colors;
}

const ColorRange& ViewerImpl::get_height_range() const
{
    return m_height_range;
}

const ColorRange& ViewerImpl::get_width_range() const
{
    return m_width_range;
}

const ColorRange& ViewerImpl::get_speed_range() const
{
    return m_speed_range;
}

const ColorRange& ViewerImpl::get_fan_speed_range() const
{
    return m_fan_speed_range;
}

const ColorRange& ViewerImpl::get_temperature_range() const
{
    return m_temperature_range;
}

const ColorRange& ViewerImpl::get_volumetric_rate_range() const
{
    return m_volumetric_rate_range;
}

const ColorRange& ViewerImpl::get_layer_time_range(EColorRangeType type) const
{
    try {
        return m_layer_time_range[static_cast<size_t>(type)];
    }
    catch (...) {
        return ColorRange::Dummy_Color_Range;
    }
}

float ViewerImpl::get_travels_radius() const
{
    return m_travels_radius;
}

void ViewerImpl::set_travels_radius(float radius)
{
    m_travels_radius = std::clamp(radius, 0.05f, 0.5f);
    update_heights_widths();
}

float ViewerImpl::get_wipes_radius() const
{
    return m_wipes_radius;
}

void ViewerImpl::set_wipes_radius(float radius)
{
    m_wipes_radius = std::clamp(radius, 0.05f, 0.5f);
    update_heights_widths();
}

#if !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
Vec3 ViewerImpl::get_cog_marker_position() const
{
    return m_cog_marker.get_position();
}

float ViewerImpl::get_cog_marker_scale_factor() const
{
    return m_cog_marker_scale_factor;
}

void ViewerImpl::set_cog_marker_scale_factor(float factor)
{
    m_cog_marker_scale_factor = std::max(factor, 0.001f);
}

bool ViewerImpl::is_tool_marker_enabled() const
{
    return m_tool_marker.is_enabled();
}

void ViewerImpl::enable_tool_marker(bool value)
{
    m_tool_marker.enable(value);
}

const Vec3& ViewerImpl::get_tool_marker_position() const
{
    return m_tool_marker.get_position();
}

void ViewerImpl::set_tool_marker_position(const Vec3& position)
{
    m_tool_marker.set_position(position);
}

float ViewerImpl::get_tool_marker_offset_z() const
{
    return m_tool_marker.get_offset_z();
}

void ViewerImpl::set_tool_marker_offset_z(float offset_z)
{
    m_tool_marker.set_offset_z(offset_z);
}

float ViewerImpl::get_tool_marker_scale_factor() const
{
    return m_tool_marker_scale_factor;
}

void ViewerImpl::set_tool_marker_scale_factor(float factor)
{
    m_tool_marker_scale_factor = std::max(factor, 0.001f);
}

const Color& ViewerImpl::get_tool_marker_color() const
{
    return m_tool_marker.get_color();
}

void ViewerImpl::set_tool_marker_color(const Color& color)
{
    m_tool_marker.set_color(color);
}

float ViewerImpl::get_tool_marker_alpha() const
{
    return m_tool_marker.get_alpha();
}

void ViewerImpl::set_tool_marker_alpha(float alpha)
{
    m_tool_marker.set_alpha(alpha);
}
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS

static bool is_visible(const PathVertex& v, const Settings& settings)
{
    try {
        return ((v.type == EMoveType::Travel      && !settings.options_visibility.at(EOptionType::Travels)) ||
                (v.type == EMoveType::Wipe        && !settings.options_visibility.at(EOptionType::Wipes)) ||
                (v.type == EMoveType::Retract     && !settings.options_visibility.at(EOptionType::Retractions)) ||
                (v.type == EMoveType::Unretract   && !settings.options_visibility.at(EOptionType::Unretractions)) ||
                (v.type == EMoveType::Seam        && !settings.options_visibility.at(EOptionType::Seams)) ||
                (v.type == EMoveType::ToolChange  && !settings.options_visibility.at(EOptionType::ToolChanges)) ||
                (v.type == EMoveType::ColorChange && !settings.options_visibility.at(EOptionType::ColorChanges)) ||
                (v.type == EMoveType::PausePrint  && !settings.options_visibility.at(EOptionType::PausePrints)) ||
                (v.type == EMoveType::CustomGCode && !settings.options_visibility.at(EOptionType::CustomGCodes)) ||
                (v.type == EMoveType::Extrude     && !settings.extrusion_roles_visibility.at(v.role))) ? false : true;
    }
    catch (...) {
        return false;
    }
}

void ViewerImpl::update_view_full_range()
{
    const std::array<uint32_t, 2>& layers_range = m_layers.get_view_range();
    const bool travels_visible = m_settings.options_visibility.at(EOptionType::Travels);
    const bool wipes_visible   = m_settings.options_visibility.at(EOptionType::Wipes);

    auto first_it = m_vertices.begin();
    while (first_it != m_vertices.end() &&
           (first_it->layer_id < layers_range[0] || !is_visible(*first_it, m_settings))) {
        ++first_it;
    }

    // If the first vertex is an extrusion, add an extra step to properly detect the first segment
    if (first_it != m_vertices.begin() && first_it->type == EMoveType::Extrude)
        --first_it;

    if (first_it == m_vertices.end())
        m_view_range.set_full(Range());
    else {
        if (travels_visible || wipes_visible) {
            // if the global range starts with a travel/wipe move, extend it to the travel/wipe start
            while (first_it != m_vertices.begin() &&
                   ((travels_visible && first_it->is_travel()) ||
                    (wipes_visible && first_it->is_wipe()))) {
                --first_it;
            }
        }

        auto last_it = first_it;
        while (last_it != m_vertices.end() && last_it->layer_id <= layers_range[1]) {
            ++last_it;
        }
        if (last_it != first_it)
            --last_it;

        // remove disabled trailing options, if any 
        auto rev_first_it = std::make_reverse_iterator(first_it);
        if (rev_first_it != m_vertices.rbegin())
            --rev_first_it;
        auto rev_last_it = std::make_reverse_iterator(last_it);
        if (rev_last_it != m_vertices.rbegin())
            --rev_last_it;

        bool reduced = false;
        while (rev_last_it != rev_first_it && !is_visible(*rev_last_it, m_settings)) {
            ++rev_last_it;
            reduced = true;
        }

        if (reduced && rev_last_it != m_vertices.rend())
            last_it = rev_last_it.base() - 1;

        if (travels_visible || wipes_visible) {
            // if the global range ends with a travel/wipe move, extend it to the travel/wipe end
            while (last_it != m_vertices.end() && last_it + 1 != m_vertices.end() &&
                   ((travels_visible && last_it->is_travel() && (last_it + 1)->is_travel()) ||
                    (wipes_visible && last_it->is_wipe() && (last_it + 1)->is_wipe()))) {
                  ++last_it;
            }
        }

        if (first_it != last_it)
            m_view_range.set_full(std::distance(m_vertices.begin(), first_it), std::distance(m_vertices.begin(), last_it));
        else
            m_view_range.set_full(Range());

        if (m_settings.top_layer_only_view_range) {
            const std::array<uint32_t, 2>& full_range = m_view_range.get_full();
            auto top_first_it = m_vertices.begin() + full_range[0];
            bool shortened = false;
            while (top_first_it != m_vertices.end() && (top_first_it->layer_id < layers_range[1] || !is_visible(*top_first_it, m_settings))) {
                ++top_first_it;
                shortened = true;
            }
            if (shortened)
                --top_first_it;

            // when spiral vase mode is enabled and only one layer is shown, extend the range by one step
            if (m_settings.spiral_vase_mode && layers_range[0] > 0 && layers_range[0] == layers_range[1])
                --top_first_it;
            m_view_range.set_enabled(std::distance(m_vertices.begin(), top_first_it), full_range[1]);
        }
        else
            m_view_range.set_enabled(m_view_range.get_full());
    }

    m_settings.update_view_full_range = false;
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

    const std::vector<float> times = m_layers.get_times(m_settings.time_mode);
    for (size_t i = 0; i < m_layer_time_range.size(); ++i) {
        for (float t : times) {
            m_layer_time_range[i].update(t);
        }
    }
}

void ViewerImpl::update_heights_widths()
{
    if (m_heights_widths_angles_buf_id == 0)
        return;

    glsafe(glBindBuffer(GL_TEXTURE_BUFFER, m_heights_widths_angles_buf_id));
    Vec3* buffer = static_cast<Vec3*>(glMapBuffer(GL_TEXTURE_BUFFER, GL_WRITE_ONLY));
    glcheck();

    for (size_t i = 0; i < m_vertices.size(); ++i) {
        const PathVertex& v = m_vertices[i];
        if (v.is_travel()) {
            buffer[i][0] = m_travels_radius;
            buffer[i][1] = m_travels_radius;
        }
        else if (v.is_wipe()) {
            buffer[i][0] = m_wipes_radius;
            buffer[i][1] = m_wipes_radius;
        }
    }

    glsafe(glUnmapBuffer(GL_TEXTURE_BUFFER));
    glsafe(glBindBuffer(GL_TEXTURE_BUFFER, 0));
}

Color ViewerImpl::select_color(const PathVertex& v) const
{
    if (v.type == EMoveType::Noop)
        return Dummy_Color;

    if (v.is_wipe())
        return Wipe_Color;

    if (v.is_option())
        return get_option_color(type_to_option(v.type));

    const size_t role = static_cast<size_t>(v.role);
    switch (m_settings.view_type)
    {
    case EViewType::FeatureType:
    {
        assert((v.is_travel() && role < Travels_Colors.size()) || v.is_extrusion());
        return v.is_travel() ? Travels_Colors[role] : get_extrusion_role_color(v.role);
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
        return v.is_travel() ? Travels_Colors[role] :
            m_layer_time_range[0].get_color_at(m_layers.get_layer_time(m_settings.time_mode, static_cast<size_t>(v.layer_id)));
    }
    case EViewType::LayerTimeLogarithmic:
    {
        assert(!v.is_travel() || role < Travels_Colors.size());
        return v.is_travel() ? Travels_Colors[role] :
            m_layer_time_range[1].get_color_at(m_layers.get_layer_time(m_settings.time_mode, static_cast<size_t>(v.layer_id)));
    }
    case EViewType::Tool:
    {
        assert(static_cast<size_t>(v.extruder_id) < m_tool_colors.size());
        return m_tool_colors[v.extruder_id];
    }
    case EViewType::ColorPrint:
    {
        return m_layers.layer_contains_colorprint_options(static_cast<size_t>(v.layer_id)) ? Dummy_Color :
            m_tool_colors[static_cast<size_t>(v.color_id) % m_tool_colors.size()];
    }
    default: { break; }
    }

    return Dummy_Color;
}

void ViewerImpl::render_segments(const Mat4x4& view_matrix, const Mat4x4& projection_matrix, const Vec3& camera_position)
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

void ViewerImpl::render_options(const Mat4x4& view_matrix, const Mat4x4& projection_matrix)
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

#if !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
void ViewerImpl::render_cog_marker(const Mat4x4& view_matrix, const Mat4x4& projection_matrix)
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

void ViewerImpl::render_tool_marker(const Mat4x4& view_matrix, const Mat4x4& projection_matrix)
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

    const Vec3& origin = m_tool_marker.get_position();
    const Vec3 offset = { 0.0f, 0.0f, m_tool_marker.get_offset_z() };
    const Vec3 position = origin + offset;
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
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
