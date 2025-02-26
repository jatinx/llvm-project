//===--- Forest.cpp - Parse forest  ------------------------------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang-pseudo/Forest.h"
#include "clang-pseudo/Token.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/FormatVariadic.h"

namespace clang {
namespace pseudo {

std::string ForestNode::dump(const Grammar &G) const {
  switch (kind()) {
  case Ambiguous:
    return llvm::formatv("{0} := <ambiguous>", G.symbolName(symbol()));
  case Terminal:
    return llvm::formatv("{0} := tok[{1}]", G.symbolName(symbol()),
                         startTokenIndex());
  case Sequence:
    return G.dumpRule(rule());
  case Opaque:
    return llvm::formatv("{0} := <opaque>", G.symbolName(symbol()));
  }
  llvm_unreachable("Unhandled node kind!");
}

std::string ForestNode::dumpRecursive(const Grammar &G,
                                      bool Abbreviated) const {
  // Count visits of nodes so we can mark those seen multiple times.
  llvm::DenseMap<const ForestNode *, /*VisitCount*/ unsigned> VisitCounts;
  std::function<void(const ForestNode *)> CountVisits =
      [&](const ForestNode *P) {
        if (VisitCounts[P]++ > 0)
          return; // Don't count children as multiply visited.
        if (P->kind() == Ambiguous)
          llvm::for_each(P->alternatives(), CountVisits);
        else if (P->kind() == Sequence)
          llvm::for_each(P->elements(), CountVisits);
      };
  CountVisits(this);

  // We print a "#<id>" for nonterminal forest nodes that are being dumped
  // multiple times.
  llvm::DenseMap<const ForestNode *, size_t> ReferenceIds;
  std::string Result;
  constexpr Token::Index KEnd = std::numeric_limits<Token::Index>::max();
  std::function<void(const ForestNode *, unsigned, Token::Index,
                     llvm::Optional<SymbolID>)>
      Dump = [&](const ForestNode *P, unsigned Level, Token::Index End,
                 llvm::Optional<SymbolID> ElidedParent) {
        llvm::ArrayRef<const ForestNode *> Children;
        auto EndOfElement = [&](size_t ChildIndex) {
          return ChildIndex + 1 == Children.size()
                     ? End
                     : Children[ChildIndex + 1]->startTokenIndex();
        };
        if (P->kind() == Ambiguous) {
          Children = P->alternatives();
        } else if (P->kind() == Sequence) {
          Children = P->elements();
          if (Abbreviated) {
            if (P->startTokenIndex() == End)
              return;
            for (size_t I = 0; I < Children.size(); ++I)
              if (Children[I]->startTokenIndex() == P->startTokenIndex() &&
                  EndOfElement(I) == End) {
                return Dump(
                    Children[I], Level, End,
                    /*ElidedParent=*/ElidedParent.getValueOr(P->symbol()));
              }
          }
        }

        // FIXME: pretty ascii trees
        if (End == KEnd)
          Result += llvm::formatv("[{0,3}, end) ", P->startTokenIndex());
        else
          Result += llvm::formatv("[{0,3}, {1,3}) ", P->startTokenIndex(), End);
        Result.append(2 * Level, ' ');
        if (ElidedParent.hasValue()) {
          Result += G.symbolName(*ElidedParent);
          Result += "~";
        }
        Result.append(P->dump(G));

        if (VisitCounts.find(P)->getSecond() > 1 &&
            P->kind() != ForestNode::Terminal) {
          // The first time, print as #1. Later, =#1.
          auto It = ReferenceIds.try_emplace(P, ReferenceIds.size() + 1);
          Result +=
              llvm::formatv(" {0}#{1}", It.second ? "" : "=", It.first->second);
        }
        Result.push_back('\n');

        ++Level;
        for (size_t I = 0; I < Children.size(); ++I)
          Dump(Children[I], Level,
               P->kind() == Sequence ? EndOfElement(I) : End, llvm::None);
      };
  Dump(this, 0, KEnd, llvm::None);
  return Result;
}

llvm::ArrayRef<ForestNode>
ForestArena::createTerminals(const TokenStream &Code) {
  ForestNode *Terminals = Arena.Allocate<ForestNode>(Code.tokens().size());
  size_t Index = 0;
  for (const auto &T : Code.tokens()) {
    new (&Terminals[Index])
        ForestNode(ForestNode::Terminal, tokenSymbol(T.Kind),
                   /*Start=*/Index, /*TerminalData*/ 0);
    ++Index;
  }
  NodeCount = Index;
  return llvm::makeArrayRef(Terminals, Index);
}

} // namespace pseudo
} // namespace clang
