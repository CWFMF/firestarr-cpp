/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include "CellPoints.h"
#include "FireSpread.h"
#include "PointSpread.h"
#ifdef DEBUG_THREADS
#include "Log.h"
#include "Statistics.h"
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
#ifdef DEBUG_THREADS
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

public:
  ~SpreadThreadPool() noexcept
  {
    should_stop = true;
    std::unique_lock<mutex> lock{mutex_};
    lock.unlock();
    cv_.notify_all();
#ifdef DEBUG_THREADS
    std::ranges::sort(sizes_);
    Statistics s{sizes_};
    logging::note(
      "ThreadPool queue stats: {:d} calls - {:0.1f} - {:0.1f} (mean {:0.1f}, median {:0.1f})",
      sizes_.size(),
      s.min(),
      s.max(),
      s.mean(),
      s.median()
    );
    std::ranges::sort(active_);
    Statistics a{active_};
    logging::note(
      "ThreadPool active stats: {:d} calls - {:0.1f} - {:0.1f} (mean {:0.1f}, median {:0.1f})",
      active_.size(),
      a.min(),
      a.max(),
      a.mean(),
      a.median()
    );
#endif
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
          {
            std::unique_lock<mutex> lock{mutex_};
            cv_.wait(lock, [this]() { return should_stop || !jobs_.empty(); });
            if (should_stop)
            {
              return;
            }
#ifdef DEBUG_THREADS
            ++active_threads_;
            sizes_.emplace_back(jobs_.size());
            active_.emplace_back(active_threads_);
#endif
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
          auto& [cell_pts, offsets_after_duration, arrival_time] = input;
          // if (!prom.get_future().valid())
          // {
          //   logging::error("invalid promise");
          //   continue;
          // }
          // logging::note("Jobs waiting: {:d}", jobs_.size());
          // indicate that it's done
          prom.set_value(spread_points(*cell_pts, *offsets_after_duration, arrival_time));
#ifdef DEBUG_THREADS
          --active_threads_;
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
