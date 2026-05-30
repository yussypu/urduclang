#include "UrduBidiHelper.h"

namespace urdu {

bool BidiHelper::isUrduChar(uint32_t codepoint) {
    // Arabic block: U+0600 - U+06FF (includes Urdu)
    // Arabic Supplement: U+0750 - U+077F
    // Arabic Extended-A: U+08A0 - U+08FF
    // Arabic Presentation Forms-A: U+FB50 - U+FDFF
    // Arabic Presentation Forms-B: U+FE70 - U+FEFF
    return (codepoint >= 0x0600 && codepoint <= 0x06FF) ||
           (codepoint >= 0x0750 && codepoint <= 0x077F) ||
           (codepoint >= 0x08A0 && codepoint <= 0x08FF) ||
           (codepoint >= 0xFB50 && codepoint <= 0xFDFF) ||
           (codepoint >= 0xFE70 && codepoint <= 0xFEFF);
}

bool BidiHelper::containsUrdu(llvm::StringRef str) {
    const char* ptr = str.data();
    const char* end = ptr + str.size();

    while (ptr < end) {
        uint32_t cp = decodeUTF8(ptr, end);
        if (isUrduChar(cp)) {
            return true;
        }
    }
    return false;
}

std::string BidiHelper::wrapRTL(llvm::StringRef text) {
    std::string result;
    result += bidi::RLI;
    result += text.str();
    result += bidi::PDI;
    return result;
}

std::string BidiHelper::wrapLTR(llvm::StringRef text) {
    std::string result;
    result += bidi::LRI;
    result += text.str();
    result += bidi::PDI;
    return result;
}

std::string BidiHelper::formatDiagnostic(llvm::StringRef urduMessage,
                                          llvm::StringRef codeSnippet) {
    std::string result;
    // Start with RTL context for the Urdu message
    result += bidi::RLI;
    result += urduMessage.str();

    if (!codeSnippet.empty()) {
        // Insert LTR code snippet
        result += ": ";
        result += bidi::LRI;
        result += codeSnippet.str();
        result += bidi::PDI;
    }

    result += bidi::PDI;
    return result;
}

uint32_t BidiHelper::decodeUTF8(const char*& ptr, const char* end) {
    if (ptr >= end) {
        return 0;
    }

    unsigned char c = static_cast<unsigned char>(*ptr);
    uint32_t codepoint;
    size_t len;

    if ((c & 0x80) == 0) {
        // ASCII: 0xxxxxxx
        codepoint = c;
        len = 1;
    } else if ((c & 0xE0) == 0xC0) {
        // 2-byte: 110xxxxx 10xxxxxx
        codepoint = c & 0x1F;
        len = 2;
    } else if ((c & 0xF0) == 0xE0) {
        // 3-byte: 1110xxxx 10xxxxxx 10xxxxxx
        codepoint = c & 0x0F;
        len = 3;
    } else if ((c & 0xF8) == 0xF0) {
        // 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        codepoint = c & 0x07;
        len = 4;
    } else {
        // Invalid UTF-8, skip one byte
        ++ptr;
        return 0xFFFD; // Replacement character
    }

    // Check if we have enough bytes
    if (ptr + len > end) {
        ++ptr;
        return 0xFFFD;
    }

    // Decode continuation bytes
    for (size_t i = 1; i < len; ++i) {
        unsigned char cont = static_cast<unsigned char>(ptr[i]);
        if ((cont & 0xC0) != 0x80) {
            // Invalid continuation byte
            ++ptr;
            return 0xFFFD;
        }
        codepoint = (codepoint << 6) | (cont & 0x3F);
    }

    ptr += len;
    return codepoint;
}

size_t BidiHelper::utf8CharLength(unsigned char firstByte) {
    if ((firstByte & 0x80) == 0) return 1;
    if ((firstByte & 0xE0) == 0xC0) return 2;
    if ((firstByte & 0xF0) == 0xE0) return 3;
    if ((firstByte & 0xF8) == 0xF0) return 4;
    return 1; // Invalid, treat as single byte
}

bool BidiHelper::isUTF8Start(unsigned char byte) {
    // A byte is a valid UTF-8 start if it's ASCII or a multi-byte lead
    return (byte & 0x80) == 0 || (byte & 0xC0) == 0xC0;
}

} // namespace urdu
