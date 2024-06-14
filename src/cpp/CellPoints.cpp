/* Copyright (c) Jordan Evens, 2005, 2021 */
/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "CellPoints.h"
#include "Log.h"
#include "ConvexHull.h"

namespace tbd::sim
{
static const double MIN_X = std::numeric_limits<double>::min();
static const double MAX_X = std::numeric_limits<double>::max();
// const double TAN_PI_8 = std::tan(std::numbers::pi / 8);
// const double LEN_PI_8 = TAN_PI_8 / 2;
constexpr double DIST_22_5 = 0.2071067811865475244008443621048490392848359376884740365883398689;
constexpr double P_0_5 = 0.5 + DIST_22_5;
constexpr double M_0_5 = 0.5 - DIST_22_5;

inline constexpr double distPtPt(const tbd::sim::InnerPos& a, const tbd::sim::InnerPos& b) noexcept
{
#ifdef _WIN32
  return (((b.x() - a.x()) * (b.x() - a.x())) + ((b.y() - a.y()) * (b.y() - a.y())));
#else
  return (std::pow((b.x() - a.x()), 2) + std::pow((b.y() - a.y()), 2));
#endif
}

CellPoints::CellPoints() noexcept
  : pts_(),
    dists_()
{
  std::fill_n(dists_.begin(), NUM_DIRECTIONS, INVALID_DISTANCE);
}

CellPoints::CellPoints(size_t) noexcept
  : CellPoints()
{
}

CellPoints::CellPoints(const double x, const double y) noexcept
  : CellPoints()
{
  insert(x, y);
}

void CellPoints::insert(const double x, const double y) noexcept
{
  InnerPos p{x, y};
  insert(p);
  logging::check_fatal(
    p.x() != x || p.y() != y,
    "Inserting (%0.4f, %0.4f) gives (%0.4f, %0.4f)\n",
    x,
    y,
    p.x(),
    p.y());
  //   insert(InnerPos{x, y});
}

CellPoints::CellPoints(const InnerPos& p) noexcept
  : CellPoints()
{
  insert(p);
}

void CellPoints::insert(const InnerPos& p) noexcept
{
  // should always be in the same cell so do this once
  const auto cell_x = static_cast<tbd::Idx>(p.x());
  const auto cell_y = static_cast<tbd::Idx>(p.y());
  insert(cell_x, cell_y, p);
}
void CellPoints::insert(const double cell_x, const double cell_y, const InnerPos& p) noexcept
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
  auto& n_pos = pts_[FURTHEST_N];
  auto& nne_pos = pts_[FURTHEST_NNE];
  auto& ne_pos = pts_[FURTHEST_NE];
  auto& ene_pos = pts_[FURTHEST_ENE];
  auto& e_pos = pts_[FURTHEST_E];
  auto& ese_pos = pts_[FURTHEST_ESE];
  auto& se_pos = pts_[FURTHEST_SE];
  auto& sse_pos = pts_[FURTHEST_SSE];
  auto& s_pos = pts_[FURTHEST_S];
  auto& ssw_pos = pts_[FURTHEST_SSW];
  auto& sw_pos = pts_[FURTHEST_SW];
  auto& wsw_pos = pts_[FURTHEST_WSW];
  auto& w_pos = pts_[FURTHEST_W];
  auto& wnw_pos = pts_[FURTHEST_WNW];
  auto& nw_pos = pts_[FURTHEST_NW];
  auto& nnw_pos = pts_[FURTHEST_NNW];
  const auto x = p.x() - cell_x;
  const auto y = p.y() - cell_y;
  // north is closest to point (0.5, 1.0)
  const auto cur_n = ((x - 0.5) * (x - 0.5)) + ((1 - y) * (1 - y));
  if (cur_n < n)
  {
    n_pos = p;
    n = cur_n;
  }
  // south is closest to point (0.5, 0.0)
  const auto cur_s = ((x - 0.5) * (x - 0.5)) + (y * y);
  if (cur_s < s)
  {
    s_pos = p;
    s = cur_s;
  }
  // northeast is closest to point (1.0, 1.0)
  const auto cur_ne = ((1 - x) * (1 - x)) + ((1 - y) * (1 - y));
  if (cur_ne < ne)
  {
    ne_pos = p;
    ne = cur_ne;
  }
  // southwest is closest to point (0.0, 0.0)
  const auto cur_sw = (x * x) + (y * y);
  if (cur_sw < sw)
  {
    sw_pos = p;
    sw = cur_sw;
  }
  // east is closest to point (1.0, 0.5)
  const auto cur_e = ((1 - x) * (1 - x)) + ((y - 0.5) * (y - 0.5));
  if (cur_e < e)
  {
    e_pos = p;
    e = cur_e;
  }
  // west is closest to point (0.0, 0.5)
  const auto cur_w = (x * x) + ((y - 0.5) * (y - 0.5));
  if (cur_w < w)
  {
    w_pos = p;
    w = cur_w;
  }
  // southeast is closest to point (1.0, 0.0)
  const auto cur_se = ((1 - x) * (1 - x)) + (y * y);
  if (cur_se < se)
  {
    se_pos = p;
    se = cur_se;
  }
  // northwest is closest to point (0.0, 1.0)
  const auto cur_nw = (x * x) + ((1 - y) * (1 - y));
  if (cur_nw < nw)
  {
    nw_pos = p;
    nw = cur_nw;
  }
  // south-southwest is closest to point (0.5 - 0.207, 0.0)
  const auto cur_ssw = ((x - M_0_5) * (x - M_0_5)) + (y * y);
  if (cur_ssw < ssw)
  {
    ssw_pos = p;
    ssw = cur_ssw;
  }
  // south-southeast is closest to point (0.5 + 0.207, 0.0)
  const auto cur_sse = ((x - P_0_5) * (x - P_0_5)) + (y * y);
  if (cur_sse < sse)
  {
    sse_pos = p;
    sse = cur_sse;
  }
  // north-northwest is closest to point (0.5 - 0.207, 1.0)
  const auto cur_nnw = ((x - M_0_5) * (x - M_0_5)) + ((1 - y) * (1 - y));
  if (cur_nnw < nnw)
  {
    nnw_pos = p;
    nnw = cur_nnw;
  }
  // north-northeast is closest to point (0.5 + 0.207, 1.0)
  const auto cur_nne = ((x - P_0_5) * (x - P_0_5)) + ((1 - y) * (1 - y));
  if (cur_nne < nne)
  {
    nne_pos = p;
    nne = cur_nne;
  }
  // west-southwest is closest to point (0.0, 0.5 - 0.207)
  const auto cur_wsw = (x * x) + ((y - M_0_5) * (y - M_0_5));
  if (cur_wsw < wsw)
  {
    wsw_pos = p;
    wsw = cur_wsw;
  }
  // west-northwest is closest to point (0.0, 0.5 + 0.207)
  const auto cur_wnw = (x * x) + ((y - P_0_5) * (y - P_0_5));
  if (cur_wnw < wnw)
  {
    wnw_pos = p;
    wnw = cur_wnw;
  }
  // east-southeast is closest to point (1.0, 0.5 - 0.207)
  const auto cur_ese = ((1 - x) * (1 - x)) + ((y - M_0_5) * (y - M_0_5));
  if (cur_ese < ese)
  {
    ese_pos = p;
    ese = cur_ese;
  }
  // east-northeast is closest to point (1.0, 0.5 + 0.207)
  const auto cur_ene = ((1 - x) * (1 - x)) + ((y - P_0_5) * (y - P_0_5));
  if (cur_ene < ene)
  {
    ene_pos = p;
    ene = cur_ene;
  }
}
CellPoints::CellPoints(const vector<InnerPos>& pts) noexcept
  : CellPoints()
{
  insert(pts.begin(), pts.end());
}
void CellPoints::insert(const CellPoints& rhs)
{
  // we know distances in each direction so just pick closer
  for (size_t i = 0; i < pts_.size(); ++i)
  {
    if (rhs.dists_[i] < dists_[i])
    {
      // closer so replace
      dists_[i] = rhs.dists_[i];
      pts_[i] = rhs.pts_[i];
    }
  }
}
const merged_map_type apply_offsets_spreadkey(
  const double duration,
  const OffsetSet& offsets,
  const points_type& cell_pts)
{
  using cellpoints_map_type = map<Location, pair<CellIndex, CellPoints>>;
  // NOTE: really tried to do this in parallel, but not enough points
  // in a cell for it to work well
  merged_map_type result{};
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
        // try to insert a pair with no direction and no points
        auto e = result.try_emplace(
          Location{
            static_cast<Idx>(y),
            static_cast<Idx>(x)},
          tbd::topo::DIRECTION_NONE,
          NULL);
        {
          auto& pair1 = e.first->second;
          // always add point since we're calling try_emplace with empty list
          pair1.second.emplace_back(x, y);
          // pair1.second.insert(x, y);
          const Location& dst = e.first->first;
          if (src != dst)
          {
            // we inserted a pair of (src, dFst), which means we've never
            // calculated the relativeIndex for this so add it to main map
            pair1.first |= relativeIndex(
              src,
              dst);
          }
        }
        {
          auto e1 = r1.try_emplace(
            Location{
              static_cast<Idx>(y),
              static_cast<Idx>(x)},
            tbd::topo::DIRECTION_NONE,
            NULL);
          logging::check_fatal(e.second != e1.second,
                               "Inserted into one but not other");
          // FIX: nested so we can use same variable names
          auto& pair1 = e1.first->second;
          // always add point since we're calling try_emplace with empty list
          pair1.second.insert(x, y);
          // pair1.second.insert(x, y);
          const Location& dst = e.first->first;
          if (src != dst)
          {
            // we inserted a pair of (src, dst), which means we've never
            // calculated the relativeIndex for this so add it to main map
            pair1.first |= relativeIndex(
              src,
              dst);
          }
          auto& pair0 = e.first->second;
          const auto& pts_old = pair0.second;
          auto c1 = CellPoints(pts_old);
          auto& c0 = pair1.second;
          // make sure CellPoints created by insertion match construction from list version
          for (size_t i = 0; i < c0.pts_.size(); ++i)
          {
            auto& d0 = c1.dists_[i];
            auto& d1 = c0.dists_[i];
            auto& p0 = c0.pts_[i];
            auto& p1 = c1.pts_[i];
            logging::check_equal(d0, d1, "distance");
            logging::check_equal(p0.x(), p1.x(), "x");
            logging::check_equal(p0.y(), p1.y(), "y");
          }
          auto s0 = c0.unique();
          vector<tbd::sim::InnerPos> pts_hull{pts_old.begin(), pts_old.end()};
          hull(pts_hull);
          set<Offset> s1{pts_hull.begin(), pts_hull.end()};
          if (s0 != s1)
          {
            for (const auto& p : s0)
            {
              printf("(%0.4f, %0.4f)\n", p.x(), p.y());
            }
            printf("***************************\n");
            for (const auto& p : s1)
            {
              printf("(%0.4f, %0.4f)\n", p.x(), p.y());
            }
            //   logging::check_equal(s0.size(), s1.size(), "number of points");
            logging::check_fatal(s0 != s1,
                                 "Expected sets to be equal");
          }
        }
      }
    }
  }
  return static_cast<const merged_map_type>(result);
}
}