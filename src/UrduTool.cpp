// Standalone compiler driver for Urdu C.
// Usage: urduc-tool <source.c> [clang options...]
//
// Transforms the Urdu source to standard C, compiles it with clang, then
// rewrites clang's diagnostics back onto the original file in Urdu and exits
// with clang's real exit code.

#include "UrduPreprocessor.h"
#include "UrduTranslator.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include <optional>
#include <string>
#include <vector>

using namespace llvm;

namespace {

void printUsage(const char* progName) {
    errs() << "اردو-کلینگ (Urdu-Clang)\n\n";
    errs() << "استعمال / Usage:\n";
    errs() << "  " << progName << " <source.c> [-o output] [options]\n\n";
    errs() << "مثالیں / Examples:\n";
    errs() << "  " << progName << " hello.c              # Compile hello.c\n";
    errs() << "  " << progName << " hello.c -o hello     # Compile to 'hello'\n";
    errs() << "  " << progName << " -E hello.c           # Preprocess only (show C)\n";
    errs() << "\n";
}

// Prefer Homebrew LLVM clang, whose diagnostic wording matches the catalog,
// then fall back to whatever is on PATH.
std::string findClang() {
    const char* preferred[] = {
        "/opt/homebrew/opt/llvm/bin/clang",
        "/usr/local/opt/llvm/bin/clang",
    };
    for (const char* p : preferred) {
        if (sys::fs::exists(p))
            return p;
    }
    if (auto found = sys::findProgramByName("clang"))
        return *found;
    if (sys::fs::exists("/usr/bin/clang"))
        return "/usr/bin/clang";
    return {};
}

} // anonymous namespace

int main(int argc, const char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-help" || a == "--help" || a == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    // Separate the source file from the flags we forward to clang.
    std::string sourceFile;
    std::vector<std::string> passthrough;
    bool preprocessOnly = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-E") {
            preprocessOnly = true;
        } else if (!arg.empty() && arg[0] != '-' && sourceFile.empty()) {
            sourceFile = arg;
        } else {
            passthrough.push_back(arg);
        }
    }

    if (sourceFile.empty()) {
        errs() << "خطا: سورس فائل درکار ہے\n";
        errs() << "Error: source file required\n";
        return 1;
    }

    auto bufferOrErr = MemoryBuffer::getFile(sourceFile);
    if (!bufferOrErr) {
        errs() << "خطا: فائل نہیں پڑھی جا سکی: " << sourceFile << "\n";
        errs() << "Error: cannot read file: " << sourceFile << "\n";
        return 1;
    }

    // Newlines are preserved, so line numbers in the generated C match the source.
    urdu::Preprocessor preprocessor;
    std::string transformed = preprocessor.transform(bufferOrErr.get()->getBuffer());

    if (preprocessOnly) {
        outs() << "// Transformed from Urdu C to standard C\n";
        outs() << "// اردو سی سے معیاری سی میں تبدیل شدہ\n\n";
        outs() << transformed;
        return 0;
    }

    // Write to a temp .c file so clang treats it as C; keep the original stem
    // in the name. It is cleaned up below.
    SmallString<128> tempPath;
    {
        int fd;
        StringRef stem = sys::path::stem(sourceFile);
        if (auto ec = sys::fs::createTemporaryFile("urduc-" + stem.str(), "c", fd, tempPath)) {
            errs() << "خطا: عارضی فائل نہیں بن سکی\n";
            return 1;
        }
        raw_fd_ostream tempOut(fd, /*shouldClose=*/true);
        tempOut << transformed;
    }

    std::string clangPath = findClang();
    if (clangPath.empty()) {
        errs() << "خطا: clang نہیں ملا\n";
        errs() << "Error: clang not found. Add LLVM to your PATH.\n";
        (void)sys::fs::remove(tempPath);
        return 1;
    }

    // Only ask for color when stderr is a terminal, so captured output stays
    // clean in pipes.
    bool wantColor = errs().is_displayed();

    std::vector<std::string> argStrings;
    argStrings.push_back(clangPath);
    argStrings.push_back(std::string(tempPath));
    argStrings.push_back(wantColor ? "-fdiagnostics-color=always"
                                   : "-fno-color-diagnostics");

#ifdef __APPLE__
    // Homebrew clang needs the macOS SDK path to find system headers.
    const char* sdkPaths[] = {
        "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk",
        "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/"
        "Developer/SDKs/MacOSX.sdk",
    };
    for (const char* sdk : sdkPaths) {
        if (sys::fs::exists(sdk)) {
            argStrings.push_back("-isysroot");
            argStrings.push_back(sdk);
            break;
        }
    }
#endif

    for (const auto& a : passthrough)
        argStrings.push_back(a);

    std::vector<StringRef> args(argStrings.begin(), argStrings.end());

    // Capture clang's stderr so we can rewrite paths and translate to Urdu.
    SmallString<128> errPath;
    if (auto ec = sys::fs::createTemporaryFile("urduc-diag", "txt", errPath)) {
        errs() << "خطا: عارضی فائل نہیں بن سکی\n";
        (void)sys::fs::remove(tempPath);
        return 1;
    }

    std::optional<StringRef> redirects[] = {
        std::nullopt,         // stdin: inherit
        std::nullopt,         // stdout: inherit
        StringRef(errPath),   // stderr: capture
    };

    std::string execErr;
    bool execFailed = false;
    int rc = sys::ExecuteAndWait(clangPath, args, /*Env=*/std::nullopt, redirects,
                                 /*SecondsToWait=*/0, /*MemoryLimit=*/0, &execErr,
                                 &execFailed);

    // Emit clang's diagnostics in Urdu, pointing at the user's file.
    if (auto diagBuf = MemoryBuffer::getFile(errPath)) {
        std::string diag = diagBuf.get()->getBuffer().str();
        if (!diag.empty()) {
            urdu::Translator translator;
            translator.loadCatalogNear(
                sys::fs::getMainExecutable(argv[0], (void*)(intptr_t)&main));

            // Rewrite the temp path back to the original filename.
            const std::string temp = std::string(tempPath);
            size_t pos = 0;
            while ((pos = diag.find(temp, pos)) != std::string::npos) {
                diag.replace(pos, temp.size(), sourceFile);
                pos += sourceFile.size();
            }

            errs() << translator.translate(diag);
        }
    }

    (void)sys::fs::remove(tempPath);
    (void)sys::fs::remove(errPath);

    if (execFailed) {
        errs() << "خطا: clang نہیں چل سکا: " << execErr << "\n";
        return 1;
    }

    if (rc == 0)
        errs() << "✓ تالیف کامیاب\n";
    else
        errs() << "✗ تالیف ناکام\n";

    return rc;
}
