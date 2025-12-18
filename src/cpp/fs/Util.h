/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_UTIL_H
#define FS_UTIL_H
#include "stdafx.h"
#include "StrictType.h"
namespace fs
{
struct Radians;
struct Degrees : public StrictType<Degrees, units::CompassDegrees>
{
  using StrictType::StrictType;
  explicit constexpr Degrees(const DirectionSize degrees) noexcept
    : Degrees{static_cast<MathSize>(degrees)}
  { }
};
struct Radians : public StrictType<Radians, units::CompassRadians>
{
  using StrictType::StrictType;
  static consteval Radians Pi() { return Radians{M_PI}; };
  static consteval Radians PiX2() { return Radians{2 * M_PI}; };
  static consteval Radians D_360() { return Radians{Degrees{static_cast<AspectSize>(360)}}; };
  static consteval Radians D_270() { return Radians{Degrees{static_cast<AspectSize>(270)}}; };
  static consteval Radians D_180() { return Radians{Degrees{static_cast<AspectSize>(180)}}; };
  static consteval Radians D_090() { return Radians{Degrees{static_cast<AspectSize>(90)}}; };
  explicit constexpr Radians(const Degrees& degrees) noexcept
    : Radians{degrees.value / M_RADIANS_TO_DEGREES}
  { }
  explicit constexpr Radians(const AspectSize aspect) noexcept : Radians{Degrees{aspect}} { }
  [[nodiscard]] constexpr Degrees asDegrees() const
  {
    return Degrees{value * M_RADIANS_TO_DEGREES};
  }
  [[nodiscard]] static constexpr Radians fix(const Radians& radians)
  {
    if (radians > PiX2())
    {
      return radians - PiX2();
    }
    if (radians < Radians::Zero())
    {
      return radians + PiX2();
    }
    return radians;
  }
  /**
   * \brief Ensure that value lies between 0 and 2 * PI
   * \param theta value to ensure is within bounds
   * \return value within range of (0, 2 * PI]
   */
  [[nodiscard]] constexpr Radians fix() const { return Radians::fix(*this); }
  /**
   * \brief Convert Bearing to Heading (opposite angle)
   * \param azimuth Bearing
   * \return Heading
   */
  [[nodiscard]] constexpr Radians to_heading() const { return (*this + D_180()).fix(); }
};
static constexpr Degrees INVALID_DIRECTION{std::numeric_limits<DirectionSize>::max()};
// static constexpr MathSize to_radians(const MathSize v) { return Radians{Degrees{v}}.value; };
// static constexpr MathSize to_degrees(const MathSize v) { return Degrees{Radians{v}}.value; };
// static constexpr MathSize fix_radians(const MathSize v) { return Radians{v}.fix().value; };
static constexpr Radians abs(const Radians& radians) { return Radians{radians.value}; };
static constexpr MathSize tan(const Radians& radians) { return std::tan(radians.value); };
static constexpr MathSize sin(const Radians& radians) { return fs::sin(radians.value); };
static constexpr MathSize cos(const Radians& radians) { return fs::cos(radians.value); };
// static constexpr MathSize to_degrees(const MathSize v) { return Degrees{Radians{v}}.value; };
// static constexpr MathSize fix_radians(const MathSize v) { return Radians{v}.fix().value; };
#ifndef TIFFTAG_GDAL_NODATA
#define TIFFTAG_GDAL_NODATA 42113
#endif
constexpr YearSize TM_YEAR_OFFSET = 1900;
constexpr int TM_MONTH_OFFSET = 1;
class FileList : public vector<string>
{
public:
  using vector::vector;
// HACK: this is in C++23 but needs gcc 15
#ifndef __cpp_lib_containers_ranges
  void append_range(const FileList& rhs) noexcept { insert(end(), rhs.cbegin(), rhs.cend()); }
#endif
};
/**
 * Open file and register GeoTIFF tags so we can read and write properly
 * @param filename Name of file to open
 * @param mode Mode to open file with
 * @return Handle to open TIFF with fields registered
 */
TIFF* GeoTiffOpen(const char* const filename, const char* const mode);
/**
 * Call snprintf() but show where and throw exception when anything gets cut off.
 */
int sxprintf(char* buffer, size_t N, const char* format, va_list* args);
int sxprintf(char* buffer, size_t N, const char* format, ...);
/**
 * Call snprintf() but don't need to determine size of array when calling
 * and complain when anything gets cut off.
 */
template <std::size_t N>
int sxprintf(char (&buffer)[N], const char* format, va_list* args)
{
  return sxprintf(&buffer[0], N, format, args);
}
template <std::size_t N>
int sxprintf(char (&buffer)[N], const char* format, ...)
{
  va_list args;
  va_start(args, format);
  auto r = sxprintf(buffer, format, &args);
  va_end(args);
  return r;
}
/**
 * \brief Convert day and hour to DurationSize representing time
 * \tparam T Type used for representing day
 * \param day Day
 * \param hour Hour
 * \return DurationSize representing time at day and hour
 */
template <typename T>
[[nodiscard]] DurationSize to_time(const T day, const int hour) noexcept
{
  return day + hour / (1.0 * DAY_HOURS);
}
/**
 * \brief Convert time index to DurationSize representing time
 * \param t_index index for array of times
 * \return DurationSize representing time at day and hour
 */
[[nodiscard]] constexpr DurationSize to_time(const size_t t_index) noexcept
{
  return static_cast<DurationSize>(t_index) / DAY_HOURS;
}
/**
 * \brief Convert day and hour into time index
 * \tparam T Type used for representing day
 * \param day Day
 * \param hour Hour
 * \return index for array of times
 */
template <typename T>
[[nodiscard]] size_t time_index(const T day, const int hour) noexcept
{
  return static_cast<size_t>(day) * DAY_HOURS + hour;
}
/**
 * \brief Convert day and hour into time index since min_date
 * \param day Day
 * \param hour Hour
 * \param min_date Date at time index 0
 * \return index for array of times
 */
template <typename T>
[[nodiscard]] size_t time_index(const T day, const int hour, const Day min_date) noexcept
{
  return time_index(day, hour) - static_cast<size_t>(DAY_HOURS) * min_date;
}
/**
 * \brief Convert DurationSize into time index
 * \param time time to get time index for
 * \return index for array of times
 */
[[nodiscard]] constexpr size_t time_index(const DurationSize time) noexcept
{
  return static_cast<size_t>(time * DAY_HOURS);
}
/**
 * \brief Convert DurationSize into time index since min_date
 * \param time Time to convert to time index
 * \param min_date Date at time index 0
 * \return index for array of times
 */
[[nodiscard]] constexpr size_t time_index(const DurationSize time, const Day min_date) noexcept
{
  return time_index(time) - static_cast<size_t>(DAY_HOURS) * min_date;
}
/**
 * \brief Return the passed value as given type
 * \tparam T Type to return
 * \param value Value to return
 * \return Value casted to type T
 */
template <class T>
[[nodiscard]] T no_convert(int value, int) noexcept
{
  return static_cast<T>(value);
}
/**
 * \brief Ensure that value lies between 0 and 2 * PI
 * \param theta value to ensure is within bounds
 * \return value within range of (0, 2 * PI]
 */
[[nodiscard]] constexpr MathSize fix_radians(const MathSize theta)
{
  return Radians{theta}.fix().value;
}
/**
 * \brief Convert degrees to radians
 * \param degrees Angle in degrees
 * \return Angle in radians
 */
[[nodiscard]] constexpr MathSize to_radians(const MathSize degrees) noexcept
{
  return Radians{Degrees{degrees}}.value;
}
// only calculate this once and reuse it
/**
 * \brief 360 degrees in radians
 */
static constexpr MathSize RAD_360{Radians::D_360().value};
/**
 * \brief 270 degrees in radians
 */
static constexpr MathSize RAD_270{Radians::D_270().value};
/**
 * \brief 180 degrees in radians
 */
static constexpr MathSize RAD_180{Radians::D_180().value};
/**
 * \brief 90 degrees in radians
 */
static constexpr MathSize RAD_090{Radians::D_090().value};
/**
 * \brief Convert radians to degrees
 * \param radians Value in radians
 * \return Value in degrees
 */
[[nodiscard]] constexpr MathSize to_degrees(const MathSize radians)
{
  return Radians{radians}.asDegrees().value;
}
/**
 * \brief Convert Bearing to Heading (opposite angle)
 * \param azimuth Bearing
 * \return Heading
 */
[[nodiscard]] constexpr MathSize to_heading(const MathSize azimuth)
{
  return Radians{azimuth}.to_heading().value;
}
/**
 * \brief Read from a stream until delimiter is found
 * \tparam Elem Elem for stream
 * \tparam Traits Traits for stream
 * \tparam Alloc Allocator for stream
 * \param stream Stream to read from
 * \param str gstring to read into
 * \param delimiter Delimiter to stop at
 * \return
 */
template <class Elem, class Traits, class Alloc>
std::basic_istream<Elem, Traits>& getline(
  std::basic_istream<Elem, Traits>* stream,
  std::basic_string<Elem, Traits, Alloc>* str,
  const Elem delimiter
)
{
  // make template so we can use pointers directly
  return getline(*stream, *str, delimiter);
}
/**
 * \brief Check if a directory exists
 * \param dir Directory to check existence of
 * \return Whether or not the directory exists
 */
[[nodiscard]] bool directory_exists(const char* dir) noexcept;
/**
 * \brief Check if a file exists
 * \param path File to check existence of
 * \return Whether or not the file exists
 */
[[nodiscard]] bool file_exists(const char* path) noexcept;
/**
 * \brief Get a list of items in the given directory matching the given regex
 * \param name Directory to search
 * \param match regular expression to match in names
 * \param for_files Match files and not directories
 * \return FileList of rasters in the directory
 */
[[nodiscard]] FileList read_directory(
  const string_view name,
  const string_view match = "*",
  const bool for_files = true
);
/**
 * \brief Get a list of rasters in the given directory for the specified year
 * \param dir Root directory to look for rasters in
 * \param year Year to use rasters for if available, else default
 * \return FileList of rasters in the directory
 */
[[nodiscard]] FileList find_rasters(const string_view dir, YearSize year);
/**
 * \brief Make the given directory if it does not exist
 * \param dir Directory to create
 */
void make_directory(const char* dir) noexcept;
/**
 * \brief Make the given directory and any parent directories that do not exist
 * \param dir Directory to create
 */
void make_directory_recursive(const char* dir) noexcept;
/**
 * \brief Square a number
 * \param x number to square
 * \return x squared
 */
template <class T>
[[nodiscard]] inline constexpr MathSize sq(const T& x)
{
  return static_cast<MathSize>(x) * static_cast<MathSize>(x);
}
/**
 * \brief Return base raised to the power N
 * \tparam N Power to raise to
 * \tparam T Type passed and returned
 * \param base Base to raise to power N
 * \return base raised to power N
 */
template <unsigned int N, class T>
[[nodiscard]] constexpr T pow_int(const T& base)
{
  // https://stackoverflow.com/questions/16443682/c-power-of-integer-template-meta-programming
  return N == 0     ? 1
       : N % 2 == 0 ? pow_int<N / 2, T>(base) * pow_int<N / 2, T>(base)
                    : base * pow_int<(N - 1) / 2, T>(base) * pow_int<(N - 1) / 2, T>(base);
}
/**
 * \brief Makes a bit mask of all 1's the specified size
 * \tparam N Number of bits
 * \tparam T Type to return
 * \return Bit mask of all 1's of specified size
 */
template <unsigned int N, class T>
[[nodiscard]] constexpr T bit_mask()
{
  return static_cast<T>(pow_int<N, int64_t>(2) - 1);
}
/**
 * \brief Round value to specified precision
 * \tparam N Precision to round to
 * \param value Value to round
 * \return Value after rounding
 */
template <unsigned int N>
[[nodiscard]] MathSize round_to_precision(const MathSize value) noexcept
{
  // HACK: this can't actually make the value be the precision we want due to
  // floating point storage, but we can round it to what it would be if it were
  // that precision
  static const auto b = pow_int<N, int64_t>(10);
  return round(value * b) / b;
}
/**
 * \brief Convert date and time into tm struct
 * \param year year
 * \param month month
 * \param day day of month
 * \param hour hour
 * \param minute minute
 * \return tm representing date and time
 */
tm to_tm(const YearSize year, const int month, const int day, const int hour, const int minute);
/**
 * \brief Convert tm struct into internal represenation
 * \param t tm struct to convert
 * \return internal time representation
 */
DurationSize to_time(const tm& t);
/**
 * \brief Convert date and time into internal represenation
 * \param year year
 * \param month month
 * \param day day of month
 * \param hour hour
 * \param minute minute
 * \return internal time representation
 */
DurationSize to_time(
  const YearSize year,
  const int month,
  const int day,
  const int hour,
  const int minute
);
/**
 * \brief Read a date from the given stream
 * \param iss Stream to read from
 * \param str string to read date into
 * \param t tm to parse date into
 */
void read_date(istringstream* iss, string* str, tm* t);
/**
 * \brief Provides the ability to determine how many times something is used during a simulation.
 */
class UsageCount
{
  /**
   * \brief How many times this has been used
   */
  atomic<size_t> count_;
  /**
   * \brief What this is tracking usage of
   */
  string for_what_;

public:
  ~UsageCount();
  /**
   * \brief Constructor
   * \param for_what What this is tracking usage of
   */
  explicit UsageCount(string for_what) noexcept;
  UsageCount(UsageCount&& rhs) noexcept = delete;
  UsageCount(const UsageCount& rhs) noexcept = delete;
  UsageCount& operator=(UsageCount&& rhs) noexcept = delete;
  UsageCount& operator=(const UsageCount& rhs) noexcept = delete;
  /**
   * \brief Increment operator
   * \return Value after increment
   */
  UsageCount& operator++() noexcept;
};
// https://stackoverflow.com/questions/15843525/how-do-you-insert-the-value-in-a-sorted-vector
/**
 * \brief Insert value into vector in sorted order even if it is already present
 * \tparam T Type of value to insert
 * \param vec vector to insert into
 * \param item Item to insert
 * \return iterator result of insert
 */
template <typename T>
[[nodiscard]] typename std::vector<T>::iterator insert_sorted(std::vector<T>* vec, T const& item)
{
  return vec->insert(std::upper_bound(vec->begin(), vec->end(), item), item);
}
/**
 * \brief Insert value into vector in sorted order if it does not already exist
 * \tparam T Type of value to insert
 * \param vec vector to insert into
 * \param item Item to insert
 */
template <typename T>
void insert_unique(std::vector<T>* vec, T const& item)
{
  const auto i = std::lower_bound(vec->begin(), vec->end(), item);
  if (i == vec->end() || *i != item)
  {
    vec->insert(i, item);
  }
}
/**
 * \brief Do binary search over function for T that results in value
 * \tparam T Type of input values
 * \tparam V Result type of fct
 * \param lower Lower bound
 * \param upper Upper bound
 * \param value Value to look for
 * \param fct Function taking T and returning V
 * \return T value that results in closest to value
 * \return
 */
template <typename T, typename V>
T binary_find(const T lower, const T upper, const V value, const std::function<V(T)>& fct)
{
  const auto mid = lower + (upper - lower) / 2;
  if (lower == upper)
  {
    return lower;
  }
  if (fct(mid) < value)
  {
    return binary_find(lower, mid, value, fct);
  }
  return binary_find(mid + 1, upper, value, fct);
}
/**
 * \brief Check lower and upper limits before doing binary search over function for T that results
 * in value
 * \tparam T Type of input values
 * \tparam V Result type of fct
 * \param lower Lower bound to check
 * \param upper Upper bound to check
 * \param value Value to look for
 * \param fct Function taking T and returning V
 * \return T value that results in closest to value
 */
template <typename T, typename V>
T binary_find_checked(const T lower, const T upper, const V value, const std::function<V(T)>& fct)
{
  if (fct(lower) < value)
  {
    return lower;
  }
  if (fct(upper) >= value)
  {
    return upper;
  }
  return binary_find(lower, upper, value, fct);
}
struct string_hash
{
  using is_transparent = void;
  [[nodiscard]] size_t operator()(const char* txt) const
  {
    return std::hash<std::string_view>{}(txt);
  }
  [[nodiscard]] size_t operator()(std::string_view txt) const
  {
    return std::hash<std::string_view>{}(txt);
  }
  [[nodiscard]] size_t operator()(const std::string& txt) const
  {
    return std::hash<std::string>{}(txt);
  }
};
template <class V>
using string_map = unordered_map<string, V, string_hash, std::equal_to<>>;
/**
 * \brief Determine month and day that a day of the year represents
 * \param year Year to determine for
 * \param day_of_year Day of year to determine for
 * \param month Month that was determined
 * \param day_of_month Day of month that was determined
 */
void month_and_day(
  const YearSize year,
  const size_t day_of_year,
  size_t* month,
  size_t* day_of_month
);
/**
 * \brief Determine if year is a leap year
 * \param year Year to determine for
 * \return Whether or not the given year is a leap year
 */
[[nodiscard]] bool is_leap_year(const YearSize year);
/**
 * Make a nicely formatted timestamp string for the given simulation time
 * @param year Year time is for
 * @param time Simulation time (fractional day of year)
 * @return YYYY-mm-dd HH:00 time string for given time
 */
[[nodiscard]] string make_timestamp(const YearSize year, const DurationSize time);
/**
 * Convert circle angle to the angle that would be on an ellipse with
 * given length-to-breadth ratio
 * @param length_to_breadth length-to-breadth ratio
 * @param theta direction to convert to ellipse direction (radians)
 */
[[nodiscard]] inline MathSize ellipse_angle(const MathSize length_to_breadth, const MathSize theta)
{
  return (fix_radians(atan2(sin(theta) / length_to_breadth, cos(theta))));
}
// make a set of shared pointers that compares the underlying objects
template <class T>
struct sp_less
{
  bool operator()(const std::shared_ptr<T>& lhs, const std::shared_ptr<T>& rhs) const
  {
    if (!lhs || !rhs)
    {
      return !lhs && rhs;
    }
    return *lhs < *rhs;
  }
};
template <class T>
class sp_set : public std::set<shared_ptr<T>, sp_less<T>>
{
  using base = std::set<shared_ptr<T>, sp_less<T>>;

public:
  template <class... Args>
  auto sp_emplace(Args&&... args)
  {
    // make it so emplace will work on object type and not shared_ptr
    return base::emplace(new T(std::forward<Args>(args)...));
  }
};
}
#endif
