#pragma once

namespace gdt
{
    [[noreturn]] void panic(
        char const *file,
        unsigned line,
        char const *message
    );
}

#define gdt_panic() (::gdt::panic(__FILE__, __LINE__, "gdt_panic()"))
