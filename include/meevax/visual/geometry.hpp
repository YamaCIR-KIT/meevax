#ifndef INCLUDED_MEEVAX_VISUAL_GEOMETRY_HPP
#define INCLUDED_MEEVAX_VISUAL_GEOMETRY_HPP

#include <utility>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/register/point.hpp>

#include <Eigen/Core>

BOOST_GEOMETRY_REGISTER_POINT_2D(Eigen::Vector2d, double, cs::cartesian, x(), y())

namespace meevax::visual
{
  using vector = Eigen::Vector2d;
  using point = vector;

  using polygon = boost::geometry::model::polygon<point>;

  class geometry
  {
    using pointer = point*;

    pointer const position_;

  public:
    polygon extents;

    geometry(pointer p)
      : position_ {p}
    {}

    auto& position()
    {
      return *position_;
    }
  };
} // namespace meevax::visual

#endif // INCLUDED_MEEVAX_VISUAL_GEOMETRY_HPP

