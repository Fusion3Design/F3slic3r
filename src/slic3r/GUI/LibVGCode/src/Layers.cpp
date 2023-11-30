//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "Layers.hpp"

#include "../include/PathVertex.hpp"

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
        if (vertex.type == EMoveType::Extrude && vertex.role != EGCodeExtrusionRole::Custom)
            item.z = vertex.position[2];
        item.range.set(vertex_id, vertex_id);
        item.times = vertex.times;
        item.contains_colorprint_options |= is_colorprint_option(vertex);
    }
    else {
        Item& item = m_items.back();
        if (vertex.type == EMoveType::Extrude && vertex.role != EGCodeExtrusionRole::Custom && item.z != vertex.position[2])
            item.z = vertex.position[2];
        item.range.set_max(vertex_id);
        for (size_t i = 0; i < Time_Modes_Count; ++i) {
            item.times[i] += vertex.times[i];
        }
        item.contains_colorprint_options |= is_colorprint_option(vertex);
    }
}

void Layers::reset()
{
    m_items.clear();
    m_view_range.reset();
}

bool Layers::empty() const
{
    return m_items.empty();
}

size_t Layers::count() const
{
    return m_items.size();
}

std::vector<float> Layers::get_times(ETimeMode mode) const
{
    std::vector<float> ret;
    if (mode < ETimeMode::COUNT) {
        for (const Item& item : m_items) {
            ret.emplace_back(item.times[static_cast<size_t>(mode)]);
        }
    }
    return ret;
}

std::vector<float> Layers::get_zs() const
{
    std::vector<float> ret;
    ret.reserve(m_items.size());
    for (const Item& item : m_items) {
        ret.emplace_back(item.z);
    }
    return ret;
}

float Layers::get_layer_time(ETimeMode mode, size_t layer_id) const
{
    return (mode < ETimeMode::COUNT&& layer_id < m_items.size()) ?
        m_items[layer_id].times[static_cast<size_t>(mode)] : 0.0f;
}

float Layers::get_layer_z(size_t layer_id) const
{
  return (layer_id < m_items.size()) ? m_items[layer_id].z : 0.0f;
}

size_t Layers::get_layer_id_at(float z) const
{
    auto iter = std::upper_bound(m_items.begin(), m_items.end(), z, [](float z, const Item& item) { return item.z < z; });
    return std::distance(m_items.begin(), iter);
}

const std::array<uint32_t, 2>& Layers::get_view_range() const
{
    return m_view_range.get();
}

void Layers::set_view_range(const std::array<uint32_t, 2>& range)
{
    set_view_range(range[0], range[1]);
}

void Layers::set_view_range(uint32_t min, uint32_t max)
{
    m_view_range.set(min, max);
}

bool Layers::layer_contains_colorprint_options(size_t layer_id) const
{
    return (layer_id < m_items.size()) ? m_items[layer_id].contains_colorprint_options : false;
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
