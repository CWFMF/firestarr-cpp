/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "stdafx.h"
#include "Cell.h"
#include "FireSpread.h"

namespace fs::sim
{
using topo::Cell;
using fs::wx::Direction;
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
  [[nodiscard]] static constexpr Event makeEvent(
    const DurationSize time,
    const Cell& cell,
    const Type type)
  {
    return {
      time,
      cell,
      type,
      0};
  }
  /**
   * \brief Make simulation end event
   * \param time Time to schedule for
   * \return Event created
   */
  [[nodiscard]] static constexpr Event makeEnd(const DurationSize time)
  {
    return makeEvent(
      time,
      NoLocation,
      END_SIMULATION);
  }
  /**
   * \brief Make new fire event
   * \param time Time to schedule for
   * \param cell Cell to start new fire in
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeNewFire(
    const DurationSize time,
    const Cell& cell)
  {
    return makeEvent(
      time,
      cell,
      NEW_FIRE);
  }
  /**
   * \brief Make simulation save event
   * \param time Time to schedule for
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeSave(const DurationSize time)
  {
    return makeEvent(
      time,
      NoLocation,
      SAVE);
  }
  /**
   * \brief Make fire spread event
   * \param time Time to schedule for
   * \return Event created
   */
  [[nodiscard]] static Event constexpr makeFireSpread(const DurationSize time)
  {
    return makeEvent(
      time,
      NoLocation,
      FIRE_SPREAD);
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
    const Cell& cell)
  {
    return {
      time,
      cell,
      FIRE_SPREAD,
      0};
  }
  ~Event() = default;
  /**
   * \brief Move constructor
   * \param rhs Event to move from
   */
  Event(Event&& rhs) noexcept = default;
  /**
   * \brief Copy constructor
   * \param rhs Event to copy from
   */
  Event(const Event& rhs) = delete;
  /**
   * \brief Move assignment
   * \param rhs Event to move from
   * \return This, after assignment
   */
  Event& operator=(Event&& rhs) noexcept = default;
  /**
   * \brief Copy assignment
   * \param rhs Event to copy from
   * \return This, after assignment
   */
  Event& operator=(const Event& rhs) = delete;
  /**
   * \brief Time of Event (decimal days)
   * \return Time of Event (decimal days)
   */
  [[nodiscard]] constexpr DurationSize time() const
  {
    return time_;
  }
  /**
   * \brief Type of Event
   * \return Type of Event
   */
  [[nodiscard]] constexpr Type type() const
  {
    return type_;
  }
  /**
   * \brief Duration that Event Cell has been burning (decimal days)
   * \return Duration that Event Cell has been burning (decimal days)
   */
  [[nodiscard]] constexpr DurationSize timeAtLocation() const
  {
    return time_at_location_;
  }
  /**
   * \brief Cell Event takes place in
   * \return Cell Event takes place in
   */
  [[nodiscard]] constexpr const Cell& cell() const
  {
    return cell_;
  }
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
  constexpr Event(const DurationSize time,
                  const Cell& cell,
                  const Type type,
                  const DurationSize time_at_location)
    : time_(time),
      time_at_location_(time_at_location),
      cell_(cell),
      type_(type)
  {
  }
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
};
}
