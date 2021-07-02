#include <gdt/unreachable.hxx>

int test_unreachable(int, char**)
{
    if (2 + 2 == 3)
    {
        gdt_unreachable();
    }

    return 0;
}
