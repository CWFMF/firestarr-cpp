/* SPDX-FileCopyrightText: 2005, 2021 Jordan Evens */
/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "CellPoints.h"
#include "Location.h"
#include "Log.h"
#include "Scenario.h"
namespace fs
{
constexpr InnerSize DIST_22_5 =
  static_cast<InnerSize>(0.2071067811865475244008443621048490392848359376884740365883398689);
constexpr InnerSize P_0_5 = static_cast<InnerSize>(0.5) + DIST_22_5;
constexpr InnerSize M_0_5 = static_cast<InnerSize>(0.5) - DIST_22_5;
// not sure what's going on with this and wondering if it doesn't keep number exactly
// shouldn't be any way to be further than twice the entire width of the area
static const auto INVALID_DISTANCE = static_cast<DistanceSize>(MAX_ROWS * MAX_ROWS);
static const InnerPos INVALID_POSITION{};
static const pair<DistanceSize, InnerPos> INVALID_PAIR{INVALID_DISTANCE, {}};
static const Idx INVALID_LOCATION = INVALID_PAIR.second.x();
#ifdef DEBUG_POINTS
inline void assert_all_equal(
  const CellPoints::array_dist_pts& pts,
  const InnerSize x,
  const InnerSize y
)
{
  for (size_t i = 0; i < pts.second.size(); ++i)
  {
    logging::check_equal(pts.second[i].x(), x, "point x");
    logging::check_equal(pts.second[i].y(), y, "point y");
  }
}
inline void assert_all_invalid(const CellPoints::array_dist_pts& pts)
{
  for (size_t i = 0; i < pts.first.size(); ++i)
  {
    logging::check_equal(INVALID_DISTANCE, pts.first[i], "distances");
  }
  assert_all_equal(pts, INVALID_LOCATION, INVALID_LOCATION);
}
#endif
set<InnerPos> CellPoints::unique() const noexcept
{
  if (pts_dirty_)
  {
    pts_unique_ = {};
    for (size_t i = 0; i < pts_.first.size(); ++i)
    {
      if (INVALID_DISTANCE != pts_.first[i])
      {
        const auto& p = pts_.second[i];
#ifdef DEBUG_POINTS
        const Location loc{cell_y_, cell_x_};
        const Location loc1{static_cast<Idx>(p.y()), static_cast<Idx>(p.x())};
        logging::check_equal(loc1.column(), loc.column(), "column");
        logging::check_equal(loc1.row(), loc.row(), "row");
#endif
        pts_unique_.emplace(p);
      }
    }
#ifdef DEBUG_POINTS
    if (pts_unique_.empty())
    {
      assert_all_invalid(pts_);
    }
#endif
    pts_dirty_ = false;
  }
  return pts_unique_;
}
CellPoints::CellPoints(const Idx cell_x, const Idx cell_y) noexcept
  : pts_({}), pts_unique_({}), pts_dirty_(false), cell_x_(cell_x), cell_y_(cell_y),
    src_(DIRECTION_NONE)
{
  std::fill(pts_.first.begin(), pts_.first.end(), INVALID_DISTANCE);
  std::fill(pts_.second.begin(), pts_.second.end(), INVALID_POSITION);
#ifdef DEBUG_POINTS
  assert_all_invalid(pts_);
#endif
}
CellPoints::CellPoints() noexcept : CellPoints(INVALID_LOCATION, INVALID_LOCATION)
{
#ifdef DEBUG_POINTS
  // already done but check again since debugging
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
CellPoints::CellPoints(const InnerSize x, const InnerSize y) noexcept
  : CellPoints(static_cast<Idx>(x), static_cast<Idx>(y))
{
#ifdef DEBUG_POINTS
  assert_all_invalid(pts_);
#endif
  insert_(x, y);
#ifdef DEBUG_POINTS
  assert_all_equal(pts_, x, y);
#endif
}
CellPoints& CellPoints::insert(const InnerSize x, const InnerSize y) noexcept
{
#ifdef DEBUG_POINTS
  const bool was_empty = empty();
  logging::check_fatal(is_invalid(), "Invalid when calling insert(%f, %f)", x, y);
#endif
  insert_(x, y);
#ifdef DEBUG_POINTS
  logging::check_fatal(unique().empty(), "Empty after insert of (%f, %f)", x, y);
  if (was_empty)
  {
    assert_all_equal(pts_, x, y);
  }
  else
  {
    for (size_t i = 0; i < pts_.first.size(); ++i)
    {
      logging::check_fatal(
        INVALID_DISTANCE == pts_.first[i], "Invalid distance at position %ld", i
      );
    }
  }
#endif
  return *this;
}
CellPoints::CellPoints(const InnerPos& p) noexcept : CellPoints(p.x(), p.y())
{
#ifdef DEBUG_POINTS
  assert_all_equal(pts_, p.x(), p.y());
#endif
}
CellPoints& CellPoints::insert(const InnerPos& p) noexcept
{
#ifdef DEBUG_POINTS
  const auto x = p.x();
  const auto y = p.y();
  const bool was_empty = empty();
  logging::check_fatal(is_invalid(), "Invalid when calling insert(%f, %f)", x, y);
#endif
  insert(p.x(), p.y());
#ifdef DEBUG_POINTS
  logging::check_fatal(unique().empty(), "Empty after insert of (%f, %f)", x, y);
  if (was_empty)
  {
    assert_all_equal(pts_, x, y);
  }
  else
  {
    for (size_t i = 0; i < pts_.first.size(); ++i)
    {
      logging::check_fatal(
        INVALID_DISTANCE == pts_.first[i], "Invalid distance at position %ld", i
      );
    }
  }
#endif
  return *this;
}
using DISTANCE_PAIR = pair<DistanceSize, DistanceSize>;
#define D_PTS(x, y) (DISTANCE_PAIR{static_cast<DistanceSize>(x), static_cast<DistanceSize>(y)})
constexpr std::array<DISTANCE_PAIR, NUM_DIRECTIONS> POINTS_OUTER{
  D_PTS(0.5, 1.0),
  // north-northeast is closest to point (0.5 + 0.207, 1.0)
  D_PTS(P_0_5, 1.0),
  // northeast is closest to point (1.0, 1.0)
  D_PTS(1.0, 1.0),
  // east-northeast is closest to point (1.0, 0.5 + 0.207)
  D_PTS(1.0, P_0_5),
  // east is closest to point (1.0, 0.5)
  D_PTS(1.0, 0.5),
  // east-southeast is closest to point (1.0, 0.5 - 0.207)
  D_PTS(1.0, M_0_5),
  // southeast is closest to point (1.0, 0.0)
  D_PTS(1.0, 0.0),
  // south-southeast is closest to point (0.5 + 0.207, 0.0)
  D_PTS(P_0_5, 0.0),
  // south is closest to point (0.5, 0.0)
  D_PTS(0.5, 0.0),
  // south-southwest is closest to point (0.5 - 0.207, 0.0)
  D_PTS(M_0_5, 0.0),
  // southwest is closest to point (0.0, 0.0)
  D_PTS(0.0, 0.0),
  // west-southwest is closest to point (0.0, 0.5 - 0.207)
  D_PTS(0.0, M_0_5),
  // west is closest to point (0.0, 0.5)
  D_PTS(0.0, 0.5),
  // west-northwest is closest to point (0.0, 0.5 + 0.207)
  D_PTS(0.0, P_0_5),
  // northwest is closest to point (0.0, 1.0)
  D_PTS(0.0, 1.0),
  // north-northwest is closest to point (0.5 - 0.207, 1.0)
  D_PTS(M_0_5, 1.0)
};
CellPoints::array_dists CellPoints::find_distances(const InnerSize p_x, const InnerSize p_y)
  const noexcept
{
#ifdef DEBUG_GRIDS
  logging::check_fatal(
    p_y < 0 || p_y >= MAX_ROWS, "p_y %f is out of bounds (%f, %f)", p_y, 0, MAX_ROWS
  );
  logging::check_fatal(
    p_x < 0 || p_x >= MAX_COLUMNS, "p_x %f is out of bounds (%f, %f)", p_x, 0, MAX_COLUMNS
  );
#endif
  const auto x = p_x - cell_x_;
  const auto y = p_y - cell_y_;
#ifdef DEBUG_GRIDS
  logging::check_fatal(x < 0 || x > 1, "x %f is out of cell (%f, %f)", x, 0, 1);
  logging::check_fatal(y < 0 || y > 1, "y %f is out of cell (%f, %f)", y, 0, 1);
#endif
  const auto x0 = static_cast<DistanceSize>(x);
  const auto y0 = static_cast<DistanceSize>(y);
  array_dists d{};
  for (size_t i = 0; i < d.size(); ++i)
  {
    const auto& p = POINTS_OUTER[i];
    const auto& x1 = p.first;
    const auto& y1 = p.second;
    d[i] = ((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
  }
  return d;
#undef DISTANCE_1D
#undef DISTANCE
#undef dist
}
CellPoints& CellPoints::insert_(const InnerSize x, const InnerSize y) noexcept
{
#ifdef DEBUG_POINTS
  logging::check_fatal(
    is_invalid(), "Expected cell_x_ and cell_y_ to be set before calling insert_ (%f, %f)", x, y
  );
  logging::check_equal(static_cast<Idx>(x), cell_x_, "CellPoints x");
  logging::check_equal(static_cast<Idx>(y), cell_y_, "CellPoints y");
  logging::check_fatal(
    x < 0 || x >= MAX_COLUMNS, "x %f is out of bounds (%f, %f)", x, 0, MAX_COLUMNS
  );
  logging::check_fatal(y < 0 || y >= MAX_ROWS, "y %f is out of bounds (%f, %f)", y, 0, MAX_ROWS);
#endif
  auto dists = find_distances(x, y);
  for (size_t i = 0; i < dists.size(); ++i)
  {
    const auto& d = dists[i];
    auto& p_d = pts_.first[i];
    auto& p_pt = pts_.second[i];
    if (d < p_d)
    {
      p_d = d;
      p_pt = InnerPos(x, y);
      // only mark as dirty if actually changing a value
      pts_dirty_ = true;
    }
  }
  return *this;
}
void CellPoints::add_source(const CellIndex src)
{
#ifdef DEBUG_POINTS
#endif
  src_ |= src;
#ifdef DEBUG_POINTS
  // logical and produces input
  logging::check_equal(src_ & src, src, "source mask");
  logging::check_equal(!(src_ & src), !src, "source non-zero");
#endif
}
CellPoints& CellPoints::merge(const CellPoints& rhs)
{
#ifdef DEBUG_POINTS
  logging::check_fatal(
    !((rhs.is_invalid() || is_invalid()) || (cell_x_ == rhs.cell_x_ && cell_y_ == rhs.cell_y_)),
    "Expected for_cell_ to be the same or at least one invalid but have (%d, %d) and (%d, %d)",
    cell_x_,
    cell_y_,
    rhs.cell_x_,
    rhs.cell_y_
  );
  const auto prev_x = cell_x_;
  const auto prev_y = cell_y_;
#endif
  // either both invalid or lower one is valid
  cell_x_ = min(cell_x_, rhs.cell_x_);
  cell_y_ = min(cell_y_, rhs.cell_y_);
#ifdef DEBUG_POINTS
  if (INVALID_LOCATION == rhs.cell_x_)
  {
    logging::check_equal(cell_x_, prev_x, "orig cell_x_");
    logging::check_equal(cell_y_, prev_y, "orig cell_y_");
  }
  if (INVALID_LOCATION == rhs.cell_y_)
  {
    logging::check_equal(cell_x_, prev_x, "orig cell_x_");
    logging::check_equal(cell_y_, prev_y, "orig cell_y_");
  }
  if (INVALID_LOCATION == prev_x)
  {
    logging::check_equal(cell_x_, rhs.cell_x_, "orig cell_x_");
    logging::check_equal(cell_y_, rhs.cell_y_, "orig cell_y_");
  }
  if (INVALID_LOCATION == prev_y)
  {
    logging::check_equal(cell_x_, rhs.cell_x_, "orig cell_x_");
    logging::check_equal(cell_y_, rhs.cell_y_, "orig cell_y_");
  }
  if (cell_x_ == rhs.cell_x_)
  {
    logging::check_equal(cell_y_, rhs.cell_y_, "merged rhs cell_y_");
  }
  if (cell_x_ == prev_x)
  {
    logging::check_equal(cell_y_, prev_y, "merged rhs cell_y_");
  }
#endif
  // we know distances in each direction so just pick closer
  for (size_t i = 0; i < pts_.first.size(); ++i)
  {
    if (rhs.pts_.first[i] < pts_.first[i])
    {
      pts_.first[i] = rhs.pts_.first[i];
      pts_.second[i] = rhs.pts_.second[i];
      // only mark as dirty if actually changing a value
      pts_dirty_ = true;
    }
  }
  add_source(rhs.src_);
  return *this;
}
CellPointsMap apply_offsets_spreadkey(
  const DurationSize duration,
  const OffsetSet& offsets,
  const spreading_points::mapped_type& cell_pts
)
{
  // NOTE: really tried to do this in parallel, but not enough points
  // in a cell for it to work well
  CellPointsMap r1{};
#ifdef DEBUG_POINTS
  logging::check_fatal(offsets.empty(), "offsets.empty()");
  const auto s0 = offsets.size();
  const auto s1 = cell_pts.size();
  logging::check_fatal(0 == s0, "Applying no offsets");
  logging::check_fatal(0 == s1, "Applying offsets to no points");
#endif
  // apply offsets to point
  for (const auto& out : offsets)
  {
    const auto x_o = static_cast<InnerSize>(duration * out.x());
    const auto y_o = static_cast<InnerSize>(duration * out.y());
#ifdef DEBUG_POINTS
    logging::check_fatal(cell_pts.empty(), "cell_pts.empty()");
#endif
    for (const auto& pts_for_cell : cell_pts)
    {
      const Location& src = std::get<0>(pts_for_cell);
      const CellPoints& pts = std::get<1>(pts_for_cell);
      const auto& u = pts.unique();
#ifdef DEBUG_POINTS
      const Location loc1{src.row(), src.column()};
      logging::check_equal(src.hash(), loc1.hash(), "hash");
      logging::check_fatal(
        u.size() > NUM_DIRECTIONS,
        "Expected less than %d unique points but have %ld",
        NUM_DIRECTIONS,
        u.size()
      );
      logging::check_fatal(u.empty(), "Should not have empty CellPoints");
#endif
      for (const auto& p : u)
      {
        // putting results in copy of offsets and returning that
        // at the end of everything, we're just adding something to every InnerSize in the set by
        // duration?
        const auto x = x_o + p.x();
        const auto y = y_o + p.y();
#ifdef DEBUG_POINTS
        const Location from_xy{static_cast<Idx>(y), static_cast<Idx>(x)};
        auto seek_cell_pts = r1.map_.find(from_xy);
#endif
        CellPoints& cell_pts = r1.insert(x, y);
#ifdef DEBUG_POINTS
        if (r1.map_.end() != seek_cell_pts)
        {
          logging::check_equal(&(seek_cell_pts->second), &cell_pts, "cell_pts");
        }
        logging::check_fatal(r1.unique().empty(), "Empty after inserting (%f, %f)", x, y);
#endif
        const Location& dst = cell_pts.location();
#ifdef DEBUG_POINTS
        logging::check_equal(dst.column(), from_xy.column(), "column");
        logging::check_equal(dst.row(), from_xy.row(), "row");
        logging::check_equal(dst.hash(), from_xy.hash(), "hash");
        CellPoints& cell_pts1 = r1.map_[dst];
        if (r1.map_.end() != seek_cell_pts)
        {
          logging::check_equal(&(seek_cell_pts->second), &cell_pts1, "cell_pts1");
        }
        logging::check_equal(cell_pts.cell_x_, cell_pts1.cell_x_, "cell_x_ lookup");
        logging::check_equal(cell_pts.cell_y_, cell_pts1.cell_y_, "cell_y_ lookup");
        logging::check_equal(static_cast<Idx>(x), dst.column(), "column/x");
        logging::check_equal(static_cast<Idx>(y), dst.row(), "row/y");
#endif
        if (src != dst)
        {
          // we inserted a pair of (src, dst), which means we've never
          // calculated the relativeIndex for this so add it to main map
          cell_pts.add_source(relativeIndex(src, dst));
        }
      }
    }
  }
#ifdef DEBUG_POINTS
  logging::check_fatal(r1.map_.empty(), "r1.map_.empty()");
  logging::check_fatal(offsets.empty(), "Applied no offsets");
  logging::check_fatal(cell_pts.empty(), "Applied offsets to no points");
#endif
  return r1;
}
/**
 * \brief Move constructor
 * \param rhs CellPoints to move from
 */
CellPoints::CellPoints(CellPoints&& rhs) noexcept
  : pts_(std::move(rhs.pts_)), pts_unique_(std::move(rhs.pts_unique_)),
    pts_dirty_(static_cast<bool>(rhs.pts_dirty_)), cell_x_(rhs.cell_x_), cell_y_(rhs.cell_y_),
    src_(rhs.src_)
{ }
/**
 * \brief Copy constructor
 * \param rhs CellPoints to copy from
 */
CellPoints::CellPoints(const CellPoints& rhs) noexcept
  : pts_({}), pts_unique_(rhs.pts_unique_.cbegin(), rhs.pts_unique_.cend()),
    pts_dirty_(static_cast<bool>(rhs.pts_dirty_)), cell_x_(rhs.cell_x_), cell_y_(rhs.cell_y_),
    src_(rhs.src_)
{
  std::copy(rhs.pts_.first.cbegin(), rhs.pts_.first.cend(), pts_.first.begin());
  std::copy(rhs.pts_.second.cbegin(), rhs.pts_.second.cend(), pts_.second.begin());
}
/**
 * \brief Move assignment
 * \param rhs CellPoints to move from
 * \return This, after assignment
 */
CellPoints& CellPoints::operator=(CellPoints&& rhs) noexcept
{
  pts_ = std::move(rhs.pts_);
  pts_unique_ = std::move(rhs.pts_unique_);
  pts_dirty_ = static_cast<bool>(rhs.pts_dirty_);
  cell_x_ = rhs.cell_x_;
  cell_y_ = rhs.cell_y_;
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
  std::copy(rhs.pts_.first.cbegin(), rhs.pts_.first.cend(), pts_.first.begin());
  std::copy(rhs.pts_.second.cbegin(), rhs.pts_.second.cend(), pts_.second.begin());
  pts_unique_ = rhs.pts_unique_;
  pts_dirty_ = static_cast<bool>(rhs.pts_dirty_);
  cell_x_ = rhs.cell_x_;
  cell_y_ = rhs.cell_y_;
  src_ = rhs.src_;
  return *this;
}
bool CellPoints::operator<(const CellPoints& rhs) const noexcept
{
  if (cell_x_ == rhs.cell_x_)
  {
    if (cell_y_ == rhs.cell_y_)
    {
      for (size_t i = 0; i < pts_.first.size(); ++i)
      {
        if (pts_.second[i] != rhs.pts_.second[i])
        {
          return pts_.second[i] < rhs.pts_.second[i];
        }
      }
      // all points are equal if we got here
    }
    return cell_y_ < rhs.cell_y_;
  }
  return cell_x_ < rhs.cell_x_;
}
bool CellPoints::operator==(const CellPoints& rhs) const noexcept
{
  if (cell_x_ == rhs.cell_x_ && cell_y_ == rhs.cell_y_)
  {
    for (size_t i = 0; i < pts_.second.size(); ++i)
    {
      if (pts_.second[i] != rhs.pts_.second[i])
      {
        return false;
      }
    }
    // all points are equal if we got here
    return true;
  }
  return false;
}
bool CellPoints::empty() const
{
  // NOTE: is_invalid() should never be true if it's checking cell_x_
  return unique().empty();
}
#ifdef DEBUG_POINTS
bool CellPoints::is_invalid() const
{
#ifdef DEBUG_POINTS
  // if one is invalid then they both should be
  logging::check_equal(
    cell_x_ == INVALID_LOCATION, cell_y_ == INVALID_LOCATION, "CellPoints Idx is invalid"
  );
  // if invalid then no points should be in list
  if (cell_x_ == INVALID_LOCATION)
  {
    assert_all_invalid(pts_);
  }
#endif
  logging::check_fatal(
    cell_x_ == INVALID_LOCATION, "CellPoints should always be initialized with some Location"
  );
  return cell_x_ == INVALID_LOCATION;
}
#endif
[[nodiscard]] Location CellPoints::location() const noexcept
{
#ifdef DEBUG_POINTS
  logging::check_fatal(
    is_invalid(), "Expected cell_x_ and cell_y_ to be set before calling location()"
  );
#endif
  return Location{cell_y_, cell_x_};
}
CellPointsMap::CellPointsMap() : map_({}) { }
void CellPointsMap::emplace(const CellPoints& pts)
{
  const Location location = pts.location();
#ifdef DEBUG_POINTS
  logging::check_equal(pts.cell_x_, location.column(), "pts.cell_x_ to location");
  logging::check_equal(pts.cell_y_, location.row(), "pts.cell_y_ to location");
#endif
  auto e = map_.try_emplace(location, pts);
  CellPoints& cell_pts = e.first->second;
#ifdef DEBUG_POINTS
  logging::check_equal(cell_pts.cell_x_, location.column(), "cell_pts.cell_x_ to location");
  logging::check_equal(cell_pts.cell_y_, location.row(), "cell_pts.cell_y_ to location");
#endif
#ifdef DEBUG_POINTS
  logging::check_equal(cell_pts.cell_x_, pts.cell_x_, "cell_pts.cell_x_ to original");
  logging::check_equal(cell_pts.cell_y_, pts.cell_y_, "cell_pts.cell_y_ to original");
#endif
  if (!e.second)
  {
    // couldn't insert
    cell_pts.merge(pts);
  }
#ifdef DEBUG_POINTS
  CellPoints& cell_pts1 = map_[location];
  logging::check_equal(cell_pts.cell_x_, cell_pts1.cell_x_, "cell_x_ lookup");
  logging::check_equal(cell_pts.cell_y_, cell_pts1.cell_y_, "cell_y_ lookup");
  logging::check_fatal(INVALID_LOCATION == cell_pts.cell_x_, "CellPoints has invalid cell_x_");
  logging::check_fatal(INVALID_LOCATION == cell_pts.cell_y_, "CellPoints has invalid cell_y_");
#endif
}
CellPoints& CellPointsMap::insert(const InnerSize x, const InnerSize y) noexcept
{
  const Location location{static_cast<Idx>(y), static_cast<Idx>(x)};
  auto e = map_.try_emplace(location, x, y);
  CellPoints& cell_pts = e.first->second;
#ifdef DEBUG_POINTS
  logging::check_fatal(INVALID_LOCATION == cell_pts.cell_x_, "CellPoints has invalid cell_x_");
  logging::check_fatal(INVALID_LOCATION == cell_pts.cell_y_, "CellPoints has invalid cell_y_");
#endif
  if (!e.second)
  {
    // tried to add new CellPoints but already there
    cell_pts.insert(x, y);
  }
#ifdef DEBUG_POINTS
  logging::check_equal(static_cast<Idx>(x), cell_pts.cell_x_, "cell_x_");
  logging::check_equal(static_cast<Idx>(y), cell_pts.cell_y_, "cell_y_");
  logging::check_equal(location.column(), cell_pts.cell_x_, "cell_x_");
  logging::check_equal(location.row(), cell_pts.cell_y_, "cell_y_");
  logging::check_equal(location.row(), cell_pts.location().row(), "row");
  logging::check_equal(location.column(), cell_pts.location().column(), "column");
#endif
  return e.first->second;
}
CellPointsMap& CellPointsMap::merge(const BurnedData& unburnable, const CellPointsMap& rhs) noexcept
{
  for (const auto& kv : rhs.map_)
  {
#ifdef DEBUG_POINTS
    logging::check_fatal(
      kv.second.is_invalid(), "Trying to merge CellPointsMap with invalid CellPoints"
    );
#endif
    const auto h = kv.first.hash();
    if (!unburnable.at(h))
    {
      emplace(kv.second);
    }
  }
  return *this;
}
void CellPointsMap::remove_if(std::function<bool(const pair<Location, CellPoints>&)> F) noexcept
{
  auto it = map_.begin();
#ifdef DEBUG_POINTS
  set<Location> removed_items{};
  const auto u0 = unique();
  const auto s0 = u0.size();
  size_t removed = 0;
  logging::check_fatal(u0.empty(), "Checking removal from empty CellPoints");
#endif
  while (map_.end() != it)
  {
#ifdef DEBUG_POINTS
    const Location location = it->first;
    const CellPoints& cell_pts = it->second;
    const auto u = cell_pts.unique();
    logging::check_fatal(u.empty(), "Checking if empty CellPoints should be removed");
    logging::check_equal(location.column(), cell_pts.cell_x_, "cell_x_");
    logging::check_equal(location.row(), cell_pts.cell_y_, "cell_y_");
    logging::check_equal(location.row(), cell_pts.location().row(), "row");
    logging::check_equal(location.column(), cell_pts.location().column(), "column");
#endif
    if (F(*it))
    {
#ifdef DEBUG_POINTS
      removed_items.emplace(it->first);
#endif
#ifdef DEBUG_POINTS
      // remove if F returns true for current
      logging::verbose("Removing CellPoints for (%d, %d)", location.column(), location.row());
#endif
      it = map_.erase(it);
#ifdef DEBUG_POINTS
      // if all points from that were in the original then it should be exactly that many fewer
      removed += u.size();
#endif
    }
    else
    {
      ++it;
    }
#ifdef DEBUG_POINTS
    const auto u_cur = unique();
    logging::check_equal(u_cur.size(), s0 - removed, "u_cur.size()");
#endif
  }
#ifdef DEBUG_POINTS
  for (const auto& loc : removed_items)
  {
    auto seek = map_.find(loc);
    logging::check_fatal(
      map_.end() != seek, "Still have map entry for (%d, %d)", loc.column(), loc.row()
    );
  }
  for (const auto& kv : map_)
  {
    logging::check_fatal(
      F(kv), "Should have removed (%d, %d) but didn't", kv.first.column(), kv.first.row()
    );
  }
#endif
}
set<InnerPos> CellPointsMap::unique() const noexcept
{
  set<InnerPos> r{};
  for (const auto& kv : map_)
  {
    const auto u = kv.second.unique();
#ifdef DEBUG_POINTS
    const auto s0 = r.size();
    const auto s1 = u.size();
#endif
    r.insert(u.begin(), u.end());
#ifdef DEBUG_POINTS
    logging::check_fatal(r.size() < s1, "Less points after insertion: (%ld vs %ld)", r.size(), s1);
    logging::check_fatal(r.size() < s0, "Less points than inserted: (%ld vs %ld)", r.size(), s0);
    logging::check_fatal(
      r.size() > (s0 + s1),
      "More points than possible after insertion: (%ld vs %ld)",
      r.size(),
      (s0 + s1)
    );
#endif
  }
  return r;
}
CellPointsMap merge_list(
  const BurnedData& unburnable,
  map<SpreadKey, SpreadInfo>& spread_info,
  const DurationSize duration,
  const spreading_points& to_spread
)
{
  auto spread = std::views::transform(
    to_spread,
    [&duration, &spread_info](const spreading_points::value_type& kv0) -> CellPointsMap {
      auto& key = kv0.first;
      const auto& offsets = spread_info[key].offsets();
#ifdef DEBUG_POINTS
      logging::check_fatal(offsets.empty(), "offsets.empty()");
#endif
      const spreading_points::mapped_type& cell_pts = kv0.second;
#ifdef DEBUG_POINTS
      logging::check_fatal(cell_pts.empty(), "cell_pts.empty()");
#endif
      auto r = apply_offsets_spreadkey(duration, offsets, cell_pts);
#ifdef DEBUG_POINTS
      logging::check_fatal(r.unique().empty(), "r.unique().empty()");
#endif
      return r;
    }
  );
  auto it = spread.begin();
  CellPointsMap out{};
  while (spread.end() != it)
  {
    const CellPointsMap& cell_pts = *it;
#ifdef DEBUG_POINTS
    logging::check_fatal(cell_pts.unique().empty(), "Merging empty points");
#endif
    // // HACK: keep old behaviour until we can figure out whey removing isn't the same as not
    // adding const auto h = cell_pts.location().hash(); if (!unburnable[h])
    // {
    out.merge(unburnable, cell_pts);
#ifdef DEBUG_POINTS
    logging::check_fatal(out.unique().empty(), "Empty points after merge");
#endif
    ++it;
  }
  return out;
}
void CellPointsMap::calculate_spread(
  Scenario& scenario,
  map<SpreadKey, SpreadInfo>& spread_info,
  const DurationSize duration,
  const spreading_points& to_spread,
  const BurnedData& unburnable
)
{
  CellPointsMap cell_pts = merge_list(unburnable, spread_info, duration, to_spread);
#ifdef DEBUG_POINTS
  const auto u = cell_pts.unique();
  logging::check_fatal(u.empty(), "No points after spread");
  size_t total = 0;
  set<InnerPos> u0{};
  for (const auto& kv : cell_pts.map_)
  {
    const auto loc = kv.first;
    const auto u1 = kv.second.unique();
    logging::check_fatal(
      u1.empty(), "No points in CellPoints for (%d, %d) after spread", loc.column(), loc.row()
    );
    // make sure all points are actually in the right location
    const auto s0_cur = u0.size();
    logging::check_equal(s0_cur, total, "total");
    for (const auto& p1 : u1)
    {
      const Location loc1{static_cast<Idx>(p1.y()), static_cast<Idx>(p1.x())};
      logging::check_equal(loc1.column(), loc.column(), "column");
      logging::check_equal(loc1.row(), loc.row(), "row");
      u0.emplace(p1);
    }
    total += u1.size();
    logging::check_equal(s0_cur + u1.size(), total, "total");
  }
#endif
  cell_pts.remove_if([&scenario, &unburnable](const pair<Location, CellPoints>& kv) {
    const auto& location = kv.first;
#ifdef DEBUG_POINTS
    // look up Cell from scenario here since we don't need attributes until now
    const Cell k = scenario.cell(location);
    const auto& cell_pts = kv.second;
    const auto u = cell_pts.unique();
    logging::check_fatal(u.empty(), "Empty points when checking to remove");
    logging::check_equal(k, scenario.cell(cell_pts.location()), "CellPoints location");
    logging::check_equal(k.column(), location.column(), "Cell column");
    logging::check_equal(k.row(), location.row(), "Cell row");
#endif
    const auto h = location.hash();
    // clear out if unburnable
    const auto do_clear = unburnable.at(h);
#ifdef DEBUG_POINTS
    logging::check_equal(h, k.hash(), "Cell vs Location hash()");
    if (is_null_fuel(k))
    {
      logging::check_fatal(!do_clear, "Not clearing when not fuel");
    }
#endif
    return do_clear;
  });
  // need to merge new points back into cells that didn't spread
  merge(unburnable, cell_pts);
}
}
