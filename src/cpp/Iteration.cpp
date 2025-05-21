/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2024-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "Iteration.h"

#include "Cell.h"
#include "ProbabilityMap.h"
#include "Scenario.h"

namespace fs
{
Iteration::~Iteration()
{
  for (auto& s : scenarios_)
  {
    delete s;
  }
}

Iteration::Iteration(
  vector<Scenario*> scenarios
) noexcept
  : scenarios_(std::move(scenarios))
{
}

Iteration*
Iteration::reset(
  mt19937* mt_extinction,
  mt19937* mt_spread
)
{
  cancelled_ = false;
  final_sizes_ = {};
  for (auto& scenario : scenarios_)
  {
    static_cast<void>(scenario->reset(mt_extinction, mt_spread, &final_sizes_));
  }
  return this;
}

vector<DurationSize>
Iteration::savePoints() const
{
  return scenarios_.at(0)->savePoints();
}

DurationSize
Iteration::startTime() const
{
  return scenarios_.at(0)->startTime();
}

size_t
Iteration::size() const noexcept
{
  return scenarios_.size();
}

SafeVector
Iteration::finalSizes() const
{
  return final_sizes_;
}

void
Iteration::cancel(
  bool show_warning
) noexcept
{
  cancelled_ = true;
  for (auto& s : scenarios_)
  {
    s->cancel(show_warning);
  }
}
}
