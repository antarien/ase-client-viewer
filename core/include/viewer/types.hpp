#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for ase-client-viewer constants
 * @description Compile-time constants only - runtime state lives in Components. The viewer
 *              renders ASE documentation and design material; this file carries the display
 *              modes it can be switched into and nothing else.
 *
 *              THE TWO MODES ARE A PAIR, NOT A LIST WITH TWO ENTRIES SO FAR. TECH and DSGN
 *              are the two ways the same document is read - by whoever builds the thing and
 *              by whoever shapes it. A third mode would be a product decision, not a value
 *              appended here.
 *
 * @module      ase-client-viewer
 * @layer       5 (Client)
 * @created     2026-04-12
 * @modified    2026-08-22
 * @version     1.0.0
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] NO structs (structs belong in Components)
 *
 * ABBREVIATIONS
 *
 * VWR  = Viewer (module prefix in file names)
 * TECH = Technical (the build-facing reading of a document)
 * DSGN = Design (the shape-facing reading of the same document)
 * DSL  = Domain Specific Language (the render directives the viewer interprets)
 */

#include <cstdint>

namespace ase::viewer {

/**
 * DISPLAY MODES
 *
 * The numbers are the mode's identity, not its order: they are stored in preferences and read
 * back on the next start, so changing a value silently reopens a document in the other mode.
 */
constexpr uint8_t VIEWER_MODE_TECH = 0;  // Technical reading: structure, code, build detail
constexpr uint8_t VIEWER_MODE_DSGN = 1;  // Design reading: layout, typography, visual shape

}  // namespace ase::viewer
