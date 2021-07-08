#include <gdt/vector.hxx>

#include <gdt/assert.hxx>
#include <memory>
#include <type_traits>

using gdt::vector;

namespace
{
    struct not_always_equal : gdt::allocator<int>
    {
        int id;

        using is_always_equal = std::false_type;
        using propagate_on_container_copy_assignment = std::true_type;

        constexpr not_always_equal select_on_container_copy_construction()
        const
        {
            return {.id = id + 1};
        }

        friend constexpr bool operator==(
            const not_always_equal& lhs,
            const not_always_equal& rhs)
        noexcept
        {
            return lhs.id == rhs.id;
        }
    };

    struct no_propagate : not_always_equal
    {
        using propagate_on_container_copy_assignment = std::false_type;
        using propagate_on_container_move_assignment = std::false_type;
    };
}

consteval int test_consteval()
{
    // Default constructor.
    {
        vector<int> v;
        gdt_assert(v.size() == 0);
    }

    // Size constructor.
    {
        vector<int> v(123);
        gdt_assert(v.size() == 123);
        for (int i : v)
        {
            gdt_assert(i == 0);
        }
    }

    // Fill constructor.
    {
        vector v(123, 45);
        gdt_assert(v.size() == 123);
        for (int i : v)
        {
            gdt_assert(i == 45);
        }
    }

    // Copy range constructor.
    {
        auto il = {1, 2, 3};
        vector v(il.begin(), il.end());
        gdt_assert(v.size() == 3);
        gdt_assert(v[0] == 1);
        gdt_assert(v[1] == 2);
        gdt_assert(v[2] == 3);
    }

    // Copy constructor.
    {
        const vector v1({1, 2, 3}, not_always_equal{.id = 45});
        vector v2 = v1;
        gdt_assert(v2.get_allocator().id == 46);
        gdt_assert(v2.size() == 3);
        gdt_assert(&v2[0] != &v1[0]);
        gdt_assert(v2[0] == 1);
        gdt_assert(v2[1] == 2);
        gdt_assert(v2[2] == 3);
    }

    // Move constructor.
    {
        vector v1 = {1, 2, 3};
        auto data = v1.data();
        vector v2 = std::move(v1);
        gdt_assert(v1.data() == nullptr);
        gdt_assert(v2.data() == data);
        gdt_assert(v1.capacity() == 0);
        gdt_assert(v2.capacity() == 3);
        gdt_assert(v1.size() == 0);
        gdt_assert(v2.size() == 3);
    }

    // Move constructor with always-equal allocator.
    {
        vector v1({1, 2, 3}, gdt::allocator<int>());
        auto data = v1.data();
        vector v2(std::move(v1), gdt::allocator<int>());
        gdt_assert(v1.data() == nullptr);
        gdt_assert(v2.data() == data);
        gdt_assert(v1.capacity() == 0);
        gdt_assert(v2.capacity() == 3);
        gdt_assert(v1.size() == 0);
        gdt_assert(v2.size() == 3);
    }

    // Move constructor with equal allocator.
    {
        vector v1({1, 2, 3}, not_always_equal{.id = 45});
        auto data = v1.data();
        vector v2(std::move(v1), not_always_equal{.id = 45});
        gdt_assert(v2.get_allocator().id == 45);
        gdt_assert(v1.data() == nullptr);
        gdt_assert(v2.data() == data);
        gdt_assert(v1.capacity() == 0);
        gdt_assert(v2.capacity() == 3);
        gdt_assert(v1.size() == 0);
        gdt_assert(v2.size() == 3);
    }

    // Move constructor with non-equal allocator.
    {
        vector v1({1, 2, 3}, not_always_equal{.id = 45});
        auto data = v1.data();
        vector v2(std::move(v1), not_always_equal{.id = 67});
        gdt_assert(v2.get_allocator().id == 67);
        gdt_assert(v1.data() == data);
        gdt_assert(v2.data() != data);
        gdt_assert(v1.capacity() == 3);
        gdt_assert(v2.capacity() == 3);
        gdt_assert(v1.size() == 3);
        gdt_assert(v2.size() == 3);
    }

    // Initializer list constructor.
    {
        vector v = {1, 2, 3};
        gdt_assert(v.size() == 3);
        gdt_assert(v[0] == 1);
        gdt_assert(v[1] == 2);
        gdt_assert(v[2] == 3);
    }

    // Copy assignment with always-equal allocator.
    {
        const vector v1 = {1, 2};
        vector v2 = {3, 4, 5};
        auto data = v2.data();
        v2 = v1;
        gdt_assert(v2.data() == data);
        gdt_assert(v2.capacity() == 3);
        gdt_assert(v2.size() == 2);
        gdt_assert(v2[0] == 1);
        gdt_assert(v2[1] == 2);
    }

    // Copy assignment with equal allocator.
    {
        const vector v1({1, 2}, not_always_equal{.id = 67});
        vector v2({3, 4, 5}, not_always_equal{.id = 67});
        auto data = v2.data();
        v2 = v1;
        gdt_assert(v2.get_allocator().id == 67);
        gdt_assert(v2.data() == data);
        gdt_assert(v2.capacity() == 3);
        gdt_assert(v2.size() == 2);
        gdt_assert(v2[0] == 1);
        gdt_assert(v2[1] == 2);
    }

    // Copy assignment with non-equal allocator.
    {
        const vector v1({1, 2}, not_always_equal{.id = 67});
        vector v2({3, 4, 5}, not_always_equal{.id = 89});
        auto data = v2.data();
        v2 = v1;
        gdt_assert(v2.get_allocator().id == 67);
        gdt_assert(v2.data() != data);
        gdt_assert(v2.capacity() == 2);
        gdt_assert(v2.size() == 2);
        gdt_assert(v2[0] == 1);
        gdt_assert(v2[1] == 2);
    }

    // Copy assignment with equal no-propagate allocator.
    {
        const vector v1({1, 2}, no_propagate{{.id = 67}});
        vector v2({3, 4, 5}, no_propagate{{.id = 67}});
        auto data = v2.data();
        v2 = v1;
        gdt_assert(v2.get_allocator().id == 67);
        gdt_assert(v2.data() == data);
        gdt_assert(v2.capacity() == 3);
        gdt_assert(v2.size() == 2);
        gdt_assert(v2[0] == 1);
        gdt_assert(v2[1] == 2);
    }

    // Copy assignment with non-equal no-propagate allocator.
    {
        const vector v1({1, 2}, no_propagate{{.id = 67}});
        vector v2({3, 4, 5}, no_propagate{{.id = 89}});
        auto data = v2.data();
        v2 = v1;
        gdt_assert(v2.get_allocator().id == 89);
        gdt_assert(v2.data() == data);
        gdt_assert(v2.capacity() == 3);
        gdt_assert(v2.size() == 2);
        gdt_assert(v2[0] == 1);
        gdt_assert(v2[1] == 2);
    }

    // Move assignment with always-equal allocator.
    {
        vector v1 = {1, 2};
        vector v2 = {3, 4, 5};
        auto data = v1.data();
        v2 = std::move(v1);
        gdt_assert(v1.data() == nullptr);
        gdt_assert(v2.data() == data);
        gdt_assert(v1.capacity() == 0);
        gdt_assert(v2.capacity() == 2);
        gdt_assert(v1.size() == 0);
        gdt_assert(v2.size() == 2);
    }

    // Move assignment with equal allocator.
    {
        vector v1({1, 2}, not_always_equal{.id = 67});
        vector v2({3, 4, 5}, not_always_equal{.id = 67});
        auto data = v1.data();
        v2 = std::move(v1);
        gdt_assert(v2.get_allocator().id == 67);
        gdt_assert(v1.data() == nullptr);
        gdt_assert(v2.data() == data);
        gdt_assert(v1.capacity() == 0);
        gdt_assert(v2.capacity() == 2);
        gdt_assert(v1.size() == 0);
        gdt_assert(v2.size() == 2);
    }

    // Move assignment with non-equal allocator.
    {
        vector v1({1, 2}, not_always_equal{.id = 67});
        vector v2({3, 4, 5}, not_always_equal{.id = 89});
        auto data = v1.data();
        v2 = std::move(v1);
        gdt_assert(v2.get_allocator().id == 67);
        gdt_assert(v1.data() == nullptr);
        gdt_assert(v2.data() == data);
        gdt_assert(v1.capacity() == 0);
        gdt_assert(v2.capacity() == 2);
        gdt_assert(v1.size() == 0);
        gdt_assert(v2.size() == 2);
    }

    // Move assignment with equal no-propagate allocator.
    {
        vector v1({1, 2}, no_propagate{{.id = 67}});
        vector v2({3, 4, 5}, no_propagate{{.id = 67}});
        auto data = v1.data();
        v2 = std::move(v1);
        gdt_assert(v2.get_allocator().id == 67);
        gdt_assert(v1.data() == nullptr);
        gdt_assert(v2.data() == data);
        gdt_assert(v1.capacity() == 0);
        gdt_assert(v2.capacity() == 2);
    }

    // Move assignment with non-equal no-propagate allocator.
    {
        vector v1({1, 2}, no_propagate{{.id = 67}});
        vector v2({3, 4, 5}, no_propagate{{.id = 89}});
        auto data1 = v1.data();
        auto data2 = v2.data();
        v2 = std::move(v1);
        gdt_assert(v2.get_allocator().id == 89);
        gdt_assert(v1.data() == data1);
        gdt_assert(v2.data() == data2);
        gdt_assert(v1.capacity() == 2);
        gdt_assert(v2.capacity() == 3);
        gdt_assert(v1.size() == 2);
        gdt_assert(v2.size() == 2);
        gdt_assert(v2[0] == 1);
        gdt_assert(v2[1] == 2);
    }

    // Move assignment with non-equal no-propagate allocator re-allocation.
    {
        vector v1({1, 2, 3}, no_propagate{{.id = 67}});
        vector v2({4, 5}, no_propagate{{.id = 89}});
        auto data1 = v1.data();
        auto data2 = v2.data();
        v2 = std::move(v1);
        gdt_assert(v2.get_allocator().id == 89);
        gdt_assert(v1.data() == data1);
        gdt_assert(v2.data() != data2);
        gdt_assert(v1.capacity() == 3);
        gdt_assert(v2.capacity() == 3);
        gdt_assert(v1.size() == 3);
        gdt_assert(v2.size() == 3);
        gdt_assert(v2[0] == 1);
        gdt_assert(v2[1] == 2);
        gdt_assert(v2[2] == 3);
    }

    // Initializer list assignment.
    {
        vector v = {1, 2, 3};
        v = {4, 5};
        gdt_assert(v.capacity() == 3);
        gdt_assert(v.size() == 2);
        gdt_assert(v[0] == 4);
        gdt_assert(v[1] == 5);
    }

    // Range assign larger.
    {
        vector v = {1, 2};
        auto il = {3, 4, 5};
        v.assign(il.begin(), il.end());
        gdt_assert(v.capacity() == 3);
        gdt_assert(v.size() == 3);
        gdt_assert(v[0] == 3);
        gdt_assert(v[1] == 4);
        gdt_assert(v[2] == 5);
    }

    // Range assign smaller.
    {
        vector v = {1, 2, 3};
        auto il = {4, 5};
        v.assign(il.begin(), il.end());
        gdt_assert(v.capacity() == 3);
        gdt_assert(v.size() == 2);
        gdt_assert(v[0] == 4);
        gdt_assert(v[1] == 5);
    }

    // Fill assign larger.
    {
        vector v = {1, 2};
        v.assign(3, 4);
        gdt_assert(v.capacity() == 3);
        gdt_assert(v.size() == 3);
        gdt_assert(v[0] == 4);
        gdt_assert(v[1] == 4);
        gdt_assert(v[2] == 4);
    }

    // Fill assign smaller.
    {
        vector v = {1, 2, 3};
        v.assign(2, 4);
        gdt_assert(v.capacity() == 3);
        gdt_assert(v.size() == 2);
        gdt_assert(v[0] == 4);
        gdt_assert(v[1] == 4);
    }

    // Initializer list assign.
    {
        vector v = {1, 2, 3};
        v.assign({4, 5});
        gdt_assert(v.capacity() == 3);
        gdt_assert(v.size() == 2);
        gdt_assert(v[0] == 4);
        gdt_assert(v[1] == 5);
    }

    // Get allocator.
    {
        vector v({1, 2, 3}, not_always_equal{.id = 123});
        gdt_assert(v.get_allocator().id == 123);
    }

    // Begin/end.
    {
        vector v = {1, 2, 3};
        auto itr = v.begin();
        gdt_assert(&*itr++ == &v[0]);
        gdt_assert(&*itr++ == &v[1]);
        gdt_assert(&*itr++ == &v[2]);
        gdt_assert(itr == v.end());
    }

    // Const begin/end.
    {
        const vector v = {1, 2, 3};
        auto itr = v.begin();
        gdt_assert(&*itr++ == &v[0]);
        gdt_assert(&*itr++ == &v[1]);
        gdt_assert(&*itr++ == &v[2]);
        gdt_assert(itr == v.end());
    }

    // Reverse begin/end.
    {
        vector v = {1, 2, 3};
        auto itr = v.rbegin();
        gdt_assert(&*itr++ == &v[2]);
        gdt_assert(&*itr++ == &v[1]);
        gdt_assert(&*itr++ == &v[0]);
        gdt_assert(itr == v.rend());
    }

    // Const reverse begin/end.
    {
        const vector v = {1, 2, 3};
        auto itr = v.rbegin();
        gdt_assert(&*itr++ == &v[2]);
        gdt_assert(&*itr++ == &v[1]);
        gdt_assert(&*itr++ == &v[0]);
        gdt_assert(itr == v.rend());
    }

    // Explicitly const iterators.
    {
        vector v = {1, 2, 3};
        gdt_assert(v.cbegin() == v.begin());
        gdt_assert(v.cend() == v.end());
        gdt_assert(v.crbegin() == v.rbegin());
        gdt_assert(v.crend() == v.rend());
    }

    // Empty.
    {
        vector<int> v;
        gdt_assert(v.empty() == true);
        v.push_back(123);
        gdt_assert(v.empty() == false);
        v.pop_back();
        gdt_assert(v.empty() == true);
    }

    // Resize over capacity.
    {
        vector v = {1, 2};
        v.resize(3);
        gdt_assert(v.capacity() == 4);
        gdt_assert(v.size() == 3);
        gdt_assert(v[0] == 1);
        gdt_assert(v[1] == 2);
        gdt_assert(v[2] == 0);
    }

    // Resize within capacity.
    {
        vector v = {1, 2};
        v.reserve(3);
        auto data = v.data();
        v.resize(3);
        gdt_assert(v.data() == data);
        gdt_assert(v.capacity() == 4);
        gdt_assert(v.size() == 3);
        gdt_assert(v[0] == 1);
        gdt_assert(v[1] == 2);
        gdt_assert(v[2] == 0);
    }

    // Resize shrink.
    {
        vector v = {1, 2, 3};
        auto data = v.data();
        v.resize(2);
        gdt_assert(v.data() == data);
        gdt_assert(v.capacity() == 3);
        gdt_assert(v.size() == 2);
        gdt_assert(v[0] == 1);
        gdt_assert(v[1] == 2);
    }

    // Success.
    return 0;
}

int test_vector(int, char** const)
{
    return test_consteval();
}
