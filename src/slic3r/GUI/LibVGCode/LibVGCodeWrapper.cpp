///|/ Copyright (c) Prusa Research 2020 - 2023 Enrico Turri @enricoturri1966
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "libslic3r/libslic3r.h"
#include "LibVGCodeWrapper.hpp"

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#if ENABLE_NEW_GCODE_VIEWER
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#include "libslic3r/Print.hpp"

namespace libvgcode {

Vec3 convert(const Slic3r::Vec3f& v)
{
    return { v.x(), v.y(), v.z() };
}

Slic3r::Vec3f convert(const Vec3& v)
{
    return { v[0], v[1], v[2] };
}

Mat4x4 convert(const Slic3r::Matrix4f& m)
{
    Mat4x4 ret;
    std::memcpy(ret.data(), m.data(), 16 * sizeof(float));
    return ret;
}

Slic3r::ColorRGBA convert(const Color& c)
{
    static const float inv_255 = 1.0f / 255.0f;
    return { c[0] * inv_255, c[1] * inv_255, c[2] * inv_255, 1.0f };
}

Color convert(const Slic3r::ColorRGBA& c)
{
    return { static_cast<uint8_t>(c.r() * 255.0f), static_cast<uint8_t>(c.g() * 255.0f), static_cast<uint8_t>(c.b() * 255.0f) };
}

Slic3r::GCodeExtrusionRole convert(EGCodeExtrusionRole role)
{
    switch (role)
    {
    case EGCodeExtrusionRole::None:                     { return Slic3r::GCodeExtrusionRole::None; }
    case EGCodeExtrusionRole::Perimeter:                { return Slic3r::GCodeExtrusionRole::Perimeter; }
    case EGCodeExtrusionRole::ExternalPerimeter:        { return Slic3r::GCodeExtrusionRole::ExternalPerimeter; }
    case EGCodeExtrusionRole::OverhangPerimeter:        { return Slic3r::GCodeExtrusionRole::OverhangPerimeter; }
    case EGCodeExtrusionRole::InternalInfill:           { return Slic3r::GCodeExtrusionRole::InternalInfill; }
    case EGCodeExtrusionRole::SolidInfill:              { return Slic3r::GCodeExtrusionRole::SolidInfill; }
    case EGCodeExtrusionRole::TopSolidInfill:           { return Slic3r::GCodeExtrusionRole::TopSolidInfill; }
    case EGCodeExtrusionRole::Ironing:                  { return Slic3r::GCodeExtrusionRole::Ironing; }
    case EGCodeExtrusionRole::BridgeInfill:             { return Slic3r::GCodeExtrusionRole::BridgeInfill; }
    case EGCodeExtrusionRole::GapFill:                  { return Slic3r::GCodeExtrusionRole::GapFill; }
    case EGCodeExtrusionRole::Skirt:                    { return Slic3r::GCodeExtrusionRole::Skirt; }
    case EGCodeExtrusionRole::SupportMaterial:          { return Slic3r::GCodeExtrusionRole::SupportMaterial; }
    case EGCodeExtrusionRole::SupportMaterialInterface: { return Slic3r::GCodeExtrusionRole::SupportMaterialInterface; }
    case EGCodeExtrusionRole::WipeTower:                { return Slic3r::GCodeExtrusionRole::WipeTower; }
    case EGCodeExtrusionRole::Custom:                   { return Slic3r::GCodeExtrusionRole::Custom; }
    default:                                            { return Slic3r::GCodeExtrusionRole::None; }
    }
}

EGCodeExtrusionRole convert(Slic3r::GCodeExtrusionRole role)
{
    switch (role)
    {
    case Slic3r::GCodeExtrusionRole::None:                     { return EGCodeExtrusionRole::None; }
    case Slic3r::GCodeExtrusionRole::Perimeter:                { return EGCodeExtrusionRole::Perimeter; }
    case Slic3r::GCodeExtrusionRole::ExternalPerimeter:        { return EGCodeExtrusionRole::ExternalPerimeter; }
    case Slic3r::GCodeExtrusionRole::OverhangPerimeter:        { return EGCodeExtrusionRole::OverhangPerimeter; }
    case Slic3r::GCodeExtrusionRole::InternalInfill:           { return EGCodeExtrusionRole::InternalInfill; }
    case Slic3r::GCodeExtrusionRole::SolidInfill:              { return EGCodeExtrusionRole::SolidInfill; }
    case Slic3r::GCodeExtrusionRole::TopSolidInfill:           { return EGCodeExtrusionRole::TopSolidInfill; }
    case Slic3r::GCodeExtrusionRole::Ironing:                  { return EGCodeExtrusionRole::Ironing; }
    case Slic3r::GCodeExtrusionRole::BridgeInfill:             { return EGCodeExtrusionRole::BridgeInfill; }
    case Slic3r::GCodeExtrusionRole::GapFill:                  { return EGCodeExtrusionRole::GapFill; }
    case Slic3r::GCodeExtrusionRole::Skirt:                    { return EGCodeExtrusionRole::Skirt; }
    case Slic3r::GCodeExtrusionRole::SupportMaterial:          { return EGCodeExtrusionRole::SupportMaterial; }
    case Slic3r::GCodeExtrusionRole::SupportMaterialInterface: { return EGCodeExtrusionRole::SupportMaterialInterface; }
    case Slic3r::GCodeExtrusionRole::WipeTower:                { return EGCodeExtrusionRole::WipeTower; }
    case Slic3r::GCodeExtrusionRole::Custom:                   { return EGCodeExtrusionRole::Custom; }
    default:                                                   { return EGCodeExtrusionRole::None; }
    }
}

EMoveType convert(Slic3r::EMoveType type)
{
    switch (type)
    {
    case Slic3r::EMoveType::Noop:         { return EMoveType::Noop; }
    case Slic3r::EMoveType::Retract:      { return EMoveType::Retract; }
    case Slic3r::EMoveType::Unretract:    { return EMoveType::Unretract; }
    case Slic3r::EMoveType::Seam:         { return EMoveType::Seam; }
    case Slic3r::EMoveType::Tool_change:  { return EMoveType::ToolChange; }
    case Slic3r::EMoveType::Color_change: { return EMoveType::ColorChange; }
    case Slic3r::EMoveType::Pause_Print:  { return EMoveType::PausePrint; }
    case Slic3r::EMoveType::Custom_GCode: { return EMoveType::CustomGCode; }
    case Slic3r::EMoveType::Travel:       { return EMoveType::Travel; }
    case Slic3r::EMoveType::Wipe:         { return EMoveType::Wipe; }
    case Slic3r::EMoveType::Extrude:      { return EMoveType::Extrude; }
    default:                              { return EMoveType::COUNT; }
    }
}

EOptionType convert(const Slic3r::GUI::Preview::OptionType& type)
{
    switch (type)
    {
    case Slic3r::GUI::Preview::OptionType::Travel:          { return EOptionType::Travels; }
    case Slic3r::GUI::Preview::OptionType::Wipe:            { return EOptionType::Wipes; }
    case Slic3r::GUI::Preview::OptionType::Retractions:     { return EOptionType::Retractions; }
    case Slic3r::GUI::Preview::OptionType::Unretractions:   { return EOptionType::Unretractions; }
    case Slic3r::GUI::Preview::OptionType::Seams:           { return EOptionType::Seams; }
    case Slic3r::GUI::Preview::OptionType::ToolChanges:     { return EOptionType::ToolChanges; }
    case Slic3r::GUI::Preview::OptionType::ColorChanges:    { return EOptionType::ColorChanges; }
    case Slic3r::GUI::Preview::OptionType::PausePrints:     { return EOptionType::PausePrints; }
    case Slic3r::GUI::Preview::OptionType::CustomGCodes:    { return EOptionType::CustomGCodes; }
#if ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
    case Slic3r::GUI::Preview::OptionType::CenterOfGravity: { return EOptionType::COUNT; }
    case Slic3r::GUI::Preview::OptionType::ToolMarker:      { return EOptionType::COUNT; }
#else
    case Slic3r::GUI::Preview::OptionType::CenterOfGravity: { return EOptionType::CenterOfGravity; }
    case Slic3r::GUI::Preview::OptionType::ToolMarker:      { return EOptionType::ToolMarker; }
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
    default:                                                { return EOptionType::COUNT; }
    }
}

ETimeMode convert(const Slic3r::PrintEstimatedStatistics::ETimeMode& mode)
{
    switch (mode)
    {
    case Slic3r::PrintEstimatedStatistics::ETimeMode::Normal:  { return ETimeMode::Normal; }
    case Slic3r::PrintEstimatedStatistics::ETimeMode::Stealth: { return ETimeMode::Stealth; }
    default:                                                   { return ETimeMode::COUNT; }
    }
}

Slic3r::PrintEstimatedStatistics::ETimeMode convert(const ETimeMode& mode)
{
    switch (mode)
    {
    case ETimeMode::Normal:  { return Slic3r::PrintEstimatedStatistics::ETimeMode::Normal; }
    case ETimeMode::Stealth: { return Slic3r::PrintEstimatedStatistics::ETimeMode::Stealth; }
    default:                 { return Slic3r::PrintEstimatedStatistics::ETimeMode::Count; }
    }
}

GCodeInputData convert(const Slic3r::GCodeProcessorResult& result, float travels_radius, float wipes_radius)
{
    const std::vector<Slic3r::GCodeProcessorResult::MoveVertex>& moves = result.moves;
    GCodeInputData ret;
    ret.vertices.reserve(2 * moves.size());
    for (size_t i = 1; i < moves.size(); ++i) {
        const Slic3r::GCodeProcessorResult::MoveVertex& curr = moves[i];
        const Slic3r::GCodeProcessorResult::MoveVertex& prev = moves[i - 1];
        const EMoveType curr_type = convert(curr.type);

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
            extrusion_role = convert(curr.extrusion_role);

        float width;
        float height;
        switch (curr_type)
        {
        case EMoveType::Travel:
        {
            width  = travels_radius;
            height = travels_radius;
            break;
        }
        case EMoveType::Wipe:
        {
            width  = wipes_radius;
            height = wipes_radius;
            break;
        }
        default:
        {
            width  = curr.width;
            height = curr.height;
            break;
        }
        }

        if (type_to_option(curr_type) == libvgcode::EOptionType::COUNT) {
            if (ret.vertices.empty() || prev.type != curr.type || prev.extrusion_role != curr.extrusion_role) {
                // to allow libvgcode to properly detect the start/end of a path we need to add a 'phantom' vertex
                // equal to the current one with the exception of the position, which should match the previous move position,
                // and the times, which are set to zero
#if ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
                const libvgcode::PathVertex vertex = { convert(prev.position), height, width, curr.feedrate, curr.fan_speed,
                    curr.temperature, curr.volumetric_rate(), extrusion_role, curr_type,
                    static_cast<uint32_t>(curr.gcode_id), static_cast<uint32_t>(curr.layer_id),
                    static_cast<uint8_t>(curr.extruder_id), static_cast<uint8_t>(curr.cp_color_id), { 0.0f, 0.0f } };
#else
                const libvgcode::PathVertex vertex = { convert(prev.position), height, width, curr.feedrate, curr.fan_speed,
                    curr.temperature, curr.volumetric_rate(), 0.0f, extrusion_role, curr_type,
                    static_cast<uint32_t>(curr.gcode_id), static_cast<uint32_t>(curr.layer_id),
                    static_cast<uint8_t>(curr.extruder_id), static_cast<uint8_t>(curr.cp_color_id), { 0.0f, 0.0f } };
#endif // ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
                ret.vertices.emplace_back(vertex);
            }
        }

#if ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
        const libvgcode::PathVertex vertex = { convert(curr.position), height, width, curr.feedrate, curr.fan_speed,
            curr.temperature, curr.volumetric_rate(), extrusion_role, curr_type,  static_cast<uint32_t>(curr.gcode_id),
            static_cast<uint32_t>(curr.layer_id), static_cast<uint8_t>(curr.extruder_id), static_cast<uint8_t>(curr.cp_color_id), curr.time };
#else
        const libvgcode::PathVertex vertex = { convert(curr.position), height, width, curr.feedrate, curr.fan_speed,
            curr.temperature, curr.volumetric_rate(), curr.mm3_per_mm * (curr.position - prev.position).norm(),
            extrusion_role, curr_type, static_cast<uint32_t>(curr.gcode_id), static_cast<uint32_t>(curr.layer_id),
            static_cast<uint8_t>(curr.extruder_id), static_cast<uint8_t>(curr.cp_color_id), curr.time };
#endif // ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
        ret.vertices.emplace_back(vertex);
    }
    ret.vertices.shrink_to_fit();

    ret.spiral_vase_mode = result.spiral_vase_mode;

    return ret;
}

static void thick_lines_to_geometry(const Slic3r::Lines& lines, const std::vector<float>& widths, const std::vector<float>& heights,
    float top_z, size_t layer_id, size_t extruder_id, size_t color_id, EGCodeExtrusionRole extrusion_role, bool closed, GCodeInputData& data)
{
    if (lines.empty())
        return;

    // loop once more in case of closed loops
    const size_t lines_end = closed ? (lines.size() + 1) : lines.size();
    for (size_t ii = 0; ii < lines_end; ++ii) {
        const size_t i = (ii == lines.size()) ? 0 : ii;
        const Slic3r::Line& line = lines[i];
        // first segment of the polyline
        if (ii == 0) {
            // add a dummy vertex at the start, to separate the current line from the others
            const Slic3r::Vec2f a = unscale(line.a).cast<float>();
            libvgcode::PathVertex vertex = { convert(Slic3r::Vec3f(a.x(), a.y(), top_z)), heights[i], widths[i], 0.0f, 0.0f,
                0.0f, 0.0f, extrusion_role, EMoveType::Noop, 0, static_cast<uint32_t>(layer_id),
                static_cast<uint8_t>(extruder_id), static_cast<uint8_t>(color_id), { 0.0f, 0.0f } };
            data.vertices.emplace_back(vertex);
            // add the starting vertex of the segment
            vertex.type = EMoveType::Extrude;
            data.vertices.emplace_back(vertex);
            data.vertices.emplace_back(vertex);
        }
        // add the ending vertex of the segment
        const Slic3r::Vec2f b = unscale(line.b).cast<float>();
        const libvgcode::PathVertex vertex = { convert(Slic3r::Vec3f(b.x(), b.y(), top_z)), heights[i], widths[i], 0.0f, 0.0f,
            0.0f, 0.0f, extrusion_role, EMoveType::Extrude, 0, static_cast<uint32_t>(layer_id),
            static_cast<uint8_t>(extruder_id), static_cast<uint8_t>(color_id), { 0.0f, 0.0f } };
        data.vertices.emplace_back(vertex);
    }
}

//static void thick_lines_to_verts(const Slic3r::Lines& lines, const std::vector<double>& widths, const std::vector<double>& heights, bool closed,
//    double top_z, GCodeInputData& data)
//{
//    thick_lines_to_geometry(lines, widths, heights, closed, top_z, data);
//}


static void convert(const Slic3r::ExtrusionPath& extrusion_path, float print_z, size_t layer_id, size_t extruder_id, size_t color_id,
    EGCodeExtrusionRole extrusion_role, const Slic3r::Point& shift, GCodeInputData& data)
{
    Slic3r::Polyline polyline = extrusion_path.polyline;
    polyline.remove_duplicate_points();
    polyline.translate(shift);
    const Slic3r::Lines lines = polyline.lines();
    std::vector<float> widths(lines.size(), extrusion_path.width());
    std::vector<float> heights(lines.size(), extrusion_path.height());
    thick_lines_to_geometry(lines, widths, heights, print_z, layer_id, extruder_id, color_id, extrusion_role, false, data);
}

static void convert(const Slic3r::ExtrusionMultiPath& extrusion_multi_path, float print_z, size_t layer_id, size_t extruder_id,
    size_t color_id, EGCodeExtrusionRole extrusion_role, const Slic3r::Point& shift, GCodeInputData& data)
{
    Slic3r::Lines lines;
    std::vector<float> widths;
    std::vector<float> heights;
    for (const Slic3r::ExtrusionPath& extrusion_path : extrusion_multi_path.paths) {
        Slic3r::Polyline polyline = extrusion_path.polyline;
        polyline.remove_duplicate_points();
        polyline.translate(shift);
        const Slic3r::Lines lines_this = polyline.lines();
        append(lines, lines_this);
        widths.insert(widths.end(), lines_this.size(), extrusion_path.width());
        heights.insert(heights.end(), lines_this.size(), extrusion_path.height());
    }
    thick_lines_to_geometry(lines, widths, heights, print_z, layer_id, extruder_id, color_id, extrusion_role, false, data);
}

static void convert(const Slic3r::ExtrusionLoop& extrusion_loop, float print_z, size_t layer_id, size_t extruder_id, size_t color_id,
    EGCodeExtrusionRole extrusion_role, const Slic3r::Point& shift, GCodeInputData& data)
{
    Slic3r::Lines lines;
    std::vector<float> widths;
    std::vector<float> heights;
    for (const Slic3r::ExtrusionPath& extrusion_path : extrusion_loop.paths) {
        Slic3r::Polyline polyline = extrusion_path.polyline;
        polyline.remove_duplicate_points();
        polyline.translate(shift);
        const Slic3r::Lines lines_this = polyline.lines();
        append(lines, lines_this);
        widths.insert(widths.end(), lines_this.size(), extrusion_path.width());
        heights.insert(heights.end(), lines_this.size(), extrusion_path.height());
    }
    thick_lines_to_geometry(lines, widths, heights, print_z, layer_id, extruder_id, color_id, extrusion_role, true, data);
}

// forward declaration
static void convert(const Slic3r::ExtrusionEntityCollection& extrusion_entity_collection, float print_z, size_t layer_id,
    EGCodeExtrusionRole extrusion_role, const Slic3r::Point& shift, GCodeInputData& data);

static void convert(const Slic3r::ExtrusionEntity& extrusion_entity, float print_z, size_t layer_id, size_t extruder_id, size_t color_id,
    EGCodeExtrusionRole extrusion_role, const Slic3r::Point& shift, GCodeInputData& data)
{
    auto* extrusion_path = dynamic_cast<const Slic3r::ExtrusionPath*>(&extrusion_entity);
    if (extrusion_path != nullptr)
        convert(*extrusion_path, print_z, layer_id, extruder_id, color_id, extrusion_role, shift, data);
    else {
        auto* extrusion_loop = dynamic_cast<const Slic3r::ExtrusionLoop*>(&extrusion_entity);
        if (extrusion_loop != nullptr)
            convert(*extrusion_loop, print_z, layer_id, extruder_id, color_id, extrusion_role, shift, data);
        else {
            auto* extrusion_multi_path = dynamic_cast<const Slic3r::ExtrusionMultiPath*>(&extrusion_entity);
            if (extrusion_multi_path != nullptr)
                convert(*extrusion_multi_path, print_z, layer_id, extruder_id, color_id, extrusion_role, shift, data);
            else {
                auto* extrusion_entity_collection = dynamic_cast<const Slic3r::ExtrusionEntityCollection*>(&extrusion_entity);
                if (extrusion_entity_collection != nullptr)
                   convert(*extrusion_entity_collection, print_z, layer_id, extruder_id, color_id, extrusion_role, shift, data);
                else
                    throw Slic3r::RuntimeError("Found unexpected extrusion_entity type");
            }
        }
    }
}

static void convert(const Slic3r::ExtrusionEntityCollection& extrusion_entity_collection, float print_z, size_t layer_id,
    size_t extruder_id, size_t color_id, EGCodeExtrusionRole extrusion_role, const Slic3r::Point& shift, GCodeInputData& data)
{
    for (const Slic3r::ExtrusionEntity* extrusion_entity : extrusion_entity_collection.entities) {
        if (extrusion_entity != nullptr)
            convert(*extrusion_entity, print_z, layer_id, extruder_id, color_id, extrusion_role, shift, data);
    }
}

static void convert_brim_skirt(const Slic3r::Print& print, GCodeInputData& data)
{
//    auto start_time = std::chrono::high_resolution_clock::now();

    // number of skirt layers
    size_t total_layer_count = 0;
    for (const Slic3r::PrintObject* print_object : print.objects()) {
        total_layer_count = std::max(total_layer_count, print_object->total_layer_count());
    }
    size_t skirt_height = print.has_infinite_skirt() ? total_layer_count : std::min<size_t>(print.config().skirt_height.value, total_layer_count);
    if (skirt_height == 0 && print.has_brim())
        skirt_height = 1;

    // Get first skirt_height layers.
    //FIXME This code is fishy. It may not work for multiple objects with different layering due to variable layer height feature.
    // This is not critical as this is just an initial preview.
    const Slic3r::PrintObject* highest_object = *std::max_element(print.objects().begin(), print.objects().end(),
        [](auto l, auto r) { return l->layers().size() < r->layers().size(); });
    std::vector<float> print_zs;
    print_zs.reserve(skirt_height * 2);
    for (size_t i = 0; i < std::min(skirt_height, highest_object->layers().size()); ++i) {
        print_zs.emplace_back(float(highest_object->layers()[i]->print_z));
    }
    // Only add skirt for the raft layers.
    for (size_t i = 0; i < std::min(skirt_height, std::min(highest_object->slicing_parameters().raft_layers(), highest_object->support_layers().size())); ++i) {
        print_zs.emplace_back(float(highest_object->support_layers()[i]->print_z));
    }
    Slic3r::sort_remove_duplicates(print_zs);
    skirt_height = std::min(skirt_height, print_zs.size());
    print_zs.erase(print_zs.begin() + skirt_height, print_zs.end());

    for (size_t i = 0; i < skirt_height; ++i) {
        if (i == 0)
            convert(print.brim(), print_zs[i], i, 0, 0, EGCodeExtrusionRole::Skirt, Slic3r::Point(0, 0), data);
        convert(print.skirt(), print_zs[i], i, 0, 0, EGCodeExtrusionRole::Skirt, Slic3r::Point(0, 0), data);
    }

//    auto end_time = std::chrono::high_resolution_clock::now();
//    std::cout << "convert_brim_skirt: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << "ms\n";
}

class WipeTowerHelper
{
public:
    WipeTowerHelper(const Slic3r::Print& print)
    : m_print(print)
    {
        const Slic3r::PrintConfig& config = m_print.config();
        const Slic3r::WipeTowerData& wipe_tower_data = m_print.wipe_tower_data();
        if (wipe_tower_data.priming && config.single_extruder_multi_material_priming) {
            for (size_t i = 0; i < wipe_tower_data.priming.get()->size(); ++i) {
                m_priming.emplace_back(wipe_tower_data.priming.get()->at(i));
            }
        }
        if (wipe_tower_data.final_purge)
            m_final.emplace_back(*wipe_tower_data.final_purge.get());

        m_angle = config.wipe_tower_rotation_angle.value / 180.0f * PI;
        m_position = Slic3r::Vec2f(config.wipe_tower_x.value, config.wipe_tower_y.value);
        m_layers_count = wipe_tower_data.tool_changes.size() + (m_priming.empty() ? 0 : 1);
    }

    const std::vector<Slic3r::WipeTower::ToolChangeResult>& tool_change(size_t idx) {
        const auto& tool_changes = m_print.wipe_tower_data().tool_changes;
        return m_priming.empty() ?
            ((idx == tool_changes.size()) ? m_final : tool_changes[idx]) :
            ((idx == 0) ? m_priming : (idx == tool_changes.size() + 1) ? m_final : tool_changes[idx - 1]);
    }

    float get_angle() const { return m_angle; }
    const Slic3r::Vec2f& get_position() const { return m_position; }
    size_t get_layers_count() { return m_layers_count; }

private:
    const Slic3r::Print& m_print;
    std::vector<Slic3r::WipeTower::ToolChangeResult> m_priming;
    std::vector<Slic3r::WipeTower::ToolChangeResult> m_final;
    Slic3r::Vec2f m_position{ Slic3r::Vec2f::Zero() };
    float m_angle{ 0.0f };
    size_t m_layers_count{ 0 };
};

static void convert_wipe_tower(const Slic3r::Print& print, const std::vector<std::string>& str_tool_colors, GCodeInputData& data)
{
//    auto start_time = std::chrono::high_resolution_clock::now();

    WipeTowerHelper wipe_tower(print);
    const float angle = wipe_tower.get_angle();
    const Slic3r::Vec2f& position = wipe_tower.get_position();

    for (size_t item = 0; item < wipe_tower.get_layers_count(); ++item) {
        const std::vector<Slic3r::WipeTower::ToolChangeResult>& layer = wipe_tower.tool_change(item);
        for (const Slic3r::WipeTower::ToolChangeResult& extrusions : layer) {
            for (size_t i = 1; i < extrusions.extrusions.size(); /*no increment*/) {
                const Slic3r::WipeTower::Extrusion& e = extrusions.extrusions[i];
                if (e.width == 0.0f) {
                    ++i;
                    continue;
                }
                size_t j = i + 1;
                if (str_tool_colors.empty())
                    for (; j < extrusions.extrusions.size() && extrusions.extrusions[j].width > 0.0f; ++j);
                else
                    for (; j < extrusions.extrusions.size() && extrusions.extrusions[j].tool == e.tool && extrusions.extrusions[j].width > 0.0f; ++j);

                const size_t n_lines = j - i;
                Slic3r::Lines lines;
                std::vector<float> widths;
                std::vector<float> heights;
                lines.reserve(n_lines);
                widths.reserve(n_lines);
                heights.assign(n_lines, extrusions.layer_height);
                Slic3r::WipeTower::Extrusion e_prev = extrusions.extrusions[i - 1];

                if (!extrusions.priming) { // wipe tower extrusions describe the wipe tower at the origin with no rotation
                    e_prev.pos = Eigen::Rotation2Df(angle) * e_prev.pos;
                    e_prev.pos += position;
                }

                for (; i < j; ++i) {
                    Slic3r::WipeTower::Extrusion ee = extrusions.extrusions[i];
                    assert(ee.width > 0.0f);
                    if (!extrusions.priming) {
                        ee.pos = Eigen::Rotation2Df(angle) * ee.pos;
                        ee.pos += position;
                    }
                    lines.emplace_back(Slic3r::Point::new_scale(e_prev.pos.x(), e_prev.pos.y()), Slic3r::Point::new_scale(ee.pos.x(), ee.pos.y()));
                    widths.emplace_back(ee.width);
                    e_prev = ee;
                }

                thick_lines_to_geometry(lines, widths, heights, extrusions.print_z, item, static_cast<size_t>(e.tool), 0,
                                        EGCodeExtrusionRole::WipeTower, lines.front().a == lines.back().b, data);
            }
        }
    }

//    auto end_time = std::chrono::high_resolution_clock::now();
//    std::cout << "convert_wipe_tower: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << "ms\n";
}

static void convert_object(const Slic3r::PrintObject& object, GCodeInputData& data)
{
//    auto start_time = std::chrono::high_resolution_clock::now();

//    auto end_time = std::chrono::high_resolution_clock::now();
//    std::cout << "convert_object: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << "ms\n";
}

// mapping from Slic3r::Print to libvgcode::GCodeInputData
GCodeInputData convert(const Slic3r::Print& print, const std::vector<std::string>& str_tool_colors)
{
    GCodeInputData ret;

    if (print.is_step_done(Slic3r::psSkirtBrim) && (print.has_skirt() || print.has_brim()))
        convert_brim_skirt(print, ret);

    if (!print.wipe_tower_data().tool_changes.empty() && print.is_step_done(Slic3r::psWipeTower))
        convert_wipe_tower(print, str_tool_colors, ret);

    for (const Slic3r::PrintObject* object : print.objects()) {
        convert_object(*object, ret);
    }

    return ret;
}

} // namespace libvgcode

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // ENABLE_NEW_GCODE_VIEWER
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
