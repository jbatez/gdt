// Copyright Jo Bates 2021.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at:
// https://www.boost.org/LICENSE_1_0.txt

#include <gdt/unreachable.hxx>

int test_unreachable(int, char** const)
{
    if constexpr (2 + 2 == 3)
    {
        gdt_unreachable();
    }

    return 0;
}
