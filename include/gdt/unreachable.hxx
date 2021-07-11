// Copyright Jo Bates 2021.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at:
// https://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "panic.hxx"

#ifndef NDEBUG
#define gdt_unreachable() (\
    ::gdt::panic(__FILE__, __LINE__, "gdt_unreachable() reached"))
#elif defined(_MSC_VER) && !defined(__clang__)
#define gdt_unreachable() __assume(0)
#else
#define gdt_unreachable() __builtin_unreachable()
#endif
