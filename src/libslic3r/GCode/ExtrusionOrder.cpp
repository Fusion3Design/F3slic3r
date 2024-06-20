#include "ExtrusionOrder.hpp"
#include "GCode/SmoothPath.hpp"
#include "libslic3r/ShortestPath.hpp"

namespace Slic3r::GCode::ExtrusionOrder {

bool is_overriden(const ExtrusionEntityCollection &eec, const LayerTools &layer_tools, const std::size_t instance_id) {
    return layer_tools.wiping_extrusions().get_extruder_override(&eec, instance_id) > -1;
}

int get_extruder_id(
    const ExtrusionEntityCollection &eec,
    const LayerTools &layer_tools,
    const PrintRegion &region,
    const std::size_t instance_id
) {
    if (is_overriden(eec, layer_tools, instance_id)) {
        return layer_tools.wiping_extrusions().get_extruder_override(&eec, instance_id);
    }

    const int extruder_id = layer_tools.extruder(eec, region);
    if (! layer_tools.has_extruder(extruder_id)) {
        // Extruder is not in layer_tools - we'll print it by last extruder on this layer (could
        // happen e.g. when a wiping object is taller than others - dontcare extruders are
        // eradicated from layer_tools)
        return layer_tools.extruders.back();
    }
    return extruder_id;
}

Point get_gcode_point(const InstancePoint &point, const Point &offset) {
    return point.value + offset;
}

InstancePoint get_instance_point(const Point &point, const Point &offset) {
    return {point - offset};
}

std::optional<Point> get_gcode_point(const std::optional<InstancePoint> &point, const Point &offset) {
    if (point) {
        return get_gcode_point(*point, offset);
    }
    return std::nullopt;
}

std::optional<InstancePoint> get_instance_point(const std::optional<Point> &point, const Point &offset) {
    if (point) {
        return get_instance_point(*point, offset);
    }
    return std::nullopt;
}

std::optional<Point> get_last_position(const ExtrusionEntityReferences &extrusions, const Point &offset) {
    if (!extrusions.empty()) {
        const ExtrusionEntityReference &last_extrusion{extrusions.back()};
        auto last_loop{dynamic_cast<const ExtrusionLoop *>(&last_extrusion.extrusion_entity())};
        if (last_loop != nullptr) {
            return get_gcode_point(InstancePoint{last_loop->seam}, offset);
        }
        const InstancePoint last_point{
            last_extrusion.flipped() ? last_extrusion.extrusion_entity().first_point() :
                                       last_extrusion.extrusion_entity().last_point()};
        return get_gcode_point(last_point, offset);
    }
    return std::nullopt;
}

std::optional<Point> get_last_position(const ExtrusionEntitiesPtr &extrusions, const Point &offset){
    if (!extrusions.empty()) {
        return get_last_position({{*extrusions.back(), false}}, offset);
    }
    return std::nullopt;
}

using ExtractEntityPredicate = std::function<bool(const ExtrusionEntityCollection&, const PrintRegion&)>;

ExtrusionEntitiesPtr extract_infill_extrusions(
    const PrintRegion &region,
    const ExtrusionEntityCollection &fills,
    const LayerExtrusionRanges::const_iterator& begin,
    const LayerExtrusionRanges::const_iterator& end,
    const ExtractEntityPredicate &predicate
) {
    ExtrusionEntitiesPtr result;
    for (auto it = begin; it != end; ++ it) {
        assert(it->region() == begin->region());
        const LayerExtrusionRange &range{*it};
        for (uint32_t fill_id : range) {
            assert(dynamic_cast<ExtrusionEntityCollection*>(fills.entities[fill_id]));

            auto *eec{static_cast<ExtrusionEntityCollection*>(fills.entities[fill_id])};
            if (eec == nullptr || eec->empty() || !predicate(*eec, region)) {
                continue;
            }

            if (eec->can_reverse()) {
                // Flatten the infill collection for better path planning.
                for (auto *ee : eec->entities) {
                    result.emplace_back(ee);
                }
            } else {
                result.emplace_back(eec);
            }
        }
    }
    return result;
}

std::vector<Perimeter> extract_perimeter_extrusions(
    const Print &print,
    const Layer &layer,
    const LayerIsland &island,
    const ExtractEntityPredicate &predicate,
    const unsigned extruder_id,
    const Point &offset,
    std::optional<Point> &previous_position,
    const PathSmoothingFunction &smooth_path
) {
    std::vector<Perimeter> result;

    const LayerRegion &layerm = *layer.get_region(island.perimeters.region());
    const PrintRegion &region = print.get_print_region(layerm.region().print_region_id());

    for (uint32_t perimeter_id : island.perimeters) {
        // Extrusions inside islands are expected to be ordered already.
        // Don't reorder them.
        assert(dynamic_cast<ExtrusionEntityCollection*>(layerm.perimeters().entities[perimeter_id]));
        auto *eec = static_cast<ExtrusionEntityCollection*>(layerm.perimeters().entities[perimeter_id]);
        if (eec == nullptr || eec->empty() || !predicate(*eec, region)) {
            continue;
        }

        for (ExtrusionEntity *ee : *eec) {
            if (ee != nullptr) {
                std::optional<InstancePoint> last_position{get_instance_point(previous_position, offset)};
                bool reverse_loop{false};
                if (auto loop = dynamic_cast<const ExtrusionLoop *>(ee)) {
                    const bool is_hole = loop->is_clockwise();
                    reverse_loop = print.config().prefer_clockwise_movements ? !is_hole : is_hole;
                }
                SmoothPath path{smooth_path(&layer, ExtrusionEntityReference{*ee, reverse_loop}, extruder_id, last_position)};
                previous_position = get_gcode_point(last_position, offset);
                result.push_back(Perimeter{std::move(path), reverse_loop, ee});
            }
        }
    }

    return result;
}

std::vector<ExtrusionEntityReference> sort_fill_extrusions(const ExtrusionEntitiesPtr &fills, const Point* start_near) {
    if (fills.empty()) {
        return {};
    }
    std::vector<ExtrusionEntityReference> sorted_extrusions;

    for (const ExtrusionEntityReference &fill : chain_extrusion_references(fills, start_near)) {
        if (auto *eec = dynamic_cast<const ExtrusionEntityCollection*>(&fill.extrusion_entity()); eec) {
            for (const ExtrusionEntityReference &ee : chain_extrusion_references(*eec, start_near, fill.flipped())) {
                sorted_extrusions.push_back(ee);
            }
        } else {
            sorted_extrusions.push_back(fill);
        }
    }
    return sorted_extrusions;
}

std::vector<InfillRange> extract_infill_ranges(
    const Print &print,
    const Layer &layer,
    const LayerIsland &island,
    const Point &offset,
    std::optional<Point> &previous_position,
    const ExtractEntityPredicate &predicate,
    const PathSmoothingFunction &smooth_path,
    const unsigned extruder_id
) {
    std::vector<InfillRange> result;
    for (auto it = island.fills.begin(); it != island.fills.end();) {
        // Gather range of fill ranges with the same region.
        auto it_end = it;
        for (++ it_end; it_end != island.fills.end() && it->region() == it_end->region(); ++ it_end) ;
        const LayerRegion &layerm = *layer.get_region(it->region());
        // PrintObjects own the PrintRegions, thus the pointer to PrintRegion would be unique to a PrintObject, they would not
        // identify the content of PrintRegion accross the whole print uniquely. Translate to a Print specific PrintRegion.
        const PrintRegion &region = print.get_print_region(layerm.region().print_region_id());

        ExtrusionEntitiesPtr extrusions{extract_infill_extrusions(
            region,
            layerm.fills(),
            it,
            it_end,
            predicate
        )};

        const std::optional<InstancePoint> previous_instance_point{get_instance_point(previous_position, offset)};
        const Point* start_near{previous_instance_point ? &(previous_instance_point->value) : nullptr};
        const ExtrusionEntityReferences sorted_extrusions{sort_fill_extrusions(extrusions, start_near)};

        std::vector<SmoothPath> paths;
        for (const ExtrusionEntityReference &extrusion_reference : sorted_extrusions) {
            std::optional<InstancePoint> last_position{get_instance_point(previous_position, offset)};
            paths.push_back(smooth_path(&layer, extrusion_reference, extruder_id, last_position));
            previous_position = get_gcode_point(last_position, offset);
        }
        result.push_back({std::move(paths), &region});
        it = it_end;
    }
    return result;
}

std::vector<IslandExtrusions> extract_island_extrusions(
    const LayerSlice &lslice,
    const Print &print,
    const Layer &layer,
    const ExtractEntityPredicate &predicate,
    const PathSmoothingFunction &smooth_path,
    const Point &offset,
    const unsigned extruder_id,
    std::optional<Point> &previous_position
) {
    std::vector<IslandExtrusions> result;
    for (const LayerIsland &island : lslice.islands) {
        const LayerRegion &layerm = *layer.get_region(island.perimeters.region());
        // PrintObjects own the PrintRegions, thus the pointer to PrintRegion would be
        // unique to a PrintObject, they would not identify the content of PrintRegion
        // accross the whole print uniquely. Translate to a Print specific PrintRegion.
        const PrintRegion &region = print.get_print_region(layerm.region().print_region_id());

        const auto infill_predicate = [&](const ExtrusionEntityCollection &eec, const PrintRegion &region) {
            return predicate(eec, region) && eec.role() != ExtrusionRole::Ironing;
        };

        result.push_back(IslandExtrusions{&region});
        IslandExtrusions &island_extrusions{result.back()};

        if (print.config().infill_first) {
            island_extrusions.infill_ranges = extract_infill_ranges(
                print, layer, island, offset, previous_position, infill_predicate, smooth_path, extruder_id
            );

            island_extrusions.perimeters = extract_perimeter_extrusions(print, layer, island, predicate, extruder_id, offset, previous_position, smooth_path);
        } else {
            island_extrusions.perimeters = extract_perimeter_extrusions(print, layer, island, predicate, extruder_id, offset, previous_position, smooth_path);

            island_extrusions.infill_ranges = {extract_infill_ranges(
                print, layer, island, offset, previous_position, infill_predicate, smooth_path, extruder_id
            )};
        }
    }
    return result;
}

std::vector<InfillRange> extract_ironing_extrusions(
    const LayerSlice &lslice,
    const Print &print,
    const Layer &layer,
    const ExtractEntityPredicate &predicate,
    const PathSmoothingFunction &smooth_path,
    const Point &offset,
    const unsigned extruder_id,
    std::optional<Point> &previous_position
) {
    std::vector<InfillRange> result;

    for (const LayerIsland &island : lslice.islands) {
        const auto ironing_predicate = [&](const auto &eec, const auto &region) {
            return predicate(eec, region) && eec.role() == ExtrusionRole::Ironing;
        };

        const std::vector<InfillRange> ironing_ranges{extract_infill_ranges(
            print, layer, island, offset, previous_position, ironing_predicate, smooth_path, extruder_id
        )};
        result.insert(
            result.end(), ironing_ranges.begin(), ironing_ranges.end()
        );
    }
    return result;
}

std::vector<SliceExtrusions> get_slices_extrusions(
    const Print &print,
    const Layer &layer,
    const ExtractEntityPredicate &predicate,
    const PathSmoothingFunction &smooth_path,
    const Point &offset,
    const unsigned extruder_id,
    std::optional<Point> &previous_position
) {
    // Note: ironing.
    // FIXME move ironing into the loop above over LayerIslands?
    // First Ironing changes extrusion rate quickly, second single ironing may be done over
    // multiple perimeter regions. Ironing in a second phase is safer, but it may be less
    // efficient.

    std::vector<SliceExtrusions> result;

    for (size_t idx : layer.lslice_indices_sorted_by_print_order) {
        const LayerSlice &lslice = layer.lslices_ex[idx];
        result.emplace_back(SliceExtrusions{
            extract_island_extrusions(
                lslice, print, layer, predicate, smooth_path, offset, extruder_id, previous_position
            ),
            extract_ironing_extrusions(lslice, print, layer, predicate, smooth_path, offset, extruder_id, previous_position)
        });
    }
    return result;
}

unsigned translate_support_extruder(
    const int configured_extruder,
    const LayerTools &layer_tools,
    const ConfigOptionBools &is_soluable
) {
    if (configured_extruder <= 0) {
        // Some support will be printed with "don't care" material, preferably non-soluble.
        // Is the current extruder assigned a soluble filament?
        auto it_nonsoluble = std::find_if(layer_tools.extruders.begin(), layer_tools.extruders.end(),
            [&is_soluable](unsigned int extruder_id) { return ! is_soluable.get_at(extruder_id); });
        // There should be a non-soluble extruder available.
        assert(it_nonsoluble != layer_tools.extruders.end());
        return it_nonsoluble == layer_tools.extruders.end() ? layer_tools.extruders.front() : *it_nonsoluble;
    } else {
        return configured_extruder - 1;
    }
}

ExtrusionEntityReferences get_support_extrusions(
    const unsigned int extruder_id,
    const GCode::ObjectLayerToPrint &layer_to_print,
    unsigned int support_extruder,
    unsigned int interface_extruder
) {
    if (const SupportLayer &support_layer = *layer_to_print.support_layer;
        !support_layer.support_fills.entities.empty()) {
        ExtrusionRole role = support_layer.support_fills.role();
        bool has_support = role.is_mixed() || role.is_support_base();
        bool has_interface = role.is_mixed() || role.is_support_interface();

        bool extrude_support = has_support && support_extruder == extruder_id;
        bool extrude_interface = has_interface && interface_extruder == extruder_id;

        if (extrude_support || extrude_interface) {
            ExtrusionEntitiesPtr entities_cache;
            const ExtrusionEntitiesPtr &entities = extrude_support && extrude_interface ?
                support_layer.support_fills.entities :
                entities_cache;
            if (!extrude_support || !extrude_interface) {
                auto role = extrude_support ? ExtrusionRole::SupportMaterial :
                                              ExtrusionRole::SupportMaterialInterface;
                entities_cache.reserve(support_layer.support_fills.entities.size());
                for (ExtrusionEntity *ee : support_layer.support_fills.entities)
                    if (ee->role() == role)
                        entities_cache.emplace_back(ee);
            }
            return chain_extrusion_references(entities);
        }
    }
    return {};
}

std::vector<std::vector<SliceExtrusions>> get_overriden_extrusions(
    const Print &print,
    const GCode::ObjectsLayerToPrint &layers,
    const LayerTools &layer_tools,
    const std::vector<InstanceToPrint> &instances_to_print,
    const unsigned int extruder_id,
    const PathSmoothingFunction &smooth_path,
    std::optional<Point> &previous_position
) {
    std::vector<std::vector<SliceExtrusions>> result;

    for (const InstanceToPrint &instance : instances_to_print) {
        if (const Layer *layer = layers[instance.object_layer_to_print_id].object_layer; layer) {
            const auto predicate = [&](const ExtrusionEntityCollection &entity_collection,
                                       const PrintRegion &region) {
                if (!is_overriden(entity_collection, layer_tools, instance.instance_id)) {
                    return false;
                }

                if (get_extruder_id(
                        entity_collection, layer_tools, region, instance.instance_id
                    ) != static_cast<int>(extruder_id)) {
                    return false;
                }
                return true;
            };

            const PrintObject &print_object = instance.print_object;
            const Point &offset = print_object.instances()[instance.instance_id].shift;

            result.emplace_back(get_slices_extrusions(
                print, *layer, predicate, smooth_path, offset, extruder_id, previous_position
            ));
        }
    }
    return result;
}

std::vector<NormalExtrusions> get_normal_extrusions(
    const Print &print,
    const GCode::ObjectsLayerToPrint &layers,
    const LayerTools &layer_tools,
    const std::vector<InstanceToPrint> &instances_to_print,
    const unsigned int extruder_id,
    const PathSmoothingFunction &smooth_path,
    std::optional<Point> &previous_position
) {
    std::vector<NormalExtrusions> result;

    for (std::size_t i{0}; i < instances_to_print.size(); ++i) {
        const InstanceToPrint &instance{instances_to_print[i]};
        const PrintObject &print_object = instance.print_object;
        const Point &offset = print_object.instances()[instance.instance_id].shift;

        result.emplace_back();

        if (layers[instance.object_layer_to_print_id].support_layer != nullptr) {
            result.back().support_extrusions = get_support_extrusions(
                extruder_id,
                layers[instance.object_layer_to_print_id],
                translate_support_extruder(instance.print_object.config().support_material_extruder.value, layer_tools, print.config().filament_soluble),
                translate_support_extruder(instance.print_object.config().support_material_interface_extruder.value, layer_tools, print.config().filament_soluble)
            );

            if (const auto last_position = get_last_position(result.back().support_extrusions, offset)) {
                previous_position = last_position;
            }
        }

        if (const Layer *layer = layers[instance.object_layer_to_print_id].object_layer; layer) {
            const auto predicate = [&](const ExtrusionEntityCollection &entity_collection, const PrintRegion &region){
                if (is_overriden(entity_collection, layer_tools, instance.instance_id)) {
                    return false;
                }

                if (get_extruder_id(entity_collection, layer_tools, region, instance.instance_id) != static_cast<int>(extruder_id)) {
                    return false;
                }
                return true;
            };

            result.back().slices_extrusions = get_slices_extrusions(
                print,
                *layer,
                predicate,
                smooth_path,
                offset,
                extruder_id,
                previous_position
            );
        }
    }
    return result;
}

std::vector<ExtruderExtrusions> get_extrusions(
    const Print &print,
    const GCode::WipeTowerIntegration *wipe_tower,
    const GCode::ObjectsLayerToPrint &layers,
    const bool is_first_layer,
    const LayerTools &layer_tools,
    const std::vector<InstanceToPrint> &instances_to_print,
    const std::map<unsigned int, std::pair<size_t, size_t>> &skirt_loops_per_extruder,
    unsigned current_extruder_id,
    const PathSmoothingFunction &smooth_path,
    bool get_brim,
    std::optional<Point> previous_position
) {
    unsigned toolchange_number{0};

    std::vector<ExtruderExtrusions> extrusions;
    for (const unsigned int extruder_id : layer_tools.extruders)
    {
        if (layer_tools.has_wipe_tower && wipe_tower != nullptr) {
            if (is_toolchange_required(is_first_layer, layer_tools.extruders.back(), extruder_id, current_extruder_id)) {
                const WipeTower::ToolChangeResult tool_change{wipe_tower->get_toolchange(toolchange_number++)};
                previous_position = Point::new_scale(wipe_tower->transform_wt_pt(tool_change.end_pos));
                current_extruder_id = tool_change.new_tool;
            }
        }

        ExtruderExtrusions extruder_extrusions{extruder_id};

        if (auto loops_it = skirt_loops_per_extruder.find(extruder_id); loops_it != skirt_loops_per_extruder.end()) {
            const std::pair<size_t, size_t> loops = loops_it->second;
            for (std::size_t i = loops.first; i < loops.second; ++i) {
                const ExtrusionEntityReference entity{*print.skirt().entities[i], false};
                std::optional<InstancePoint> last_position{get_instance_point(previous_position, {0, 0})};
                SmoothPath path{smooth_path(nullptr, entity, extruder_id, last_position)};
                previous_position = get_gcode_point(last_position, {0, 0});
                extruder_extrusions.skirt.emplace_back(i, std::move(path));
            }
        }

        // Extrude brim with the extruder of the 1st region.
        using GCode::ExtrusionOrder::get_last_position;
        if (get_brim) {
            for (const ExtrusionEntity *entity : print.brim().entities) {
                const ExtrusionEntityReference entity_reference{*entity, false};
                std::optional<InstancePoint> last_position{get_instance_point(previous_position, {0, 0})};
                SmoothPath path{smooth_path(nullptr, entity_reference, extruder_id, last_position)};
                previous_position = get_gcode_point(last_position, {0, 0});
                const bool is_loop{dynamic_cast<const ExtrusionLoop *>(entity) != nullptr};
                extruder_extrusions.brim.push_back({std::move(path), is_loop});
            }
            get_brim = false;
        }

        using GCode::ExtrusionOrder::get_overriden_extrusions;
        bool is_anything_overridden = layer_tools.wiping_extrusions().is_anything_overridden();
        if (is_anything_overridden) {
            extruder_extrusions.overriden_extrusions = get_overriden_extrusions(
                print, layers, layer_tools, instances_to_print, extruder_id, smooth_path,
                previous_position
            );
        }

        using GCode::ExtrusionOrder::get_normal_extrusions;
        extruder_extrusions.normal_extrusions = get_normal_extrusions(
            print, layers, layer_tools, instances_to_print, extruder_id, smooth_path,
            previous_position
        );
        extrusions.push_back(std::move(extruder_extrusions));
    }
    return extrusions;
}

}
