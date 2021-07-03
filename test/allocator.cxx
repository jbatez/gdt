#include <gdt/allocator.hxx>

#include <gdt/assert.hxx>
#include <climits>
#include <vector>

using gdt::allocator;

static consteval int test_consteval()
{
    using schar = signed char;
    using uchar = unsigned char;

    gdt_assert((allocator<int>().max_size() == SIZE_MAX / sizeof(int)));
    gdt_assert((allocator<int, uchar>().max_size() == UCHAR_MAX / sizeof(int)));
    gdt_assert((allocator<char, uchar, schar>().max_size() == SCHAR_MAX));

    int* p = allocator<int>().allocate(123);
    allocator<int>().deallocate(p, 123);

    gdt_assert(allocator<int>() == allocator<float>());

    return 0;
}

int test_allocator(int, char**)
{
    gdt_assert(test_consteval() == 0);

    [[maybe_unused]] std::vector<int, allocator<int>> v1(123);

    struct alignas(128) foo_t {};
    [[maybe_unused]] std::vector<foo_t, allocator<foo_t>> v2(123);

    return 0;
}
