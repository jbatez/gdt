#include <gdt/assert.hxx>

int test_assert(int, char**)
{
    gdt_assert(2 == 2);
    return 0;
}
