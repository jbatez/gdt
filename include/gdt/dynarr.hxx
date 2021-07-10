#pragma once

#include "../gdt_detail/fill_iterator.hxx"
#include "allocator.hxx"
#include "assert.hxx"
#include "assume.hxx"
#include <algorithm>
#include <compare>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace gdt_detail
{
    // Determine the max number of elements
    // that could fit in the small variant.
    template<typename T, typename Allocator>
    consteval std::size_t dynarr_small_capacity()
    {
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using size_type = typename std::allocator_traits<Allocator>::size_type;
        using difference_type = typename std::allocator_traits<Allocator>::difference_type;

        using common_type = std::common_type_t<
            std::size_t, size_type, std::make_unsigned_t<difference_type>>;

        auto size_max = (std::numeric_limits<size_type>::max)();
        auto diff_max = (std::numeric_limits<difference_type>::max)();

        if (std::is_same_v<pointer, T*>)
        {
            return std::size_t(std::min({
                common_type(sizeof(T*) / sizeof(T)),
                common_type(size_max / sizeof(T)),
                common_type(diff_max),
            }));
        }
        else
        {
            return 0;
        }
    }

    // Dynarr storage.
    template<
        typename T,
        typename Allocator,
        std::size_t SmallCap = dynarr_small_capacity<T, Allocator>()>
    class dynarr_storage;

    // Dynarr iterator.
    template<typename T, typename Allocator>
    class dynarr_iterator;

    // Dynarr const iterator.
    template<typename T, typename Allocator>
    class dynarr_const_iterator;
}

namespace gdt
{
    // Dynamic array.
    template<typename T, typename Allocator = allocator<T>>
    class dynarr
    {
    public:
        // Member types.
        using value_type = T;
        using allocator_type = Allocator;
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
        using reference = value_type&;
        using const_reference = const value_type&;
        using size_type = typename std::allocator_traits<Allocator>::size_type;
        using difference_type = typename std::allocator_traits<Allocator>::difference_type;
        using iterator = gdt_detail::dynarr_iterator<T, Allocator>;
        using const_iterator = gdt_detail::dynarr_const_iterator<T, Allocator>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // Constructor.
        constexpr dynarr() noexcept(noexcept(Allocator()))
        :
            dynarr(Allocator())
        {}

        // Constructor.
        explicit constexpr dynarr(const Allocator& allocator) noexcept
        :
            _allocator{allocator},
            _storage(_allocator),
            _capacity{_small_capacity()},
            _size{0}
        {}

        // Constructor.
        explicit constexpr dynarr(
            size_type len,
            const Allocator& allocator = Allocator())
        :
            dynarr(allocator)
        {
            resize(len);
        }

        // Constructor.
        constexpr dynarr(
            size_type len,
            const T& fill_value,
            const Allocator& allocator = Allocator())
        :
            dynarr(allocator)
        {
            assign(len, fill_value);
        }

        // Constructor.
        template<typename InputIterator>
        requires std::is_base_of_v<
            std::input_iterator_tag,
            typename std::iterator_traits<InputIterator>::iterator_category>
        constexpr dynarr(
            InputIterator first,
            InputIterator last,
            const Allocator& allocator = Allocator())
        :
            dynarr(allocator)
        {
            assign(first, last);
        }

        // Constructor.
        constexpr dynarr(const dynarr& other)
        :
            dynarr(other, std::allocator_traits<Allocator>::
                select_on_container_copy_construction(other._allocator))
        {}

        // Constructor.
        constexpr dynarr(dynarr&& other) noexcept
        :
            _allocator{std::move(other._allocator)},
            _storage(
                std::move(other._storage),
                _allocator,
                other._capacity,
                other._size),
            _capacity{std::exchange(other._capacity, _small_capacity())},
            _size{std::exchange(other._size, 0)}
        {}

        // Constructor.
        constexpr dynarr(const dynarr& other, const Allocator& allocator)
        :
            dynarr(other.begin(), other.end(), allocator)
        {}

        // Constructor.
        constexpr dynarr(dynarr&& other, const Allocator& allocator)
        :
            dynarr(allocator)
        {
            if constexpr (
                std::allocator_traits<Allocator>::is_always_equal::value)
            {
                _take_buffer(other);
            }
            else if (_allocator == other._allocator)
            {
                _take_buffer(other);
            }
            else
            {
                reserve(other._size);
                _push_back_move(other.begin(), other.end());
            }
        }

        // Constructor.
        constexpr dynarr(
            std::initializer_list<T> il,
            const Allocator& allocator = Allocator())
        :
            dynarr(il.begin(), il.end(), allocator)
        {}

        // Destructor.
        constexpr ~dynarr()
        {
            _destroy_all_and_deallocate();
        }

        // Assignment.
        constexpr dynarr& operator=(const dynarr& other)
        {
            if constexpr (
                std::allocator_traits<Allocator>::
                    propagate_on_container_copy_assignment::value)
            {
                if constexpr (
                    !std::allocator_traits<Allocator>::is_always_equal::value)
                {
                    if (_allocator != other._allocator)
                    {
                        // Free old memory since we have a different allocator.
                        _reset();
                    }
                }
                _allocator = other._allocator;
            }
            assign(other.begin(), other.end());
            return *this;
        }

        // Assignment.
        constexpr dynarr& operator=(dynarr&& other)
        noexcept(
            std::allocator_traits<Allocator>::
                propagate_on_container_move_assignment::value ||
            std::allocator_traits<Allocator>::
                is_always_equal::value)
        {
            if (&other == this)
            {
                // No-op.
            }
            else if constexpr (
                std::allocator_traits<Allocator>::
                    propagate_on_container_move_assignment::value)
            {
                _destroy_all_and_deallocate();
                _allocator = std::move(other._allocator);
                _take_buffer(other);
            }
            else if constexpr (
                std::allocator_traits<Allocator>::is_always_equal::value)
            {
                _destroy_all_and_deallocate();
                _take_buffer(other);
            }
            else if (_allocator == other._allocator)
            {
                _destroy_all_and_deallocate();
                _take_buffer(other);
            }
            else
            {
                // Element-wise move since we have a different allocator.
                _reserve_for_assign(other._size);

                auto src_begin = other.begin();
                if (_size < other._size)
                {
                    auto src_end = src_begin + difference_type(_size);
                    std::move(src_begin, src_end, begin());
                    _push_back_move(src_end, other.end());
                }
                else
                {
                    auto move_end = std::move(src_begin, other.end(), begin());
                    _truncate(move_end);
                }
            }

            return *this;
        }

        // Assignment.
        constexpr dynarr& operator=(std::initializer_list<T> il)
        {
            assign(il);
            return *this;
        }

        // Assign.
        template<typename InputIterator>
        requires std::is_base_of_v<
            std::input_iterator_tag,
            typename std::iterator_traits<InputIterator>::iterator_category>
        constexpr void assign(
            InputIterator first,
            InputIterator last)
        {
            if constexpr (std::is_base_of_v<
                std::forward_iterator_tag,
                typename std::iterator_traits<InputIterator>::
                    iterator_category>)
            {
                auto d = std::distance(first, last);
                auto u = std::make_unsigned_t<decltype(d)>(d);
                gdt_assert(u <= max_size());
                _reserve_for_assign(size_type(u));
            }

            // Assign over existing elements where possible.
            auto& src = first;
            auto& src_end = last;
            bool src_exhausted = (src == src_end);

            auto dst = begin();
            auto dst_end = end();
            bool dst_exhausted = (dst == dst_end);

            for (;
                !src_exhausted && !dst_exhausted;
                src_exhausted = (++src == src_end),
                dst_exhausted = (++dst == dst_end))
            {
                *dst = *src;
            }

            // Push the rest or truncate.
            if (dst_exhausted)
            {
                for (; !src_exhausted; src_exhausted = (++src == src_end))
                {
                    push_back(*src);
                }
            }
            else
            {
                _truncate(dst);
            }
        }

        // Assign.
        constexpr void assign(size_type tgt_len, const T& fill_value)
        {
            _reserve_for_assign(tgt_len);

            // Assign over existing elements where possible.
            auto fill_len = difference_type(std::min(_size, tgt_len));
            auto fill_end = std::fill_n(begin(), fill_len, fill_value);

            // Push the rest or truncate.
            if (_size < tgt_len)
            {
                _fill_to(tgt_len, fill_value);
            }
            else
            {
                _truncate(fill_end);
            }
        }

        // Assign.
        constexpr void assign(std::initializer_list<T> il)
        {
            assign(il.begin(), il.end());
        }

        // Get allocator.
        constexpr allocator_type get_allocator() const noexcept
        {
            return _allocator;
        }

        // Begin.
        constexpr iterator begin() noexcept
        {
            return iterator(_storage.ptr(_capacity));
        }

        // Begin.
        constexpr const_iterator begin() const noexcept
        {
            return const_iterator(_storage.ptr(_capacity));
        }

        // End.
        constexpr iterator end() noexcept
        {
            return begin() + difference_type(_size);
        }

        // End.
        constexpr const_iterator end() const noexcept
        {
            return begin() + difference_type(_size);
        }

        // Reverse begin.
        constexpr reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }

        // Reverse begin.
        constexpr const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }

        // Reverse end.
        constexpr reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }

        // Reverse end.
        constexpr const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(begin());
        }

        // Const begin.
        constexpr const_iterator cbegin() const noexcept
        {
            return begin();
        }

        // Const end.
        constexpr const_iterator cend() const noexcept
        {
            return end();
        }

        // Const reverse begin.
        constexpr const_reverse_iterator crbegin() const noexcept
        {
            return rbegin();
        }

        // Const reverse end.
        constexpr const_reverse_iterator crend() const noexcept
        {
            return rend();
        }

        // Empty?
        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return _size == 0;
        }

        // Size.
        constexpr size_type size() const noexcept
        {
            return _size;
        }

        // Max size.
        constexpr size_type max_size() const noexcept
        {
            return std::allocator_traits<Allocator>::max_size(_allocator);
        }

        // Capacity.
        constexpr size_type capacity() const noexcept
        {
            return _capacity;
        }

        // Resize.
        constexpr void resize(size_type tgt_len)
        {
            _reserve_or_shrink(tgt_len);
            while (_size < tgt_len)
            {
                emplace_back();
            }
        }

        // Resize.
        constexpr void resize(size_type tgt_len, const T& fill_value)
        {
            _reserve_or_shrink(tgt_len);
            _fill_to(tgt_len, fill_value);
        }

        // Reserve.
        constexpr void reserve(size_type req_capacity)
        {
            if (_capacity < req_capacity)
            {
                _reallocate(_choose_new_capacity(req_capacity));
            }
        }

        // Shrink to fit.
        constexpr void shrink_to_fit()
        {
            if (_capacity > _size)
            {
                if constexpr (_small_capacity() == 0)
                {
                    _reallocate(_size);
                }
                else if (_size > _small_capacity())
                {
                    _reallocate(_size);
                }
                else if (_capacity > _small_capacity())
                {
                    if (std::is_constant_evaluated())
                    {
                        _reallocate(_small_capacity());
                    }
                    else
                    {
                        auto src = _storage.large;
                        auto dst = pointer(&_storage.small[0]);
                        _migrate(iterator(dst), iterator(src), _size);

                        std::allocator_traits<Allocator>::deallocate(
                            _allocator, src, _capacity);

                        _capacity = _small_capacity();
                    }
                }
            }
        }

        // Subscript.
        constexpr reference operator[](size_type i)
        {
            gdt_assume(i <= _capacity);
            return begin()[difference_type(i)];
        }

        // Subscript.
        constexpr const_reference operator[](size_type i) const
        {
            gdt_assume(i <= _capacity);
            return begin()[difference_type(i)];
        }

        // At.
        constexpr const_reference at(size_type i) const
        {
            gdt_assert(i < size());
            return begin()[difference_type(i)];
        }

        // At.
        constexpr reference at(size_type i)
        {
            gdt_assert(i < size());
            return begin()[difference_type(i)];
        }

        // Front.
        constexpr reference front()
        {
            gdt_assume(!empty());
            return *begin();
        }

        // Front.
        constexpr const_reference front() const
        {
            gdt_assume(!empty());
            return *begin();
        }

        // Back.
        constexpr reference back()
        {
            gdt_assume(!empty());
            return *(end() - 1);
        }

        // Back.
        constexpr const_reference back() const
        {
            gdt_assume(!empty());
            return *(end() - 1);
        }

        // Data.
        constexpr T* data() noexcept
        {
            return std::to_address(_storage.ptr(_capacity));
        }

        // Data.
        constexpr const T* data() const noexcept
        {
            return std::to_address(_storage.ptr(_capacity));
        }

        // Emplace back.
        template<typename... Args>
        constexpr reference emplace_back(Args&&... args)
        {
            if (_capacity == _size)
            {
                _reallocate(_choose_next_capacity());
            }

            T& dst = *end();
            _construct(std::addressof(dst), std::forward<Args>(args)...);
            ++_size;

            return dst;
        }

        // Push back.
        constexpr void push_back(const T& value)
        {
            emplace_back(value);
        }

        // Push back.
        constexpr void push_back(T&& value)
        {
            emplace_back(std::move(value));
        }

        // Pop back.
        constexpr void pop_back()
        {
            _destroy(std::addressof(back()));
            --_size;
        }

        // Emplace.
        template<typename... Args>
        constexpr iterator emplace(const_iterator position, Args&&... args)
        {
            constexpr bool must_use_ctor = true;
            return _emplace_or_insert<must_use_ctor>(
                position, std::forward<Args>(args)...);
        }

        // Insert.
        constexpr iterator insert(const_iterator position, const T& value)
        {
            constexpr bool must_use_ctor = false;
            return _emplace_or_insert<must_use_ctor>(
                position, value);
        }

        // Insert.
        constexpr iterator insert(const_iterator position, T&& value)
        {
            constexpr bool must_use_ctor = false;
            return _emplace_or_insert<must_use_ctor>(
                position, std::move(value));
        }

        // Insert.
        constexpr iterator insert(
            const_iterator position,
            size_type fill_len,
            const T& fill_value)
        {
            gdt_assert(fill_len <= max_size() - size());
            auto ptr = std::addressof(fill_value);
            return _insert(
                position,
                gdt_detail::fill_iterator(ptr, difference_type(0)),
                gdt_detail::fill_iterator(ptr, difference_type(fill_len)));
        }

        // Insert.
        template<typename InputIterator>
        requires std::is_base_of_v<
            std::input_iterator_tag,
            typename std::iterator_traits<InputIterator>::iterator_category>
        constexpr iterator insert(
            const_iterator position,
            InputIterator first,
            InputIterator last)
        {
            return _insert(position, first, last);
        }

        // Insert.
        constexpr iterator insert(
            const_iterator position,
            std::initializer_list<T> il)
        {
            return _insert(position, il.begin(), il.end());
        }

        // Erase.
        constexpr iterator erase(const_iterator position)
        {
            gdt_assume(position >= begin());
            gdt_assume(position < end());

            return erase(position, position + 1);
        }

        // Erase.
        constexpr iterator erase(const_iterator first, const_iterator last)
        {
            gdt_assume(first >= begin());
            gdt_assume(first <= last);
            gdt_assume(last <= end());

            auto beg = begin();
            auto first_idx = first - beg;
            auto last_idx = last - beg;

            // Shift forward elements after the erase range.
            auto src_begin = beg + last_idx;
            auto src_end = end();
            auto dst_begin = beg + first_idx;
            auto dst_end = std::move(src_begin, src_end, dst_begin);

            // Truncate after the shift.
            _truncate(dst_end);

            // Done.
            return dst_begin;
        }

        // Swap.
        constexpr void swap(dynarr& other)
        noexcept(
            std::allocator_traits<Allocator>::
                propagate_on_container_swap::value ||
            std::allocator_traits<Allocator>::
                is_always_equal::value)
        {
            using std::swap;

            if constexpr (
                std::allocator_traits<allocator_type>::
                    propagate_on_container_swap::value)
            {
                swap(_allocator, other._allocator);
            }
            else
            {
                gdt_assume(_allocator == other._allocator);
            }

            _storage.swap(other._storage,
                _allocator, other._allocator,
                _capacity, other._capacity,
                _size, other._size);

            swap(_capacity, other._capacity);
            swap(_size, other._size);
        }

        // Swap.
        friend constexpr void swap(dynarr& lhs, dynarr& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
        {
            lhs.swap(rhs);
        }

        // Clear.
        constexpr void clear() noexcept
        {
            _truncate(begin());
        }

        // Equality.
        friend constexpr bool operator==(const dynarr& lhs, const dynarr& rhs)
        {
            return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        }

        // Comparison.
        friend constexpr auto operator<=>(const dynarr& lhs, const dynarr& rhs)
        -> decltype(lhs[0] <=> rhs[0])
        {
            // TODO: Use lexicographical_compare_three_way.

            auto lhs_itr = lhs.begin();
            auto lhs_end = lhs.end();
            bool lhs_exhausted = (lhs_itr == lhs_end);

            auto rhs_itr = rhs.begin();
            auto rhs_end = rhs.end();
            bool rhs_exhausted = (rhs_itr == rhs_end);

            for (;
                !lhs_exhausted && !rhs_exhausted;
                lhs_exhausted = (++lhs_itr == lhs_end),
                rhs_exhausted = (++rhs_itr == rhs_end))
            {
                auto ret = (*lhs_itr <=> *rhs_itr);
                if (ret != 0)
                {
                    return ret;
                }
            }

            if (!lhs_exhausted)
            {
                return std::strong_ordering::greater;
            }
            else if (!rhs_exhausted)
            {
                return std::strong_ordering::less;
            }
            else
            {
                return std::strong_ordering::equal;
            }
        }

    private:
        // Member variables.
        [[no_unique_address]] Allocator _allocator;
        gdt_detail::dynarr_storage<T, Allocator> _storage;
        size_type _capacity;
        size_type _size;

        // Determine the max number of elements
        // that could fit in the small variant.
        static constexpr size_type _small_capacity()
        {
            return size_type(gdt_detail::dynarr_small_capacity<T, Allocator>());
        }

        // Take ownership of another dynarr's buffer.
        constexpr void _take_buffer(dynarr& other) noexcept
        {
            _storage.take(
                other._storage, _allocator, other._capacity, other._size);
            _capacity = std::exchange(other._capacity, _small_capacity());
            _size = std::exchange(other._size, 0);
        }

        // Choose the next capacity.
        constexpr size_type _choose_next_capacity()
        {
            auto req_capacity = size_type(_capacity + 1);
            gdt_assert(req_capacity != 0); // Assert no overflow.
            return _choose_new_capacity(req_capacity);
        }

        // Choose a new capacity >= `req_capacity`.
        constexpr size_type _choose_new_capacity(size_type req_capacity)
        {
            auto max_capacity = max_size();
            auto capacity_x2 = size_type(_capacity * 2);
            if (capacity_x2 < _capacity || capacity_x2 > max_capacity)
            {
                capacity_x2 = max_capacity;
            }

            return std::max(req_capacity, capacity_x2);
        }

        // Allocate a buffer with capacity `n`.
        constexpr pointer _allocate(size_type n)
        {
            if (n == 0)
            {
                return nullptr;
            }
            else
            {
                return std::allocator_traits<Allocator>::allocate(
                    _allocator, n);
            }
        }

        // Construct using allocator.
        template<typename... Args>
        constexpr void _construct(T* p, Args&&... args)
        {
            std::allocator_traits<Allocator>::construct(
                _allocator, p, std::forward<Args>(args)...);
        }

        // Destroy using allocator.
        constexpr void _destroy(T* p) noexcept
        {
            std::allocator_traits<Allocator>::destroy(_allocator, p);
        }

        // Destroy range using allocator.
        constexpr void _destroy(iterator first, iterator last) noexcept
        {
            for (; first < last; ++first)
            {
                _destroy(std::addressof(*first));
            }
        }

        // Destroy and construct with `noexcept`.
        template<typename... Args>
        constexpr void _replace(T* p, Args&&... args) noexcept
        {
            _destroy(p);
            _construct(p, std::forward<Args>(args)...);
        }

        // Move and destroy `n` elements from `src` to `dst`.
        constexpr void _migrate(iterator dst, iterator src, size_type n)
        noexcept
        {
            for (; n > 0; ++dst, ++src, --n)
            {
                _construct(std::addressof(*dst), std::move(*src));
                _destroy(std::addressof(*src));
            }
        }

        // Deallocate the current buffer.
        constexpr void _deallocate() noexcept
        {
            _storage.deallocate(_allocator, _capacity);
        }

        // Move to a new buffer with the given capacity.
        constexpr void _reallocate(size_type new_capacity)
        {
            gdt_assume(new_capacity >= _size);

            auto new_ptr = _allocate(new_capacity);
            _migrate(iterator(new_ptr), begin(), _size);
            _deallocate();
            _storage.large = new_ptr;
            _capacity = new_capacity;
        }

        // Destroy all and deallocate.
        constexpr void _destroy_all_and_deallocate() noexcept
        {
            _destroy(begin(), end());
            _deallocate();
        }

        // Erase elements at the given iterator and beyond.
        constexpr void _truncate(iterator new_end) noexcept
        {
            gdt_assume(new_end >= begin());
            gdt_assume(new_end <= end());

            _size = size_type(new_end - begin());
            _destroy(new_end, end());
        }

        // Reset to the default state.
        constexpr void _reset() noexcept
        {
            _destroy_all_and_deallocate();
            _storage.reset(_allocator);
            _capacity = _small_capacity();
            _size = 0;
        }

        // Reserve at least `tgt_len` capacity.
        // Don't bother to migrate on reallocation.
        constexpr void _reserve_for_assign(size_type tgt_len)
        {
            if (_capacity < tgt_len)
            {
                _reset();
                reserve(tgt_len);
            }
        }

        // Reserve at least `tgt_len` capacity.
        // Shrink to `tgt_len` if necessary.
        constexpr void _reserve_or_shrink(size_type tgt_len)
        {
            reserve(tgt_len);
            while (_size > tgt_len)
            {
                pop_back();
            }
        }

        // Push back move range.
        constexpr void _push_back_move(iterator first, iterator last)
        {
            for (; first < last; ++first)
            {
                push_back(std::move(*first));
            }
        }

        // Grow to at least `tgt_len` using `fill_value` for new elements.
        constexpr void _fill_to(size_type tgt_len, const T& fill_value)
        {
            while (_size < tgt_len)
            {
                push_back(fill_value);
            }
        }

        // Emplace or insert.
        // Set `must_use_ctor = true` for emplace.
        // Set `must_use_ctor = false` for insert.
        template<bool must_use_ctor, typename... Args>
        constexpr iterator _emplace_or_insert(
            const_iterator position,
            Args&&... args)
        {
            gdt_assume(position >= begin());
            gdt_assume(position <= end());

            // Emplace in the middle of migration if reallocation is necessary.
            if (_capacity == _size)
            {
                auto new_capacity = _choose_next_capacity();
                auto new_ptr = _allocate(new_capacity);
                return _emplace_migrate(
                    new_ptr, new_capacity, position,
                    std::forward<Args>(args)...);
            }

            // Emplace back if given the end iterator.
            auto old_end = end();
            if (position == old_end)
            {
                auto p = std::addressof(*old_end);
                _construct(p, std::forward<Args>(args)...);
                ++_size;
                return old_end;
            }

            // Shift all elements from position onward back otherwise.
            auto old_end_m1 = old_end - 1;
            _construct(std::addressof(*old_end), std::move(*old_end_m1));
            ++_size;

            auto beg = begin();
            auto pos = position - beg + beg;
            std::move_backward(pos, old_end_m1, old_end);

            // Emplace/insert in the newly-created gap.
            if constexpr (must_use_ctor)
            {
                _replace(std::addressof(*pos), std::forward<Args>(args)...);
            }
            else
            {
                static_assert(sizeof...(Args) == 1);
                *pos = (std::forward<Args>(args), ...);
            }

            // Done.
            return pos;
        }

        // Insert multiple.
        template<typename InputIterator>
        constexpr iterator _insert(
            const_iterator position,
            InputIterator first,
            InputIterator last)
        {
            gdt_assume(position >= begin());
            gdt_assume(position <= end());

            if constexpr (std::is_base_of_v<
                std::forward_iterator_tag,
                typename std::iterator_traits<InputIterator>::
                    iterator_category>)
            {
                // We can determine the new size ahead
                // of time with forward iterators.
                auto d = std::distance(first, last);
                auto u = std::make_unsigned_t<decltype(d)>(d);
                gdt_assert(u <= max_size() - _size);
                auto new_size = size_type(_size + u);

                // Insert in the middle of migration
                // if reallocation is necessary.
                if (_capacity < new_size)
                {
                    auto new_capacity = _choose_new_capacity(new_size);
                    auto new_ptr = _allocate(new_capacity);
                    return _insert_migrate(
                        new_ptr, new_capacity, new_size, position, first, last);
                }

                // Emplace back if given the end iterator.
                auto old_end = end();
                if (position == old_end)
                {
                    for (auto dst = old_end; first != last; ++first, ++dst)
                    {
                        _construct(std::addressof(*dst), *first);
                        ++_size;
                    }
                    return old_end;
                }

                // Insert in the middle of the current buffer otherwise.
                return _insert_mid_buffer(new_size, position, first, last);
            }
            else
            {
                // We can't determine the new size ahead of time without forward
                // iterators. The best we can do is insert one at a time.
                auto beg = begin();
                auto pos = position - beg + beg;
                for (auto dst = pos; first != last; ++first, ++dst)
                {
                    insert(begin() + dst, *first);
                }
                return begin() + pos;
            }
        }

        // Emplace in the middle of migration.
        template<typename... Args>
        constexpr iterator _emplace_migrate(
            pointer new_ptr,
            size_type new_capacity,
            const_iterator position,
            Args&&... args)
        noexcept
        {
            gdt_assume(new_capacity > _size);
            gdt_assume(position >= begin());
            gdt_assume(position <= end());

            // Migrate up to position.
            auto old_beg = begin();
            auto new_beg = iterator(new_ptr);
            auto pos_idx = position - old_beg;
            _migrate(new_beg, old_beg, size_type(pos_idx));

            // Emplace at position.
            auto new_pos = new_beg + pos_idx;
            _construct(std::addressof(*new_pos), std::forward<Args>(args)...);

            // Migrate after position.
            auto old_pos = old_beg + pos_idx;
            _migrate(new_pos + 1, old_pos, (_size - size_type(pos_idx)));

            // Free the old buffer and use the new one.
            _deallocate();
            _storage.large = new_ptr;
            _capacity = new_capacity;
            _size += 1;

            // Done.
            return new_pos;
        }

        // Insert in the middle of migration.
        template<typename ForwardIterator>
        constexpr iterator _insert_migrate(
            pointer new_ptr,
            size_type new_capacity,
            size_type new_size,
            const_iterator position,
            ForwardIterator& first,
            ForwardIterator& last)
        noexcept
        {
            gdt_assume(new_capacity >= new_size);
            gdt_assume(new_size > _size);
            gdt_assume(position >= begin());
            gdt_assume(position <= end());

            // Migrate up to position.
            auto old_beg = begin();
            auto new_beg = iterator(new_ptr);
            auto pos_idx = position - old_beg;
            _migrate(new_beg, old_beg, size_type(pos_idx));

            // Emplace at position.
            auto new_pos = new_beg + pos_idx;
            auto dst = new_pos;
            for (; first != last; ++first, ++dst)
            {
                _construct(std::addressof(*dst), *first);
            }

            // Migrate after position.
            auto old_pos = old_beg + pos_idx;
            _migrate(dst, old_pos, (_size - size_type(pos_idx)));

            // Free the old buffer and use the new one.
            _deallocate();
            _storage.large = new_ptr;
            _capacity = new_capacity;
            _size = new_size;

            // Done.
            return new_pos;
        }

        // Insert in the middle of the current buffer.
        template<typename InputIterator>
        constexpr iterator _insert_mid_buffer(
            size_type new_size,
            const_iterator position,
            InputIterator first,
            InputIterator last)
        noexcept
        {
            gdt_assume(new_size > _size);
            gdt_assume(new_size <= _capacity);
            gdt_assume(position >= begin());
            gdt_assume(position <= end());

            // Shift all elements from position onward back.
            auto beg = begin();
            auto old_end = end();

            auto dst = beg + new_size;
            auto src = beg + _size;
            auto pos = position - beg + beg;

            while (dst > old_end && src > pos)
            {
                _construct(std::addressof(*--dst), std::move(*--src));
            }

            while (src > pos)
            {
                *--dst = std::move(*--src);
            }

            // Insert from back to front if possible
            // to match above shift memory access pattern.
            // Insert front to back otherwise.
            if constexpr (std::is_base_of_v<
                std::bidirectional_iterator_tag,
                typename std::iterator_traits<InputIterator>::
                    iterator_category>)
            {
                while (dst > old_end)
                {
                    _construct(std::addressof(*--dst), *--last);
                }

                while (dst > pos)
                {
                    *--dst = *--last;
                }
            }
            else
            {
                auto dst_end = dst;
                dst = pos;

                while (dst < dst_end && dst < old_end)
                {
                    *dst++ = *first++;
                }

                while (dst < dst_end)
                {
                    _construct(std::addressof(*dst++, *first++));
                }
            }

            // Done.
            _size = new_size;
            return pos;
        }
    };

    // Deduction guide.
    template<
        typename InputIterator,
        typename Allocator = allocator<
            typename std::iterator_traits<InputIterator>::value_type>>
    dynarr(InputIterator, InputIterator, Allocator = Allocator()) ->
    dynarr<typename std::iterator_traits<InputIterator>::value_type, Allocator>;

    // Erase.
    template<typename T, typename Allocator, typename U>
    constexpr typename dynarr<T, Allocator>::size_type
    erase(dynarr<T, Allocator>& a, const U& value)
    {
        auto old_end = a.end();
        auto new_end = std::remove(a.begin(), old_end, value);
        using size_type = typename dynarr<T, Allocator>::size_type;
        auto count = size_type(old_end - new_end);
        a.erase(new_end, old_end);
        return count;
    }

    // Erase if.
    template<typename T, typename Allocator, typename Pred>
    constexpr typename dynarr<T, Allocator>::size_type
    erase_if(dynarr<T, Allocator>& a, Pred&& pred)
    {
        auto beg = a.begin();
        auto old_end = a.end();
        auto new_end = std::remove_if(beg, old_end, std::forward<Pred>(pred));
        using size_type = typename dynarr<T, Allocator>::size_type;
        auto count = size_type(old_end - new_end);
        a.erase(new_end, old_end);
        return count;
    }
}

namespace gdt_detail
{
    using namespace gdt;

    // Dynarr storage (SmallCap > 0).
    template<typename T, typename Allocator, std::size_t SmallCap>
    class dynarr_storage
    {
    public:
        // Member types.
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
        using size_type = typename std::allocator_traits<Allocator>::size_type;
        using difference_type = typename std::allocator_traits<Allocator>::difference_type;

        // Only works with T* for now.
        // Other pointer types should use SmallCap == 0
        // to get the other dynarr_storage specialization.
        static_assert(std::is_same_v<pointer, T*>);

        // Member variables.
        union
        {
            alignas(T) std::byte small[sizeof(T) * SmallCap];
            pointer large;
        };

        // Constructor.
        explicit constexpr dynarr_storage(Allocator& allocator) noexcept
        {
            reset(allocator);
        }

        // Constructor.
        constexpr dynarr_storage(
            dynarr_storage&& other,
            Allocator& allocator,
            size_type capacity,
            size_type size)
        noexcept
        {
            take(other, allocator, capacity, size);
        }

        // Pointer.
        constexpr pointer ptr(size_type capacity) noexcept
        {
            if (std::is_constant_evaluated() || capacity > SmallCap)
            {
                return large;
            }
            else
            {
                gdt_assume(capacity == SmallCap);
                return pointer(&small[0]);
            }
        }

        // Pointer.
        constexpr const_pointer ptr(size_type capacity) const noexcept
        {
            if (std::is_constant_evaluated() || capacity > SmallCap)
            {
                return large;
            }
            else
            {
                gdt_assume(capacity == SmallCap);
                return const_pointer(&small[0]);
            }
        }

        // Take.
        constexpr void take(
            dynarr_storage& other,
            Allocator& allocator,
            size_type capacity,
            size_type size)
        noexcept
        {
            if (std::is_constant_evaluated())
            {
                large = other.large;
                other.reset(allocator);
            }
            else if (capacity > SmallCap)
            {
                large = std::exchange(other.large, nullptr);
            }
            else
            {
                gdt_assume(capacity == SmallCap);

                pointer dst = pointer(&small[0]);
                pointer src = pointer(&other.small[0]);

                for (size_type i = 0; i < size; ++i, ++dst, ++src)
                {
                    std::allocator_traits<Allocator>::construct(
                        allocator, dst, std::move(*src));
                    std::allocator_traits<Allocator>::destroy(
                        allocator, src);
                }
            }
        }

        // Swap.
        constexpr void swap(
            dynarr_storage& other,
            Allocator& new_allocator,
            Allocator& other_allocator,
            size_type old_capacity,
            size_type other_capacity,
            size_type old_size,
            size_type other_size)
        noexcept
        {
            using std::swap;

            if (std::is_constant_evaluated() || 
                (old_capacity > SmallCap && other_capacity > SmallCap))
            {
                swap(large, other.large);
            }
            else if (old_capacity > SmallCap)
            {
                gdt_assume(other_capacity == SmallCap);
                auto old_large = large;

                auto dst = pointer(&small[0]);
                auto src = pointer(&other.small[0]);

                for (size_type i = 0; i < other_size; ++i, ++dst, ++src)
                {
                    std::allocator_traits<Allocator>::construct(
                        new_allocator, dst, std::move(*src));
                    std::allocator_traits<Allocator>::destroy(
                        new_allocator, src);
                }

                other.large = old_large;
            }
            else if (other_capacity > SmallCap)
            {
                gdt_assume(old_capacity == SmallCap);
                auto other_large = other.large;

                auto dst = pointer(&other.small[0]);
                auto src = pointer(&small[0]);

                for (size_type i = 0; i < old_size; ++i, ++dst, ++src)
                {
                    std::allocator_traits<Allocator>::construct(
                        other_allocator, dst, std::move(*src));
                    std::allocator_traits<Allocator>::destroy(
                        other_allocator, src);
                }

                large = other_large;
            }
            else
            {
                gdt_assume(old_capacity == SmallCap);
                gdt_assume(other_capacity == SmallCap);

                auto itr = pointer(&small[0]);
                auto otr = pointer(&other.small[0]);

                auto common_size = std::min(old_size, other_size);
                for (size_type i = 0; i < common_size; ++i, ++itr, ++otr)
                {
                    swap(*itr, *otr);
                }

                for (auto i = common_size; i < other_size; ++i, ++itr, ++otr)
                {
                    std::allocator_traits<Allocator>::construct(
                        new_allocator, itr, std::move(*otr));
                    std::allocator_traits<Allocator>::destroy(
                        new_allocator, otr);
                }

                for (auto i = common_size; i < old_size; ++i, ++itr, ++otr)
                {
                    std::allocator_traits<Allocator>::construct(
                        other_allocator, otr, std::move(*itr));
                    std::allocator_traits<Allocator>::destroy(
                        other_allocator, itr);
                }
            }
        }

        // Deallocate.
        constexpr void deallocate(
            Allocator& allocator,
            size_type capacity)
        noexcept
        {
            if (std::is_constant_evaluated() || capacity > SmallCap)
            {
                std::allocator_traits<Allocator>::deallocate(
                    allocator, large, capacity);
            }
            else
            {
                gdt_assume(capacity == SmallCap);
            }
        }

        // Reset.
        constexpr void reset(Allocator& allocator) noexcept
        {
            if (std::is_constant_evaluated())
            {
                large = std::allocator_traits<Allocator>::allocate(
                    allocator, SmallCap);
            }
        }
    };

    // Dynarr storage (SmallCap == 0).
    template<typename T, typename Allocator>
    struct dynarr_storage<T, Allocator, 0>
    {
    public:
        // Member types.
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
        using size_type = typename std::allocator_traits<Allocator>::size_type;
        using difference_type = typename std::allocator_traits<Allocator>::difference_type;

        // Member variables.
        pointer large;

        // Constructor.
        constexpr explicit dynarr_storage(Allocator&) noexcept
        :
            large{nullptr}
        {}

        // Constructor.
        constexpr dynarr_storage(
            dynarr_storage&& other,
            Allocator&,
            size_type,
            size_type)
        noexcept
        :
            large{std::exchange(other.large, nullptr)}
        {}

        // Pointer.
        constexpr pointer ptr(size_type) noexcept
        {
            return large;
        }

        // Pointer.
        constexpr const_pointer ptr(size_type) const noexcept
        {
            return large;
        }

        // Take.
        constexpr void take(
            dynarr_storage& other,
            Allocator&,
            size_type,
            size_type)
        noexcept
        {
            large = std::exchange(other.large, nullptr);
        }

        // Swap.
        constexpr void swap(
            dynarr_storage& other,
            Allocator&, Allocator&,
            size_type, size_type,
            size_type, size_type)
        noexcept
        {
            using std::swap;
            swap(large, other.large);
        }

        // Deallocate.
        constexpr void deallocate(
            Allocator& allocator,
            size_type capacity)
        noexcept
        {
            if (large != nullptr)
            {
                std::allocator_traits<Allocator>::deallocate(
                    allocator, large, capacity);
            }
        }

        // Reset.
        constexpr void reset(Allocator&) noexcept
        {
            large = nullptr;
        }
    };

    // Dynarr iterator.
    template<typename T, typename Allocator>
    class dynarr_iterator
    {
    public:
        // Constructor.
        constexpr dynarr_iterator() = default;

        // Dereference.
        constexpr T& operator*() const
        {
            return *_ptr;
        }

        // Member access.
        constexpr typename dynarr<T, Allocator>::pointer
        operator->() const
        {
            return _ptr;
        }

        // Subscript.
        constexpr T& operator[](
            typename dynarr<T, Allocator>::difference_type i)
        const
        {
            return _ptr[_ptr_diff(i)];
        }

        // Pre-increment.
        constexpr dynarr_iterator& operator++()
        {
            ++_ptr;
            return *this;
        }

        // Pre-decrement.
        constexpr dynarr_iterator& operator--()
        {
            --_ptr;
            return *this;
        }

        // Post-increment.
        constexpr dynarr_iterator operator++(int)
        {
            return dynarr_iterator(_ptr++);
        }

        // Post-decrement.
        constexpr dynarr_iterator operator--(int)
        {
            return dynarr_iterator(_ptr--);
        }

        // Addition.
        friend constexpr dynarr_iterator operator+(
            const dynarr_iterator& lhs,
            typename dynarr<T, Allocator>::difference_type rhs)
        {
            return dynarr_iterator(lhs._ptr + _ptr_diff(rhs));
        }

        // Addition.
        friend constexpr dynarr_iterator operator+(
            typename dynarr<T, Allocator>::difference_type lhs,
            const dynarr_iterator& rhs)
        {
            return dynarr_iterator(_ptr_diff(lhs) + rhs._ptr);
        }

        // Subtraction.
        friend constexpr dynarr_iterator operator-(
            const dynarr_iterator& lhs,
            typename dynarr<T, Allocator>::difference_type rhs)
        {
            return dynarr_iterator(lhs._ptr - _ptr_diff(rhs));
        }

        // Subtraction.
        friend constexpr typename dynarr<T, Allocator>::difference_type
        operator-(const dynarr_iterator& lhs, const dynarr_iterator& rhs)
        {
            return _da_diff(lhs._ptr - rhs._ptr);
        }

        // Addition assignment.
        friend constexpr dynarr_iterator& operator+=(
            dynarr_iterator& lhs,
            typename dynarr<T, Allocator>::difference_type rhs)
        {
            lhs._ptr += _ptr_diff(rhs);
            return lhs;
        }

        // Subtraction assignment.
        friend constexpr dynarr_iterator& operator-=(
            dynarr_iterator& lhs,
            typename dynarr<T, Allocator>::difference_type rhs)
        {
            lhs._ptr -= _ptr_diff(rhs);
            return lhs;
        }

        // Equality.
        friend constexpr bool operator==(
            const dynarr_iterator& lhs,
            const dynarr_iterator& rhs)
        {
            return lhs._ptr == rhs._ptr;
        }

        // Comparison.
        friend constexpr std::strong_ordering operator<=>(
            const dynarr_iterator& lhs,
            const dynarr_iterator& rhs)
        {
            return lhs._ptr <=> rhs._ptr;
        }

    private:
        // Friends.
        friend dynarr<T, Allocator>;
        friend dynarr_const_iterator<T, Allocator>;

        // Member types.
        using _pointer = typename dynarr<T, Allocator>::pointer;
        using _pointer_diff = typename std::iterator_traits<_pointer>::difference_type;
        using _dynarr_diff = typename dynarr<T, Allocator>::difference_type;

        // Member variables.
        _pointer _ptr;

        // Constructor.
        explicit constexpr dynarr_iterator(const _pointer& ptr)
        :
            _ptr{ptr}
        {}

        // Pointer diff from dynarr diff.
        static constexpr _pointer_diff _ptr_diff(_dynarr_diff d)
        {
            auto ret = _pointer_diff(d);
            gdt_assume(ret == d);
            return ret;
        }

        // Dynarr diff from pointer diff.
        static constexpr _dynarr_diff _da_diff(_pointer_diff d)
        {
            auto ret = _dynarr_diff(d);
            gdt_assume(ret == d);
            return ret;
        }
    };

    // Dynarr const iterator.
    template<typename T, typename Allocator>
    class dynarr_const_iterator
    {
    public:
        // Constructor.
        constexpr dynarr_const_iterator() = default;

        // Constructor.
        constexpr dynarr_const_iterator(
            const dynarr_iterator<T, Allocator>& other)
        :
            _ptr{other._ptr}
        {}

        // Dereference.
        constexpr const T& operator*() const
        {
            return *_ptr;
        }

        // Member access.
        constexpr typename dynarr<T, Allocator>::const_pointer
        operator->() const
        {
            return _ptr;
        }

        // Subscript.
        constexpr const T& operator[](
            typename dynarr<T, Allocator>::difference_type i)
        const
        {
            return _ptr[_ptr_diff(i)];
        }

        // Pre-increment.
        constexpr dynarr_const_iterator& operator++()
        {
            ++_ptr;
            return *this;
        }

        // Pre-decrement.
        constexpr dynarr_const_iterator& operator--()
        {
            --_ptr;
            return *this;
        }

        // Post-increment.
        constexpr dynarr_const_iterator operator++(int)
        {
            return dynarr_const_iterator(_ptr++);
        }

        // Post-decrement.
        constexpr dynarr_const_iterator operator--(int)
        {
            return dynarr_const_iterator(_ptr--);
        }

        // Addition.
        friend constexpr dynarr_const_iterator operator+(
            const dynarr_const_iterator& lhs,
            typename dynarr<T, Allocator>::difference_type rhs)
        {
            return dynarr_const_iterator(lhs._ptr + _ptr_diff(rhs));
        }

        // Addition.
        friend constexpr dynarr_const_iterator operator+(
            typename dynarr<T, Allocator>::difference_type lhs,
            const dynarr_const_iterator& rhs)
        {
            return dynarr_const_iterator(_ptr_diff(lhs) + rhs._ptr);
        }

        // Subtraction.
        friend constexpr dynarr_const_iterator operator-(
            const dynarr_const_iterator& lhs,
            typename dynarr<T, Allocator>::difference_type rhs)
        {
            return dynarr_const_iterator(lhs._ptr - _ptr_diff(rhs));
        }

        // Subtraction.
        friend constexpr typename dynarr<T, Allocator>::difference_type
        operator-(
            const dynarr_const_iterator& lhs,
            const dynarr_const_iterator& rhs)
        {
            return _da_diff(lhs._ptr - rhs._ptr);
        }

        // Addition assignment.
        friend constexpr dynarr_const_iterator& operator+=(
            dynarr_const_iterator& lhs,
            typename dynarr<T, Allocator>::difference_type rhs)
        {
            lhs._ptr += _ptr_diff(rhs);
            return lhs;
        }

        // Subtraction assignment.
        friend constexpr dynarr_const_iterator& operator-=(
            dynarr_const_iterator& lhs,
            typename dynarr<T, Allocator>::difference_type rhs)
        {
            lhs._ptr -= _ptr_diff(rhs);
            return lhs;
        }

        // Equality.
        friend constexpr bool operator==(
            const dynarr_const_iterator& lhs,
            const dynarr_const_iterator& rhs)
        {
            return lhs._ptr == rhs._ptr;
        }

        // Comparison.
        friend constexpr std::strong_ordering operator<=>(
            const dynarr_const_iterator& lhs,
            const dynarr_const_iterator& rhs)
        {
            return lhs._ptr <=> rhs._ptr;
        }

    private:
        // Friends.
        friend dynarr<T, Allocator>;

        // Member types.
        using _pointer = typename dynarr<T, Allocator>::const_pointer;
        using _pointer_diff = typename std::iterator_traits<_pointer>::difference_type;
        using _dynarr_diff = typename dynarr<T, Allocator>::difference_type;

        // Member variables.
        _pointer _ptr;

        // Constructor.
        explicit constexpr dynarr_const_iterator(const _pointer& ptr)
        :
            _ptr{ptr}
        {}

        // Pointer diff from dynarr diff.
        static constexpr _pointer_diff _ptr_diff(_dynarr_diff d)
        {
            auto ret = _pointer_diff(d);
            gdt_assume(ret == d);
            return ret;
        }

        // Dynarr diff from pointer diff.
        static constexpr _dynarr_diff _da_diff(_pointer_diff d)
        {
            auto ret = _dynarr_diff(d);
            gdt_assume(ret == d);
            return ret;
        }
    };
}

namespace std
{
    // Dynarr iterator traits.
    template<typename T, typename Allocator>
    struct iterator_traits<gdt_detail::dynarr_iterator<T, Allocator>>
    {
        using iterator_concept = contiguous_iterator_tag;
        using iterator_category = random_access_iterator_tag;
        using value_type = typename gdt::dynarr<T, Allocator>::value_type;
        using difference_type = typename gdt::dynarr<T, Allocator>::difference_type;
        using pointer = typename gdt::dynarr<T, Allocator>::pointer;
        using reference = typename gdt::dynarr<T, Allocator>::reference;
    };

    // Dynarr const iterator traits.
    template<typename T, typename Allocator>
    struct iterator_traits<gdt_detail::dynarr_const_iterator<T, Allocator>>
    {
        using iterator_concept = contiguous_iterator_tag;
        using iterator_category = random_access_iterator_tag;
        using value_type = typename gdt::dynarr<T, Allocator>::value_type;
        using difference_type = typename gdt::dynarr<T, Allocator>::difference_type;
        using pointer = typename gdt::dynarr<T, Allocator>::const_pointer;
        using reference = typename gdt::dynarr<T, Allocator>::const_reference;
    };
}
