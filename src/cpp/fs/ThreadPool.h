/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include "CellPoints.h"
#include "FireSpread.h"
#ifdef DEBUG_THREADS
#include "Log.h"
#endif
namespace fs
{
class SpreadThreadPool
{
public:
  // get working with no return values first
  using input_type = std::tuple<const CellPoints*, const OffsetSet*, DurationSize>;
  using promise_type = std::promise<CellPointsMap>;
  using future_type = std::future<CellPointsMap>;

private:
  mutable std::condition_variable cv_;
  mutable mutex mutex_{};
  mutable mutex mutex_check{};
  using job_queue = std::queue<std::pair<promise_type, input_type>>;
  job_queue jobs_{};
  std::vector<std::jthread> threads_{};
  size_t num_threads_{};
  bool should_stop{false};

public:
  ~SpreadThreadPool() noexcept
  {
    should_stop = true;
    std::unique_lock<mutex> lock{mutex_};
    lock.unlock();
    cv_.notify_all();
  }
  SpreadThreadPool(const size_t num_threads = 0) noexcept
    : num_threads_(0 == num_threads ? std::thread::hardware_concurrency() : num_threads)
  {
    for (size_t i = 0; i < num_threads_; ++i)
    {
      threads_.emplace_back([&]() {
        while (!should_stop)
        {
          job_queue::value_type pf;
          std::unique_lock<mutex> lock{mutex_};
          cv_.wait(lock, [this]() { return should_stop || !jobs_.empty(); });
          if (should_stop)
          {
            return;
          }
          {
            // std::lock_guard<mutex> lock1{mutex_check};
            // if (jobs_.empty())
            // {
            //   logging::error("jobs should not be empty");
            //   continue;
            // }
            // get work
            pf = std::move(jobs_.front());
            jobs_.pop();
          }
          auto& [prom, input] = pf;
          // if (!prom.get_future().valid())
          // {
          //   logging::error("invalid promise");
          //   continue;
          // }
          // logging::note("Jobs waiting: {:d}", jobs_.size());
          CellPointsMap r1{};
          auto& [cell_pts, offsets_after_duration, arrival_time] = input;
          // done with list so don't need mutex
          auto pt_dirs = cell_pts->point_directions();
          std::sort(pt_dirs.begin(), pt_dirs.end());
          const auto it_pt_dirs_last = std::unique(pt_dirs.begin(), pt_dirs.end());
          auto it_pt_dirs = pt_dirs.cbegin();
          while (it_pt_dirs != it_pt_dirs_last)
          {
            const auto& [pt, dir] = *it_pt_dirs;
            for (const ROSOffset& r : *offsets_after_duration)
            {
              const auto& x_o = r.offset.x;
              const auto& y_o = r.offset.y;
              const XYPos pt_new{XPos{x_o + pt.x.value}, YPos{y_o + pt.y.value}};
              std::ignore = insert(
                r1,
                pt,
                SpreadData{arrival_time, r.intensity, r.ros, r.raz, Direction{Degrees{dir}}},
                pt_new
              );
            }
            ++it_pt_dirs;
          }
          // indicate that it's done
          prom.set_value(r1);
#ifdef DEBUG_THREADS
          logging::extensive("Thread {:03d} looping", n);
#endif
        }
      });
    }
  }
  future_type spread(
    const CellPoints* cell_pts,
    const OffsetSet* offsets,
    const DurationSize arrival_time
  ) noexcept
  {
    std::unique_lock<mutex> lock{mutex_};
    jobs_.emplace(promise_type{}, input_type{cell_pts, offsets, arrival_time});
    auto result = jobs_.back().first.get_future();
    lock.unlock();
    cv_.notify_one();
    return result;
  }
};
}
