#ifndef slic3r_GCode_ExtrusionOrder_hpp_
#define slic3r_GCode_ExtrusionOrder_hpp_

#include <vector>

#include "libslic3r/GCode/WipeTowerIntegration.hpp"
#include "libslic3r/ExtrusionEntity.hpp"
#include "libslic3r/GCode/SeamPlacer.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/ShortestPath.hpp"

namespace Slic3r::GCode {
// Object and support extrusions of the same PrintObject at the same print_z.
// public, so that it could be accessed by free helper functions from GCode.cpp
struct ObjectLayerToPrint
{
    ObjectLayerToPrint() : object_layer(nullptr), support_layer(nullptr) {}
    const Layer *object_layer;
    const SupportLayer *support_layer;
    const Layer *layer() const { return (object_layer != nullptr) ? object_layer : support_layer; }
    const PrintObject *object() const {
        return (this->layer() != nullptr) ? this->layer()->object() : nullptr;
    }
    coordf_t print_z() const {
        return (object_layer != nullptr && support_layer != nullptr) ?
            0.5 * (object_layer->print_z + support_layer->print_z) :
            this->layer()->print_z;
    }
};

using ObjectsLayerToPrint = std::vector<GCode::ObjectLayerToPrint>;

struct InstanceToPrint
{
    InstanceToPrint(
        size_t object_layer_to_print_id, const PrintObject &print_object, size_t instance_id
    )
        : object_layer_to_print_id(object_layer_to_print_id)
        , print_object(print_object)
        , instance_id(instance_id) {}

    // Index into std::vector<ObjectLayerToPrint>, which contains Object and Support layers for the
    // current print_z, collected for a single object, or for possibly multiple objects with
    // multiple instances.
    const size_t object_layer_to_print_id;
    const PrintObject &print_object;
    // Instance idx of the copy of a print object.
    const size_t instance_id;
};
} // namespace Slic3r::GCode

namespace Slic3r::GCode::ExtrusionOrder {

struct InfillRange {
    std::vector<ExtrusionEntityReference> items;
    const PrintRegion *region;
};

struct IslandExtrusions {
    const PrintRegion *region;
    std::vector<ExtrusionEntity *> perimeters;
    std::vector<InfillRange> infill_ranges;
};

struct SliceExtrusions {
    std::vector<IslandExtrusions> common_extrusions;
    std::vector<InfillRange> ironing_extrusions;
};

struct NormalExtrusions {
    ExtrusionEntityReferences support_extrusions;
    std::vector<SliceExtrusions> slices_extrusions;
};

std::optional<Vec2d> get_last_position(const ExtrusionEntitiesPtr &extrusions, const Vec2d &offset);

using SeamPlacingFunciton = std::function<std::optional<Point>(const Layer &layer, ExtrusionEntity* perimeter, const std::optional<Point>&)>;

std::vector<std::vector<SliceExtrusions>> get_overriden_extrusions(
    const Print &print,
    const GCode::ObjectsLayerToPrint &layers,
    const LayerTools &layer_tools,
    const std::vector<InstanceToPrint> &instances_to_print,
    const unsigned int extruder_id,
    const SeamPlacingFunciton &place_seam,
    std::optional<Vec2d> &previous_position
);

std::vector<NormalExtrusions> get_normal_extrusions(
    const Print &print,
    const GCode::ObjectsLayerToPrint &layers,
    const LayerTools &layer_tools,
    const std::vector<InstanceToPrint> &instances_to_print,
    const unsigned int extruder_id,
    const SeamPlacingFunciton &place_seam,
    std::optional<Vec2d> &previous_position
);

struct ExtruderExtrusions {
    unsigned extruder_id;
    std::vector<std::pair<std::size_t, const ExtrusionEntity *>> skirt;
    ExtrusionEntitiesPtr brim;
    std::vector<std::vector<SliceExtrusions>> overriden_extrusions;
    std::vector<NormalExtrusions> normal_extrusions;
};

inline std::vector<ExtruderExtrusions> get_extrusions(
    const Print &print,
    const GCode::WipeTowerIntegration *wipe_tower,
    const GCode::ObjectsLayerToPrint &layers,
    const bool is_first_layer,
    const LayerTools &layer_tools,
    const std::vector<InstanceToPrint> &instances_to_print,
    const std::map<unsigned int, std::pair<size_t, size_t>> &skirt_loops_per_extruder,
    unsigned current_extruder_id,
    const SeamPlacingFunciton &place_seam,
    bool get_brim,
    std::optional<Vec2d> previous_position
) {
    unsigned toolchange_number{0};

    std::vector<ExtruderExtrusions> extrusions;
    for (const unsigned int extruder_id : layer_tools.extruders)
    {
        if (layer_tools.has_wipe_tower && wipe_tower != nullptr) {
            if (is_toolchange_required(is_first_layer, layer_tools.extruders.back(), extruder_id, current_extruder_id)) {
                const WipeTower::ToolChangeResult tool_change{wipe_tower->get_toolchange(toolchange_number++)};
                previous_position = wipe_tower->transform_wt_pt(tool_change.end_pos).cast<double>();
                current_extruder_id = tool_change.new_tool;
            }
        }

        ExtruderExtrusions extruder_extrusions{extruder_id};

        if (auto loops_it = skirt_loops_per_extruder.find(extruder_id); loops_it != skirt_loops_per_extruder.end()) {
            const std::pair<size_t, size_t> loops = loops_it->second;
            for (std::size_t i = loops.first; i < loops.second; ++i) {
                extruder_extrusions.skirt.emplace_back(i, print.skirt().entities[i]);
            }
        }

        // Extrude brim with the extruder of the 1st region.
        using GCode::ExtrusionOrder::get_last_position;
        if (get_brim) {
            extruder_extrusions.brim = print.brim().entities;
            previous_position = get_last_position(extruder_extrusions.brim, {0.0, 0.0});
            get_brim = false;
        }

        using GCode::ExtrusionOrder::get_overriden_extrusions;
        bool is_anything_overridden = layer_tools.wiping_extrusions().is_anything_overridden();
        if (is_anything_overridden) {
            extruder_extrusions.overriden_extrusions = get_overriden_extrusions(
                print, layers, layer_tools, instances_to_print, extruder_id, place_seam,
                previous_position
            );
        }

        using GCode::ExtrusionOrder::get_normal_extrusions;
        extruder_extrusions.normal_extrusions = get_normal_extrusions(
            print, layers, layer_tools, instances_to_print, extruder_id, place_seam,
            previous_position
        );
        extrusions.push_back(std::move(extruder_extrusions));
    }
    return extrusions;
}

}

#endif // slic3r_GCode_ExtrusionOrder_hpp_
