#ifndef slic3r_GCode_ExtrusionOrder_hpp_
#define slic3r_GCode_ExtrusionOrder_hpp_

#include <vector>

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

bool is_overriden(const ExtrusionEntityCollection &eec, const LayerTools &layer_tools, const std::size_t instance_id);

int get_extruder_id(
    const ExtrusionEntityCollection &eec,
    const LayerTools &layer_tools,
    const PrintRegion &region,
    const std::size_t instance_id
);

using ExtractEntityPredicate = std::function<bool(const ExtrusionEntityCollection&, const PrintRegion&)>;

ExtrusionEntitiesPtr extract_infill_extrusions(
    const Layer *layer,
    const PrintRegion &region,
    const ExtrusionEntityCollection &fills,
    LayerExtrusionRanges::const_iterator begin,
    LayerExtrusionRanges::const_iterator end,
    const ExtractEntityPredicate &predicate
);

std::vector<ExtrusionEntity *> extract_perimeter_extrusions(
    const Print &print,
    const Layer *layer,
    const LayerIsland &island,
    const ExtractEntityPredicate &predicate
);

std::vector<ExtrusionEntityReference> sort_fill_extrusions(const ExtrusionEntitiesPtr &fills, const Point* start_near);

std::vector<InfillRange> extract_infill_ranges(
    const Print &print,
    const Layer *layer,
    const LayerIsland island,
    std::optional<Point> previous_position,
    const ExtractEntityPredicate &predicate
);

void place_seams(
    const Layer *layer, const Seams::Placer &seam_placer, const std::vector<ExtrusionEntity *> &perimeters, std::optional<Point> previous_position, const bool spiral_vase
);

std::optional<Point> get_last_position(const std::vector<InfillRange> &infill_ranges);

std::optional<Point> get_last_position(const ExtrusionEntityReferences &extrusions);

std::optional<Point> get_last_position(const std::vector<ExtrusionEntity *> &perimeters);

std::optional<Point> get_last_position(const std::vector<SliceExtrusions> &slice_extrusions, const bool infill_first);

std::vector<IslandExtrusions> extract_island_extrusions(
    const LayerSlice &lslice,
    const Print &print,
    const Layer *layer,
    const Seams::Placer &seam_placer,
    const bool spiral_vase,
    const ExtractEntityPredicate &predicate,
    std::optional<Point> &previous_position
);

std::vector<InfillRange> extract_ironing_extrusions(
    const LayerSlice &lslice,
    const Print &print,
    const Layer *layer,
    const ExtractEntityPredicate &predicate,
    std::optional<Point> &previous_position
);

std::vector<SliceExtrusions> get_slices_extrusions(
    const Print &print,
    const Layer *layer,
    const Seams::Placer &seam_placer,
    std::optional<Point> previous_position,
    const bool spiral_vase,
    const ExtractEntityPredicate &predicate
);

unsigned translate_support_extruder(
    const int configured_extruder,
    const LayerTools &layer_tools,
    const ConfigOptionBools &is_soluable
);

ExtrusionEntityReferences get_support_extrusions(
    const unsigned int extruder_id,
    const GCode::ObjectLayerToPrint &layer_to_print,
    unsigned int support_extruder,
    unsigned int interface_extruder
);

std::vector<std::vector<SliceExtrusions>> get_overriden_extrusions(
    const Print &print,
    const GCode::ObjectsLayerToPrint &layers,
    const LayerTools &layer_tools,
    const std::vector<InstanceToPrint> &instances_to_print,
    const Seams::Placer &seam_placer,
    const bool spiral_vase,
    const unsigned int extruder_id,
    std::optional<Point> &previous_position
);

std::vector<NormalExtrusions> get_normal_extrusions(
    const Print &print,
    const GCode::ObjectsLayerToPrint &layers,
    const LayerTools &layer_tools,
    const std::vector<InstanceToPrint> &instances_to_print,
    const Seams::Placer &seam_placer,
    const bool spiral_vase,
    const unsigned int extruder_id,
    std::optional<Point> &previous_position
);

}

#endif // slic3r_GCode_ExtrusionOrder_hpp_
