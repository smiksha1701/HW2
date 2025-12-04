#include <string>
#include <iostream>
#include <map>

// --- Core Clang LibTooling Includes ---
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

// --- Clang AST Matcher Includes ---
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

// --- Clang Frontend/AST Includes ---
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

// =======================================================================
// Visitor for counting loops in a function body
// =======================================================================
class LoopCounter : public RecursiveASTVisitor<LoopCounter> {
public:
    bool VisitForStmt(ForStmt *S) {
        ++Count;
        return true;
    }

    bool VisitWhileStmt(WhileStmt *S) {
        ++Count;
        return true;
    }

    bool VisitDoStmt(DoStmt *S) {
        ++Count;
        return true;
    }

    unsigned int getCount() const { return Count; }

private:
    unsigned int Count = 0;
};

// =======================================================================
// Visitor for Pass 1: count how many times each function is called
// =======================================================================
class CallVisitor : public RecursiveASTVisitor<CallVisitor> {
public:
    bool VisitCallExpr(CallExpr *Call) {
        if (FunctionDecl *Func = Call->getDirectCallee()) {
            if (!Func->isImplicit()) {
                std::string name =
                    Func->getNameInfo().getName().getAsString();
                CallCounts[name]++;
            }
        }
        return true;
    }

    const std::map<std::string, unsigned int> &getCallCounts() const {
        return CallCounts;
    }

private:
    std::map<std::string, unsigned int> CallCounts;
};

// =======================================================================
// Matcher callback for Pass 2: print stats for each function
// =======================================================================
class FunctionPrinter : public MatchFinder::MatchCallback {
public:
    explicit FunctionPrinter(const std::map<std::string, unsigned int> &Counts)
        : CallCounts(Counts) {}

    void run(const MatchFinder::MatchResult &Result) override {
        const FunctionDecl *Func =
            Result.Nodes.getNodeAs<FunctionDecl>("func");
        if (!Func || !Func->hasBody() || Func->isImplicit())
            return;

        ASTContext *Context = Result.Context;
        SourceManager &SM = Context->getSourceManager();

        // Only consider functions defined in the main file
        if (!SM.isInMainFile(Func->getLocation()))
            return;

        std::string funcName =
            Func->getNameInfo().getName().getAsString();
        unsigned int numParams = Func->getNumParams();

        // Count statements in the body
        unsigned int numStmts = 0;
        if (const auto *CS = dyn_cast<CompoundStmt>(Func->getBody())) {
            numStmts = CS->size();
        }

        // Count loops using the visitor
        LoopCounter loops;
        loops.TraverseStmt(Func->getBody());
        unsigned int numLoops = loops.getCount();

        // Look up call count from the first pass
        unsigned int numCalls = 0;
        auto It = CallCounts.find(funcName);
        if (It != CallCounts.end())
            numCalls = It->second;

        FullSourceLoc fullLocation =
            Context->getFullLoc(Func->getBeginLoc());
        if (fullLocation.isValid()) {
            std::cout << "Function: " << funcName
                      << " | Args: " << numParams
                      << " | Stmts: " << numStmts
                      << " | Loops: " << numLoops
                      << " | Calls: " << numCalls
                      << " | Location: "
                      << fullLocation.getSpellingLineNumber() << ":"
                      << fullLocation.getSpellingColumnNumber()
                      << "\n";
        }
    }

private:
    const std::map<std::string, unsigned int> &CallCounts;
};

// =======================================================================
// ASTConsumer: runs both passes on the translation unit
// =======================================================================
class MyASTConsumer : public ASTConsumer {
public:
    void HandleTranslationUnit(ASTContext &Context) override {
        // ---- Pass 1: build function call counts ----
        CallVisitor callVisitor;
        callVisitor.TraverseDecl(Context.getTranslationUnitDecl());
        const auto &callCounts = callVisitor.getCallCounts();

        // ---- Pass 2: match function declarations and print stats ----
        MatchFinder finder;
        FunctionPrinter printer(callCounts);

        DeclarationMatcher funcMatcher =
            functionDecl(isDefinition()).bind("func");

        finder.addMatcher(funcMatcher, &printer);
        finder.matchAST(Context);
    }
};

// =======================================================================
// FrontendAction: creates our ASTConsumer
// =======================================================================
class MyFrontendAction : public ASTFrontendAction {
public:
    std::unique_ptr<ASTConsumer>
    CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
        (void)CI;  // unused
        (void)file;
        return std::make_unique<MyASTConsumer>();
    }
};

// =======================================================================
// main(): set up CommonOptionsParser + ClangTool
// =======================================================================
static cl::OptionCategory MyToolCategory("FunctionFinder Options");

int main(int argc, const char **argv) {
    auto ExpectedParser =
        CommonOptionsParser::create(argc, argv, MyToolCategory);
    if (!ExpectedParser) {
        cl::PrintHelpMessage();
        return 1;
    }

    CommonOptionsParser &OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(),
                   OptionsParser.getSourcePathList());

    std::cout << "Running tool...\n";
    return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
