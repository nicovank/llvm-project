//===--- AvoidConstRefCopyCheck.h - clang-tidy ------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PERFORMANCE_AVOIDCONSTREFCOPYCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PERFORMANCE_AVOIDCONSTREFCOPYCHECK_H

#include "../ClangTidyCheck.h"

namespace clang::tidy::performance {

/// Finds cases of taking a parameter by const reference and then copying it,
/// without any further usage.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/performance/avoid-const-ref-copy.html
class AvoidConstRefCopyCheck : public ClangTidyCheck {
  // TOOD RENAME AVOID REF COPY!
public:
  AvoidConstRefCopyCheck(StringRef Name, ClangTidyContext *Context);
  bool isLanguageVersionSupported(const LangOptions &LangOpts) const override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace clang::tidy::performance

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PERFORMANCE_AVOIDCONSTREFCOPYCHECK_H
