/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "ConvexHull.h"
namespace fs
{
constexpr double MIN_X = std::numeric_limits<double>::min();
constexpr double MAX_X = std::numeric_limits<double>::max();
inline constexpr double distPtPt(const InnerPos& a, const InnerPos& b) noexcept
{
#ifdef _WIN32
  return (((b.x - a.x) * (b.x - a.x)) + ((b.y - a.y) * (b.y - a.y)));
#else
  return (std::pow((b.x - a.x), 2) + std::pow((b.y - a.y), 2));
#endif
}
void hull(vector<InnerPos>& a) noexcept
{
  if (a.size() > MAX_BEFORE_CONDENSE)
  {
    size_t n_pos = 0;
    auto n = numeric_limits<double>::max();
    size_t ne_pos = 0;
    auto ne = numeric_limits<double>::max();
    size_t e_pos = 0;
    auto e = numeric_limits<double>::max();
    size_t se_pos = 0;
    auto se = numeric_limits<double>::max();
    size_t s_pos = 0;
    auto s = numeric_limits<double>::max();
    size_t sw_pos = 0;
    auto sw = numeric_limits<double>::max();
    size_t w_pos = 0;
    auto w = numeric_limits<double>::max();
    size_t nw_pos = 0;
    auto nw = numeric_limits<double>::max();
    // should always be in the same cell so do this once
    const auto cell_x = static_cast<fs::Idx>(a[0].x);
    const auto cell_y = static_cast<fs::Idx>(a[0].y);
    for (size_t i = 0; i < a.size(); ++i)
    {
      const auto& p = a[i];
      const auto x = p.x - cell_x;
      const auto y = p.y - cell_y;
      // north is closest to point (0.5, 1.0)
      const auto cur_n = ((x - 0.5) * (x - 0.5)) + ((1 - y) * (1 - y));
      if (cur_n < n)
      {
        n_pos = i;
        n = cur_n;
      }
      // south is closest to point (0.5, 0.0)
      const auto cur_s = ((x - 0.5) * (x - 0.5)) + (y * y);
      if (cur_s < s)
      {
        s_pos = i;
        s = cur_s;
      }
      // northeast is closest to point (1.0, 1.0)
      const auto cur_ne = ((1 - x) * (1 - x)) + ((1 - y) * (1 - y));
      if (cur_ne < ne)
      {
        ne_pos = i;
        ne = cur_ne;
      }
      // southwest is closest to point (0.0, 0.0)
      const auto cur_sw = (x * x) + (y * y);
      if (cur_sw < sw)
      {
        sw_pos = i;
        sw = cur_sw;
      }
      // east is closest to point (1.0, 0.5)
      const auto cur_e = ((1 - x) * (1 - x)) + ((y - 0.5) * (y - 0.5));
      if (cur_e < e)
      {
        e_pos = i;
        e = cur_e;
      }
      // west is closest to point (0.0, 0.5)
      const auto cur_w = (x * x) + ((y - 0.5) * (y - 0.5));
      if (cur_w < w)
      {
        w_pos = i;
        w = cur_w;
      }
      // southeast is closest to point (1.0, 0.0)
      const auto cur_se = ((1 - x) * (1 - x)) + (y * y);
      if (cur_se < se)
      {
        se_pos = i;
        se = cur_se;
      }
      // northwest is closest to point (0.0, 1.0)
      const auto cur_nw = (x * x) + ((1 - y) * (1 - y));
      if (cur_nw < nw)
      {
        nw_pos = i;
        nw = cur_nw;
      }
    }
    a = {a[n_pos], a[ne_pos], a[e_pos], a[se_pos], a[s_pos], a[sw_pos], a[w_pos], a[nw_pos]};
    fs::logging::check_fatal(a.size() > 8, "Expected <= 8 points but have %ld", a.size());
  }
  else
  {
    fs::logging::note("Called when shouldn't have");
  }
}
}
