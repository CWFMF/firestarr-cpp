/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
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
struct Ffmc : public Index<Ffmc>
{
  static constexpr Ffmc Zero() { return Ffmc{0}; };
  static constexpr Ffmc Invalid() { return Ffmc{-1}; };
  using Index::Index;
  /**
   * \brief Calculate Fine Fuel Moisture Code
   * \param temperature Temperature (Celsius)
   * \param rh Relative Humidity (%)
   * \param ws Wind Speed (km/h)
   * \param prec Precipitation (24hr accumulated, noon-to-noon) (mm)
   * \param ffmc_previous Fine Fuel Moisture Code for previous day
   */
  Ffmc(
    const Temperature temperature,
    const RelativeHumidity rh,
    const Speed ws,
    const Precipitation prec,
    const Ffmc ffmc_previous
  ) noexcept;
};
/**
 * \brief Duff Moisture Code value.
 */
struct Dmc : public Index<Dmc>
{
  static constexpr Dmc Zero() { return Dmc{0}; };
  static constexpr Dmc Invalid() { return Dmc{-1}; };
  using Index::Index;
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
    const Temperature temperature,
    const RelativeHumidity rh,
    const Precipitation prec,
    const Dmc dmc_previous,
    const int month,
    const MathSize latitude
  ) noexcept;
};
/**
 * \brief Drought Code value.
 */
struct Dc : public Index<Dc>
{
  static constexpr Dc Zero() { return Dc{0}; };
  static constexpr Dc Invalid() { return Dc{-1}; };
  using Index::Index;
  /**
   * \brief Calculate Drought Code
   * \param temperature Temperature (Celsius)
   * \param prec Precipitation (24hr accumulated, noon-to-noon) (mm)
   * \param dc_previous Drought Code from the previous day
   * \param month Month to calculate for
   * \param latitude Latitude to calculate for
   */
  Dc(
    const Temperature temperature,
    const Precipitation prec,
    const Dc dc_previous,
    const int month,
    const MathSize latitude
  ) noexcept;
};
/**
 * \brief Initial Spread Index value.
 */
struct Isi : public Index<Isi>
{
  static constexpr Isi Zero() { return Isi{0}; };
  static constexpr Isi Invalid() { return Isi{-1}; };
  using Index::Index;
  /**
   * \brief Calculate Initial Spread Index and verify previous value is within tolerance of
   * calculated value
   * \param value Value to check is within tolerance of calculated value
   * \param ws Wind Speed (km/h)
   * \param ffmc Fine Fuel Moisture Code
   */
  Isi(MathSize value, const Speed ws, const Ffmc ffmc) noexcept;
  /**
   * \brief Calculate Initial Spread Index
   * \param ws Wind Speed (km/h)
   * \param ffmc Fine Fuel Moisture Code
   */
  Isi(const Speed ws, const Ffmc ffmc) noexcept;
};
Isi check_isi(const MathSize value, const Speed& ws, const Ffmc& ffmc) noexcept;
/**
 * \brief Build-up Index value.
 */
struct Bui : public Index<Bui>
{
  static constexpr Bui Zero() { return Bui{0}; };
  static constexpr Bui Invalid() { return Bui{-1}; };
  using Index::Index;
  /**
   * \brief Calculate Build-up Index and verify previous value is within tolerance of calculated
   * value
   * \param value Value to check is within tolerance of calculated value
   * \param dmc Duff Moisture Code
   * \param dc Drought Code
   */
  Bui(MathSize value, const Dmc dmc, const Dc dc) noexcept;
  /**
   * \brief Calculate Build-up Index
   * \param dmc Duff Moisture Code
   * \param dc Drought Code
   */
  Bui(const Dmc dmc, const Dc dc) noexcept;
};
Bui check_bui(const MathSize value, const Dmc& dmc, const Dc& dc) noexcept;
/**
 * \brief Fire Weather Index value.
 */
struct Fwi : public Index<Fwi>
{
  static constexpr Fwi Zero() { return Fwi{0}; };
  static constexpr Fwi Invalid() { return Fwi{-1}; };
  using Index::Index;
  /**
   * \brief Calculate Fire Weather Index and verify previous value is within tolerance of calculated
   * value
   * \param value Value to check is within tolerance of calculated value
   * \param isi Initial Spread Index
   * \param bui Build-up Index
   */
  Fwi(MathSize value, const Isi isi, const Bui bui) noexcept;
  /**
   * \brief Calculate Fire Weather Index
   * \param isi Initial Spread Index
   * \param bui Build-up Index
   */
  Fwi(const Isi isi, const Bui bui) noexcept;
};
Fwi check_fwi(const MathSize value, const Isi& isi, const Bui& bui) noexcept;
/**
 * \brief Danger Severity Rating value.
 */
struct Dsr : public Index<Dsr>
{
  static constexpr Dsr Zero() { return Dsr{0}; };
  static constexpr Dsr Invalid() { return Dsr{-1}; };
  using Index::Index;
  /**
   * \brief Calculate Danger Severity Rating
   * \param fwi Fire Weather Index
   */
  explicit Dsr(const Fwi fwi) noexcept;
};
/**
 * \brief A Weather value with calculated FWI indices.
 */
struct FwiWeather : public Weather
{
  static consteval FwiWeather Zero() { return {}; }
  static consteval FwiWeather Invalid()
  {
    return {
      Weather::Invalid(),
      Ffmc::Invalid(),
      Dmc::Invalid(),
      Dc::Invalid(),
      Isi::Invalid(),
      Bui::Invalid(),
      Fwi::Invalid()
    };
  }
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
  constexpr FwiWeather() noexcept = default;
  constexpr FwiWeather(
    const Temperature temp,
    const RelativeHumidity rh,
    const Wind wind,
    const Precipitation prec,
    const Ffmc ffmc,
    const Dmc dmc,
    const Dc dc,
    const Isi isi,
    const Bui bui,
    const Fwi fwi
  ) noexcept
    : Weather(temp, rh, wind, prec), ffmc{ffmc}, dmc{dmc}, dc{dc}, isi{isi}, bui{bui}, fwi{fwi}
  { }
  constexpr FwiWeather(
    const Weather wx,
    const Ffmc ffmc,
    const Dmc dmc,
    const Dc dc,
    Isi isi = Isi::Invalid(),
    Bui bui = Bui::Invalid(),
    Fwi fwi = Fwi::Invalid()
  ) noexcept
    : Weather(wx), ffmc{ffmc}, dmc{dmc}, dc{dc},
      isi{Isi::Invalid() == isi ? Isi{wind.speed, ffmc} : isi},
      bui{Bui::Invalid() == bui ? Bui{dmc, dc} : bui},
      fwi{Fwi::Invalid() == fwi ? Fwi{isi, bui} : fwi}
  { }
  constexpr FwiWeather(
    const FwiWeather& yesterday,
    const int month,
    const MathSize latitude,
    const Temperature& temp,
    const RelativeHumidity& rh,
    const Wind& wind,
    const Precipitation& prec,
    Ffmc ffmc = Ffmc::Invalid(),
    Dmc dmc = Dmc::Invalid(),
    Dc dc = Dc::Invalid(),
    Isi isi = Isi::Invalid(),
    Bui bui = Bui::Invalid(),
    Fwi fwi = Fwi::Invalid()
  ) noexcept
    : FwiWeather(
        {.temperature = temp, .rh = rh, .wind = wind, .prec = prec},
        (Ffmc::Invalid() == ffmc) ? Ffmc{temp, rh, wind.speed, prec, yesterday.ffmc} : ffmc,
        (Dmc::Invalid() == dmc) ? Dmc{temp, rh, prec, yesterday.dmc, month, latitude} : dmc,
        (Dc::Invalid() == dc) ? Dc{temp, prec, yesterday.dc, month, latitude} : dc,
        isi,
        bui,
        fwi
      )
  { }
  auto operator<=>(const FwiWeather& rhs) const = default;
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
};
constexpr auto FFMC_MOISTURE_CONSTANT = 250.0 * 59.5 / 101.0;
constexpr MathSize ffmc_to_moisture(const MathSize ffmc) noexcept
{
  return FFMC_MOISTURE_CONSTANT * (101.0 - ffmc) / (59.5 + ffmc);
}
constexpr MathSize ffmc_to_moisture(const Ffmc& ffmc) noexcept
{
  return ffmc_to_moisture(ffmc.value);
}
constexpr Ffmc moisture_to_ffmc(const MathSize m) noexcept
{
  return Ffmc{(59.5 * (250.0 - m) / (FFMC_MOISTURE_CONSTANT + m))};
}
constexpr Ffmc ffmc_from_moisture(const MathSize m) noexcept { return Ffmc(moisture_to_ffmc(m)); }
}
#endif
