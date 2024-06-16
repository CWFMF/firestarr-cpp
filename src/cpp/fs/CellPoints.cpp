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
static const pair<double, InnerPos> INVALID_PAIR{INVALID_DISTANCE, {}};
static const Idx INVALID_LOCATION = INVALID_PAIR.second.x();
void assert_all_equal(const CellPoints::array_dists& pts, const double x, const double y)
{
  for (const auto& pt : pts)
  {
    logging::check_equal(pt.second.x(), x, "point x");
    logging::check_equal(pt.second.y(), y, "point y");
  }
}
void assert_all_invalid(const CellPoints::array_dists& pts)
{
  for (const auto& pt : pts)
  {
    logging::check_equal(INVALID_DISTANCE, pt.first, "distances");
  }
  assert_all_equal(pts, INVALID_LOCATION, INVALID_LOCATION);
}
set<InnerPos> CellPoints::unique() const noexcept
{
  set<InnerPos> result{};
#ifdef DEBUG_POINTS
  Idx cell_x_ = std::numeric_limits<Idx>::min();
  Idx cell_y_ = std::numeric_limits<Idx>::min();
#endif
  for (const auto& pt : pts_)
  {
    if (INVALID_DISTANCE != pt.first)
    {
      const auto& p = pt.second;
#ifdef DEBUG_POINTS
      cell_x_ = max(cell_x_, static_cast<Idx>(p.x()));
      cell_y_ = max(cell_y_, static_cast<Idx>(p.y()));
#endif
      result.emplace(p);
    }
  }
#ifdef DEBUG_POINTS
  for (size_t i = 0; i < pts_.size(); ++i)
  {
    if (INVALID_DISTANCE != pts_[i].first)
    {
      const auto& p = pts_[i].second;
      const Location loc1{static_cast<Idx>(p.y()), static_cast<Idx>(p.x())};
      logging::check_equal(loc1.column(), cell_x_, "column");
      logging::check_equal(loc1.row(), cell_y_, "row");
    }
  }
  if (result.empty())
  {
    assert_all_invalid(pts_);
  }
#endif
  return result;
}
CellPoints::CellPoints() noexcept : pts_({}), src_(DIRECTION_NONE)
{
  std::fill(pts_.begin(), pts_.end(), INVALID_PAIR);
#ifdef DEBUG_POINTS
  assert_all_invalid(pts_);
#endif
}
CellPoints::CellPoints(const CellPoints* rhs) noexcept : CellPoints()
{
  if (nullptr != rhs)
  {
#ifdef DEBUG_POINTS
    bool rhs_empty = rhs->unique().empty();
#endif
    merge(*rhs);
#ifdef DEBUG_POINTS
    logging::check_equal(unique().empty(), rhs_empty, "empty");
#endif
  }
#ifdef DEBUG_POINTS
  else
  {
    assert_all_invalid(pts_);
  }
#endif
}
CellPoints::CellPoints(const double x, const double y) noexcept : CellPoints() { insert(x, y); }
CellPoints& CellPoints::insert(const double x, const double y) noexcept
{
  const auto cell_x = static_cast<fs::Idx>(x);
  const auto cell_y = static_cast<fs::Idx>(y);
#ifdef DEBUG_POINTS
  bool was_empty = unique().empty();
#endif
  insert(cell_x, cell_y, x, y);
#ifdef DEBUG_POINTS
  logging::check_fatal(unique().empty(), "Empty after insert of (%f, %f)", x, y);
  for (size_t i = 0; i < pts_.size(); ++i)
  {
    if (was_empty)
    {
      logging::check_equal(pts_[i].second.x(), x, "point x");
      logging::check_equal(pts_[i].second.y(), y, "point y");
    }
    logging::check_fatal(INVALID_DISTANCE == pts_[i].first, "Invalid distance at position %ld", i);
  }
#endif
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
  const auto x = p_x - cell_x;
  const auto y = p_y - cell_y;
#define DISTANCE_1D(a, b) (((a) - (b)) * ((a) - (b)))
#define DISTANCE_XY(x0, y0) (DISTANCE_1D((x), (x0)) + DISTANCE_1D((y), (y0)))
#define DISTANCE(x0, y0) (pair<double, InnerPos>{DISTANCE_XY(x0, y0), InnerPos{p_x, p_y}})
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
    // NOTE: comparing pair will look at distance first
    pts_[i] = min(pts_[i], dists[i]);
  }
  return *this;
}
CellPoints::CellPoints(const vector<InnerPos>& pts) noexcept : CellPoints()
{
  insert(pts.begin(), pts.end());
}
void CellPoints::add_source(const CellIndex src) { src_ |= src; }
CellPoints& CellPoints::merge(const CellPoints& rhs)
{
  // we know distances in each direction so just pick closer
  for (size_t i = 0; i < pts_.size(); ++i)
  {
    pts_[i] = min(pts_[i], rhs.pts_[i]);
  }
  src_ |= rhs.src_;
  return *this;
}
CellPoints merge_cellpoints(const CellPoints& lhs, const CellPoints& rhs)
{
  CellPoints result{};
  // we know distances in each direction so just pick closer
  for (size_t i = 0; i < result.pts_.size(); ++i)
  {
    result.pts_[i] = min(lhs.pts_[i], rhs.pts_[i]);
  }
  // HACK: allow empty list but still having a source
  result.src_ = lhs.src_ | rhs.src_;
  return result;
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
          // was just inserted, so except all distances to be max and points invalid
          for (size_t i = 0; i < pts.size(); ++i)
          {
            logging::check_equal(pts[i].first, INVALID_DISTANCE, "distance");
            logging::check_equal(pts[i].second.x(), INVALID_POINT.x(), "point x");
            logging::check_equal(pts[i].second.y(), INVALID_POINT.y(), "point y");
          }
          // always add point since we're calling try_emplace with empty list
          cell_pts.insert(x, y);
          // was just inserted, so except all distances to be max and points invalid
          for (size_t i = 0; i < pts.size(); ++i)
          {
            logging::check_fatal(pts[i].first == INVALID_DISTANCE, "Distance %ld is invalid", i);
            logging::check_equal(pts[i].second.x(), x, "inserted x");
            logging::check_equal(pts[i].second.y(), y, "inserted y");
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
CellPoints::CellPoints(CellPoints&& rhs) noexcept : pts_(std::move(rhs.pts_)), src_(rhs.src_) { }
/**
 * \brief Copy constructor
 * \param rhs CellPoints to copy from
 */
CellPoints::CellPoints(const CellPoints& rhs) noexcept : pts_({}), src_(rhs.src_)
{
  std::copy(rhs.pts_.begin(), rhs.pts_.end(), pts_.begin());
}
/**
 * \brief Move assignment
 * \param rhs CellPoints to move from
 * \return This, after assignment
 */
CellPoints& CellPoints::operator=(CellPoints&& rhs) noexcept
{
  pts_ = std::move(rhs.pts_);
  src_ = rhs.src_;
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
  src_ = rhs.src_;
  return *this;
}
}
