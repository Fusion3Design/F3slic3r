#ifndef libslic3r_SeamShells_hpp_
#define libslic3r_SeamShells_hpp_

#include <vector>
#include <tcbspan/span.hpp>

#include "libslic3r/Polygon.hpp"
#include "libslic3r/GCode/SeamGeometry.hpp"

namespace Slic3r {
class Layer;
}

namespace Slic3r::Seams::Perimeters {
struct Perimeter;
}

namespace Slic3r::Seams::Shells::Impl {

struct BoundedPolygon {
    Polygon polygon;
    BoundingBox bounding_box;
    bool is_hole{false};
};

using BoundedPolygons = std::vector<BoundedPolygon>;

BoundedPolygons project_to_geometry(const Geometry::Extrusions &extrusions, const double max_bb_distance);
}

namespace Slic3r::Seams::Shells {
template<typename T = Perimeters::Perimeter> struct Slice
{
    T boundary;
    std::size_t layer_index;
};

template<typename T = Perimeters::Perimeter> using Shell = std::vector<Slice<T>>;

template<typename T = Perimeters::Perimeter> using Shells = std::vector<Shell<T>>;

Shells<Polygon> create_shells(
    const std::vector<Geometry::Extrusions> &extrusions, const double max_distance
);
} // namespace Slic3r::Seams::Shells

#endif // libslic3r_SeamShells_hpp_
