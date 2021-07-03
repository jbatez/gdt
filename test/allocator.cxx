#include <gdt/allocator.hxx>

#include <gdt/assert.hxx>
#include <climits>
#include <vector>

static consteval int test_consteval()
{
    int* p = gdt::allocator<int>().allocate(123);
    gdt::allocator<int>().deallocate(p, 123);
    return 123;
}

int test_allocator(int, char**)
{
    gdt_assert(test_consteval() == 123);

    gdt_assert((gdt::allocator<int>().max_size() == SIZE_MAX / sizeof(int)));
    gdt_assert((gdt::allocator<int, unsigned char>().max_size() == UCHAR_MAX));

    [[maybe_unused]] std::vector<int, gdt::allocator<int>> v1(123);

    struct alignas(128) foo_t {};
    [[maybe_unused]] std::vector<foo_t, gdt::allocator<foo_t>> v2(123);

    return 0;
}
