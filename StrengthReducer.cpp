#include <cmath>
#include <iostream>
#include <string>

// Core Clang tooling includes
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Core/Replacement.h"

// AST Matchers
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

// Frontend / rewriting
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Lex/Lexer.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;

// ======================================================================
// Helper: check if an integer is a power of two
// ======================================================================
static bool isPowerOfTwo(int64_t x, unsigned &ShiftAmount) {
    if (x <= 0)
        return false;

    // power of two has only one bit set
    if ((x & (x - 1)) != 0)
        return false;

    // compute shift
    ShiftAmount = 0;
    while ((1LL << ShiftAmount) != x)
        ++ShiftAmount;

    return true;
}

// ======================================================================
// Match callback: perform strength reduction on matched binary operators
// ======================================================================
class StrengthReducerCallback : public MatchFinder::MatchCallback {
public:
    explicit StrengthReducerCallback(Rewriter &R) : TheRewriter(R) {}

    void run(const MatchFinder::MatchResult &Result) override {
        // We matched a binary operator bound to "binOp"
        const auto *BinOp =
            Result.Nodes.getNodeAs<BinaryOperator>("binOp");
        const auto *RHS =
            Result.Nodes.getNodeAs<IntegerLiteral>("rhs");
        const auto *LHSExpr =
            Result.Nodes.getNodeAs<Expr>("lhs");

        if (!BinOp || !RHS || !LHSExpr)
            return;

        // Get integer value on RHS
        // Use APInt instead of APSInt, as getValue() returns APInt
        llvm::APInt Value = RHS->getValue();
        int64_t IntVal = Value.getSExtValue();

        unsigned Shift = 0;
        if (!isPowerOfTwo(IntVal, Shift))
            return; // nothing to do

        // Determine the replacement operator string
        std::string OpStr;
        if (BinOp->getOpcode() == BO_Mul) {
            OpStr = " << ";
        } else if (BinOp->getOpcode() == BO_Div) {
            OpStr = " >> ";
        } else {
            return; // Should not happen based on matcher
        }

        // Get source manager and language options
        const SourceManager &SM = *Result.SourceManager;
        const LangOptions &LangOpts = Result.Context->getLangOpts();

        // Get the source text for the LHS expression
        SourceRange LHSRange = LHSExpr->getSourceRange();
        CharSourceRange CharRange =
            CharSourceRange::getTokenRange(LHSRange);

        StringRef LHSCode = Lexer::getSourceText(CharRange, SM, LangOpts);
        if (LHSCode.empty())
            return;

        // Build replacement text: "LHS << Shift" or "LHS >> Shift"
        std::string NewText;
        NewText.reserve(LHSCode.size() + 8);
        NewText += LHSCode;
        NewText += OpStr; 
        NewText += std::to_string(Shift);

        // Replace the whole binary operator expression
        CharSourceRange BinRange =
            CharSourceRange::getTokenRange(BinOp->getSourceRange());
        TheRewriter.ReplaceText(BinRange, NewText);
    }

private:
    Rewriter &TheRewriter;
};

// ======================================================================
// ASTConsumer: sets up the matcher
// ======================================================================
class MyASTConsumer : public ASTConsumer {
public:
    explicit MyASTConsumer(Rewriter &R) : Handler(R) {
        // Match: <expr> * <integer-power-of-two> OR <expr> / <integer-power-of-two>
        auto OpMatcher =
            binaryOperator(
                anyOf(hasOperatorName("*"), hasOperatorName("/")),
                hasLHS(expr().bind("lhs")),
                hasRHS(integerLiteral().bind("rhs")))
            .bind("binOp");

        Finder.addMatcher(OpMatcher, &Handler);
    }

    void HandleTranslationUnit(ASTContext &Context) override {
        Finder.matchAST(Context);
    }

private:
    MatchFinder Finder;
    StrengthReducerCallback Handler;
};

// ======================================================================
// FrontendAction: wraps Rewriter and ASTConsumer
// ======================================================================
class MyFrontendAction : public ASTFrontendAction {
public:
    std::unique_ptr<ASTConsumer>
    CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
        (void)file; // unused

        TheRewriter.setSourceMgr(CI.getSourceManager(),
                                 CI.getLangOpts());
        return std::make_unique<MyASTConsumer>(TheRewriter);
    }

    void EndSourceFileAction() override {
        SourceManager &SM = TheRewriter.getSourceMgr();
        FileID MainFileID = SM.getMainFileID();

        // Print the transformed source to stdout
        TheRewriter.getEditBuffer(MainFileID)
            .write(llvm::outs());
    }

private:
    Rewriter TheRewriter;
};

// ======================================================================
// main: standard ClangTool setup
// ======================================================================
static cl::OptionCategory ToolCategory("StrengthReducer options");

int main(int argc, const char **argv) {
    auto ExpectedParser =
        CommonOptionsParser::create(argc, argv, ToolCategory);
    if (!ExpectedParser) {
        cl::PrintHelpMessage();
        return 1;
    }

    CommonOptionsParser &OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(),
                   OptionsParser.getSourcePathList());

    return Tool.run(
        newFrontendActionFactory<MyFrontendAction>().get());
}
