/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_RANGEITERATOR_H
#define FS_RANGEITERATOR_H
#include <iterator>
#include "debug_settings.h"
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
#ifndef DEBUG_ITERATOR
    logging::verbose(
      "Range is from %f to %f with step %f %s",
      start_,
      end_,
      increment_,
      inclusive_ ? "inclusive" : "exclusive"
    );
  }
#endif
  RangeIterator() = default;
  RangeIterator(const RangeIterator& rhs) = default;
  RangeIterator(RangeIterator&& rhs) = default;
  RangeIterator& operator=(const RangeIterator& rhs) = default;
  RangeIterator& operator=(RangeIterator&& rhs) = default;

public:
  inline value_type operator*() const { return start_ + step_ * increment_; }
  inline RangeIterator& operator++()
  {
#ifndef DEBUG_ITERATOR
    logging::check_fatal(
      *(*this) < start_,
      "operator++() %g less than start value %g (+%g) for step %d",
      *(*this),
      start_,
      (*(*this) - start_),
      step_
    );
#endif
    step_++;
#ifndef DEBUG_ITERATOR
    logging::check_fatal(
      *this > end(),
      "operator++() %g more than end value %g (%+g) for step %d",
      *(*this),
      *end(),
      (*(*this) - *end()),
      step_
    );
#endif
    return *this;
  }
  inline RangeIterator operator++(int)
  {
#ifndef DEBUG_ITERATOR
    logging::check_fatal(
      *(*this) < start_,
      "operator++(int) %g less than start value %g (+%g) for step %d",
      *(*this),
      start_,
      step_
    );
#endif
    RangeIterator oTmp = *(*this);
    *(*this) += increment_;
#ifndef DEBUG_ITERATOR
    logging::check_fatal(
      *this > end(),
      "operator++(int) %g more than end value %g (%+g) for step %d",
      *(*this),
      *end(),
      step_
    );
#endif
    return oTmp;
  }
  inline RangeIterator& operator--()
  {
#ifndef DEBUG_ITERATOR
    logging::check_fatal(
      *this > end(),
      "operator--() %g more than end value %g (%+g) for step %d",
      *(*this),
      *end(),
      step_
    );
#endif
    *(*this) -= increment_;
#ifndef DEBUG_ITERATOR
    logging::check_fatal(
      *(*this) < start_,
      "operator--() %g less than start value %g (+%g) for step %d",
      *(*this),
      start_,
      step_
    );
#endif
    return *this;
  }
  inline RangeIterator operator--(int)
  {
    RangeIterator oTmp = *(*this);
#ifndef DEBUG_ITERATOR
    logging::check_fatal(
      *this > end(),
      "operator--(int) %g more than end value %g (%+g) for step %d",
      *(*this),
      *end(),
      step_
    );
#endif
    *(*this) -= increment_;
#ifndef DEBUG_ITERATOR
    logging::check_fatal(
      *(*this) < start_,
      "operator--(int) %g less than start value %g (+%g) for step %d",
      *(*this),
      start_,
      step_
    );
#endif
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
  auto begin() const { return RangeIterator<value_type>(this, start_); }
  auto end() const
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
}
auto range_int(const int start, const int end, const int step, const bool inclusive = true)
{
  std::ignore = inclusive;
  return RangeIterator<int>(start, end, step);
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
