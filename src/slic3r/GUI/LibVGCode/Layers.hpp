///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_LAYERS_HPP
#define VGCODE_LAYERS_HPP

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include "Range.hpp"

#include <vector>

namespace libvgcode {

struct PathVertex;

class Layers
{
public:
		void update(const PathVertex& vertex, uint32_t vertex_id);
		void reset();

		bool empty() const;
		size_t get_layers_count() const;

		bool layer_contains_colorprint_options(uint32_t layer_id) const;

private:
		struct Item
		{
				Range range;
				bool contains_colorprint_options{ false };

				Item() = default;
		};

		std::vector<Item> m_items;
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_LAYERS_HPP