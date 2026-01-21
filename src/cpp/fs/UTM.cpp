/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "UTM.h"
#include <proj.h>
#include "Log.h"
#include "Point.h"
#include "unstable.h"
#include "Util.h"
namespace fs
{
class Point;
PJ* normalized_context(PJ_CONTEXT* C, const string_view proj4_from, const string_view proj4_to)
{
  // FIX: this is WGS84, but do we need to support more than that for lat/long
  PJ* P = proj_create_crs_to_crs(C, string(proj4_from).c_str(), string(proj4_to).c_str(), nullptr);
  fs::logging::check_fatal(nullptr == P, "Failed to create transformation object");
  // This will ensure that the order of coordinates for the input CRS
  // will be longitude, latitude, whereas EPSG:4326 mandates latitude,
  // longitude
  PJ* P_norm = proj_normalize_for_visualization(C, P);
  fs::logging::check_fatal(nullptr == P_norm, "Failed to normalize transformation object");
  proj_destroy(P);
  return P_norm;
}
void from_lat_long(const string_view proj4, const fs::Point& point, MathSize* x, MathSize* y)
{
  // see https://proj.org/en/stable/development/quickstart.html
  // do this in a function so we can hide and clean up intial context
  PJ_CONTEXT* C = proj_context_create();
  auto P = normalized_context(C, "EPSG:4326", proj4);
  // Given that we have used proj_normalize_for_visualization(), the order
  // of coordinates is longitude, latitude, and values are expressed in
  // degrees.
  const PJ_COORD a = proj_coord(point.longitude(), point.latitude(), 0, 0);
  // transform to UTM, then back to geographical
  const PJ_COORD b = proj_trans(P, PJ_FWD, a);
  *x = b.enu.e;
  *y = b.enu.n;
  // #ifdef DEBUG_PROJ
  PJ_COORD c = proj_trans(P, PJ_INV, b);
  fs::logging::verbose(
    "longitude: %f, latitude: %f => easting: %.3f, northing: %.3f => x: %.3f, y: %.3f => longitude: %g, latitude: %g",
    point.longitude(),
    point.latitude(),
    b.enu.e,
    b.enu.n,
    b.xy.x,
    b.xy.y,
    c.lp.lam,
    c.lp.phi
  );
  // #endif
  proj_destroy(P);
  proj_context_destroy(C);
}
fs::Point to_lat_long(const string_view proj4, const MathSize x, const MathSize y)
{
  // see https://proj.org/en/stable/development/quickstart.html
  // do this in a function so we can hide and clean up intial context
  PJ_CONTEXT* C = proj_context_create();
  auto P = normalized_context(C, proj4, "EPSG:4326");
  // Given that we have used proj_normalize_for_visualization(), the order
  // of coordinates is longitude, latitude, and values are expressed in
  // degrees.
  logging::verbose("proj_coord(%f, %f, 0, 0)", x, y);
  const PJ_COORD a = proj_coord(x, y, 0, 0);
  // transform to  geographical
  const PJ_COORD b = proj_trans(P, PJ_FWD, a);
  // Point is (lat, lon)
  Point point{b.lp.phi, b.lp.lam};
  proj_destroy(P);
  proj_context_destroy(C);
  return point;
}
string try_fix_meridian(const string_view proj4)
{
  const auto proj = find_value("+proj=", proj4);
  if (0 != strcmp("tmerc", proj.c_str()) && 0 != strcmp("utm", proj.c_str()))
  {
    logging::debug("try_fix_meridian() has no effect on non utm/tmerc projections");
    return string(proj4);
  }
  const auto [lat_0, lon_0, k, x_0, y_0] =
    [&]() -> std::tuple<MathSize, MathSize, MathSize, MathSize, MathSize> {
    if (0 == strcmp("utm", proj.c_str()))
    {
      const auto zone = find_value("+zone=", proj4);
      // zone 15 is -93 and other zones are 6 degrees difference
      const auto z = stod(zone);
      const auto lon_0 = fs::utm_central_meridian(z);
      logging::note("UTM zone %s == %f turned into meridian %f", zone.c_str(), z, lon_0);
      logging::debug("Using default values for utm");
      return {0.0, lon_0, 0.9996, 500000.0, 0.0};
    }
    logging::debug("Using existing values for tmerc");
    const auto lat_0 = find_value("+lat_0=", proj4);
    const auto lon_0 = find_value("+lon_0=", proj4);
    const auto k = find_value("+k=", proj4);
    const auto x_0 = find_value("+x_0=", proj4);
    const auto y_0 = find_value("+y_0=", proj4);
    return {stod(lat_0), stod(lon_0), stod(k), stod(x_0), stod(y_0)};
  }();
  const auto ellps = find_value("+ellps=", proj4, "GRS80");
  const auto units = find_value("+units=", proj4, "m");
  return std::format(
    "+proj={} +lat_0={:0.9f} +lon_0={:0.9f} +k={:0.9f} +x_0={:0.9f} +y_0={:0.9f} +ellps={} +units={}",
    "tmerc",
    lat_0,
    lon_0,
    k,
    x_0,
    y_0,
    ellps,
    units
  );
}
}
