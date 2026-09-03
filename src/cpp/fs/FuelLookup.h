/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FUELLOOKUP_H
#define FS_FUELLOOKUP_H
// change imports in here instead of changing which files get imported to use FBP
#include "FuelOldLookup.h"
namespace fs::fuel
{
using LazyFuelLookup = fs::fuelold::LazyFuelOldLookup;
using FuelLookup = fs::fuelold::FuelOldLookup;
using fs::fuelold::check_fuel;
using fs::fuelold::fuel_by_code;
using fs::fuelold::is_null_fuel;
using fs::fuelold::simplify_fuel_name;
}
#endif
