cmake_minimum_required(VERSION 3.22)

#[[
ASE CLIENT BUILD CONFIGURATION

@file        CMakeLists.txt
@brief       Build configuration for ase-client-viewer
@description Native GTK4 markdown documentation viewer with ECS architecture.

@module      ase-client-viewer
@layer       5 (Clients)
@created     2026-04-12
@modified    2026-04-12
@version     00.00.01.00001 [seed]
]]

# Parse local VERSION if not building from root
if(NOT DEFINED ASE_CLIENT_VIEWER_VERSION)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
        message(FATAL_ERROR "VERSION file not found: ${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    endif()
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _VERSION_LINES)
    foreach(_LINE ${_VERSION_LINES})
        if(_LINE MATCHES "^ASE_CLIENT_VIEWER=(.*)$")
            set(ASE_CLIENT_VIEWER_VERSION "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    if(NOT DEFINED ASE_CLIENT_VIEWER_VERSION)
        message(FATAL_ERROR "ASE_CLIENT_VIEWER version not found in VERSION file")
    endif()
endif()

project(ase-client-viewer VERSION ${ASE_CLIENT_VIEWER_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Force -fPIC on all compilations (FetchContent libraries don't always respect the above)
if(NOT WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

# Generate version.hpp from template (SSOT)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/core/include/viewer/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/viewer/version.hpp"
    @ONLY
)

# Generate NerdFont icon name → UTF-8 codepoint table from the web SSOT
# (clients/ase-client-web/src/utils/icon-replacer.ts). The table is the
# direct source of truth for both the web and native viewers; keeping a
# build-time codegen guarantees the native viewer never drifts from the
# web's icon set.
set(ASE_NERDFONT_TS_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/../ase-client-web/src/utils/icon-replacer.ts")
set(ASE_NERDFONT_HPP_OUT
    "${CMAKE_CURRENT_BINARY_DIR}/include/render/nerdfont_table.hpp")
add_custom_command(
    OUTPUT  "${ASE_NERDFONT_HPP_OUT}"
    COMMAND ${CMAKE_COMMAND} -E env python3
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_nerdfont_table.py"
            "${ASE_NERDFONT_TS_SRC}"
            "${ASE_NERDFONT_HPP_OUT}"
    DEPENDS "${ASE_NERDFONT_TS_SRC}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_nerdfont_table.py"
    COMMENT "Generating nerdfont_table.hpp from icon-replacer.ts (SSOT)"
    VERBATIM
)
add_custom_target(viewer_nerdfont_table DEPENDS "${ASE_NERDFONT_HPP_OUT}")

message(STATUS "Configuring ${PROJECT_NAME} v${PROJECT_VERSION}")

# ── System Dependencies ──────────────────────────────────────────────
#
# gtkmm-4 / libadwaita-1 are consumed transitively through the ase::adp::gtk
# adapter linked below. Graphviz stays a direct pkg-config check because
# the adapter does not (and should not) know about the Mermaid render
# path.

find_package(PkgConfig REQUIRED)
pkg_check_modules(GRAPHVIZ IMPORTED_TARGET libgvc libcdt libcgraph)

# ── ase::adp::gtk adapter (L3 - 3rd party GTK isolation) ──────────────

if(NOT TARGET ase::adp::gtk)
    set(ASE_ADAPTER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../adapter)
    add_subdirectory(${ASE_ADAPTER_PATH}/ase-adp-gtk ${CMAKE_BINARY_DIR}/_deps/ase-adp-gtk-build)
endif()

# ── ase::utils (L0 - foundation) ─────────────────────────────────────
# Needed for ase::utils::format_hex_color used by theme.cpp when
# building the CSS string from the ase::colors SSOT.

if(NOT TARGET ase::utils)
    set(ASE_FOUNDATION_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../foundation)
    add_subdirectory(${ASE_FOUNDATION_PATH}/ase-utils ${CMAKE_BINARY_DIR}/_deps/ase-utils-build)
endif()

# ── ase::imgcache (shared) ───────────────────────────────────────────
# Shared LRU image cache for native Cairo/Pixbuf rendering. SSOT for
# all PNG/JPG/SVG loading in the DSL renderers (figure block, hero
# backgrounds, inline images). Owned exclusively by the viewer build —
# root CMakeLists.txt does NOT add this subdirectory (no double-add).
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/ase-imgcache
    ${CMAKE_BINARY_DIR}/_deps/ase-imgcache-build
)

# ── ase::adp::treesitter (adapter/) ───────────────────────────────────
# Header-only adapter that wraps the C grammar entry-points of tree-
# sitter behind ase::adp::treesitter::grammar_*() inline functions so the
# extern "C" linkage stays out of viewer source. Owned exclusively by
# the viewer build — root CMakeLists.txt does NOT add this either.
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../adapter/ase-adp-treesitter
    ${CMAKE_BINARY_DIR}/_deps/ase-adp-treesitter-build
)

# ── Core Library (platform-agnostic) ─────────────────────────────────

add_library(viewer_core STATIC
    core/src/engine.cpp

    core/include/viewer/engine.hpp
    core/include/viewer/types.hpp
    core/include/viewer/viewer_module.hpp
)

target_include_directories(viewer_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
)

target_compile_features(viewer_core PUBLIC cxx_std_20)

# ── Desktop Executable ───────────────────────────────────────────────

# ── Fetch cmark-gfm (needed by ase-markdown, fetched here for proper target scope) ──

if(NOT TARGET libcmark-gfm-static)
    include(FetchContent)
    FetchContent_Declare(cmark_gfm
        GIT_REPOSITORY https://github.com/github/cmark-gfm.git
        GIT_TAG 0.29.0.gfm.13
        GIT_SHALLOW TRUE
    )
    set(CMARK_TESTS OFF CACHE BOOL "" FORCE)
    set(CMARK_SHARED OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(cmark_gfm)
endif()

# ── Link ase-alloc (L0) ──────────────────────────────────────────────

if(NOT TARGET ase::alloc)
    set(ASE_FOUNDATION_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../foundation)
    add_subdirectory(${ASE_FOUNDATION_PATH}/ase-alloc ${CMAKE_BINARY_DIR}/_deps/ase-alloc-build)
endif()

# ── Link ase-markdown (L1) ───────────────────────────────────────────

if(NOT TARGET ase::markdown)
    set(ASE_CORE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../core)
    add_subdirectory(${ASE_CORE_PATH}/ase-markdown ${CMAKE_BINARY_DIR}/_deps/ase-markdown-build)
endif()

# ── Fetch tree-sitter (syntax highlighting) ─────────────────────────

if(NOT TARGET tree-sitter)
    include(FetchContent)
    FetchContent_Declare(tree_sitter
        GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
        GIT_TAG v0.22.6
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(tree_sitter)

    # tree-sitter is a C library — build from source as static lib
    if(NOT TARGET tree-sitter)
        file(GLOB TS_SOURCES ${tree_sitter_SOURCE_DIR}/lib/src/*.c)
        add_library(tree-sitter STATIC ${TS_SOURCES})
        target_include_directories(tree-sitter PUBLIC
            ${tree_sitter_SOURCE_DIR}/lib/include
            ${tree_sitter_SOURCE_DIR}/lib/src
        )
    endif()
endif()

# ── Fetch tree-sitter language grammars ─────────────────────────────

FetchContent_Declare(ts_cpp GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-cpp.git GIT_TAG v0.22.3 GIT_SHALLOW TRUE)
FetchContent_Declare(ts_c GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-c.git GIT_TAG v0.21.4 GIT_SHALLOW TRUE)
FetchContent_Declare(ts_python GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-python.git GIT_TAG v0.21.0 GIT_SHALLOW TRUE)
FetchContent_Declare(ts_bash GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-bash.git GIT_TAG v0.21.0 GIT_SHALLOW TRUE)
FetchContent_Declare(ts_json GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-json.git GIT_TAG v0.21.0 GIT_SHALLOW TRUE)
FetchContent_Declare(ts_typescript GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-typescript.git GIT_TAG v0.21.2 GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(ts_cpp ts_c ts_python ts_bash ts_json ts_typescript)

# Build grammar parsers as static libs
add_library(ts-grammar-cpp STATIC ${ts_cpp_SOURCE_DIR}/src/parser.c ${ts_cpp_SOURCE_DIR}/src/scanner.c)
target_include_directories(ts-grammar-cpp PRIVATE ${ts_cpp_SOURCE_DIR}/src ${tree_sitter_SOURCE_DIR}/lib/include)

add_library(ts-grammar-c STATIC ${ts_c_SOURCE_DIR}/src/parser.c)
target_include_directories(ts-grammar-c PRIVATE ${ts_c_SOURCE_DIR}/src ${tree_sitter_SOURCE_DIR}/lib/include)

add_library(ts-grammar-python STATIC ${ts_python_SOURCE_DIR}/src/parser.c ${ts_python_SOURCE_DIR}/src/scanner.c)
target_include_directories(ts-grammar-python PRIVATE ${ts_python_SOURCE_DIR}/src ${tree_sitter_SOURCE_DIR}/lib/include)

add_library(ts-grammar-bash STATIC ${ts_bash_SOURCE_DIR}/src/parser.c ${ts_bash_SOURCE_DIR}/src/scanner.c)
target_include_directories(ts-grammar-bash PRIVATE ${ts_bash_SOURCE_DIR}/src ${tree_sitter_SOURCE_DIR}/lib/include)

add_library(ts-grammar-json STATIC ${ts_json_SOURCE_DIR}/src/parser.c)
target_include_directories(ts-grammar-json PRIVATE ${ts_json_SOURCE_DIR}/src ${tree_sitter_SOURCE_DIR}/lib/include)

add_library(ts-grammar-typescript STATIC ${ts_typescript_SOURCE_DIR}/typescript/src/parser.c ${ts_typescript_SOURCE_DIR}/typescript/src/scanner.c)
target_include_directories(ts-grammar-typescript PRIVATE ${ts_typescript_SOURCE_DIR}/typescript/src ${tree_sitter_SOURCE_DIR}/lib/include)

# ── Link ase-adp-microtex (adapter/) ────────────────────────────
# LaTeX math rendering is handled by the dedicated adapter subgit which
# isolates MicroTeX's tex::* inheritance from client code. We consume
# only the clean ase::adp::microtex public API (the C++ namespace is still
# ase::adp::microtex; the renamed CMake target alias is ase::adp::microtex).
if(NOT TARGET ase::adp::microtex)
    set(ASE_ADAPTER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../adapter)
    add_subdirectory(${ASE_ADAPTER_PATH}/ase-adp-microtex ${CMAKE_BINARY_DIR}/_deps/ase-adp-microtex-build)
endif()

# ── Fetch muparser (ASE-Math plots) ─────────────────────────────────

if(NOT TARGET muparser)
    FetchContent_Declare(muparser
        GIT_REPOSITORY https://github.com/beltoforion/muparser.git
        GIT_TAG v2.3.4
        GIT_SHALLOW TRUE
    )
    set(ENABLE_SAMPLES OFF CACHE BOOL "" FORCE)
    set(ENABLE_OPENMP OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(ENABLE_WIDE_CHAR OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(muparser)
    if(TARGET muparser)
        set_target_properties(muparser PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()
endif()

# ── Desktop Executable ───────────────────────────────────────────────

#
# SOLID/SSOT layout: main.cpp is a pure orchestrator (~100 lines).
# Every feature lives in its own translation unit with a single
# responsibility under include/viewer and src/. Add new features by
# dropping a new .cpp file here and an .hpp file under include/viewer.

add_executable(ase-viewer
    src/main.cpp
    src/window.cpp
    src/theme.cpp
    src/fs_utils.cpp
    src/read_tracker.cpp
    src/file_monitor.cpp
    src/header_bar.cpp
    src/status_bar.cpp
    src/keyboard.cpp
    src/tree_view.cpp
    src/canvas.cpp
    src/settings.cpp
    src/preferences.cpp
    src/render/vwr_rndr_text.cpp
    src/render/vwr_rndr_inline.cpp
    src/render/vwr_rndr_doc.cpp
    src/render/vwr_rndr_synt.cpp
    src/render/vwr_rndr_math.cpp
    src/render/vwr_rndr_mmrd.cpp
    src/render/vwr_rndr_amdl.cpp
    src/render/dsl/vwr_dsl_dispatch.cpp
    src/render/dsl/vwr_dsl_hero.cpp
    src/render/dsl/vwr_dsl_cols.cpp
    src/render/dsl/vwr_dsl_card.cpp
    src/render/dsl/vwr_dsl_tabs.cpp
    src/render/dsl/vwr_dsl_acrn.cpp
    src/render/dsl/vwr_dsl_tmln.cpp
    src/render/dsl/vwr_dsl_stat.cpp
    src/render/dsl/vwr_dsl_step.cpp
    src/render/dsl/vwr_dsl_leaf.cpp
    src/render/dsl/vwr_dsl_meda.cpp
    src/render/dsl/vwr_dsl_misc.cpp
)

target_include_directories(ase-viewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../sha-client-web/sha-web-console/generated
)
add_dependencies(ase-viewer viewer_nerdfont_table)

# Web public root — used by the image renderer to resolve absolute
# /cms/images/... paths that the CMS-style demo files reference. The
# native viewer is built against the same image assets the web client
# ships, so the SSOT path is clients/ase-client-web/public/.
target_compile_definitions(ase-viewer PRIVATE
    ASE_WEB_PUBLIC_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/../ase-client-web/public"
)

target_include_directories(ase-viewer PRIVATE
    ${tree_sitter_SOURCE_DIR}/lib/include
    ${ts_cpp_SOURCE_DIR}/src
    ${ts_c_SOURCE_DIR}/src
    ${ts_python_SOURCE_DIR}/src
    ${ts_bash_SOURCE_DIR}/src
    ${ts_json_SOURCE_DIR}/src
    ${ts_typescript_SOURCE_DIR}/typescript/src
)

target_link_libraries(ase-viewer
    PRIVATE
        viewer_core
        ase::markdown
        ase::utils          # format_hex_color for theme.cpp SSOT
        ase::imgcache       # shared LRU image cache (SSOT for image loading)
        ase::adp::treesitter # header-only adapter wrapping tree-sitter grammar lookups
        ase::adp::gtk        # transitively exports gtkmm-4 + libadwaita-1
        tree-sitter
        ts-grammar-cpp
        ts-grammar-c
        ts-grammar-python
        ts-grammar-bash
        ts-grammar-json
        ts-grammar-typescript
        muparser
        ase::adp::microtex
)

# Graphviz for Mermaid rendering (optional — compiled with HAVE_GRAPHVIZ)
if(GRAPHVIZ_FOUND)
    target_link_libraries(ase-viewer PRIVATE PkgConfig::GRAPHVIZ)
    target_compile_definitions(ase-viewer PRIVATE HAVE_GRAPHVIZ)
endif()

set_target_properties(ase-viewer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Installation
include(GNUInstallDirs)
install(TARGETS ase-viewer DESTINATION ${CMAKE_INSTALL_BINDIR})
