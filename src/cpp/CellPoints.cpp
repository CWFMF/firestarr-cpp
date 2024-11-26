/* Copyright (c) Jordan Evens, 2005, 2021 */
/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "CellPoints.h"
#include "Log.h"
#include "Location.h"
#include "Scenario.h"

namespace tbd::sim
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
set<XYPos> CellPoints::unique() const noexcept
{
  // // if any point is invalid then they all have to be
  if (INVALID_DISTANCE == pts_.first[0])
  {
    return {};
  }
  else
  {
    const auto& pts_all = std::views::transform(
      pts_.second,
      [this](const auto& p) {
        return XYPos(p.first + cell_x_y_.first, p.second + cell_x_y_.second);
      });
    return {pts_all.cbegin(), pts_all.cend()};
  }
}
#ifdef DEBUG_CELLPOINTS
size_t CellPoints::size() const noexcept
{
  return unique().size();
}
#endif
CellPoints::CellPoints(const Idx cell_x, const Idx cell_y) noexcept
  : arrival_time_(INVALID_TIME),
    intensity_at_arrival_(0),
    ros_at_arrival_(INVALID_ROS),
    raz_at_arrival_(Direction::Invalid),
    pts_({}),
    cell_x_y_(cell_x, cell_y),
    src_(topo::DIRECTION_NONE)
{
  std::fill(pts_.first.begin(), pts_.first.end(), INVALID_DISTANCE);
  std::fill(pts_.second.begin(), pts_.second.end(), INVALID_INNER_POSITION);

#ifdef DEBUG_CELLPOINTS
  logging::note("CellPoints is size %ld after creation and should be empty", size());
#endif
}
CellPoints::CellPoints() noexcept
  : CellPoints(INVALID_XY_LOCATION, INVALID_XY_LOCATION)
{
}
CellPoints::CellPoints(const CellPoints* rhs) noexcept
  : CellPoints()
{
  logging::check_fatal(nullptr == rhs, "Initializing CellPoints from nullptr");
  *this = *rhs;
}
CellPoints::CellPoints(
  const DurationSize& arrival_time,
  const IntensitySize intensity,
  const ROSSize& ros,
  const Direction& raz,
  const XYSize x,
  const XYSize y) noexcept
  : CellPoints(static_cast<Idx>(x), static_cast<Idx>(y))
{
  insert(arrival_time, intensity, ros, raz, x, y);
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

CellPoints& CellPoints::insert(
  const DurationSize& arrival_time,
  const IntensitySize intensity,
  const ROSSize& ros,
  const Direction& raz,
  const XYSize x,
  const XYSize y) noexcept
{
#ifdef DEBUG_CELLPOINTS
  logging::note(
    "Insert (%f, %f) at time %f with ROS %f, Intensity %d, RAZ %f",
    x,
    y,
    arrival_time,
    ros,
    intensity,
    raz.asDegrees());
#endif
  // constexpr auto TIME_EPSILON = 0.000001;
  if (0 < arrival_time && 0 > arrival_time_)
  {
    logging::verbose("No time so setting ros to %f at time %f", ros, arrival_time);
    // record ros and time if nothing yet
    arrival_time_ = arrival_time;
    ros_at_arrival_ = ros;
    intensity_at_arrival_ = intensity;
    raz_at_arrival_ = raz;
  }
  // // else if (abs(arrival_time - arrival_time_) <= TIME_EPSILON)
  // else if (arrival_time == arrival_time_)
  // {
  //   logging::verbose("Same time so setting ros to max(%f, %f) at time %f", ros, ros_at_arrival_, arrival_time);
  //   // the same time so pick higher ros
  //   if (
  //     (ros_at_arrival_ < ros)
  //     || (ros_at_arrival_ == ros && intensity > intensity_at_arrival_))
  //   {
  //     ros_at_arrival_ = ros;
  //     intensity_at_arrival_ = intensity;
  //     raz_at_arrival_ = raz;
  //   }
  //   // arrival_time_ = arrival_time;
  // }
  // NOTE: use location inside cell so smaller types can be more precise
  // since digits aren't wasted on cell
  const auto p0 = InnerPos(
    static_cast<InnerSize>(x - cell_x_y_.first),
    static_cast<InnerSize>(y - cell_x_y_.second));
  const auto x0 = static_cast<DistanceSize>(p0.first);
  const auto y0 = static_cast<DistanceSize>(p0.second);
  // static_assert(pts_.first.size() == NUM_DIRECTIONS);
  for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
  {
    const auto& p1 = POINTS_OUTER[i];
    const auto& x1 = p1.first;
    const auto& y1 = p1.second;
    const auto d = ((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
    auto& p_d = pts_.first[i];
    auto& p_p = pts_.second[i];
    p_p = (d < p_d) ? p0 : p_p;
    p_d = (d < p_d) ? d : p_d;
    // // worse than two checks + assignment
    // const auto& [p_new, d_new] =
    //   (d < p_d)
    //     ? std::make_tuple(p0, d)
    //     : std::make_tuple(p_p, p_d);
    // p_p = p_new;
    // p_d = d_new;
    // // worse than two checks + assignment
    // std::tie(p_d, p_p) =
    //   (d < p_d)
    //     ? std::make_tuple(d, p0)
    //     : std::make_tuple(p_d, p_p);
  }
#ifdef DEBUG_CELLPOINTS
  logging::note("now have %ld points", size());
#endif
  return *this;
}
#undef D_PTS
CellPoints::CellPoints(const XYPos& p) noexcept
  : CellPoints(p.first, p.second)
{
}

CellPoints& CellPoints::insert(const InnerPos& p) noexcept
{
  // HACK: FIX: just do something for now
  insert(
    INVALID_TIME,
    NO_INTENSITY,
    NO_ROS,
    Direction::Invalid,
    p.first,
    p.second);
  return *this;
}

void CellPoints::add_source(const CellIndex src)
{
  src_ |= src;
}
CellPoints& CellPoints::merge(const CellPoints& rhs)
{
#ifdef DEBUG_CELLPOINTS
  const auto n0 = size();
  const auto n1 = rhs.size();
#endif
  // either both invalid or lower one is valid
  cell_x_y_ = min(cell_x_y_, rhs.cell_x_y_);
  // we know distances in each direction so just pick closer
  for (size_t i = 0; i < pts_.first.size(); ++i)
  {
    if (rhs.pts_.first[i] < pts_.first[i])
    {
      pts_.first[i] = rhs.pts_.first[i];
      pts_.second[i] = rhs.pts_.second[i];
    }
  }
  add_source(rhs.src_);
#ifdef DEBUG_CELLPOINTS
  logging::note(
    "Merging %ld with %ld gives %ld pts",
    n0,
    n1,
    size());
#endif
  return *this;
}
bool CellPoints::operator<(const CellPoints& rhs) const noexcept
{
  if (cell_x_y_ == rhs.cell_x_y_)
  {
    return pts_.second < rhs.pts_.second;
    // all points are equal if we got here
  }
  return cell_x_y_ < rhs.cell_x_y_;
}
bool CellPoints::operator==(const CellPoints& rhs) const noexcept
{
  return (
    cell_x_y_ == rhs.cell_x_y_
    && pts_.second == rhs.pts_.second);
}
bool CellPoints::empty() const
{
  // NOTE: if anything is invalid then everything must be
  return (INVALID_DISTANCE == pts_.first[0]);
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
  const DurationSize& arrival_time,
  const IntensitySize intensity,
  const ROSSize& ros,
  const Direction& raz,
  const XYSize x,
  const XYSize y) noexcept
{
#ifdef DEBUG_CELLPOINTS
  const auto n0 = size();
#endif
  const Location location{static_cast<Idx>(y), static_cast<Idx>(x)};
  auto e = map_.try_emplace(location, arrival_time, intensity, ros, raz, x, y);
  CellPoints& cell_pts = e.first->second;
  if (!e.second)
  {
    // FIX: should use max of whatever ROS has entered during this time and not just first ros
    // tried to add new CellPoints but already there
    cell_pts.insert(arrival_time, intensity, ros, raz, x, y);
  }
#ifdef DEBUG_CELLPOINTS
  logging::note(
    "insert with size %ld of (%f, %f) at time %f with ROS %f gives size %ld",
    n0,
    x,
    y,
    arrival_time,
    ros,
    size());
#endif
  return cell_pts;
}
CellPoints& CellPointsMap::insert(
  const Location& src,
  const DurationSize& arrival_time,
  const IntensitySize intensity,
  const ROSSize& ros,
  const Direction& raz,
  const XYSize x,
  const XYSize y) noexcept
{
  CellPoints& cell_pts = insert(arrival_time, intensity, ros, raz, x, y);
  const Location& dst = cell_pts.location();
  // adds 0 if the same so try without checking
  // if (src != dst)
  {
    // we inserted a pair of (src, dst), which means we've never
    // calculated the relativeIndex for this so add it to main map
    cell_pts.add_source(
      relativeIndex(
        src,
        dst));
  }
  return cell_pts;
}
CellPointsMap& CellPointsMap::merge(
  const BurnedData& unburnable,
  const CellPointsMap& rhs) noexcept
{
  // FIX: if we iterate through both they should be sorted
  for (const auto& kv : rhs.map_)
  {
    const auto h = kv.first.hash();
    if (!unburnable[h])
    {
      const CellPoints& pts = kv.second;
      const Location location = pts.location();
      auto e = map_.try_emplace(location, pts);
      CellPoints& cell_pts = e.first->second;
      if (!e.second)
      {
        // couldn't insert
        cell_pts.merge(pts);
      }
    }
  }
  return *this;
}
void CellPointsMap::remove_if(std::function<bool(const pair<Location, CellPoints>&)> F) noexcept
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
