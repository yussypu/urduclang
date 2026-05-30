# اردو-کلینگ (Urdu-Clang)

Write C using Urdu-script keywords. `urduc-tool` rewrites Urdu tokens (`عدد`, `اگر`, `واپس`, `چھاپو`) into standard C, compiles the result with Clang, and reports any errors in Urdu, pointing back at your original file.

```c
#شامل <stdio.h>

عدد اصل() {
    چھاپو("!السلام علیکم، دنیا\n");
    واپس 0;
}
```
```bash
./build/urduc-tool examples/hello.c -o hello && ./hello
```

## What it is

A source-to-source transpiler that sits in front of Clang.

`urdu::Preprocessor::transform()` (`src/UrduPreprocessor.cpp`) is a UTF-8 aware token scanner. It looks each identifier and directive up in a keyword map (`src/UrduKeywordMap.cpp`) and substitutes the C equivalent. String literals, char literals, and comments are tokenized separately, so their contents are never rewritten. The transform is line-preserving, so error line numbers match your source.

`src/UrduTool.cpp` is the driver. It writes the transformed C to a temp file, compiles it with `clang`, then captures clang's diagnostics and rewrites them: the temp path becomes your filename and the message is translated to Urdu via `src/UrduTranslator.cpp`. The process exits with clang's real exit code.

`diagnostics/urdu_messages.json` holds about 60 English to Urdu diagnostic phrases. They are merged on top of a small built-in set and loaded automatically at runtime.

## Building

Built against Homebrew LLVM 20.1.2 on macOS (Apple clang host). A plain configure works, because CMake locates Homebrew LLVM and zstd for you.

```bash
brew install llvm zstd        # one-time, if not already present
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
scripts/run_tests.sh
```

`run_tests.sh` transforms, compiles, runs, and diffs each program's output against `test/expected/`. All five samples pass:

| test | transform | compile | run | output vs expected |
|------|-----------|---------|-----|--------------------|
| `hello_urdu`     | pass | pass | pass | pass |
| `fibonacci_urdu` | pass | pass | pass | pass |
| `loops_urdu`     | pass | pass | pass | pass |
| `structs_urdu`   | pass | pass | pass | pass |
| `switch_urdu`    | pass | pass | pass | pass |

## Usage

```bash
./build/urduc-tool -E file.c             # show the transformed C (no compile)
./build/urduc-tool file.c -o prog        # compile and link
./build/urduc-tool file.c -o prog -O2    # extra flags are forwarded to clang
scripts/urduc file.c -o prog             # wrapper (prefers urduc-tool)
```

### Urdu diagnostics

A real compile error is reported in Urdu, against your file, on the right line:

```
$ ./build/urduc-tool broken.c -o x
broken.c:3:13: خطا: اظہار متوقع تھا
    3 |     int x = ;
      |             ^
✗ تالیف ناکام            # exit code 1
```

The caret line still shows the generated C (`int x = ;`), since that is what clang sees. The message, the level label, the filename, and the line and column are all mapped back. Exit codes propagate, so this composes in scripts and `make`.

## How errors flow

```
your.c --transform--> /tmp/urduc-your-XXXX.c --clang--> stderr (English, temp path)
                                                            |
                          translate + rewrite temp to your.c
                                                            |
                                                   Urdu, your.c --> you
```

Because the transform preserves newlines, line numbers survive the round trip. Columns can shift on lines that contained Urdu (Urdu code points and their C keywords differ in byte width); line-level accuracy is exact.

## Keyword reference

Source of truth: `src/UrduKeywordMap.cpp`. A few highlights:

| Urdu | C | | Urdu | C |
|------|---|-|------|---|
| `عدد` | `int` | | `اگر` | `if` |
| `حرف` | `char` | | `ورنہ` | `else` |
| `خالی` | `void` | | `جبکہ` | `while` |
| `ساخت` | `struct` | | `کیلئے` | `for` |
| `#شامل` | `#include` | | `واپس` | `return` |
| `چھاپو` | `printf` | | `بدل` | `switch` |
| `پڑھو` | `scanf` | | `اصل` | `main` |

## Layout

```
src/        UrduTool.cpp (entry: transform + drive clang + Urdu diagnostics),
            UrduPreprocessor.cpp (engine), UrduKeywordMap.cpp (table),
            UrduTranslator.cpp (English to Urdu catalog),
            UrduDiagConsumer.cpp (diagnostic consumer for the plugin path),
            UrduBidiHelper.cpp (UTF-8/RTL), UrduPlugin.cpp (-fplugin target)
include/    headers
test/       *.c samples + expected/ golden output for all five
examples/   scratch sources (not part of the build or tests)
scripts/    urduc (wrapper), run_tests.sh
diagnostics/urdu_messages.json  (auto-loaded at runtime)
```

## Notes and limits

There are two build outputs. `urduc-tool` is the standalone, tested transpiler. `libUrduClang.dylib` is an experimental `-fplugin` `FrontendAction` (see `ARCHITECTURE.md`); it builds, but the standalone tool is the supported path.

The setup is tuned for macOS. Clang discovery and the SDK `-isysroot` use Homebrew and Xcode paths. On Linux it should work once `clang` is on `PATH`, but that has not been exercised here.

Diagnostic coverage is the curated catalog in `diagnostics/urdu_messages.json` plus the built-ins. Phrases outside it pass through in English.
