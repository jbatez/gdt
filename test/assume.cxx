// Copyright Jo Bates 2021.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at:
// https://www.boost.org/LICENSE_1_0.txt

#include <gdt/assume.hxx>

int test_assume(int, char** const)
{
    gdt_assume(2 == 2);
    return 0;
}
