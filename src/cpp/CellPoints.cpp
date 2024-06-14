/* Copyright (c) Jordan Evens, 2005, 2021 */
/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "CellPoints.h"
#include "Log.h"
#include "ConvexHull.h"
#include "Location.h"

namespace tbd::sim
{
static const double MIN_X = std::numeric_limits<double>::min();
static const double MAX_X = std::numeric_limits<double>::max();
// const double TAN_PI_8 = std::tan(std::numbers::pi / 8);
// const double LEN_PI_8 = TAN_PI_8 / 2;
constexpr double DIST_22_5 = 0.2071067811865475244008443621048490392848359376884740365883398689;
constexpr double P_0_5 = 0.5 + DIST_22_5;
constexpr double M_0_5 = 0.5 - DIST_22_5;
static const InnerPos INVALID_POINT{};
//   static constexpr double INVALID_DISTANCE = std::numeric_limits<double>::max();
// not sure what's going on with this and wondering if it doesn't keep number exactly
// shouldn't be any way to be further than twice the entire width of the area
static const double INVALID_DISTANCE = static_cast<double>(MAX_ROWS * MAX_ROWS);

inline constexpr double distPtPt(const tbd::sim::InnerPos& a, const tbd::sim::InnerPos& b) noexcept
{
#ifdef _WIN32
  return (((b.x() - a.x()) * (b.x() - a.x())) + ((b.y() - a.y()) * (b.y() - a.y())));
#else
  return (std::pow((b.x() - a.x()), 2) + std::pow((b.y() - a.y()), 2));
#endif
}
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
const CellPoints::array_pts CellPoints::points() const
{
  return pts_;
}

CellPoints::CellPoints() noexcept
  : pts_({}),
    dists_({}),
    src_(topo::DIRECTION_NONE),
    is_empty_(true)
{
  std::fill(pts_.begin(), pts_.end(), INVALID_POINT);
  std::fill(dists_.begin(), dists_.end(), INVALID_DISTANCE);
}

// CellPoints::CellPoints(size_t) noexcept
//   : CellPoints()
// {
// }
CellPoints::CellPoints(const CellPoints* rhs) noexcept
  : CellPoints()
{
  if (nullptr != rhs)
  {
    merge(*rhs);
  }
}
CellPoints::CellPoints(const double x, const double y) noexcept
  : CellPoints()
{
  insert(x, y);
}

CellPoints& CellPoints::insert(const double x, const double y) noexcept
{
  const auto cell_x = static_cast<tbd::Idx>(x);
  const auto cell_y = static_cast<tbd::Idx>(y);
  const bool was_empty = is_empty_;
  insert(cell_x, cell_y, x, y);
  // HACK: somehow this makes it produce the same results as it was
  const auto u = unique();
  set<InnerPos> result{};
  for (size_t i = 0; i < pts_.size(); ++i)
  {
    if (was_empty)
    {
      logging::check_equal(pts_[i].x(), x, "point x");
      logging::check_equal(pts_[i].y(), y, "point y");
    }
    logging::check_fatal(
      INVALID_DISTANCE == dists_[i],
      "Invalid distance at position %ld",
      i);
  }

  return *this;
}

CellPoints::CellPoints(const InnerPos& p) noexcept
  : CellPoints()
{
  insert(p);
}

CellPoints& CellPoints::insert(const InnerPos& p) noexcept
{
  insert(p.x(), p.y());
  return *this;
}
CellPoints& CellPoints::insert(const double cell_x, const double cell_y, const double p_x, const double p_y) noexcept
{
  auto& n = dists_[FURTHEST_N];
  auto& nne = dists_[FURTHEST_NNE];
  auto& ne = dists_[FURTHEST_NE];
  auto& ene = dists_[FURTHEST_ENE];
  auto& e = dists_[FURTHEST_E];
  auto& ese = dists_[FURTHEST_ESE];
  auto& se = dists_[FURTHEST_SE];
  auto& sse = dists_[FURTHEST_SSE];
  auto& s = dists_[FURTHEST_S];
  auto& ssw = dists_[FURTHEST_SSW];
  auto& sw = dists_[FURTHEST_SW];
  auto& wsw = dists_[FURTHEST_WSW];
  auto& w = dists_[FURTHEST_W];
  auto& wnw = dists_[FURTHEST_WNW];
  auto& nw = dists_[FURTHEST_NW];
  auto& nnw = dists_[FURTHEST_NNW];
  const InnerPos p{p_x, p_y};
  const auto x = p.x() - cell_x;
  const auto y = p.y() - cell_y;
  // north is closest to point (0.5, 1.0)
  const auto cur_n = ((x - 0.5) * (x - 0.5)) + ((1 - y) * (1 - y));
  if (cur_n < n)
  {
    pts_[FURTHEST_N] = p;
    n = cur_n;
  }
  // south is closest to point (0.5, 0.0)
  const auto cur_s = ((x - 0.5) * (x - 0.5)) + (y * y);
  if (cur_s < s)
  {
    pts_[FURTHEST_S] = p;
    s = cur_s;
  }
  // northeast is closest to point (1.0, 1.0)
  const auto cur_ne = ((1 - x) * (1 - x)) + ((1 - y) * (1 - y));
  if (cur_ne < ne)
  {
    pts_[FURTHEST_NE] = p;
    ne = cur_ne;
  }
  // southwest is closest to point (0.0, 0.0)
  const auto cur_sw = (x * x) + (y * y);
  if (cur_sw < sw)
  {
    pts_[FURTHEST_SW] = p;
    sw = cur_sw;
  }
  // east is closest to point (1.0, 0.5)
  const auto cur_e = ((1 - x) * (1 - x)) + ((y - 0.5) * (y - 0.5));
  if (cur_e < e)
  {
    pts_[FURTHEST_E] = p;
    e = cur_e;
  }
  // west is closest to point (0.0, 0.5)
  const auto cur_w = (x * x) + ((y - 0.5) * (y - 0.5));
  if (cur_w < w)
  {
    pts_[FURTHEST_W] = p;
    w = cur_w;
  }
  // southeast is closest to point (1.0, 0.0)
  const auto cur_se = ((1 - x) * (1 - x)) + (y * y);
  if (cur_se < se)
  {
    pts_[FURTHEST_SE] = p;
    se = cur_se;
  }
  // northwest is closest to point (0.0, 1.0)
  const auto cur_nw = (x * x) + ((1 - y) * (1 - y));
  if (cur_nw < nw)
  {
    pts_[FURTHEST_NW] = p;
    nw = cur_nw;
  }
  // south-southwest is closest to point (0.5 - 0.207, 0.0)
  const auto cur_ssw = ((x - M_0_5) * (x - M_0_5)) + (y * y);
  if (cur_ssw < ssw)
  {
    pts_[FURTHEST_SSW] = p;
    ssw = cur_ssw;
  }
  // south-southeast is closest to point (0.5 + 0.207, 0.0)
  const auto cur_sse = ((x - P_0_5) * (x - P_0_5)) + (y * y);
  if (cur_sse < sse)
  {
    pts_[FURTHEST_SSE] = p;
    sse = cur_sse;
  }
  // north-northwest is closest to point (0.5 - 0.207, 1.0)
  const auto cur_nnw = ((x - M_0_5) * (x - M_0_5)) + ((1 - y) * (1 - y));
  if (cur_nnw < nnw)
  {
    pts_[FURTHEST_NNW] = p;
    nnw = cur_nnw;
  }
  // north-northeast is closest to point (0.5 + 0.207, 1.0)
  const auto cur_nne = ((x - P_0_5) * (x - P_0_5)) + ((1 - y) * (1 - y));
  if (cur_nne < nne)
  {
    pts_[FURTHEST_NNE] = p;
    nne = cur_nne;
  }
  // west-southwest is closest to point (0.0, 0.5 - 0.207)
  const auto cur_wsw = (x * x) + ((y - M_0_5) * (y - M_0_5));
  if (cur_wsw < wsw)
  {
    pts_[FURTHEST_WSW] = p;
    wsw = cur_wsw;
  }
  // west-northwest is closest to point (0.0, 0.5 + 0.207)
  const auto cur_wnw = (x * x) + ((y - P_0_5) * (y - P_0_5));
  if (cur_wnw < wnw)
  {
    pts_[FURTHEST_WNW] = p;
    wnw = cur_wnw;
  }
  // east-southeast is closest to point (1.0, 0.5 - 0.207)
  const auto cur_ese = ((1 - x) * (1 - x)) + ((y - M_0_5) * (y - M_0_5));
  if (cur_ese < ese)
  {
    pts_[FURTHEST_ESE] = p;
    ese = cur_ese;
  }
  // east-northeast is closest to point (1.0, 0.5 + 0.207)
  const auto cur_ene = ((1 - x) * (1 - x)) + ((y - P_0_5) * (y - P_0_5));
  if (cur_ene < ene)
  {
    pts_[FURTHEST_ENE] = p;
    ene = cur_ene;
  }
  // either we had something already or we should now?
  is_empty_ = false;
  return *this;
}
CellPoints::CellPoints(const vector<InnerPos>& pts) noexcept
  : CellPoints()
{
  insert(pts.begin(), pts.end());
}
void CellPoints::add_source(const CellIndex src)
{
  src_ |= src;
}
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
    is_empty_ |= rhs.is_empty_;
  }
  // HACK: allow empty list but still having a source
  src_ |= rhs.src_;
  return *this;
}
const cellpoints_map_type apply_offsets_spreadkey(
  const double duration,
  const OffsetSet& offsets,
  const points_type& cell_pts)
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
      const OffsetSet& pts = std::get<1>(pts_for_cell);
      for (const auto& p : pts)
      {
        // putting results in copy of offsets and returning that
        // at the end of everything, we're just adding something to every double in the set by duration?
        const double x = x_o + p.x();
        const double y = y_o + p.y();
        // don't need cell attributes, just location
        //   Location dst = Location(
        //     static_cast<Idx>(y),
        //     static_cast<Idx>(x));

        auto e = r1.try_emplace(
          Location{
            static_cast<Idx>(y),
            static_cast<Idx>(x)},
          nullptr);

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
            logging::check_fatal(dists[i] == INVALID_DISTANCE,
                                 "Distance %ld is invalid",
                                 i);
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
        // pair1.second.insert(x, y);
        const Location& dst = e.first->first;
        if (src != dst)
        {
          // we inserted a pair of (src, dst), which means we've never
          // calculated the relativeIndex for this so add it to main map
          cell_pts.add_source(
            relativeIndex(
              src,
              dst));
        }
      }
    }
  }
  return static_cast<const cellpoints_map_type>(r1);
}

// const merged_map_type convert_map(const cellpoints_map_type& m)
// {
//   merged_map_type merged{};
//   for (const auto& kv : m)
//   {
//     const Location dst = kv.first;
//     auto& for_dst = merged[dst];
//     const CellIndex src = kv.second.first;
//     const CellPoints& pts = kv.second.second;
//     const auto u = pts.unique();
//     for_dst.first |= src;
//     for_dst.second.insert(for_dst.second.end(), u.begin(), u.end());
//   }
//   return merged;
// }
}
