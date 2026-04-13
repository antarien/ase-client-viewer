/**
 * DSL directive dispatcher.
 *
 * Routes NODE_BLOCK_DIRECTIVE / NODE_LEAF_DIRECTIVE nodes to the
 * specialist renderer for their directive name. Each renderer lives in
 * its own translation unit (vwr_dsl_*.cpp) and shares the helpers from
 * vwr_dsl_common.hpp + the inline markup builder from
 * vwr_rndr_inline.hpp. Unknown directive names fall through to
 * render_generic which keeps the content visible inside a purple panel.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_dsl_dispatch.hpp"
#include "vwr_dsl_hero.hpp"
#include "vwr_dsl_cols.hpp"
#include "vwr_dsl_card.hpp"
#include "vwr_dsl_tabs.hpp"
#include "vwr_dsl_acrn.hpp"
#include "vwr_dsl_tmln.hpp"
#include "vwr_dsl_stat.hpp"
#include "vwr_dsl_step.hpp"
#include "vwr_dsl_leaf.hpp"
#include "vwr_dsl_meda.hpp"
#include "vwr_dsl_misc.hpp"
#include "vwr_dsl_common.hpp"

namespace ase::viewer::render::dsl {

double render_directive(RenderContext& ctx, const ase::markdown::Node* node) {
    int cw = ctx.width - ctx.margin_left - ctx.margin_right;
    double h = 0.0;

    // Block directives.
    if      (name_is(node, "hero"))      h = render_hero(ctx, node, cw);
    else if (name_is(node, "callout"))   h = render_callout(ctx, node, cw);
    else if (name_is(node, "aside"))     h = render_aside(ctx, node, cw);
    else if (name_is(node, "quote"))     h = render_quote(ctx, node, cw);
    else if (name_is(node, "cards"))     h = render_cards(ctx, node, cw);
    else if (name_is(node, "tabs"))      h = render_tabs(ctx, node, cw);
    else if (name_is(node, "timeline"))  h = render_timeline(ctx, node, cw);
    else if (name_is(node, "steps"))     h = render_steps(ctx, node, cw);
    else if (name_is(node, "stats"))     h = render_stats(ctx, node, cw);
    else if (name_is(node, "accordion")) h = render_accordion(ctx, node, cw);
    else if (name_is(node, "columns"))   h = render_columns(ctx, node, cw);
    else if (name_is(node, "terminal"))  h = render_terminal(ctx, node, cw);
    else if (name_is(node, "matrix"))    h = render_matrix(ctx, node, cw);
    // Leaf directives.
    else if (name_is(node, "badge"))     h = render_badge(ctx, node, cw);
    else if (name_is(node, "divider"))   h = render_divider(ctx, cw);
    else if (name_is(node, "spacer"))    h = render_spacer(ctx);
    else if (name_is(node, "progress"))  h = render_progress(ctx, node, cw);
    else if (name_is(node, "kbd"))       h = render_kbd(ctx, node);
    // Generic fallback.
    else                                  h = render_generic(ctx, node, cw);

    ctx.y += h;
    return h;
}

}  // namespace ase::viewer::render::dsl
