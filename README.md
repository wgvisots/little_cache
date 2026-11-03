# little_cache

A header-only, thread-safe recycler for objects that are expensive to construct, such as large buffers.

`little_cache<T>::get()` returns a `std::shared_ptr<T>`. When the last copy of that pointer goes away, the object is
put back in the cache instead of being deleted, and a later `get()` hands it out again. Recycled objects keep whatever
state they had, so reset them before use, and don't leave anything sensitive in them.

```cpp
#include <little_cache/little_cache.h>

std::shared_ptr<std::vector<char>> buffer = little_cache<std::vector<char>>::get();
buffer->resize(messageSize); // may already hold an old message: always size and fill it yourself

// Or have every returned buffer emptied, so each get() starts from an empty vector:
std::shared_ptr<std::vector<char>> clean = little_cache<std::vector<char>, 0, little_cache_clear>::get();
```

- One cache per type. The second template parameter makes independent caches of the same type:
  `little_cache<std::vector<char>, 1>`.
- The third template parameter is a reset policy, applied to every object as it comes back:
  - `little_cache_keep` (the default) leaves it as it was.
  - `little_cache_clear` calls `clear()`, so it comes back empty but keeps its memory (a vector keeps its
    capacity). The old contents are not erased from that memory, only made invisible.
  - Your own: any default-constructible type with a `noexcept` `void operator()(T&) const`.

  Caches with different policies are independent caches. To choose a policy you also have to give the
  second parameter: `little_cache<std::vector<char>, 1, little_cache_clear>`.
- At most `MAX_CACHED_ITEMS` (1024) objects are kept at rest per cache; any more are deleted.
- `purge()` destroys the objects at rest. `numCachedItems()` counts them.
- Every function is safe to call from any number of threads at once.

## Using it

Install it (`cmake -B build && cmake --install build`, or the Debian package from `cpack`), then:

```cmake
find_package(little_cache 1.1 REQUIRED)
target_link_libraries(your_target PRIVATE little_cache::little_cache)
```

## Testing

```sh
cmake -B build && cmake --build build && ctest --test-dir build
cmake -B build-tsan -DLITTLE_CACHE_SANITIZER=thread && cmake --build build-tsan
setarch "$(uname -m)" -R ./build-tsan/test/runUnitTests   # see below
```

`LITTLE_CACHE_SANITIZER` also takes `address,undefined`, and `LITTLE_CACHE_COVERAGE=ON` instruments the tests for
coverage. On recent Linux kernels, ThreadSanitizer stops with "unexpected memory mapping" unless address-space
randomisation is turned off for the process, which is what `setarch -R` does.
