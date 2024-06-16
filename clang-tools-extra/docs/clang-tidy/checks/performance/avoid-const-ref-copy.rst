.. title:: clang-tidy - performance-avoid-const-ref-copy

performance-avoid-const-ref-copy
================================

Finds cases of taking a parameter by const reference and then copying it,
without any further usage. In such cases, it is almost always better to pass by
value, or to create a second overload taking the parameter by rvalue reference.

Note that in the rvalue reference case, the number of overloads needed to cover
all cases optimally is exponential in the number of parameters.

.. code-block:: c++

  struct S {
    S(const T& a) : a(a) {}
    S(const T& a, const T& b) : a(a), b(b) {}

    T a, b;
  };

could be improved with

.. code-block:: c++

    struct S {
        S(T a) : a(std::move(a)) {}
        S(T a, T b) : a(std::move(a)), b(std::move(b)) {}

        T a, b;
    };

or

.. code-block:: c++

    struct S {
        S(const T& a) : a(a) {}
        S(T&& a) : a(std::move(a)) {}

        S(const T& a, const T& b) : a(a), b(b) {}
        S(T&& a, const T& b) : a(std::move(a)), b(b) {}
        S(const T& a, T&& b) : a(a), b(std::move(b)) {}
        S(T&& a, T&& b) : a(std::move(a)), b(std::move(b)) {}

        T a, b;
    };
