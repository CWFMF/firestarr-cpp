/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_SIMPLEFUELLOOKUP_H
#define FS_SIMPLEFUELLOOKUP_H
#include "stdafx.h"
#include "SimpleFuelType.h"
namespace fs::simplefbp
{
class SimpleFuelLookupImpl;
string simplify_name(const string_view fuel);
/**
 * \brief Provides ability to look up a fuel type based on name or code.
 */
class SimpleFuelLookup
{
public:
  ~SimpleFuelLookup() = default;
  /**
   * \brief Construct by reading from a file
   * \param filename File to read from. Uses .lut format from Prometheus
   */
  explicit SimpleFuelLookup(const char* filename);
  SimpleFuelLookup(const SimpleFuelLookup& rhs) noexcept = default;
  SimpleFuelLookup(SimpleFuelLookup&& rhs) noexcept = default;
  SimpleFuelLookup& operator=(const SimpleFuelLookup& rhs) noexcept = default;
  SimpleFuelLookup& operator=(SimpleFuelLookup&& rhs) noexcept = default;
  /**
   * \brief Look up a SimpleFuelType based on the given code
   * \param value Value to use for lookup
   * \param nodata Value that represents no data
   * \return SimpleFuelType based on the given code
   */
  [[nodiscard]] const SimpleFuelType* codeToFuel(FuelSize value, FuelSize nodata) const;
  /**
   * \brief List all fuels and their codes
   */
  void listFuels() const;
  /**
   * \brief Look up the original code for the given SimpleFuelType
   * \param value Value to use for lookup
   * \return code for the given SimpleFuelType
   */
  [[nodiscard]] FuelSize fuelToCode(const SimpleFuelType* fuel) const;
  /**
   * \brief Look up a SimpleFuelType ba1ed on the given code
   * \param value Value to use for lookup
   * \param nodata Value that represents no data
   * \return SimpleFuelType based on the given code
   */
  [[nodiscard]] const SimpleFuelType* operator()(FuelSize value, FuelSize nodata) const;
  /**
   * \brief Retrieve set of FuelTypes that are used in the lookup table
   * \return set of FuelTypes that are used in the lookup table
   */
  [[nodiscard]] set<const SimpleFuelType*> usedFuels() const;
  /**
   * \brief Look up a SimpleFuelType based on the given name
   * \param name Name of the fuel to find
   * \return SimpleFuelType based on the given name
   */
  [[nodiscard]] const SimpleFuelType* byName(const string_view name) const;
  /**
   * \brief Look up a SimpleFuelType based on the given simplified name
   * \param name Simplified name of the fuel to find
   * \return SimpleFuelType based on the given name
   */
  [[nodiscard]] const SimpleFuelType* bySimplifiedName(const string_view name) const;
  /**
   * \brief Array of all FuelTypes available to be used in simulations
   */
  static const array<const SimpleFuelType*, NUMBER_OF_FUELS> Fuels;

private:
  /**
   * \brief Implementation class for SimpleFuelLookup
   */
  shared_ptr<SimpleFuelLookupImpl> impl_{nullptr};
};
/**
 * \brief Look up a SimpleFuelType based on the given code
 * \param code Value to use for lookup
 * \return SimpleFuelType based on the given code
 */
[[nodiscard]] constexpr const SimpleFuelType* fuel_by_code(const FuelCodeSize& code)
{
  return SimpleFuelLookup::Fuels.at(code);
}
/**
 * \brief Get SimpleFuelType based on the given cell
 * \param cell Cell to retrieve SimpleFuelType for
 * \return SimpleFuelType based on the given cell
 */
[[nodiscard]] constexpr const SimpleFuelType* check_fuel(const Cell& cell)
{
  return fuel_by_code(cell.fuelCode());
}
/**
 * \brief Whether or not there is no fuel in the Cell
 * \param cell Cell to check
 * \return Whether or not there is no fuel in the Cell
 */
[[nodiscard]] constexpr bool is_null_fuel(const SimpleFuelType* fuel)
{
  return INVALID_FUEL_CODE == SimpleFuelType::safeCode(fuel);
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
namespace fs::testing
{
int test_fbp(const int argc, const char* const argv[]);
}
#endif
