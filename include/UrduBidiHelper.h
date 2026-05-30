#ifndef URDU_BIDI_HELPER_H
#define URDU_BIDI_HELPER_H

#include "llvm/ADT/StringRef.h"
#include <string>

namespace urdu {

// Unicode bidirectional control characters
namespace bidi {
    // Right-to-Left Mark
    constexpr const char* RLM = "\u200F";
    // Left-to-Right Mark
    constexpr const char* LRM = "\u200E";
    // Right-to-Left Embedding
    constexpr const char* RLE = "\u202B";
    // Left-to-Right Embedding
    constexpr const char* LRE = "\u202A";
    // Pop Directional Formatting
    constexpr const char* PDF = "\u202C";
    // Right-to-Left Override
    constexpr const char* RLO = "\u202E";
    // Left-to-Right Override
    constexpr const char* LRO = "\u202D";
    // Right-to-Left Isolate
    constexpr const char* RLI = "\u2067";
    // Left-to-Right Isolate
    constexpr const char* LRI = "\u2066";
    // Pop Directional Isolate
    constexpr const char* PDI = "\u2069";
}

class BidiHelper {
public:
    // Check if a character is in the Arabic/Urdu Unicode block
    static bool isUrduChar(uint32_t codepoint);

    // Check if a string contains Urdu/Arabic characters
    static bool containsUrdu(llvm::StringRef str);

    // Wrap RTL text for proper terminal display
    static std::string wrapRTL(llvm::StringRef text);

    // Wrap LTR text (code identifiers) within RTL context
    static std::string wrapLTR(llvm::StringRef text);

    // Format a mixed diagnostic message with proper bidi markers
    static std::string formatDiagnostic(llvm::StringRef urduMessage,
                                         llvm::StringRef codeSnippet);

    // Decode a UTF-8 character and return its codepoint
    // Returns the codepoint and advances the pointer
    static uint32_t decodeUTF8(const char*& ptr, const char* end);

    // Get the byte length of a UTF-8 character from its first byte
    static size_t utf8CharLength(unsigned char firstByte);

    // Check if we're at a valid UTF-8 character boundary
    static bool isUTF8Start(unsigned char byte);
};

} // namespace urdu

#endif // URDU_BIDI_HELPER_H
