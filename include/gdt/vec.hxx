#pragma once

#include <gdt/assume.hxx>
#include <cstddef>

namespace gdt
{
    // Vector.
    template<typename T, std::size_t N> struct vec;
    template<typename T> using vec2 = vec<T, 2>;
    template<typename T> using vec3 = vec<T, 3>;
    template<typename T> using vec4 = vec<T, 4>;

    // Vector.
    template<typename T, std::size_t N>
    struct vec
    {
    public:
        // Constructor.
        constexpr vec() = default;

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
    vec(const T&, const vec3<T>&) -> vec<T, 4>;

    // Deduction guide.
    template<typename T>
    vec(const vec3<T>&, const T&) -> vec<T, 4>;
}
