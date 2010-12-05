#include "../cell_id.hpp"
#include "../../testing/testing.hpp"
#include "../../coding/hex.hpp"
#include "../../base/pseudo_random.hpp"
#include "../../std/cmath.hpp"
#include "../../std/string.hpp"
#include "../../std/cmath.hpp"

typedef m2::CellId<30> CellIdT;

UNIT_TEST(ToCellId)
{
  string s("2130000");
  s.append(CellIdT::DEPTH_LEVELS - 1 - s.size(), '0');
  TEST_EQUAL((CellIdConverter<Bounds<0, 0, 4, 4>, CellIdT>::ToCellId(1.5, 2.5).ToString()),
             s, ());
  TEST_EQUAL(CellIdT::FromString(s),
             (CellIdConverter<Bounds<0, 0, 4, 4>, CellIdT>::ToCellId(1.5, 2.5)), ());
}

UNIT_TEST(CommonCell)
{
  TEST_EQUAL((CellIdConverter<Bounds<0, 0, 4, 4>, CellIdT>::Cover2PointsWithCell(
      3.5, 2.5, 2.5, 3.5)),
             CellIdT::FromString("3"), ());
  TEST_EQUAL((CellIdConverter<Bounds<0, 0, 4, 4>, CellIdT>::Cover2PointsWithCell(
      2.25, 1.75, 2.75, 1.25)),
             CellIdT::FromString("12"), ());
}

namespace
{
  template <typename T1, typename T2>
  bool PairsAlmostEqual(pair<T1, T1> const & p1, pair<T2, T2> const & p2)
  {
    return fabs(p1.first - p2.first) + fabs(p1.second - p2.second) < 0.00001;
  }
}

UNIT_TEST(RandomRecode)
{
  PseudoRNG32 rng;
  for (size_t i = 0; i < 1000; ++i)
  {
    uint32_t x = rng.Generate() % 2000;
    uint32_t y = rng.Generate() % 1000;
    pair<double, double> xy =
        CellIdConverter<Bounds<0, 0, 2000, 1000>, CellIdT>::FromCellId(
            CellIdConverter<Bounds<0, 0, 2000, 1000>, CellIdT>::ToCellId(x, y));
    TEST(fabs(xy.first  - x) < 0.0002, (x, y, xy));
    TEST(fabs(xy.second - y) < 0.0001, (x, y, xy));
  }
}
