//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "Layers.hpp"

#include "PathVertex.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include <assert.h>

namespace libvgcode {

static bool is_colorprint_option(const PathVertex& v)
{
    return v.type == EMoveType::PausePrint || v.type == EMoveType::CustomGCode;
}

void Layers::update(const PathVertex& vertex, uint32_t vertex_id)
{

    if (m_items.empty() || vertex.layer_id == m_items.size()) {
        // this code assumes that gcode paths are sent sequentially, one layer after the other
        assert(vertex.layer_id == static_cast<uint32_t>(m_items.size()));
        Item& item = m_items.emplace_back(Item());
        item.range.set(vertex_id, vertex_id);
        item.times = vertex.times;
        item.contains_colorprint_options |= is_colorprint_option(vertex);
    }
    else {
        Item& item = m_items.back();
        item.range.set(item.range.get()[0], vertex_id);
        for (size_t i = 0; i < static_cast<size_t>(ETimeMode::COUNT); ++i) {
            item.times[i] += vertex.times[i];
        }
        item.contains_colorprint_options |= is_colorprint_option(vertex);
    }
}

void Layers::reset()
{
    m_items.clear();
}

bool Layers::empty() const
{
    return m_items.empty();
}

size_t Layers::count() const
{
    return m_items.size();
}

float Layers::get_time(ETimeMode mode, uint32_t layer_id) const
{
    return (mode < ETimeMode::COUNT&& layer_id < static_cast<uint32_t>(m_items.size())) ?
        m_items[layer_id].times[static_cast<size_t>(mode)] : 0.0f;
}

std::vector<float> Layers::get_times(ETimeMode mode) const
{
    std::vector<float> ret;
    if (mode < ETimeMode::COUNT) {
        const size_t mode_id = static_cast<size_t>(mode);
        for (const Item& item : m_items) {
            ret.emplace_back(item.times[mode_id]);
        }
    }
    return ret;
}

bool Layers::layer_contains_colorprint_options(uint32_t layer_id) const
{
    return (layer_id < static_cast<uint32_t>(m_items.size())) ? m_items[layer_id].contains_colorprint_options : false;
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
