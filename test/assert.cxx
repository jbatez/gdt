#include <gdt/assert.hxx>

int test_assert(int, char** const)
{
    gdt_assert(2 == 2);
    return 0;
}
