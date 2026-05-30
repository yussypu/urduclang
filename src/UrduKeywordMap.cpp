#include "UrduKeywordMap.h"

namespace urdu {

KeywordMap& KeywordMap::instance() {
    static KeywordMap instance;
    return instance;
}

KeywordMap::KeywordMap() {
    initializeKeywords();
    initializeDirectives();
    initializeStdLib();
}

void KeywordMap::initializeKeywords() {
    // Types
    keywords_["عدد"] = "int";           // adad
    keywords_["اعشاری"] = "float";       // ashaari
    keywords_["مزدوج"] = "double";       // muzdawaj
    keywords_["حرف"] = "char";          // harf
    keywords_["خالی"] = "void";          // khaali
    keywords_["طویل"] = "long";          // taweel
    keywords_["مختصر"] = "short";        // mukhtasar
    keywords_["بےدستخط"] = "unsigned";   // be-dastakhat
    keywords_["بادستخط"] = "signed";     // ba-dastakhat

    // Control Flow
    keywords_["اگر"] = "if";             // agar
    keywords_["ورنہ"] = "else";          // warna
    keywords_["جبکہ"] = "while";         // jabkeh
    keywords_["کیلئے"] = "for";          // ke-liye
    keywords_["واپس"] = "return";        // waapas
    keywords_["توڑ"] = "break";          // tor
    keywords_["جاری"] = "continue";      // jaari
    keywords_["بدل"] = "switch";         // badal
    keywords_["صورت"] = "case";          // soorat
    keywords_["طےشدہ"] = "default";      // tay-shuda
    keywords_["کرو"] = "do";             // karo
    keywords_["جاؤ"] = "goto";           // jao

    // Storage & Qualifiers
    keywords_["جامد"] = "static";        // jaamid
    keywords_["بیرونی"] = "extern";      // bairooni
    keywords_["اندراج"] = "register";    // indiraaj
    keywords_["مستقل"] = "const";        // mustaqil
    keywords_["متغیر"] = "volatile";     // mutaghayyir
    keywords_["ساخت"] = "struct";        // saakht
    keywords_["اتحاد"] = "union";        // ittehaad
    keywords_["شمار"] = "enum";          // shumaar
    keywords_["قسم"] = "typedef";        // qism
    keywords_["ناپ"] = "sizeof";         // naap
}

void KeywordMap::initializeDirectives() {
    // Preprocessor Directives (without the #)
    directives_["شامل"] = "include";     // shaamil
    directives_["وضاحت"] = "define";     // wazaahat
    directives_["اگر"] = "if";           // agar (preprocessor #if)
    directives_["ورنہ"] = "else";        // warna (preprocessor #else)
    directives_["ختم"] = "endif";        // khatam
    directives_["اگرتعریف"] = "ifdef";   // agar-tareef
    directives_["اگرنہتعریف"] = "ifndef"; // agar-na-tareef
    directives_["خطا"] = "error";        // khata
    directives_["انتباہ"] = "warning";   // intibah
    directives_["عمل"] = "pragma";       // amal
}

void KeywordMap::initializeStdLib() {
    // Standard Library Mappings
    stdlib_["چھاپو"] = "printf";         // chhapo (print)
    stdlib_["پڑھو"] = "scanf";           // parho (read)
    stdlib_["اصل"] = "main";             // asl (main/origin)
    stdlib_["میموری"] = "malloc";        // memory
    stdlib_["آزاد"] = "free";            // azaad (free)
    stdlib_["نقل"] = "strcpy";           // naqal (copy)
    stdlib_["موازنہ"] = "strcmp";        // mawazna (compare)
    stdlib_["لمبائی"] = "strlen";        // lambai (length)
}

std::optional<llvm::StringRef> KeywordMap::lookupKeyword(llvm::StringRef urduToken) const {
    auto it = keywords_.find(urduToken);
    if (it != keywords_.end()) {
        return llvm::StringRef(it->second);
    }
    return std::nullopt;
}

std::optional<llvm::StringRef> KeywordMap::lookupDirective(llvm::StringRef urduDirective) const {
    auto it = directives_.find(urduDirective);
    if (it != directives_.end()) {
        return llvm::StringRef(it->second);
    }
    return std::nullopt;
}

std::optional<llvm::StringRef> KeywordMap::lookupStdLib(llvm::StringRef urduFunc) const {
    auto it = stdlib_.find(urduFunc);
    if (it != stdlib_.end()) {
        return llvm::StringRef(it->second);
    }
    return std::nullopt;
}

std::optional<llvm::StringRef> KeywordMap::lookup(llvm::StringRef urduToken) const {
    // Try keywords first
    if (auto result = lookupKeyword(urduToken)) {
        return result;
    }
    // Then stdlib
    if (auto result = lookupStdLib(urduToken)) {
        return result;
    }
    return std::nullopt;
}

bool KeywordMap::isUrduToken(llvm::StringRef token) const {
    return keywords_.count(token) || directives_.count(token) || stdlib_.count(token);
}

} // namespace urdu
