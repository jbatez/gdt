#include <gdt/assume.hxx>

int test_assume(int, char**)
{
    gdt_assume(2 == 2);
    return 0;
}
