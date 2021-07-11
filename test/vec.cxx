#include <gdt/vec.hxx>

#include <gdt/assert.hxx>

using gdt::vec;

consteval int test_consteval()
{
    vec v1 = {1, 2};
    vec v2 = {v1, 3};
    vec v3 = gdt::vec4<int>{v1, v1};

    // Success.
    return 0;
}

int test_vec(int, char** const)
{
    // Consteval.
    gdt_assert(test_consteval() == 0);

    // Success.
    return 0;
}