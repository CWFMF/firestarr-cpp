/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_FUELLOOKUP_H
#define FS_FUELLOOKUP_H
// change imports in here instead of changing which files get imported to use FBP
#include "SimpleFuelLookup.h"
namespace fs::fuel
{
using LazyFuelLookup = fs::simplefbp::LazySimpleFuelLookup;
using FuelLookup = fs::simplefbp::SimpleFuelLookup;
using fs::simplefbp::check_fuel;
using fs::simplefbp::fuel_by_code;
using fs::simplefbp::is_null_fuel;
}
#endif
