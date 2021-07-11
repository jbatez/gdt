// Copyright Jo Bates 2021.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at:
// https://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "panic.hxx"

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
