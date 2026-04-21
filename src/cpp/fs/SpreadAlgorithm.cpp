/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "SpreadAlgorithm.h"
#include "Cell.h"
#include "FuelType.h"
#include "Location.h"
#include "Log.h"
#include "Radians.h"
#include "RangeIterator.h"
#include "unstable.h"
#include "Util.h"
namespace fs
{
// Offset calc_offset(
//   const Radians theta,
//   const MathSize min_ros,
//   const MathSize cell_size,
//   HorizontalAdjustment correction_factor_xy,
//   MathSize tfc,
//   const Radians& head_raz,
//   MathSize head_ros,
//   MathSize back_ros,
//   MathSize length_to_breadth
// ) noexcept
// {
//   auto correction_factor = [&](const Radians& theta) {
//     const auto [x, y] = correction_factor_xy(theta);
//     // CHECK: Pretty sure you can't spread farther horizontally than the spread distance,
//     regardless
//     // of angle?
//     return min(1.0, sqrt(x * x + y * y));
//   };
//   const auto a = (head_ros + back_ros) / 2.0;
//   const auto c = a - back_ros;
//   const auto flank_ros = a / length_to_breadth;
//   const auto a_sq = a * a;
//   const auto flank_ros_sq = flank_ros * flank_ros;
//   const auto a_sq_sub_c_sq = a_sq - (c * c);
//   const auto ac = a * c;
//   const auto cos_t = cos(theta);
//   const auto cos_t_sq = cos_t * cos_t;
//   const auto f_sq_cos_t_sq = flank_ros_sq * cos_t_sq;
//   const auto sin_t_sq = 1.0 - cos_t_sq;
//   // // or could be: 1.0 = cos^2 + sin^2
//   // const auto sin_t = sin(theta);
//   // const auto sin_t_sq = sin_t * sin_t;
//   // or could be: 1.0 = cos^2 + sin^2
//   const auto ros = abs(
//     (a * ((flank_ros * cos_t * sqrt(f_sq_cos_t_sq + a_sq_sub_c_sq * sin_t_sq) - ac * sin_t_sq) /
//     (f_sq_cos_t_sq + a_sq * sin_t_sq))
//      + c)
//     / cos_t
//   );
//   if (ros < min_ros)
//   {
//     return Offset{0, 0};
//   }
//   const auto ros_cell = ros / cell_size;
//   const auto intensity = fire_intensity(tfc, ros);
//   // spreading, so figure out offset from current point
//   return Offset{
//     static_cast<DistanceSize>(ros_cell * sin(theta)),
//     static_cast<DistanceSize>(ros_cell * cos(theta))
//   };
// }
HorizontalAdjustment horizontal_adjustment(const AspectSize slope_azimuth, const SlopeSize slope)
{
  // do everything we can to avoid calling trig functions unnecessarily
  constexpr auto no_correction = [](const Radians&) noexcept { return std::tuple{1.0, 1.0}; };
  if (0 == slope)
  {
    // do check once and make function just return 1.0 if no slope
    return no_correction;
  }
  const auto b_semi = cos(atan(slope / 100.0));
  const Radians slope_radians{Degrees{slope_azimuth}};
  const auto do_correction = [=](const Radians& theta) noexcept {
    // never gets called if isInvalid() so don't check
    // figure out how far the ground distance is in map distance horizontally
    const auto angle_unrotated = theta - slope_radians;
    // if (to_degrees(angle_unrotated) == 270 || to_degrees(angle_unrotated) == 90)
    // {
    //   // CHECK: if we're going directly across the slope then horizontal distance is same as
    //   spread
    //   // distance
    //   return 1.0;
    // }
    const auto tan_u = tan(angle_unrotated);
    const auto y = b_semi / sqrt(b_semi * tan_u * (b_semi * tan_u) + 1.0);
    const auto x = y * tan_u;
    return std::tuple{x, y};
  };
  return do_correction;
}
[[nodiscard]] OffsetSet OriginalSpreadAlgorithm::calculate_offsets(
  HorizontalAdjustment correction_factor_xy,
  MathSize tfc,
  const Radians& head_raz,
  MathSize head_ros,
  MathSize back_ros,
  MathSize length_to_breadth
) const noexcept
{
  auto correction_factor = [&](const Radians& theta) {
    const auto [x, y] = correction_factor_xy(theta);
    // CHECK: Pretty sure you can't spread farther horizontally than the spread distance, regardless
    // of angle?
    return min(1.0, sqrt(x * x + y * y));
  };
  OffsetSet offsets{};
  const auto add_offset = [&, tfc](const Radians& direction, const MathSize ros) {
    if (ros < min_ros_)
    {
      return false;
    }
    const auto ros_cell = ros / cell_size_;
    const auto intensity = fire_intensity(tfc, ros);
    // spreading, so figure out offset from current point
    offsets.emplace_back(
      intensity,
      ros,
      direction.asDegrees(),
      Offset{
        static_cast<DistanceSize>(ros_cell * sin(direction)),
        static_cast<DistanceSize>(ros_cell * cos(direction))
      }
    );
    return true;
  };
  // if not over spread threshold then don't spread
  // HACK: set ros in boolean if we get that far so that we don't have to repeat the if body
  if (!add_offset(head_raz, head_ros * correction_factor(head_raz)))
  {
    return offsets;
  }
  const auto a = (head_ros + back_ros) / 2.0;
  const auto c = a - back_ros;
  const auto flank_ros = a / length_to_breadth;
  const auto a_sq = a * a;
  const auto flank_ros_sq = flank_ros * flank_ros;
  const auto a_sq_sub_c_sq = a_sq - (c * c);
  const auto ac = a * c;
  const auto calculate_ros = [=](const Radians& theta) noexcept {
    const auto cos_t = cos(theta);
    const auto cos_t_sq = cos_t * cos_t;
    const auto f_sq_cos_t_sq = flank_ros_sq * cos_t_sq;
    const auto sin_t_sq = 1.0 - cos_t_sq;
    // // or could be: 1.0 = cos^2 + sin^2
    // const auto sin_t = sin(theta);
    // const auto sin_t_sq = sin_t * sin_t;
    // or could be: 1.0 = cos^2 + sin^2
    return abs(
      (a * ((flank_ros * cos_t * sqrt(f_sq_cos_t_sq + a_sq_sub_c_sq * sin_t_sq) - ac * sin_t_sq) / (f_sq_cos_t_sq + a_sq * sin_t_sq))
       + c)
      / cos_t
    );
  };
  const auto add_offsets = [&, head_raz](const Radians& angle_radians, const MathSize ros_flat) {
    if (ros_flat < min_ros_)
    {
      return false;
    }
    auto direction = (Radians{angle_radians} + head_raz).fix();
    // spread is symmetrical across the center axis, but needs to be adjusted if on a slope
    // intentionally don't use || because we want both of these to happen all the time
    auto added = add_offset(direction, ros_flat * correction_factor(direction));
    direction = (head_raz - Radians{angle_radians}).fix();
    added |= add_offset(direction, ros_flat * correction_factor(direction));
    return added;
  };
  const auto add_offsets_calc_ros = [&](const Radians& angle_radians) {
    return add_offsets(angle_radians, calculate_ros(angle_radians));
  };
  bool added = add_offset(head_raz, head_ros);
  MathSize i = max_angle_;
  while (added && i < 90)
  {
    added = add_offsets_calc_ros(Radians::from_degrees(i));
    i += max_angle_;
  }
  if (added)
  {
    added = add_offsets(Radians::D_090(), flank_ros * sqrt(a_sq_sub_c_sq) / a);
    i = 90 + max_angle_;
    while (added && i < 180)
    {
      added = add_offsets_calc_ros(Radians::from_degrees(i));
      i += max_angle_;
    }
    if (added)
    {
      // only use back ros if every other angle is spreading since this should be lowest
      //  180
      if (back_ros >= min_ros_)
      {
        const auto direction{Radians{head_raz}.to_heading().fix()};
        static_cast<void>(!add_offset(direction, back_ros * correction_factor(direction)));
      }
    }
  }
  return offsets;
}
[[nodiscard]] OffsetSet WidestEllipseAlgorithm::calculate_offsets(
  HorizontalAdjustment correction_factor_xy,
  MathSize tfc,
  const Radians& head_raz,
  MathSize head_ros,
  MathSize back_ros,
  MathSize length_to_breadth
) const noexcept
{
  auto correction_factor = [&](const Radians& theta) {
    const auto [x, y] = correction_factor_xy(theta);
    // CHECK: Pretty sure you can't spread farther horizontally than the spread distance, regardless
    // of angle?
    return min(1.0, sqrt(x * x + y * y));
  };
  OffsetSet offsets{};
  const auto add_offset = [&, tfc](const Radians& direction, const MathSize ros) {
#ifdef DEBUG_POINTS
    const auto s0 = offsets.size();
#endif
    if (ros < min_ros_)
    {
      // #endif
      return false;
    }
    const auto ros_cell = ros / cell_size_;
    const auto intensity = fire_intensity(tfc, ros);
    // spreading, so figure out offset from current point
    offsets.emplace_back(
      intensity,
      ros,
      Direction{direction},
      Offset{
        static_cast<DistanceSize>(ros_cell * sin(direction)),
        static_cast<DistanceSize>(ros_cell * cos(direction))
      }
    );
#ifdef DEBUG_POINTS
    const auto s1 = offsets.size();
    logging::check_equal(s0 + 1, s1, "offsets.size()");
    logging::check_fatal(offsets.empty(), "offsets.empty()");
#endif
    return true;
  };
  // if not over spread threshold then don't spread
  // HACK: set ros in boolean if we get that far so that we don't have to repeat the if body
  if (!add_offset(head_raz, head_ros * correction_factor(head_raz)))
  {
    // #endif
    return offsets;
  }
#ifdef DEBUG_POINTS
  logging::check_fatal(offsets.empty(), "offsets.empty()");
#endif
  const auto a = (head_ros + back_ros) / 2.0;
  const auto c = a - back_ros;
  const auto flank_ros = a / length_to_breadth;
  const auto a_sq = a * a;
  const auto flank_ros_sq = flank_ros * flank_ros;
  const auto a_sq_sub_c_sq = a_sq - (c * c);
  const auto ac = a * c;
  const auto calculate_ros = [=](const Radians& theta) noexcept {
    const auto cos_t = cos(theta);
    const auto cos_t_sq = cos_t * cos_t;
    const auto f_sq_cos_t_sq = flank_ros_sq * cos_t_sq;
    // or could be: 1.0 = cos^2 + sin^2
    const auto sin_t = sin(theta);
    const auto sin_t_sq = sin_t * sin_t;
    return abs(
      (a * ((flank_ros * cos_t * sqrt(f_sq_cos_t_sq + a_sq_sub_c_sq * sin_t_sq) - ac * sin_t_sq) / (f_sq_cos_t_sq + a_sq * sin_t_sq))
       + c)
      / cos_t
    );
  };
  const auto add_offsets = [&, head_raz](const Radians& angle_radians, const MathSize ros_flat) {
    if (ros_flat < min_ros_)
    {
      return false;
    }
    auto direction = (Radians{angle_radians} + head_raz).fix();
    // spread is symmetrical across the center axis, but needs to be adjusted if on a slope
    // intentionally don't use || because we want both of these to happen all the time
    auto added = add_offset(direction, ros_flat * correction_factor(direction));
    direction = (head_raz - Radians{angle_radians}).fix();
    added |= add_offset(direction, ros_flat * correction_factor(direction));
    return added;
  };
  const auto add_offsets_calc_ros = [&](const Radians& angle_radians) {
    return add_offsets(angle_radians, calculate_ros(angle_radians));
  };
  bool added = true;
#define STEP_X 0.2
#define STEP_MAX Radians::from_degrees(max_angle_)
  MathSize step_x = STEP_X / pow(length_to_breadth, 0.5);
  Radians theta{0.0};
  Radians angle{0.0};
  Radians last_theta{0.0};
  MathSize cur_x = 1.0;
  // widest point should be at origin, which is 'c' away from origin
  MathSize widest = atan2(flank_ros, c);
  // size_t num_angles = 0;
  MathSize widest_x = cos(widest);
  Radians step_max{STEP_MAX / pow(length_to_breadth, 0.5)};
  while (added && cur_x > (STEP_MAX / 4.0).value)
  {
    // ++num_angles;
    theta = min(Radians{acos(cur_x)}, last_theta + step_max);
    angle = ellipse_angle(length_to_breadth, Radians{theta});
    added = add_offsets_calc_ros(angle);
    cur_x = cos(theta);
    last_theta = theta;
    if (theta > (STEP_MAX / 2.0))
    {
      step_max = STEP_MAX;
    }
    cur_x -= step_x;
    if (cur_x > widest_x && abs(cur_x - widest_x) < step_x)
    {
      cur_x = widest_x;
    }
  }
  if (added)
  {
    angle = ellipse_angle(length_to_breadth, Radians{(Radians::D_090() + theta) / 2.0});
    added = add_offsets_calc_ros(angle);
    // always just do one between the last angle and 90
    theta = Radians::D_090();
    // ++num_angles;
    angle = ellipse_angle(length_to_breadth, Radians{theta});
    added = add_offsets(Radians::D_090(), flank_ros * sqrt(a_sq_sub_c_sq) / a);
    cur_x = cos(theta);
    last_theta = theta;
  }
  cur_x -= (step_x / 2.0);
  step_x *= length_to_breadth;
  Radians max_angle{Radians::D_180() - (length_to_breadth * step_max)};
  MathSize min_x = cos(max_angle);
  while (added && cur_x >= min_x)
  {
    // ++num_angles;
    theta = max(Radians{acos(cur_x)}, last_theta + step_max);
    angle = ellipse_angle(length_to_breadth, Radians{theta});
    if (angle > max_angle)
    {
      break;
    }
    added = add_offsets_calc_ros(angle);
    cur_x = cos(theta);
    last_theta = theta;
    cur_x -= step_x;
  }
  if (added)
  {
    // only use back ros if every other angle is spreading since this should be lowest
    //  180
    if (back_ros >= min_ros_)
    {
      const auto direction{Radians{head_raz}.to_heading().fix()};
      static_cast<void>(!add_offset(direction, back_ros * correction_factor(direction)));
    }
  }
#ifdef DEBUG_POINTS
  if (head_ros >= min_ros_)
  {
    logging::check_fatal(offsets.empty(), "Empty when ros of {:f} >= {:f}", head_ros, min_ros_);
  }
#endif
  return offsets;
}
// [[nodiscard]] OffsetSet ParametricEllipseAlgorithm::calculate_offsets(
//   HorizontalAdjustment correction_factor,
//   MathSize tfc,
//   const Radians& head_raz,
//   MathSize head_ros,
//   MathSize back_ros,
//   MathSize length_to_breadth
// ) const noexcept
// {
//   // if (head_ros < min_ros_)
//   // {
//   //   return {};
//   // }
//   logging::note("{:>10} {:>10} {:>10} {:>10}", "RAZ", "HROS", "BROS", "L:B");
//   logging::note(
//     "{:10f} {:10f} {:10f} {:10f}", head_raz.asDegrees().value, head_ros, back_ros,
//     length_to_breadth
//   );
//   OffsetSet offsets{};
//   const auto cos_raz = cos(head_raz);
//   const auto sin_raz = sin(head_raz);
//   // calculate centered on (0, 0) first
//   // we know this is equivalent to 2 units along the x-axis in the ellipse
//   const auto dist_total = (head_ros + back_ros);
//   const auto a = dist_total / 2;
//   const auto b = a / length_to_breadth;
//   // origin relative to (0, 0) is back_ros + part of head_ros that gets to (0, 0)
//   const auto origin = (back_ros - a);
//   // this ellipse is scaled so it goes from -1 to 1 in the x direction
//   assert((0 == origin) == (0 == length_to_breadth));
//   const auto c = sqrt(a * a - b * b);
//   const auto e_sq = 1 - ((b / a) * (b / a));
//   logging::check_tolerance(1E-10, c / a, sqrt(e_sq), "eccentricity");
//   auto f = [&](const auto x) { return sqrt((a * a - x * x) * (1 - e_sq)); };
//   auto add_offset = [&](const auto x, const auto y) {
//     logging::note("Checking ({}, {})", x, y);
//     const auto x0 = (x - origin);
//     logging::note("Shifted ({}, {})", x0, y);
//     const auto ros = sqrt(x0 * x0 + y * y);
//     if (ros >= min_ros_)
//     {
//       // // the distance from (0, 0) to (1, 0) needs to be scaled according to the ros
//       // const auto ratio = 1 / a;
//       // convert into distance within a cell
//       const auto ros_cell = 1 / cell_size_;
//       // mirrored across x-axis
//       // FIX: check this
//       const auto theta = Radians{acos(y / x)};
//       // spread is symmetrical across the center axis, but needs to be adjusted if on a slope
//       const auto [x_horizontal_ratio, y_horizontal_ratio] = correction_factor(theta);
//       // FIX: this needs to be used properly
//       const auto intensity = fire_intensity(tfc, ros);
//       // need to rotate around origin
//       const auto x1 = x * cos_raz - y * sin_raz;
//       const auto y1 = x * sin_raz + y * cos_raz;
//       const Offset rotated{Offset{x1, y1}};
//       logging::note("Rotated ({}, {})", x1, y1);
//       const auto x2 = x0 * ros_cell * x_horizontal_ratio;
//       const auto y2 = y * ros_cell * y_horizontal_ratio;
//       logging::note("Scaled ({}, {})", x2, y2);
//       logging::check_fatal(x2 > 1, "x should be in range -1 to 1 at most but got {}", x2);
//       logging::check_fatal(x2 < -1, "x should be in range -1 to 1 at most but got {}", x2);
//       logging::check_fatal(y2 > 1, "x should be in range -1 to 1 at most but got {}", y2);
//       logging::check_fatal(y2 < -1, "x should be in range -1 to 1 at most but got {}", y2);
//       // const Offset rotated{0, 0};
//       // spreading, so figure out offset from current point
//       const auto old = calc_offset(
//         theta,
//         min_ros_,
//         cell_size_,
//         correction_factor,
//         tfc,
//         head_raz,
//         head_ros,
//         back_ros,
//         length_to_breadth
//       );
//       logging::note("old is ({}, {})", old.x(), old.y());
//       logging::note("new is ({}, {})", x2, y2);
//       offsets.emplace_back(
//         intensity,
//         ros,
//         theta,   // is this right?
//         // direction.asDegrees(),
//         Offset{x2, y2}
//       );
//     }
//   };
//   auto add_offsets = [&](const auto x) {
//     const auto y = f(x);
//     add_offset(x, y);
//     if (0 != y)
//     {
//       add_offset(x, -y);
//     }
//   };
//   for (auto r : range(-1, 1, 0.05))
//   {
//     add_offsets(a * r);
//   }
//   add_offsets(a);
//   exit(0);
//   return offsets;
// }
[[nodiscard]] OffsetSet StandardEllipseAlgorithm::calculate_offsets(
  HorizontalAdjustment correction_factor_xy,
  MathSize tfc,
  const Radians& head_raz,
  MathSize head_ros,
  MathSize back_ros,
  MathSize length_to_breadth
) const noexcept
{
  static bool once = false;
  auto correction_factor = [&](const Radians& theta) {
    const auto [x, y] = correction_factor_xy(theta);
    // CHECK: Pretty sure you can't spread farther horizontally than the spread distance, regardless
    // of angle?
    return min(1.0, sqrt(x * x + y * y));
  };
  OffsetSet offsets{};
  const auto add_offset = [&, tfc](const Radians& direction, const MathSize ros) {
#ifdef DEBUG_POINTS
    const auto s0 = offsets.size();
#endif
    if (ros < min_ros_)
    {
      // #endif
      return false;
    }
    const auto ros_cell = ros / cell_size_;
    const auto intensity = fire_intensity(tfc, ros);
    // if (once)
    // {
    //   const auto actual = Offset{
    //     static_cast<DistanceSize>(ros * sin(direction)),
    //     static_cast<DistanceSize>(ros * cos(direction))
    //   };
    //   logging::note("Actual ({}, {})", actual.x(), actual.y());
    // }
    // spreading, so figure out offset from current point
    offsets.emplace_back(
      intensity,
      ros,
      Direction{direction},
      Offset{
        static_cast<DistanceSize>(ros_cell * sin(direction)),
        static_cast<DistanceSize>(ros_cell * cos(direction))
      }
    );
#ifdef DEBUG_POINTS
    const auto s1 = offsets.size();
    logging::check_equal(s0 + 1, s1, "offsets.size()");
    logging::check_fatal(offsets.empty(), "offsets.empty()");
#endif
    return true;
  };
  // if not over spread threshold then don't spread
  // HACK: set ros in boolean if we get that far so that we don't have to repeat the if body
  if (!add_offset(head_raz, head_ros * correction_factor(head_raz)))
  {
    // #endif
    return offsets;
  }
#ifdef DEBUG_POINTS
  logging::check_fatal(offsets.empty(), "offsets.empty()");
#endif
  const auto a = (head_ros + back_ros) / 2.0;
  const auto c = a - back_ros;
  const auto flank_ros = a / length_to_breadth;
  const auto a_sq = a * a;
  const auto flank_ros_sq = flank_ros * flank_ros;
  const auto a_sq_sub_c_sq = a_sq - (c * c);
  const auto ac = a * c;
  const auto calculate_ros = [=](const Radians& theta) noexcept {
    const auto cos_t = cos(theta);
    const auto cos_t_sq = cos_t * cos_t;
    const auto f_sq_cos_t_sq = flank_ros_sq * cos_t_sq;
    // or could be: 1.0 = cos^2 + sin^2
    const auto sin_t = sin(theta);
    const auto sin_t_sq = sin_t * sin_t;
    return abs(
      (a * ((flank_ros * cos_t * sqrt(f_sq_cos_t_sq + a_sq_sub_c_sq * sin_t_sq) - ac * sin_t_sq) / (f_sq_cos_t_sq + a_sq * sin_t_sq))
       + c)
      / cos_t
    );
  };
  const auto add_offsets = [&, head_raz](const Radians& angle_radians, const MathSize ros_flat) {
    if (ros_flat < min_ros_)
    {
      return false;
    }
    auto direction = (Radians{angle_radians} + head_raz).fix();
    // spread is symmetrical across the center axis, but needs to be adjusted if on a slope
    // intentionally don't use || because we want both of these to happen all the time
    auto added = add_offset(direction, ros_flat * correction_factor(direction));
    direction = (head_raz - Radians{angle_radians}).fix();
    added |= add_offset(direction, ros_flat * correction_factor(direction));
    return added;
  };
  const auto add_offsets_calc_ros = [&](const Radians& angle_radians) {
    return add_offsets(angle_radians, calculate_ros(angle_radians));
  };
  bool added = true;
#define STEP_X 0.2
#define STEP_MAX Radians::from_degrees(max_angle_)
  MathSize step_x = STEP_X / pow(length_to_breadth, 0.5);
  Radians theta{0.0};
  Radians angle{0.0};
  Radians last_theta{0.0};
  MathSize cur_x = 1.0;
  // widest point should be at origin, which is 'c' away from origin
  MathSize widest = atan2(flank_ros, c);
  // size_t num_angles = 0;
  MathSize widest_x = cos(widest);
  Radians step_max{STEP_MAX / pow(length_to_breadth, 0.5)};
  while (added && cur_x > (STEP_MAX / 4.0).value)
  {
    // ++num_angles;
    theta = min(Radians{acos(cur_x)}, last_theta + step_max);
    angle = ellipse_angle(length_to_breadth, Radians{theta});
    added = add_offsets_calc_ros(angle);
    cur_x = cos(theta);
    last_theta = theta;
    if (theta > (STEP_MAX / 2.0))
    {
      step_max = STEP_MAX;
    }
    cur_x -= step_x;
    if (cur_x > widest_x && abs(cur_x - widest_x) < step_x)
    {
      cur_x = widest_x;
    }
  }
  if (added)
  {
    angle = ellipse_angle(length_to_breadth, Radians{(Radians::D_090() + theta) / 2.0});
    added = add_offsets_calc_ros(angle);
    // always just do one between the last angle and 90
    theta = Radians::D_090();
    // ++num_angles;
    angle = ellipse_angle(length_to_breadth, Radians{theta});
    added = add_offsets(Radians::D_090(), flank_ros * sqrt(a_sq_sub_c_sq) / a);
    cur_x = cos(theta);
    last_theta = theta;
  }
  cur_x -= (step_x / 2.0);
  step_x *= length_to_breadth;
  Radians max_angle{Radians::D_180() - (length_to_breadth * step_max)};
  MathSize min_x = cos(max_angle);
  while (added && cur_x >= min_x)
  {
    // ++num_angles;
    theta = max(Radians{acos(cur_x)}, last_theta + step_max);
    angle = ellipse_angle(length_to_breadth, Radians{theta});
    if (angle > max_angle)
    {
      break;
    }
    added = add_offsets_calc_ros(angle);
    cur_x = cos(theta);
    last_theta = theta;
    cur_x -= step_x;
  }
  if (added)
  {
    // only use back ros if every other angle is spreading since this should be lowest
    //  180
    if (back_ros >= min_ros_)
    {
      const auto direction{Radians{head_raz}.to_heading().fix()};
      static_cast<void>(!add_offset(direction, back_ros * correction_factor(direction)));
    }
  }
#ifdef DEBUG_POINTS
  if (head_ros >= min_ros_)
  {
    logging::check_fatal(offsets.empty(), "Empty when ros of {:f} >= {:f}", head_ros, min_ros_);
  }
#endif
  once = once || [&]() {
    logging::note("{:>10} {:>10} {:>10} {:>10}", "RAZ", "HROS", "BROS", "L:B");
    logging::note(
      "{:10f} {:10f} {:10f} {:10f}",
      head_raz.asDegrees().value,
      head_ros,
      back_ros,
      length_to_breadth
    );
    {
      // don't rotate or shift ellipse since needs to rotate after shift
      // just add x_o to points and no y shift
      // then rotate around origin for theta
      const auto dist_total = (head_ros + back_ros);
      const auto a = dist_total / 2;
      // l:b * (2 * b) = (2 * a)
      // L * 2 * b = 2 * a
      // L * b = a
      // b = a / L
      // L = a / b
      // b * L = a
      // b = a / L
      // const auto b = length_to_breadth / a;
      const auto b = a / length_to_breadth;
      // const auto b = (dist_total / length_to_breadth) / 2;
      // convert compass (C) to math (M) angle
      //       C    M   diff  90-M
      // N    90    0 = 90      90
      // E     0   90 = -90      0
      // S   270  180 = 90     -90
      // W   180  270 = -90   -180
      // convert compass to math angle
      const auto theta = Radians::D_090() - head_raz;
      // HACK: don't rotate right now
      // const auto theta = Radians::D_360();
      const auto cos_raz = cos(theta);
      const auto sin_raz = sin(theta);
      logging::note(
        "a = {}, b = {}, c = {}, theta = {}, cos(theta) = {}, sin(theta) = {}",
        a,
        b,
        c,
        theta.asDegrees().value,
        cos_raz,
        sin_raz
      );
      const auto a_sq = a * a;
      const auto b_sq = b * b;
      const auto c = sqrt(a_sq - b_sq);
      logging::note("a_sq = {}, b_sq = {}, c = {}", a_sq, b_sq, c);
      const auto e_sq = 1 - (b_sq / a_sq);
      logging::check_equal(e_sq, (1 - (b / a) * (b / a)), "e^2");
      // logging::check_equal(sqrt(e_sq), c / a, "e");
      logging::note("a_sq = {}, b_sq = {}, e_sq = {}", a_sq, b_sq, e_sq);
      const auto x_o = (a - back_ros);
      const auto y_o = 0;
      const auto v0 = -back_ros;
      const auto v1 = head_ros;
      logging::note("(x_o, y_o)) = ({},{})", x_o, y_o);
      logging::note(
        "Vertices are [\n\t({},{}),\n\t({},{}),\n\t({},{}),\n\t({},{})\n]",
        v1,
        y_o,
        -x_o,
        b,
        -x_o,
        -b,
        v0,
        y_o
      );
      // const auto x_sq = x_o * x_o;
      // const auto y_sq = y_o * y_o;
      assert((0 == x_o) == (0 == length_to_breadth));
      logging::check_tolerance(1E-10, c / a, sqrt(e_sq), "eccentricity");
      // logging::note("x_sq = {}, y_sq = {}", x_sq, y_sq);
      // need to rotate around origin
      auto rotate = [](const Offset o, const Radians theta) {
        auto [x, y] = o;
        const auto cos_raz = cos(theta);
        const auto sin_raz = sin(theta);
        return Offset{x * cos_raz - y * sin_raz, x * sin_raz + y * cos_raz};
      };
      // convert compass (C) to math (M) angle
      //       C    M   diff  90-M
      // N    90    0 = 90      90
      // E     0   90 = -90      0
      // S   270  180 = 90     -90
      // W   180  270 = -90   -180
      auto r0 = head_raz;
      auto r1 = -r0;
      auto f = [&](const auto x) { return sqrt((a_sq - (x * x)) * (1 - e_sq)); };
      auto f1 = [&](const auto x) { return b / a * sqrt(a_sq - (x * x)); };
      const auto step = 1E-3;
      for (auto x : range(a, -a, -step))
      {
        const auto x0 = x + x_o;
        const auto y0 = f(x);
        const auto y1 = f1(x);
        const auto theta = Radians::D_090() - head_raz;
        const auto [xr, yr] = rotate(Offset{x0, y0}, -theta);
        logging::note(
          "f({:+0.3f}) = ({:+0.3f} | {:+0.3f}) => @{} ({:+0.3f}, {:+0.3f})",
          x0,
          y0,
          y1,
          theta.asDegrees().value,
          xr,
          yr
        );
      }
      for (auto [i, r, d, o] : offsets)
      {
        auto [x, y] = o;
        const auto xS = (x * cell_size_);
        const auto yS = y * cell_size_;
        const auto sign = y / abs(y);
        auto [xR, yR] = rotate({xS, yS}, r0);
        // undo offset
        const auto xO = xR - x_o;
        const auto yO = yR;
        const auto xf = xO;
        const auto yf = sign * f(xf);
        // add offset
        const auto xo = xf + x_o;
        const auto yo = yf;
        // unrotate
        const auto [xr, yr] = rotate({xo, yo}, r1);
        // unscale
        const auto xs = xr / cell_size_;
        const auto ys = yr / cell_size_;
        logging::note(
          // "orig ({:+0.3f}, {:+0.3f}) => "
          "S ({:+0.3f}, {:+0.3f}) => R@{:03} ({:+0.3f}, {:+0.3f}) => o ({:+0.3f}, {:+0.3f}) => F ({:+0.3f}, {:+0.3f}) => o ({:+0.3f}, {:+0.3f}) => r@{:03} ({:+0.3f}, {:+0.3f}) => s ({:+0.3f}, {:+0.3f}) = {:+0.3f}",
          // x,
          // y,
          xS,
          yS,
          (r0).asDegrees().value,
          xR,
          yR,
          xO,
          yO,
          xf,
          yf,
          xo,
          yo,
          (r1).asDegrees().value,
          xr,
          yr,
          xs,
          ys,
          ys - y
        );
      }
    }
    return true;
  }();
  return offsets;
}
////////////////////////////////////////////////////////////
[[nodiscard]] OffsetSet ParametricEllipseAlgorithm::calculate_offsets(
  HorizontalAdjustment correction_factor_xy,
  MathSize tfc,
  const Radians& head_raz,
  MathSize head_ros,
  MathSize back_ros,
  MathSize length_to_breadth
) const noexcept
{
  static bool once = false;
  auto correction_factor = [&](const Radians& theta) {
    const auto [x, y] = correction_factor_xy(theta);
    // CHECK: Pretty sure you can't spread farther horizontally than the spread distance, regardless
    // of angle?
    return min(1.0, sqrt(x * x + y * y));
  };
  OffsetSet offsets{};
  const auto add_offset = [&, tfc](const Radians& direction, const MathSize x, const MathSize y) {
#ifdef DEBUG_POINTS
    const auto s0 = offsets.size();
#endif
    const auto ros = sqrt(x * x + y * y);
    if (ros < min_ros_)
    {
      // #endif
      return false;
    }
    const auto ros_cell = ros / cell_size_;
    const auto intensity = fire_intensity(tfc, ros);
    offsets.emplace_back(
      intensity,
      ros,
      Direction{direction},
      Offset{static_cast<DistanceSize>(ros_cell * x), static_cast<DistanceSize>(ros_cell * y)}
    );
#ifdef DEBUG_POINTS
    const auto s1 = offsets.size();
    logging::check_equal(s0 + 1, s1, "offsets.size()");
    logging::check_fatal(offsets.empty(), "offsets.empty()");
#endif
    return true;
  };
  constexpr auto N = 24;
  logging::note("{:>10} {:>10} {:>10} {:>10} {:>10}", "HROS", "BROS", "LBR", "RAZ", "N");
  logging::note(
    "{:10f} {:10f} {:10f} {:10f}",
    head_ros,
    back_ros,
    length_to_breadth,
    head_raz.asDegrees().value,
    N
  );
  {
    const auto dist_total = (head_ros + back_ros);
    const auto a = dist_total / 2;
    const auto b = a / length_to_breadth;
    // convert compass (C) to math (M) angle
    //       C    M   diff  90-M
    // N    90    0 = 90      90
    // E     0   90 = -90      0
    // S   270  180 = 90     -90
    // W   180  270 = -90   -180
    // convert compass to math angle
    const auto c = a - back_ros;
    const auto theta = Radians::D_090() - head_raz;
    logging::note(
      " 1:    a = {:0.3f}, b = {:0.3f}, c = {:0.3f}, theta = {:0.1f} deg / {:0.6f} rad",
      a,
      b,
      c,
      theta.asDegrees().value,
      theta.value
    );
    constexpr Degrees degrees{360.0 / N};
    constexpr Radians radians{degrees};
    logging::note(" 2:    degrees = {:0.1f}, radians = {:0.6f}", degrees.value, radians.value);
    const auto R = a;
    logging::note(" 3:    {:>10} {:>10} {:>10} {:>10}", "degrees", "radians", "x", "y");
    vector<std::tuple<Degrees, Radians, MathSize, MathSize>> step3{};
    auto d = 0.0;
    while (d <= 360.0)
    {
      const auto deg = Degrees{d};
      const auto rad = Radians{deg};
      const auto x0 = R * cos(rad);
      const auto y0 = R * sin(rad);
      logging::note(
        // " 3:    t({:0.1f} deg, {:0.6f} rad) => (x0, y0) = ({:0.3f}, {:0.3f})",
        " 3:    {:10.1f} {:10.6f} {:10.3f} {:10.3f}",
        deg.value,
        rad.value,
        x0,
        y0
      );
      step3.emplace_back(deg, rad, x0, y0);
      d += degrees.value;
    }
    vector<std::tuple<Degrees, Radians, MathSize, MathSize>> step4{};
    logging::note(" 4:    {:>10} {:>10} {:>10} {:>10}", "degrees", "radians", "x", "y");
    const auto origin = std::pair{-c, 0.0};
    logging::note(
      // " 3:    t({:0.1f} deg, {:0.6f} rad) => (x0, y0) = ({:0.3f}, {:0.3f})",
      " 4:    {:>10} {:>10} {:10.3f} {:10.3f}",
      "n/a",
      "n/a",
      origin.first,
      origin.second
    );
    const auto Rx = a;
    const auto Ry = b;
    for (auto [deg, rad, x0, y0] : step3)
    {
      const auto x1 = Rx * cos(rad);
      const auto y1 = Ry * sin(rad);
      logging::note(
        // " 3:    t({:0.1f} deg, {:0.6f} rad) => (x0, y0) = ({:0.3f}, {:0.3f})",
        " 4:    {:10.1f} {:10.6f} {:10.3f} {:10.3f}",
        deg.value,
        rad.value,
        x1,
        y1
      );
      step4.emplace_back(deg, rad, x1, y1);
    }
    vector<std::tuple<Degrees, Radians, MathSize, MathSize>> step5{};
    logging::note(" 5:    {:>10} {:>10} {:>10} {:>10}", "degrees", "radians", "x", "y");
    const auto cos_t = cos(theta);
    const auto sin_t = sin(theta);
    const auto origin_rot = std::pair{origin.first * cos_t, origin.first * sin_t};
    logging::note(
      // " 3:    t({:0.1f} deg, {:0.6f} rad) => (x0, y0) = ({:0.3f}, {:0.3f})",
      " 5:    {:>10} {:>10} {:10.3f} {:10.3f}",
      "n/a",
      "n/a",
      origin_rot.first,
      origin_rot.second
    );
    const auto Rx_cos = Rx * cos_t;
    const auto Rx_sin = Rx * sin_t;
    const auto Ry_cos = Ry * cos_t;
    const auto Ry_sin = Ry * sin_t;
    for (auto [deg, rad, x1, y1] : step4)
    {
      const auto r_cos = cos(rad);
      const auto r_sin = sin(rad);
      const auto x2 = Rx_cos * r_cos - Ry_sin * r_sin;
      const auto y2 = Rx_sin * r_cos + Ry_cos * r_sin;
      logging::note(" 5:    {:10.1f} {:10.6f} {:10.3f} {:10.3f}", deg.value, rad.value, x2, y2);
      step5.emplace_back(deg, rad, x2, y2);
    }
    ///////////////////////////////////////
    vector<std::tuple<Degrees, Radians, MathSize, MathSize>> step6{};
    logging::note(" 5:    {:>10} {:>10} {:>10} {:>10}", "degrees", "radians", "x", "y");
    const auto x_o = origin_rot.first;
    const auto y_o = origin_rot.second;
    const auto origin_shift = std::pair{origin_rot.first - x_o, origin_rot.second - y_o};
    logging::note(
      // " 3:    t({:0.1f} deg, {:0.6f} rad) => (x0, y0) = ({:0.3f}, {:0.3f})",
      " 6:    {:>10} {:>10} {:10.3f} {:10.3f}",
      "n/a",
      "n/a",
      origin_shift.first,
      origin_shift.second
    );
    for (auto [deg, rad, x2, y2] : step5)
    {
      const auto x3 = x2 - x_o;
      const auto y3 = y2 - y_o;
      logging::note(" 6:    {:10.1f} {:10.6f} {:10.3f} {:10.3f}", deg.value, rad.value, x3, y3);
      step6.emplace_back(deg, rad, x3, y3);
      add_offset(rad, x3, y3);
    }
  }
  return offsets;
}
}
