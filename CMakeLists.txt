cmake_minimum_required(VERSION 3.20)
project(UrduClang VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Locate dependencies automatically.
# LLVM/Clang and zstd are often Homebrew-installed and not on CMake's default
# search path. Fill in the *_DIR hints from `brew --prefix` (or the usual
# prefixes) so a bare `cmake -S . -B build` works. Explicit -D overrides still
# win, since these are only set when empty.
find_program(BREW_EXECUTABLE brew)

function(_uc_hint_dir out_var pkg subpath)
    if(${out_var})
        return()
    endif()
    set(_prefixes "")
    if(BREW_EXECUTABLE)
        execute_process(COMMAND ${BREW_EXECUTABLE} --prefix ${pkg}
                        OUTPUT_VARIABLE _p OUTPUT_STRIP_TRAILING_WHITESPACE
                        ERROR_QUIET RESULT_VARIABLE _rc)
        if(_rc EQUAL 0)
            list(APPEND _prefixes "${_p}")
        endif()
    endif()
    list(APPEND _prefixes "/opt/homebrew/opt/${pkg}" "/usr/local/opt/${pkg}")
    foreach(_pre ${_prefixes})
        if(EXISTS "${_pre}/${subpath}")
            set(${out_var} "${_pre}/${subpath}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

_uc_hint_dir(LLVM_DIR  llvm lib/cmake/llvm)
_uc_hint_dir(Clang_DIR llvm lib/cmake/clang)
_uc_hint_dir(zstd_DIR  zstd lib/cmake/zstd)

# LLVM's exported targets reference zstd transitively; make sure find_package
# inside LLVMConfig can locate it.
if(zstd_DIR)
    get_filename_component(_zstd_prefix "${zstd_DIR}/../../.." ABSOLUTE)
    list(APPEND CMAKE_PREFIX_PATH "${_zstd_prefix}")
endif()

# Find LLVM and Clang
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

# Add LLVM definitions and include directories
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})

# Include our headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# LLVM is built with -fno-rtti, so we need to match that
if(NOT LLVM_ENABLE_RTTI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

# Source files for the plugin
set(URDU_CLANG_SOURCES
    src/UrduKeywordMap.cpp
    src/UrduBidiHelper.cpp
    src/UrduPreprocessor.cpp
    src/UrduTranslator.cpp
    src/UrduDiagConsumer.cpp
    src/UrduPlugin.cpp
)

# Build as a shared library (Clang plugin)
add_library(UrduClang SHARED ${URDU_CLANG_SOURCES})

# Link against Clang libraries
target_link_libraries(UrduClang
    PRIVATE
    clangAST
    clangBasic
    clangFrontend
    clangLex
    clangTooling
    clangRewrite
)

# Also build a standalone tool
add_executable(urduc-tool src/UrduTool.cpp ${URDU_CLANG_SOURCES})

target_link_libraries(urduc-tool
    PRIVATE
    clangAST
    clangBasic
    clangFrontend
    clangLex
    clangTooling
    clangRewrite
    clangDriver
    clangSerialization
    clangParse
    clangSema
    clangAnalysis
    clangEdit
    clangFormat
)

# Link LLVM libraries
llvm_map_components_to_libnames(llvm_libs
    Core
    Support
    Option
)

target_link_libraries(UrduClang PRIVATE ${llvm_libs})
target_link_libraries(urduc-tool PRIVATE ${llvm_libs})

# Installation
install(TARGETS UrduClang LIBRARY DESTINATION lib)
install(TARGETS urduc-tool RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/urdu-clang)
install(FILES diagnostics/urdu_messages.json DESTINATION share/urdu-clang)
install(FILES scripts/urduc DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE)
