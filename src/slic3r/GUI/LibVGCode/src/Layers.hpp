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

#include "../include/Types.hpp"
#include "Range.hpp"

namespace libvgcode {

struct PathVertex;

class Layers
{
public:
		void update(const PathVertex& vertex, uint32_t vertex_id);
		void reset();

		bool empty() const;
		size_t count() const;

		std::vector<float> get_times(ETimeMode mode) const;
		std::vector<float> get_zs() const;

		float get_layer_time(ETimeMode mode, size_t layer_id) const;
		float get_layer_z(size_t layer_id) const;
		size_t get_layer_id_at(float z) const;

		const std::array<uint32_t, 2>& get_view_range() const;
		void set_view_range(const std::array<uint32_t, 2>& range);
		void set_view_range(uint32_t min, uint32_t max);

		bool layer_contains_colorprint_options(size_t layer_id) const;

private:
		struct Item
		{
				float z{ 0.0f };
				Range range;
				std::array<float, Time_Modes_Count> times{ 0.0f, 0.0f };
				bool contains_colorprint_options{ false };
		};

		std::vector<Item> m_items;
		Range m_view_range;
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_LAYERS_HPP