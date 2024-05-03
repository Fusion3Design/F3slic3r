#include "libslic3r/GCode/SeamShells.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>

namespace Slic3r::Seams::Shells::Impl {

BoundedPolygons project_to_geometry(const Geometry::Extrusions &external_perimeters) {
    BoundedPolygons result;
    result.reserve(external_perimeters.size());

    using std::transform, std::back_inserter;

    transform(
        external_perimeters.begin(), external_perimeters.end(), back_inserter(result),
        [](const Geometry::Extrusion &external_perimeter) {
            const auto [choosen_index, _]{Geometry::pick_closest_bounding_box(
                external_perimeter.bounding_box,
                external_perimeter.island_boundary_bounding_boxes
            )};

            const bool is_hole{choosen_index != 0};
            const Polygon &adjacent_boundary{
                !is_hole ? external_perimeter.island_boundary.contour :
                           external_perimeter.island_boundary.holes[choosen_index - 1]};
            return BoundedPolygon{adjacent_boundary, external_perimeter.island_boundary_bounding_boxes[choosen_index], is_hole};
        }
    );
    return result;
}

std::vector<BoundedPolygons> project_to_geometry(const std::vector<Geometry::Extrusions> &extrusions) {
    std::vector<BoundedPolygons> result(extrusions.size());

    for (std::size_t layer_index{0}; layer_index < extrusions.size(); ++layer_index) {
        result[layer_index] = project_to_geometry(extrusions[layer_index]);
    }

    return result;
}

Shells<Polygon> map_to_shells(
    std::vector<BoundedPolygons> &&layers, const Geometry::Mapping &mapping, const std::size_t shell_count
) {
    Shells<Polygon> result(shell_count);
    for (std::size_t layer_index{0}; layer_index < layers.size(); ++layer_index) {
        BoundedPolygons &perimeters{layers[layer_index]};
        for (std::size_t perimeter_index{0}; perimeter_index < perimeters.size();
             perimeter_index++) {
            Polygon &perimeter{perimeters[perimeter_index].polygon};
            result[mapping[layer_index][perimeter_index]].push_back(
                Slice<Polygon>{std::move(perimeter), layer_index}
            );
        }
    }
    return result;
}
} // namespace Slic3r::Seams::Shells::Impl

namespace Slic3r::Seams::Shells {
Shells<Polygon> create_shells(
    const std::vector<Geometry::Extrusions> &extrusions, const double max_distance
) {
    std::vector<Impl::BoundedPolygons> projected{Impl::project_to_geometry(extrusions)};

    std::vector<std::size_t> layer_sizes;
    layer_sizes.reserve(projected.size());
    for (const Impl::BoundedPolygons &perimeters : projected) {
        layer_sizes.push_back(perimeters.size());
    }

    const auto &[shell_mapping, shell_count]{Geometry::get_mapping(
        layer_sizes,
        [&](const std::size_t layer_index,
            const std::size_t item_index) -> Geometry::MappingOperatorResult {
            const Impl::BoundedPolygons &layer{projected[layer_index]};
            const Impl::BoundedPolygons &next_layer{projected[layer_index + 1]};
            if (next_layer.empty()) {
                return std::nullopt;
            }

            BoundingBoxes next_layer_bounding_boxes;
            for (const Impl::BoundedPolygon &bounded_polygon : next_layer) {
                next_layer_bounding_boxes.emplace_back(bounded_polygon.bounding_box);
            }

            const auto [perimeter_index, distance] = Geometry::pick_closest_bounding_box(
                layer[item_index].bounding_box, next_layer_bounding_boxes
            );

            if (distance > max_distance) {
                return std::nullopt;
            }
            return std::pair{perimeter_index, 1.0 / distance};
        }
    )};

    return Impl::map_to_shells(std::move(projected), shell_mapping, shell_count);
}
} // namespace Slic3r::Seams::Shells
