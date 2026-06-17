# delegate

A small, header-only C++ delegate: a non-owning, type-safe, zero-allocation handle to a bound member
function or a free function. One pointer to the object, one pointer to a generated call stub — no heap,
no `std::function`, no virtual dispatch. The target is fixed at compile time, so the call is a single
indirect jump.

Use it where you'd reach for `std::function` but the callable is a known method or free function and you
want a fixed-size, allocation-free callback — event loops, dispatchers, observer hooks on hot paths.

## Requirements

- A C++20 compiler (uses concepts; tested on GCC 14+, Clang 18+, AppleClang 16+).
- Header-only — nothing to build to use it.

## Usage

```cpp
#include <delegate/delegate.h>

struct Button {
    void OnClick(int x, int y) { /* ... */ }
};

int Add(int a, int b) { return a + b; }

// Bind a member function to an instance.
Button button;
delegate::Delegate<void(int, int)> on_click = delegate::CreateDelegate<&Button::OnClick>(&button);
on_click(10, 20);

// Bind a free (or static) function — no object.
auto add = delegate::CreateDelegate<&Add>();
int sum = add(2, 3);  // 5

// Default-constructed delegates bind nothing; check before calling.
delegate::Delegate<int(int, int)> later;
if (later.IsValid()) later(1, 2);
later = delegate::CreateDelegate<&Add>();  // bind later
```

`CreateDelegate` deduces the signature from the bound target, so the `Delegate<...>` type is only spelled
out when you store one. `const`/`volatile`/`noexcept`-qualified methods and methods inherited from a base
class are all bindable.

### A note on argument forwarding

The signature `Delegate<R(Args...)>` *is* the call boundary: arguments are passed by `Args...`, not
perfect-forwarded through. When zero-copy matters, put the reference in the signature
(`Delegate<void(const Big&)>`), exactly as you would for a normal function declaration.

## Consuming it

### CMake — FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(delegate
    GIT_REPOSITORY https://github.com/sbogomolov/delegate.git
    GIT_TAG main  # pin to a release tag in production
)
FetchContent_MakeAvailable(delegate)

target_link_libraries(your_target PRIVATE delegate::delegate)
```

### CMake — installed package

```sh
cmake -S . -B build
cmake --install build --prefix /your/prefix
```

```cmake
find_package(delegate REQUIRED)
target_link_libraries(your_target PRIVATE delegate::delegate)
```

### Or just copy the header

`include/delegate/delegate.h` is self-contained — drop it into your include path.

## Build & test

```sh
./cmake_gen.sh           # Debug, ASan + UBSan on
./cmake_gen.sh Release   # unsanitized
cmake --build build
ctest --test-dir build --output-on-failure
```

The test suite is fetched against GoogleTest and built only when `delegate` is the top-level project, so
consumers pulling it in via FetchContent don't build it.

## License

[Apache License 2.0](LICENSE).
