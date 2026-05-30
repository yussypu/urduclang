#ifndef URDU_DIAG_CONSUMER_H
#define URDU_DIAG_CONSUMER_H

#include "UrduPreprocessor.h"
#include "UrduTranslator.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <string>

namespace urdu {

class DiagConsumer : public clang::DiagnosticConsumer {
public:
    DiagConsumer(llvm::raw_ostream& os,
                 const Preprocessor* preprocessor = nullptr,
                 clang::DiagnosticOptions* diagOpts = nullptr);

    void HandleDiagnostic(clang::DiagnosticsEngine::Level level,
                          const clang::Diagnostic& info) override;

    void BeginSourceFile(const clang::LangOptions& langOpts,
                         const clang::Preprocessor* pp) override;

    void EndSourceFile() override;

    // Load diagnostic translations from JSON file
    bool loadTranslations(llvm::StringRef jsonPath);

private:
    // Translate a diagnostic message to Urdu
    std::string translateMessage(llvm::StringRef message) const;

    // Get Urdu name for diagnostic level
    llvm::StringRef getLevelName(clang::DiagnosticsEngine::Level level) const;

    // Format location with source mapping
    std::string formatLocation(const clang::Diagnostic& info) const;

    llvm::raw_ostream& os_;
    const Preprocessor* preprocessor_;
    const clang::SourceManager* sm_ = nullptr;

    // Shared English -> Urdu diagnostic translator.
    Translator translator_;
};

} // namespace urdu

#endif // URDU_DIAG_CONSUMER_H
