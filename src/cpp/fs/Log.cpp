/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Log.h"
#ifndef DEBUG_NOEXCEPT_OFF
#error DEBUG_NOEXCEPT_OFF not defined
#endif
namespace fs::logging
{
// Current logging level
int logging_level_ = LOG_DEBUG;
// do this in .cpp so that we don't get unused warnings including the .h
static const char* LOG_LABELS[] = {
  "EXTENSIVE: ",
  "VERBOSE:   ",
  "DEBUG:     ",
  "INFO:      ",
  "NOTE:      ",
  "WARNING:   ",
  "ERROR:     ",
  "FATAL:     ",
  "SILENT:    "
};
stringstream pre_file_log{};
mutex mutex_;
void set_log_level(const int log_level) noexcept { logging_level_ = log_level; }
void increase_log_level() noexcept
{
  // HACK: make sure we never go below 0
  logging_level_ = max(0, get_log_level() - 1);
}
void decrease_log_level() noexcept
{
  // HACK: make sure we never go above silent
  logging_level_ = min(LOG_SILENT, get_log_level() + 1);
}
int get_log_level() noexcept { return logging_level_; }
// closes automatically when ofstream is deconstructed
ofstream out_{};
int open_log_file(const char* filename) noexcept
{
  // turn off buffering so lines write to file immediately
  out_.rdbuf()->pubsetbuf(0, 0);
  out_.open(filename);
  if (!out_.is_open())
  {
    return false;
  }
  const auto log_so_far = pre_file_log.str();
  if (!log_so_far.empty())
  {
    out_ << log_so_far;
    pre_file_log.clear();
  }
  return true;
}
int close_log_file() noexcept
{
  if (out_.is_open())
  {
    out_.close();
    return out_.fail();
  }
  return 0;
}
string format_log_message(const char* prefix, const string msg)
{
  // do this separately from output() so we can redo it for fatal errors
  // NOTE: create string first so that entire line writes
  // (otherwise threads might mix lines)
  const string tmp;
  stringstream iss(tmp);
#ifdef NDEBUG
  const time_t now = time(nullptr);
  auto buf = localtime(&now);
  iss << put_time(buf, "[%F %T] ");
#endif
  // try to make output consistent if in debug mode
  iss << prefix;
  iss << msg;
  return iss.str();
}
string output(const int log_level, const string msg) DEBUG_NOEXCEPT_OFF
{
#ifdef NDEBUG
  if (get_log_level() > log_level)
  {
    return "";
  }
#endif
  try
  {
    lock_guard<mutex> lock(mutex_);
    auto msg_fmt = format_log_message(LOG_LABELS[log_level], msg);
#ifndef NDEBUG
    // if debugging then output everything to log file but not necessarily stdout
    if (log_level >= get_log_level())
#endif
    {
      cout << msg_fmt << "\n";
    }
#ifndef NDEBUG
    // if debugging then output everything to log file but not necessarily stdout
    if (log_level >= LOG_VERBOSE)
#endif
    {   // fflush(stdout);
      if (out_.is_open())
      {
        out_ << msg << "\n";
        out_.flush();
      }
      else
      {
        pre_file_log << msg << "\n";
      }
    }
    return msg_fmt;
  }
  catch (const std::exception& ex)
  {
    fatal(ex);
    std::terminate();
  }
}
void check_tolerance(
  const MathSize epsilon,
  const MathSize lhs,
  const MathSize rhs,
  const char* name
) DEBUG_NOEXCEPT_OFF
{
  const auto difference = abs(lhs - rhs);
  check_fatal(difference >= epsilon, [&]() {
    return std::format(
      "Difference too big for {:s}: ({:g} > {:g}) for {:g} vs {:g}",
      name,
      difference,
      epsilon,
      rhs,
      lhs
    );
  });
}
}
