#pragma once

#include "panic.hxx"

#define gdt_assert(x) (static_cast<void>( \
    (x) || (::gdt::panic(__FILE__, __LINE__, "gdt_assert("#x") failed"), 0) \
))
