#include "UrduDiagConsumer.h"
#include "UrduBidiHelper.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Lex/Preprocessor.h"

namespace urdu {

DiagConsumer::DiagConsumer(llvm::raw_ostream& os,
                           const Preprocessor* preprocessor,
                           clang::DiagnosticOptions* diagOpts)
    : os_(os), preprocessor_(preprocessor) {
    // Built-in translations come from Translator; callers may extend the
    // catalog via loadTranslations(). The level labels printed below are
    // handled separately by getLevelName().
}

void DiagConsumer::HandleDiagnostic(clang::DiagnosticsEngine::Level level,
                                     const clang::Diagnostic& info) {
    llvm::SmallString<256> message;
    info.FormatDiagnostic(message);

    std::string urduMessage = translateMessage(message.str());
    llvm::StringRef levelName = getLevelName(level);

    // Wrap the line in an RTL isolate, then print the level label.
    os_ << bidi::RLI;
    switch (level) {
        case clang::DiagnosticsEngine::Fatal:
        case clang::DiagnosticsEngine::Error:
            os_ << "\033[1;31m" << levelName << "\033[0m";
            break;
        case clang::DiagnosticsEngine::Warning:
            os_ << "\033[1;33m" << levelName << "\033[0m";
            break;
        case clang::DiagnosticsEngine::Note:
            os_ << "\033[1;36m" << levelName << "\033[0m";
            break;
        default:
            os_ << levelName;
    }

    os_ << ": ";

    if (info.hasSourceManager()) {
        clang::SourceLocation loc = info.getLocation();
        if (loc.isValid()) {
            const clang::SourceManager& sm = info.getSourceManager();
            clang::PresumedLoc ploc = sm.getPresumedLoc(loc);
            if (ploc.isValid()) {
                os_ << bidi::LRI << ploc.getFilename() << ":"
                    << ploc.getLine() << ":" << ploc.getColumn()
                    << bidi::PDI << ": ";
            }
        }
    }

    os_ << urduMessage;
    os_ << bidi::PDI << "\n";

    if (level >= clang::DiagnosticsEngine::Error) {
        NumErrors++;
    } else if (level == clang::DiagnosticsEngine::Warning) {
        NumWarnings++;
    }
}

void DiagConsumer::BeginSourceFile(const clang::LangOptions& langOpts,
                                    const clang::Preprocessor* pp) {
    if (pp) {
        sm_ = &pp->getSourceManager();
    }
}

void DiagConsumer::EndSourceFile() {
    sm_ = nullptr;
}

bool DiagConsumer::loadTranslations(llvm::StringRef jsonPath) {
    return translator_.loadFromJson(jsonPath);
}

std::string DiagConsumer::translateMessage(llvm::StringRef message) const {
    return translator_.translate(message);
}

llvm::StringRef DiagConsumer::getLevelName(clang::DiagnosticsEngine::Level level) const {
    switch (level) {
        case clang::DiagnosticsEngine::Ignored:
            return "نظرانداز";  // nazarandaaz
        case clang::DiagnosticsEngine::Note:
            return "نوٹ";       // note
        case clang::DiagnosticsEngine::Remark:
            return "تبصرہ";     // tabsara
        case clang::DiagnosticsEngine::Warning:
            return "انتباہ";    // intibah
        case clang::DiagnosticsEngine::Error:
            return "خطا";       // khata
        case clang::DiagnosticsEngine::Fatal:
            return "مہلک خطا";  // mohlik khata
    }
    return "خطا";
}

} // namespace urdu
