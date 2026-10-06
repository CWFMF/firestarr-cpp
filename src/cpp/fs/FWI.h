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
/**
 * \brief A Weather value with calculated FWI indices.
 */
struct FwiWeatherImpl : public Weather
{
  static consteval FwiWeatherImpl Zero() { return {}; }
  static consteval FwiWeatherImpl Invalid()
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
  constexpr FwiWeatherImpl() noexcept = default;
  constexpr FwiWeatherImpl(
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
      fwi{Fwi::Invalid() == fwi ? Fwi{this->isi, this->bui} : fwi}
  { }
  constexpr FwiWeatherImpl(
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
    : FwiWeatherImpl{Weather{temp, rh, wind, prec}, ffmc, dmc, dc, isi, bui, fwi}
  { }
  constexpr FwiWeatherImpl(
    const FwiWeatherImpl& yesterday,
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
    : FwiWeatherImpl(
        {.temperature = temp, .rh = rh, .wind = wind, .prec = prec},
        (Ffmc::Invalid() == ffmc) ? Ffmc{temp, rh, wind.speed, prec, yesterday.ffmc} : ffmc,
        (Dmc::Invalid() == dmc) ? Dmc{temp, rh, prec, yesterday.dmc, month, latitude} : dmc,
        (Dc::Invalid() == dc) ? Dc{temp, prec, yesterday.dc, month, latitude} : dc,
        isi,
        bui,
        fwi
      )
  { }
  auto operator<=>(const FwiWeatherImpl& rhs) const = default;
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
class FwiWeather
{
private:
  static mutex mutex_;
  ptr<const FwiWeatherImpl> lookup(const FwiWeatherImpl& wx) noexcept
  {
    // keep unique FwiWeatherImpl and then just do pointer comparison for equality
    lock_guard<mutex> lock(mutex_);
    static set<FwiWeatherImpl> fwi_values{};
    static const FwiWeatherImpl empty{};
    if (empty == wx)
    {
      return nullptr;
    }
    auto e = fwi_values.emplace(wx);
    return &(*e.first);
  }

public:
  // static FwiWeather Zero() { return FwiWeather(FwiWeatherImpl::Zero()); }
  // static FwiWeather Invalid() { return FwiWeather(FwiWeatherImpl::Invalid()); }
  const Ffmc& ffmc() const noexcept { return impl_->ffmc; }
  const Dmc& dmc() const noexcept { return impl_->dmc; }
  const Dc& dc() const noexcept { return impl_->dc; }
  const Isi& isi() const noexcept { return impl_->isi; }
  const Bui& bui() const noexcept { return impl_->bui; }
  const Fwi& fwi() const noexcept { return impl_->fwi; }
  const Temperature& temperature() const noexcept { return impl_->temperature; };
  const RelativeHumidity& rh() const noexcept { return impl_->rh; };
  const Wind& wind() const noexcept { return impl_->wind; };
  const Precipitation& prec() const noexcept { return impl_->prec; };
  constexpr FwiWeather() noexcept = default;
  FwiWeather(const FwiWeatherImpl& wx) noexcept : impl_{FwiWeather::lookup(wx)} { }
  FwiWeather(const FwiWeather& rhs) noexcept : impl_{rhs.impl_} { }
  FwiWeather(FwiWeather&& rhs) noexcept : impl_{rhs.impl_} { }
  FwiWeather& operator=(const FwiWeather& rhs) noexcept
  {
    impl_ = rhs.impl_;
    return *this;
  }
  FwiWeather& operator=(FwiWeather&& rhs) noexcept
  {
    impl_ = rhs.impl_;
    return *this;
  }
  FwiWeather(
    const Weather wx,
    const Ffmc ffmc,
    const Dmc dmc,
    const Dc dc,
    const Isi isi = Isi::Invalid(),
    const Bui bui = Bui::Invalid(),
    const Fwi fwi = Fwi::Invalid()
  ) noexcept
    : FwiWeather{FwiWeatherImpl{Weather(wx), ffmc, dmc, dc, isi, bui, fwi}}
  { }
  FwiWeather(
    const Temperature temp,
    const RelativeHumidity rh,
    const Wind wind,
    const Precipitation prec,
    const Ffmc ffmc,
    const Dmc dmc,
    const Dc dc,
    // Isi isi = Isi::Invalid(),
    // Bui bui = Bui::Invalid(),
    // Fwi fwi = Fwi::Invalid()
    const Isi isi,
    const Bui bui,
    const Fwi fwi
  ) noexcept
    : FwiWeather{Weather{temp, rh, wind, prec}, ffmc, dmc, dc, isi, bui, fwi}
  { }
  FwiWeather(
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
    : FwiWeather{FwiWeatherImpl{
        *yesterday.impl_,
        month,
        latitude,
        temp,
        rh,
        wind,
        prec,
        ffmc,
        dmc,
        dc,
        isi,
        bui,
        fwi
      }}
  { }
  auto operator<=>(const FwiWeather& rhs) const { return *impl_ <=> *rhs.impl_; }
  auto operator==(const FwiWeather& rhs) const { return impl_ == rhs.impl_; }
  [[nodiscard]] MathSize mcFfmcPct() const { return impl_->mcFfmcPct(); }
  [[nodiscard]] MathSize mcDmcPct() const { return impl_->mcDmcPct(); }
  [[nodiscard]] MathSize mcFfmc() const { return impl_->mcFfmc(); }
  [[nodiscard]] MathSize mcDmc() const { return impl_->mcDmc(); }
  [[nodiscard]] MathSize ffmcEffect() const { return impl_->ffmcEffect(); }
  [[nodiscard]] bool isNull() const { return nullptr == impl_; }

private:
  ptr<const FwiWeatherImpl> impl_{nullptr};
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
