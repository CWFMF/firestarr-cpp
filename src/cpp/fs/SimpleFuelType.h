/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_SIMPLE_FUELTYPE_H
#define FS_SIMPLE_FUELTYPE_H
#include "stdafx.h"
#include "Duff.h"
#include "FireSpread.h"
#include "FuelType.h"
#include "FWI.h"
#include "Survival.h"
#include "unstable.h"
namespace fs::simplefbp
{
using fs::duff::Duff;
using fs::fuel::probability_peat;
using fs::fuel::survival_probability;
using SimpleFuelType = fs::FuelType;
/**
 * \brief Base class for all FuelTypes.
 * \tparam BulkDensity Duff Bulk Density (kg/m^3) [Anderson table 1] * 1000
 * \tparam InorganicPercent Inorganic percent of Duff layer (%) [Anderson table 1]
 * \tparam DuffDepth Depth of Duff layer (cm * 10) [Anderson table 1]
 */
class SimpleFuelBase : public SimpleFuelType
{
private:
  MathSize bulk_density_{};
  MathSize inorganic_percent_{};
  MathSize duff_depth_{};

public:
  ~SimpleFuelBase() override = default;
  /**
   * \brief Constructor
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   * \param can_crown Whether or not this fuel type can have a crown fire
   * \param duff_ffmc Type of duff near the surface
   * \param duff_dmc Type of duff deeper underground
   */
  constexpr SimpleFuelBase(
    const FuelCodeSize& code,
    const char* name,
    const bool can_crown,
    const MathSize bulk_density,
    const MathSize inorganic_percent,
    const MathSize duff_depth,
    const Duff* duff_ffmc,
    const Duff* duff_dmc
  )
    : FuelType(code, name, can_crown), bulk_density_(bulk_density),
      inorganic_percent_(inorganic_percent), duff_depth_(duff_depth), duff_ffmc_(duff_ffmc),
      duff_dmc_(duff_dmc)
  { }
  SimpleFuelBase(SimpleFuelBase&& rhs) noexcept = delete;
  SimpleFuelBase(const SimpleFuelBase& rhs) = delete;
  SimpleFuelBase& operator=(SimpleFuelBase&& rhs) noexcept = delete;
  SimpleFuelBase& operator=(const SimpleFuelBase& rhs) = delete;
  /**
   * \brief Is fuel a valid fuel type
   */
  [[nodiscard]] bool isValid() const override { return true; }
  /**
   * \brief Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   * \param rss Surface Rate of spread (ROS) (m/min) [ST-X-3 eq 55]
   * \param rso Critical surface fire spread rate (RSO) [ST-X-3 eq 57]
   * \return Crown Fraction Burned (CFB) [ST-X-3 eq 58]
   */
  [[nodiscard]] MathSize crownFractionBurned(const MathSize rss, const MathSize rso)
    const noexcept override
  {
    // can't burn crown if it doesn't exist
    return cfl() > 0 ? max(0.0, 1.0 - exp(-0.230 * (rss - rso))) : 0.0;
  }
  /**
   * \brief Calculate probability of burning [Anderson eq 1]
   * \param mc_fraction moisture content (% / 100)
   * \return Calculate probability of burning [Anderson eq 1]
   */
  [[nodiscard]] ThresholdSize probabilityPeat(const MathSize mc_fraction) const noexcept override
  {
    return probability_peat(bulkDensity(), inorganicPercent(), mc_fraction);
  }
  /**
   * \brief Survival probability calculated using probability of ony survival based on multiple
   * formulae
   * \param wx FwiWeather to calculate survival probability for
   * \return Chance of survival (% / 100)
   */
  [[nodiscard]] ThresholdSize survivalProbability(const FwiWeather& wx) const noexcept override
  {
    return survival_probability(
      bulkDensity(), inorganicPercent(), *duffFfmcType(), *duffDmcType(), dmcRatio(), wx
    );
  }
  /**
   * \brief Duff Bulk Density (kg/m^3) [Anderson table 1]
   * \return Duff Bulk Density (kg/m^3) [Anderson table 1]
   */
  [[nodiscard]] constexpr MathSize bulkDensity() const
  {
    return bulk_density_;
    // BulkDensity / 1000.0;
  }
  /**
   * \brief Inorganic Percent (% / 100) [Anderson table 1]
   * \return Inorganic Percent (% / 100) [Anderson table 1]
   */
  [[nodiscard]] constexpr MathSize inorganicPercent() const
  {
    return inorganic_percent_;
    // InorganicPercent / 100.0;
  }
  /**
   * \brief DuffDepth Depth of Duff layer (cm) [Anderson table 1]
   * \return DuffDepth Depth of Duff layer (cm) [Anderson table 1]
   */
  [[nodiscard]] constexpr MathSize duffDepth() const
  {
    return duff_depth_;
    // DuffDepth / 10.0;
  }
  /**
   * \brief Type of duff deeper underground
   * \return Type of duff deeper underground
   */
  [[nodiscard]] constexpr const Duff* duffDmcType() const { return duff_dmc_; }
  /**
   * \brief Type of duff near the surface
   * \return Type of duff near the surface
   */
  [[nodiscard]] constexpr const Duff* duffFfmcType() const { return duff_ffmc_; }
  /**
   * \brief What fraction of the duff layer should use FFMC to determine moisture
   * \return What fraction of the duff layer should use FFMC to determine moisture
   */
  [[nodiscard]] constexpr MathSize ffmcRatio() const { return 1 - dmcRatio(); }
  /**
   * \brief What fraction of the duff layer should use DMC to determine moisture
   * \return What fraction of the duff layer should use DMC to determine moisture
   */
  [[nodiscard]] constexpr MathSize dmcRatio() const
  {
    return (duffDepth() - fs::fuel::DUFF_FFMC_DEPTH) / duffDepth();
  }
  [[nodiscard]] const FuelType* summer() const noexcept override { return this; }
  [[nodiscard]] const FuelType* spring() const noexcept override { return this; }

private:
  /**
   * \brief Type of duff near the surface
   */
  const Duff* duff_ffmc_{nullptr};
  /**
   * \brief Type of duff deeper underground
   */
  const Duff* duff_dmc_{nullptr};
};
}
#endif
