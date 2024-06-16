//===--- AvoidConstRefCopyCheck.cpp - clang-tidy --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AvoidConstRefCopyCheck.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang::tidy::performance {

AvoidConstRefCopyCheck::AvoidConstRefCopyCheck(StringRef Name,
                                               ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {}

bool AvoidConstRefCopyCheck::isLanguageVersionSupported(
    const LangOptions &LangOpts) const {
  return LangOpts.CPlusPlus;
}

void AvoidConstRefCopyCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      cxxConstructorDecl(
          forEachConstructorInitializer(cxxCtorInitializer(
              isMemberInitializer(), isWritten(),
              withInitializer(cxxConstructExpr(
                  argumentCountIs(1),
                  hasDeclaration(cxxConstructorDecl(isCopyConstructor())),
                  hasArgument(0, declRefExpr(hasDeclaration(
                                     parmVarDecl(hasType(lValueReferenceType()))
                                         .bind("parameter")))))),
              forField(fieldDecl()))),
          hasParent(cxxRecordDecl().bind("class")))
          .bind("constructor"),
      this);
}

void AvoidConstRefCopyCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Class = Result.Nodes.getNodeAs<CXXRecordDecl>("class");
  const auto *Ctor = Result.Nodes.getNodeAs<CXXConstructorDecl>("constructor");
  const auto *Parameter = Result.Nodes.getNodeAs<ParmVarDecl>("parameter");

  // Check that there is not already an overload with the exact same types
  // except for that one const& argument that is now &&.
  for (const auto *OtherCtor : Class->ctors()) {
    if (OtherCtor == Ctor ||
        OtherCtor->getNumParams() != Ctor->getNumParams()) {
      continue;
    }

    bool IsProperOverload = true;
    for (unsigned i = 0; i < Ctor->getNumParams(); ++i) {
      const auto *Param = Ctor->getParamDecl(i);
      const auto *OtherParam = OtherCtor->getParamDecl(i);

      if (Param != Parameter) {
        if (Param->getType() != OtherParam->getType()) {
          IsProperOverload = false;
          break;
        }

        continue;
      }

      if ((OtherParam->getType()->getTypeClass() != Type::RValueReference) ||
          (Param->getType().getNonReferenceType().getUnqualifiedType() !=
           OtherParam->getType().getNonReferenceType().getUnqualifiedType())) {
        IsProperOverload = false;
        break;
      }
    }

    if (IsProperOverload) {
      return;
    }
  }

  diag(Parameter->getBeginLoc(),
       "constructor takes parameter '%0' by const reference and then copies "
       "it; pass it by value and move or create another overload taking an "
       "rvalue reference")
      << Parameter->getName();
}

} // namespace clang::tidy::performance
