#include "UrduPreprocessor.h"
#include "UrduKeywordMap.h"
#include "UrduBidiHelper.h"
#include <cctype>

namespace urdu {

Preprocessor::Preprocessor() = default;

std::string Preprocessor::transform(llvm::StringRef source) {
    mappings_.clear();
    std::string result;
    result.reserve(source.size());

    auto tokens = tokenize(source);
    size_t transformedOffset = 0;

    for (const auto& token : tokens) {
        std::string processed = processToken(token);

        // Record mapping if the token was transformed
        if (processed != token.text) {
            mappings_.push_back({
                token.offset,
                transformedOffset,
                token.length,
                processed.size()
            });
        }

        result += processed;
        transformedOffset += processed.size();
    }

    return result;
}

std::vector<Preprocessor::Token> Preprocessor::tokenize(llvm::StringRef source) {
    std::vector<Token> tokens;
    const char* ptr = source.data();
    const char* end = ptr + source.size();
    const char* start = ptr;

    while (ptr < end) {
        const char* tokenStart = ptr;
        size_t offset = tokenStart - start;

        // Check for preprocessor directive
        if (*ptr == '#') {
            std::string directive;
            directive += *ptr++;

            // Skip whitespace after #
            while (ptr < end && (*ptr == ' ' || *ptr == '\t')) {
                directive += *ptr++;
            }

            // Read directive name (could be Urdu)
            const char* nameStart = ptr;
            while (ptr < end) {
                unsigned char c = static_cast<unsigned char>(*ptr);
                if (BidiHelper::isUTF8Start(c)) {
                    size_t len = BidiHelper::utf8CharLength(c);
                    if (ptr + len <= end) {
                        // Check if it's an Urdu char or ASCII identifier char
                        const char* temp = ptr;
                        uint32_t cp = BidiHelper::decodeUTF8(temp, end);
                        if (BidiHelper::isUrduChar(cp) || std::isalnum(c) || c == '_') {
                            while (ptr < temp) directive += *ptr++;
                        } else {
                            break;
                        }
                    } else {
                        break;
                    }
                } else if (std::isalnum(c) || c == '_') {
                    directive += *ptr++;
                } else {
                    break;
                }
            }

            tokens.push_back({TokenType::Directive, directive, offset, directive.size()});
            continue;
        }

        // Check for string literal
        if (*ptr == '"') {
            std::string str;
            str += *ptr++;
            while (ptr < end && *ptr != '"') {
                if (*ptr == '\\' && ptr + 1 < end) {
                    str += *ptr++;
                    str += *ptr++;
                } else {
                    str += *ptr++;
                }
            }
            if (ptr < end) str += *ptr++; // closing quote
            tokens.push_back({TokenType::StringLiteral, str, offset, str.size()});
            continue;
        }

        // Check for char literal
        if (*ptr == '\'') {
            std::string str;
            str += *ptr++;
            while (ptr < end && *ptr != '\'') {
                if (*ptr == '\\' && ptr + 1 < end) {
                    str += *ptr++;
                    str += *ptr++;
                } else {
                    str += *ptr++;
                }
            }
            if (ptr < end) str += *ptr++; // closing quote
            tokens.push_back({TokenType::CharLiteral, str, offset, str.size()});
            continue;
        }

        // Check for line comment
        if (*ptr == '/' && ptr + 1 < end && *(ptr + 1) == '/') {
            std::string comment;
            while (ptr < end && *ptr != '\n') {
                comment += *ptr++;
            }
            tokens.push_back({TokenType::Comment, comment, offset, comment.size()});
            continue;
        }

        // Check for block comment
        if (*ptr == '/' && ptr + 1 < end && *(ptr + 1) == '*') {
            std::string comment;
            comment += *ptr++;
            comment += *ptr++;
            while (ptr < end) {
                if (*ptr == '*' && ptr + 1 < end && *(ptr + 1) == '/') {
                    comment += *ptr++;
                    comment += *ptr++;
                    break;
                }
                comment += *ptr++;
            }
            tokens.push_back({TokenType::Comment, comment, offset, comment.size()});
            continue;
        }

        // Check for whitespace
        if (std::isspace(static_cast<unsigned char>(*ptr))) {
            std::string ws;
            while (ptr < end && std::isspace(static_cast<unsigned char>(*ptr))) {
                ws += *ptr++;
            }
            tokens.push_back({TokenType::Whitespace, ws, offset, ws.size()});
            continue;
        }

        // Check for identifier (ASCII or Urdu)
        unsigned char firstByte = static_cast<unsigned char>(*ptr);
        if (std::isalpha(firstByte) || firstByte == '_' ||
            (BidiHelper::isUTF8Start(firstByte) && BidiHelper::utf8CharLength(firstByte) > 1)) {

            std::string ident;
            while (ptr < end) {
                unsigned char c = static_cast<unsigned char>(*ptr);
                if (std::isalnum(c) || c == '_') {
                    ident += *ptr++;
                } else if (BidiHelper::isUTF8Start(c) && BidiHelper::utf8CharLength(c) > 1) {
                    // Multi-byte UTF-8: check if it's Urdu
                    const char* temp = ptr;
                    uint32_t cp = BidiHelper::decodeUTF8(temp, end);
                    if (BidiHelper::isUrduChar(cp)) {
                        while (ptr < temp) ident += *ptr++;
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
            tokens.push_back({TokenType::Identifier, ident, offset, ident.size()});
            continue;
        }

        // Check for numbers (don't transform)
        if (std::isdigit(firstByte)) {
            std::string num;
            while (ptr < end && (std::isalnum(static_cast<unsigned char>(*ptr)) ||
                                 *ptr == '.' || *ptr == 'x' || *ptr == 'X')) {
                num += *ptr++;
            }
            tokens.push_back({TokenType::Other, num, offset, num.size()});
            continue;
        }

        // Everything else (operators, punctuation, etc.)
        std::string other;
        other += *ptr++;
        tokens.push_back({TokenType::Other, other, offset, other.size()});
    }

    return tokens;
}

std::string Preprocessor::processToken(const Token& token) {
    const auto& keywordMap = KeywordMap::instance();

    switch (token.type) {
        case TokenType::Identifier: {
            if (auto replacement = keywordMap.lookup(token.text)) {
                return replacement->str();
            }
            return token.text;
        }

        case TokenType::Directive: {
            // # + optional whitespace + directive name
            std::string result;
            size_t i = 0;

            if (i < token.text.size() && token.text[i] == '#') {
                result += token.text[i++];
            }
            while (i < token.text.size() &&
                   (token.text[i] == ' ' || token.text[i] == '\t')) {
                result += token.text[i++];
            }

            std::string directiveName = token.text.substr(i);
            if (auto replacement = keywordMap.lookupDirective(directiveName)) {
                result += replacement->str();
            } else {
                result += directiveName;
            }

            return result;
        }

        case TokenType::StringLiteral:
        case TokenType::CharLiteral:
        case TokenType::Comment:
        case TokenType::Whitespace:
        case TokenType::Other:
        default:
            return token.text;
    }
}

bool Preprocessor::isWordBoundary(uint32_t cp) {
    if (cp < 128) {
        char c = static_cast<char>(cp);
        return !std::isalnum(static_cast<unsigned char>(c)) && c != '_';
    }
    // For non-ASCII, Urdu characters are not word boundaries
    return !BidiHelper::isUrduChar(cp);
}

size_t Preprocessor::mapToOriginal(size_t transformedPos) const {
    size_t offset = 0;

    for (const auto& mapping : mappings_) {
        if (transformedPos < mapping.transformedOffset) {
            break;
        }
        if (transformedPos < mapping.transformedOffset + mapping.transformedLength) {
            // Inside a transformed region, map proportionally
            double ratio = static_cast<double>(transformedPos - mapping.transformedOffset) /
                          mapping.transformedLength;
            return mapping.originalOffset +
                   static_cast<size_t>(ratio * mapping.originalLength);
        }
        // Adjust offset for the size difference
        offset += mapping.originalLength - mapping.transformedLength;
    }

    return transformedPos + offset;
}

size_t Preprocessor::mapToTransformed(size_t originalPos) const {
    size_t offset = 0;

    for (const auto& mapping : mappings_) {
        if (originalPos < mapping.originalOffset) {
            break;
        }
        if (originalPos < mapping.originalOffset + mapping.originalLength) {
            // Inside an original region, map proportionally
            double ratio = static_cast<double>(originalPos - mapping.originalOffset) /
                          mapping.originalLength;
            return mapping.transformedOffset +
                   static_cast<size_t>(ratio * mapping.transformedLength);
        }
        // Adjust offset for the size difference
        offset += mapping.transformedLength - mapping.originalLength;
    }

    return originalPos + offset;
}

} // namespace urdu
