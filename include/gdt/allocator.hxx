// Copyright Jo Bates 2021.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at:
// https://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "assert.hxx"
#include <algorithm>
#include <climits>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>

namespace gdt
{
    // Allocator.
    template<
        typename T,
        typename SizeT = std::size_t,
        typename DiffT = std::ptrdiff_t>
    requires
        std::is_integral_v<SizeT> && std::is_unsigned_v<SizeT> &&
        std::is_integral_v<DiffT> && std::is_signed_v<DiffT>
    struct allocator
    {
        // Member types.
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

        // Max size.
        constexpr size_type max_size() noexcept
        {
            using common_type = std::common_type_t<
                std::size_t, SizeT, std::make_unsigned_t<DiffT>>;

            auto size_max = (std::numeric_limits<SizeT>::max)();
            auto diff_max = (std::numeric_limits<DiffT>::max)();

            return size_type((std::min)({
                common_type(SIZE_MAX / sizeof(value_type)),
                common_type(size_max / sizeof(value_type)),
                common_type(diff_max),
            }));
        }

        // Allocate.
        [[nodiscard]] constexpr T* allocate(size_type n)
        {
            gdt_assert(n <= max_size());
            auto size = std::size_t(sizeof(T) * n);

            if (std::is_constant_evaluated())
            {
                return std::allocator<T>().allocate(std::size_t(n));
            }

            void* p;
            if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
            {
                auto align = std::align_val_t(alignof(T));
                p = ::operator new(size, align, std::nothrow_t());
            }
            else
            {
                p = ::operator new(size, std::nothrow_t());
            }

            gdt_assert(p != nullptr);
            return static_cast<T*>(p);
        }

        // Deallocate.
        constexpr void deallocate(T* p, size_type n)
        {
            if (std::is_constant_evaluated())
            {
                std::allocator<T>().deallocate(p, std::size_t(n));
            }
            else if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
            {
                auto align = std::align_val_t(alignof(T));
                ::operator delete(static_cast<void*>(p), align);
            }
            else
            {
                ::operator delete(static_cast<void*>(p));
            }
        }

        // Equality.
        template<typename U>
        friend constexpr bool operator==(const allocator&, const allocator<U>&)
        noexcept
        {
            return true;
        }
    };
}
