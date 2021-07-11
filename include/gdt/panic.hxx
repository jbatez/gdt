// Copyright Jo Bates 2021.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at:
// https://www.boost.org/LICENSE_1_0.txt

#pragma once

namespace gdt
{
    [[noreturn]] void panic(
        const char* file,
        unsigned line,
        const char* message);
}

#define gdt_panic() (::gdt::panic(__FILE__, __LINE__, "gdt_panic()"))
