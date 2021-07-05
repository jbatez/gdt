#pragma once

#include "allocator.hxx"
#include "assert.hxx"
#include "assume.hxx"
#include <algorithm>
#include <compare>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace gdt_detail
{
    // Vector iterator.
    template<typename T, typename Allocator>
    class vector_iterator;

    // Vector const iterator.
    template<typename T, typename Allocator>
    class vector_const_iterator;
}

namespace gdt
{
    // Vector.
    template<typename T, typename Allocator = allocator<T>>
    class vector
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
        using iterator = gdt_detail::vector_iterator<T, Allocator>;
        using const_iterator = gdt_detail::vector_const_iterator<T, Allocator>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // Constructor.
        constexpr vector() noexcept(noexcept(Allocator()))
        :
            vector(Allocator())
        {}

        // Constructor.
        explicit constexpr vector(const Allocator& allocator) noexcept
        :
            _allocator{allocator},
            _ptr{nullptr},
            _capacity{0},
            _size{0}
        {}

        // Constructor.
        explicit constexpr vector(
            size_type len,
            const Allocator& allocator = Allocator())
        :
            vector(allocator)
        {
            resize(len);
        }

        // Constructor.
        constexpr vector(
            size_type len,
            const T& fill_value,
            const Allocator& allocator = Allocator())
        :
            vector(allocator)
        {
            assign(len, fill_value);
        }

        // Constructor.
        template<typename InputIterator>
        requires std::is_base_of_v<
            std::input_iterator_tag,
            typename std::iterator_traits<InputIterator>::iterator_category>
        constexpr vector(
            InputIterator first,
            InputIterator last,
            const Allocator& allocator = Allocator())
        :
            vector(allocator)
        {
            assign(first, last);
        }

        // Constructor.
        constexpr vector(const vector& other)
        :
            vector(other, std::allocator_traits<Allocator>::
                select_on_container_copy_construction(other._allocator))
        {}

        // Constructor.
        constexpr vector(vector&& other) noexcept
        :
            _allocator{std::move(other._allocator)},
            _ptr{std::exchange(other._ptr, nullptr)},
            _capacity{std::exchange(other._capacity, 0)},
            _size{std::exchange(other._size, 0)}
        {}

        // Constructor.
        constexpr vector(const vector& other, const Allocator& allocator)
        :
            vector(other.begin(), other.end(), allocator)
        {}

        // Constructor.
        constexpr vector(vector&& other, const Allocator& allocator)
        :
            vector(allocator)
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
        constexpr vector(
            std::initializer_list<T> il,
            const Allocator& allocator = Allocator())
        :
            vector(allocator)
        {
            assign(il);
        }

        // Destructor.
        constexpr ~vector()
        {
            _destroy_all_and_deallocate();
        }

        // Assignment.
        constexpr vector& operator=(const vector& other)
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
                        _reset();
                    }
                }
                _allocator = other._allocator;
            }
            assign(other.begin(), other.end());
            return *this;
        }

        // Assignment.
        constexpr vector& operator=(vector&& other)
        noexcept(
            std::allocator_traits<Allocator>::
                propagate_on_container_move_assignment::value ||
            std::allocator_traits<Allocator>::
                is_always_equal::value)
        {
            if (&other == this)
            {
                return *this;
            }
            else if constexpr (
                std::allocator_traits<Allocator>::
                    propagate_on_container_move_assignment::value)
            {
                _destroy_all_and_deallocate();
                _allocator = std::move(other._allocator);
                _take_buffer(other);
                return *this;
            }
            else if constexpr (
                std::allocator_traits<Allocator>::is_always_equal::value)
            {
                _destroy_all_and_deallocate();
                _take_buffer(other);
                return *this;
            }
            else if (_allocator == other._allocator)
            {
                _destroy_all_and_deallocate();
                _take_buffer(other);
                return *this;
            }
            else
            {
                // Element-wise move since we have a different allocator.
                _reserve_without_migrate(other._size);

                // Assign over existing elements where possible.
                auto src_begin = other.begin();
                auto src_len = std::min(_size, other._size);
                auto src_end = src_begin + difference_type(src_len);
                std::move(src_begin, src_end, begin());

                // Push the rest or erase the old leftovers.
                if (_size < other._size)
                {
                    _push_back_move(src_end, other.end());
                }
                else
                {
                    _erase_after(src_len);
                }

                // Done.
                return *this;
            }
        }

        // Assignment.
        constexpr vector& operator=(std::initializer_list<T> il)
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
                _reserve_without_migrate(u);
            }

            // Assign over existing elements where possible.
            size_type i = 0;
            auto& itr = first;
            for (; i < _size && itr != last; ++i, ++itr)
            {
                (*this)[i] = *itr;
            }

            // Push the rest or erase the old leftovers.
            if (itr != last)
            {
                _push_back(itr, last);
            }
            else
            {
                _erase_after(i);
            }
        }

        // Assign.
        constexpr void assign(size_type tgt_len, const T& fill_value)
        {
            _reserve_without_migrate(tgt_len);

            // Assign over existing elements where possible.
            auto fill_len = difference_type(std::min(_size, tgt_len));
            std::fill_n(begin(), fill_len, fill_value);

            // Push the rest or erase the old leftovers.
            if (_size < tgt_len)
            {
                _fill_to(tgt_len, fill_value);
            }
            else
            {
                _erase_after(tgt_len);
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
            return iterator(_ptr);
        }

        // Begin.
        constexpr const_iterator begin() const noexcept
        {
            return const_iterator(_ptr);
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
                _reallocate(_size);
            }
        }

        // Subscript.
        constexpr reference operator[](size_type i)
        {
            auto d = (std::numeric_limits<difference_type>::max)();
            auto u = std::make_unsigned_t<decltype(d)>(d);
            gdt_assume(i <= u);
            return begin()[difference_type(i)];
        }

        // Subscript.
        constexpr const_reference operator[](size_type i) const
        {
            auto d = (std::numeric_limits<difference_type>::max)();
            auto u = std::make_unsigned_t<decltype(d)>(d);
            gdt_assume(i <= u);
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
            return std::to_address(_ptr);
        }

        // Data.
        constexpr const T* data() const noexcept
        {
            return std::to_address(_ptr);
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
            _size--;
        }

        // Emplace.
        template<typename... Args>
        constexpr iterator emplace(const_iterator position, Args&&... args)
        {
            if (_capacity == _size)
            {
                // Emplace in the middle of migration to a larger buffer.
                auto new_capacity = _choose_next_capacity();
                auto new_ptr = _allocate(new_capacity);
                return _migrate_emplace(
                    new_ptr, new_capacity, position,
                    std::forward<Args>(args)...);
            }
            else
            {
                // Replace an existing element after making a gap.
                return _replace_emplace(position, std::forward<Args>(args)...);
            }
        }

        // Insert.
        constexpr iterator insert(const_iterator position, const T& value)
        {
            return emplace(position, value);
        }

        // Insert.
        constexpr iterator insert(const_iterator position, T&& value)
        {
            return emplace(position, std::move(value));
        }

        // Insert.
        constexpr iterator insert(
            const_iterator position,
            size_type fill_len,
            const T& fill_value)
        {
            auto req_capacity = size_type(_size + fill_len);
            gdt_assert(req_capacity >= fill_len); // Assert no overflow.

            if (_capacity < req_capacity)
            {
                // Insert in the middle of migration to a larger buffer.
                auto new_capacity = _choose_new_capacity(req_capacity);
                auto new_ptr = _allocate(new_capacity);
                return _migrate_insert(
                    new_ptr, new_capacity, position, fill_len, fill_value);
            }
            else
            {
                // Replace existing elements after making a gap.
                return _replace_insert(position, fill_len, fill_value);
            }
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
            if constexpr (std::is_base_of_v<
                std::forward_iterator_tag,
                typename std::iterator_traits<InputIterator>::
                    iterator_category>)
            {
                auto d = std::distance(first, last);
                auto u = std::make_unsigned_t<decltype(d)>(d);
                gdt_assert(u <= max_size());
                auto fill_len = size_type(u);

                auto req_capacity = size_type(_size + fill_len);
                gdt_assert(req_capacity >= fill_len); // Assert no overflow.

                if (_capacity < req_capacity)
                {
                    // Insert in the middle of migration to a larger buffer.
                    auto new_capacity = _choose_new_capacity(req_capacity);
                    auto new_ptr = _allocate(new_capacity);
                    return _migrate_insert(
                        new_ptr, new_capacity, position, fill_len, first);
                }
                else
                {
                    // Replace existing elements after making a gap.
                    return _replace_insert(position, fill_len, first, last);
                }
            }
            else
            {
                // No way to predict how many elements; insert one at a time.
                auto pos_idx = position - begin();
                auto dst_idx = pos_idx;
                for (; first != last; ++first, ++dst_idx)
                {
                    insert(begin() + dst_idx, *first);
                }
                return begin() + pos_idx;
            }
        }

        // Insert.
        constexpr iterator insert(
            const_iterator position,
            std::initializer_list<T> il)
        {
            return insert(position, il.begin(), il.end());
        }

        // Erase.
        constexpr iterator erase(const_iterator position)
        {
            return erase(position, position + 1);
        }

        // Erase.
        constexpr iterator erase(const_iterator first, const_iterator last);

        // Swap.
        constexpr void swap(vector& other)
        noexcept(
            std::allocator_traits<Allocator>::
                propagate_on_container_swap::value ||
            std::allocator_traits<Allocator>::
                is_always_equal::value);

        // Clear.
        constexpr void clear() noexcept
        {
            _destroy(0, _size);
            _size = 0;
        }

    private:
        // Member variables.
        [[no_unique_address]] Allocator _allocator;
        pointer _ptr;
        size_type _capacity;
        size_type _size;

        // Take ownership of another vector's buffer.
        constexpr void _take_buffer(vector& other) noexcept
        {
            _ptr = std::exchange(other._ptr, nullptr);
            _capacity = std::exchange(other._capacity, 0);
            _size = std::exchange(other._size, 0);
        }

        // Choose a new capacity >= `req_capacity`.
        constexpr size_type _choose_new_capacity(size_type req_capacity)
        noexcept
        {
            auto max_capacity = max_size();
            auto capacity_x2 = size_type(_capacity * 2);
            if (capacity_x2 < _capacity || capacity_x2 > max_capacity)
            {
                capacity_x2 = max_capacity;
            }

            return std::max(req_capacity, capacity_x2);
        }

        // Choose the next capacity.
        constexpr size_type _choose_next_capacity()
        {
            auto req_capacity = size_type(_capacity + 1);
            gdt_assert(req_capacity != 0); // Assert no overflow.
            return _choose_new_capacity(req_capacity);
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

        // Deallocate current buffer.
        constexpr void _deallocate() noexcept
        {
            std::allocator_traits<Allocator>::deallocate(
                _allocator, _ptr, _capacity);
        }

        // Construct using allocator.
        template<typename... Args>
        constexpr void _construct(T* p, Args&&... args)
        {
            std::allocator_traits<Allocator>::construct(
                _allocator, p, std::forward<Args>(args)...);
        }

        // Destroy using allocator.
        constexpr void _destroy(T* p)
        {
            std::allocator_traits<Allocator>::destroy(_allocator, p);
        }

        // Move and destroy `n` elements from `src` to `dst`.
        constexpr void _migrate(iterator dst, iterator src, size_type n)
        noexcept // `noexcept` is important here!
        {
            for (; n > 0; ++dst, ++src, --n)
            {
                _construct(std::addressof(*dst), std::move(*src));
                _destroy(std::addressof(*src));
            }
        }

        // Move to a new buffer with the given capacity.
        constexpr void _reallocate(size_type new_capacity)
        {
            gdt_assume(new_capacity >= _size);

            auto new_ptr = _allocate(new_capacity);
            _migrate(iterator(new_ptr), begin(), _size);
            _deallocate();

            _ptr = new_ptr;
            _capacity = new_capacity;
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

        // Reserve at least `tgt_len` capacity.
        // Don't bother to migrate on reallocation.
        constexpr void _reserve_without_migrate(size_type tgt_len)
        {
            if (_capacity < tgt_len)
            {
                _reset();
                reserve(tgt_len);
            }
        }

        // Push back range.
        template<typename InputIterator>
        constexpr void _push_back(InputIterator first, InputIterator last)
        {
            for (; first < last; ++first)
            {
                push_back(*first);
            }
        }

        // Push back move range.
        template<typename InputIterator>
        constexpr void _push_back_move(InputIterator first, InputIterator last)
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

        // Migrate to a new buffer with an emplace in the middle.
        template<typename... Args>
        constexpr iterator _migrate_emplace(
            pointer new_ptr,
            size_type new_capacity,
            const_iterator position,
            Args&&... args)
        noexcept // `noexcept` is important here!
        {
            gdt_assume(new_capacity > _size);

            auto old_begin = begin();
            auto new_begin = iterator(new_ptr);
            auto pos_idx = position - old_begin;

            // Migrate up to position.
            _migrate(new_begin, old_begin, size_type(pos_idx));

            // Emplace at position.
            auto dst = new_begin + pos_idx;
            _construct(std::addressof(*dst), std::forward<Args>(args)...);

            // Migrate after position.
            auto migrate_len = size_type(_size - size_type(pos_idx));
            _migrate(dst + 1, old_begin + pos_idx, migrate_len);

            // Free the old buffer and use the new buffer.
            _deallocate();
            _ptr = new_ptr;
            _capacity = new_capacity;
            _size += 1;

            // Done.
            return dst;
        }

        // Migrate to a new buffer with inserts in the middle.
        constexpr iterator _migrate_insert(
            pointer new_ptr,
            size_type new_capacity,
            const_iterator position,
            size_type fill_len,
            const T& fill_value)
        noexcept // `noexcept` is important here!
        {
            gdt_assume(new_capacity > _size);

            auto old_begin = begin();
            auto new_begin = iterator(new_ptr);
            auto pos_idx = position - old_begin;

            // Migrate up to position.
            _migrate(new_begin, old_begin, size_type(pos_idx));

            // Insert at position.
            auto dst = new_begin + pos_idx;
            auto ret = dst;
            for (; fill_len > 0; --fill_len, ++dst)
            {
                construct(std::addressof(*dst), fill_value);
            }

            // Migrate after position.
            auto migrate_len = size_type(_size - size_type(pos_idx));
            _migrate(dst, old_begin + pos_idx, migrate_len);

            // Free the old buffer and use the new buffer.
            _deallocate();
            _ptr = new_ptr;
            _capacity = new_capacity;
            _size += fill_len;

            // Done.
            return ret;
        }

        // Migrate to a new buffer with inserts in the middle.
        template<typename InputIterator>
        constexpr iterator _migrate_insert(
            pointer new_ptr,
            size_type new_capacity,
            const_iterator position,
            size_type fill_len,
            InputIterator fill_itr)
        noexcept // `noexcept` is important here!
        {
            gdt_assume(new_capacity > _size);

            auto old_begin = begin();
            auto new_begin = iterator(new_ptr);
            auto pos_idx = position - old_begin;

            // Migrate up to position.
            _migrate(new_begin, old_begin, size_type(pos_idx));

            // Insert at position.
            auto dst = new_begin + pos_idx;
            auto ret = dst;
            for (; fill_len > 0; --fill_len, ++dst)
            {
                _construct(std::addressof(*dst), *fill_itr++);
            }

            // Migrate after position.
            auto migrate_len = size_type(_size - size_type(pos_idx));
            _migrate(dst, old_begin + pos_idx, migrate_len);

            // Free the old buffer and use the new buffer.
            _deallocate();
            _ptr = new_ptr;
            _capacity = new_capacity;
            _size += fill_len;

            // Done.
            return ret;
        }

        // Destroy and construct.
        template<typename... Args>
        constexpr void _replace(T* p, Args&&... args)
        noexcept // `noexcept` is important here!
        {
            _destroy(p);
            _construct(p, std::forward<Args>(args)...);
        }

        // Emplace in the existing buffer.
        template<typename... Args>
        constexpr iterator _replace_emplace(
            const_iterator position,
            Args&&... args)
        {
            gdt_assume(_capacity > _size);

            auto old_end = end();
            if (position == old_end)
            {
                // Emplace at the end.
                _construct(
                    std::addressof(*old_end), std::forward<Args>(args)...);
                ++_size;
                return old_end;
            }
            else
            {
                // Move all elements >= `position` back one index.
                auto old_end_m1 = old_end - 1;
                _construct(std::addressof(*old_end), std::move(*old_end_m1));
                ++_size;

                auto beg = begin();
                auto dst = position - beg + beg;
                std::move_backward(dst, old_end_m1, old_end);

                // Replace the element at `position`.
                _replace(std::addressof(*dst), std::forward<Args>(args)...);

                // Done.
                return dst;
            }
        }

        // Insert in the existing buffer.
        constexpr iterator _replace_insert(
            const_iterator position,
            size_type fill_len,
            const T& fill_value)
        noexcept // `noexcept` is important here!
        {
            gdt_assume(size_type(_size + fill_len) >= fill_len);
            auto old_end = end();

            // Move all elements >= `position` back `fill_len` indices.
            auto src = old_end;
            for (auto rem = fill_len; rem > 0 && src > position; --rem)
            {
                --src;
                auto dst = src + fill_len;
                _construct(std::addressof(*dst), std::move(*src));
            }

            auto beg = begin();
            auto pos = position - beg + beg;
            std::move_backward(pos, src, src + fill_len);

            // Copy-construct elements past the end of the old array.
            auto dst = pos + fill_len;
            while (dst > old_end && dst > pos)
            {
                _construct(std::addressof(*--dst), fill_value);
            }

            // Copy-assign elements inside the old array.
            while (dst > pos)
            {
                *--dst = fill_value;
            }

            // Done.
            _size += fill_len;
            return pos;
        }

        // Insert in the existing buffer.
        template<typename InputIterator>
        constexpr iterator _replace_insert(
            const_iterator position,
            size_type fill_len,
            InputIterator fill_begin,
            InputIterator fill_end)
        noexcept // `noexcept` is important here!
        {
            gdt_assume(size_type(_size + fill_len) >= fill_len);
            auto old_end = end();

            // Move all elements >= `position` back `fill_len` indices.
            auto src = old_end;
            for (auto rem = fill_len; rem > 0 && src > position; --rem)
            {
                --src;
                auto dst = src + fill_len;
                _construct(std::addressof(*dst), std::move(*src));
            }

            auto beg = begin();
            auto pos = position - beg + beg;
            std::move_backward(pos, src, src + fill_len);

            if constexpr (std::is_base_of_v<
                std::bidirectional_iterator_tag,
                typename std::iterator_traits<InputIterator>::
                    iterator_category>)
            {
                auto& fill_itr = fill_end;

                // Copy-construct elements past the end of the old array.
                auto dst = pos + fill_len;
                while (dst > old_end && dst > pos)
                {
                    _construct(std::addressof(*--dst), *--fill_itr);
                }

                // Copy-assign elements inside the old array.
                while (dst > pos)
                {
                    *--dst = *--fill_itr;
                }
            }
            else
            {
                auto& fill_itr = fill_begin;

                // Copy-assign elements inside the old array.
                auto dst = pos;
                auto dst_end = pos + fill_len;
                while (dst < old_end && dst < dst_end)
                {
                    *dst++ = *fill_itr++;
                }

                // Copy-construct elements past the end of the old array.
                while (dst < dst_end)
                {
                    _construct(std::addressof(*dst++), *fill_itr++);
                }
            }

            // Done.
            _size += fill_len;
            return pos;
        }

        // Destroy from `i` to `j`.
        constexpr void _destroy(size_type i, size_type j)
        noexcept // `noexcept` is important here!
        {
            gdt_assume(i <= j);
            gdt_assume(j <= _size);

            for (; i < j; ++i)
            {
                _destroy(std::addressof((*this)[i]));
            }
        }

        // Destroy all and deallocate.
        constexpr void _destroy_all_and_deallocate()
        noexcept // `noexcept` is important here!
        {
            _destroy(0, _size);
            _deallocate();
        }

        // Erase elements at the given index and beyond.
        constexpr void _erase_after(size_type i) noexcept
        {
            _destroy(i, _size);
            _size = i;
        }

        // Reset to the default state.
        constexpr void _reset() noexcept
        {
            _destroy_all_and_deallocate();
            _ptr = nullptr;
            _capacity = 0;
            _size = 0;
        }
    };

    // Deduction guide.
    template<
        typename InputIterator,
        typename Allocator = allocator<
            typename std::iterator_traits<InputIterator>::value_type>>
    vector(InputIterator, InputIterator, Allocator = Allocator()) ->
    vector<typename std::iterator_traits<InputIterator>::value_type, Allocator>;

    // Swap.
    template<typename T, typename Allocator>
    constexpr void swap(vector<T, Allocator>& x, vector<T, Allocator>& y)
    noexcept(noexcept(x.swap(y)));
}

namespace gdt_detail
{
    using namespace gdt;

    // Vector iterator.
    template<typename T, typename Allocator>
    class vector_iterator
    {
    public:
        // Constructor.
        constexpr vector_iterator() = default;

        // Dereference.
        constexpr T& operator*() const
        {
            return *_ptr;
        }

        // Member access.
        constexpr typename vector<T, Allocator>::pointer
        operator->() const noexcept
        {
            return _ptr;
        }

        // Subscript.
        constexpr T& operator[](
            typename vector<T, Allocator>::difference_type n)
        const
        {
            return _ptr[_ptr_diff(n)];
        }

        // Pre-increment.
        constexpr vector_iterator& operator++()
        {
            ++_ptr;
            return *this;
        }

        // Pre-decrement.
        constexpr vector_iterator& operator--()
        {
            --_ptr;
            return *this;
        }

        // Post-increment.
        constexpr vector_iterator operator++(int)
        {
            return vector_iterator(_ptr++);
        }

        // Post-decrement.
        constexpr vector_iterator operator--(int)
        {
            return vector_iterator(_ptr--);
        }

        // Addition.
        friend constexpr vector_iterator operator+(
            const vector_iterator& a,
            typename vector<T, Allocator>::difference_type n)
        {
            return vector_iterator(a._ptr + _ptr_diff(n));
        }

        // Addition.
        friend constexpr vector_iterator operator+(
            typename vector<T, Allocator>::difference_type n,
            const vector_iterator& a)
        {
            return vector_iterator(_ptr_diff(n) + a._ptr);
        }

        // Subtraction.
        friend constexpr vector_iterator operator-(
            const vector_iterator& a,
            typename vector<T, Allocator>::difference_type n)
        {
            return vector_iterator(a._ptr - _ptr_diff(n));
        }

        // Subtraction.
        friend constexpr typename vector<T, Allocator>::difference_type
        operator-(const vector_iterator& b, const vector_iterator& a)
        {
            return _vec_diff(b._ptr - a._ptr);
        }

        // Addition assignment.
        friend constexpr vector_iterator& operator+=(
            vector_iterator& r,
            typename vector<T, Allocator>::difference_type n)
        {
            r._ptr += _ptr_diff(n);
            return r;
        }

        // Subtraction assignment.
        friend constexpr vector_iterator& operator-=(
            vector_iterator& r,
            typename vector<T, Allocator>::difference_type n)
        {
            r._ptr -= _ptr_diff(n);
            return r;
        }

        // Equality.
        friend constexpr bool operator==(
            const vector_iterator& a,
            const vector_iterator& b)
        {
            return a._ptr == b._ptr;
        }

        // Comparison.
        friend constexpr auto operator<=>(
            const vector_iterator& a,
            const vector_iterator& b)
        {
            return a._ptr <=> b._ptr;
        }

    private:
        // Friends.
        friend vector<T, Allocator>;
        friend vector_const_iterator<T, Allocator>;

        // Member types.
        using _pointer = typename vector<T, Allocator>::pointer;
        using _pointer_diff = typename std::iterator_traits<_pointer>::difference_type;
        using _vector_diff = typename vector<T, Allocator>::difference_type;

        // Member variables.
        _pointer _ptr;

        // Constructor.
        explicit constexpr vector_iterator(const _pointer& ptr) noexcept
        :
            _ptr{ptr}
        {}

        // Pointer diff from vector diff.
        static constexpr _pointer_diff _ptr_diff(_vector_diff n)
        {
            auto ret = _pointer_diff(n);
            gdt_assume(ret == n);
            return ret;
        }

        // Vector diff from pointer diff.
        static constexpr _vector_diff _vec_diff(_pointer_diff n)
        {
            auto ret = _vector_diff(n);
            gdt_assume(ret == n);
            return ret;
        }
    };

    // Vector const iterator.
    template<typename T, typename Allocator>
    class vector_const_iterator
    {
    public:
        // Constructor.
        constexpr vector_const_iterator() = default;

        // Constructor.
        constexpr vector_const_iterator(
            const vector_iterator<T, Allocator>& a)
        noexcept
        :
            _ptr{a._ptr}
        {}

        // Dereference.
        constexpr const T& operator*() const
        {
            return *_ptr;
        }

        // Member access.
        constexpr typename vector<T, Allocator>::const_pointer
        operator->() const noexcept
        {
            return _ptr;
        }

        // Subscript.
        constexpr const T& operator[](
            typename vector<T, Allocator>::difference_type n)
        const
        {
            return _ptr[_ptr_diff(n)];
        }

        // Pre-increment.
        constexpr vector_const_iterator& operator++()
        {
            ++_ptr;
            return *this;
        }

        // Pre-decrement.
        constexpr vector_const_iterator& operator--()
        {
            --_ptr;
            return *this;
        }

        // Post-increment.
        constexpr vector_const_iterator operator++(int)
        {
            return vector_const_iterator(_ptr++);
        }

        // Post-decrement.
        constexpr vector_const_iterator operator--(int)
        {
            return vector_const_iterator(_ptr--);
        }

        // Addition.
        friend constexpr vector_const_iterator operator+(
            const vector_const_iterator& a,
            typename vector<T, Allocator>::difference_type n)
        {
            return vector_const_iterator(a._ptr + _ptr_diff(n));
        }

        // Addition.
        friend constexpr vector_const_iterator operator+(
            typename vector<T, Allocator>::difference_type n,
            const vector_const_iterator& a)
        {
            return vector_const_iterator(_ptr_diff(n) + a._ptr);
        }

        // Subtraction.
        friend constexpr vector_const_iterator operator-(
            const vector_const_iterator& a,
            typename vector<T, Allocator>::difference_type n)
        {
            return vector_const_iterator(a._ptr - _ptr_diff(n));
        }

        // Subtraction.
        friend constexpr typename vector<T, Allocator>::difference_type
        operator-(
            const vector_const_iterator& b,
            const vector_const_iterator& a)
        {
            return _vec_diff(b._ptr - a._ptr);
        }

        // Addition assignment.
        friend constexpr vector_const_iterator& operator+=(
            vector_const_iterator& r,
            typename vector<T, Allocator>::difference_type n)
        {
            r._ptr += _ptr_diff(n);
            return r;
        }

        // Subtraction assignment.
        friend constexpr vector_const_iterator& operator-=(
            vector_const_iterator& r,
            typename vector<T, Allocator>::difference_type n)
        {
            r._ptr -= _ptr_diff(n);
            return r;
        }

        // Equality.
        friend constexpr bool operator==(
            const vector_const_iterator& a,
            const vector_const_iterator& b)
        {
            return a._ptr == b._ptr;
        }

        // Comparison.
        friend constexpr auto operator<=>(
            const vector_const_iterator& a,
            const vector_const_iterator& b)
        {
            return a._ptr <=> b._ptr;
        }

    private:
        // Friends.
        friend vector<T, Allocator>;

        // Member types.
        using _pointer = typename vector<T, Allocator>::const_pointer;
        using _pointer_diff = typename std::iterator_traits<_pointer>::difference_type;
        using _vector_diff = typename vector<T, Allocator>::difference_type;

        // Member variables.
        _pointer _ptr;

        // Constructor.
        explicit constexpr vector_const_iterator(const _pointer& ptr) noexcept
        :
            _ptr{ptr}
        {}

        // Pointer diff from vector diff.
        static constexpr _pointer_diff _ptr_diff(_vector_diff n)
        {
            auto ret = _pointer_diff(n);
            gdt_assume(ret == n);
            return ret;
        }

        // Vector diff from pointer diff.
        static constexpr _vector_diff _vec_diff(_pointer_diff n)
        {
            auto ret = _vector_diff(n);
            gdt_assume(ret == n);
            return ret;
        }
    };
}

namespace std
{
    // Vector iterator traits.
    template<typename T, typename Allocator>
    struct iterator_traits<gdt_detail::vector_iterator<T, Allocator>>
    {
        using iterator_concept = contiguous_iterator_tag;
        using iterator_category = random_access_iterator_tag;
        using value_type = typename gdt::vector<T, Allocator>::value_type;
        using difference_type = typename gdt::vector<T, Allocator>::difference_type;
        using pointer = typename gdt::vector<T, Allocator>::pointer;
        using reference = typename gdt::vector<T, Allocator>::reference;
    };

    // Vector const iterator traits.
    template<typename T, typename Allocator>
    struct iterator_traits<gdt_detail::vector_const_iterator<T, Allocator>>
    {
        using iterator_concept = contiguous_iterator_tag;
        using iterator_category = random_access_iterator_tag;
        using value_type = typename gdt::vector<T, Allocator>::value_type;
        using difference_type = typename gdt::vector<T, Allocator>::difference_type;
        using pointer = typename gdt::vector<T, Allocator>::const_pointer;
        using reference = typename gdt::vector<T, Allocator>::const_reference;
    };
}
