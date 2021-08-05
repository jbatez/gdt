# GDT (Game Dev Templates)

A header-only C++ library meant to complement the Standard Library with a focus
on game development.

## Why?

The C++ Standard Library is... fine. It's massive and overly-complicated in
parts, but it's an integral part of modern C++ for better or worse.

It has some glaring problems for game developers in-particular:

- **Exceptions** - The Standard Library makes liberal use of exceptions, but
  game developers tend to avoid exceptions like the plague.
- **Platform-Specific Implementations** - The C++ Standard leaves a lot of room
  for implementation-specific details, which means the the behavior of something
  as fundamental as, say, `std::vector`, varies between platforms. Games often
  target multiple platforms; having a common, portable implementation of
  something like `std::vector` would be nice.
- **Missing Primitives** - Almost every game needs arithmetic vectors and/or
  matricies at some point. They're as fundamental in game development as
  character strings; maybe even more so.

GDT aims to address these issues and add a few other nice-to-haves here and
there.

## Requirements

GDT several C++20 features. It's been tested with

- VS 2019 v16.10 on Windows
- Clang 12 on macOS

## Usage

GDT is a header-only library. All you need to do is make sure the `include`
directory is in your project's include path somehow.

Instead of exceptions, GDT expects a user-provided implementation of the
`gdt::panic` function. It calls this function any time an exceptional error
occurs, such as when memory allocation fails or when attempting to grow a
container past its maximum size. It's up to the user to decide what to do in
these cases, but GDT considers these errors fatal and assumes the program is
about to exit.

A simple implementation might look like:

```c++
#include <gdt/panic.hxx>

#include <cstdio>
#include <cstdlib>

[[noreturn]] void gdt::panic(
    const char* file,
    unsigned line,
    const char* message)
{
    std::fprintf(stderr, "%s:%u: %s\n", file, line, message);
    std::exit(EXIT_FAILURE);
}
```

A more-complicated game might choose to integrate `gdt::panic` with some kind of
bug reporting tool.

## <gdt/panic.hxx>

```c++
#define gdt_panic() (::gdt::panic(__FILE__, __LINE__, "gdt_panic()"))
```

A macro to call `gdt::panic` with the current file name and line number.

## <gdt/assert.hxx>

```c++
#define gdt_assert(x) static_cast<void>(\
    (x) || (::gdt::panic(__FILE__, __LINE__, "gdt_assert("#x") failed"), 0))
```

Panic if the given condition is false. Unlike `<cassert>`/`<assert.h>`, the
condition is always checked regardless of the `NDEBUG` macro. For something
more like `<cassert>`/`<assert.h>`, consider using `<gdt/assume.hxx>`.

## <gdt/assume.hxx>

```c++
#ifndef NDEBUG
#define gdt_assume(x) static_cast<void>(\
    (x) || (::gdt::panic(__FILE__, __LINE__, "gdt_assume("#x") failed"), 0))
#elif defined(__clang__)
#define gdt_assume(x) __builtin_assume(x)
#elif defined(_MSC_VER)
#define gdt_assume(x) __assume(x)
#else
#define gdt_assume(x) static_cast<void>((x) || (__builtin_unreachable(), 0))
#endif
```

In debug builds (`NDEBUG` is not defined), panics if the given condition is
false. In release builds (`NDEBUG` is defined), provides a hint that the given
condition is always true so the compiler can optimize around the assumption.

## <gdt/unreachable.hxx>

```c++
#ifndef NDEBUG
#define gdt_unreachable() (\
    ::gdt::panic(__FILE__, __LINE__, "gdt_unreachable() reached"))
#elif defined(_MSC_VER) && !defined(__clang__)
#define gdt_unreachable() __assume(0)
#else
#define gdt_unreachable() __builtin_unreachable()
#endif
```

In debug builds (`NDEBUG` is not defined), panics if control flow reaches it.
In release builds (`NDEBUG` is defined), provides a hint that control flow
never reaches it so the compiler can optimize around the assumption.

## <gdt/allocator.hxx>

```c++
namespace gdt
{
    // Allocator.
    template<
        typename T,
        typename SizeT = std::size_t,
        typename DiffT = std::ptrdiff_t>
    requires
        std::is_integral_v<SizeT> && std::is_unsigned_v<SizeT> &&
        std::is_integral_v<DiffT> && std::is_signed_v<DiffT>
    struct allocator;
}
```

Drop-in replacement for `std::allocator` that uses the `std::nothrow_t` versions
of `::operator new`, panicking on memory allocation failure instead of throwing
an exception as `std::allocator` would.

Additionally allows overriding the `size_type` and `difference_type` member
types via the `SizeT` and `DiffT` template parameters respectively. You could,
for example, override `SizeT` with `uint32_t` to make `gdt::dynarr` use a 32-bit
size and capacity on a 64-bit system. Note: unlike the Standard Library, GDT
containers allow allocators where `size_type` can't represent all non-negative
values of `difference_type`, meaning `size_type = uint32_t` and
`different_type = ptrdiff_t` where `ptrdiff_t` is 64-bit is perfectly fine.

## <gdt/dynarr.hxx>

```c++
namespace gdt
{
    // Dynamic array.
    template<typename T, typename Allocator = allocator<T>>
    class dynarr;
}
```

Drop-in replacement for `std::vector` optimized around use-cases that don't
throw exceptions. It can propagate exceptions if the given template arguments
throw exceptions, but only provides "basic exception safety" in those cases,
or calls `std::terminate` if providing exception safety would be
overly-difficult. For example, `std::vector::emplace` sometimes constructs the
new object outside the vector's buffer, then move-assigns it into the buffer
for exception safety reasons, but `gdt::dynarr` just constructs the new object
in-place and calls `std::terminate` if the constructor throws an exception.

`gdt::dynarr` might have some slightly different requirements than `std::vector`
around special member functions on the value type, but most types that implement 
idiomatic copy and/or move semantics should work just fine.

## <gdt/vec.hxx>

```c++
namespace gdt
{
    // Vector.
    template<typename T, std::size_t N> struct vec;
    template<typename T> using vec2 = vec<T, 2>;
    template<typename T> using vec3 = vec<T, 3>;
    template<typename T> using vec4 = vec<T, 4>;
}
```

An arithmetic vector.

Vectors can be trivially default-constructed:

```c++
vec3<float> v;
```

Constructed with a scalar copied to each component:

```c++
vec3<int> v(123);
gdt_assert(v.x() == 123);
gdt_assert(v.y() == 123);
gdt_assert(v.z() == 123);
```

Constructed with a specific value for each component:

```c++
vec v = {1, 2, 3};
gdt_assert(v.x() == 1);
gdt_assert(v.y() == 2);
gdt_assert(v.z() == 3);
```

Constructed from smaller vectors:

```c++
vec v = {1, vec(2, 3)};
gdt_assert(v.x() == 1);
gdt_assert(v.y() == 2);
gdt_assert(v.z() == 3);
```

Or by converting and/or truncating the components from another vector:

```c++
vec3<int> v(vec4<float>(1.1f, 2.2f, 3.3f, 4.4f));
gdt_assert(v.x() == 1);
gdt_assert(v.y() == 2);
gdt_assert(v.z() == 3);
```

Components can be accessed by index, by `xyzw`, or by `rgba`:

```c++
vec4<float> v;
gdt_assert(&v.x() == &v[0]);
gdt_assert(&v.y() == &v[1]);
gdt_assert(&v.z() == &v[2]);
gdt_assert(&v.w() == &v[3]);
gdt_assert(&v.r() == &v[0]);
gdt_assert(&v.g() == &v[1]);
gdt_assert(&v.b() == &v[2]);
gdt_assert(&v.a() == &v[3]);
```

Component swizzling is supported as well:

```c++
vec v = {1, 2, 3};
gdt_assert(all(v.xy() == vec(1, 2)));
gdt_assert(all(v.yx() == vec(2, 1)));
gdt_assert(all(v.ggbr() == vec(2, 2, 3, 1)));

v.set_zx(vec(4, 5));
gdt_assert(v.z() == 4);
gdt_assert(v.x() == 5);
```

Operators are component-wise, including comparison operators:

```c++
gdt_assert(all(-vec(1, 2, 3) == vec(-1, -2, -3)));
gdt_assert(all(1 + vec(2, 3) == vec(3, 4)));
gdt_assert(all((vec(1, 1) & vec(1, 0)) == vec(1, 0)));
```

To collapse boolean vectors, use the `gdt::any` and `gdt::all` functions:

```c++
gdt_assert(any(vec(1, 2) != vec(1, 3)));
gdt_assert(all(vec(1, 2) == vec(1, 2)));
```

Most math functions from `<cmath>` work on vectors as well, except you
either need to use the `gdt::` namespace versions or use 
argument-dependent lookup:

```c++
auto s = gdt::sin(vec(1.2f, 3.4f));
gdt_assert(s.x() == gdt::sin(1.2f));
gdt_assert(s.y() == gdt::sin(3.4f));

using std::cos;
auto c = cos(vec(1.2f, 3.4f));
gdt_assert(c.x() == cos(1.2f));
gdt_assert(c.y() == cos(3.4f));

vec2<float> i;
auto f = gdt::modf(vec(1.25f, 3.5f), &i);
gdt_assert(all(i == vec(1.0f, 3.0f)));
gdt_assert(all(f == vec(0.25f, 0.5f)));
```
