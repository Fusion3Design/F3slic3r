//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak, Vojtěch Bubník @bubnikv
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "Toolpaths.hpp"
#include "ViewRange.hpp"
#include "Settings.hpp"
#include "Shaders.hpp"
#include "OpenGLUtils.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
#include "libslic3r/GCode/GCodeProcessor.hpp"
//################################################################################################################################

#include <map>
#include <assert.h>
#include <exception>

namespace libvgcode {

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
static uint8_t valueof(Slic3r::EMoveType type)
{
    return static_cast<uint8_t>(type);
}

static uint8_t valueof(Slic3r::GCodeExtrusionRole role)
{
    return static_cast<uint8_t>(role);
}

static Vec3f toVec3f(const Eigen::Matrix<float, 3, 1, Eigen::DontAlign>& v)
{
    return { v.x(), v.y(), v.z() };
}
//################################################################################################################################

static const Color Dummy_Color{ 0.0f, 0.0f, 0.0f };
static const Color Default_Tool_Color{ 1.0f, 0.502f, 0.0f };

static const std::map<EMoveType, Color> Options_Colors{ {
    { EMoveType::Retract,     { 0.803f, 0.135f, 0.839f } },
    { EMoveType::Unretract,   { 0.287f, 0.679f, 0.810f } },
    { EMoveType::Seam,        { 0.900f, 0.900f, 0.900f } },
    { EMoveType::ToolChange,  { 0.758f, 0.744f, 0.389f } },
    { EMoveType::ColorChange, { 0.856f, 0.582f, 0.546f } },
    { EMoveType::PausePrint,  { 0.322f, 0.942f, 0.512f } },
    { EMoveType::CustomGCode, { 0.886f, 0.825f, 0.262f } }
} };

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

static const std::vector<Color> Travels_Colors{ {
    { 0.219f, 0.282f, 0.609f }, // Move
    { 0.112f, 0.422f, 0.103f }, // Extrude
    { 0.505f, 0.064f, 0.028f }  // Retract
} };

static const Color Wipe_Color{ 1.0f, 1.0f, 0.0f };

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

Toolpaths::~Toolpaths()
{
    reset();
    if (m_options_shader_id != 0)
        glDeleteProgram(m_options_shader_id);
    if (m_segments_shader_id != 0)
        glDeleteProgram(m_segments_shader_id);
}

void Toolpaths::init()
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
}

void Toolpaths::load(const Slic3r::GCodeProcessorResult& gcode_result, const std::vector<std::string>& str_tool_colors,
    const Settings& settings)
{
    m_tool_colors.clear();
    if (settings.view_type == EViewType::Tool && !gcode_result.extruder_colors.empty())
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

    m_cog_marker.reset();

    reset();

    m_vertices.reserve(2 * gcode_result.moves.size());
    for (size_t i = 1; i < gcode_result.moves.size(); ++i) {
        const Slic3r::GCodeProcessorResult::MoveVertex& curr = gcode_result.moves[i];
        const Slic3r::GCodeProcessorResult::MoveVertex& prev = gcode_result.moves[i - 1];
        const EMoveType curr_type = static_cast<EMoveType>(valueof(curr.type));
        const EGCodeExtrusionRole curr_role = static_cast<EGCodeExtrusionRole>(valueof(curr.extrusion_role));

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
            if (prev.type != curr.type) {
                // to be able to properly detect the start/end of a path we add a 'phantom' vertex equal to the current one with
                // the exception of the position
                const PathVertex vertex = { toVec3f(prev.position), height, width, curr.feedrate, curr.fan_speed,
                    curr.temperature, curr.volumetric_rate(), extrusion_role, curr_type,
                    static_cast<uint8_t>(curr.extruder_id), static_cast<uint8_t>(curr.cp_color_id),
                    static_cast<uint32_t>(curr.layer_id) };
                m_vertices.emplace_back(vertex);
            }
        }

        const PathVertex vertex = { toVec3f(curr.position), height, width, curr.feedrate, curr.fan_speed, curr.temperature,
            curr.volumetric_rate(), extrusion_role, curr_type, static_cast<uint8_t>(curr.extruder_id),
            static_cast<uint8_t>(curr.cp_color_id), static_cast<uint32_t>(curr.layer_id) };
        m_vertices.emplace_back(vertex);

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
    }
    m_vertices.shrink_to_fit();

    m_valid_lines_bitset = BitSet<>(m_vertices.size());
    m_valid_lines_bitset.setAll();

    // buffers to send to gpu
    std::vector<Vec3f> positions;
    std::vector<Vec3f> heights_widths_angles;
    positions.reserve(m_vertices.size());
    heights_widths_angles.reserve(m_vertices.size());
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        const PathVertex& v = m_vertices[i];
        const EMoveType move_type = v.type;

        const bool prev_line_valid = i > 0 && m_valid_lines_bitset[i - 1];
        const Vec3f prev_line = prev_line_valid ? v.position - m_vertices[i - 1].position : toVec3f(0.0f);

        const bool this_line_valid = i + 1 < m_vertices.size() &&
                                     m_vertices[i + 1].position != v.position &&
                                     m_vertices[i + 1].type == move_type &&
                                     move_type != EMoveType::Seam;
        const Vec3f this_line = this_line_valid ? m_vertices[i + 1].position - v.position : toVec3f(0.0f);

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
        heights_widths_angles.push_back(toVec3f(v.height, v.width, angle));
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
}

void Toolpaths::update_enabled_entities(const ViewRange& range, const Settings& settings)
{
    std::vector<uint32_t> enabled_segments;
    std::vector<uint32_t> enabled_options;

    for (uint32_t i = range.get_current_min(); i < range.get_current_max(); i++) {
        const PathVertex& v = m_vertices[i];

        if (!m_valid_lines_bitset[i] && !v.is_option())
            continue;
        if (v.is_travel()) {
            if (!settings.options_visibility.at(EOptionType::Travels))
                continue;
        }
        else if (v.is_wipe()) {
            if (!settings.options_visibility.at(EOptionType::Wipes))
                continue;
        }
        else if (v.is_option()) {
            if (!settings.options_visibility.at(type_to_option(v.type)))
                  continue;
        }
        else if (v.is_extrusion()) {
            if (!settings.extrusion_roles_visibility.at(v.role))
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

//################################################################################################################################
    // Debug
    m_enabled_segments_range = (m_enabled_segments_count > 0) ?
        std::make_pair((uint32_t)enabled_segments.front(), (uint32_t)enabled_segments.back()) : std::make_pair((uint32_t)0, (uint32_t)0);
    m_enabled_options_range = (m_enabled_options_count > 0) ?
        std::make_pair((uint32_t)enabled_options.front(), (uint32_t)enabled_options.back()) : std::make_pair((uint32_t)0, (uint32_t)0);
//################################################################################################################################

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

void Toolpaths::update_colors(const Settings& settings)
{
    update_color_ranges(settings);

    std::vector<float> colors(m_vertices.size());
    for (size_t i = 0; i < m_vertices.size(); i++) {
        colors[i] = encode_color(select_color(m_vertices[i], settings));
    }

    // update gpu buffer for colors
    assert(m_colors_buf_id > 0);
    glsafe(glBindBuffer(GL_TEXTURE_BUFFER, m_colors_buf_id));
    glsafe(glBufferData(GL_TEXTURE_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW));
    glsafe(glBindBuffer(GL_TEXTURE_BUFFER, 0));
}

void Toolpaths::render(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix, const Vec3f& camera_position, const Settings& settings)
{
    render_segments(view_matrix, projection_matrix, camera_position);
    render_options(view_matrix, projection_matrix);
    if (settings.options_visibility.at(EOptionType::ToolMarker))
        render_tool_marker(view_matrix, projection_matrix);
    if (settings.options_visibility.at(EOptionType::CenterOfGravity))
        render_cog_marker(view_matrix, projection_matrix);
}

const std::array<std::vector<float>, static_cast<size_t>(ETimeMode::COUNT)>& Toolpaths::get_layers_times() const
{
    return m_layers_times;
}

Vec3f Toolpaths::get_cog_marker_position() const
{
    return m_cog_marker.get_position();
}

float Toolpaths::get_cog_marker_scale_factor() const
{
    return m_cog_marker_scale_factor;
}

const Vec3f& Toolpaths::get_tool_marker_position() const
{
    return m_tool_marker.get_position();
}

float Toolpaths::get_tool_marker_scale_factor() const
{
    return m_tool_marker_scale_factor;
}

const Color& Toolpaths::get_tool_marker_color() const
{
    return m_tool_marker.get_color();
}

float Toolpaths::get_tool_marker_alpha() const
{
    return m_tool_marker.get_alpha();
}

void Toolpaths::set_cog_marker_scale_factor(float factor)
{
    m_cog_marker_scale_factor = std::max(factor, 0.001f);
}

void Toolpaths::set_tool_marker_position(const Vec3f& position)
{
    m_tool_marker.set_position(position);
}

void Toolpaths::set_tool_marker_scale_factor(float factor)
{
    m_tool_marker_scale_factor = std::max(factor, 0.001f);
}

void Toolpaths::set_tool_marker_color(const Color& color)
{
    m_tool_marker.set_color(color);
}

void Toolpaths::set_tool_marker_alpha(float alpha)
{
    m_tool_marker.set_alpha(alpha);
}

//################################################################################################################################
// Debug
size_t Toolpaths::get_vertices_count() const
{
    return m_vertices.size();
}

size_t Toolpaths::get_enabled_segments_count() const
{
    return m_enabled_segments_count;
}

size_t Toolpaths::get_enabled_options_count() const
{
    return m_enabled_options_count;
}

const std::pair<uint32_t, uint32_t>& Toolpaths::get_enabled_segments_range() const
{
    return m_enabled_segments_range;
}

const std::pair<uint32_t, uint32_t>& Toolpaths::get_enabled_options_range() const
{
    return m_enabled_options_range;
}

const std::array<float, 2>& Toolpaths::get_height_range() const
{
    return m_height_range.get_range();
}

const std::array<float, 2>& Toolpaths::get_width_range() const
{
    return m_width_range.get_range();
}

const std::array<float, 2>& Toolpaths::get_speed_range() const
{
    return m_speed_range.get_range();
}

const std::array<float, 2>& Toolpaths::get_fan_speed_range() const
{
    return m_fan_speed_range.get_range();
}

const std::array<float, 2>& Toolpaths::get_temperature_range() const
{
    return m_temperature_range.get_range();
}

const std::array<float, 2>& Toolpaths::get_volumetric_rate_range() const
{
    return m_volumetric_rate_range.get_range();
}

const std::array<float, 2>& Toolpaths::get_layer_time_linear_range() const
{
    return m_layer_time_range[0].get_range();
}

const std::array<float, 2>& Toolpaths::get_layer_time_logarithmic_range() const
{
    return m_layer_time_range[1].get_range();
}
//################################################################################################################################

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

void Toolpaths::reset()
{
    m_vertices.clear();
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

void Toolpaths::update_color_ranges(const Settings& settings)
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
            if (!v.is_custom_gcode() || settings.extrusion_roles_visibility.at(EGCodeExtrusionRole::Custom)) {
                m_width_range.update(round_to_bin(v.width));
                m_volumetric_rate_range.update(round_to_bin(v.volumetric_rate));
            }
            m_fan_speed_range.update(v.fan_speed);
            m_temperature_range.update(v.temperature);
        }
        if ((v.is_travel() && settings.options_visibility.at(EOptionType::Travels)) || v.is_extrusion())
            m_speed_range.update(v.feedrate);
    }

    for (size_t i = 0; i < m_layers_times.size(); ++i) {
        for (const float time : m_layers_times[static_cast<uint8_t>(settings.time_mode)]) {
            m_layer_time_range[i].update(time);
        }
    }
}

Color Toolpaths::select_color(const PathVertex& v, const Settings& settings) const
{
    if (v.type == EMoveType::Noop)
        return Dummy_Color;

    if (v.is_wipe())
        return Wipe_Color;

    if (v.is_option())
        return Options_Colors.at(v.type);

    const size_t role = static_cast<size_t>(v.role);
    switch (settings.view_type)
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
        return v.is_travel() ? Travels_Colors[role] : m_layer_time_range[0].get_color_at(m_layers_times[static_cast<size_t>(settings.time_mode)][v.layer_id]);
    }
    case EViewType::LayerTimeLogarithmic:
    {
        assert(!v.is_travel() || role < Travels_Colors.size());
        return v.is_travel() ? Travels_Colors[role] : m_layer_time_range[1].get_color_at(m_layers_times[static_cast<size_t>(settings.time_mode)][v.layer_id]);
    }
    case EViewType::Tool:
    {
        assert(v.get_extruder_id() < m_tool_colors.size());
        return m_tool_colors[v.extruder_id];
    }
    case EViewType::ColorPrint:
    {
        return m_tool_colors[v.color_id % m_tool_colors.size()];
    }
    }

    return Dummy_Color;
}

void Toolpaths::render_segments(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix, const Vec3f& camera_position)
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

void Toolpaths::render_options(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix)
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

void Toolpaths::render_cog_marker(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix)
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

void Toolpaths::render_tool_marker(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix)
{
    if (m_tool_marker_shader_id == 0)
        return;

    int curr_shader;
    glsafe(glGetIntegerv(GL_CURRENT_PROGRAM, &curr_shader));
    const bool curr_cull_face = glIsEnabled(GL_CULL_FACE);
    int curr_depth_mask;
    glsafe(glGetIntegerv(GL_DEPTH_WRITEMASK, &curr_depth_mask));
    const bool curr_blend = glIsEnabled(GL_BLEND);
    glcheck();
    int curr_blend_func;
    glsafe(glGetIntegerv(GL_BLEND_SRC_ALPHA, &curr_blend_func));

    glsafe(glDisable(GL_CULL_FACE));
    glsafe(glDepthMask(GL_FALSE));
    glsafe(glEnable(GL_BLEND));
    glsafe(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    glsafe(glUseProgram(m_tool_marker_shader_id));

    glsafe(glUniform3fv(m_uni_tool_marker_world_origin, 1, m_tool_marker.get_position().data()));
    glsafe(glUniform1f(m_uni_tool_marker_scale_factor, m_tool_marker_scale_factor));
    glsafe(glUniformMatrix4fv(m_uni_tool_marker_view_matrix, 1, GL_FALSE, view_matrix.data()));
    glsafe(glUniformMatrix4fv(m_uni_tool_marker_projection_matrix, 1, GL_FALSE, projection_matrix.data()));
    const Color& color = m_tool_marker.get_color();
    glsafe(glUniform4f(m_uni_tool_marker_color_base, color[0], color[1], color[2], m_tool_marker.get_alpha()));

    m_tool_marker.render();

    glsafe(glBlendFunc(GL_SRC_ALPHA, curr_blend_func));
    if (!curr_blend)
        glsafe(glDisable(GL_BLEND));
    glsafe(glDepthMask(curr_depth_mask));
    if (!curr_cull_face)
        glsafe(glEnable(GL_CULL_FACE));

    glsafe(glUseProgram(curr_shader));
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
