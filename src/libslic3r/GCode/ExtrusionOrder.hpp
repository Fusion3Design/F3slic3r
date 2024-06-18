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

std::vector<ExtruderExtrusions> get_extrusions(
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
);

}

#endif // slic3r_GCode_ExtrusionOrder_hpp_
