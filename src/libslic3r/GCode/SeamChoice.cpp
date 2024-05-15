#include "libslic3r/GCode/SeamChoice.hpp"

namespace Slic3r::Seams {
std::optional<SeamChoice> maybe_choose_seam_point(
    const Perimeters::Perimeter &perimeter, const SeamPicker &seam_picker
) {
    using Perimeters::PointType;
    using Perimeters::PointClassification;

    std::vector<PointType>
        type_search_order{PointType::enforcer, PointType::common, PointType::blocker};
    std::vector<PointClassification> classification_search_order{
        PointClassification::embedded, PointClassification::common, PointClassification::overhang};
    for (const PointType point_type : type_search_order) {
        for (const PointClassification point_classification : classification_search_order) {
            if (std::optional<SeamChoice> seam_choice{
                    seam_picker(perimeter, point_type, point_classification)}) {
                return seam_choice;
            }
        }
        if (!Perimeters::extract_points(perimeter, point_type).empty()) {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

SeamChoice choose_seam_point(const Perimeters::Perimeter &perimeter, const SeamPicker &seam_picker) {
    using Perimeters::PointType;
    using Perimeters::PointClassification;

    std::optional<SeamChoice> seam_choice{maybe_choose_seam_point(perimeter, seam_picker)};

    if (seam_choice) {
        return *seam_choice;
    }

    // Failed to choose any reasonable point!
    return SeamChoice{0, 0, perimeter.positions.front()};
}

std::optional<SeamChoice> choose_degenerate_seam_point(const Perimeters::Perimeter &perimeter) {
    if (!perimeter.positions.empty()) {
        return SeamChoice{0, 0, perimeter.positions.front()};
    }
    return std::nullopt;
}

std::optional<std::vector<SeamChoice>> maybe_get_shell_seam(
    const Shells::Shell<> &shell,
    const std::function<std::optional<SeamChoice>(const Perimeters::Perimeter &, std::size_t)> &chooser
) {
    std::vector<SeamChoice> result;
    result.reserve(shell.size());
    for (std::size_t i{0}; i < shell.size(); ++i) {
        const Shells::Slice<> &slice{shell[i]};
        if (slice.boundary.is_degenerate) {
            if (std::optional<SeamChoice> seam_choice{
                    choose_degenerate_seam_point(slice.boundary)}) {
                result.push_back(*seam_choice);
            } else {
                result.emplace_back();
            }
        } else {
            const std::optional<SeamChoice> choice{chooser(slice.boundary, i)};
            if (!choice) {
                return std::nullopt;
            }
            result.push_back(*choice);
        }
    }
    return result;
}

std::vector<SeamChoice> get_shell_seam(
    const Shells::Shell<> &shell,
    const std::function<SeamChoice(const Perimeters::Perimeter &, std::size_t)> &chooser
) {
    std::optional<std::vector<SeamChoice>> seam{maybe_get_shell_seam(
        shell,
        [&](const Perimeters::Perimeter &perimeter, std::size_t slice_index) {
            return chooser(perimeter, slice_index);
        }
    )};
    if (!seam) {
        // Should be unreachable as chooser always returns a SeamChoice!
        return std::vector<SeamChoice>(shell.size());
    }
    return *seam;
}

std::vector<std::vector<SeamPerimeterChoice>> get_object_seams(
    Shells::Shells<> &&shells,
    const std::function<std::vector<SeamChoice>(const Shells::Shell<>&)> &get_shell_seam
) {
    std::vector<std::vector<SeamPerimeterChoice>> layer_seams(get_layer_count(shells));

    for (Shells::Shell<> &shell : shells) {
        std::vector<SeamChoice> seam{get_shell_seam(shell)};

        for (std::size_t perimeter_id{}; perimeter_id < shell.size(); ++perimeter_id) {
            const SeamChoice &choice{seam[perimeter_id]};
            Perimeters::Perimeter &perimeter{shell[perimeter_id].boundary};
            layer_seams[shell[perimeter_id].layer_index].emplace_back(choice, std::move(perimeter));
        }
    }

    return layer_seams;
}
} // namespace Slic3r::Seams
