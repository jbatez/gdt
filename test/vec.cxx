// Copyright Jo Bates 2021.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at:
// https://www.boost.org/LICENSE_1_0.txt

#include <gdt/vec.hxx>

#include <gdt/assert.hxx>

using gdt::all;
using gdt::vec;
using gdt::vec2;
using gdt::vec3;
using gdt::vec4;

consteval int test_consteval()
{
    // Scalar constructor.
    {
        vec3<int> v(123);
        gdt_assert(v.x() == 123);
        gdt_assert(v.y() == 123);
        gdt_assert(v.z() == 123);
    }

    // Component constructor.
    {
        vec v = {1, 2, 3};
        gdt_assert(v.x() == 1);
        gdt_assert(v.y() == 2);
        gdt_assert(v.z() == 3);
    }

    // Component constructor.
    {
        vec v = {1, vec(2, 3)};
        gdt_assert(v.x() == 1);
        gdt_assert(v.y() == 2);
        gdt_assert(v.z() == 3);
    }

    // Conversion/truncation constructor.
    {
        vec3<int> v(vec4<float>(1.1f, 2.2f, 3.3f, 4.4f));
        gdt_assert(v.x() == 1);
        gdt_assert(v.y() == 2);
        gdt_assert(v.z() == 3);
    }

    // Component access.
    {
        vec4<float> v;
        gdt_assert(&v.x() == &v[0]);
        gdt_assert(&v.y() == &v[1]);
        gdt_assert(&v.z() == &v[2]);
        gdt_assert(&v.w() == &v[3]);
        gdt_assert(&v.r() == &v[0]);
        gdt_assert(&v.g() == &v[1]);
        gdt_assert(&v.b() == &v[2]);
        gdt_assert(&v.a() == &v[3]);
    }

    // Component swizzling.
    {
        vec v = {1, 2, 3};
        gdt_assert(all(v.xy() == vec(1, 2)));
        gdt_assert(all(v.yx() == vec(2, 1)));
        gdt_assert(all(v.ggbr() == vec(2, 2, 3, 1)));

        v.set_zx(vec(4, 5));
        gdt_assert(v.z() == 4);
        gdt_assert(v.x() == 5);
    }

    // Operators.
    {
        gdt_assert(all(-vec(1, 2, 3) == vec(-1, -2, -3)));
        gdt_assert(all(1 + vec(2, 3) == vec(3, 4)));
        gdt_assert(all((vec(1, 1) & vec(1, 0)) == vec(1, 0)));
    }

    // Any/all.
    {
        gdt_assert(any(vec(1, 2) != vec(1, 3)));
        gdt_assert(all(vec(1, 2) == vec(1, 2)));
    }

    // Success.
    return 0;
}

int test_vec(int, char** const)
{
    // Consteval.
    gdt_assert(test_consteval() == 0);

    // Math functions.
    {
        auto s = gdt::sin(vec(1.2f, 3.4f));
        gdt_assert(s.x() == gdt::sin(1.2f));
        gdt_assert(s.y() == gdt::sin(3.4f));

        using std::cos;
        auto c = cos(vec(1.2f, 3.4f));
        gdt_assert(c.x() == cos(1.2f));
        gdt_assert(c.y() == cos(3.4f));

        vec2<float> i;
        auto f = gdt::modf(vec(1.25f, 3.5f), &i);
        gdt_assert(all(i == vec(1.0f, 3.0f)));
        gdt_assert(all(f == vec(0.25f, 0.5f)));
    }

    // Success.
    return 0;
}
