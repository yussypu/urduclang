#ifndef URDU_TRANSLATOR_H
#define URDU_TRANSLATOR_H

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include <string>

namespace urdu {

// Translates clang diagnostic text from English to Urdu.
//
// It ships with a built-in catalog of common phrases and can additionally
// merge a larger catalog from diagnostics/urdu_messages.json at runtime.
// Translation is phrase-substitution: every known English phrase found in a
// message is replaced by its Urdu equivalent, longest phrases first so that
// e.g. "fatal error" wins over "error".
class Translator {
public:
    Translator();

    // Merge translations from a JSON object file ({ "english": "urdu", ... }).
    // Returns false if the file can't be read or parsed.
    bool loadFromJson(llvm::StringRef jsonPath);

    // Locate diagnostics/urdu_messages.json relative to the running executable
    // (handles both the build tree and an installed layout) and merge it.
    // Returns true if a catalog was found and loaded.
    bool loadCatalogNear(llvm::StringRef exePath);

    // Translate a single diagnostic message.
    std::string translate(llvm::StringRef message) const;

    // Number of phrases currently known.
    size_t size() const { return translations_.size(); }

private:
    void loadBuiltins();

    llvm::StringMap<std::string> translations_;
};

} // namespace urdu

#endif // URDU_TRANSLATOR_H
