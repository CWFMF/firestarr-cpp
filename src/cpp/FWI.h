/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FWI_H
#define FS_FWI_H
#include "stdafx.h"
#include "Index.h"
#include "Weather.h"
namespace fs
{
/**
 * \brief Fine Fuel Moisture Code value.
 */
struct Ffmc : public Index<Ffmc>
{
  using Index::Index;
  Ffmc(
    const Temperature& temperature,
    const RelativeHumidity& rh,
    const Speed& ws,
    const Precipitation& prec,
    const Ffmc& ffmc_previous
  ) noexcept;
  auto operator<=>(const Ffmc& rhs) const = default;
};
/**
 * \brief Duff Moisture Code value.
 */
struct Dmc : public Index<Dmc>
{
  using Index::Index;
  Dmc(
    const Temperature& temperature,
    const RelativeHumidity& rh,
    const Precipitation& prec,
    const Dmc& dmc_previous,
    const int month,
    const MathSize latitude
  ) noexcept;
  auto operator<=>(const Dmc& rhs) const = default;
};
/**
 * \brief Drought Code value.
 */
struct Dc : public Index<Dc>
{
  using Index::Index;
  Dc(
    const Temperature& temperature,
    const Precipitation& prec,
    const Dc& dc_previous,
    const int month,
    const MathSize latitude
  ) noexcept;
  auto operator<=>(const Dc& rhs) const = default;
};
/**
 * \brief Initial Spread Index value.
 */
struct Isi : public Index<Isi>
{
  using Index::Index;
  Isi(const Speed& ws, const Ffmc& ffmc) noexcept;
  auto operator<=>(const Isi& rhs) const = default;
};
Isi check_isi(const MathSize value, const Speed& ws, const Ffmc& ffmc) noexcept;
/**
 * \brief Build-up Index value.
 */
struct Bui : public Index<Bui>
{
  using Index::Index;
  Bui(const Dmc& dmc, const Dc& dc) noexcept;
  auto operator<=>(const Bui& rhs) const = default;
};
Bui check_bui(const MathSize value, const Dmc& dmc, const Dc& dc) noexcept;
/**
 * \brief Fire Weather Index value.
 */
struct Fwi : public Index<Fwi>
{
  using Index::Index;
  Fwi(const Isi& isi, const Bui& bui) noexcept;
  auto operator<=>(const Fwi& rhs) const = default;
};
Fwi check_fwi(const MathSize value, const Isi& isi, const Bui& bui) noexcept;
/**
 * \brief Danger Severity Rating value.
 */
struct Dsr : public Index<Dsr>
{
  using Index::Index;
  Dsr(const Fwi& fwi) noexcept;
  auto operator<=>(const Dsr& rhs) const = default;
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
