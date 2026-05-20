/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_PROJECT_H
#define FS_PROJECT_H
#include "stdafx.h"
#include "Point.h"
namespace fs
{
using fs::FullCoordinates;
using fs::MathSize;
std::optional<FullCoordinates> to_proj4(
  const string& proj4,
  const fs::Point& point,
  MathSize* x,
  MathSize* y
);
}
#endif
