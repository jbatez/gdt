#include <gdt/dynarr.hxx>

#include <gdt/assert.hxx>
#include <memory>
#include <type_traits>

using gdt::dynarr;

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
        dynarr<int> a;
        gdt_assert(a.size() == 0);
    }

    // Size constructor.
    {
        dynarr<int> a(123);
        gdt_assert(a.size() == 123);
        for (int i : a)
        {
            gdt_assert(i == 0);
        }
    }

    // Fill constructor.
    {
        dynarr a(123, 45);
        gdt_assert(a.size() == 123);
        for (int i : a)
        {
            gdt_assert(i == 45);
        }
    }

    // Copy range constructor.
    {
        auto il = {1, 2, 3};
        dynarr a(il.begin(), il.end());
        gdt_assert(a.size() == 3);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
        gdt_assert(a[2] == 3);
    }

    // Copy constructor.
    {
        const dynarr a1({1, 2, 3}, not_always_equal{.id = 45});
        dynarr a2 = a1;
        gdt_assert(a2.get_allocator().id == 46);
        gdt_assert(a2.size() == 3);
        gdt_assert(&a2[0] != &a1[0]);
        gdt_assert(a2[0] == 1);
        gdt_assert(a2[1] == 2);
        gdt_assert(a2[2] == 3);
    }

    // Move constructor.
    {
        dynarr a1 = {1, 2, 3};
        auto data = a1.data();
        dynarr a2 = std::move(a1);
        gdt_assert(a1.data() == nullptr);
        gdt_assert(a2.data() == data);
        gdt_assert(a1.capacity() == 0);
        gdt_assert(a2.capacity() == 3);
        gdt_assert(a1.size() == 0);
        gdt_assert(a2.size() == 3);
    }

    // Move constructor with always-equal allocator.
    {
        dynarr a1({1, 2, 3}, gdt::allocator<int>());
        auto data = a1.data();
        dynarr a2(std::move(a1), gdt::allocator<int>());
        gdt_assert(a1.data() == nullptr);
        gdt_assert(a2.data() == data);
        gdt_assert(a1.capacity() == 0);
        gdt_assert(a2.capacity() == 3);
        gdt_assert(a1.size() == 0);
        gdt_assert(a2.size() == 3);
    }

    // Move constructor with equal allocator.
    {
        dynarr a1({1, 2, 3}, not_always_equal{.id = 45});
        auto data = a1.data();
        dynarr a2(std::move(a1), not_always_equal{.id = 45});
        gdt_assert(a2.get_allocator().id == 45);
        gdt_assert(a1.data() == nullptr);
        gdt_assert(a2.data() == data);
        gdt_assert(a1.capacity() == 0);
        gdt_assert(a2.capacity() == 3);
        gdt_assert(a1.size() == 0);
        gdt_assert(a2.size() == 3);
    }

    // Move constructor with non-equal allocator.
    {
        dynarr a1({1, 2, 3}, not_always_equal{.id = 45});
        auto data = a1.data();
        dynarr a2(std::move(a1), not_always_equal{.id = 67});
        gdt_assert(a2.get_allocator().id == 67);
        gdt_assert(a1.data() == data);
        gdt_assert(a2.data() != data);
        gdt_assert(a1.capacity() == 3);
        gdt_assert(a2.capacity() == 3);
        gdt_assert(a1.size() == 3);
        gdt_assert(a2.size() == 3);
    }

    // Initializer list constructor.
    {
        dynarr a = {1, 2, 3};
        gdt_assert(a.size() == 3);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
        gdt_assert(a[2] == 3);
    }

    // Copy assignment with always-equal allocator.
    {
        const dynarr a1 = {1, 2};
        dynarr a2 = {3, 4, 5};
        auto data = a2.data();
        a2 = a1;
        gdt_assert(a2.data() == data);
        gdt_assert(a2.capacity() == 3);
        gdt_assert(a2.size() == 2);
        gdt_assert(a2[0] == 1);
        gdt_assert(a2[1] == 2);
    }

    // Copy assignment with equal allocator.
    {
        const dynarr a1({1, 2}, not_always_equal{.id = 67});
        dynarr a2({3, 4, 5}, not_always_equal{.id = 67});
        auto data = a2.data();
        a2 = a1;
        gdt_assert(a2.get_allocator().id == 67);
        gdt_assert(a2.data() == data);
        gdt_assert(a2.capacity() == 3);
        gdt_assert(a2.size() == 2);
        gdt_assert(a2[0] == 1);
        gdt_assert(a2[1] == 2);
    }

    // Copy assignment with non-equal allocator.
    {
        const dynarr a1({1, 2}, not_always_equal{.id = 67});
        dynarr a2({3, 4, 5}, not_always_equal{.id = 89});
        auto data = a2.data();
        a2 = a1;
        gdt_assert(a2.get_allocator().id == 67);
        gdt_assert(a2.data() != data);
        gdt_assert(a2.capacity() == 2);
        gdt_assert(a2.size() == 2);
        gdt_assert(a2[0] == 1);
        gdt_assert(a2[1] == 2);
    }

    // Copy assignment with equal no-propagate allocator.
    {
        const dynarr a1({1, 2}, no_propagate{{.id = 67}});
        dynarr a2({3, 4, 5}, no_propagate{{.id = 67}});
        auto data = a2.data();
        a2 = a1;
        gdt_assert(a2.get_allocator().id == 67);
        gdt_assert(a2.data() == data);
        gdt_assert(a2.capacity() == 3);
        gdt_assert(a2.size() == 2);
        gdt_assert(a2[0] == 1);
        gdt_assert(a2[1] == 2);
    }

    // Copy assignment with non-equal no-propagate allocator.
    {
        const dynarr a1({1, 2}, no_propagate{{.id = 67}});
        dynarr a2({3, 4, 5}, no_propagate{{.id = 89}});
        auto data = a2.data();
        a2 = a1;
        gdt_assert(a2.get_allocator().id == 89);
        gdt_assert(a2.data() == data);
        gdt_assert(a2.capacity() == 3);
        gdt_assert(a2.size() == 2);
        gdt_assert(a2[0] == 1);
        gdt_assert(a2[1] == 2);
    }

    // Move assignment with always-equal allocator.
    {
        dynarr a1 = {1, 2};
        dynarr a2 = {3, 4, 5};
        auto data = a1.data();
        a2 = std::move(a1);
        gdt_assert(a1.data() == nullptr);
        gdt_assert(a2.data() == data);
        gdt_assert(a1.capacity() == 0);
        gdt_assert(a2.capacity() == 2);
        gdt_assert(a1.size() == 0);
        gdt_assert(a2.size() == 2);
    }

    // Move assignment with equal allocator.
    {
        dynarr a1({1, 2}, not_always_equal{.id = 67});
        dynarr a2({3, 4, 5}, not_always_equal{.id = 67});
        auto data = a1.data();
        a2 = std::move(a1);
        gdt_assert(a2.get_allocator().id == 67);
        gdt_assert(a1.data() == nullptr);
        gdt_assert(a2.data() == data);
        gdt_assert(a1.capacity() == 0);
        gdt_assert(a2.capacity() == 2);
        gdt_assert(a1.size() == 0);
        gdt_assert(a2.size() == 2);
    }

    // Move assignment with non-equal allocator.
    {
        dynarr a1({1, 2}, not_always_equal{.id = 67});
        dynarr a2({3, 4, 5}, not_always_equal{.id = 89});
        auto data = a1.data();
        a2 = std::move(a1);
        gdt_assert(a2.get_allocator().id == 67);
        gdt_assert(a1.data() == nullptr);
        gdt_assert(a2.data() == data);
        gdt_assert(a1.capacity() == 0);
        gdt_assert(a2.capacity() == 2);
        gdt_assert(a1.size() == 0);
        gdt_assert(a2.size() == 2);
    }

    // Move assignment with equal no-propagate allocator.
    {
        dynarr a1({1, 2}, no_propagate{{.id = 67}});
        dynarr a2({3, 4, 5}, no_propagate{{.id = 67}});
        auto data = a1.data();
        a2 = std::move(a1);
        gdt_assert(a2.get_allocator().id == 67);
        gdt_assert(a1.data() == nullptr);
        gdt_assert(a2.data() == data);
        gdt_assert(a1.capacity() == 0);
        gdt_assert(a2.capacity() == 2);
    }

    // Move assignment with non-equal no-propagate allocator.
    {
        dynarr a1({1, 2}, no_propagate{{.id = 67}});
        dynarr a2({3, 4, 5}, no_propagate{{.id = 89}});
        auto data1 = a1.data();
        auto data2 = a2.data();
        a2 = std::move(a1);
        gdt_assert(a2.get_allocator().id == 89);
        gdt_assert(a1.data() == data1);
        gdt_assert(a2.data() == data2);
        gdt_assert(a1.capacity() == 2);
        gdt_assert(a2.capacity() == 3);
        gdt_assert(a1.size() == 2);
        gdt_assert(a2.size() == 2);
        gdt_assert(a2[0] == 1);
        gdt_assert(a2[1] == 2);
    }

    // Move assignment with non-equal no-propagate allocator re-allocation.
    {
        dynarr a1({1, 2, 3}, no_propagate{{.id = 67}});
        dynarr a2({4, 5}, no_propagate{{.id = 89}});
        auto data1 = a1.data();
        auto data2 = a2.data();
        a2 = std::move(a1);
        gdt_assert(a2.get_allocator().id == 89);
        gdt_assert(a1.data() == data1);
        gdt_assert(a2.data() != data2);
        gdt_assert(a1.capacity() == 3);
        gdt_assert(a2.capacity() == 3);
        gdt_assert(a1.size() == 3);
        gdt_assert(a2.size() == 3);
        gdt_assert(a2[0] == 1);
        gdt_assert(a2[1] == 2);
        gdt_assert(a2[2] == 3);
    }

    // Initializer list assignment.
    {
        dynarr a = {1, 2, 3};
        a = {4, 5};
        gdt_assert(a.capacity() == 3);
        gdt_assert(a.size() == 2);
        gdt_assert(a[0] == 4);
        gdt_assert(a[1] == 5);
    }

    // Range assign larger.
    {
        dynarr a = {1, 2};
        auto il = {3, 4, 5};
        a.assign(il.begin(), il.end());
        gdt_assert(a.capacity() == 3);
        gdt_assert(a.size() == 3);
        gdt_assert(a[0] == 3);
        gdt_assert(a[1] == 4);
        gdt_assert(a[2] == 5);
    }

    // Range assign smaller.
    {
        dynarr a = {1, 2, 3};
        auto il = {4, 5};
        a.assign(il.begin(), il.end());
        gdt_assert(a.capacity() == 3);
        gdt_assert(a.size() == 2);
        gdt_assert(a[0] == 4);
        gdt_assert(a[1] == 5);
    }

    // Fill assign larger.
    {
        dynarr a = {1, 2};
        a.assign(3, 4);
        gdt_assert(a.capacity() == 3);
        gdt_assert(a.size() == 3);
        gdt_assert(a[0] == 4);
        gdt_assert(a[1] == 4);
        gdt_assert(a[2] == 4);
    }

    // Fill assign smaller.
    {
        dynarr a = {1, 2, 3};
        a.assign(2, 4);
        gdt_assert(a.capacity() == 3);
        gdt_assert(a.size() == 2);
        gdt_assert(a[0] == 4);
        gdt_assert(a[1] == 4);
    }

    // Initializer list assign.
    {
        dynarr a = {1, 2, 3};
        a.assign({4, 5});
        gdt_assert(a.capacity() == 3);
        gdt_assert(a.size() == 2);
        gdt_assert(a[0] == 4);
        gdt_assert(a[1] == 5);
    }

    // Get allocator.
    {
        dynarr a({1, 2, 3}, not_always_equal{.id = 123});
        gdt_assert(a.get_allocator().id == 123);
    }

    // Begin/end.
    {
        dynarr a = {1, 2, 3};
        auto itr = a.begin();
        gdt_assert(&*itr++ == &a[0]);
        gdt_assert(&*itr++ == &a[1]);
        gdt_assert(&*itr++ == &a[2]);
        gdt_assert(itr == a.end());
    }

    // Const begin/end.
    {
        const dynarr a = {1, 2, 3};
        auto itr = a.begin();
        gdt_assert(&*itr++ == &a[0]);
        gdt_assert(&*itr++ == &a[1]);
        gdt_assert(&*itr++ == &a[2]);
        gdt_assert(itr == a.end());
    }

    // Reverse begin/end.
    {
        dynarr a = {1, 2, 3};
        auto itr = a.rbegin();
        gdt_assert(&*itr++ == &a[2]);
        gdt_assert(&*itr++ == &a[1]);
        gdt_assert(&*itr++ == &a[0]);
        gdt_assert(itr == a.rend());
    }

    // Const reverse begin/end.
    {
        const dynarr a = {1, 2, 3};
        auto itr = a.rbegin();
        gdt_assert(&*itr++ == &a[2]);
        gdt_assert(&*itr++ == &a[1]);
        gdt_assert(&*itr++ == &a[0]);
        gdt_assert(itr == a.rend());
    }

    // Explicitly const iterators.
    {
        dynarr a = {1, 2, 3};
        gdt_assert(a.cbegin() == a.begin());
        gdt_assert(a.cend() == a.end());
        gdt_assert(a.crbegin() == a.rbegin());
        gdt_assert(a.crend() == a.rend());
    }

    // Empty.
    {
        dynarr<int> a;
        gdt_assert(a.empty() == true);
        a.push_back(123);
        gdt_assert(a.empty() == false);
        a.pop_back();
        gdt_assert(a.empty() == true);
    }

    // Resize over capacity.
    {
        dynarr a = {1, 2};
        a.resize(3);
        gdt_assert(a.capacity() == 4);
        gdt_assert(a.size() == 3);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
        gdt_assert(a[2] == 0);
    }

    // Resize within capacity.
    {
        dynarr a = {1, 2};
        a.reserve(3);
        auto data = a.data();
        a.resize(3);
        gdt_assert(a.data() == data);
        gdt_assert(a.capacity() == 4);
        gdt_assert(a.size() == 3);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
        gdt_assert(a[2] == 0);
    }

    // Resize shrink.
    {
        dynarr a = {1, 2, 3};
        auto data = a.data();
        a.resize(2);
        gdt_assert(a.data() == data);
        gdt_assert(a.capacity() == 3);
        gdt_assert(a.size() == 2);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
    }

    // Resize over capacity.
    {
        dynarr a = {1, 2};
        a.resize(3, 4);
        gdt_assert(a.capacity() == 4);
        gdt_assert(a.size() == 3);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
        gdt_assert(a[2] == 4);
    }

    // Resize within capacity.
    {
        dynarr a = {1, 2};
        a.reserve(3);
        auto data = a.data();
        a.resize(3, 4);
        gdt_assert(a.data() == data);
        gdt_assert(a.capacity() == 4);
        gdt_assert(a.size() == 3);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
        gdt_assert(a[2] == 4);
    }

    // Resize shrink.
    {
        dynarr a = {1, 2, 3};
        auto data = a.data();
        a.resize(2, 4);
        gdt_assert(a.data() == data);
        gdt_assert(a.capacity() == 3);
        gdt_assert(a.size() == 2);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
    }

    // Reserve.
    {
        dynarr a = {1, 2, 3};

        a.reserve(45);
        gdt_assert(a.capacity() == 45);
        gdt_assert(a.size() == 3);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
        gdt_assert(a[2] == 3);

        a.reserve(46);
        auto data = a.data();
        gdt_assert(a.capacity() == 90);
        gdt_assert(a.size() == 3);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
        gdt_assert(a[2] == 3);

        a.reserve(47);
        gdt_assert(a.data() == data);
        gdt_assert(a.capacity() == 90);
        gdt_assert(a.size() == 3);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
        gdt_assert(a[2] == 3);
    }

    // Shrink to fit.
    {
        dynarr a = {1, 2, 3};

        auto data = a.data();
        a.shrink_to_fit();
        gdt_assert(a.data() == data);
        gdt_assert(a.capacity() == 3);
        gdt_assert(a.size() == 3);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
        gdt_assert(a[2] == 3);

        a.reserve(45);
        a.shrink_to_fit();
        gdt_assert(a.capacity() == 3);
        gdt_assert(a.size() == 3);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
        gdt_assert(a[2] == 3);
    }

    // At.
    {
        dynarr a = {1, 2, 3};
        gdt_assert(&a.at(0) == &a[0]);
        gdt_assert(&a.at(1) == &a[1]);
        gdt_assert(&a.at(2) == &a[2]);
    }

    // Const at.
    {
        const dynarr a = {1, 2, 3};
        gdt_assert(&a.at(0) == &a[0]);
        gdt_assert(&a.at(1) == &a[1]);
        gdt_assert(&a.at(2) == &a[2]);
    }

    // Front.
    {
        dynarr a = {1, 2, 3};
        gdt_assert(&a.front() == &a[0]);
    }

    // Const front.
    {
        const dynarr a = {1, 2, 3};
        gdt_assert(&a.front() == &a[0]);
    }

    // Back.
    {
        dynarr a = {1, 2, 3};
        gdt_assert(&a.back() == &a[2]);
    }

    // Const back.
    {
        const dynarr a = {1, 2, 3};
        gdt_assert(&a.back() == &a[2]);
    }

    // Emplace back.
    {
        dynarr a = {1, 2};

        a.emplace_back();
        gdt_assert(a.capacity() == 4);
        gdt_assert(a.size() == 3);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
        gdt_assert(a[2] == 0);

        auto data = a.data();
        a.emplace_back(4);
        gdt_assert(a.data() == data);
        gdt_assert(a.capacity() == 4);
        gdt_assert(a.size() == 4);
        gdt_assert(a[0] == 1);
        gdt_assert(a[1] == 2);
        gdt_assert(a[2] == 0);
        gdt_assert(a[3] == 4);
    }

    // Push back.
    {
        const dynarr a1 = {1, 2};
        dynarr a2 = {{dynarr{3, 4}}};
        a2.push_back(a1);
        gdt_assert((a1 == dynarr{1, 2}));
        gdt_assert((a2 == dynarr{dynarr{3, 4}, dynarr{1, 2}}));
    }

    // Push back move.
    {
        dynarr a1 = {1, 2};
        dynarr a2 = {{dynarr{3, 4}}};
        auto data = a1.data();
        a2.push_back(std::move(a1));
        gdt_assert((a1.data() == nullptr));
        gdt_assert((a2 == dynarr{dynarr{3, 4}, dynarr{1, 2}}));
        gdt_assert((a2[1].data() == data));
    }

    // TODO: More.

    // Success.
    return 0;
}

int test_dynarr(int, char** const)
{
    return test_consteval();
}
