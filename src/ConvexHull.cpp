// Copyright (c) 2005-2022, Jordan Evens
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "ConvexHull.h"

constexpr double MIN_X = std::numeric_limits<double>::min();
constexpr double MAX_X = std::numeric_limits<double>::max();

inline constexpr double distPtPt(const tbd::sim::InnerPos& a, const tbd::sim::InnerPos& b) noexcept
{
  return (std::pow((b.x - a.x), 2) + std::pow((b.y - a.y), 2));
}
void hull(vector<tbd::sim::InnerPos>& a) noexcept
{
  if (a.size() > MAX_BEFORE_CONDENSE)
  {
    size_t n_pos = 0;
    auto n = numeric_limits<double>::min();
    size_t ne_pos = 0;
    auto ne = numeric_limits<double>::min();
    size_t e_pos = 0;
    auto e = numeric_limits<double>::min();
    size_t se_pos = 0;
    auto se = numeric_limits<double>::min();
    size_t s_pos = 0;
    auto s = numeric_limits<double>::min();
    size_t sw_pos = 0;
    auto sw = numeric_limits<double>::min();
    size_t w_pos = 0;
    auto w = numeric_limits<double>::min();
    size_t nw_pos = 0;
    auto nw = numeric_limits<double>::min();
    // should always be in the same cell so do this once
    const auto cell_x = static_cast<tbd::Idx>(a[0].x);
    const auto cell_y = static_cast<tbd::Idx>(a[0].y);
    for (size_t i = 0; i < a.size(); ++i)
    {
      const auto& p = a[i];
      const auto x = p.x - cell_x;
      const auto y = p.y - cell_y;
      const auto cur_n = y * y;
      if (cur_n > n)
      {
        n_pos = i;
        n = cur_n;
      }
      const auto cur_s = ((1 - y) * (1 - y));
      if (cur_s > s)
      {
        s_pos = i;
        s = cur_s;
      }
      const auto cur_ne = (x * x) + (y * y);
      if (cur_ne > ne)
      {
        ne_pos = i;
        ne = cur_ne;
      }
      const auto cur_sw = ((1 - x) * (1 - x)) + ((1 - y) * (1 - y));
      if (cur_sw > sw)
      {
        sw_pos = i;
        sw = cur_sw;
      }
      const auto cur_e = (x * x);
      if (cur_e > e)
      {
        e_pos = i;
        e = cur_e;
      }
      const auto cur_w = ((1 - x) * (1 - x));
      if (cur_w > w)
      {
        w_pos = i;
        w = cur_w;
      }
      const auto cur_se = (x * x) + ((1 - y) * (1 - y));
      if (cur_se > se)
      {
        se_pos = i;
        se = cur_se;
      }
      const auto cur_nw = ((1 - x) * (1 - x)) + (y * y);
      if (cur_nw > nw)
      {
        nw_pos = i;
        nw = cur_nw;
      }
    }
    a = {
      a[n_pos],
      a[ne_pos],
      a[e_pos],
      a[se_pos],
      a[s_pos],
      a[sw_pos],
      a[w_pos],
      a[nw_pos]
    };
    tbd::logging::check_fatal(a.size() > 8, "Expected <= 8 points but have %ld", a.size());
  }
  else
  {
    tbd::logging::note("Called when shouldn't have");
  }
}
