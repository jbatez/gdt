#pragma once

#include "allocator.hxx"
#include <compare>
#include <initializer_list>
#include <iterator>
#include <memory>

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
    class vector {
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
        explicit constexpr vector(const Allocator&) noexcept;

        // Constructor.
        explicit constexpr vector(size_type n, const Allocator& = Allocator());

        // Constructor.
        constexpr vector(
            size_type n,
            const T& value,
            const Allocator& = Allocator()
        );

        // Constructor.
        template<typename InputIterator>
        constexpr vector(
            InputIterator first,
            InputIterator last,
            const Allocator& = Allocator()
        );

        // Constructor.
        constexpr vector(const vector& x);

        // Constructor.
        constexpr vector(vector&&) noexcept;

        // Constructor.
        constexpr vector(const vector&, const Allocator&);

        // Constructor.
        constexpr vector(vector&&, const Allocator&);

        // Constructor.
        constexpr vector(std::initializer_list<T>, const Allocator& = Allocator());

        // Destructor.
        constexpr ~vector();

        // Assignment.
        constexpr vector& operator=(const vector& x);

        // Assignment.
        constexpr vector& operator=(vector&& x)
        noexcept(
            std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value ||
            std::allocator_traits<Allocator>::is_always_equal::value
        );

        // Assignment.
        constexpr vector& operator=(std::initializer_list<T>);

        // Assign.
        template<typename InputIterator>
        constexpr void assign(InputIterator first, InputIterator last);

        // Assign.
        constexpr void assign(size_type n, const T& u);

        // Assign.
        constexpr void assign(std::initializer_list<T>);

        // Get allocator.
        constexpr allocator_type get_allocator() const noexcept;

        // Begin.
        constexpr iterator begin() noexcept;

        // Begin.
        constexpr const_iterator begin() const noexcept;

        // End.
        constexpr iterator end() noexcept;

        // End.
        constexpr const_iterator end() const noexcept;

        // Reverse begin.
        constexpr reverse_iterator rbegin() noexcept;

        // Reverse begin.
        constexpr const_reverse_iterator rbegin() const noexcept;

        // Reverse end.
        constexpr reverse_iterator rend() noexcept;

        // Reverse end.
        constexpr const_reverse_iterator rend() const noexcept;

        // Const begin.
        constexpr const_iterator cbegin() const noexcept;

        // Const end.
        constexpr const_iterator cend() const noexcept;

        // Const reverse begin.
        constexpr const_reverse_iterator crbegin() const noexcept;

        // Const reverse end.
        constexpr const_reverse_iterator crend() const noexcept;

        // Empty?
        [[nodiscard]] constexpr bool empty() const noexcept;

        // Size.
        constexpr size_type size() const noexcept;

        // Max size.
        constexpr size_type max_size() const noexcept;

        // Capacity.
        constexpr size_type capacity() const noexcept;

        // Resize.
        constexpr void resize(size_type sz);

        // Resize.
        constexpr void resize(size_type sz, const T& c);

        // Reserve.
        constexpr void reserve(size_type n);

        // Shrink to fit.
        constexpr void shrink_to_fit();

        // Subscript.
        constexpr reference operator[](size_type n);

        // Subscript.
        constexpr const_reference operator[](size_type n) const;

        // At.
        constexpr const_reference at(size_type n) const;

        // At.
        constexpr reference at(size_type n);

        // Front.
        constexpr reference front();

        // Front.
        constexpr const_reference front() const;

        // Back.
        constexpr reference back();

        // Back.
        constexpr const_reference back() const;

        // Data.
        constexpr T* data() noexcept;

        // Data.
        constexpr const T* data() const noexcept;

        // Emplace back.
        template<typename... Args>
        constexpr reference emplace_back(Args&&... args);

        // Push back.
        constexpr void push_back(const T& x);

        // Push back.
        constexpr void push_back(T&& x);

        // Pop back.
        constexpr void pop_back();

        // Emplace.
        template<typename... Args>
        constexpr iterator emplace(const_iterator position, Args&&... args);

        // Insert.
        constexpr iterator insert(const_iterator position, const T& x);

        // Insert.
        constexpr iterator insert(const_iterator position, T&& x);

        // Insert.
        constexpr iterator insert(
            const_iterator position,
            size_type n,
            const T& x
        );

        // Insert.
        template<typename InputIterator>
        constexpr iterator insert(
            const_iterator position,
            InputIterator first,
            InputIterator last
        );

        // Insert.
        constexpr iterator insert(
            const_iterator position,
            std::initializer_list<T> il
        );

        // Erase.
        constexpr iterator erase(const_iterator position);

        // Erase.
        constexpr iterator erase(const_iterator first, const_iterator last);

        // Swap.
        constexpr void swap(vector&)
        noexcept(
            std::allocator_traits<Allocator>::propagate_on_container_swap::value ||
            std::allocator_traits<Allocator>::is_always_equal::value
        );

        // Clear.
        constexpr void clear() noexcept;
    };

    // Deduction guide.
    template<
        typename InputIterator,
        typename Allocator = allocator<
            typename std::iterator_traits<InputIterator>::value_type
        >
    >
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
    class vector_iterator {
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
            typename vector<T, Allocator>::difference_type n
        ) const {
            return _ptr[n];
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
            typename vector<T, Allocator>::difference_type n
        ) {
            return vector_iterator(a._ptr + n);
        }

        // Addition.
        friend constexpr vector_iterator operator+(
            typename vector<T, Allocator>::difference_type n,
            const vector_iterator& a
        ) {
            return vector_iterator(n + a._ptr);
        }

        // Subtraction.
        friend constexpr vector_iterator operator-(
            const vector_iterator& a,
            typename vector<T, Allocator>::difference_type n
        ) {
            return vector_iterator(a._ptr - n);
        }

        // Subtraction.
        friend constexpr typename vector<T, Allocator>::difference_type
        operator-(const vector_iterator& b, const vector_iterator& a)
        {
            return b._ptr - a._ptr;
        }

        // Addition assignment.
        friend constexpr vector_iterator& operator+=(
            vector_iterator& r,
            typename vector<T, Allocator>::difference_type n
        ) {
            r._ptr += n;
            return *r;
        }

        // Subtraction assignment.
        friend constexpr vector_iterator& operator-=(
            vector_iterator& r,
            typename vector<T, Allocator>::difference_type n
        ) {
            r._ptr -= n;
            return *r;
        }

        // Equality.
        friend constexpr bool operator==(
            const vector_iterator& a,
            const vector_iterator& b
        ) {
            return a == b;
        }

        // Comparison.
        friend constexpr auto operator<=>(
            const vector_iterator& a,
            const vector_iterator& b
        ) {
            return a <=> b;
        }

    private:
        // Pointer.
        typename vector<T, Allocator>::pointer _ptr;

        // Constructor.
        explicit constexpr vector_iterator(
            const typename vector<T, Allocator>::pointer& ptr
        ) noexcept :
            _ptr{ptr}
        {}
    };

    // Vector const iterator.
    template<typename T, typename Allocator>
    class vector_const_iterator {
    public:
        // Constructor.
        constexpr vector_const_iterator() = default;

        // Constructor.
        constexpr vector_const_iterator(
            const vector_iterator<T, Allocator>& a
        ) noexcept :
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
            typename vector<T, Allocator>::difference_type n
        ) const {
            return _ptr[n];
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
            typename vector<T, Allocator>::difference_type n
        ) {
            return vector_const_iterator(a._ptr + n);
        }

        // Addition.
        friend constexpr vector_const_iterator operator+(
            typename vector<T, Allocator>::difference_type n,
            const vector_const_iterator& a
        ) {
            return vector_const_iterator(n + a._ptr);
        }

        // Subtraction.
        friend constexpr vector_const_iterator operator-(
            const vector_const_iterator& a,
            typename vector<T, Allocator>::difference_type n
        ) {
            return vector_const_iterator(a._ptr - n);
        }

        // Subtraction.
        friend constexpr typename vector<T, Allocator>::difference_type
        operator-(
            const vector_const_iterator& b,
            const vector_const_iterator& a
        ) {
            return b._ptr - a._ptr;
        }

        // Addition assignment.
        friend constexpr vector_const_iterator& operator+=(
            vector_const_iterator& r,
            typename vector<T, Allocator>::difference_type n
        ) {
            r._ptr += n;
            return *r;
        }

        // Subtraction assignment.
        friend constexpr vector_const_iterator& operator-=(
            vector_const_iterator& r,
            typename vector<T, Allocator>::difference_type n
        ) {
            r._ptr -= n;
            return *r;
        }

        // Equality.
        friend constexpr bool operator==(
            const vector_const_iterator& a,
            const vector_const_iterator& b
        ) {
            return a == b;
        }

        // Comparison.
        friend constexpr auto operator<=>(
            const vector_const_iterator& a,
            const vector_const_iterator& b
        ) {
            return a <=> b;
        }

    private:
        // Pointer.
        typename vector<T, Allocator>::const_pointer _ptr;

        // Constructor.
        explicit constexpr vector_const_iterator(
            const typename vector<T, Allocator>::const_pointer& ptr
        ) noexcept :
            _ptr{ptr}
        {}
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
