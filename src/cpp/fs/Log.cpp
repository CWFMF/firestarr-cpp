/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Log.h"
namespace fs::logging
{
int Log::logging_level_ = LOG_DEBUG;
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
void Log::setLogLevel(const int log_level) noexcept { logging_level_ = log_level; }
void Log::increaseLogLevel() noexcept
{
  // HACK: make sure we never go below 0
  logging_level_ = max(0, getLogLevel() - 1);
}
void Log::decreaseLogLevel() noexcept
{
  // HACK: make sure we never go above silent
  logging_level_ = min(LOG_SILENT, getLogLevel() + 1);
}
int Log::getLogLevel() noexcept { return logging_level_; }
static ofstream out_{};
int Log::openLogFile(const char* filename) noexcept
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
int Log::closeLogFile() noexcept
{
  if (out_.is_open())
  {
    out_.close();
    return out_.fail();
  }
  return 0;
}
string format_log_message(const char* prefix, const char* const format, va_list* args)
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
  static char buffer[1024]{0};
  vsnprintf(buffer, std::size(buffer), format, *args);
  iss << buffer;
  return iss.str();
}
string output(const int log_level, const char* const format, va_list* args)
#ifdef NDEBUG
  noexcept
#endif
{
#ifdef NDEBUG
  if (Log::getLogLevel() > log_level)
  {
    return "";
  }
#endif
  try
  {
    lock_guard<mutex> lock(mutex_);
    auto msg = format_log_message(LOG_LABELS[log_level], format, args);
#ifndef NDEBUG
    // if debugging then output everything to log file but not necessarily stdout
    if (log_level >= Log::getLogLevel())
#endif
    {
      cout << msg << "\n";
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
    return msg;
  }
  catch (const std::exception& ex)
  {
    logging::fatal(ex);
    std::terminate();
  }
}
string output(const int log_level, const char* const format, ...)
#ifdef NDEBUG
  noexcept
#endif
{
  va_list args;
  va_start(args, format);
  return output(log_level, format, &args);
  va_end(args);
}
void extensive(const char* const format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_EXTENSIVE)
  {
    va_list args;
    va_start(args, format);
    output(LOG_EXTENSIVE, format, &args);
    va_end(args);
  }
}
void verbose(const char* const format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_VERBOSE)
  {
    va_list args;
    va_start(args, format);
    output(LOG_VERBOSE, format, &args);
    va_end(args);
  }
}
void debug(const char* const format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_DEBUG)
  {
    va_list args;
    va_start(args, format);
    output(LOG_DEBUG, format, &args);
    va_end(args);
  }
}
void info(const char* const format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_INFO)
  {
    va_list args;
    va_start(args, format);
    output(LOG_INFO, format, &args);
    va_end(args);
  }
}
void note(const char* const format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_NOTE)
  {
    va_list args;
    va_start(args, format);
    output(LOG_NOTE, format, &args);
    va_end(args);
  }
}
void warning(const char* const format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_WARNING)
  {
    va_list args;
    va_start(args, format);
    output(LOG_WARNING, format, &args);
    va_end(args);
  }
}
void error(const char* const format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_ERROR)
  {
    va_list args;
    va_start(args, format);
    output(LOG_ERROR, format, &args);
    va_end(args);
  }
}
void fatal(const char* const format, va_list* args)
#ifdef NDEBUG
  noexcept
#endif
{
  // HACK: call the other version
  fatal<int>(format, args);
}
void fatal(const char* const format, ...)
#ifdef NDEBUG
  noexcept
#endif
{
  va_list args;
  va_start(args, format);
  fatal(format, &args);
  va_end(args);
}
void fatal(const std::exception& ex)
{
  output(LOG_FATAL, "%s", ex.what());
  Log::closeLogFile();
#ifdef NDEBUG
  exit(EXIT_FAILURE);
#endif
}
void fatal(const std::exception& ex, const char* const format, ...)
{
  va_list args;
  va_start(args, format);
  output(LOG_FATAL, format, &args);
  // cppcheck-suppress va_end_missing
  fatal(ex);
}
void check_fatal(const bool condition, const char* const format, va_list* args)
#ifdef NDEBUG
  noexcept
#endif
{
  if (condition)
  {
    fatal(format, args);
  }
}
inline void check_fatal(const bool condition, const char* const format, ...)
#ifdef NDEBUG
  noexcept
#endif
{
  if (condition)
  {
    va_list args;
    va_start(args, format);
    fatal(format, &args);
    // cppcheck-suppress va_end_missing
  }
}
void check_equal(const MathSize lhs, const MathSize rhs, const char* name)
#ifdef NDEBUG
  noexcept
#endif
{
  logging::check_fatal(lhs != rhs, "Expected %s to be %f but got %f", name, rhs, lhs);
}
void check_equal(const char* lhs, const char* rhs, const char* name)
#ifdef NDEBUG
  noexcept
#endif
{
  logging::check_fatal(0 != strcmp(lhs, rhs), "Expected %s to be %s got %s", name, rhs, lhs);
}
void check_equal(const bool lhs, const bool rhs, const char* name)
#ifdef NDEBUG
  noexcept
#endif
{
  bool a = lhs ? true : false;
  bool b = rhs ? true : false;
  logging::check_fatal(
    a != b, "Expected %s to be %s but got %s", name, a ? "true" : "false", b ? "true" : "false"
  );
}
void SelfLogger::log_output(const int level, const char* const format, ...) const noexcept
{
  // FIX: better/any way to call this from other level-specific functions?
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(level, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_extensive(const char* const format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_EXTENSIVE, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_verbose(const char* const format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_VERBOSE, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_debug(const char* const format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_DEBUG, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_info(const char* const format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_INFO, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_note(const char* const format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_NOTE, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_warning(const char* const format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_WARNING, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_error(const char* const format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_ERROR, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_check_fatal(bool condition, const char* const format, ...) const
#ifdef NDEBUG
  noexcept
#endif
{
  if (condition)
  {
    va_list args;
    va_start(args, format);
    const auto fmt = add_log(format);
    logging::fatal(fmt.c_str(), &args);
    va_end(args);
  }
}
void SelfLogger::log_fatal(const char* const format, ...) const
#ifdef NDEBUG
  noexcept
#endif
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::fatal(fmt.c_str(), &args);
  va_end(args);
}
Log::~Log() noexcept { closeLogFile(); };
void check_tolerance(
  const MathSize epsilon,
  const MathSize lhs,
  const MathSize rhs,
  const char* name
)
#ifdef NDEBUG
  noexcept
#endif
{
  const auto difference = abs(lhs - rhs);
  logging::check_fatal(
    difference >= epsilon,
    "Difference too big for %s: (%g > %g) for %g vs %g",
    name,
    difference,
    epsilon,
    rhs,
    lhs
  );
}
}
