#include "UrduPreprocessor.h"
#include "UrduDiagConsumer.h"
#include "UrduKeywordMap.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace {

class UrduFrontendAction : public PluginASTAction {
public:
    UrduFrontendAction() : preprocessor_() {}

protected:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& ci,
                                                    llvm::StringRef) override {
        // The AST is not needed; this action only transforms the source.
        return std::make_unique<ASTConsumer>();
    }

    bool BeginSourceFileAction(CompilerInstance& ci) override {
        SourceManager& sm = ci.getSourceManager();
        FileID mainFileID = sm.getMainFileID();

        auto buffer = sm.getBufferOrNone(mainFileID);
        if (!buffer) {
            llvm::errs() << "خطا: سورس فائل نہیں پڑھی جا سکی\n";
            return false;
        }

        std::string transformed = preprocessor_.transform(buffer->getBuffer());

        auto newBuffer = llvm::MemoryBuffer::getMemBufferCopy(
            transformed,
            sm.getFileEntryRefForID(mainFileID)->getName()
        );
        ci.getPreprocessorOpts().addRemappedFile(
            sm.getFileEntryRefForID(mainFileID)->getName().str(),
            newBuffer.release()
        );

        auto* diagConsumer = new urdu::DiagConsumer(
            llvm::errs(),
            &preprocessor_,
            &ci.getDiagnosticOpts()
        );
        ci.getDiagnostics().setClient(diagConsumer, /*ShouldOwnClient=*/true);

        return true;
    }

    bool ParseArgs(const CompilerInstance& ci,
                   const std::vector<std::string>& args) override {
        for (const auto& arg : args) {
            if (arg == "-help") {
                llvm::errs() << "اردو-کلینگ: اردو میں سی پروگرامنگ\n";
                llvm::errs() << "Urdu-Clang: C programming in Urdu\n";
            }
        }
        return true;
    }

    ActionType getActionType() override {
        return AddBeforeMainAction;
    }

private:
    urdu::Preprocessor preprocessor_;
};

} // anonymous namespace

// Register the plugin
static FrontendPluginRegistry::Add<UrduFrontendAction>
    X("urdu-clang", "Transform Urdu C source to standard C");
