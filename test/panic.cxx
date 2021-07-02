#include <gdt/panic.hxx>

int test_panic(int, char**)
{
    if (2 + 2 == 3)
    {
        gdt_panic();
    }

    return 0;
}
