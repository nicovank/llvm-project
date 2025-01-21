#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/Analysis/PathDiagnostic.h"
#include "clang/Basic/OperatorKinds.h"
#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/MemRegion.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramState_Fwd.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <utility>

using namespace clang;
using namespace ento;

using LastLookupInfo = std::pair<const Expr *, const Expr *>;
REGISTER_MAP_WITH_PROGRAMSTATE(MapLastLookup, const MemRegion *, LastLookupInfo)

namespace {
bool areSameExpr(const ASTContext &Context, const Expr &First,
                 const Expr &Second) {
  llvm::FoldingSetNodeID FirstID, SecondID;
  First.Profile(FirstID, Context, true);
  Second.Profile(SecondID, Context, true);
  return FirstID == SecondID;
}

bool isIterator(QualType Type) {
  // This checker won't work well when key_type has the word 'iterator' in it.
  return Type.getAsString().find("iterator") != std::string::npos;
}

const Expr *extractKeyFromMemberCall(const CXXMemberCall *Call) {
  const StringRef MethodName =
      Call->getOriginExpr()->getMethodDecl()->getName();

  if (MethodName == "at" || MethodName == "contains" || MethodName == "count" ||
      MethodName == "find") {
    return Call->getArgExpr(0)->IgnoreParenImpCasts();
  }

  if (MethodName == "emplace") {
    // TODO: std::make_pair, {}, first argument, std::piecewise_construct.
  }

  if (MethodName == "emplace_hint") {
    // TODO.
  }

  if (MethodName == "erase") {
    if (Call->getNumArgs() == 1 &&
        !isIterator(Call->getArgExpr(0)->getType())) {
      return Call->getArgExpr(0)->IgnoreParenImpCasts();
    }

    return nullptr;
  }

  if (MethodName == "insert") {
    // TODO.
  }

  if (MethodName == "insert_or_assign") {
    if (Call->getNumArgs() == 2) {
      return Call->getArgExpr(0)->IgnoreParenImpCasts();
    }

    // Hinted case.
    if (Call->getNumArgs() == 3) {
      return Call->getArgExpr(1)->IgnoreParenImpCasts();
    }

    return nullptr;
  }

  if (MethodName == "try_emplace") {
    if (!isIterator(Call->getArgExpr(0)->getType())) {
      return Call->getArgExpr(0)->IgnoreParenImpCasts();
    }

    return Call->getNumArgs() != 1 ? Call->getArgExpr(1)->IgnoreParenImpCasts()
                                   : nullptr;
  }

  return nullptr;
}

const Expr *
extractKeyFromMemberOperatorCall(const CXXMemberOperatorCall *Call) {
  assert(Call->getOriginExpr()->getOperator() == clang::OO_Subscript);
  assert(Call->getOriginExpr()->getNumArgs() == 2);
  return Call->getOriginExpr()->getArg(1)->IgnoreParenImpCasts();
}

const Expr *extractKeyFromInstanceCall(const CXXInstanceCall *Call) {
  if (const auto *MC = dyn_cast<CXXMemberCall>(Call)) {
    return extractKeyFromMemberCall(MC);
  }

  if (const auto *MOC = dyn_cast<CXXMemberOperatorCall>(Call)) {
    return extractKeyFromMemberOperatorCall(MOC);
  }

  return nullptr;
}

const Expr *getLastLookupExprIfDoubleLookup(const CheckerContext &Ctx,
                                            const MemRegion *Region,
                                            const Expr *KeyExpr) {
  const LastLookupInfo *Last = Ctx.getState()->get<MapLastLookup>(Region);
  if (Last && areSameExpr(Ctx.getASTContext(), *KeyExpr, *Last->first)) {
    return Last->second;
  }
  return nullptr;
}

ProgramStateRef updateLastLookup(const CheckerContext &Ctx,
                                 const MemRegion *Region, const Expr *KeyExpr,
                                 const Expr *OriginExpr) {
  return Ctx.getState()->set<MapLastLookup>(
      Region, std::make_pair(KeyExpr, OriginExpr));
}

ProgramStateRef clearLastLookup(const CheckerContext &Ctx,
                                const MemRegion *Region) {
  return Ctx.getState()->remove<MapLastLookup>(Region);
}

bool isOnMapType(const CXXInstanceCall &Call) {
  // TODO: Check if the type has a key_type typedef.
  return true;
}

std::optional<std::string> getMethodOrOperatorName(const CallEvent &Call) {
  if (const auto *MemberCall = dyn_cast<CXXMemberCall>(&Call)) {
    return MemberCall->getOriginExpr()->getMethodDecl()->getName().str();
  }

  if (const auto *MemberOperatorCall = dyn_cast<CXXMemberOperatorCall>(&Call)) {
    const OverloadedOperatorKind OperatorKind =
        MemberOperatorCall->getOriginExpr()->getOperator();

    if (OperatorKind == clang::OO_Equal) {
      return "operator=";
    }

    if (OperatorKind == clang::OO_Subscript) {
      return "operator[]";
    }

    // We don't care about other operators.
  }

  return std::nullopt;
}

struct MapDoubleLookupChecker
    : public Checker<check::DeadSymbols, check::PreCall> {
  void checkDeadSymbols(SymbolReaper &SR, CheckerContext &Ctx) const {
    ProgramStateRef State = Ctx.getState();
    for (const auto &[Region, _] : Ctx.getState()->get<MapLastLookup>()) {
      if (!SR.isLiveRegion(Region)) {
        State = State->remove<MapLastLookup>(Region);
      }
    }
    Ctx.addTransition(State);
  }

  void checkPreCall(const CallEvent &Call, CheckerContext &Ctx) const {
    const auto *InstanceCall = dyn_cast<CXXInstanceCall>(&Call);
    if (!InstanceCall) {
      return;
    }

    if (!isOnMapType(*InstanceCall)) {
      return;
    }

    const std::optional<std::string> MethodOrOperatorName =
        getMethodOrOperatorName(Call);
    if (!MethodOrOperatorName.has_value()) {
      return;
    }

    // TODO: swap, extract, merge, equal_range, insert_range.

    const MemRegion *Region = InstanceCall->getCXXThisVal().getAsRegion();
    if (*MethodOrOperatorName == "clear" ||
        *MethodOrOperatorName == "operator=" ||
        *MethodOrOperatorName == "rehash" ||
        *MethodOrOperatorName == "reserve") {
      Ctx.addTransition(clearLastLookup(Ctx, Region));
      // TODO: For operator=, should this set the lookup value to the other?
      return;
    }

    const Expr *KeyExpr = extractKeyFromInstanceCall(InstanceCall);
    if (!KeyExpr) {
      // There was a lookup (or possible lookup), but we couldn't figure out
      // what the key was. Clear it to avoid false positives.
      Ctx.addTransition(clearLastLookup(Ctx, Region));
      return;
    }

    if (const Expr *LastLookupExpr =
            getLastLookupExprIfDoubleLookup(Ctx, Region, KeyExpr)) {
      auto Report = std::make_unique<PathSensitiveBugReport>(
          BT, "key double looked up", Ctx.generateNonFatalErrorNode());
      Report->addRange(Call.getSourceRange());
      Report->addNote(
          "previous lookup was here",
          PathDiagnosticLocation(LastLookupExpr, Ctx.getSourceManager(),
                                 Ctx.getCurrentAnalysisDeclContext()));
      Ctx.emitReport(std::move(Report));
    }

    // TODO: Don't some methods invalidate references?

    Call.getOriginExpr()->getExprLoc().dump(Ctx.getSourceManager());
    llvm::errs() << "setting LastLookup for ";
    Region->dump();
    llvm::errs() << " to ";
    KeyExpr->dump();
    llvm::errs() << "with origin expression ";
    Call.getOriginExpr()->getBeginLoc().dump(Ctx.getSourceManager());
    llvm::errs() << "\n";
    Ctx.addTransition(
        updateLastLookup(Ctx, Region, KeyExpr, Call.getOriginExpr()));
  }

private:
  const BugType BT = BugType(this, "TODO Description");
};
} // namespace

void ento::registerMapDoubleLookupChecker(CheckerManager &mgr) {
  mgr.registerChecker<MapDoubleLookupChecker>();
}

bool ento::shouldRegisterMapDoubleLookupChecker(const CheckerManager &mgr) {
  return true;
}
