/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_LOG_H
#define FS_LOG_H
#include "stdafx.h"
#include <cstdlib>
#include <format>
#include <tuple>
#ifdef NDEBUG
#define DEBUG_NOEXCEPT_OFF noexcept
#else
#define DEBUG_NOEXCEPT_OFF
#endif
namespace fs::logging
{
enum class level
{
  silent = 0,
  fatal = 1,
  error = 2,
  warning = 3,
  note = 4,
  info = 5,
  debug = 6,
  verbose = 7,
  extensive = 8,
  min = silent,
  max = extensive
};
void set_log_level(const logging::level log_level) noexcept;
void increase_log_level() noexcept;
void decrease_log_level() noexcept;
logging::level get_log_level() noexcept;
inline bool should_log(const logging::level log_level) noexcept
{
  return get_log_level() >= log_level;
}
inline bool should_not_log(const logging::level log_level) noexcept
{
  return get_log_level() < log_level;
}
int open_log_file(const char* filename) noexcept;
int close_log_file() noexcept;
// log takes either a message that's ready to output, or a function that creates one
// HACK: this means we don't care how the message is generated, and none of the arguments should get
// resolved unless necessary
string output(logging::level log_level, const string msg) DEBUG_NOEXCEPT_OFF;
inline string output(logging::level log_level, std::function<string()> fct) DEBUG_NOEXCEPT_OFF
{
  return output(log_level, fct());
}
template <typename... Args>
inline string output(logging::level log_level, std::format_string<Args...> format, Args&&... args)
  DEBUG_NOEXCEPT_OFF
{
  // HACK: check log_level here too for now
  if (should_not_log(log_level))
  {
    return "";
  }
  return output(log_level, std::format(format, std::forward<Args>(args)...));
}
template <typename... Args>
inline void extensive(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  constexpr auto log_level = logging::level::extensive;
  // HACK: check log_level here too for now
  if (should_not_log(log_level))
  {
    return;
  }
  output(log_level, std::format(format, std::forward<Args>(args)...));
}
template <typename... Args>
inline void verbose(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  constexpr auto log_level = logging::level::verbose;
  // HACK: check log_level here too for now
  if (should_not_log(log_level))
  {
    return;
  }
  output(log_level, std::format(format, std::forward<Args>(args)...));
}
template <typename... Args>
inline void debug(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  constexpr auto log_level = logging::level::debug;
  // HACK: check log_level here too for now
  if (should_not_log(log_level))
  {
    return;
  }
  output(log_level, std::format(format, std::forward<Args>(args)...));
}
template <typename... Args>
inline void info(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  constexpr auto log_level = logging::level::info;
  // HACK: check log_level here too for now
  if (should_not_log(log_level))
  {
    return;
  }
  output(log_level, std::format(format, std::forward<Args>(args)...));
}
template <typename... Args>
inline void note(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  constexpr auto log_level = logging::level::note;
  // HACK: check log_level here too for now
  if (should_not_log(log_level))
  {
    return;
  }
  output(log_level, std::format(format, std::forward<Args>(args)...));
}
template <typename... Args>
inline void warning(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  constexpr auto log_level = logging::level::warning;
  // HACK: check log_level here too for now
  if (should_not_log(log_level))
  {
    return;
  }
  output(log_level, std::format(format, std::forward<Args>(args)...));
}
template <typename... Args>
inline void error(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  constexpr auto log_level = logging::level::error;
  // HACK: check log_level here too for now
  if (should_not_log(log_level))
  {
    return;
  }
  output(log_level, std::format(format, std::forward<Args>(args)...));
}
template <typename... Args>
inline int fatal(std::format_string<Args...> format, Args&&... args) DEBUG_NOEXCEPT_OFF
{
  auto msg = output(logging::level::fatal, std::format(format, std::forward<Args>(args)...));
#ifndef NDEBUG
  // HACK: just throw the format for a start - just want to see stack traces when debugging
  throw std::runtime_error(msg);
#endif
  exit(EXIT_FAILURE);
  return EXIT_FAILURE;
}
template <typename T>
inline T fatal(const string msg, const bool throw_msg = true) DEBUG_NOEXCEPT_OFF
{
  output(logging::level::fatal, msg);
#ifndef NDEBUG
  if (throw_msg)
  {
    // HACK: just throw the format for a start - just want to see stack traces when debugging
    throw std::runtime_error(msg);
  }
#else
  std::ignore = throw_msg;
#endif
  exit(EXIT_FAILURE);
}
template <typename T>
inline T fatal(std::function<string()> fct, const bool throw_msg = true) DEBUG_NOEXCEPT_OFF
{
  return fatal<T>(fct(), throw_msg);
}
template <typename T>
inline T fatal(const std::exception& ex)
{
  auto r = fatal<T>(ex.what(), false);
  throw ex;
  // HACK: to satisfy return type
  return r;
}
template <typename T>
inline T fatal(const std::exception& ex, std::function<string()> fct)
{
  std::ignore = fatal<T>(fct(), false);
  return fatal<T>(ex);
}
// overrides for void return
inline int fatal(const string msg, const bool throw_msg = true) DEBUG_NOEXCEPT_OFF
{
  std::ignore = fatal<int>(msg, throw_msg);
  return EXIT_FAILURE;
}
inline void fatal(std::function<string()> fct, const bool throw_msg = true) DEBUG_NOEXCEPT_OFF
{
  // HACK: just need any template
  std::ignore = fatal<int>(fct, throw_msg);
}
inline int fatal(const std::exception& ex)
{
  std::ignore = fatal<int>(ex);
  return EXIT_FAILURE;
}
inline int fatal(const std::exception& ex, std::function<string()> fct)
{
  fatal(fct(), false);
  return fatal(ex);
}
inline void check_fatal(bool condition, std::function<string()> fct) DEBUG_NOEXCEPT_OFF
{
  if (condition)
  {
    std::ignore = fatal<int>(fct);
  }
}
template <typename... Args>
inline void check_fatal(bool condition, std::format_string<Args...> format, Args&&... args)
  DEBUG_NOEXCEPT_OFF
{
  if (!condition)
  {
    return;
  }
  fatal(format, std::forward<Args>(args)...);
}
template <typename T>
inline void check_equal(const T lhs, const T rhs, const char* name) DEBUG_NOEXCEPT_OFF
{
  check_fatal(lhs != rhs, "Expected {:s} to be {} but got {}", name, rhs, lhs);
}
inline void check_equal(const char* lhs, const char* rhs, const char* name) DEBUG_NOEXCEPT_OFF
{
  check_fatal(0 != strcmp(lhs, rhs), "Expected {:s} to be {:s} got {:s}", name, rhs, lhs);
}
inline void check_equal(const bool lhs, const bool rhs, const char* name) DEBUG_NOEXCEPT_OFF
{
  check_fatal(
    lhs != rhs,
    "Expected {:s} to be {:s} but got {:s}",
    name,
    lhs ? "true" : "false",
    rhs ? "true" : "false"
  );
}
template <class V>
inline void check_equal_verbose(
  const logging::level log_level,
  const V& lhs,
  const V& rhs,
  const char* name
) DEBUG_NOEXCEPT_OFF
{
  check_equal(lhs, rhs, name);
  output(log_level, std::format("{:s} matches", name));
}
void check_tolerance(
  const MathSize epsilon,
  const MathSize lhs,
  const MathSize rhs,
  const char* name
) DEBUG_NOEXCEPT_OFF;
}
#endif
