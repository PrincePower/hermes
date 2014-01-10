// This file is part of HermesCommon.
//
// Hermes2D is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Hermes2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Hermes2D.  If not, see <http://www.gnu.org/licenses/>.

/*! \file memory_handling.h
\brief File containing common definitions, and basic global enums etc. for HermesCommon.
*/
#ifndef __HERMES_COMMON_MEMORY_HANDLING_H
#define __HERMES_COMMON_MEMORY_HANDLING_H

#ifdef WITH_PJLIB
#include "pjlib.h"
#endif

#include "exceptions.h"
#include "api.h"
#include <type_traits>

namespace Hermes
{
#ifdef WITH_PJLIB
  HERMES_COMMON_API extern pj_caching_pool HermesCommonMemoryPoolCache;
  class GlobalPoolCache
  {
  public:
    GlobalPoolCache()
    {
      pj_init();
      pj_caching_pool_init(&HermesCommonMemoryPoolCache, NULL, 1024 * 1024 * 1024);

      this->globalPool = pj_pool_create(&HermesCommonMemoryPoolCache.factory, // the factory
        "pool-Global", // pool's name
        100000000, // initial size
        10000000, // increment size
        NULL);
    };

    inline void* pool_calloc(pj_size_t count, pj_size_t elem)
    {
      return pj_pool_calloc(globalPool, count, elem);
    };

    inline void* pool_alloc(pj_size_t size)
    {
      return pj_pool_alloc(globalPool, size);
    };

    pj_pool_t* globalPool;
  };
  HERMES_COMMON_API extern GlobalPoolCache hermesCommonGlobalPoolCache;
#endif

  template<typename Caller, typename ArrayItem>
  ArrayItem* calloc_with_check(int size, Caller* const caller, bool force_malloc = false)
  {
    if (size == 0)
      return nullptr;
    ArrayItem* new_array;
    if (force_malloc && std::is_pod<ArrayItem>::value)
      new_array = (ArrayItem*)calloc(size, sizeof(ArrayItem));
    else
    {
#ifdef WITH_PJLIB
      new_array = (ArrayItem*)hermesCommonGlobalPoolCache.pool_calloc(size, sizeof(ArrayItem));
#else
      new_array = new ArrayItem[size];
      memset(new_array, 0, sizeof(ArrayItem) * size);
#endif
    }
    if (new_array)
      return new_array;
    else
    {
      caller->free();
      throw Hermes::Exceptions::Exception("Hermes::calloc_with_check() failed to allocate.", size * sizeof(ArrayItem));
      return nullptr;
    }
  }

  template<typename ArrayItem>
  ArrayItem* calloc_with_check(int size, bool force_malloc = false)
  {
    if (size == 0)
      return nullptr;
    ArrayItem* new_array;
    if (force_malloc && std::is_pod<ArrayItem>::value)
      new_array = (ArrayItem*)calloc(size, sizeof(ArrayItem));
    else
    {
#ifdef WITH_PJLIB
      new_array = (ArrayItem*)hermesCommonGlobalPoolCache.pool_calloc(size, sizeof(ArrayItem));
#else
      new_array = new ArrayItem[size];
      memset(new_array, 0, sizeof(ArrayItem) * size);
#endif
    }
    if (new_array)
      return new_array;
    else
    {
      throw Hermes::Exceptions::Exception("Hermes::calloc_with_check() failed to allocate.", size * sizeof(ArrayItem));
      return nullptr;
    }
  }

  template<typename Caller, typename ArrayItem>
  ArrayItem* malloc_with_check(int size, Caller* const caller, bool force_malloc = false)
  {
    if (size == 0)
      return nullptr;
    ArrayItem* new_array;
    if (force_malloc && std::is_pod<ArrayItem>::value)
      new_array = (ArrayItem*)malloc(size * sizeof(ArrayItem));
    else
#ifdef WITH_PJLIB
      new_array = (ArrayItem*)hermesCommonGlobalPoolCache.pool_alloc(size * sizeof(ArrayItem));
#else
      new_array = new ArrayItem[size];
#endif
    if (new_array)
      return new_array;
    else
    {
      if (caller)
        caller->free();
      throw Hermes::Exceptions::Exception("Hermes::malloc_with_check() failed to allocate.", size * sizeof(ArrayItem));
      return nullptr;
    }
  }

  template<typename ArrayItem>
  ArrayItem* malloc_with_check(int size, bool force_malloc = false)
  {
    if (size == 0)
      return nullptr;
    ArrayItem* new_array;
    if (force_malloc && std::is_pod<ArrayItem>::value)
      new_array = (ArrayItem*)malloc(size * sizeof(ArrayItem));
    else
#ifdef WITH_PJLIB
      new_array = (ArrayItem*)hermesCommonGlobalPoolCache.pool_alloc(size * sizeof(ArrayItem));
#else
      new_array = new ArrayItem[size];
#endif
    if (new_array)
      return new_array;
    else
    {
      throw Hermes::Exceptions::Exception("Hermes::malloc_with_check() failed to allocate.", size * sizeof(ArrayItem));
      return nullptr;
    }
  }

  template<typename Caller, typename ArrayItem>
  ArrayItem* realloc_with_check(ArrayItem*& original_array, int new_size, Caller* const caller)
  {
    if (new_size == 0)
      return nullptr;
    static_assert(std::is_pod<ArrayItem>::value, "ArrayItem must be POD for reallocation.");

    ArrayItem* new_array = (ArrayItem*)realloc(original_array, new_size * sizeof(ArrayItem));
    if (new_array)
      return original_array = new_array;
    else
    {
      caller->free();
      throw Hermes::Exceptions::Exception("Hermes::realloc_with_check() failed to reallocate.", new_size * sizeof(ArrayItem));
      return nullptr;
    }
  }

  template<typename ArrayItem>
  void free_with_check(ArrayItem*& ptr, bool force_malloc = false)
  {
    if (ptr)
    {
      if (force_malloc && std::is_pod<ArrayItem>::value)
        ::free(ptr);
      else
#ifndef WITH_PJLIB
        delete[] ptr;
#endif
      ptr = nullptr;
    }
  }
}
#endif
