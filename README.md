# SharedPtr — A Minimal `std::shared_ptr`-like Smart Pointer

This repository demonstrates how to implement a reference-counted smart pointer from scratch and exercise it with a small test program.  It is **header-only**, works with single objects *and* dynamically-allocated arrays, and can be built with an atomic reference counter for thread-safety.

## Directory layout

| File            | Purpose                                                         |
|-----------------|-----------------------------------------------------------------|
| **SharedPtr.h** | Header-only implementation of `SharedPtr<T>` and `SharedPtr<T[]>` |
| **main.cpp**    | Self-contained test-drive that exercises the main API            |

---

## Building & running

You need a C++17-capable compiler (GCC ≥ 7, Clang ≥ 5, MSVC ≥ 19.14, or Apple Clang ≥ 10).

```bash
# Plain (non-atomic) reference counter
$ g++ -std=c++17 -O2 main.cpp -o demo
$ ./demo

# Thread-safe build (atomic counter)
$ g++ -std=c++17 -O2 -DSHPTR_THREADSAFE main.cpp -o demo
$ ./demo
```

> **Tip :** the only difference is the pre-processor flag `-DSHPTR_THREADSAFE`.  When defined, `SharedPtr.h` aliases the counter type to `std::atomic<std::size_t>`; otherwise it uses a plain `std::size_t`.

---

## Example output

```
*** Non‑atomic build ***

--- basic lifecycle ---
Foo(42) constructed
use_count = 1
after copy, use_count = 2
p->value = 99
use_count after q dies = 1
Foo(99) destroyed

--- array demo ---
arr[2] = 3

--- swap and move ---
Foo(1) constructed
Foo(2) constructed
after swap  a->value=2 b->value=1
Foo(7) constructed
m is null, n.use_count=1
Foo(7) destroyed
Foo(1) destroyed
Foo(2) destroyed

All tests finished.
```

---

## What the code does

### `SharedPtr<T>` — single objects

1. **Control block**   Holds a raw pointer and a reference counter.
2. **Reference management**   Copy/assignment increment the counter; destruction or `reset()` decrement it and delete the managed object when the count reaches 0.
3. **Observers / accessors**   `get()`, `use_count()`, `unique()`, `operator*`, `operator->`.
4. **Modifiers**   `reset()`, `swap()`, assignment from raw pointer.
5. **ADL-friendly `swap`**   Non-member overload lives in the same namespace, so generic code can simply call `swap(a, b)`.

### `SharedPtr<T[]>` — dynamic arrays

A partial specialization frees the memory with `delete[]` and provides `operator[](std::size_t)`.

### Thread-safety option

If `SHPTR_THREADSAFE` is defined, the counter type is `std::atomic<std::size_t>`; otherwise it is a plain `std::size_t`.  No other synchronization is provided.

---

## Shared pointers in a nutshell  

> *Why not just use `std::shared_ptr`?* — You should, in production code.  The point here is educational.

| Pros                               | Cons / Caveats                                             |
|------------------------------------|------------------------------------------------------------|
| Automatic lifetime management      | **Overhead**: one heap allocation for control block        |
| Safe copying / assignment           | **Cycles**: reference cycles leak unless `std::weak_ptr`   |
| Works well with STL algorithms      | **Thread contention** if many threads mutate the counter   |
| Can share ownership across modules  | **Not a silver bullet** — still need to avoid *owning* raw pointers |

### Best practices

* Prefer `std::unique_ptr` for exclusive ownership; reach for shared pointers only when you *truly* need shared semantics.
* Break cycles with `std::weak_ptr` (or redesign ownership).
* Avoid mixing raw pointers and owning smart pointers.
* Keep `shared_ptr` instances small and cheap to move (they are, but copies still touch an atomic counter in MT code).

---

## License

This example is released under the MIT License.  Use it freely in your own learning projects.