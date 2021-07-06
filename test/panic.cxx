#include <gdt/panic.hxx>

#include <cstdio>
#include <cstdlib>

[[noreturn]] void gdt::panic(
    const char* file,
    unsigned line,
    const char* message)
{
    std::fprintf(stderr, "%s:%u: %s\n", file, line, message);
    std::exit(EXIT_FAILURE);
}

int test_panic(int, char** const)
{
    if constexpr (2 + 2 == 3)
    {
        gdt_panic();
    }

    return 0;
}
