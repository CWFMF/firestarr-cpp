/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FWI_H
#define FS_FWI_H
#include "unstable.h"
#include "Weather.h"
namespace fs
{
/**
 * \brief Fine Fuel Moisture Code value.
 */
struct Ffmc
{
  MathSize value{0};
  explicit constexpr Ffmc(const MathSize value_ = 0) : value{value_} { }
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
  auto operator<=>(const Ffmc& rhs) const = default;
};
/**
 * \brief Duff Moisture Code value.
 */
struct Dmc
{
  MathSize value{0};
  explicit constexpr Dmc(const MathSize value_ = 0) : value{value_} { }
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
  auto operator<=>(const Dmc& rhs) const = default;
};
/**
 * \brief Drought Code value.
 */
struct Dc
{
  MathSize value{0};
  explicit constexpr Dc(const MathSize value_ = 0) : value{value_} { }
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
  auto operator<=>(const Dc& rhs) const = default;
};
/**
 * \brief Initial Spread Index value.
 */
struct Isi
{
  MathSize value{0};
  explicit constexpr Isi(const MathSize value_ = 0) : value{value_} { }
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
  auto operator<=>(const Isi& rhs) const = default;
};
Isi check_isi(const MathSize value, const Speed& ws, const Ffmc& ffmc) noexcept;
/**
 * \brief Build-up Index value.
 */
struct Bui
{
  MathSize value{0};
  explicit constexpr Bui(const MathSize value_ = 0) : value{value_} { }
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
  auto operator<=>(const Bui& rhs) const = default;
};
Bui check_bui(const MathSize value, const Dmc& dmc, const Dc& dc) noexcept;
/**
 * \brief Fire Weather Index value.
 */
struct Fwi
{
  MathSize value{0};
  explicit constexpr Fwi(const MathSize value_ = 0) : value{value_} { }
  /**
   * \brief Calculate Fire Weather Index and verify previous value is within tolerance of
   * calculated value
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
  auto operator<=>(const Fwi& rhs) const = default;
};
Fwi check_fwi(const MathSize value, const Isi& isi, const Bui& bui) noexcept;
/**
 * \brief Danger Severity Rating value.
 */
struct Dsr
{
  MathSize value{0};
  explicit constexpr Dsr(const MathSize value_ = 0) : value{value_} { }
  /**
   * \brief Calculate Danger Severity Rating
   * \param fwi Fire Weather Index
   */
  explicit Dsr(const Fwi fwi) noexcept;
  auto operator<=>(const Dsr& rhs) const = default;
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
namespace ffmc
{
static constexpr Ffmc zero{0.0};
static constexpr Ffmc invalid{-1.0};
}
namespace dmc
{
static constexpr Dmc zero{0};
static constexpr Dmc invalid{-1};
}
namespace dc
{
static constexpr Dc zero{0};
static constexpr Dc invalid{-1};
}
namespace isi
{
static constexpr Isi zero{0};
static constexpr Isi invalid{-1};
}
namespace bui
{
static constexpr Bui zero{0};
static constexpr Bui invalid{-1};
}
namespace fwi
{
static constexpr Fwi zero{0};
static constexpr Fwi invalid{-1};
}
namespace dsr
{
static constexpr Dsr zero{0};
static constexpr Dsr invalid{-1};
}
/**
 * \brief A Weather value with calculated FWI indices.
 */
struct FwiWeather : public Weather
{
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
    const Weather wx,
    const Ffmc ffmc,
    const Dmc dmc,
    const Dc dc,
    Isi isi = isi::invalid,
    Bui bui = bui::invalid,
    Fwi fwi = fwi::invalid
  ) noexcept
    : Weather(wx), ffmc{ffmc}, dmc{dmc}, dc{dc},
      isi{isi::invalid == isi ? Isi{wind.speed, ffmc} : isi},
      bui{bui::invalid == bui ? Bui{dmc, dc} : bui}, fwi{fwi::invalid == fwi ? Fwi{isi, bui} : fwi}
  { }
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
    : FwiWeather{Weather{temp, rh, wind, prec}, ffmc, dmc, dc, isi, bui, fwi}
  { }
  constexpr FwiWeather(
    const FwiWeather& yesterday,
    const int month,
    const MathSize latitude,
    const Temperature& temp,
    const RelativeHumidity& rh,
    const Wind& wind,
    const Precipitation& prec,
    Ffmc ffmc = ffmc::invalid,
    Dmc dmc = dmc::invalid,
    Dc dc = dc::invalid,
    Isi isi = isi::invalid,
    Bui bui = bui::invalid,
    Fwi fwi = fwi::invalid
  ) noexcept
    : FwiWeather(
        {.temperature = temp, .rh = rh, .wind = wind, .prec = prec},
        (ffmc::invalid == ffmc) ? Ffmc{temp, rh, wind.speed, prec, yesterday.ffmc} : ffmc,
        (dmc::invalid == dmc) ? Dmc{temp, rh, prec, yesterday.dmc, month, latitude} : dmc,
        (dc::invalid == dc) ? Dc{temp, prec, yesterday.dc, month, latitude} : dc,
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
}
#endif
