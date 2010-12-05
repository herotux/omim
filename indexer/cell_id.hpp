#pragma once
#include "mercator.hpp"

#include "../geometry/cellid.hpp"
#include "../geometry/rect2d.hpp"

#include "../base/base.hpp"
#include "../base/assert.hpp"

#include "../std/utility.hpp"
#include "../std/string.hpp"

typedef double CoordT;
typedef pair<CoordT, CoordT> CoordPointT;

typedef m2::CellId<19> RectId;

int64_t PointToInt64(CoordT x, CoordT y);
inline int64_t PointToInt64(CoordPointT const & pt) { return PointToInt64(pt.first, pt.second); }
CoordPointT Int64ToPoint(int64_t v);

pair<int64_t, int64_t> RectToInt64(m2::RectD const & r);
m2::RectD Int64ToRect(pair<int64_t, int64_t> const & p);

template <int MinX, int MinY, int MaxX, int MaxY>
struct Bounds
{
  enum
  {
    minX = MinX,
    maxX = MaxX,
    minY = MinY,
    maxY = MaxY
  };
};

//typedef Bounds<-180, -90, 180, 90> OrthoBounds;

template <typename BoundsT, typename CellIdT>
class CellIdConverter
{
public:

  static CoordT XToCellIdX(CoordT x)
  {
    return (x - BoundsT::minX) / StepX();
  }
  static CoordT YToCellIdY(CoordT y)
  {
    return (y - BoundsT::minY) / StepY();
  }

  static CellIdT ToCellId(CoordT x, CoordT y)
  {
    uint32_t const ix = static_cast<uint32_t>(XToCellIdX(x));
    uint32_t const iy = static_cast<uint32_t>(YToCellIdY(y));
    CellIdT id = CellIdT::FromXY(ix, iy);
#if 0 // DEBUG
    pair<uint32_t, uint32_t> ixy = id.XY();
    ASSERT(Abs(ixy.first  - ix) <= 1, (x, y, id, ixy));
    ASSERT(Abs(ixy.second - iy) <= 1, (x, y, id, ixy));
    CoordT minX, minY, maxX, maxY;
    GetCellBounds(id, minX, minY, maxX, maxY);
    ASSERT(minX <= x && x <= maxX, (x, y, id, minX, minY, maxX, maxY));
    ASSERT(minY <= y && y <= maxY, (x, y, id, minX, minY, maxX, maxY));
#endif
    return id;
  }

  static CellIdT Cover2PointsWithCell(CoordT x1, CoordT y1, CoordT x2, CoordT y2)
  {
    CellIdT id1 = ToCellId(x1, y1);
    CellIdT id2 = ToCellId(x2, y2);
    while (id1 != id2)
    {
      id1 = id1.Parent();
      id2 = id2.Parent();
    }
#if 0 // DEBUG
    CoordT minX, minY, maxX, maxY;
    GetCellBounds(id1, minX, minY, maxX, maxY);
    ASSERT(minX <= x1 && x1 <= maxX, (x1, y1, x2, y2, id1, minX, minY, maxX, maxY));
    ASSERT(minX <= x2 && x2 <= maxX, (x1, y1, x2, y2, id1, minX, minY, maxX, maxY));
    ASSERT(minY <= y1 && y1 <= maxY, (x1, y1, x2, y2, id1, minX, minY, maxX, maxY));
    ASSERT(minY <= y2 && y2 <= maxY, (x1, y1, x2, y2, id1, minX, minY, maxX, maxY));
#endif
    return id1;
  }

  static CoordPointT FromCellId(CellIdT id)
  {
    pair<uint32_t, uint32_t> const xy = id.XY();
    return CoordPointT(xy.first * StepX() + BoundsT::minX, xy.second * StepY() + BoundsT::minY);
  }

  static void GetCellBounds(CellIdT id, CoordT & minX, CoordT &  minY, CoordT & maxX, CoordT & maxY)
  {
    pair<uint32_t, uint32_t> const xy = id.XY();
    uint32_t const r = id.Radius();
    minX = (xy.first - r) * StepX() + BoundsT::minX;
    maxX = (xy.first + r) * StepX() + BoundsT::minX;
    minY = (xy.second - r) * StepY() + BoundsT::minY;
    maxY = (xy.second + r) * StepY() + BoundsT::minY;
  }
private:
  inline static CoordT StepX()
  {
    return CoordT(BoundsT::maxX - BoundsT::minX) / CellIdT::MAX_COORD;
  }
  inline static CoordT StepY()
  {
    return CoordT(BoundsT::maxY - BoundsT::minY) / CellIdT::MAX_COORD;
  }
};
