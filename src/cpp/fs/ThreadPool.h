/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
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
  using input_type = std::tuple<CellPoints, OffsetSet, DurationSize>;
  using promise_type = std::promise<CellPointsMap>;
  using future_type = std::future<CellPointsMap>;

private:
  mutable mutex mutex_{};
  list<std::pair<promise_type, input_type>> jobs_{};
  std::vector<std::thread> threads_{};
  size_t num_threads_{};
  bool should_stop{false};

public:
  ~SpreadThreadPool() noexcept
  {
    should_stop = true;
    for (auto& thread : threads_)
    {
      thread.join();
    }
  }
  SpreadThreadPool(const size_t num_threads = 0) noexcept
    : num_threads_(0 == num_threads ? std::thread::hardware_concurrency() : num_threads)
  {
    for (size_t i = 0; i < num_threads_; ++i)
    {
      threads_.emplace_back([&]() {
#ifdef DEBUG_THREADS
        const auto n = i;
#endif
        // FIX: not interrupting task (but could?)
        input_type input;
        promise_type prom;
        while (!should_stop)
        {
          bool should_wait{true};
          // waiting to get lock for job
          {
            lock_guard<mutex> lock{mutex_};
            if (!jobs_.empty())
            {
              should_wait = false;
              // get work
              auto& pf = jobs_.front();
              prom = std::move(pf.first);
              input = std::move(pf.second);
              jobs_.pop_front();
            }
          }
          if (should_wait)
          {
            std::this_thread::sleep_for(1ns);
          }
          else
          {
            CellPointsMap r1{};
            auto& [cell_pts, offsets_after_duration, arrival_time] = input;
            // done with list so don't need mutex
            auto pt_dirs = cell_pts.point_directions();
            std::sort(pt_dirs.begin(), pt_dirs.end());
            const auto it_pt_dirs_last = std::unique(pt_dirs.begin(), pt_dirs.end());
            auto it_pt_dirs = pt_dirs.cbegin();
            while (it_pt_dirs != it_pt_dirs_last)
            {
              const auto& [pt, dir] = *it_pt_dirs;
              for (const ROSOffset& r : offsets_after_duration)
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
          }
#ifdef DEBUG_THREADS
          logging::extensive("Thread {:03d} looping", n);
#endif
        }
      });
    }
  }
  future_type spread(
    CellPoints&& cell_pts,
    const OffsetSet offsets,
    const DurationSize arrival_time
  ) noexcept
  {
    lock_guard<mutex> lock{mutex_};
    jobs_.emplace_back(
      promise_type{}, input_type{std::move(cell_pts), std::move(offsets), arrival_time}
    );
    return jobs_.back().first.get_future();
  }
  void stop() noexcept { should_stop = true; }
};
}
