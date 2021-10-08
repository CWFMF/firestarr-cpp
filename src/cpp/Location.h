/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_LOCATION_H
#define FS_LOCATION_H

#include "Util.h"
namespace fs
{
namespace topo
{
/**
 * \brief A location with a row and column.
 */
class Location
{
public:
  Location() = default;
  /**
   * \brief Construct using hash of row and column
   * \param hash HashSize derived form row and column
   */
  explicit constexpr Location(
    const HashSize hash
  ) noexcept
    : topo_data_(hash & HashMask)
  {
  }
  /**
   * \brief Constructor
   * \param row Row
   * \param column Column
   */
  constexpr Location(
    const Idx row,
    const Idx column
  ) noexcept
    : Location(doHash(row, column))
  {
  }
  /**
   * \brief Row
   * \return Row
   */
  [[nodiscard]] constexpr Idx
  row() const noexcept
  {
    return static_cast<Idx>(hash() / MAX_COLUMNS);
  }
  /**
   * \brief Column
   * \return Column
   */
  [[nodiscard]] constexpr Idx
  column() const noexcept
  {
    return static_cast<Idx>(hash() % MAX_COLUMNS);
  }
  /**
   * \brief Hash derived from row and column
   * \return Hash derived from row and column
   */
  [[nodiscard]] constexpr HashSize
  hash() const noexcept
  {
    // can get away with just casting because all the other bits are outside this area
    return static_cast<HashSize>(topo_data_);
  }
  /**
   * \brief Equality operator
   * \param rhs Location to compare to
   * \return Whether or not these are equivalent
   */
  [[nodiscard]] constexpr bool
  operator==(
    const Location& rhs
  ) const noexcept
  {
    return hash() == rhs.hash();
  }
  /**
   * \brief Inequality operator
   * \param rhs Location to compare to
   * \return Whether or not these are not equivalent
   */
  [[nodiscard]] constexpr bool
  operator!=(
    const Location& rhs
  ) const noexcept
  {
    return !(*this == rhs);
  }
  /**
   * \brief Full stored hash that may contain data from subclasses
   * \return Full stored hash that may contain data from subclasses
   */
  [[nodiscard]] constexpr Topo
  fullHash() const
  {
    return topo_data_;
  }
protected:
  /**
   * \brief Stored hash that contains row and column data
   */
  Topo topo_data_;
  /**
   * \brief Number of bits to use for storing location data
   */
  static constexpr uint32_t LocationBits = 32;
  /**
   * \brief Hash mask for bits being used for location data
   */
  static constexpr Topo HashMask = util::bit_mask<LocationBits, Topo>();
  static_assert(HashMask >= MAX_COLUMNS * MAX_ROWS - 1);
  static_assert(HashMask <= std::numeric_limits<HashSize>::max());
  /**
   * \brief Construct with given hash that may contain data from subclasses
   * \param topo Hash to store
   */
  explicit constexpr Location(
    const Topo& topo
  ) noexcept
    : topo_data_(topo)
  {
  }
  /**
   * \brief Create a hash from given values
   * \param row Row
   * \param column Column
   * \return Hash
   */
  [[nodiscard]] static constexpr HashSize
  doHash(
    const Idx row,
    const Idx column
  ) noexcept
  {
    return static_cast<HashSize>(row) * static_cast<HashSize>(MAX_COLUMNS)
         + static_cast<HashSize>(column);
  }
};
}
}
#endif
