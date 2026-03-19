/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_ARGUMENT_PARSER_H
#define FS_ARGUMENT_PARSER_H
#include "stdafx.h"
#include "FWI.h"
#include "Log.h"
#include "Weather.h"
namespace fs
{
using fs::logging::Log;
struct Usage
{
  string description{};
  string positional_arg_summary{};
};
enum class PositionalArgumentsRequired
{
  Required,
  NotRequired
};
class ArgumentParser
{
private:
  vector<string> positional_args_{};
  size_t cur_positional_{0};
  PositionalArgumentsRequired require_positional_{};
  bool help_requested_{false};

public:
  virtual ~ArgumentParser() = default;
  ArgumentParser(
    const Usage usage,
    const int argc,
    const char* const argv[],
    const PositionalArgumentsRequired require_positional = PositionalArgumentsRequired::Required
  );
  ArgumentParser(
    const vector<Usage> usages,
    const int argc,
    const char* const argv[],
    const PositionalArgumentsRequired require_positional = PositionalArgumentsRequired::Required
  );
  // HACK: copying is breaking current argument tracking
  ArgumentParser(const ArgumentParser& rhs) = delete;
  ArgumentParser(ArgumentParser& rhs) = delete;
  ArgumentParser& operator=(const ArgumentParser& rhs) = delete;
  ArgumentParser& operator=(ArgumentParser& rhs) = delete;
  /**
   * \brief Parse arguments that were given to constructor
   * \return string Positional arguments
   */
  virtual void parse_args();
  /**
   * \brief If any more positional arguments are available
   */
  bool has_positional() const;
  /**
   * \brief Get next positional argument
   */
  string get_positional();
  /**
   * \brief Indicate positional arguments should all be used by now and error if not
   */
  string cur_arg();
  string get_args();
  string get_arg() noexcept;
  void done_positional();
  bool help_requested() { return help_requested_; }
  void mark_parsed(const string arg);
  bool was_parsed(const string arg);
  void show_args();
  void log_args();
  void show_usage_and_exit(int exit_code);
  void show_usage_and_exit();
  void show_help_and_exit();
  size_t argc() const { return arguments_.size(); }

protected:
  size_t cur_arg_{0};
  size_t skipped_args_{0};
  vector<string> arguments_{};
  string binary_directory_{};
  string binary_name_{};
};
enum MODE
{
  SIMULATION,
  TEST,
  SURFACE
};
class SettingsArgumentParser : public ArgumentParser
{
public:
  using ArgumentParser::ArgumentParser;
  void parse_args() override;
};
class MainArgumentParser : public SettingsArgumentParser
{
public:
  MODE mode{SIMULATION};
  string output_directory{};
  string wx_file_name;
  string log_file_name = "firestarr.log";
  string log_file;
  // HACK: should be output_directory, but if log_file_name starts with '/' it could be anything
  string log_directory()
  {
    static const auto d = [&] {
      const auto last_index = log_file.find_last_of('/') + 1;
      return log_file.substr(0, last_index);
    }();
    return d;
  }
  string fuel_name;
  string perim;
  bool test_all = false;
  MathSize hours = INVALID_TIME;
  size_t size = 0;
  // ffmc, dmc, dc are required for simulation & surface mode, so no indication of it not being
  // provided
  Ffmc ffmc{Ffmc::Invalid()};
  Dmc dmc{Dmc::Invalid()};
  Dc dc{Dc::Invalid()};
  MathSize wind_direction = Direction::Invalid().value;
  MathSize wind_speed{Speed::Invalid().value};
  SlopeSize slope = static_cast<SlopeSize>(INVALID_SLOPE);
  AspectSize aspect = static_cast<AspectSize>(INVALID_ASPECT);
  // FIX: need to get rain since noon yesterday to start of this hourly weather
  Precipitation apcp_prev{Precipitation::Zero()};
  MainArgumentParser(const int argc, const char* const argv[]);
  void parse_args() override;
  FwiWeather get_test_weather() const;
  FwiWeather get_yesterday_weather() const;
};
}
#endif
