// Copyright Jo Bates 2021.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at:
// https://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "panic.hxx"

#define gdt_assert(x) static_cast<void>(\
    (x) || (::gdt::panic(__FILE__, __LINE__, "gdt_assert("#x") failed"), 0))
