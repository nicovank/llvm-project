// RUN: %check_clang_tidy -std=c++20 %s modernize-use-starts-ends-with %t -- \
// RUN:   -- -isystem %clang_tidy_headers

#include <string.h>
#include <string>

std::string foo(std::string);
std::string bar();

class sub_string : public std::string {};
class sub_sub_string : public sub_string {};

struct string_like {
  bool starts_with(const char *s) const;
  size_t find(const char *s, size_t pos = 0) const;
};

struct string_like_camel {
  bool startsWith(const char *s) const;
  size_t find(const char *s, size_t pos = 0) const;
};

struct prefer_underscore_version {
  bool starts_with(const char *s) const;
  bool startsWith(const char *s) const;
  size_t find(const char *s, size_t pos = 0) const;
};

struct prefer_underscore_version_flip {
  bool startsWith(const char *s) const;
  bool starts_with(const char *s) const;
  size_t find(const char *s, size_t pos = 0) const;
};

struct prefer_underscore_version_inherit : public string_like {
  bool startsWith(const char *s) const;
};

void test(std::string s, std::string_view sv, sub_string ss, sub_sub_string sss,
          string_like sl, string_like_camel slc, prefer_underscore_version puv,
          prefer_underscore_version_flip puvf,
          prefer_underscore_version_inherit puvi) {

  std::string suffix = "suffix";
  s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
  // CHECK-MESSAGES: :[[@LINE-1]]:{{[0-9]+}}: warning: use ends_with
  // CHECK-FIXES: s.ends_with(suffix);

  // Expressions that don't trigger the check are here.
  #define EQ(x, y) ((x) == (y))
  EQ(s.find("a"), 0);

  #define DOTFIND(x, y) (x).find(y)
  DOTFIND(s, "a") == 0;

  #define STARTS_WITH_COMPARE(x, y) (x).compare(0, (x).size(), (y))
  STARTS_WITH_COMPARE(s, s) == 0;

  s.compare(0, 1, "ab") == 0;
}
