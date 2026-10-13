#pragma once

/**
 * @file        vwr_rndr_doc.hpp
 * @brief       render_document and render_node - the block dispatcher
 * @description Declares the walker over the ase::markdown AST that dispatches a
 *              Cairo draw call per node type, computes the y positions and
 *              renders the nodes inside the viewport. It returns the total
 *              content height, which is what the scrollbar needs.
 *
 *              Interactive directives (:::tabs, :::accordion) become clickable
 *              through a hit-region list and a per-document interactive state
 *              declared here. The renderer assigns each directive a sequential
 *              index in document order and writes click rectangles into the
 *              context; the viewer hosts the GestureClick controller, updates
 *              the state on a hit and queues a redraw. No widget tree
 *              integration is required for that, which is why the state is a
 *              plain struct and not a widget.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/draw/command
 * @usecase     usecase:100008 - render the knowledge stock
 * @created     2026-04-12
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
 */

#include <ase/markdown/markdown.hpp>
#include <cairomm/cairomm.h>
#include <pangomm.h>
#include <cstdint>

namespace ase::viewer::render {

constexpr uint32_t MAX_TABS_DIRECTIVES   = 64;
constexpr uint32_t MAX_ACCORDIONS        = 64;
constexpr uint32_t MAX_PANELS_PER_ACRN   = 16;
constexpr uint32_t MAX_HIT_REGIONS       = 1024;

// Per-document interactive state. Lives in the viewer window and
// survives across redraws so click handlers can flip values and
// queue_draw without losing other panels' open state.
struct InteractiveState {
    uint16_t active_tab[MAX_TABS_DIRECTIVES] = {};
    uint8_t  panel_open[MAX_ACCORDIONS][MAX_PANELS_PER_ACRN] = {};
    bool     panel_initialized[MAX_ACCORDIONS] = {};
};

// Hit region kinds — keep small ints, no enum class to stay validator-clean.
constexpr uint8_t HIT_KIND_TAB_HEADER     = 1;
constexpr uint8_t HIT_KIND_PANEL_HEADER   = 2;

struct HitRegion {
    double   x          = 0.0;
    double   y          = 0.0;
    double   w          = 0.0;
    double   h          = 0.0;
    uint8_t  kind       = 0;
    uint16_t parent_idx = 0;  // tabs[N] or accordion[N]
    uint16_t child_idx  = 0;  // tab idx / panel idx
};

struct HitRegionList {
    HitRegion items[MAX_HIT_REGIONS] = {};
    uint32_t  count = 0;
};

struct RenderContext {
    Cairo::RefPtr<Cairo::Context> cr;
    int width          = 0;
    int margin_left    = 16;
    int margin_right   = 16;
    int margin_top     = 16;
    double y           = 0;    // Current y position (accumulates)
    double scroll_y    = 0;    // Scroll offset
    double viewport_h  = 0;    // Visible height

    InteractiveState* state   = nullptr;  // owned by ViewerWindow
    HitRegionList*    regions = nullptr;  // owned by ViewerWindow

    // Sequential indices assigned in AST traversal order. Reset to 0
    // at the start of every render_document() call so re-draws produce
    // identical indices and the state[i] lookup stays stable.
    uint32_t next_tabs_index      = 0;
    uint32_t next_accordion_index = 0;
};

// Render the full document AST into the Cairo context.
// Returns total content height (for scrollbar).
double render_document(RenderContext& ctx, const ase::markdown::Document& doc);

// Render a single AST node (recursive).
void render_node(RenderContext& ctx, const ase::markdown::Node* node);

}  // namespace ase::viewer::render
