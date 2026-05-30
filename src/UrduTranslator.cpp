#include "UrduTranslator.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include <algorithm>
#include <vector>

namespace urdu {

Translator::Translator() {
    loadBuiltins();
}

void Translator::loadBuiltins() {
    // A compact set of common diagnostics so the tool is useful even if the
    // JSON catalog can't be found. The catalog (loadCatalogNear) extends this.
    translations_["expected ';'"] = "';' متوقع تھا";
    translations_["expected ')'"] = "')' متوقع تھا";
    translations_["expected '('"] = "'(' متوقع تھا";
    translations_["expected '}'"] = "'}' متوقع تھا";
    translations_["expected '{'"] = "'{' متوقع تھا";
    translations_["expected ']'"] = "']' متوقع تھا";
    translations_["expected '['"] = "'[' متوقع تھا";
    translations_["expected expression"] = "اظہار متوقع تھا";
    translations_["expected identifier"] = "شناخت کنندہ متوقع تھا";
    translations_["use of undeclared identifier"] = "غیر اعلان شدہ شناخت کنندہ کا استعمال";
    translations_["unknown type name"] = "نامعلوم قسم کا نام";
    translations_["implicit declaration of function"] = "فنکشن کا مضمر اعلان";
    translations_["too few arguments to function call"] = "فنکشن کال میں دلائل کم ہیں";
    translations_["too many arguments to function call"] = "فنکشن کال میں دلائل زیادہ ہیں";
    translations_["redefinition of"] = "کی دوبارہ تعریف";
    translations_["conflicting types for"] = "کے لیے متضاد اقسام";
    translations_["control reaches end of non-void function"] =
        "کنٹرول غیر خالی فنکشن کے آخر تک پہنچتا ہے";
    translations_["unused variable"] = "غیر استعمال شدہ متغیر";
    translations_["file not found"] = "فائل نہیں ملی";

    // Diagnostic level labels (matched with the trailing colon so they only
    // ever replace the label, never the word inside a longer phrase).
    translations_["fatal error:"] = "مہلک خطا:";
    translations_["error:"] = "خطا:";
    translations_["warning:"] = "انتباہ:";
    translations_["note:"] = "نوٹ:";
}

bool Translator::loadFromJson(llvm::StringRef jsonPath) {
    auto bufferOrErr = llvm::MemoryBuffer::getFile(jsonPath);
    if (!bufferOrErr)
        return false;

    auto json = llvm::json::parse(bufferOrErr.get()->getBuffer());
    if (!json)
        return false;

    auto* obj = json->getAsObject();
    if (!obj)
        return false;

    for (const auto& kv : *obj) {
        if (auto str = kv.second.getAsString())
            translations_[kv.first] = str->str();
    }
    return true;
}

bool Translator::loadCatalogNear(llvm::StringRef exePath) {
    llvm::StringRef binDir = llvm::sys::path::parent_path(exePath);

    // Candidate locations, relative to the executable:
    //   build/urduc-tool        -> ../diagnostics/urdu_messages.json
    //   <prefix>/bin/urduc-tool -> ../share/urdu-clang/urdu_messages.json
    //   alongside the binary     -> ./urdu_messages.json
    auto tryLoad = [&](llvm::SmallString<256> p) {
        return loadFromJson(p);
    };

    {
        llvm::SmallString<256> p(binDir);
        llvm::sys::path::append(p, "..", "diagnostics", "urdu_messages.json");
        if (tryLoad(p))
            return true;
    }
    {
        llvm::SmallString<256> p(binDir);
        llvm::sys::path::append(p, "..", "share", "urdu-clang", "urdu_messages.json");
        if (tryLoad(p))
            return true;
    }
    {
        llvm::SmallString<256> p(binDir);
        llvm::sys::path::append(p, "urdu_messages.json");
        if (tryLoad(p))
            return true;
    }
    return false;
}

std::string Translator::translate(llvm::StringRef message) const {
    // Replace longest phrases first so specific multi-word diagnostics win over
    // their substrings (e.g. "fatal error:" before "error:").
    std::vector<llvm::StringRef> keys;
    keys.reserve(translations_.size());
    for (const auto& kv : translations_)
        keys.push_back(kv.first());
    std::sort(keys.begin(), keys.end(),
              [](llvm::StringRef a, llvm::StringRef b) { return a.size() > b.size(); });

    std::string result = message.str();
    for (llvm::StringRef key : keys) {
        if (key.empty())
            continue;
        const std::string& value = translations_.find(key)->second;
        const std::string needle = key.str();
        size_t pos = 0;
        while ((pos = result.find(needle, pos)) != std::string::npos) {
            result.replace(pos, needle.size(), value);
            pos += value.size();
        }
    }
    return result;
}

} // namespace urdu
