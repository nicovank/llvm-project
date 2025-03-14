//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <vector>

// iterator       begin();
// iterator       end();
// const_iterator begin()  const;
// const_iterator end()    const;
// const_iterator cbegin() const;
// const_iterator cend()   const;

#include <vector>
#include <cassert>
#include <iterator>

#include "test_macros.h"
#include "min_allocator.h"

TEST_CONSTEXPR_CXX20 bool tests() {
  using IterRefT = std::iterator_traits<std::vector<bool>::iterator>::reference;
  ASSERT_SAME_TYPE(IterRefT, std::vector<bool>::reference);

  using ConstIterRefT = std::iterator_traits<std::vector<bool>::const_iterator>::reference;
#if !defined(_LIBCPP_VERSION) || defined(_LIBCPP_ABI_BITSET_VECTOR_BOOL_CONST_SUBSCRIPT_RETURN_BOOL)
  ASSERT_SAME_TYPE(ConstIterRefT, bool);
#else
  ASSERT_SAME_TYPE(ConstIterRefT, std::__bit_const_reference<std::vector<bool> >);
#endif
  {
    typedef bool T;
    typedef std::vector<T> C;
    C c;
    C::iterator i = c.begin();
    C::iterator j = c.end();
    assert(std::distance(i, j) == 0);
    assert(i == j);
  }
  {
    typedef bool T;
    typedef std::vector<T> C;
    const C c;
    C::const_iterator i = c.begin();
    C::const_iterator j = c.end();
    assert(std::distance(i, j) == 0);
    assert(i == j);
  }
  {
    typedef bool T;
    typedef std::vector<T> C;
    C c;
    C::const_iterator i = c.cbegin();
    C::const_iterator j = c.cend();
    assert(std::distance(i, j) == 0);
    assert(i == j);
    assert(i == c.end());
  }
  {
    typedef bool T;
    typedef std::vector<T> C;
    C::iterator i;
    C::const_iterator j;
    (void)i;
    (void)j;
  }
#if TEST_STD_VER >= 11
  {
    typedef bool T;
    typedef std::vector<T, min_allocator<T>> C;
    C c;
    C::iterator i = c.begin();
    C::iterator j = c.end();
    assert(std::distance(i, j) == 0);

    assert(i == j);
    assert(!(i != j));

    assert(!(i < j));
    assert((i <= j));

    assert(!(i > j));
    assert((i >= j));

#  if TEST_STD_VER >= 20
    // P1614 + LWG3352
    std::same_as<std::strong_ordering> decltype(auto) r = i <=> j;
    assert(r == std::strong_ordering::equal);
#  endif
  }
  {
    typedef bool T;
    typedef std::vector<T, min_allocator<T>> C;
    const C c;
    C::const_iterator i = c.begin();
    C::const_iterator j = c.end();
    assert(std::distance(i, j) == 0);

    assert(i == j);
    assert(!(i != j));

    assert(!(i < j));
    assert((i <= j));

    assert(!(i > j));
    assert((i >= j));

#  if TEST_STD_VER >= 20
    // P1614 + LWG3352
    std::same_as<std::strong_ordering> decltype(auto) r = i <=> j;
    assert(r == std::strong_ordering::equal);
#  endif
  }
  {
    typedef bool T;
    typedef std::vector<T, min_allocator<T>> C;
    C c;
    C::const_iterator i = c.cbegin();
    C::const_iterator j = c.cend();
    assert(std::distance(i, j) == 0);
    assert(i == j);
    assert(i == c.end());
  }
  {
    typedef bool T;
    typedef std::vector<T, min_allocator<T>> C;
    C::iterator i;
    C::const_iterator j;
    (void)i;
    (void)j;
  }
#endif
#if TEST_STD_VER > 11
  { // N3644 testing
    std::vector<bool>::iterator ii1{}, ii2{};
    std::vector<bool>::iterator ii4 = ii1;
    std::vector<bool>::const_iterator cii{};
    assert(ii1 == ii2);
    assert(ii1 == ii4);

    assert(!(ii1 != ii2));

    assert((ii1 == cii));
    assert((cii == ii1));
    assert(!(ii1 != cii));
    assert(!(cii != ii1));
    assert(!(ii1 < cii));
    assert(!(cii < ii1));
    assert((ii1 <= cii));
    assert((cii <= ii1));
    assert(!(ii1 > cii));
    assert(!(cii > ii1));
    assert((ii1 >= cii));
    assert((cii >= ii1));
    assert(cii - ii1 == 0);
    assert(ii1 - cii == 0);

#  if TEST_STD_VER >= 20
    // P1614 + LWG3352
    std::same_as<std::strong_ordering> decltype(auto) r1 = ii1 <=> ii2;
    assert(r1 == std::strong_ordering::equal);

    std::same_as<std::strong_ordering> decltype(auto) r2 = cii <=> ii2;
    assert(r2 == std::strong_ordering::equal);
#  endif // TEST_STD_VER > 20
  }
#endif

  return true;
}

int main(int, char**) {
  tests();
#if TEST_STD_VER > 17
  static_assert(tests());
#endif
  return 0;
}
