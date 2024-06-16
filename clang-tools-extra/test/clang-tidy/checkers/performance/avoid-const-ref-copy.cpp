// RUN: %check_clang_tidy %s performance-avoid-const-ref-copy %t

namespace std {
template <typename T>
struct remove_reference {
  typedef T type;
};

template <typename T>
struct remove_reference<T&> {
  typedef T type;
};

template <typename T>
struct remove_reference<T&&> {
  typedef T type;
};

template <typename T>
constexpr typename std::remove_reference<T>::type&& move(T&&);
} // namespace std

struct T {};

struct S1 {
  S1(const T& a) : a(a) {}
  // CHECK-MESSAGES: :[[@LINE-1]]:{{[0-9]+}}: warning: constructor takes parameter 'a' by const reference and then copies it
  S1(const T& a, const T& b) : a(a), b(b) {}
  // CHECK-MESSAGES: :[[@LINE-1]]:{{[0-9]+}}: warning: constructor takes parameter 'a' by const reference and then copies it
  // CHECK-MESSAGES: :[[@LINE-2]]:{{[0-9]+}}: warning: constructor takes parameter 'b' by const reference and then copies it
  T a, b;
};

// Ignore if an overload already exists.
struct S2 {
  S2(const T& a) : a(a) {}
  S2(T&& a) : a(std::move(a)) {}
  T a;
};

// Ignore if passing by value (and copying). That's a different issue.
struct S3 {
  S3(T a) : a(a) {}
  T a;
};

// Passing by non-const reference is also flagged.
struct S4 {
  S4(T& a) : a(a) {}
  // CHECK-MESSAGES: :[[@LINE-1]]:{{[0-9]+}}: warning: constructor takes parameter 'a' by const reference and then copies it
  T a;
};

struct S5 {
  // Only one of the parameters has an rvalue reference overload constructor.
  S5(const T& a, const T& b) : a(a), b(b) {}
  // CHECK-MESSAGES: :[[@LINE-1]]:{{[0-9]+}}: warning: constructor takes parameter 'b' by const reference and then copies it
  S5(T&& a, const T& b) : a(std::move(a)), b(b) {}
  // CHECK-MESSAGES: :[[@LINE-1]]:{{[0-9]+}}: warning: constructor takes parameter 'b' by const reference and then copies it
  T a, b;
};

struct S6 {
  // There is an overload taking both parameters by rvalue reference, but no individual ones.
  S6(const T& a, const T& b) : a(a), b(b) {}
  // CHECK-MESSAGES: :[[@LINE-1]]:{{[0-9]+}}: warning: constructor takes parameter 'a' by const reference and then copies it
  // CHECK-MESSAGES: :[[@LINE-2]]:{{[0-9]+}}: warning: constructor takes parameter 'b' by const reference and then copies it
  S6(T&& a, T&& b) : a(std::move(a)), b(std::move(b)) {}
  T a, b;
};

// Ignore copy constructor.
struct S7 {
  S7(const S7& s) = default;
};

struct S8 {
  S8(const T& a) : a(a) {}
  // CHECK-MESSAGES: :[[@LINE-1]]:{{[0-9]+}}: warning: constructor takes parameter 'a' by const reference and then copies it
  S8(T& a) : a(a) {}
  // CHECK-MESSAGES: :[[@LINE-1]]:{{[0-9]+}}: warning: constructor takes parameter 'a' by const reference and then copies it
  T a;
};

// This most likely won't compile once called, but we can still report.
struct S9 {
  S9(const T& a) : a(a) {}
  // CHECK-MESSAGES: :[[@LINE-1]]:{{[0-9]+}}: warning: constructor takes parameter 'a' by const reference and then copies it
  S9(T a) : a(std::move(a)) {}
  T a;
};
