/* Copyright (c) Queen's Printer for Ontario, 2020. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <odbcinst.h>
#include <odbcinstext.h>
namespace tbd::util
{
/**
 * \brief Calculate tm fields from values already there
 * @param t tm object to update
 */
void fix_tm(tm* t);
}
