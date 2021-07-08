#pragma once

#include "../gdt/assume.hxx"
#include <compare>
#include <iterator>

namespace gdt_detail
{
    // Vector iterator.
    template<typename T, typename DiffT>
    class fill_iterator
    {
    public:
        // Constructor.
        constexpr fill_iterator() = default;

        // Constructor.
        constexpr fill_iterator(const T* ptr, DiffT iteration)
        :
            _ptr{ptr},
            _iteration{iteration}
        {}

        // Dereference.
        constexpr const T& operator*() const
        {
            return *_ptr;
        }

        // Member access.
        constexpr const T* operator->() const
        {
            return _ptr;
        }

        // Subscript.
        constexpr const T& operator[](DiffT) const
        {
            return *_ptr;
        }

        // Pre-increment.
        constexpr fill_iterator& operator++()
        {
            ++_iteration;
            return *this;
        }

        // Pre-decrement.
        constexpr fill_iterator& operator--()
        {
            --_iteration;
            return *this;
        }

        // Post-increment.
        constexpr fill_iterator operator++(int)
        {
            return {_ptr, _iteration++};
        }

        // Post-decrement.
        constexpr fill_iterator operator--(int)
        {
            return {_ptr, _iteration--};
        }

        // Addition.
        friend constexpr fill_iterator operator+(
            const fill_iterator& lhs,
            DiffT rhs)
        {
            return {lhs._ptr, lhs._iteration + rhs};
        }

        // Addition.
        friend constexpr fill_iterator operator+(
            DiffT lhs,
            const fill_iterator& rhs)
        {
            return {rhs._ptr, lhs + rhs._iteration};
        }

        // Subtraction.
        friend constexpr fill_iterator operator-(
            const fill_iterator& lhs,
            DiffT rhs)
        {
            return {lhs._ptr, lhs._iteration + rhs};
        }

        // Subtraction.
        friend constexpr DiffT operator-(
            const fill_iterator& lhs,
            const fill_iterator& rhs)
        {
            gdt_assume(lhs._ptr == rhs._ptr);
            return lhs._iteration - rhs._iteration;
        }

        // Addition assignment.
        friend constexpr fill_iterator& operator+=(
            fill_iterator& lhs,
            DiffT rhs)
        {
            lhs._iteration += rhs;
            return lhs;
        }

        // Subtraction assignment.
        friend constexpr fill_iterator& operator-=(
            fill_iterator& lhs,
            DiffT rhs)
        {
            lhs._iteration -= rhs;
            return lhs;
        }

        // Equality.
        friend constexpr bool operator==(
            const fill_iterator& lhs,
            const fill_iterator& rhs)
        {
            gdt_assume(lhs._ptr == rhs._ptr);
            return lhs._iteration == rhs._iteration;
        }

        // Comparison.
        friend constexpr std::strong_ordering operator<=>(
            const fill_iterator& lhs,
            const fill_iterator& rhs)
        {
            gdt_assume(lhs._ptr == rhs._ptr);
            return lhs._iteration <=> rhs._iteration;
        }

    private:
        // Member variables.
        const T* _ptr;
        DiffT _iteration;
    };
}

namespace std
{
    // Fill iterator traits.
    template<typename T, typename DiffT>
    struct iterator_traits<gdt_detail::fill_iterator<T, DiffT>>
    {
        using iterator_concept = random_access_iterator_tag;
        using iterator_category = random_access_iterator_tag;
        using value_type = T;
        using difference_type = DiffT;
        using pointer = const T*;
        using reference = const T&;
    };
}
