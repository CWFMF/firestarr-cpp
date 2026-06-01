/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include "CellPoints.h"
#include "FireSpread.h"
#ifdef DEBUG_THREADS
#include "Log.h"
#endif
#define THREAD_STATS 1
#ifdef THREAD_STATS
#include "Statistics.h"
#endif
namespace fs
{
static CellPointsMap spread_points(
  const CellPoints& cell_pts,
  const OffsetSet& offsets_after_duration,
  const DurationSize arrival_time
) noexcept
{
  CellPointsMap r1{};
  // done with list so don't need mutex
  auto pt_dirs = cell_pts.point_directions();
  auto it_pt_dirs = pt_dirs.cbegin();
  while (it_pt_dirs != pt_dirs.cend())
  {
    const auto& [pt, dir] = *it_pt_dirs;
    for (const ROSOffset& r : offsets_after_duration)
    {
      const auto& x_o = r.offset.x;
      const auto& y_o = r.offset.y;
      const XYPos pt_new{XPos{x_o + pt.x.value}, YPos{y_o + pt.y.value}};
      std::ignore = r1.insert(
        pt, SpreadData{arrival_time, r.intensity, r.ros, r.raz, Direction{Degrees{dir}}}, pt_new
      );
    }
    ++it_pt_dirs;
  }
  return r1;
}
template <class InputType, class ReturnType>
class ThreadPool
{
public:
  // get working with no return values first
  using input_type = InputType;
  using return_type = ReturnType;
  using promise_type = std::promise<ReturnType>;
  using future_type = std::future<ReturnType>;

private:
#ifdef THREAD_STATS
  std::vector<MathSize> sizes_{};
  std::vector<MathSize> active_{};
  atomic<size_t> active_threads_{0};
#endif
  mutable std::condition_variable cv_;
  mutable mutex mutex_{};
  mutable mutex mutex_check{};
  using job_queue = std::queue<std::pair<promise_type, input_type>>;
  job_queue jobs_{};
  std::vector<std::jthread> threads_{};
  size_t num_threads_{};
  bool should_stop{false};
  string name_{};

public:
  virtual ~ThreadPool() noexcept
  {
    should_stop = true;
    std::unique_lock<mutex> lock{mutex_};
    lock.unlock();
    cv_.notify_all();
#ifdef THREAD_STATS
    std::ranges::sort(sizes_);
    Statistics s{sizes_};
    logging::note(
      "ThreadPool for {} queue stats: {:d} calls - {:0.1f} - {:0.1f} (mean {:0.1f}, median {:0.1f})",
      name_,
      sizes_.size(),
      s.min(),
      s.max(),
      s.mean(),
      s.median()
    );
    std::ranges::sort(active_);
    Statistics a{active_};
    logging::note(
      "ThreadPool for {} active stats: {:d} calls - {:0.1f} - {:0.1f} (mean {:0.1f}, median {:0.1f})",
      name_,
      active_.size(),
      a.min(),
      a.max(),
      a.mean(),
      a.median()
    );
#endif
  }
  virtual ReturnType process(input_type input) noexcept = 0;
  ThreadPool(string name, const size_t num_threads = 0) noexcept
    : num_threads_{0 == num_threads ? std::thread::hardware_concurrency() : num_threads},
      name_{std::move(name)}
  {
    for (size_t i = 0; i < num_threads_; ++i)
    {
      threads_.emplace_back([&]() {
        while (!should_stop)
        {
          typename job_queue::value_type pf;
          {
            std::unique_lock<mutex> lock{mutex_};
            cv_.wait(lock, [this]() { return should_stop || !jobs_.empty(); });
            if (should_stop)
            {
              return;
            }
#ifdef THREAD_STATS
            ++active_threads_;
            sizes_.emplace_back(jobs_.size());
            active_.emplace_back(active_threads_);
#endif
            pf = std::move(jobs_.front());
            jobs_.pop();
          }
          auto& [prom, input] = pf;
          prom.set_value(process(input));
#ifdef THREAD_STATS
          --active_threads_;
#endif
        }
      });
    }
  }
  future_type schedule(input_type inputs) noexcept
  {
    std::unique_lock<mutex> lock{mutex_};
    jobs_.emplace(promise_type{}, inputs);
    auto result = jobs_.back().first.get_future();
    lock.unlock();
    cv_.notify_one();
    return result;
  }
};
class SpreadThreadPool
  : public ThreadPool<
      std::tuple<const CellPoints*, const OffsetSet*, DurationSize>,
      fs::CellPointsMap>
{
public:
  return_type process(input_type input) noexcept override
  {
    auto [cell_pts, offsets_after_duration, arrival_time] = input;
    return spread_points(*cell_pts, *offsets_after_duration, arrival_time);
  }
  ~SpreadThreadPool() noexcept override = default;
  SpreadThreadPool(const size_t num_threads = 0) noexcept : ThreadPool("spread", num_threads) { }
  future_type spread(
    const CellPoints* cell_pts,
    const OffsetSet* offsets,
    const DurationSize arrival_time
  ) noexcept
  {
    return schedule(input_type{cell_pts, offsets, arrival_time});
  }
};
}
