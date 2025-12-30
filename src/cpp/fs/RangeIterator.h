/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_RANGEITERATOR_H
#define FS_RANGEITERATOR_H
#include <iterator>
#include "Log.h"
#include "unstable.h"
namespace fs
{
template <class T = MathSize>
class RangeIterator
{
  friend class iterable;

public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = int;
  using value_type = T;
  RangeIterator(
    const value_type start,
    const value_type end,
    const value_type increment,
    const bool inclusive = true
  )
    : start_(start), end_(end), increment_(increment), inclusive_(inclusive)
  {
    logging::verbose(
      "Range is from %f to %f with step %f %s",
      start_,
      end_,
      increment_,
      inclusive_ ? "inclusive" : "exclusive"
    );
  }
  RangeIterator() = default;
  RangeIterator(const RangeIterator& rhs) = default;
  RangeIterator(RangeIterator&& rhs) = default;
  RangeIterator& operator=(const RangeIterator& rhs) = default;
  RangeIterator& operator=(RangeIterator&& rhs) = default;

public:
  inline value_type operator*() const { return start_ + step_ * increment_; }
  inline RangeIterator& operator++()
  {
    logging::check_fatal(
      *(*this) < start_,
      "operator++() %g less than start value %g (+%g) for step %d",
      *(*this),
      start_,
      (*(*this) - start_),
      step_
    );
    step_++;
    logging::check_fatal(
      *this > end(),
      "operator++() %g more than end value %g (%+g) for step %d",
      *(*this),
      *end(),
      (*(*this) - *end()),
      step_
    );
    return *this;
  }
  inline RangeIterator operator++(int)
  {
    logging::check_fatal(
      *(*this) < start_,
      "operator++(int) %g less than start value %g (+%g) for step %d",
      *(*this),
      start_,
      step_
    );
    RangeIterator oTmp = *(*this);
    *(*this) += increment_;
    logging::check_fatal(
      *this > end(),
      "operator++(int) %g more than end value %g (%+g) for step %d",
      *(*this),
      *end(),
      step_
    );
    return oTmp;
  }
  inline RangeIterator& operator--()
  {
    logging::check_fatal(
      *this > end(),
      "operator--() %g more than end value %g (%+g) for step %d",
      *(*this),
      *end(),
      step_
    );
    *(*this) -= increment_;
    logging::check_fatal(
      *(*this) < start_,
      "operator--() %g less than start value %g (+%g) for step %d",
      *(*this),
      start_,
      step_
    );
    return *this;
  }
  inline RangeIterator operator--(int)
  {
    RangeIterator oTmp = *(*this);
    logging::check_fatal(
      *this > end(),
      "operator--(int) %g more than end value %g (%+g) for step %d",
      *(*this),
      *end(),
      step_
    );
    *(*this) -= increment_;
    logging::check_fatal(
      *(*this) < start_,
      "operator--(int) %g less than start value %g (+%g) for step %d",
      *(*this),
      start_,
      step_
    );
    return oTmp;
  }
  inline difference_type operator-(const RangeIterator& rhs) const
  {
    return static_cast<difference_type>(step_ - rhs.step_);
  }
  inline auto operator<=>(const RangeIterator<value_type>& rhs) const = default;
  // inline bool operator==(const RangeIterator& rhs) const { return *(*this) == rhs; }
  // inline bool operator!=(const RangeIterator& rhs) const { return !(*(*this) == rhs); }
  // inline bool operator>(const RangeIterator& rhs) const { return *(*this) < *rhs; }
  // inline bool operator<(const RangeIterator& rhs) const { return *(*this) > *rhs; }
  // inline bool operator>=(const RangeIterator& rhs) const { return *(*this) <= *rhs; }
  // inline bool operator<=(const RangeIterator& rhs) const { return *(*this) >= *rhs; }
  auto begin() { return RangeIterator<value_type>(this, start_); }
  auto end()
  {
    // if inclusive then end is slightly past end value so end is included
    return RangeIterator<value_type>(
      this, end_ + (inclusive_ ? increment_ : static_cast<value_type>(0))
    );
  }

private:
  RangeIterator(const RangeIterator* rhs, const T value)
    : start_(rhs->start_), end_(rhs->end_), increment_(rhs->increment_),
      step_(static_cast<difference_type>((value - start_) / increment_)),
      inclusive_(rhs->inclusive_)
  { }
  value_type start_{};
  value_type end_{};
  value_type increment_{};
  size_t step_{0};
  bool inclusive_{};
};
static_assert(
  std::bidirectional_iterator<RangeIterator<MathSize>>,
  "iterator must be an iterator!"
);
static_assert(std::bidirectional_iterator<RangeIterator<int>>, "iterator must be an iterator!");
auto range(
  const MathSize start,
  const MathSize end,
  const MathSize step,
  const bool inclusive = true
)
{
  std::ignore = inclusive;
  return RangeIterator(start, end, step);
  // // convert into integer range and then back
  // const auto steps = static_cast<int>(floor((end - start) / step));
  // // const auto max_value = start + steps * step;
  // logging::info("Range is from %f to %f with step %f (%d steps)", start, end, step, steps);
  // // +1 to include end
  // auto it = std::views::iota(0, steps + (inclusive ? 1 : 0));
  // return std::views::transform(it, [=](const int v) {
  //   // static int cur_step = 0;
  //   // const auto r = start + v / static_cast<MathSize>(steps);
  //   const auto r = start + v * step;
  //   // printf("%f\n", r);
  //   // FIX: logging doesn't work within this?
  //   logging::check_fatal(r < start, "%f less than start value %f for step %d", r, start, v);
  //   logging::check_fatal(r > end, "%f more than end value %f for step %d", r, end, v);
  //   // logging::check_fatal(cur_step > steps, "%ld more than steps value %ld for step %d",
  //   cur_step,
  //   // steps, v);
  //   // ++cur_step;
  //   const auto diff = r - start;
  //   const auto epsilon = step * 1E-5;
  //   if (1 == v)
  //   {
  //     logging::check_fatal(
  //       abs(diff - step) > epsilon, "%f different than step increment %f for step %d", diff,
  //       step, v
  //     );
  //   }
  //   else if (0 < v)
  //   {
  //     logging::check_fatal(
  //       step > diff, "%f smaller than step increment %f for step %d", diff, step, v
  //     );
  //     const int v0 = static_cast<int>((r - start) / step);
  //     logging::check_equal(
  //       v, v0, std::format("current step for {} with start {} and step {}", r, start,
  //       step).c_str()
  //     );
  //   }
  //   return r;
  // });
}
auto range_int(const int start, const int end, const int step, const bool inclusive = true)
{
  std::ignore = inclusive;
  return RangeIterator<int>(start, end, step);
  // // convert into integer range and then back
  // const auto steps = static_cast<int>(floor((end - start) / step));
  // // const auto max_value = start + steps * step;
  // logging::debug("Range is from %d to %d with step %d (%d steps)", start, end, step, steps);
  // // +1 to include end
  // auto it = std::views::iota(0, steps + (inclusive ? 1 : 0));
  // return std::views::transform(it, [=](const int v) {
  //   // static int cur_step = 0;
  //   // const auto r = start + v / static_cast<MathSize>(steps);
  //   const int r = start + v * step;
  //   // printf("%f\n", r);
  //   logging::check_fatal(r < start, "%g less than start value %g (+%g) for step %d", r, start,
  //   v); logging::check_fatal(r > end, "%g more than end value %g (%+g) for step %d", r, end, v);
  //   // logging::check_fatal(cur_step > steps, "%ld more than steps value %ld for step %d",
  //   cur_step,
  //   // steps, v);
  //   // ++cur_step;
  //   const auto epsilon = step * 1E-5;
  //   const auto diff = r - start;
  //   if (1 == v)
  //   {
  //     logging::check_fatal(
  //       abs(diff - step) > epsilon, "%d different than step increment %d for step %d", diff,
  //       step, v
  //     );
  //   }
  //   else if (0 < v)
  //   {
  //     logging::check_fatal(
  //       step > diff, "%d smaller than step increment %d for step %d", diff, step, v
  //     );
  //     const int v0 = static_cast<int>((r - start) / step);
  //     logging::check_equal(
  //       v, v0, std::format("current step for {} with start {} and step {}", r, start,
  //       step).c_str()
  //     );
  //   }
  //   return r;
  // });
}
}
namespace fs::testing
{
auto check_range(
  const char* name_fct,
  const char* name_param,
  const auto fct_a,
  const auto fct_b,
  const MathSize epsilon,
  const MathSize start,
  const MathSize end,
  const MathSize step,
  const bool inclusive = true
)
{
  logging::debug("Checking %s", name_fct);
  for (auto v : range(start, end, step, inclusive))
  {
    const auto msg = std::format("{} ({} = {})", name_fct, name_param, v);
    logging::check_tolerance(epsilon, fct_a(v), fct_b(v), msg.c_str());
    logging::verbose(msg.c_str());
  }
}
}
#endif
