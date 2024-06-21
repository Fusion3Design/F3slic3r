#ifndef slic3r_GCode_ExtrusionOrder_hpp_
#define slic3r_GCode_ExtrusionOrder_hpp_

#include <vector>

#include "libslic3r/GCode/SmoothPath.hpp"
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
    std::vector<SmoothPath> items;
    const PrintRegion *region;
};

struct Perimeter {
    GCode::SmoothPath smooth_path;
    bool reversed;
    const ExtrusionEntity *extrusion_entity;
};

struct IslandExtrusions {
    const PrintRegion *region;
    std::vector<Perimeter> perimeters;
    std::vector<InfillRange> infill_ranges;
    bool infill_first{false};
};

struct SliceExtrusions {
    std::vector<IslandExtrusions> common_extrusions;
    std::vector<InfillRange> ironing_extrusions;
};

struct SupportPath {
    SmoothPath path;
    bool is_interface;
};

struct NormalExtrusions {
    Point instance_offset;
    std::vector<SupportPath> support_extrusions;
    std::vector<SliceExtrusions> slices_extrusions;
};

struct OverridenExtrusions {
    Point instance_offset;
    std::vector<SliceExtrusions> slices_extrusions;
};

struct InstancePoint {
    Point local_point;
};

using PathSmoothingFunction = std::function<SmoothPath(
    const Layer *, const ExtrusionEntityReference &, const unsigned extruder_id, std::optional<InstancePoint> &previous_position
)>;

struct BrimPath {
    SmoothPath path;
    bool is_loop;
};

struct ExtruderExtrusions {
    unsigned extruder_id;
    std::vector<std::pair<std::size_t, GCode::SmoothPath>> skirt;
    std::vector<BrimPath> brim;
    std::vector<OverridenExtrusions> overriden_extrusions;
    std::vector<NormalExtrusions> normal_extrusions;
    std::optional<Point> wipe_tower_start;
};

static constexpr const double min_gcode_segment_length = 0.002;

bool is_empty(const std::vector<SliceExtrusions> &extrusions);

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
);

inline std::optional<InstancePoint> get_first_point(const SmoothPath &path) {
    for (const SmoothPathElement & element : path) {
        if (!element.path.empty()) {
            return InstancePoint{element.path.front().point};
        }
    }
    return std::nullopt;
}

inline std::optional<InstancePoint> get_first_point(const std::vector<SmoothPath> &smooth_paths) {
    for (const SmoothPath &path : smooth_paths) {
        if (auto result = get_first_point(path)) {
            return result;
        }
    }
    return std::nullopt;
}

inline std::optional<InstancePoint> get_first_point(const std::vector<InfillRange> &infill_ranges) {
    for (const InfillRange &infill_range : infill_ranges) {
        if (auto result = get_first_point(infill_range.items)) {
            return result;
        }
    }
    return std::nullopt;
}

inline std::optional<InstancePoint> get_first_point(const std::vector<Perimeter> &perimeters) {
    for (const Perimeter &perimeter : perimeters) {
        if (auto result = get_first_point(perimeter.smooth_path)) {
            return result;
        }
    }
    return std::nullopt;
}

inline std::optional<InstancePoint> get_first_point(const std::vector<IslandExtrusions> &extrusions) {
    for (const IslandExtrusions &island : extrusions) {
        if (island.infill_first) {
            if (auto result = get_first_point(island.infill_ranges)) {
                return result;
            }
            if (auto result = get_first_point(island.perimeters)) {
                return result;
            }
        } else {
            if (auto result = get_first_point(island.perimeters)) {
                return result;
            }
            if (auto result = get_first_point(island.infill_ranges)) {
                return result;
            }
        }
    }
    return std::nullopt;
}

inline std::optional<InstancePoint> get_first_point(const std::vector<SliceExtrusions> &extrusions) {
    for (const SliceExtrusions &slice : extrusions) {
        if (auto result = get_first_point(slice.common_extrusions)) {
            return result;
        }
    }
    return std::nullopt;
}

inline std::optional<Point> get_first_point(const ExtruderExtrusions &extrusions) {
    for (const BrimPath &brim_path : extrusions.brim) {
        if (auto result = get_first_point(brim_path.path)) {
            return result->local_point;
        };
    }
    for (const auto&[_, path] : extrusions.skirt) {
        if (auto result = get_first_point(path)) {
            return result->local_point;
        };
    }
    for (const OverridenExtrusions &overriden_extrusions : extrusions.overriden_extrusions) {
        if (auto result = get_first_point(overriden_extrusions.slices_extrusions)) {
            return result->local_point - overriden_extrusions.instance_offset;
        }
    }

    for (const NormalExtrusions &normal_extrusions : extrusions.normal_extrusions) {
        for (const SupportPath &support_path : normal_extrusions.support_extrusions) {
            if (auto result = get_first_point(support_path.path)) {
                return result->local_point + normal_extrusions.instance_offset;
            }
        }
        if (auto result = get_first_point(normal_extrusions.slices_extrusions)) {
            return result->local_point + normal_extrusions.instance_offset;
        }
    }

    return std::nullopt;
}

inline std::optional<Point> get_first_point(const std::vector<ExtruderExtrusions> &extrusions) {
    if (extrusions.empty()) {
        return std::nullopt;
    }

    if (extrusions.front().wipe_tower_start) {
        return extrusions.front().wipe_tower_start;
    }

    for (const ExtruderExtrusions &extruder_extrusions : extrusions) {
        if (auto result = get_first_point(extruder_extrusions)) {
            return result;
        }
    }

    return std::nullopt;
}
}

#endif // slic3r_GCode_ExtrusionOrder_hpp_
