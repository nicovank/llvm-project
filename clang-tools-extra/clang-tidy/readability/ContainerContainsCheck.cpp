//===--- ContainerContainsCheck.cpp - clang-tidy --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ContainerContainsCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang::tidy::readability {

namespace {
struct NotMatchingBoundType {
  NotMatchingBoundType(std::string ArgumentTypeBoundID, DynTypedNode Node)
      : ArgumentTypeBoundID(std::move(ArgumentTypeBoundID)),
        Node(std::move(Node)) {}
  bool operator()(const ast_matchers::internal::BoundNodesMap &Nodes) const {
    const Type *ParamType = Node.get<Type>();
    const Type *ArgType = Nodes.getNodeAs<Type>(ArgumentTypeBoundID);
    if (!ParamType || !ArgType) {
      return true;
    }

    ParamType = ParamType->getUnqualifiedDesugaredType();
    ArgType = ArgType->getUnqualifiedDesugaredType();

    if (ParamType->isReferenceType()) {
      ParamType = ParamType->getPointeeType()->getUnqualifiedDesugaredType();
    }

    while (ParamType->isPointerType()) {
      if (!ArgType->isPointerType()) {
        return true;
      }

      ParamType = ParamType->getPointeeType()->getUnqualifiedDesugaredType();
      ArgType = ArgType->getPointeeType()->getUnqualifiedDesugaredType();
    }

    return ParamType != ArgType;
  }

private:
  std::string ArgumentTypeBoundID;
  DynTypedNode Node;
};

AST_MATCHER_P(Type, matchesBoundType, std::string, ArgumentTypeBoundID) {
  return Builder->removeBindings(
      NotMatchingBoundType(ArgumentTypeBoundID, DynTypedNode::create(Node)));
}
} // namespace

void ContainerContainsCheck::registerMatchers(MatchFinder *Finder) {
  const auto HasContainsMethod = hasMethod(cxxMethodDecl(
      isConst(), parameterCountIs(1), returns(booleanType()),
      hasName("contains"), unless(isDeleted()), isPublic(),
      hasParameter(0, hasType(matchesBoundType("argumentType")))));
  const auto ContainerWithContains = hasType(
      hasUnqualifiedDesugaredType(recordType(hasDeclaration(cxxRecordDecl(anyOf(
          HasContainsMethod, hasAnyBase(hasType(hasCanonicalType(hasDeclaration(
                                 cxxRecordDecl(HasContainsMethod)))))))))));

  // Hack in CountCall and FindCall: hasArgument(0, ...) always ignores implicit
  // casts. We do not want this behavior. We use hasAnyArgument instead, which
  // does not ignore implicit casts. Thankfully, we only have one argument in
  // every case making this possible. Really, hasArgument should also respect
  // the current traversal mode. GitHub issues #54919 and #75754 track this.

  const auto CountCall =
      cxxMemberCallExpr(argumentCountIs(1),
                        callee(cxxMethodDecl(hasName("count"))),
                        hasAnyArgument(hasType(type().bind("argumentType"))),
                        on(ContainerWithContains))
          .bind("call");

  const auto FindCall =
      cxxMemberCallExpr(argumentCountIs(1),
                        callee(cxxMethodDecl(hasName("find"))),
                        hasAnyArgument(hasType(type().bind("argumentType"))),
                        on(ContainerWithContains))
          .bind("call");

  const auto EndCall = cxxMemberCallExpr(argumentCountIs(0),
                                         callee(cxxMethodDecl(hasName("end"))),
                                         on(ContainerWithContains));

  const auto Literal0 = integerLiteral(equals(0));
  const auto Literal1 = integerLiteral(equals(1));

  // Find membership tests which use `count()`.
  Finder->addMatcher(implicitCastExpr(hasImplicitDestinationType(booleanType()),
                                      hasSourceExpression(CountCall))
                         .bind("positiveComparison"),
                     this);
  Finder->addMatcher(
      binaryOperator(hasOperatorName("!="),
                     hasOperands(ignoringParenImpCasts(CountCall),
                                 ignoringParenImpCasts(Literal0)))
          .bind("positiveComparison"),
      this);
  Finder->addMatcher(binaryOperator(hasOperatorName(">"),
                                    hasLHS(ignoringParenImpCasts(CountCall)),
                                    hasRHS(ignoringParenImpCasts(Literal0)))
                         .bind("positiveComparison"),
                     this);
  Finder->addMatcher(binaryOperator(hasOperatorName("<"),
                                    hasLHS(ignoringParenImpCasts(Literal0)),
                                    hasRHS(ignoringParenImpCasts(CountCall)))
                         .bind("positiveComparison"),
                     this);
  Finder->addMatcher(binaryOperator(hasOperatorName(">="),
                                    hasLHS(ignoringParenImpCasts(CountCall)),
                                    hasRHS(ignoringParenImpCasts(Literal1)))
                         .bind("positiveComparison"),
                     this);
  Finder->addMatcher(binaryOperator(hasOperatorName("<="),
                                    hasLHS(ignoringParenImpCasts(Literal1)),
                                    hasRHS(ignoringParenImpCasts(CountCall)))
                         .bind("positiveComparison"),
                     this);

  // Find inverted membership tests which use `count()`.
  Finder->addMatcher(
      binaryOperator(hasOperatorName("=="),
                     hasOperands(ignoringParenImpCasts(CountCall),
                                 ignoringParenImpCasts(Literal0)))
          .bind("negativeComparison"),
      this);
  Finder->addMatcher(binaryOperator(hasOperatorName("<="),
                                    hasLHS(ignoringParenImpCasts(CountCall)),
                                    hasRHS(ignoringParenImpCasts(Literal0)))
                         .bind("negativeComparison"),
                     this);
  Finder->addMatcher(binaryOperator(hasOperatorName(">="),
                                    hasLHS(ignoringParenImpCasts(Literal0)),
                                    hasRHS(ignoringParenImpCasts(CountCall)))
                         .bind("negativeComparison"),
                     this);
  Finder->addMatcher(binaryOperator(hasOperatorName("<"),
                                    hasLHS(ignoringParenImpCasts(CountCall)),
                                    hasRHS(ignoringParenImpCasts(Literal1)))
                         .bind("negativeComparison"),
                     this);
  Finder->addMatcher(binaryOperator(hasOperatorName(">"),
                                    hasLHS(ignoringParenImpCasts(Literal1)),
                                    hasRHS(ignoringParenImpCasts(CountCall)))
                         .bind("negativeComparison"),
                     this);

  // Find membership tests based on `find() == end()`.
  Finder->addMatcher(binaryOperator(hasOperatorName("!="),
                                    hasOperands(ignoringParenImpCasts(FindCall),
                                                ignoringParenImpCasts(EndCall)))
                         .bind("positiveComparison"),
                     this);
  Finder->addMatcher(binaryOperator(hasOperatorName("=="),
                                    hasOperands(ignoringParenImpCasts(FindCall),
                                                ignoringParenImpCasts(EndCall)))
                         .bind("negativeComparison"),
                     this);
}

void ContainerContainsCheck::check(const MatchFinder::MatchResult &Result) {
  // Extract the information about the match
  const auto *Call = Result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
  const auto *PositiveComparison =
      Result.Nodes.getNodeAs<Expr>("positiveComparison");
  const auto *NegativeComparison =
      Result.Nodes.getNodeAs<Expr>("negativeComparison");
  assert((!PositiveComparison || !NegativeComparison) &&
         "only one of PositiveComparison or NegativeComparison should be set");
  bool Negated = NegativeComparison != nullptr;
  const auto *Comparison = Negated ? NegativeComparison : PositiveComparison;

  // Diagnose the issue.
  auto Diag =
      diag(Call->getExprLoc(), "use 'contains' to check for membership");

  // Don't fix it if it's in a macro invocation. Leave fixing it to the user.
  SourceLocation FuncCallLoc = Comparison->getEndLoc();
  if (!FuncCallLoc.isValid() || FuncCallLoc.isMacroID())
    return;

  // Create the fix it.
  const auto *Member = cast<MemberExpr>(Call->getCallee());
  Diag << FixItHint::CreateReplacement(
      Member->getMemberNameInfo().getSourceRange(), "contains");
  SourceLocation ComparisonBegin = Comparison->getSourceRange().getBegin();
  SourceLocation ComparisonEnd = Comparison->getSourceRange().getEnd();
  SourceLocation CallBegin = Call->getSourceRange().getBegin();
  SourceLocation CallEnd = Call->getSourceRange().getEnd();
  Diag << FixItHint::CreateReplacement(
      CharSourceRange::getCharRange(ComparisonBegin, CallBegin),
      Negated ? "!" : "");
  Diag << FixItHint::CreateRemoval(CharSourceRange::getTokenRange(
      CallEnd.getLocWithOffset(1), ComparisonEnd));
}

} // namespace clang::tidy::readability
