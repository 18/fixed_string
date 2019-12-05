//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
// Copyright (c) 2019 Krystian Stasiowski (sdkrystian at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/fixed_string
//

#ifndef BOOST_FIXED_STRING_DETAIL_FIXED_STRING_HPP
#define BOOST_FIXED_STRING_DETAIL_FIXED_STRING_HPP

#include <boost/fixed_string/config.hpp>
#include <iterator>
#include <type_traits>
#include <limits>

namespace boost {
namespace fixed_string {
namespace detail {

template<std::size_t, typename, typename>
class fixed_string;

// At minimum an integral type shall not qualify as an iterator (Agustin Berge)
template<class T>
using is_input_iterator =
    std::integral_constant<bool,
        ! std::is_integral<T>::value>;

// Find the smallest width integral type that can hold a value as large as N (Glen Fernandes)
template<std::size_t N>
using smallest_width =
    typename std::conditional<(N <= (std::numeric_limits<unsigned char>::max)()), unsigned char,
    typename std::conditional<(N <= (std::numeric_limits<unsigned short>::max)()), unsigned short,
    typename std::conditional<(N <= (std::numeric_limits<unsigned int>::max)()), unsigned int,
    typename std::conditional<(N <= (std::numeric_limits<unsigned long>::max)()), unsigned long,
    typename std::conditional<(N <= (std::numeric_limits<unsigned long long>::max)()), unsigned long long,
    void>::type>::type>::type>::type>::type;

// Optimization for using the smallest possible type
template<std::size_t N, typename CharT, typename Traits>
class fixed_string_base_zero
{
public:
  BOOST_FIXED_STRING_CPP11_CXPER
  fixed_string_base_zero() noexcept { };

  BOOST_FIXED_STRING_CPP11_CXPER
  fixed_string_base_zero(std::size_t n) noexcept : size_(n) { }

  BOOST_FIXED_STRING_CPP14_CXPER
  CharT*
  data_impl() noexcept
  {
    return data_;
  }

  BOOST_FIXED_STRING_CPP14_CXPER
  CharT const*
  data_impl() const noexcept
  {
    return data_;
  }

  BOOST_FIXED_STRING_CPP11_CXPER
  std::size_t
  size_impl() const noexcept
  {
    return size_;
  }

  BOOST_FIXED_STRING_CPP14_CXPER
  std::size_t
  set_size(std::size_t n) noexcept
  {
    return size_ = n;
  }

  BOOST_FIXED_STRING_CPP14_CXPER
  void
  term_impl() noexcept
  {
    Traits::assign(data_[size_], 0);
  }

  smallest_width<N> size_{0};
#ifdef BOOST_FIXED_STRING_ALLOW_UNINIT_MEM
  CharT data_[N + 1];
#else
  CharT data_[N + 1]{};
#endif
};

// Optimization for when the size is 0
template<typename CharT, typename Traits>
class fixed_string_base_zero<0, CharT, Traits>
{
public:
  BOOST_FIXED_STRING_CPP11_CXPER
  fixed_string_base_zero() noexcept {  }

  BOOST_FIXED_STRING_CPP11_CXPER
  fixed_string_base_zero(std::size_t) noexcept { }

  // not possible to constexpr with the static there
  CharT*
  data_impl() const noexcept
  {
    static CharT null{};
    return &null;
  }

  BOOST_FIXED_STRING_CPP11_CXPER
  std::size_t
  size_impl() const noexcept
  {
    return 0;
  }

  BOOST_FIXED_STRING_CPP11_CXPER
  std::size_t
  set_size(std::size_t) noexcept
  {
    return 0;
  }

  BOOST_FIXED_STRING_CPP14_CXPER
  void
  term_impl() noexcept
  {

  }
};

// Optimization for storing the size in the last element
template<std::size_t N, typename CharT, typename Traits>
class fixed_string_base_null
{
public:
  BOOST_FIXED_STRING_CPP14_CXPER
  fixed_string_base_null() noexcept { set_size(0); }

  BOOST_FIXED_STRING_CPP14_CXPER
  fixed_string_base_null(std::size_t n) noexcept { set_size(n); }

  BOOST_FIXED_STRING_CPP14_CXPER
  CharT*
  data_impl() noexcept
  {
    return data_;
  }

  BOOST_FIXED_STRING_CPP14_CXPER
  CharT const*
  data_impl() const noexcept
  {
    return data_;
  }

  BOOST_FIXED_STRING_CPP11_CXPER
  std::size_t
  size_impl() const noexcept
  {
    return N - data_[N];
  }

  BOOST_FIXED_STRING_CPP14_CXPER
  std::size_t
  set_size(std::size_t n) noexcept
  {
    return data_[N] = (N - n);
  }

  BOOST_FIXED_STRING_CPP14_CXPER
  void
  term_impl() noexcept
  {
    Traits::assign(data_[size_impl()], 0);
  }

#ifdef BOOST_FIXED_STRING_ALLOW_UNINIT_MEM
  CharT data_[N + 1];
#else
  CharT data_[N + 1]{};
#endif
};

//#define BOOST_FIXED_STRING_NO_NULL_OPTIMIZATION

// Decides which size optimization to use
// If the size is zero, the object will have no members
// Otherwise, if CharT can hold the max size of the string, store the size in the last char
// Otherwise, store the size of the string using a member of the smallest type possible
template<std::size_t N, typename CharT, typename Traits>
using optimization_base = 
#ifndef BOOST_FIXED_STRING_NO_NULL_OPTIMIZATION
    typename std::conditional<(N <= (std::numeric_limits<CharT>::max)()) && (N != 0), 
        fixed_string_base_null<N, CharT, Traits>,
        fixed_string_base_zero<N, CharT, Traits>>::type;
#else
    fixed_string_base_zero<N, CharT, Traits>;
#endif

template<typename CharT, typename Traits>
inline
int
lexicographical_compare(
    CharT const* s1, std::size_t n1,
    CharT const* s2, std::size_t n2)
{
    if(n1 < n2)
        return Traits::compare(
            s1, s2, n1) <= 0 ? -1 : 1;
    if(n1 > n2)
        return Traits::compare(
            s1, s2, n2) >= 0 ? 1 : -1;
    return Traits::compare(s1, s2, n1);
}

template<typename CharT, typename Traits>
inline
int
lexicographical_compare(
    basic_string_view<CharT, Traits> s1,
    CharT const* s2, std::size_t n2)
{
    return detail::lexicographical_compare<
        CharT, Traits>(s1.data(), s1.size(), s2, n2);
}

template<std::size_t N, typename CharT, typename Traits >
inline
int
lexicographical_compare(
    const fixed_string<N, CharT, Traits>& s1,
    CharT const* s2, std::size_t n2)
{
    return detail::lexicographical_compare<
        CharT, Traits>(s1.data(), s1.size(), s2, n2);
}

template<typename CharT, typename Traits>
inline
int
lexicographical_compare(
    basic_string_view<CharT, Traits> s1,
    basic_string_view<CharT, Traits> s2)
{
    return detail::lexicographical_compare<CharT, Traits>(
        s1.data(), s1.size(), s2.data(), s2.size());
}

template<std::size_t N, std::size_t M, typename CharT, typename Traits>
inline
int 
lexicographical_compare(
    const fixed_string<N, CharT, Traits>& s1, 
    const fixed_string<M, CharT, Traits>& s2)
{
  return detail::lexicographical_compare<CharT, Traits>(
    s1.data(), s1.size(), s2.data(), s2.size());
}

// Maximum number of characters in the decimal
// representation of a binary number. This includes
// the potential minus sign.
//
inline
std::size_t constexpr
max_digits(std::size_t bytes)
{
    return static_cast<std::size_t>(
        bytes * 2.41) + 1 + 1;
}

template<typename CharT, class Integer, typename Traits>
inline
CharT*
raw_to_string(
    CharT* buf, Integer x, std::true_type)
{
    if(x == 0)
    {
        Traits::assign(*--buf, '0');
        return buf;
    }
    if(x < 0)
    {
        x = -x;
        for(;x > 0; x /= 10)
            Traits::assign(*--buf ,
                "0123456789"[x % 10]);
        Traits::assign(*--buf, '-');
        return buf;
    }
    for(;x > 0; x /= 10)
        Traits::assign(*--buf ,
            "0123456789"[x % 10]);
    return buf;
}

template<typename CharT, class Integer, typename Traits>
inline
CharT*
raw_to_string(
    CharT* buf, Integer x, std::false_type)
{
    if(x == 0)
    {
        *--buf = '0';
        return buf;
    }
    for(;x > 0; x /= 10)
        Traits::assign(*--buf ,
            "0123456789"[x % 10]);
    return buf;
}

template<
    typename CharT,
    class Integer,
    typename Traits = std::char_traits<CharT>>
inline
CharT*
raw_to_string(CharT* last, std::size_t size, Integer i)
{
    BOOST_FIXED_STRING_ASSERT(size >= max_digits(sizeof(Integer)));
    return raw_to_string<CharT, Integer, Traits>(
        last, i, std::is_signed<Integer>{});
}

template<
    typename Traits,
    typename CharT,
    typename ForwardIterator>
inline
ForwardIterator
find_not_of(
  ForwardIterator first, ForwardIterator last, const CharT* str, std::size_t n) noexcept
{
  for (; first != last; ++first)
    if (!Traits::find(str, n, *first))
      return first;
  return last;
}

} // detail
} // fixed_string
} // boost

#endif
