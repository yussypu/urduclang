#ifndef URDU_PREPROCESSOR_H
#define URDU_PREPROCESSOR_H

#include "llvm/ADT/StringRef.h"
#include <string>
#include <vector>
#include <utility>

namespace urdu {

// Represents a source location mapping from transformed to original
struct SourceMapping {
    size_t originalOffset;
    size_t transformedOffset;
    size_t originalLength;
    size_t transformedLength;
};

class Preprocessor {
public:
    Preprocessor();

    // Transform Urdu C source to standard C
    // Returns the transformed source
    std::string transform(llvm::StringRef source);

    // Get the source mappings for error reporting
    const std::vector<SourceMapping>& getMappings() const { return mappings_; }

    // Map a position in transformed source back to original
    size_t mapToOriginal(size_t transformedPos) const;

    // Map a position in original source to transformed
    size_t mapToTransformed(size_t originalPos) const;

private:
    // Token types for the scanner
    enum class TokenType {
        Identifier,     // Urdu or ASCII identifier
        Directive,      // #directive
        StringLiteral,  // "..."
        CharLiteral,    // '...'
        Comment,        // // or /* */
        Whitespace,
        Operator,
        Other
    };

    struct Token {
        TokenType type;
        std::string text;
        size_t offset;
        size_t length;
    };

    // Tokenize the source (simple UTF-8 aware tokenizer)
    std::vector<Token> tokenize(llvm::StringRef source);

    // Process a single token, replacing Urdu keywords
    std::string processToken(const Token& token);

    // Check if character is a word boundary
    bool isWordBoundary(uint32_t cp);

    // Check if we're inside a string or comment
    bool isInStringOrComment(llvm::StringRef source, size_t pos);

    std::vector<SourceMapping> mappings_;
};

} // namespace urdu

#endif // URDU_PREPROCESSOR_H
