/* SPDX-FileCopyrightText: 2020-2021 Queen's Printer for Ontario */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_MEMORYPOOL_H
#define FS_MEMORYPOOL_H

namespace fs
{
namespace util
{
/**
 * \brief Keeps track of data structures so they can be reused instead of freeing and reacquiring
 * them.
 * \tparam T Type to keep a pool of
 */
template <class T>
class MemoryPool
{
public:
  ~MemoryPool() = default;
  MemoryPool() = default;
  MemoryPool(const MemoryPool& rhs) = delete;
  MemoryPool(MemoryPool&& rhs) = delete;
  MemoryPool&
  operator=(const MemoryPool& rhs) = delete;
  MemoryPool&
  operator=(MemoryPool&& rhs) = delete;
  /**
   * \brief Acquire a new T either by returning an unused one or allocating a new one
   * \return a T*
   */
  T*
  acquire()
  {
    if (!assets_.empty())
    {
      lock_guard<mutex> lock(mutex_);
      // check again once we have the mutex
      if (!assets_.empty())
      {
        const auto v = std::move(assets_.back()).release();
        assets_.pop_back();
        // this is already reset before it was given back
        return v;
      }
    }
    return new T();
  }
  /**
   * \brief Add a T* to the pool of available T*s
   * \param asset T* to return to pool
   */
  void
  release(
    T* asset
  )
  {
    if (nullptr == asset)
    {
      return;
    }
    lock_guard<mutex> lock(mutex_);
    assets_.push_back(unique_ptr<T>(asset));
  }
private:
  /**
   * \brief Mutex for parallel access
   */
  mutable mutex mutex_{};
  /**
   * \brief Available assets that can be acquired
   */
  vector<unique_ptr<T>> assets_{};
};
/**
 * \brief Call clear on T before releasing it
 * \tparam T Type of item to release
 * \param t Item to release
 * \param pool MemoryPool to release item into
 * \return nullptr
 */
template <typename T>
T*
check_clear(
  T* t,
  MemoryPool<T>& pool
)
{
  if (nullptr != t)
  {
    t->clear();
    pool.release(t);
  }
  return nullptr;
}
/**
 * \brief Call reset on T before releasing it
 * \tparam T Type of item to release
 * \param t Item to release
 * \param pool MemoryPool to release item into
 * \return nullptr
 */
template <typename T>
T*
check_reset(
  T* t,
  MemoryPool<T>& pool
)
{
  if (nullptr != t)
  {
    t->reset();
    pool.release(t);
  }
  return nullptr;
}
}
}
#endif
