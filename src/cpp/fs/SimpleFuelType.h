/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_SIMPLE_FUELTYPE_H
#define FS_SIMPLE_FUELTYPE_H
#include "stdafx.h"
#include "FuelType.h"
#include "unstable.h"
namespace fs::simplefbp
{
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
    // Anderson table 1
    const auto pb = bulkDensity();
    // Anderson table 1
    const auto fi = inorganicPercent();
    const auto pi = fi * pb;
    // Inorganic ratio
    const auto ri = fi / (1 - fi);
    const auto const_part = -19.329 + 1.7170 * ri + 23.059 * pi;
    // Anderson eq 1
    return 1 / (1 + exp(17.047 * mc_fraction / (1 - fi) + const_part));
  }
  /**
   * \brief Survival probability calculated using probability of ony survival based on multiple
   * formulae
   * \param wx FwiWeather to calculate survival probability for
   * \return Chance of survival (% / 100)
   */
  [[nodiscard]] ThresholdSize survivalProbability(const FwiWeather& wx) const noexcept override
  {
    // divide by 100 since we need moisture ratio
    //    IFERROR(((1 / (1 + EXP($G$43 + $I$43 *
    //            (Q$44 * $O$43 + $N$43)))) -
    //            (1 / (1 + EXP($G$43 + $I$43 * (2.5 * $O$43 + $N$43)))))
    //            / (1 / (1 + EXP($G$43 + $I$43 * $N$43))), 0)
    // HACK: use same constants for all fuels because they seem to work nicer than
    // using the ratios, but they change anyway because of the other fuel attributes
    static const auto WFfmc = 0.25;
    static const auto WDmc = 1.0;
    static const auto RatioHartford = 0.5;
    static const auto RatioFrandsen = 1.0 - RatioHartford;
    static const auto RatioAspen = 0.5;
    static const auto RatioFuel = 1.0 - RatioAspen;
    const auto mc_ffmc = wx.mcFfmc() * WFfmc + WDmc;
    static const auto McFfmcSaturated = 2.5 * WFfmc + WDmc;
    static const auto McDmc = WDmc;
    const auto prob_ffmc_peat = probabilityPeat(mc_ffmc);
    const auto prob_ffmc_peat_saturated = probabilityPeat(McFfmcSaturated);
    const auto prob_ffmc_peat_zero = probabilityPeat(McDmc);
    const auto prob_ffmc_peat_weighted =
      (prob_ffmc_peat - prob_ffmc_peat_saturated) / prob_ffmc_peat_zero;
    const auto prob_ffmc = duffFfmcType()->probabilityOfSurvival(mc_ffmc * 100);
    const auto prob_ffmc_saturated = duffFfmcType()->probabilityOfSurvival(McFfmcSaturated * 100);
    const auto prob_ffmc_zero = duffFfmcType()->probabilityOfSurvival(McDmc);
    const auto prob_ffmc_weighted = (prob_ffmc - prob_ffmc_saturated) / prob_ffmc_zero;
    const auto term_otway = exp(-3.11 + 0.12 * wx.dmc.value);
    const auto prob_otway = term_otway / (1 + term_otway);
    const auto mc_pct = wx.mcDmcPct() * dmcRatio() + wx.mcFfmcPct() * ffmcRatio();
    const auto prob_weight_ffmc = duffFfmcType()->probabilityOfSurvival(mc_pct);
    const auto prob_weight_ffmc_peat = probabilityPeat(mc_pct / 100);
    const auto prob_weight_dmc = duffDmcType()->probabilityOfSurvival(wx.mcDmcPct());
    const auto prob_weight_dmc_peat = probabilityPeat(wx.mcDmc());
    // chance of survival is 1 - chance of it not surviving in every fuel
    const auto tot_prob =
      1
      - (1 - prob_ffmc_peat_weighted) * (1 - prob_ffmc_weighted)
          * ((1 - prob_otway) * RatioAspen + ((1 - prob_weight_ffmc_peat) * RatioHartford + (1 - prob_weight_ffmc) * RatioFrandsen) * ((1 - prob_weight_dmc_peat) * RatioHartford + (1 - prob_weight_dmc) * RatioFrandsen) * RatioFuel);
    return tot_prob;
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
    return (duffDepth() - DUFF_FFMC_DEPTH) / duffDepth();
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
/**
 * \brief Placeholder fuel that throws exceptions if it ever gets used.
 */
class InvalidFuel final : public FuelType
{
public:
  InvalidFuel() noexcept : InvalidFuel(0, nullptr) { }
  /**
   * \brief Placeholder fuel that throws exceptions if it ever gets used.
   * \param code Code to identify fuel with
   * \param name Name of the fuel
   */
  constexpr InvalidFuel(const FuelCodeSize& code, const char* name) noexcept
    : FuelType(code, name, false)
  { }
  ~InvalidFuel() override = default;
  InvalidFuel(const InvalidFuel& rhs) noexcept = delete;
  InvalidFuel(InvalidFuel&& rhs) noexcept = delete;
  InvalidFuel& operator=(const InvalidFuel& rhs) noexcept = delete;
  InvalidFuel& operator=(InvalidFuel&& rhs) noexcept = delete;
  /**
   * \brief Is fuel a valid fuel type
   */
  [[nodiscard]] bool isValid() const override { return false; }
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize grass_curing(const int nd, const FwiWeather& wx) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize cbh() const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize cfl() const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize buiEffect(MathSize) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize crownConsumption(MathSize) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize calculateRos(int, const FwiWeather&, MathSize) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize calculateIsf(const SpreadInfo&, MathSize) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize surfaceFuelConsumption(const SpreadInfo&) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize lengthToBreadth(MathSize) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize finalRos(const SpreadInfo&, MathSize, MathSize, MathSize) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize criticalSurfaceIntensity(const SpreadInfo&) const override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] MathSize crownFractionBurned(MathSize, MathSize) const noexcept override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] ThresholdSize probabilityPeat(MathSize) const noexcept override;
  /**
   * \brief Throw a runtime_error
   * \return Throw a runtime_error
   */
  [[nodiscard]] ThresholdSize survivalProbability(const FwiWeather&) const noexcept override;
  [[nodiscard]] const FuelType* summer() const noexcept override { return this; }
  [[nodiscard]] const FuelType* spring() const noexcept override { return this; }
};
}
#endif
