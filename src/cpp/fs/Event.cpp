/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Event.h"
namespace fs
{
std::partial_ordering Event::operator<=>(const Event& rhs) const
{
  if (const auto cmp = time <=> rhs.time; 0 != cmp)
  {
    return cmp;
  }
  if (const auto cmp = type <=> rhs.type; 0 != cmp)
  {
    return cmp;
  }
  // if (const auto cmp = time_at_location <=> rhs.time_at_location; 0 != cmp)
  // {
  //   return cmp;
  // }
  // if (const auto cmp = cell <=> rhs.cell; 0 != cmp)
  // {
  //   return cmp;
  // }
  if (const auto cmp = cell.hash() <=> rhs.cell.hash(); 0 != cmp)
  {
    return cmp;
  }
  // if (const auto cmp = ros <=> rhs.ros; 0 != cmp)
  // {
  //   return cmp;
  // }
  // if (const auto cmp = intensity <=> rhs.intensity; 0 != cmp)
  // {
  //   return cmp;
  // }
  // if (const auto cmp = raz.asValue() <=> rhs.raz.asValue(); 0 != cmp)
  // {
  //   return cmp;
  // }
  // if (const auto cmp = source <=> rhs.source; 0 != cmp)
  // {
  //   return cmp;
  // }
  return std::partial_ordering::equivalent;
};
}
