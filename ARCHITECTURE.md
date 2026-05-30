# اردو-کلینگ (Urdu-Clang) Architecture

## Overview

A Clang LibTooling plugin that lets you write C in Urdu. Urdu keywords (`اگر`, `جبکہ`, `واپس`) map to their C equivalents, and compiler diagnostics are emitted in Urdu with RTL support.

Target: Clang 17+ (LibTooling / `FrontendAction` plugin API).
Language: C only for now.
Build: CMake, links against LLVM/Clang shared libs.

## Architecture

There are two layers.

### Layer 1: Urdu to C preprocessor (source transformation)

Clang plugins cannot easily modify the lexer, since the lexer is baked into libclang. So instead of forking the lexer, we do a source-to-source transformation before Clang parses:

```
Urdu .c file -> [UrduPreprocessor] -> standard C (in-memory) -> [Clang frontend] -> AST -> codegen
```

This is implemented as a custom `FrontendAction` that:

1. Reads the source file.
2. Replaces Urdu tokens with C tokens (keyword mapping).
3. Feeds the transformed source to Clang's preprocessor via `PreprocessorOptions::addRemappedFile()`.

Reasons for this approach:

- No need to fork Clang or modify the lexer.
- Works as a loadable plugin (`-fplugin=`) or a standalone tool.
- The Urdu mapping stays a plain lookup table.
- Source locations are preserved via `SourceManager` remapping, for accurate diagnostics.

### Layer 2: Urdu diagnostics (DiagnosticConsumer)

A custom `DiagnosticConsumer` that:

1. Intercepts Clang diagnostic messages.
2. Maps them to Urdu translations.
3. Handles RTL text output (terminal and JSON output modes).

## Keyword mapping table

### Types
| Urdu | C | Transliteration |
|------|---|-----------------|
| `عدد` | `int` | adad |
| `اعشاری` | `float` | ashaari |
| `مزدوج` | `double` | muzdawaj |
| `حرف` | `char` | harf |
| `خالی` | `void` | khaali |
| `طویل` | `long` | taweel |
| `مختصر` | `short` | mukhtasar |
| `بےدستخط` | `unsigned` | be-dastakhat |
| `بادستخط` | `signed` | ba-dastakhat |

### Control flow
| Urdu | C | Transliteration |
|------|---|-----------------|
| `اگر` | `if` | agar |
| `ورنہ` | `else` | warna |
| `جبکہ` | `while` | jabkeh |
| `کیلئے` | `for` | ke-liye |
| `واپس` | `return` | waapas |
| `توڑ` | `break` | tor |
| `جاری` | `continue` | jaari |
| `بدل` | `switch` | badal |
| `صورت` | `case` | soorat |
| `طےشدہ` | `default` | tay-shuda |
| `کرو` | `do` | karo |
| `جاؤ` | `goto` | jao |

### Storage and qualifiers
| Urdu | C | Transliteration |
|------|---|-----------------|
| `جامد` | `static` | jaamid |
| `بیرونی` | `extern` | bairooni |
| `اندراج` | `register` | indiraaj |
| `مستقل` | `const` | mustaqil |
| `متغیر` | `volatile` | mutaghayyir |
| `ساخت` | `struct` | saakht |
| `اتحاد` | `union` | ittehaad |
| `شمار` | `enum` | shumaar |
| `قسم` | `typedef` | qism |
| `ناپ` | `sizeof` | naap |

### Preprocessor directives
| Urdu | C | Transliteration |
|------|---|-----------------|
| `#شامل` | `#include` | shaamil |
| `#وضاحت` | `#define` | wazaahat |
| `#اگر` | `#if` | agar |
| `#ورنہ` | `#else` | warna |
| `#ختم` | `#endif` | khatam |

### Standard library mappings
| Urdu | C | Notes |
|------|---|-------|
| `چھاپو` | `printf` | print |
| `پڑھو` | `scanf` | read |
| `اصل` | `main` | main / origin |

## File structure

```
urdu-clang/
├── CMakeLists.txt                  # Top-level CMake config
├── README.md
├── ARCHITECTURE.md                 # This file
├── src/
│   ├── UrduPlugin.cpp              # Plugin entry point, FrontendAction registration
│   ├── UrduPreprocessor.cpp        # Source-to-source Urdu to C token replacement
│   ├── UrduKeywordMap.cpp          # The keyword lookup table
│   ├── UrduDiagConsumer.cpp        # Urdu diagnostic message translation
│   └── UrduBidiHelper.cpp          # RTL/bidi text utilities
├── include/
│   ├── UrduPreprocessor.h
│   ├── UrduKeywordMap.h
│   ├── UrduDiagConsumer.h
│   └── UrduBidiHelper.h
├── test/
│   ├── hello_urdu.c                # Hello World in Urdu
│   ├── fibonacci_urdu.c            # Fibonacci in Urdu
│   ├── structs_urdu.c              # Struct usage in Urdu
│   └── expected/                   # Expected outputs for testing
├── scripts/
│   ├── urduc                       # Wrapper script: urduc myfile.c -o myfile
│   └── run_tests.sh
└── diagnostics/
    └── urdu_messages.json          # Urdu translations of common Clang diagnostics
```

## Implementation notes

### UTF-8 token boundary detection

Urdu characters are multi-byte in UTF-8 (3 to 4 bytes each). The preprocessor must:

- Not split tokens mid-codepoint.
- Handle mixed Urdu/ASCII identifiers (for example `myعدد`, which should not be a partial match).
- Respect string literals and comments, and not replace inside `"..."`, `//`, or `/* */`.

The scanner uses a small UTF-8 state machine and only replaces tokens at word boundaries (whitespace, operators, punctuation).

### Source location mapping

After replacing `اگر` (6 bytes in UTF-8) with `if` (2 bytes), source locations shift, and clang's error messages would point at the wrong columns. A position map relates transformed positions back to original positions. The plugin path uses `PreprocessorOptions::addRemappedFile()` with a `SourceManager` overlay; the standalone tool keeps line numbers exact by preserving newlines.

### RTL terminal output

Urdu text renders right to left. Error messages that mix Urdu text with C identifiers (left to right) need bidi handling. Urdu segments are wrapped with Unicode bidi isolates (`U+2067` RLI, `U+2066` LRI, `U+2069` PDI). JSON output mode leaves rendering to the consumer.

### Preprocessor directive handling

`#شامل` needs special handling, because Clang's preprocessor runs before the plugin sees the tokens. The transformation has to run before Clang's preprocessor. Either intercept and remap the whole file in `BeginSourceFileAction`, or run as a standalone tool that pipes into clang. The standalone tool takes the second route.

## Example: Hello World in Urdu C

```c
#شامل <stdio.h>

عدد اصل() {
    چھاپو("!السلام علیکم، دنیا\n");
    واپس 0;
}
```

Compiles to:

```c
#include <stdio.h>

int main() {
    printf("!السلام علیکم، دنیا\n");
    return 0;
}
```

## Dependencies

- LLVM/Clang 17+ development libraries (`libclang-dev`, `llvm-dev`).
- CMake 3.20+.
- A C++17 compiler.
- ICU is optional; a lightweight UTF-8 decoder is used instead.
