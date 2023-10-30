//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "Layers.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include <assert.h>

namespace libvgcode {

void Layers::update(uint32_t layer_id, uint32_t vertex_id)
{
    if (m_ranges.empty() || layer_id == m_ranges.size()) {
        // this code assumes that gcode paths are sent sequentially, one layer after the other
        assert(layer_id == static_cast<uint32_t>(m_ranges.size()));
        Range& range = m_ranges.emplace_back(Range());
        range.set(vertex_id, vertex_id);
    }
    else {
        Range& range = m_ranges.back();
        range.set(range.get()[0], vertex_id);
    }
}

void Layers::reset()
{
    m_ranges.clear();
}

bool Layers::empty() const
{
    return m_ranges.empty();
}

size_t Layers::size() const
{
    return m_ranges.size();
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
