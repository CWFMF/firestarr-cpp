/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "FWI.h"
#include "Log.h"
#include "Weather.h"
#ifndef FS_ARGUMENT_PARSER_H
#define FS_ARGUMENT_PARSER_H
namespace fs
{
using fs::logging::Log;
const char* cur_arg();
string get_args();
void show_args();
void log_args();
void show_usage_and_exit(int exit_code);
void show_usage_and_exit();
void show_help_and_exit();
const char* get_arg() noexcept;
void mark_parsed(const char* arg);
bool was_parsed(const char* arg);
template <class T>
T parse(std::function<T()> fct)
{
  mark_parsed(cur_arg());
  return fct();
}
template <class T>
T parse_once(std::function<T()> fct)
{
  if (was_parsed(cur_arg()))
  {
    printf("\nArgument %s already specified\n\n", cur_arg());
    show_usage_and_exit();
  }
  return parse(fct);
}
bool parse_flag(bool not_inverse);
template <class T>
T parse_value()
{
  return parse_once<T>([] { return stod(get_arg()); });
}
size_t parse_size_t();
const char* parse_raw();
string parse_string();
template <class T>
T parse_index()
{
  return parse_once<T>([] { return T(stod(get_arg())); });
}
void register_argument(string v, string help, bool required, std::function<void()> fct);
template <class T>
void register_setter(
  std::function<void(T)> fct_set,
  string v,
  string help,
  bool required,
  std::function<T()> fct
)
{
  register_argument(v, help, required, [=] { fct_set(fct()); });
}
template <class T>
void register_setter(T& variable, string v, string help, bool required, std::function<T()> fct)
{
  register_argument(v, help, required, [&variable, fct] { variable = fct(); });
}
void register_flag(std::function<void(bool)> fct, bool not_inverse, string v, string help);
void register_flag(bool& variable, bool not_inverse, string v, string help);
template <class T>
void register_index(T& index, string v, string help, bool required)
{
  register_argument(v, help, required, [&] { index = parse_index<T>(); });
}
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
  size_t cur_arg_{0};
  PositionalArgumentsRequired require_positional_{};

public:
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
  void done_positional();
};
enum MODE
{
  SIMULATION,
  TEST,
  SURFACE
};
class MainArgumentParser : public ArgumentParser
{
public:
  MODE mode{SIMULATION};
  string output_directory{};
  string wx_file_name;
  string log_file_name = "firestarr.log";
  string log_file;
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
