/* Copyright (c) Jordan Evens, 2005, 2021 */
/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "CellPoints.h"
#include "Log.h"
#include "Location.h"
#include "Scenario.h"

namespace fs::sim
{
constexpr InnerSize DIST_22_5 = static_cast<InnerSize>(0.2071067811865475244008443621048490392848359376884740365883398689);
constexpr InnerSize P_0_5 = static_cast<InnerSize>(0.5) + DIST_22_5;
constexpr InnerSize M_0_5 = static_cast<InnerSize>(0.5) - DIST_22_5;
//   static constexpr auto INVALID_DISTANCE = std::numeric_limits<InnerSize>::max();
// not sure what's going on with this and wondering if it doesn't keep number exactly
// shouldn't be any way to be further than twice the entire width of the area
static const auto INVALID_DISTANCE = static_cast<DistanceSize>(MAX_ROWS * MAX_ROWS);
static const XYPos INVALID_XY_POSITION{};
static const pair<DistanceSize, XYPos> INVALID_XY_PAIR{INVALID_DISTANCE, {}};
static const XYSize INVALID_XY_LOCATION = INVALID_XY_PAIR.second.first;
static const InnerPos INVALID_INNER_POSITION{};
static const pair<DistanceSize, InnerPos> INVALID_INNER_PAIR{INVALID_DISTANCE, {}};
static const InnerSize INVALID_INNER_LOCATION = INVALID_INNER_PAIR.second.first;
static const SpreadData INVALID_SPREAD_DATA{
  INVALID_TIME};
set<XYPos> CellPoints::unique() const noexcept
{
  logging::check_equal(
    INVALID_DISTANCE != pts_.distances()[0],
    can_burn_,
    "can_burn_");
  // // if any point is invalid then they all have to be
  if (
    // !can_burn_ ||
    INVALID_DISTANCE == pts_.distances()[0])
  {
    return {};
  }
  else
  {
    const auto& pts_all = std::views::transform(
      pts_.points(),
      [this](const auto& p) {
        return XYPos(p.first + cell_x_y_.first, p.second + cell_x_y_.second);
      });
    return {pts_all.begin(), pts_all.end()};
  }
}
#ifdef DEBUG_CELLPOINTS
size_t CellPoints::size() const noexcept
{
  return unique().size();
}
#endif
CellPoints::CellPoints(
  const HashSize hash_uninit,
  const bool can_burn_uninit,
  const bool can_burn_unburnable,
  const bool can_burn_non_fuel,
  const bool can_burn_has_not_burned,
  const bool can_burn,
  const CellPos& cell) noexcept
  : can_burn_(can_burn),
    can_burn_has_not_burned_(can_burn_has_not_burned),
    can_burn_non_fuel_(can_burn_non_fuel),
    can_burn_unburnable_(can_burn_unburnable),
    can_burn_uninit_(can_burn_uninit),
    pts_(),
    cell_x_y_(cell),
    hash_uninit_(hash_uninit)

{
  // logging::check_equal(hash_uninit_, cell_x_y_.hash(), "hash_uninit");
  // if (can_burn_)
  {
    std::fill(pts_.distances().begin(), pts_.distances().end(), INVALID_DISTANCE);
    std::fill(pts_.points().begin(), pts_.points().end(), INVALID_INNER_POSITION);

#ifdef DEBUG_CELLPOINTS
    logging::note("CellPoints is size %ld after creation and should be empty", size());
#endif
  }
}
CellPoints::CellPoints(
  const BurnedData& unburnable,
  const Scenario& scenario,
  const CellPos& cell) noexcept
  // : CellPoints(!unburnable[cell_x_y_.hash()], cell)
  : CellPoints(
    cell_x_y_.hash(),
    !unburnable[cell_x_y_.hash()],
    !unburnable[cell.hash()],
    scenario.hasNotBurned(cell.hash()),
    scenario.hasNotBurned(cell.hash()),
    !unburnable[cell.hash()],
    // !scenario.isUnburnable(cell.hash()),
    // // !unburnable[cell.hash()],
    // !scenario.isUnburnable(cell.hash()),
    cell)
{
  logging::check_equal(
    !scenario.cannotSpread(hash_uninit_),
    can_burn_uninit_,
    "can_burn_uninit_");
  logging::check_equal(
    !scenario.cannotSpread(cell_x_y_.hash()),
    can_burn_unburnable_,
    "can_burn_unburnable_");
  // logging::check_equal(
  //   unburnable[hash_uninit_],
  //   unburnable[cell_x_y_.hash()],
  //   "unburnable[hash_uninit]");
  // logging::check_equal(
  //   !unburnable[hash_uninit_],
  //   can_burn_,
  //   "unburnable[hash_uninit]");
}
CellPoints::CellPoints() noexcept
  : CellPoints(INVALID_INDEX, false, false, false, false, false, CellPos{INVALID_INDEX, INVALID_INDEX})
{
}
CellPoints::CellPoints(const CellPoints* rhs) noexcept
  : CellPoints()
{
  logging::check_fatal(nullptr == rhs, "Initializing CellPoints from nullptr");
  *this = *rhs;
}
CellPoints::CellPoints(
  const BurnedData& unburnable,
  const Scenario& scenario,
  const XYSize x,
  const XYSize y) noexcept
  : CellPoints(
    unburnable,
    scenario,
    CellPos(static_cast<Idx>(x), static_cast<Idx>(y)))
{
  insert(
    x,
    y);
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
  D_PTS(M_0_5, 1.0)};

// TODO: add angle
CellPoints& CellPoints::insert(
  const XYSize x,
  const XYSize y) noexcept
{
  if (can_burn_)
  {
    insert_pt(x, y, cell_x_y_, pts_);
  }
  return *this;
}
#undef D_PTS
void insert_pt(
  const InnerPos& p0,
  CellPointArrays& pts) noexcept
{
  const auto x0 = static_cast<DistanceSize>(p0.first);
  const auto y0 = static_cast<DistanceSize>(p0.second);

  // CHECK: FIX: is this initializing everything to false or just one element?
  for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
  {
    const auto& p1 = POINTS_OUTER[i];
    const auto& x1 = p1.first;
    const auto& y1 = p1.second;
    const auto d = ((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
    auto& p_d = pts.distances()[i];
    auto& p_p = pts.points()[i];
    p_p = (d < p_d) ? p0 : p_p;
    p_d = (d < p_d) ? d : p_d;
  }
}

void insert_pt(
  const InnerSize x,
  const InnerSize y,
  CellPointArrays& pts) noexcept
{
  // NOTE: use location inside cell so smaller types can be more precise
  // since digits aren't wasted on cell
  const auto p0 = InnerPos(x, y);
  insert_pt(p0, pts);
}
void insert_pt(
  const XYSize x,
  const XYSize y,
  const CellPos& cell_x_y,
  CellPointArrays& pts) noexcept
{
  insert_pt(
    static_cast<InnerSize>(x - cell_x_y.first),
    static_cast<InnerSize>(y - cell_x_y.second),
    pts);
}

CellPoints& CellPoints::insert(const InnerPos& p) noexcept
{
  insert(
    p.first,
    p.second);
  return *this;
}

// CellPointArrays::CellPointArrays(const InnerSize x, const InnerSize y)
//   : CellPointArrays(InnerPos(x, y))
// {
// }

// CellPointArrays::CellPointArrays(const InnerPos& p0)
// {
//   // need to calculate distances, but we know everything is the same point
//   const auto x0 = static_cast<DistanceSize>(p0.first);
//   const auto y0 = static_cast<DistanceSize>(p0.second);
//   std::fill_n(&(points()[0]), NUM_DIRECTIONS, p0);
//   auto& dists = distances();
//   for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
//   {
//     const auto& p1 = POINTS_OUTER[i];
//     const auto& x1 = p1.first;
//     const auto& y1 = p1.second;
//     dists[i] = ((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
//   }
// }
// CellPointArrays& CellPointArrays::insert(const InnerSize x, const InnerSize y)
// {
//   insert_pt(x, y, *this);
//   return *this;
// }
bool CellPoints::operator<(const CellPoints& rhs) const noexcept
{
  // if (can_burn_ == rhs.can_burn_)
  {
    if (cell_x_y_ == rhs.cell_x_y_)
    {
      return pts_.points() < rhs.pts_.points();
    }
    return cell_x_y_ < rhs.cell_x_y_;
  }
  // return can_burn_;
}
bool CellPoints::operator==(const CellPoints& rhs) const noexcept
{
  return (
    cell_x_y_ == rhs.cell_x_y_
    // && can_burn_ && rhs.can_burn_
    && pts_.points() == rhs.pts_.points());
}
bool CellPoints::empty() const
{
  logging::check_equal(
    INVALID_DISTANCE != pts_.distances()[0],
    can_burn_,
    "can_burn_");
  // NOTE: if anything is invalid then everything must be
  return (
    // !can_burn_ ||
    INVALID_DISTANCE == pts_.distances()[0]);
}
[[nodiscard]] Location CellPoints::location() const noexcept
{
  return Location{cell_x_y_.second, cell_x_y_.first};
}
CellPointsMap::CellPointsMap()
  : map_({})
{
}
CellPoints& CellPointsMap::insert(
  const BurnedData& unburnable,
  const Scenario& scenario,
  const XYSize x,
  const XYSize y) noexcept
{
#ifdef DEBUG_CELLPOINTS
  const auto n0 = size();
#endif
  const Location location{static_cast<Idx>(y), static_cast<Idx>(x)};
  auto e = map_.try_emplace(location.hash(),
                            unburnable,
                            scenario,
                            x,
                            y);
  CellPoints& cell_pts = e.first->second;
  if (!e.second)
  {
    // FIX: should use max of whatever ROS has entered during this time and not just first ros
    // tried to add new CellPoints but already there
    cell_pts.insert(
      x,
      y);
  }
  return cell_pts;
}
void CellPointsMap::remove_if(std::function<bool(const pair<HashSize, CellPoints>&)> F) noexcept
{
  auto it = map_.begin();
  while (map_.end() != it)
  {
    if (F(*it))
    {
      it = map_.erase(it);
    }
    else
    {
      ++it;
    }
  }
}
set<XYPos> CellPointsMap::unique() const noexcept
{
  set<XYPos> r{};
  for (auto& lp : map_)
  {
    for (auto& p : lp.second.unique())
    {
      r.insert(p);
    }
  }
  return r;
}
#ifdef DEBUG_CELLPOINTS
size_t CellPointsMap::size() const noexcept
{
  return unique().size();
}
#endif
}
