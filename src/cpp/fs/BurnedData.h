/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_BURNEDDATA_H
#define FS_BURNEDDATA_H
#include "stdafx.h"
#include "ConstantGrid.h"
namespace fs
{
using namespace fuel;
class BurnedData
{
public:
  BurnedData(const CellGrid& cells) noexcept;
  BurnedData(const FuelGrid& fuel) noexcept;
  bool at(const HashSize h) const noexcept;
  void set(const HashSize h) noexcept;
  void clear() noexcept;
  BurnedData() noexcept = default;
  BurnedData(const BurnedData& rhs) noexcept;
  BurnedData(BurnedData&& rhs) noexcept;
  BurnedData& operator=(const BurnedData& rhs) noexcept;
  BurnedData& operator=(BurnedData&& rhs) noexcept;

private:
  // std::bitset doesn't heap allocate
  using dtype = std::bitset<static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS>;
  uptr<dtype> data_{nullptr};
  static uptr<dtype> from_grid(auto& grid, auto fct);
};
}
#endif
