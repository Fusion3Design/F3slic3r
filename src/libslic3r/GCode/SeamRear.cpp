#include "libslic3r/GCode/SeamRear.hpp"
#include "libslic3r/GCode/SeamGeometry.hpp"

namespace Slic3r::Seams::Rear {
using Perimeters::PointType;
using Perimeters::PointClassification;

namespace Impl {

BoundingBoxf get_bounding_box(const Shells::Shell<> &shell) {
    BoundingBoxf result;
    for (const Shells::Slice<> &slice : shell) {
        result.merge(BoundingBoxf{slice.boundary.positions});
    }
    return result;
}

std::optional<SeamChoice> get_rearest_point(
    const Perimeters::Perimeter &perimeter,
    const PointType point_type,
    const PointClassification point_classification
) {
    double max_y{-std::numeric_limits<double>::infinity()};
    std::optional<std::size_t> choosen_index;
    for (std::size_t i{0}; i < perimeter.positions.size(); ++i) {
        const Perimeters::PointType _point_type{perimeter.point_types[i]};
        const Perimeters::PointClassification _point_classification{perimeter.point_classifications[i]};

        if (point_type == _point_type && point_classification == _point_classification) {
            const Vec2d &position{perimeter.positions[i]};
            if (position.y() > max_y) {
                max_y = position.y();
                choosen_index = i;
            }
        }
    }
    if (choosen_index) {
        return SeamChoice{*choosen_index, *choosen_index, perimeter.positions[*choosen_index]};
    }
    return std::nullopt;
}

std::optional<SeamChoice> StraightLine::operator()(
    const Perimeters::Perimeter &perimeter,
    const PointType point_type,
    const PointClassification point_classification
) const {
    std::vector<PerimeterLine> possible_lines;
    for (std::size_t i{0}; i < perimeter.positions.size() - 1; ++i) {
        if (perimeter.point_types[i] != point_type) {
            continue;
        }
        if (perimeter.point_classifications[i] != point_classification) {
            continue;
        }
        if (perimeter.point_types[i + 1] != point_type) {
            continue;
        }
        if (perimeter.point_classifications[i + 1] != point_classification) {
            continue;
        }
        possible_lines.push_back(PerimeterLine{perimeter.positions[i], perimeter.positions[i+1], i, i + 1});
    }
    if (possible_lines.empty()) {
        return std::nullopt;
    }

    const AABBTreeLines::LinesDistancer<PerimeterLine> possible_distancer{possible_lines};
    const BoundingBoxf bounding_box{perimeter.positions};

    const std::vector<std::pair<Vec2d, std::size_t>> intersections{
        possible_distancer.intersections_with_line<true>(PerimeterLine{
            this->prefered_position, Vec2d{this->prefered_position.x(), bounding_box.min.y()},
            0, 0})};
    if (!intersections.empty()) {
        const auto[position, line_index]{intersections.front()};
        if (position.y() < bounding_box.max.y() -
                this->rear_project_threshold * (bounding_box.max.y() - bounding_box.min.y())) {
            return std::nullopt;
        }
        const PerimeterLine &intersected_line{possible_lines[line_index]};
        const SeamChoice intersected_choice{intersected_line.previous_index, intersected_line.next_index, position};
        return intersected_choice;
    }
    return std::nullopt;
}
} // namespace Impl

std::vector<std::vector<SeamPerimeterChoice>> get_object_seams(
    Shells::Shells<> &&shells,
    const double rear_project_threshold
) {
    double average_x_center{0.0};
    std::size_t count{0};
    for (const Shells::Shell<> &shell : shells) {
        for (const Shells::Slice<> &slice : shell) {
            if (slice.boundary.positions.empty()) {
                continue;
            }
            const BoundingBoxf slice_bounding_box{slice.boundary.positions};
            average_x_center += (slice_bounding_box.min.x() + slice_bounding_box.max.x()) / 2.0;
            count++;
        }
    }
    average_x_center /= count;
    return Seams::get_object_seams(std::move(shells), [&](const Shells::Shell<> &shell) {
        BoundingBoxf bounding_box{Impl::get_bounding_box(shell)};
        const Vec2d back_center{average_x_center, bounding_box.max.y()};
        std::optional<std::vector<SeamChoice>> straight_seam {
            Seams::maybe_get_shell_seam(shell, [&](const Perimeters::Perimeter &perimeter, std::size_t) {
                return Seams::maybe_choose_seam_point(
                    perimeter,
                    Impl::StraightLine{back_center, rear_project_threshold}
                );
            })
        };
        if (!straight_seam) {
            return Seams::get_shell_seam(shell, [&](const Perimeters::Perimeter &perimeter, std::size_t) {
                return Seams::choose_seam_point(perimeter, Impl::get_rearest_point);
            });
        }
        return *straight_seam;
    });
}
} // namespace Slic3r::Seams::Rear
