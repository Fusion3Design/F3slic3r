#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/GCode/SeamPainting.hpp"
#include "test_data.hpp"

#include "libslic3r/GCode/SeamShells.hpp"

using namespace Slic3r;
using namespace Slic3r::Seams;

constexpr bool debug_files{false};

struct ProjectionFixture
{
    Polygon extrusion_path{
        Point{scaled(Vec2d{-1.0, -1.0})}, Point{scaled(Vec2d{1.0, -1.0})},
        Point{scaled(Vec2d{1.0, 1.0})}, Point{scaled(Vec2d{-1.0, 1.0})}};

    ExPolygon island_boundary;
    Seams::Geometry::Extrusions extrusions;
    double extrusion_width{0.2};

    ProjectionFixture() {
        extrusions.emplace_back(extrusion_path.bounding_box(), extrusion_width, island_boundary);
    }
};

TEST_CASE_METHOD(ProjectionFixture, "Project to geometry matches", "[Seams][SeamShells]") {
    Polygon boundary_polygon{extrusion_path};
    // Add + 0.1 to check that boundary polygon has been picked.
    boundary_polygon.scale(1.0 + extrusion_width / 2.0 + 0.1);
    island_boundary.contour = boundary_polygon;

    Shells::Impl::BoundedPolygons result{Shells::Impl::project_to_geometry(extrusions)};
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].polygon.size() == 4);
    // Boundary polygon is picked.
    CHECK(result[0].polygon[0].x() == Approx(scaled(-(1.0 + extrusion_width / 2.0 + 0.1))));
}

void serialize_shells(
    std::ostream &out, const Shells::Shells<Polygon> &shells, const double layer_height
) {
    out << "x,y,z,layer_index,slice_id,shell_id" << std::endl;
    for (std::size_t shell_id{}; shell_id < shells.size(); ++shell_id) {
        const Shells::Shell<Polygon> &shell{shells[shell_id]};
        for (std::size_t slice_id{}; slice_id < shell.size(); ++slice_id) {
            const Shells::Slice<Polygon> &slice{shell[slice_id]};
            for (const Point &point : slice.boundary) {
                // clang-format off
                out
                    << point.x() << ","
                    << point.y() << ","
                    << slice.layer_index * 1e6 * layer_height << ","
                    << slice.layer_index << ","
                    << slice_id << ","
                    << shell_id << std::endl;
                // clang-format on
            }
        }
    }
}

TEST_CASE_METHOD(Test::SeamsFixture, "Create shells", "[Seams][SeamShells][Integration]") {
    if constexpr (debug_files) {
        std::ofstream csv{"shells.csv"};
        serialize_shells(csv, shell_polygons, print->full_print_config().opt_float("layer_height"));
    }

    CHECK(shell_polygons.size() == 39);
}
