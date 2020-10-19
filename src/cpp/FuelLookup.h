/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FUELLOOKUP_H
#define FS_FUELLOOKUP_H
#include "stdafx.h"
#include "Cell.h"
#include "FireWeather.h"
#include "FuelType.h"
namespace fs
{
class FuelLookupImpl;
string simplify_fuel_name(const string_view fuel);
/**
 * \brief Provides ability to look up a fuel type based on name or code.
 */
class FuelLookup
{
public:
  ~FuelLookup() = default;
  /**
   * \brief Construct by reading from a file
   * \param filename File to read from. Uses .lut format from Prometheus
   */
  explicit FuelLookup(const char* filename);
  FuelLookup(const FuelLookup& rhs) noexcept = default;
  FuelLookup(FuelLookup&& rhs) noexcept = default;
  FuelLookup& operator=(const FuelLookup& rhs) noexcept = default;
  FuelLookup& operator=(FuelLookup&& rhs) noexcept = default;
  /**
   * \brief Look up a FuelType based on the given code
   * \param value Value to use for lookup
   * \param nodata Value that represents no data
   * \return FuelType based on the given code
   */
  [[nodiscard]] const FuelType* codeToFuel(FuelSize value, FuelSize nodata) const;
  /**
   * \brief List all fuels and their codes
   */
  void listFuels() const;
  /**
   * \brief Look up the original code for the given FuelType
   * \param value Value to use for lookup
   * \return code for the given FuelType
   */
  [[nodiscard]] FuelSize fuelToCode(const FuelType* fuel) const;
  /**
   * \brief Look up a FuelType ba1ed on the given code
   * \param value Value to use for lookup
   * \param nodata Value that represents no data
   * \return FuelType based on the given code
   */
  [[nodiscard]] const FuelType* operator()(FuelSize value, FuelSize nodata) const;
  /**
   * \brief Retrieve set of FuelTypes that are used in the lookup table
   * \return set of FuelTypes that are used in the lookup table
   */
  [[nodiscard]] set<const FuelType*> usedFuels() const;
  /**
   * \brief Look up a FuelType based on the given name
   * \param name Name of the fuel to find
   * \return FuelType based on the given name
   */
  [[nodiscard]] const FuelType* byName(const string_view name) const;
  /**
   * \brief Look up a FuelType based on the given simplified name
   * \param name Simplified name of the fuel to find
   * \return FuelType based on the given name
   */
  [[nodiscard]] const FuelType* bySimplifiedName(const string_view name) const;
  /**
   * \brief Array of all FuelTypes available to be used in simulations
   */
  static const array<const FuelType*, NUMBER_OF_FUELS> Fuels;

private:
  /**
   * \brief Implementation class for FuelLookup
   */
  shared_ptr<FuelLookupImpl> impl_{nullptr};
};
/**
 * \brief Look up a FuelType based on the given code
 * \param code Value to use for lookup
 * \return FuelType based on the given code
 */
[[nodiscard]] constexpr const FuelType* fuel_by_code(const FuelCodeSize& code)
{
  return FuelLookup::Fuels.at(code);
}
/**
 * \brief Get FuelType based on the given cell
 * \param cell Cell to retrieve FuelType for
 * \return FuelType based on the given cell
 */
[[nodiscard]] constexpr const FuelType* check_fuel(const Cell& cell)
{
  return fuel_by_code(cell.fuelCode());
}
/**
 * \brief Whether or not there is no fuel in the Cell
 * \param cell Cell to check
 * \return Whether or not there is no fuel in the Cell
 */
[[nodiscard]] constexpr bool is_null_fuel(const FuelType* fuel)
{
  return INVALID_FUEL_CODE == FuelType::safeCode(fuel);
}
/**
 * \brief Whether or not there is no fuel in the Cell
 * \param cell Cell to check
 * \return Whether or not there is no fuel in the Cell
 */
[[nodiscard]] constexpr bool is_null_fuel(const Cell& cell)
{
  return is_null_fuel(fuel_by_code(cell.fuelCode()));
}
}
#endif
