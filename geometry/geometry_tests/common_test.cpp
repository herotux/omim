#include "../../base/SRC_FIRST.hpp"
#include "../../base/macros.hpp"

#include "equality.hpp"

#include "../../testing/testing.hpp"

#include "../rect2d.hpp"

using namespace test;

UNIT_TEST(Rect)
{
  m2::Rect<double> rect(0, 0, 500, 300);

  double factor[] = { 0.2, 0.3, 0.5, 0.7, 1.0, 1.3, 1.5, 2.0 };
  for (size_t i = 0; i < ARRAY_SIZE(factor); ++i)
  {
    m2::Rect<double> r(rect);
    r.Scale(factor[i]);
    TEST(is_equal_center(rect, r), ());

    r = rect;
    r.Scale(-factor[i]);
    TEST(is_equal_center(rect, r), ());
  }

  m2::RectD external(0, 0, 100, 100);
  TEST(external.IsIntersect(m2::RectD(10, 10, 90, 90)), ());
  TEST(external.IsPointInside(m2::PointD(5, 33)), ());
  TEST(external.IsIntersect(m2::RectD(99, 99, 1000, 1000)), ());
}

UNIT_TEST(Point)
{
  double const l = sqrt(2.0);
  double const a = math::pi / 4.0;
  m2::PointD const start(0.0, 0.0);

  TEST(is_equal(start.Move(l, a), m2::PointD(1, 1)), ());
  TEST(is_equal(start.Move(l, math::pi - a), m2::PointD(-1, 1)), ());
  TEST(is_equal(start.Move(l, -math::pi + a), m2::PointD(-1, -1)), ());
  TEST(is_equal(start.Move(l, - a), m2::PointD(1, -1)), ());
}
