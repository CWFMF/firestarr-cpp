/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "LogPoints.h"
#define LOG_POINTS_RELATIVE
// #undef LOG_POINTS_RELATIVE
#define LOG_POINTS_CELL
#undef LOG_POINTS_CELL
#include "Util.h"
namespace fs
{
// FIX: clean this up but for now just hide the details from outside
LogPoints::~LogPoints()
{
  if (nullptr != log_points_)
  {
    fclose(log_points_);
    fclose(log_stages_);
  }
}
LogPoints::LogPoints(
  const string_view output_directory,
  bool do_log,
  size_t id,
  DurationSize start_time
) noexcept
  : id_(id), start_time_(start_time), last_stage_(STAGE_INVALID),
    last_step_(std::numeric_limits<size_t>::max()), stage_id_()
{
  if (do_log)
  {
    constexpr auto HEADER_STAGES = "step_id,scenario,stage,step,time\n";
#ifdef LOG_POINTS_CELL
    constexpr auto HEADER_POINTS = "step_id,column,row,x,y\n";
#else
    constexpr auto HEADER_POINTS = "step_id,x,y\n";
#endif
    char log_name[2048];
    sxprintf(log_name, "%s/scenario_%05ld_points.txt", string(output_directory).c_str(), id);
    log_points_ = fopen(log_name, "w");
    sxprintf(log_name, "%s/scenario_%05ld_stages.txt", string(output_directory).c_str(), id);
    log_stages_ = fopen(log_name, "w");
    fprintf(log_points_, HEADER_POINTS);
    fprintf(log_stages_, HEADER_STAGES);
  }
  else
  {
    log_points_ = nullptr;
    log_stages_ = nullptr;
  }
}
void LogPoints::log(
  size_t step,
  const char stage,
  const DurationSize time,
  const XYSize x,
  const XYSize y
) const noexcept
{
  if (!isLogging())
  {
    return;
  }
  log_unchecked(step, stage, time, x, y);
}
void LogPoints::log_unchecked(
  size_t step,
  const char stage,
  const DurationSize time,
  const XYSize x,
  const XYSize y
) const noexcept
{
  static const auto FMT_LOG_STAGE = "%s,%ld,%c,%ld,%f\n";
#ifdef LOG_POINTS_CELL
  static const auto FMT_LOG_POINT = "%s,%d,%d,%f,%f\n";
  const auto column = static_cast<Idx>(x);
  const auto row = static_cast<Idx>(y);
#else
  static const auto FMT_LOG_POINT = "%s,%f,%f\n";
#endif
#ifdef LOG_POINTS_RELATIVE
  constexpr auto MID = MAX_COLUMNS / 2;
  const auto p_x = x - MID;
  const auto p_y = y - MID;
  const auto t = time - start_time_;
#else
  const auto p_x = x;
  const auto p_y = y;
  const auto t = time;
#endif
  // time should always be the same for each step, regardless of stage
  if (last_step_ != step || last_stage_ != stage)
  {
    sxprintf(stage_id_, "%ld%c%ld", id_, stage, step);
    last_stage_ = stage;
    last_step_ = step;
#ifdef DEBUG_POINTS
    last_time_ = t;
#endif
    fprintf(log_stages_, FMT_LOG_STAGE, stage_id_, id_, stage, step, t);
#ifdef DEBUG_POINTS
  }
  else
  {
    logging::check_fatal(
      t != last_time_, "Expected %s to have time %f but got %f", stage_id_, last_time_, t
    );
#endif
  }
  fprintf(
    log_points_,
    FMT_LOG_POINT,
    stage_id_,
#ifdef LOG_POINTS_CELL
    column,
    row,
#endif
    static_cast<double>(p_x),
    static_cast<double>(p_y)
  );
}
bool LogPoints::isLogging() const noexcept { return nullptr != log_points_; }
}
