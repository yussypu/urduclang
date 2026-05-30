#ifndef URDU_KEYWORD_MAP_H
#define URDU_KEYWORD_MAP_H

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include <string>
#include <optional>

namespace urdu {

class KeywordMap {
public:
    static KeywordMap& instance();

    // Look up an Urdu token and return its C equivalent, if any
    std::optional<llvm::StringRef> lookupKeyword(llvm::StringRef urduToken) const;

    // Look up an Urdu preprocessor directive and return its C equivalent
    std::optional<llvm::StringRef> lookupDirective(llvm::StringRef urduDirective) const;

    // Look up an Urdu standard library function and return its C equivalent
    std::optional<llvm::StringRef> lookupStdLib(llvm::StringRef urduFunc) const;

    // Combined lookup: checks keywords, directives, and stdlib
    std::optional<llvm::StringRef> lookup(llvm::StringRef urduToken) const;

    // Check if a token is an Urdu keyword/identifier
    bool isUrduToken(llvm::StringRef token) const;

private:
    KeywordMap();
    void initializeKeywords();
    void initializeDirectives();
    void initializeStdLib();

    llvm::StringMap<std::string> keywords_;
    llvm::StringMap<std::string> directives_;
    llvm::StringMap<std::string> stdlib_;
};

} // namespace urdu

#endif // URDU_KEYWORD_MAP_H
