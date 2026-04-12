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
@version     00.00.01.00001 [stub]
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

project(ase-client-viewer VERSION ${ASE_CLIENT_VIEWER_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Generate version.hpp from template (SSOT)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/core/include/viewer/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/viewer/version.hpp"
    @ONLY
)

message(STATUS "Configuring ${PROJECT_NAME} v${PROJECT_VERSION}")

# ── System Dependencies ──────────────────────────────────────────────

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)

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

# ── Link ase-markdown (L1) ───────────────────────────────────────────

if(NOT TARGET ase::markdown)
    set(ASE_CORE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../core)
    add_subdirectory(${ASE_CORE_PATH}/ase-markdown ${CMAKE_BINARY_DIR}/_deps/ase-markdown-build)
endif()

# ── Desktop Executable ───────────────────────────────────────────────

add_executable(ase-viewer
    src/main.cpp
    src/render/vwr_rndr_text.cpp
    src/render/vwr_rndr_doc.cpp
    src/render/vwr_rndr_synt.cpp
)

target_include_directories(ase-viewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(ase-viewer
    PRIVATE
        viewer_core
        ase::markdown
        PkgConfig::GTKMM
)

set_target_properties(ase-viewer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Installation
include(GNUInstallDirs)
install(TARGETS ase-viewer DESTINATION ${CMAKE_INSTALL_BINDIR})
