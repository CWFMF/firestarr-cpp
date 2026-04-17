/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Log.h"
#include <ctime>
#ifndef DEBUG_NOEXCEPT_OFF
#error DEBUG_NOEXCEPT_OFF not defined
#endif
namespace fs::logging
{
// Current logging level
logging::level logging_level_ = logging::level::debug;
// do this in .cpp so that we don't get unused warnings including the .h
static const char* LOG_LABELS[] = {
  "SILENT:    ",
  "FATAL:     ",
  "ERROR:     ",
  "WARNING:   ",
  "NOTE:      ",
  "INFO:      ",
  "DEBUG:     ",
  "VERBOSE:   ",
  "EXTENSIVE: "
};
stringstream pre_file_log{};
mutex mutex_;
void set_log_level(const logging::level log_level) noexcept { logging_level_ = log_level; }
void increase_log_level() noexcept
{
  // HACK: make sure we never go above max
  logging_level_ =
    logging::level{min(static_cast<int>(logging::level::max), static_cast<int>(get_log_level()) + 1)
    };
}
void decrease_log_level() noexcept
{
  // HACK: make sure we never go below min
  logging_level_ =
    logging::level{max(static_cast<int>(logging::level::min), static_cast<int>(get_log_level()) - 1)
    };
}
logging::level get_log_level() noexcept { return logging_level_; }
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
  stringstream iss{};
  iss
  // try to make output consistent if in debug mode
#ifdef NDEBUG
    << put_time(
         [&]() {
           const time_t now = time(nullptr);
           return localtime(&now);
         }(),
         "[%F %T] "
       )
#endif
    << prefix << msg;
  return iss.str();
}
string output(const logging::level log_level, const string msg) DEBUG_NOEXCEPT_OFF
{
#ifdef NDEBUG
  if (should_not_log(log_level))
  {
    return "";
  }
#endif
  try
  {
    lock_guard<mutex> lock(mutex_);
    auto msg_fmt = format_log_message(LOG_LABELS[static_cast<int>(log_level)], msg);
#ifndef NDEBUG
    // if debugging then output everything to log file but not necessarily stdout
    if (should_log(log_level))
#endif
    {
      cout << msg_fmt << "\n";
    }
#ifndef NDEBUG
    // if debugging then output everything to log file but not necessarily stdout
    if (should_log(logging::level::verbose))
#endif
    {   // fflush(stdout);
      if (out_.is_open())
      {
        out_ << msg_fmt << "\n";
        out_.flush();
      }
      else
      {
        pre_file_log << msg_fmt << "\n";
        pre_file_log.clear();
      }
    }
    return msg_fmt;
  }
  catch (const std::exception& ex)
  {
    exit(fatal(ex));
  }
}
}
