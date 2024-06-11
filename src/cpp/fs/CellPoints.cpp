/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "CellPoints.h"
#include "ConvexHull.h"
#include "Location.h"
#include "Log.h"
namespace fs
{
static const double MIN_X = std::numeric_limits<double>::min();
static const double MAX_X = std::numeric_limits<double>::max();
constexpr double DIST_22_5 = 0.2071067811865475244008443621048490392848359376884740365883398689;
constexpr double P_0_5 = 0.5 + DIST_22_5;
constexpr double M_0_5 = 0.5 - DIST_22_5;
static const InnerPos INVALID_POINT{};
// not sure what's going on with this and wondering if it doesn't keep number exactly
// shouldn't be any way to be further than twice the entire width of the area
static const double INVALID_DISTANCE = static_cast<double>(MAX_ROWS * MAX_ROWS);
set<InnerPos> CellPoints::unique() const noexcept
{
  set<InnerPos> result{};
  if (!is_empty_)
  {
    for (size_t i = 0; i < pts_.size(); ++i)
    {
      if (INVALID_DISTANCE != dists_[i])
      {
        result.emplace(pts_[i]);
      }
    }
  }
  else
  {
    for (size_t i = 0; i < pts_.size(); ++i)
    {
      logging::check_equal(INVALID_DISTANCE, dists_[i], "distances");
      logging::check_equal(pts_[i].x(), INVALID_POINT.x(), "point x");
      logging::check_equal(pts_[i].y(), INVALID_POINT.y(), "point y");
    }
  }
  return result;
}
const CellPoints::array_pts CellPoints::points() const { return pts_; }
CellPoints::CellPoints() noexcept : pts_({}), dists_({}), src_(DIRECTION_NONE), is_empty_(true)
{
  std::fill(pts_.begin(), pts_.end(), INVALID_POINT);
  std::fill(dists_.begin(), dists_.end(), INVALID_DISTANCE);
  for (size_t i = 0; i < pts_.size(); ++i)
  {
    logging::check_equal(INVALID_DISTANCE, dists_[i], "distances");
    logging::check_equal(pts_[i].x(), INVALID_POINT.x(), "point x");
    logging::check_equal(pts_[i].y(), INVALID_POINT.y(), "point y");
  }
}
CellPoints::CellPoints(const CellPoints* rhs) noexcept : CellPoints()
{
  if (nullptr != rhs)
  {
    merge(*rhs);
  }
  for (size_t i = 0; i < pts_.size(); ++i)
  {
    logging::check_equal(INVALID_DISTANCE, dists_[i], "distances");
    logging::check_equal(pts_[i].x(), INVALID_POINT.x(), "point x");
    logging::check_equal(pts_[i].y(), INVALID_POINT.y(), "point y");
  }
}
CellPoints::CellPoints(const double x, const double y) noexcept : CellPoints() { insert(x, y); }
CellPoints& CellPoints::insert(const double x, const double y) noexcept
{
  const auto cell_x = static_cast<fs::Idx>(x);
  const auto cell_y = static_cast<fs::Idx>(y);
  const bool was_empty = is_empty_;
  insert(cell_x, cell_y, x, y);
  //   // HACK: somehow this makes it produce the same results as it was
  logging::check_fatal(empty(), "Empty after insert of (%f, %f)", x, y);
  for (size_t i = 0; i < pts_.size(); ++i)
  {
    if (was_empty)
    {
      logging::check_equal(pts_[i].x(), x, "point x");
      logging::check_equal(pts_[i].y(), y, "point y");
    }
    logging::check_fatal(INVALID_DISTANCE == dists_[i], "Invalid distance at position %ld", i);
  }
  const auto u = unique();
  return *this;
}
CellPoints::CellPoints(const InnerPos& p) noexcept : CellPoints() { insert(p); }
CellPoints& CellPoints::insert(const InnerPos& p) noexcept
{
  insert(p.x(), p.y());
  return *this;
}
CellPoints::array_dists CellPoints::find_distances(
  const double cell_x,
  const double cell_y,
  const double p_x,
  const double p_y
) noexcept
{
  const InnerPos p{p_x, p_y};
  const auto x = p.x() - cell_x;
  const auto y = p.y() - cell_y;
#define DISTANCE_1D(a, b) (((a) - (b)) * ((a) - (b)))
#define DISTANCE(x0, y0) (DISTANCE_1D((x), (x0)) + DISTANCE_1D((y), (y0)))
  // NOTE: order of x0/x and y0/y shouldn't matter since squaring
  return {// north is closest to point (0.5, 1.0)
          DISTANCE(0.5, 1.0),
          // north-northeast is closest to point (0.5 + 0.207, 1.0)
          DISTANCE(P_0_5, 1.0),
          // northeast is closest to point (1.0, 1.0)
          DISTANCE(1.0, 1.0),
          // east-northeast is closest to point (1.0, 0.5 + 0.207)
          DISTANCE(1.0, P_0_5),
          // east is closest to point (1.0, 0.5)
          DISTANCE(1.0, 0.5),
          // east-southeast is closest to point (1.0, 0.5 - 0.207)
          DISTANCE(1.0, M_0_5),
          // southeast is closest to point (1.0, 0.0)
          DISTANCE(1.0, 0),
          // south-southeast is closest to point (0.5 + 0.207, 0.0)
          DISTANCE(P_0_5, 0.0),
          // south is closest to point (0.5, 0.0)
          DISTANCE(0.5, 0.0),
          // south-southwest is closest to point (0.5 - 0.207, 0.0)
          DISTANCE(M_0_5, 0.0),
          // southwest is closest to point (0.0, 0.0)
          DISTANCE(0.0, 0.0),
          // west-southwest is closest to point (0.0, 0.5 - 0.207)
          DISTANCE(0.0, M_0_5),
          // west is closest to point (0.0, 0.5)
          DISTANCE(0.0, 0.5),
          // west-northwest is closest to point (0.0, 0.5 + 0.207)
          DISTANCE(0.0, P_0_5),
          // northwest is closest to point (0.0, 1.0)
          DISTANCE(0.0, 1.0),
          // north-northwest is closest to point (0.5 - 0.207, 1.0)
          DISTANCE(M_0_5, 1.0)
  };
#undef DISTANCE_1D
#undef DISTANCE
}
CellPoints& CellPoints::insert(
  const double cell_x,
  const double cell_y,
  const double p_x,
  const double p_y
) noexcept
{
  array_dists dists = find_distances(cell_x, cell_y, p_x, p_y);
  for (size_t i = 0; i < dists.size(); ++i)
  {
    if (dists[i] <= dists_[i])
    {
      dists_[i] = dists[i];
      pts_[i] = InnerPos{p_x, p_y};
    }
  }
  // either we had something already or we should now?
  is_empty_ = false;
  return *this;
}
CellPoints::CellPoints(const vector<InnerPos>& pts) noexcept : CellPoints()
{
  insert(pts.begin(), pts.end());
}
void CellPoints::add_source(const CellIndex src) { src_ |= src; }
CellPoints& CellPoints::merge(const CellPoints& rhs)
{
  if (!rhs.is_empty_)
  {
    // we know distances in each direction so just pick closer
    for (size_t i = 0; i < pts_.size(); ++i)
    {
      if (rhs.dists_[i] <= dists_[i])
      {
        // closer so replace
        dists_[i] = rhs.dists_[i];
        pts_[i] = rhs.pts_[i];
      }
    }
    is_empty_ &= rhs.is_empty_;
  }
  // HACK: allow empty list but still having a source
  src_ |= rhs.src_;
  return *this;
}
const cellpoints_map_type apply_offsets_spreadkey(
  const double duration,
  const OffsetSet& offsets,
  const spreading_points::mapped_type& cell_pts
)
{
  // NOTE: really tried to do this in parallel, but not enough points
  // in a cell for it to work well
  cellpoints_map_type r1{};
  // apply offsets to point
  for (const auto& out : offsets)
  {
    const double x_o = duration * out.x();
    const double y_o = duration * out.y();
    for (const auto& pts_for_cell : cell_pts)
    {
      const Location& src = std::get<0>(pts_for_cell);
      const CellPoints& pts = std::get<1>(pts_for_cell);
      for (const auto& p : pts.unique())
      {
        // putting results in copy of offsets and returning that
        // at the end of everything, we're just adding something to every double in the set by
        // duration?
        const double x = x_o + p.x();
        const double y = y_o + p.y();
        auto e = r1.try_emplace(Location{static_cast<Idx>(y), static_cast<Idx>(x)}, nullptr);
        auto& cell_pts = e.first->second;
#ifdef DEBUG_POINTS
        if (e.second)
        {
          auto& pts = cell_pts.pts_;
          auto& dists = cell_pts.dists_;
          // was just inserted, so except all distances to be max and points invalid
          for (size_t i = 0; i < pts.size(); ++i)
          {
            logging::check_equal(dists[i], INVALID_DISTANCE, "distance");
            logging::check_equal(pts[i].x(), INVALID_POINT.x(), "point x");
            logging::check_equal(pts[i].y(), INVALID_POINT.y(), "point y");
          }
          // always add point since we're calling try_emplace with empty list
          cell_pts.insert(x, y);
          // was just inserted, so except all distances to be max and points invalid
          for (size_t i = 0; i < pts.size(); ++i)
          {
            logging::check_fatal(dists[i] == INVALID_DISTANCE, "Distance %ld is invalid", i);
            logging::check_equal(pts[i].x(), x, "inserted x");
            logging::check_equal(pts[i].y(), y, "inserted y");
          }
        }
        else
        {
#endif
          // always add point since we're calling try_emplace with empty list
          cell_pts.insert(x, y);
#ifdef DEBUG_POINTS
        }
#endif
        const Location& dst = e.first->first;
        if (src != dst)
        {
          // we inserted a pair of (src, dst), which means we've never
          // calculated the relativeIndex for this so add it to main map
          cell_pts.add_source(relativeIndex(src, dst));
        }
      }
    }
  }
  return static_cast<const cellpoints_map_type>(r1);
}
/**
 * \brief Move constructor
 * \param rhs CellPoints to move from
 */
CellPoints::CellPoints(CellPoints&& rhs) noexcept
  : pts_(std::move(rhs.pts_)), dists_(std::move(rhs.dists_)), src_(rhs.src_),
    is_empty_(rhs.is_empty_)
{ }
/**
 * \brief Copy constructor
 * \param rhs CellPoints to copy from
 */
CellPoints::CellPoints(const CellPoints& rhs) noexcept
  : pts_({}), dists_({}), src_(rhs.src_), is_empty_(rhs.is_empty_)
{
  std::copy(rhs.pts_.begin(), rhs.pts_.end(), pts_.begin());
  std::copy(rhs.dists_.begin(), rhs.dists_.end(), dists_.begin());
}
/**
 * \brief Move assignment
 * \param rhs CellPoints to move from
 * \return This, after assignment
 */
CellPoints& CellPoints::operator=(CellPoints&& rhs) noexcept
{
  pts_ = std::move(rhs.pts_);
  dists_ = std::move(rhs.dists_);
  src_ = rhs.src_;
  is_empty_ = rhs.is_empty_;
  return *this;
}
/**
 * \brief Copy assignment
 * \param rhs CellPoints to copy from
 * \return This, after assignment
 */
CellPoints& CellPoints::operator=(const CellPoints& rhs) noexcept
{
  std::copy(rhs.pts_.begin(), rhs.pts_.end(), pts_.begin());
  std::copy(rhs.dists_.begin(), rhs.dists_.end(), dists_.begin());
  src_ = rhs.src_;
  is_empty_ = rhs.is_empty_;
  return *this;
}
}
