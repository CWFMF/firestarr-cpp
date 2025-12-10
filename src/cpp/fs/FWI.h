/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FWI_H
#define FS_FWI_H
#include "stdafx.h"
#include "Weather.h"
namespace fs
{
/**
 * \brief Fine Fuel Moisture Code value.
 */
class Ffmc : public Index<Ffmc>
{
public:
  //! @cond Doxygen_Suppress
  using Index::Index;
  //! @endcond
  /**
   * \brief Calculate Fine Fuel Moisture Code
   * \param temperature Temperature (Celsius)
   * \param rh Relative Humidity (%)
   * \param ws Wind Speed (km/h)
   * \param prec Precipitation (24hr accumulated, noon-to-noon) (mm)
   * \param ffmc_previous Fine Fuel Moisture Code for previous day
   */
  Ffmc(
    const Temperature& temperature,
    const RelativeHumidity& rh,
    const Speed& ws,
    const Precipitation& prec,
    const Ffmc& ffmc_previous
  ) noexcept;
  /**
   * \brief Fine Fuel Moisture Code of 0
   */
  static constexpr Ffmc Zero() { return Ffmc{0}; };
  static constexpr Ffmc Invalid() { return Ffmc{-1}; };
};
/**
 * \brief Duff Moisture Code value.
 */
class Dmc : public Index<Dmc>
{
public:
  //! @cond Doxygen_Suppress
  using Index::Index;
  //! @endcond
  /**
   * \brief Duff Moisture Code
   * \param temperature Temperature (Celsius)
   * \param rh Relative Humidity (%)
   * \param prec Precipitation (24hr accumulated, noon-to-noon) (mm)
   * \param dmc_previous Duff Moisture Code for previous day
   * \param month Month to calculate for
   * \param latitude Latitude to calculate for
   */
  Dmc(
    const Temperature& temperature,
    const RelativeHumidity& rh,
    const Precipitation& prec,
    const Dmc& dmc_previous,
    const int month,
    const MathSize latitude
  ) noexcept;
  /**
   * \brief Duff Moisture Code of 0
   */
  static constexpr Dmc Zero() { return Dmc{0}; };
  static constexpr Dmc Invalid() { return Dmc{-1}; };
};
/**
 * \brief Drought Code value.
 */
class Dc : public Index<Dc>
{
public:
  //! @cond Doxygen_Suppress
  using Index::Index;
  //! @endcond
  /**
   * \brief Calculate Drought Code
   * \param temperature Temperature (Celsius)
   * \param prec Precipitation (24hr accumulated, noon-to-noon) (mm)
   * \param dc_previous Drought Code from the previous day
   * \param month Month to calculate for
   * \param latitude Latitude to calculate for
   */
  Dc(
    const Temperature& temperature,
    const Precipitation& prec,
    const Dc& dc_previous,
    const int month,
    const MathSize latitude
  ) noexcept;
  /**
   * \brief Drought Code of 0
   */
  static constexpr Dc Zero() { return Dc{0}; };
  static constexpr Dc Invalid() { return Dc{-1}; };
};
/**
 * \brief Initial Spread Index value.
 */
class Isi : public Index<Isi>
{
public:
  /**
   * \brief Calculate Initial Spread Index and verify previous value is within tolerance of
   * calculated value
   * \param value Value to check is within tolerance of calculated value
   * \param ws Wind Speed (km/h)
   * \param ffmc Fine Fuel Moisture Code
   */
  Isi(MathSize value, const Speed& ws, const Ffmc& ffmc) noexcept;
  /**
   * \brief Calculate Initial Spread Index
   * \param ws Wind Speed (km/h)
   * \param ffmc Fine Fuel Moisture Code
   */
  Isi(const Speed& ws, const Ffmc& ffmc) noexcept;
  /**
   * \brief Initial Spread Index of 0
   */
  static constexpr Isi Zero() { return Isi{0}; };
  static constexpr Isi Invalid() { return Isi{-1}; };

private:
  //! @cond Doxygen_Suppress
  using Index::Index;
  //! @endcond
};
/**
 * \brief Build-up Index value.
 */
class Bui : public Index<Bui>
{
public:
  /**
   * \brief Calculate Build-up Index and verify previous value is within tolerance of calculated
   * value
   * \param value Value to check is within tolerance of calculated value
   * \param dmc Duff Moisture Code
   * \param dc Drought Code
   */
  Bui(MathSize value, const Dmc& dmc, const Dc& dc) noexcept;
  /**
   * \brief Calculate Build-up Index
   * \param dmc Duff Moisture Code
   * \param dc Drought Code
   */
  Bui(const Dmc& dmc, const Dc& dc) noexcept;
  /**
   * \brief Build-up Index of 0
   */
  static constexpr Bui Zero() { return Bui{0}; };
  static constexpr Bui Invalid() { return Bui{-1}; };

private:
  //! @cond Doxygen_Suppress
  using Index::Index;
  //! @endcond
};
/**
 * \brief Fire Weather Index value.
 */
class Fwi : public Index<Fwi>
{
public:
  /**
   * \brief Calculate Fire Weather Index and verify previous value is within tolerance of calculated
   * value
   * \param value Value to check is within tolerance of calculated value
   * \param isi Initial Spread Index
   * \param bui Build-up Index
   */
  Fwi(MathSize value, const Isi& isi, const Bui& bui) noexcept;
  /**
   * \brief Calculate Fire Weather Index
   * \param isi Initial Spread Index
   * \param bui Build-up Index
   */
  Fwi(const Isi& isi, const Bui& bui) noexcept;
  /**
   * \brief Fire Weather Index of 0
   */
  static constexpr Fwi Zero() { return Fwi{0}; };
  static constexpr Fwi Invalid() { return Fwi{-1}; };

private:
  //! @cond Doxygen_Suppress
  using Index::Index;
  //! @endcond
};
/**
 * \brief Danger Severity Rating value.
 */
class Dsr : public Index<Dsr>
{
public:
  //! @cond Doxygen_Suppress
  using Index::Index;
  //! @endcond
  /**
   * \brief Calculate Danger Severity Rating
   * \param fwi Fire Weather Index
   */
  explicit Dsr(const Fwi& fwi) noexcept;
  /**
   * \brief Danger Severity Rating of 0
   */
  static constexpr Dsr Zero() { return Dsr{0}; };
  static constexpr Dsr Invalid() { return Dsr{-1}; };
};
/**
 * \brief A Weather value with calculated FWI indices.
 */
class FwiWeather : public Weather
{
public:
  /**
   * \brief FwiWeather with 0 for all Indices
   */
  static constexpr FwiWeather Zero()
  {
    return FwiWeather{
      Temperature::Zero(),
      RelativeHumidity::Zero(),
      Wind::Zero(),
      Precipitation::Zero(),
      Ffmc::Zero(),
      Dmc::Zero(),
      Dc::Zero(),
      Isi::Zero(),
      Bui::Zero(),
      Fwi::Zero()
    };
  };
  static constexpr FwiWeather Invalid()
  {
    return FwiWeather{
      Temperature::Invalid(),
      RelativeHumidity::Invalid(),
      Wind::Invalid(),
      Precipitation::Invalid(),
      Ffmc::Invalid(),
      Dmc::Invalid(),
      Dc::Invalid(),
      Isi::Invalid(),
      Bui::Invalid(),
      Fwi::Invalid()
    };
  };
  /**
   * \brief Construct with 0 for all values
   */
  FwiWeather() noexcept;
  /**
   * \brief construct by applying noon weather to yesterday's indices
   * \param yesterday FwiWeather yesterday used for startup indices
   * \param month Month to calculate for
   * \param latitude Latitude to calculate for
   * \param temp Temperature (Celsius)
   * \param rh Relative Humidity (%)
   * \param wind Wind (km/h)
   * \param prec Precipitation (24hr accumulated, noon-to-noon) (mm)
   */
  FwiWeather(
    const FwiWeather& yesterday,
    const int month,
    const double latitude,
    const Temperature& temp,
    const RelativeHumidity& rh,
    const Wind& wind,
    const Precipitation& prec
  );
  /**
   * \brief Constructor
   * \param temp Temperature (Celsius)
   * \param rh Relative Humidity (%)
   * \param wind Wind (km/h)
   * \param prec Precipitation (1hr accumulation) (mm)
   * \param ffmc Fine Fuel Moisture Code
   * \param dmc Duff Moisture Code
   * \param dc Drought Code
   * \param isi Initial Spread Index
   * \param bui Build-up Index
   * \param fwi Fire Weather Index
   */
  FwiWeather(
    const Temperature& temp,
    const RelativeHumidity& rh,
    const Wind& wind,
    const Precipitation& prec,
    const Ffmc& ffmc,
    const Dmc& dmc,
    const Dc& dc,
    const Isi& isi,
    const Bui& bui,
    const Fwi& fwi
  ) noexcept;
  /**
   * \brief Construct by calculating FWI
   * \param temp Temperature (Celsius)
   * \param rh Relative Humidity (%)
   * \param wind Wind (km/h)
   * \param prec Precipitation (1hr accumulation) (mm)
   * \param ffmc Fine Fuel Moisture Code
   * \param dmc Duff Moisture Code
   * \param dc Drought Code
   * \param isi Initial Spread Index
   * \param bui Build-up Index
   */
  FwiWeather(
    const Temperature& temp,
    const RelativeHumidity& rh,
    const Wind& wind,
    const Precipitation& prec,
    const Ffmc& ffmc,
    const Dmc& dmc,
    const Dc& dc,
    const Isi& isi,
    const Bui& bui
  ) noexcept;
  /**
   * \brief Construct by calculating ISI, BUI, & FWI
   * \param temp Temperature (Celsius)
   * \param rh Relative Humidity (%)
   * \param wind Wind (km/h)
   * \param prec Precipitation (1hr accumulation) (mm)
   * \param ffmc Fine Fuel Moisture Code
   * \param dmc Duff Moisture Code
   * \param dc Drought Code
   */
  FwiWeather(
    const Temperature& temp,
    const RelativeHumidity& rh,
    const Wind& wind,
    const Precipitation& prec,
    const Ffmc& ffmc,
    const Dmc& dmc,
    const Dc& dc
  ) noexcept;
  /**
   * \brief Construct by recalculating with different wind Speed and Ffmc
   * \param wx Original weather values
   * \param ws Wind Speed to use
   * \param ffmc Fine Fuel Moisture Code to use
   */
  FwiWeather(const FwiWeather& wx, const Speed& ws, const Ffmc& ffmc) noexcept;
  ~FwiWeather() override = default;
  constexpr FwiWeather(FwiWeather&& rhs) noexcept = default;
  constexpr FwiWeather(const FwiWeather& rhs) noexcept = default;
  FwiWeather& operator=(FwiWeather&& rhs) noexcept = default;
  FwiWeather& operator=(const FwiWeather& rhs) = default;
  /**
   * \brief Moisture content (%) based on Ffmc
   * \return Moisture content (%) based on Ffmc
   */
  [[nodiscard]] MathSize mcFfmcPct() const;
  /**
   * \brief Moisture content (%) based on Dmc
   * \return Moisture content (%) based on Dmc
   */
  [[nodiscard]] MathSize mcDmcPct() const;
  /**
   * \brief Moisture content (ratio) based on Ffmc
   * \return Moisture content (ratio) based on Ffmc
   */
  [[nodiscard]] MathSize mcFfmc() const;
  /**
   * \brief Moisture content (ratio) based on Dmc
   * \return Moisture content (ratio) based on Dmc
   */
  [[nodiscard]] MathSize mcDmc() const;
  /**
   * \brief Ffmc effect used for spread
   * \return Ffmc effect used for spread
   */
  [[nodiscard]] MathSize ffmcEffect() const;

private:
  /**
   * \brief Calculate based on indices plus new Wind, Ffmc, and Isi
   * \param wx FwiWeather to use most indices from
   * \param wind Wind to override with
   * \param ffmc Ffmc to override with
   * \param isi Isi calculated from given Wind and Ffmc to override with
   */
  FwiWeather(const FwiWeather& wx, const Wind& wind, const Ffmc& ffmc, const Isi& isi) noexcept;
  /**
   * \brief Calculate based on indices plus new Wind and Ffmc
   * \param wx FwiWeather to use most indices from
   * \param wind Wind to override with
   * \param ffmc Ffmc to override with
   */
  FwiWeather(const FwiWeather& wx, const Wind& wind, const Ffmc& ffmc) noexcept;

public:
  /**
   * \brief Fine Fuel Moisture Code
   */
  Ffmc ffmc{};
  /**
   * \brief Duff Moisture Code
   */
  Dmc dmc{};
  /**
   * \brief Drought Code
   */
  Dc dc{};
  /**
   * \brief Initial Spread Index
   */
  Isi isi{};
  /**
   * \brief Build-up Index
   */
  Bui bui{};
  /**
   * \brief Fire Weather Index
   */
  Fwi fwi{};
};
[[nodiscard]] constexpr bool operator<(const FwiWeather& lhs, const FwiWeather& rhs)
{
  if (lhs.temperature == rhs.temperature)
  {
    if (lhs.rh == rhs.rh)
    {
      if (lhs.wind == rhs.wind)
      {
        if (lhs.prec == rhs.prec)
        {
          if (lhs.ffmc == rhs.ffmc)
          {
            if (lhs.dmc == rhs.dmc)
            {
              if (lhs.dc == rhs.dc)
              {
                // HACK: these should be the same, but if we loaded from file may not be exact
                // assert(lhs.isi == rhs.isi);
                // assert(lhs.bui == rhs.bui);
                // assert(lhs.fwi == rhs.fwi);
              }
              return lhs.dc < rhs.dc;
            }
            return lhs.dmc < rhs.dmc;
          }
          return lhs.ffmc < rhs.ffmc;
        }
        return lhs.prec < rhs.prec;
      }
      return lhs.wind < rhs.wind;
    }
    return lhs.rh < rhs.rh;
  }
  return lhs.temperature < rhs.temperature;
}
[[nodiscard]] constexpr bool operator!=(const FwiWeather& lhs, const FwiWeather& rhs)
{
  return lhs.temperature != rhs.temperature || lhs.rh != rhs.rh || lhs.wind != rhs.wind
      || lhs.prec != rhs.prec || lhs.ffmc != rhs.ffmc || lhs.dmc != rhs.dmc || lhs.dc != rhs.dc
    // || lhs.isi != rhs.isi
    // || lhs.bui != rhs.bui
    // || lhs.fwi != rhs.fwi
    ;
}
[[nodiscard]] constexpr bool operator==(const FwiWeather& lhs, const FwiWeather& rhs)
{
  return !(lhs != rhs);
}
constexpr auto FFMC_MOISTURE_CONSTANT = 250.0 * 59.5 / 101.0;
constexpr MathSize ffmc_to_moisture(const MathSize ffmc) noexcept
{
  return FFMC_MOISTURE_CONSTANT * (101.0 - ffmc) / (59.5 + ffmc);
}
constexpr MathSize ffmc_to_moisture(const Ffmc& ffmc) noexcept
{
  return ffmc_to_moisture(ffmc.value);
}
constexpr MathSize moisture_to_ffmc(const MathSize m) noexcept
{
  return (59.5 * (250.0 - m) / (FFMC_MOISTURE_CONSTANT + m));
}
constexpr Ffmc ffmc_from_moisture(const MathSize m) noexcept { return Ffmc(moisture_to_ffmc(m)); }
}
#endif
