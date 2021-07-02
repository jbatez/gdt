#pragma once

#include "panic.hxx"

#ifndef NDEBUG
#define gdt_unreachable() ( \
    ::gdt::panic(__FILE__, __LINE__, "gdt_unreachable() reached") \
)
#elif defined(_MSC_VER) && !defined(__clang__)
#define gdt_unreachable() __assume(0)
#else
#define gdt_unreachable() __builtin_unreachable()
#endif
