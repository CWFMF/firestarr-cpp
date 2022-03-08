// Copyright (c) 2020-2021, Queen's Printer for Ontario.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "stdafx.h"
#include "Log.h"
namespace firestarr::logging
{
int Log::logging_level_ = LOG_DEBUG;
// do this in .cpp so that we don't get unused warnings including the .h
static const char* LOG_LABELS[] =
  {
    "EXTENSIVE: ",
    "VERBOSE:   ",
    "DEBUG:     ",
    "INFO:      ",
    "NOTE:      ",
    "WARNING:   ",
    "ERROR:     ",
    "FATAL:     ",
    "SILENT:    "};
mutex mutex_;
void Log::setLogLevel(const int log_level) noexcept
{
  logging_level_ = log_level;
}
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
int Log::getLogLevel() noexcept
{
  return logging_level_;
}
static ofstream out_stream_;
int Log::openLogFile(const char* filename) noexcept
{
  out_stream_.open(filename);
  return out_stream_.is_open();
}
int Log::closeLogFile() noexcept
{
  out_stream_.close();
  return !out_stream_.is_open();
}
void output(const int log_level, const char* format, va_list* args) noexcept
{
  if (Log::getLogLevel() > log_level)
  {
    return;
  }
  try
  {
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
    iss << LOG_LABELS[log_level];
    static char buffer[1024]{0};
    vsprintf(buffer, format, *args);
    iss << buffer << "\n";
    {
      lock_guard<mutex> lock(mutex_);
      cout << iss.str();
      out_stream_ << iss.str();
    }
  }
  catch (...)
  {
    std::terminate();
  }
}
void extensive(const char* format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_EXTENSIVE)
  {
    va_list args;
    va_start(args, format);
    output(LOG_EXTENSIVE, format, &args);
    va_end(args);
  }
}
void verbose(const char* format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_VERBOSE)
  {
    va_list args;
    va_start(args, format);
    output(LOG_VERBOSE, format, &args);
    va_end(args);
  }
}
void debug(const char* format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_DEBUG)
  {
    va_list args;
    va_start(args, format);
    output(LOG_DEBUG, format, &args);
    va_end(args);
  }
}
void info(const char* format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_INFO)
  {
    va_list args;
    va_start(args, format);
    output(LOG_INFO, format, &args);
    va_end(args);
  }
}
void note(const char* format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_NOTE)
  {
    va_list args;
    va_start(args, format);
    output(LOG_NOTE, format, &args);
    va_end(args);
  }
}
void warning(const char* format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_WARNING)
  {
    va_list args;
    va_start(args, format);
    output(LOG_WARNING, format, &args);
    va_end(args);
  }
}
void error(const char* format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_ERROR)
  {
    va_list args;
    va_start(args, format);
    output(LOG_ERROR, format, &args);
    va_end(args);
  }
}
void fatal(const char* format, va_list* args) noexcept
{
  // HACK: call the other version
  fatal<int>(format, args);
}
void fatal(const char* format, ...) noexcept
{
  va_list args;
  va_start(args, format);
  fatal(format, &args);
  // cppcheck-suppress va_end_missing
  // va_end(args);
}
void check_fatal(const bool condition, const char* format, va_list* args) noexcept
{
  if (condition)
  {
    fatal(format, args);
  }
}
void check_fatal(const bool condition, const char* format, ...) noexcept
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

void SelfLogger::log_extensive(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_EXTENSIVE, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_verbose(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_VERBOSE, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_debug(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_DEBUG, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_info(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_INFO, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_note(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_NOTE, fmt.c_str(), &args);
  va_end(args);
}

void SelfLogger::log_warning(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_WARNING, fmt.c_str(), &args);
  va_end(args);
}

void SelfLogger::log_error(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_ERROR, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_check_fatal(bool condition, const char* format, ...) const noexcept
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

void SelfLogger::log_fatal(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::fatal(fmt.c_str(), &args);
  va_end(args);
}
}
