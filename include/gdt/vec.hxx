// Copyright Jo Bates 2021.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at:
// https://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <gdt/assume.hxx>
#include <cmath>
#include <cstddef>
#include <memory>
#include <type_traits>

namespace gdt
{
    // Vector.
    template<typename T, std::size_t N> struct vec;
    template<typename T> using vec2 = vec<T, 2>;
    template<typename T> using vec3 = vec<T, 3>;
    template<typename T> using vec4 = vec<T, 4>;
}

namespace gdt_detail
{
    using namespace gdt;

    // Is vector.
    template<typename T>
    struct is_vec : std::false_type {};

    template<typename T, std::size_t N>
    struct is_vec<vec<T, N>> : std::true_type {};

    template<typename T>
    constexpr bool is_vec_v = is_vec<T>::value;
}

namespace gdt
{
    // Vector.
    template<typename T, std::size_t N>
    struct vec
    {
        // Disallow vectors of vectors.
        static_assert(!gdt_detail::is_vec_v<T>);

    public:
        // Constructor.
        constexpr vec() = default;

        // Constructor.
        explicit constexpr vec(const T& s)
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                _data[i] = s;
            }
        }

        // Constructor.
        constexpr vec(const T& x, const T& y)
        requires (N == 2)
        :
            _data{x, y}
        {}

        // Constructor.
        constexpr vec(const T& x, const T& y, const T& z)
        requires (N == 3)
        :
            _data{x, y, z}
        {}

        // Constructor.
        constexpr vec(const T& x, const vec2<T>& yz)
        requires (N == 3)
        :
            _data{x, yz[0], yz[1]}
        {}

        // Constructor.
        constexpr vec(const vec2<T>& xy, const T& z)
        requires (N == 3)
        :
            _data{xy[0], xy[1], z}
        {}

        // Constructor.
        constexpr vec(const T& x, const T& y, const T& z, const T& w)
        requires (N == 4)
        :
            _data{x, y, z, w}
        {}

        // Constructor.
        constexpr vec(const T& x, const T& y, const vec2<T>& zw)
        requires (N == 4)
        :
            _data{x, y, zw[0], zw[1]}
        {}

        // Constructor.
        constexpr vec(const T& x, const vec2<T>& yz, const T& w)
        requires (N == 4)
        :
            _data{x, yz[0]. yz[1], w}
        {}

        // Constructor.
        constexpr vec(const vec2<T>& xy, const T& z, const T& w)
        requires (N == 4)
        :
            _data{xy[0], xy[1], z, w}
        {}

        // Constructor.
        constexpr vec(const vec2<T>& xy, const vec2<T>& zw)
        requires (N == 4)
        :
            _data{xy[0], xy[1], zw[0], zw[1]}
        {}

        // Constructor.
        constexpr vec(const T& x, const vec3<T>& yzw)
        requires (N == 4)
        :
            _data{x, yzw[0], yzw[1], yzw[2]}
        {}

        // Constructor.
        constexpr vec(const vec3<T>& xyz, const T& w)
        requires (N == 4)
        :
            _data{xyz[0], xyz[1], xyz[2], w}
        {}

        // Constructor.
        template<typename OtherT, std::size_t OtherN>
        explicit constexpr vec(const vec<OtherT, OtherN>& other)
        requires (OtherN >= N)
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                _data[i] = T(other[i]);
            }
        }

        // Subscript.
        constexpr T& operator[](std::size_t i)
        {
            gdt_assume(i <= N);
            return _data[i];
        }

        // Subscript.
        constexpr const T& operator[](std::size_t i) const
        {
            gdt_assume(i <= N);
            return _data[i];
        }

        // Individual component accessors.
        #define gdt(i, I)\
        constexpr T& i() requires (N > I)\
        {\
            return _data[I];\
        }\
        constexpr const T& i() const requires (N > I)\
        {\
            return _data[I];\
        }
        gdt(x, 0)
        gdt(y, 1)
        gdt(z, 2)
        gdt(w, 3)
        gdt(r, 0)
        gdt(g, 1)
        gdt(b, 2)
        gdt(a, 3)
        #undef gdt

        // 2-component getters.
        #define gdt_1(i, j)\
        constexpr vec2<T> i##j() const\
        {\
            return {i(), j()};\
        }
        #define gdt_2(i)\
        gdt_1(i, x)\
        gdt_1(i, y)\
        gdt_1(i, z)\
        gdt_1(i, w)
        gdt_2(x)
        gdt_2(y)
        gdt_2(z)
        gdt_2(w)
        #undef gdt_2
        #define gdt_2(i)\
        gdt_1(i, r)\
        gdt_1(i, g)\
        gdt_1(i, b)\
        gdt_1(i, a)
        gdt_2(r)
        gdt_2(g)
        gdt_2(b)
        gdt_2(a)
        #undef gdt_2
        #undef gdt_1

        // 3-component getters.
        #define gdt_1(i, j, k)\
        constexpr vec3<T> i##j##k() const\
        {\
            return {i(), j(), k()};\
        }
        #define gdt_2(i, j)\
        gdt_1(i, j, x)\
        gdt_1(i, j, y)\
        gdt_1(i, j, z)\
        gdt_1(i, j, w)
        #define gdt_3(i)\
        gdt_2(i, x)\
        gdt_2(i, y)\
        gdt_2(i, z)\
        gdt_2(i, w)
        gdt_3(x)
        gdt_3(y)
        gdt_3(z)
        gdt_3(w)
        #undef gdt_3
        #undef gdt_2
        #define gdt_2(i, j)\
        gdt_1(i, j, r)\
        gdt_1(i, j, g)\
        gdt_1(i, j, b)\
        gdt_1(i, j, a)
        #define gdt_3(i)\
        gdt_2(i, r)\
        gdt_2(i, g)\
        gdt_2(i, b)\
        gdt_2(i, a)
        gdt_3(r)
        gdt_3(g)
        gdt_3(b)
        gdt_3(a)
        #undef gdt_3
        #undef gdt_2
        #undef gdt_1

        // 4-component getters.
        #define gdt_1(i, j, k, l)\
        constexpr vec4<T> i##j##k##l() const \
        {\
            return {i(), j(), k(), l()};\
        }
        #define gdt_2(i, j, k)\
        gdt_1(i, j, k, x)\
        gdt_1(i, j, k, y)\
        gdt_1(i, j, k, z)\
        gdt_1(i, j, k, w)
        #define gdt_3(i, j)\
        gdt_2(i, j, x)\
        gdt_2(i, j, y)\
        gdt_2(i, j, z)\
        gdt_2(i, j, w)
        #define gdt_4(i)\
        gdt_3(i, x)\
        gdt_3(i, y)\
        gdt_3(i, z)\
        gdt_3(i, w)
        gdt_4(x)
        gdt_4(y)
        gdt_4(z)
        gdt_4(w)
        #undef gdt_4
        #undef gdt_3
        #undef gdt_2
        #define gdt_2(i, j, k)\
        gdt_1(i, j, k, r)\
        gdt_1(i, j, k, g)\
        gdt_1(i, j, k, b)\
        gdt_1(i, j, k, a)
        #define gdt_3(i, j)\
        gdt_2(i, j, r)\
        gdt_2(i, j, g)\
        gdt_2(i, j, b)\
        gdt_2(i, j, a)
        #define gdt_4(i)\
        gdt_3(i, r)\
        gdt_3(i, g)\
        gdt_3(i, b)\
        gdt_3(i, a)
        gdt_4(r)
        gdt_4(g)
        gdt_4(b)
        gdt_4(a)
        #undef gdt_4
        #undef gdt_3
        #undef gdt_2
        #undef gdt_1

        // 2-component setters.
        #define gdt(i, j)\
        constexpr void set_##i##j(const vec2<T>& v)\
        {\
            i() = v[0];\
            j() = v[1];\
        }
        gdt(x, y)
        gdt(x, z)
        gdt(x, w)
        gdt(y, x)
        gdt(y, z)
        gdt(y, w)
        gdt(z, x)
        gdt(z, y)
        gdt(z, w)
        gdt(w, x)
        gdt(w, y)
        gdt(w, z)
        gdt(r, g)
        gdt(r, b)
        gdt(r, a)
        gdt(g, r)
        gdt(g, b)
        gdt(g, a)
        gdt(b, r)
        gdt(b, g)
        gdt(b, a)
        gdt(a, r)
        gdt(a, g)
        gdt(a, b)
        #undef gdt

        // 3-component setters.
        #define gdt(i, j, k)\
        constexpr void set_##i##j##k(const vec3<T>& v)\
        {\
            i() = v[0];\
            j() = v[1];\
            k() = v[2];\
        }
        gdt(x, y, z)
        gdt(x, y, w)
        gdt(x, z, y)
        gdt(x, z, w)
        gdt(x, w, y)
        gdt(x, w, z)
        gdt(y, x, z)
        gdt(y, x, w)
        gdt(y, z, x)
        gdt(y, z, w)
        gdt(y, w, x)
        gdt(y, w, z)
        gdt(z, x, y)
        gdt(z, x, w)
        gdt(z, y, x)
        gdt(z, y, w)
        gdt(z, w, x)
        gdt(z, w, y)
        gdt(w, x, y)
        gdt(w, x, z)
        gdt(w, y, x)
        gdt(w, y, z)
        gdt(w, z, x)
        gdt(w, z, y)
        gdt(r, g, b)
        gdt(r, g, a)
        gdt(r, b, g)
        gdt(r, b, a)
        gdt(r, a, g)
        gdt(r, a, b)
        gdt(g, r, b)
        gdt(g, r, a)
        gdt(g, b, r)
        gdt(g, b, a)
        gdt(g, a, r)
        gdt(g, a, b)
        gdt(b, r, g)
        gdt(b, r, a)
        gdt(b, g, r)
        gdt(b, g, a)
        gdt(b, a, r)
        gdt(b, a, g)
        gdt(a, r, g)
        gdt(a, r, b)
        gdt(a, g, r)
        gdt(a, g, b)
        gdt(a, b, r)
        gdt(a, b, g)
        #undef gdt

        // 4-component setters.
        #define gdt(i, j, k, l)\
        constexpr void set_##i##j##k##l(const vec4<T>& v)\
        {\
            i() = v[0];\
            j() = v[1];\
            k() = v[2];\
            l() = v[3];\
        }
        gdt(x, y, z, w)
        gdt(x, y, w, z)
        gdt(x, z, y, w)
        gdt(x, z, w, y)
        gdt(x, w, y, z)
        gdt(x, w, z, y)
        gdt(y, x, z, w)
        gdt(y, x, w, z)
        gdt(y, z, x, w)
        gdt(y, z, w, x)
        gdt(y, w, x, z)
        gdt(y, w, z, x)
        gdt(z, x, y, w)
        gdt(z, x, w, y)
        gdt(z, y, x, w)
        gdt(z, y, w, x)
        gdt(z, w, x, y)
        gdt(z, w, y, x)
        gdt(w, x, y, z)
        gdt(w, x, z, y)
        gdt(w, y, x, z)
        gdt(w, y, z, x)
        gdt(w, z, x, y)
        gdt(w, z, y, x)
        gdt(r, g, b, a)
        gdt(r, g, a, b)
        gdt(r, b, g, a)
        gdt(r, b, a, g)
        gdt(r, a, g, b)
        gdt(r, a, b, g)
        gdt(g, r, b, a)
        gdt(g, r, a, b)
        gdt(g, b, r, a)
        gdt(g, b, a, r)
        gdt(g, a, r, b)
        gdt(g, a, b, r)
        gdt(b, r, g, a)
        gdt(b, r, a, g)
        gdt(b, g, r, a)
        gdt(b, g, a, r)
        gdt(b, a, r, g)
        gdt(b, a, g, r)
        gdt(a, r, g, b)
        gdt(a, r, b, g)
        gdt(a, g, r, b)
        gdt(a, g, b, r)
        gdt(a, b, r, g)
        gdt(a, b, g, r)
        #undef gdt

        // Unary operators.
        #define gdt(op)\
        friend constexpr auto operator op(const vec& v)\
        {\
            vec<decltype(op v[0]), N> ret;\
            for (std::size_t i = 0; i < N; ++i)\
            {\
                ret[i] = op v[i];\
            }\
            return ret;\
        }
        gdt(+)
        gdt(-)
        gdt(~)
        gdt(!)
        #undef gdt

        // Vector-scalar and scalar-vector binary operators.
        #define gdt(op)\
        template<typename U> requires (!gdt_detail::is_vec_v<U>)\
        friend constexpr auto operator op(const vec& lhs, const U& rhs)\
        {\
            vec<decltype(lhs[0] op rhs), N> ret;\
            for (std::size_t i = 0; i < N; ++i)\
            {\
                ret[i] = lhs[i] op rhs;\
            }\
            return ret;\
        }\
        template<typename U> requires (!gdt_detail::is_vec_v<U>)\
        friend constexpr auto operator op(const U& lhs, const vec& rhs)\
        {\
            vec<decltype(lhs op rhs[0]), N> ret;\
            for (std::size_t i = 0; i < N; ++i)\
            {\
                ret[i] = lhs op rhs[i];\
            }\
            return ret;\
        }
        gdt(+)
        gdt(-)
        gdt(*)
        gdt(/)
        gdt(%)
        #undef gdt

        // Vector-vector binary operators.
        #define gdt(op)\
        template<typename U>\
        friend constexpr auto operator op(const vec& lhs, const vec<U, N>& rhs)\
        {\
            vec<decltype(lhs[0] op rhs[0]), N> ret;\
            for (std::size_t i = 0; i < N; ++i)\
            {\
                ret[i] = lhs[i] op rhs[i];\
            }\
            return ret;\
        }
        gdt(+)
        gdt(-)
        gdt(*)
        gdt(/)
        gdt(%)
        gdt(&)
        gdt(|)
        gdt(^)
        gdt(<<)
        gdt(>>)
        gdt(<)
        gdt(>)
        gdt(<=)
        gdt(>=)
        gdt(==)
        gdt(!=)
        gdt(<=>)
        #undef gdt

    private:
        // Member variables.
        T _data[N];
    };

    // Deduction guide.
    template<typename T>
    vec(const T&, const T&) -> vec<T, 2>;

    // Deduction guide.
    template<typename T>
    vec(const T&, const T&, const T&) -> vec<T, 3>;

    // Deduction guide.
    template<typename T>
    vec(const T&, const vec2<T>&) -> vec<T, 3>;

    // Deduction guide.
    template<typename T>
    vec(const vec2<T>&, const T&) -> vec<T, 3>;

    // Deduction guide.
    template<typename T>
    vec(const T&, const T&, const T&, const T&) -> vec<T, 4>;

    // Deduction guide.
    template<typename T>
    vec(const T&, const T&, const vec2<T>&) -> vec<T, 4>;

    // Deduction guide.
    template<typename T>
    vec(const T&, const vec2<T>&, const T&) -> vec<T, 4>;

    // Deduction guide.
    template<typename T>
    vec(const vec2<T>&, const T&, const T&) -> vec<T, 4>;

    // Deduction guide.
    template<typename T>
    vec(const vec2<T>&, const vec2<T>&) -> vec<T, 4>;

    // Deduction guide.
    template<typename T>
    vec(const T&, const vec3<T>&) -> vec<T, 4>;

    // Deduction guide.
    template<typename T>
    vec(const vec3<T>&, const T&) -> vec<T, 4>;

    // Unary math functions.
    #define gdt(func)\
    using std::func;\
    template<typename T, std::size_t N>\
    constexpr auto func(const vec<T, N>& v)\
    {\
        vec<decltype(func(v[0])), N> ret;\
        for (std::size_t i = 0; i < N; ++i)\
        {\
            ret[i] = func(v[i]);\
        }\
        return ret;\
    }
    gdt(acos)
    gdt(asin)
    gdt(atan)
    gdt(cos)
    gdt(sin)
    gdt(tan)
    gdt(acosh)
    gdt(asinh)
    gdt(atanh)
    gdt(cosh)
    gdt(sinh)
    gdt(tanh)
    gdt(exp)
    gdt(exp2)
    gdt(expm1)
    gdt(ilogb)
    gdt(log)
    gdt(log10)
    gdt(log1p)
    gdt(log2)
    gdt(logb)
    gdt(cbrt)
    gdt(abs)
    gdt(fabs)
    gdt(sqrt)
    gdt(erf)
    gdt(erfc)
    gdt(lgamma)
    gdt(tgamma)
    gdt(ceil)
    gdt(floor)
    gdt(nearbyint)
    gdt(rint)
    gdt(lrint)
    gdt(llrint)
    gdt(round)
    gdt(lround)
    gdt(llround)
    gdt(trunc)
    gdt(nan)
    gdt(nanf)
    gdt(nanl)
    gdt(fpclassify)
    gdt(isfinite)
    gdt(isinf)
    gdt(isnan)
    gdt(isnormal)
    gdt(signbit)
    #undef gdt

    // Unary math functions with out params.
    #define gdt(func)\
    using std::func;\
    template<typename T, typename U, std::size_t N>\
    constexpr auto func(const vec<T, N>& v, vec<U, N>* p)\
    {\
        gdt_assume(p != nullptr);\
        vec<decltype(func(v[0], std::addressof((*p)[0]))), N> ret;\
        for (std::size_t i = 0; i < N; ++i)\
        {\
            ret[i] = func(v[i], std::addressof((*p)[i]));\
        }\
        return ret;\
    }
    gdt(frexp)
    gdt(modf)
    #undef gdt

    // Binary math functions.
    #define gdt(func)\
    using std::func;\
    template<typename T, typename U, std::size_t N>\
    constexpr auto func(const vec<T, N>& v1, const vec<U, N>& v2)\
    {\
        vec<decltype(func(v1[0], v2[0])), N> ret;\
        for (std::size_t i = 0; i < N; ++i)\
        {\
            ret[i] = func(v1[i], v2[i]);\
        }\
        return ret;\
    }
    gdt(atan2)
    gdt(ldexp)
    gdt(scalbn)
    gdt(scalbln)
    gdt(hypot)
    gdt(pow)
    gdt(fmod)
    gdt(remainder)
    gdt(copysign)
    gdt(nextafter)
    gdt(nexttoward)
    gdt(fdim)
    gdt(fmax)
    gdt(fmin)
    gdt(isgreater)
    gdt(isgreaterequal)
    gdt(isless)
    gdt(islessequal)
    gdt(islessgreater)
    gdt(isunordered)
    #undef gdt

    // Binary math functions with out params.
    #define gdt(func)\
    using std::func;\
    template<typename T, typename U, typename V, std::size_t N>\
    constexpr auto func(\
        const vec<T, N>& v1,\
        const vec<U, N>& v2,\
        vec<V, N>* p)\
    {\
        gdt_assume(p != nullptr);\
        vec<decltype(func(v1[0], v2[0], std::addressof((*p)[0]))), N> ret;\
        for (std::size_t i = 0; i < N; ++i)\
        {\
            ret[i] = func(v1[i], v2[i], std::addressof((*p)[i]));\
        }\
        return ret;\
    }
    gdt(remquo)
    #undef gdt

    // Ternary math functions.
    #define gdt(func)\
    using std::func;\
    template<typename T, typename U, typename V, std::size_t N>\
    constexpr auto func(\
        const vec<T, N>& v1,\
        const vec<U, N>& v2,\
        const vec<V, N>& v3)\
    {\
        vec<decltype(func(v1[0], v2[0], v3[0])), N> ret;\
        for (std::size_t i = 0; i < N; ++i)\
        {\
            ret[i] = func(v1[i], v2[i], v3[i]);\
        }\
        return ret;\
    }
    gdt(hypot)
    gdt(fma)
    gdt(lerp)
    #undef gdt
}
