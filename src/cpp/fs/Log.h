/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_LOG_H
#define FS_LOG_H
#include "stdafx.h"
#ifdef NDEBUG
#define DEBUG_NOEXCEPT_OFF noexcept
#else
#undef DEBUG_NOEXCEPT_OFF
#endif
namespace fs::logging
{
static constexpr int LOG_EXTENSIVE = 0;
static constexpr int LOG_VERBOSE = 1;
static constexpr int LOG_DEBUG = 2;
static constexpr int LOG_INFO = 3;
static constexpr int LOG_NOTE = 4;
static constexpr int LOG_WARNING = 5;
static constexpr int LOG_ERROR = 6;
static constexpr int LOG_FATAL = 7;
static constexpr int LOG_SILENT = 8;
void set_log_level(int log_level) noexcept;
void increase_log_level() noexcept;
void decrease_log_level() noexcept;
int get_log_level() noexcept;
int open_log_file(const char* filename) noexcept;
int close_log_file() noexcept;
string output(int log_level, const char* const format, va_list* args) DEBUG_NOEXCEPT_OFF;
string output(int log_level, const char* const format, ...) DEBUG_NOEXCEPT_OFF;
void extensive(const char* const format, ...) noexcept;
void verbose(const char* const format, ...) noexcept;
void debug(const char* const format, ...) noexcept;
void info(const char* const format, ...) noexcept;
void note(const char* const format, ...) noexcept;
void warning(const char* const format, ...) noexcept;
void error(const char* const format, ...) noexcept;
void check_fatal(bool condition, const char* const format, ...) DEBUG_NOEXCEPT_OFF;
void check_equal(const MathSize lhs, const MathSize rhs, const char* name) DEBUG_NOEXCEPT_OFF;
void check_equal(const char* lhs, const char* rhs, const char* name) DEBUG_NOEXCEPT_OFF;
void check_equal(const bool lhs, const bool rhs, const char* name) DEBUG_NOEXCEPT_OFF;
template <class V>
void check_equal(const V& lhs, const V& rhs, const char* name) DEBUG_NOEXCEPT_OFF
{
  const auto fmt = typeid(lhs) == typeid(size_t) ? "Expected %s to be %ld but got %ld"
                                                 : "Expected %s to be %d but got %d";
  check_fatal(lhs != rhs, fmt, name, rhs, lhs);
}
template <class V>
void check_equal_verbose(const int log_level, const V& lhs, const V& rhs, const char* name)
  DEBUG_NOEXCEPT_OFF
{
  check_equal(lhs, rhs, name);
  output(log_level, "%s matches", name);
}
void check_tolerance(
  const MathSize epsilon,
  const MathSize lhs,
  const MathSize rhs,
  const char* name
) DEBUG_NOEXCEPT_OFF;
void fatal(const char* const format, ...) DEBUG_NOEXCEPT_OFF;
void fatal(const std::exception& ex);
void fatal(const std::exception& ex, const char* const format, ...);
template <class T>
T fatal(const char* const format, va_list* args) DEBUG_NOEXCEPT_OFF
{
  auto msg = output(LOG_FATAL, format, args);
  fflush(stdout);
  close_log_file();
#ifdef NDEBUG
  exit(EXIT_FAILURE);
#else
  // HACK: just throw the format for a start - just want to see stack traces when debugging
  throw std::runtime_error(msg);
#endif
}
template <class T>
T fatal(const char* const format, ...) DEBUG_NOEXCEPT_OFF
{
  va_list args;
  va_start(args, format);
  return fatal<T>(format, &args);
}
class SelfLogger
{
public:
  virtual ~SelfLogger() = default;
  virtual string add_log(const char* const format) const noexcept = 0;
  void log_output(const int level, const char* const format, ...) const noexcept;
  void log_extensive(const char* const format, ...) const noexcept;
  void log_verbose(const char* const format, ...) const noexcept;
  void log_debug(const char* const format, ...) const noexcept;
  void log_info(const char* const format, ...) const noexcept;
  void log_note(const char* const format, ...) const noexcept;
  void log_warning(const char* const format, ...) const noexcept;
  void log_error(const char* const format, ...) const noexcept;
  void log_check_fatal(bool condition, const char* const format, ...) const DEBUG_NOEXCEPT_OFF;
  void log_fatal(const char* const format, ...) const DEBUG_NOEXCEPT_OFF;
};
}
#endif
