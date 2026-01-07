/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "fs/ArgumentParser.h"
#include "fs/SimpleFBP.h"
int main(const int argc, const char* const argv[])
{
  constexpr auto fct_main = fs::testing::test_duff;
  static const fs::Usage USAGE_TEST{"Run tests and exit", ""};
  fs::SettingsArgumentParser parser{
    USAGE_TEST, argc, argv, fs::PositionalArgumentsRequired::NotRequired
  };
  parser.parse_args();
  exit(fct_main(argc, argv));
}
