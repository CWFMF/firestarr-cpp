/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "BurnedData.h"
#include "FuelLookup.h"
namespace fs
{
BurnedData::BurnedData(const CellGrid& cells) noexcept
  : data_(from_grid(cells, [](const auto& v) { return fuel_by_code(v.fuelCode()); }))
{ }
BurnedData::BurnedData(const FuelGrid& fuel) noexcept
  : data_{from_grid(fuel, [](const auto& v) { return v; })}
{ }
bool BurnedData::at(const HashSize h) const noexcept { return (*data_)[h]; }
void BurnedData::set(const HashSize h) noexcept { data_->set(h); }
void BurnedData::clear() noexcept
{
  if (nullptr == data_)
  {
    data_ = std::make_unique<dtype>(false);
  }
  else
  {
    data_->reset();
  }
}
BurnedData::BurnedData(const BurnedData& rhs) noexcept
  : data_{nullptr == rhs.data_ ? nullptr : make_unique<dtype>(*rhs.data_)}
{ }
BurnedData::BurnedData(BurnedData&& rhs) noexcept
  : data_(nullptr == rhs.data_ ? nullptr : std::move(rhs.data_))
{
  rhs.data_ = nullptr;
}
BurnedData& BurnedData::operator=(const BurnedData& rhs) noexcept
{
  if (nullptr == rhs.data_)
  {
    data_ = nullptr;
    return *this;
  }
  if (nullptr == data_)
  {
    data_ = std::make_unique<dtype>();
  }
  *data_ = *rhs.data_;
  return *this;
}
BurnedData& BurnedData::operator=(BurnedData&& rhs) noexcept
{
  if (nullptr == rhs.data_)
  {
    data_ = nullptr;
    return *this;
  }
  data_ = std::move(rhs.data_);
  rhs.data_ = nullptr;
  return *this;
}
uptr<BurnedData::dtype> BurnedData::from_grid(auto& grid, auto fct)
{
  // FIX: change to use grid iterator
  auto result = make_unique<dtype>(false);
  // make a template we can copy to reset things
  for (Idx r = 0; r < grid.rows(); ++r)
  {
    for (Idx c = 0; c < grid.columns(); ++c)
    {
      const Location location(r, c);
      (*result)[location.hash()] = (nullptr == fct(grid.at(location)));
    }
  }
  return result;
}
}
