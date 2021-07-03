#pragma once

namespace gdt
{
    [[noreturn]] void panic(
        const char* file,
        unsigned line,
        const char* message);
}

#define gdt_panic() (::gdt::panic(__FILE__, __LINE__, "gdt_panic()"))
