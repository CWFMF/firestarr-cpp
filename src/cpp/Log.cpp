/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2024-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Log.h"
namespace fs::logging
{
int Log::logging_level_ = LOG_DEBUG;
void
Log::setLogLevel(
  const int log_level
) noexcept
{
  logging_level_ = log_level;
}
void
Log::increaseLogLevel() noexcept
{
  // HACK: make sure we never go below 0
  logging_level_ = max(0, getLogLevel() - 1);
}
void
Log::decreaseLogLevel() noexcept
{
  // HACK: make sure we never go above silent
  logging_level_ = min(LOG_SILENT, getLogLevel() + 1);
}
int
Log::getLogLevel() noexcept
{
  return logging_level_;
}
void
Log::output(
  const char* name,
  const char* format,
  va_list* args
) noexcept
{
  try
  {
    const time_t now = time(nullptr);
    auto buf = localtime(&now);
    // NOTE: create string first so that entire line writes
    // (otherwise threads might mix lines)
    const string tmp;
    stringstream iss(tmp);
    iss << put_time(buf, "[%F %T] ") << name;
    static char buffer[1024]{0};
    vsprintf(buffer, format, *args);
    iss << buffer << "\n";
    cout << iss.str();
  }
  catch (...)
  {
    std::terminate();
  }
}
void
extensive(
  const char* format,
  ...
) noexcept
{
  if (Log::getLogLevel() <= LOG_EXTENSIVE)
  {
    va_list args;
    va_start(args, format);
    Log::output("EXTENSIVE:", format, &args);
    va_end(args);
  }
}
void
verbose(
  const char* format,
  ...
) noexcept
{
  if (Log::getLogLevel() <= LOG_VERBOSE)
  {
    va_list args;
    va_start(args, format);
    Log::output("VERBOSE:", format, &args);
    va_end(args);
  }
}
void
debug(
  const char* format,
  ...
) noexcept
{
  if (Log::getLogLevel() <= LOG_DEBUG)
  {
    va_list args;
    va_start(args, format);
    Log::output("DEBUG: ", format, &args);
    va_end(args);
  }
}
void
info(
  const char* format,
  ...
) noexcept
{
  if (Log::getLogLevel() <= LOG_INFO)
  {
    va_list args;
    va_start(args, format);
    Log::output("INFO:  ", format, &args);
    va_end(args);
  }
}
void
note(
  const char* format,
  ...
) noexcept
{
  if (Log::getLogLevel() <= LOG_NOTE)
  {
    va_list args;
    va_start(args, format);
    Log::output("NOTE:  ", format, &args);
    va_end(args);
  }
}
void
warning(
  const char* format,
  ...
) noexcept
{
  if (Log::getLogLevel() <= LOG_WARNING)
  {
    va_list args;
    va_start(args, format);
    Log::output("WARN:  ", format, &args);
    va_end(args);
  }
}
void
error(
  const char* format,
  ...
) noexcept
{
  if (Log::getLogLevel() <= LOG_ERROR)
  {
    va_list args;
    va_start(args, format);
    Log::output("ERROR: ", format, &args);
    va_end(args);
  }
}
void
fatal(
  const char* format,
  va_list* args
) noexcept
{
  // HACK: call the other version
  fatal<int>(format, args);
}
void
fatal(
  const char* format,
  ...
) noexcept
{
  va_list args;
  va_start(args, format);
  fatal(format, &args);
  // cppcheck-suppress va_end_missing
  // va_end(args);
}
void
check_fatal(
  const bool condition,
  const char* format,
  va_list* args
) noexcept
{
  if (condition)
  {
    fatal(format, args);
  }
}
void
check_fatal(
  const bool condition,
  const char* format,
  ...
) noexcept
{
  if (condition)
  {
    va_list args;
    va_start(args, format);
    fatal(format, &args);
    // cppcheck-suppress va_end_missing
    // va_end(args);
  }
}
}
