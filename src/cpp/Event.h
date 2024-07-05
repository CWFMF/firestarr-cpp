/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_EVENT_H
#define FS_EVENT_H
#include "stdafx.h"
#include "Cell.h"
#include "FireSpread.h"
namespace fs
{
/**
 * \brief A specific Event scheduled in a specific Scenario.
 */
class Event
{
public:
  /**
   * \brief Cell representing no location
   */
  static constexpr Cell NoLocation{};
  // HACK: use type, so we can sort without having to give different times to them
  /**
   * \brief Type of Event
   */
  enum Type
  {
    SAVE,
    END_SIMULATION,
    NEW_FIRE,
    FIRE_SPREAD,
  };
  /**
   * \brief Make simulation end event
   * \param time Time to schedule for
   * \return Event created
   */
  [[nodiscard]] static constexpr Event makeEnd(const DurationSize time)
  {
    return {time, NoLocation, 0, END_SIMULATION, nullptr, 0};
  }
  /**
   * \brief Make new fire event
   * \param time Time to schedule for
   * \param cell Cell to start new fire in
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeNewFire(const DurationSize time, const Cell& cell)
  {
    return {time, cell, 0, NEW_FIRE, nullptr, 0};
  }
  /**
   * \brief Make simulation save event
   * \param time Time to schedule for
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeSave(const DurationSize time)
  {
    return {time, NoLocation, 0, SAVE, nullptr, 0};
  }
  /**
   * \brief Make fire spread event
   * \param time Time to schedule for
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeFireSpread(const DurationSize time)
  {
    return makeFireSpread(time, nullptr);
  }
  /**
   * \brief Make fire spread event
   * \param time Time to schedule for
   * \param intensity Intensity to spread with (kW/m)
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeFireSpread(
    const DurationSize time,
    const SpreadInfo* spread_info
  )
  {
    return makeFireSpread(time, spread_info, NoLocation);
  }
  /**
   * \brief Make fire spread event
   * \param time Time to schedule for
   * \param intensity Intensity to spread with (kW/m)
   * \param cell Cell to spread in
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeFireSpread(
    const DurationSize time,
    const SpreadInfo* spread_info,
    const Cell& cell
  )
  {
    return makeFireSpread(time, spread_info, cell, 254);
  }
  /**
   * \brief Make fire spread event
   * \param time Time to schedule for
   * \param intensity Intensity to spread with (kW/m)
   * \param cell Cell to spread in
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeFireSpread(
    const DurationSize time,
    const SpreadInfo* spread_info,
    const Cell& cell,
    const CellIndex source
  )
  {
    return {time, cell, source, FIRE_SPREAD, spread_info, 0};
  }
  ~Event() = default;
  Event(Event&& rhs) noexcept = default;
  Event(const Event& rhs) = delete;
  Event& operator=(Event&& rhs) noexcept = default;
  Event& operator=(const Event& rhs) = delete;
  /**
   * \brief Time of Event (decimal days)
   * \return Time of Event (decimal days)
   */
  [[nodiscard]] constexpr DurationSize time() const { return time_; }
  /**
   * \brief Type of Event
   * \return Type of Event
   */
  [[nodiscard]] constexpr Type type() const { return type_; }
  /**
   * \brief Duration that Event Cell has been burning (decimal days)
   * \return Duration that Event Cell has been burning (decimal days)
   */
  [[nodiscard]] constexpr DurationSize timeAtLocation() const { return time_at_location_; }
  /**
   * \brief Burn Intensity (kW/m)
   * \return Burn Intensity (kW/m)
   */
  [[nodiscard]] constexpr IntensitySize intensity() const
  {
    return nullptr == spread_info_ ? 1 : spread_info_->maxIntensity();
  }
  /**
   * \brief Head fire spread direction
   * \return Head fire spread direction
   */
  [[nodiscard]] constexpr Direction raz() const
  {
    return nullptr == spread_info_ ? fs::Direction::Zero : spread_info_->headDirection();
  }
  /**
   * \brief Head fire rate of spread (m/min)
   * \return Head fire rate of spread (m/min)
   */
  [[nodiscard]] constexpr MathSize ros() const
  {
    return nullptr == spread_info_ ? 0 : spread_info_->headRos();
  }
  /**
   * \brief Cell Event takes place in
   * \return Cell Event takes place in
   */
  [[nodiscard]] constexpr const Cell& cell() const { return cell_; }
  /**
   * \brief CellIndex for relative Cell that spread into from
   * \return CellIndex for relative Cell that spread into from
   */
  [[nodiscard]] constexpr CellIndex source() const { return source_; }

private:
  /**
   * \brief Constructor
   * \param time Time to schedule for
   * \param cell CellIndex for relative Cell that spread into from
   * \param source Source that Event is coming from
   * \param type Type of Event
   * \param intensity Intensity to spread with (kW/m)
   * \param time_at_location Duration that Event Cell has been burning (decimal days)
   */
  constexpr Event(
    const DurationSize time,
    const Cell& cell,
    const CellIndex source,
    const Type type,
    const SpreadInfo* spread_info,
    const DurationSize time_at_location
  )
    : time_(time), time_at_location_(time_at_location), cell_(cell), type_(type),
      spread_info_(spread_info), source_(source)
  { }
  /**
   * \brief Time to schedule for
   */
  DurationSize time_;
  /**
   * \brief Duration that Event Cell has been burning (decimal days)
   */
  DurationSize time_at_location_;
  /**
   * \brief Cell to spread in
   */
  Cell cell_;
  /**
   * \brief Type of Event
   */
  Type type_;
  /**
   * \brief Spread information at time and place of event
   */
  const SpreadInfo* spread_info_{nullptr};
  /**
   * \brief CellIndex for relative Cell that spread into from
   */
  CellIndex source_;
};
}
#endif
