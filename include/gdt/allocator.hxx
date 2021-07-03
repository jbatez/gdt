#pragma once

#include "assert.hxx"
#include <cstddef>
#include <type_traits>

namespace gdt
{
    template<
        typename T,
        typename SizeT = std::size_t,
        typename DiffT = std::ptrdiff_t
    > requires
        std::is_integral_v<SizeT> && std::is_unsigned_v<SizeT> &&
        std::is_integral_v<DiffT> && std::is_signed_v<DiffT>
    struct allocator
    {
        using value_type = T;
        using size_type = SizeT;
        using difference_type = DiffT;
        using propagate_on_container_move_assignment = std::true_type;
        using is_always_equal = std::true_type;

        // Constructor.
        constexpr allocator() noexcept {}

        // Constructor.
        constexpr allocator(const allocator&) noexcept {}

        // Constructor.
        template<typename U>
        constexpr allocator(const allocator<U>&) noexcept {}

        // Destructor.
        constexpr ~allocator() {}

        // Assignment.
        constexpr allocator& operator=(const allocator&) = default;

        // Allocate.
        [[nodiscard]] constexpr T* allocate(size_type n);

        // Deallocate.
        constexpr void deallocate(T* p, size_type n);
    };
}
