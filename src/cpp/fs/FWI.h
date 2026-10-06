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
struct Ffmc : public StrictType<Ffmc>
{
  using StrictType::StrictType;
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
struct Dmc : public StrictType<Dmc>
{
  using StrictType::StrictType;
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
struct Dc : public StrictType<Dc>
{
  using StrictType::StrictType;
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
struct Isi : public StrictType<Isi>
{
  using StrictType::StrictType;
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
struct Bui : public StrictType<Bui>
{
  using StrictType::StrictType;
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
struct Fwi : public StrictType<Fwi>
{
  using StrictType::StrictType;
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
struct Dsr : public StrictType<Dsr>
{
  using StrictType::StrictType;
  /**
   * \brief Calculate Danger Severity Rating
   * \param fwi Fire Weather Index
   */
  explicit Dsr(const Fwi fwi) noexcept;
};
MathSize ffmc_effect(const Ffmc ffmc) noexcept;
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
