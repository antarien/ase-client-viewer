/**
 * DSL generic / fallback directive renderer.
 *
 * Used by the dispatcher when no specialist renderer matches the
 * directive name. Renders the inline children inside a tinted purple
 * panel with the directive name as a small label so unsupported
 * directives stay visible (instead of silently becoming empty boxes).
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_dsl_misc.hpp"

#include <ase/imgcache/image_cache.hpp>
#include "../vwr_rndr_image_path.hpp"

namespace ase::viewer::render::dsl {

double render_generic(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Fallback for unknown directives. No direct web counterpart — uses
    // the standard block scale (p-4 = 16, text-xs label, text-sm body)
    // so unknown blocks fit visually next to known ones.
    constexpr double PAD     = 16.0;
    constexpr int    LBL_PX  = 12;   // text-xs
    constexpr int    BODY_PX = 14;   // text-sm
    constexpr double GAP     = 4.0;

    std::string body_markup = build_inline_content(node);
    auto body_layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(PAD));
    auto body_fd = body_layout->get_font_description();
    body_fd.set_size(BODY_PX * Pango::SCALE);
    body_layout->set_font_description(body_fd);
    body_layout->set_markup(body_markup);
    int bw = 0, bh = 0;
    body_layout->get_pixel_size(bw, bh);

    const double label_h = LBL_PX + GAP;
    const double total_h = (body_markup.empty() ? (label_h + 2 * PAD)
                                                : (label_h + bh + 2 * PAD));

    ctx.cr->set_source_rgba(PURPLE_R, PURPLE_G, PURPLE_B, 0.04);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, 4);
    ctx.cr->fill();

    ctx.cr->set_source_rgba(PURPLE_R, PURPLE_G, PURPLE_B, 0.30);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, 3, total_h);
    ctx.cr->fill();

    const char* name = (node->directive_name != nullptr) ? node->directive_name : "directive";
    auto label_layout = create_body_layout(ctx.cr, cw);
    auto fd = label_layout->get_font_description();
    fd.set_size(LBL_PX * Pango::SCALE);
    fd.set_weight(Pango::Weight::BOLD);
    label_layout->set_font_description(fd);
    label_layout->set_text(name);
    ctx.cr->set_source_rgb(PURPLE_R, PURPLE_G, PURPLE_B);
    draw_layout(ctx.cr, label_layout, ctx.margin_left + PAD, ctx.y + PAD);

    if (!body_markup.empty()) {
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_b(::ase::colors::TEXT_PANEL_MUTED));
        draw_layout(ctx.cr, body_layout, ctx.margin_left + PAD, ctx.y + PAD + label_h);
    }

    return total_h + 16.0;  // my-4 outer
}

// ─────────────────────────────────────────────────────────────────────
// toc — auto-generated table of contents
// ─────────────────────────────────────────────────────────────────────
//
// The web variant uses an IntersectionObserver against #h1..#h6 elements
// in the rendered DOM to highlight the active heading. Native viewer has
// no scroll-spy concept, and the AST walker that calls render_directive
// does not carry earlier headings — full TOC requires a heading collector
// pre-pass that mirrors the IntersectionObserver semantics in render
// context state.
//
// This implementation renders a labelled box so the directive stays
// visible and the doc author sees their `:::toc` was recognised. Full
// heading enumeration ships with the render-context-state extension.
double render_toc(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    (void)node;
    constexpr double PAD     = 16.0;
    constexpr int    LBL_PX  = 10;   // text-2xs
    constexpr int    BODY_PX = 12;   // text-xs
    constexpr double RADIUS  = 4.0;

    auto label_layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(PAD));
    auto lfd = label_layout->get_font_description();
    lfd.set_size(LBL_PX * Pango::SCALE);
    lfd.set_weight(Pango::Weight::BOLD);
    label_layout->set_font_description(lfd);
    label_layout->set_text("TABLE OF CONTENTS");
    int lw = 0, lh = 0;
    label_layout->get_pixel_size(lw, lh);

    auto body_layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(PAD));
    auto bfd = body_layout->get_font_description();
    bfd.set_size(BODY_PX * Pango::SCALE);
    body_layout->set_font_description(bfd);
    body_layout->set_text("(auto-generated from document headings)");
    int bw = 0, bh = 0;
    body_layout->get_pixel_size(bw, bh);

    const double total_h = 2 * PAD + lh + 12 + bh;

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_TOC_BG),
                           rgb_g(::ase::colors::CMS_TOC_BG),
                           rgb_b(::ase::colors::CMS_TOC_BG));
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, RADIUS);
    ctx.cr->fill();

    ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                            rgb_g(::ase::colors::BORDER_A20),
                            rgb_b(::ase::colors::BORDER_A20),
                            rgb_a(::ase::colors::BORDER_A20));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, RADIUS);
    ctx.cr->stroke();

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_LABEL),
                           rgb_g(::ase::colors::TEXT_LABEL),
                           rgb_b(::ase::colors::TEXT_LABEL));
    draw_layout(ctx.cr, label_layout, ctx.margin_left + PAD, ctx.y + PAD);

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_TOC_LINK),
                           rgb_g(::ase::colors::CMS_TOC_LINK),
                           rgb_b(::ase::colors::CMS_TOC_LINK));
    draw_layout(ctx.cr, body_layout, ctx.margin_left + PAD, ctx.y + PAD + lh + 12);

    return total_h + 16.0;  // my-4 outer
}

// ─────────────────────────────────────────────────────────────────────
// changelog
// ─────────────────────────────────────────────────────────────────────
double render_changelog(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsChangelogBlock.tsx):
    //   container: my-4, bg CMS_CHANGELOG_BG, border 1 SURFACE_3, radius 4
    //   header (if version|date): px-4 py-3, gap-3, border-bottom SURFACE_3
    //     version: text-sm bold CMS_CHANGELOG_ICON
    //     date:    text-xs TEXT_PANEL_MUTED
    //   body: px-4 py-3, text-sm, list items colorized by prefix
    constexpr double PAD_X    = 16.0;
    constexpr double PAD_Y    = 12.0;
    constexpr double GAP      = 12.0;
    constexpr int    VER_PX   = 14;
    constexpr int    DATE_PX  = 12;
    constexpr int    BODY_PX  = 14;
    constexpr double RADIUS   = 4.0;

    const char* version = get_attr(node, "version");
    const char* date    = get_attr(node, "date");

    // Body: build inline content (lists/paragraphs as Pango markup).
    std::string body_md = build_inline_content(node);
    auto body_layout = create_body_layout(ctx.cr, cw - 2 * static_cast<int>(PAD_X));
    auto bfd = body_layout->get_font_description();
    bfd.set_size(BODY_PX * Pango::SCALE);
    body_layout->set_font_description(bfd);
    body_layout->set_markup(body_md);
    int bw = 0, bh = 0;
    body_layout->get_pixel_size(bw, bh);

    const double hdr_h = (version || date) ? (VER_PX + 2 * PAD_Y) : 0.0;
    const double body_h = bh + 2 * PAD_Y;
    const double total_h = hdr_h + body_h;

    // Container background
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_CHANGELOG_BG),
                           rgb_g(::ase::colors::CMS_CHANGELOG_BG),
                           rgb_b(::ase::colors::CMS_CHANGELOG_BG));
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, RADIUS);
    ctx.cr->fill();

    // Header
    if (hdr_h > 0) {
        double hx = ctx.margin_left + PAD_X;
        const double hy = ctx.y + PAD_Y;

        if (version != nullptr) {
            std::string v_markup;
            ::ase::viewer::render::append_escaped(v_markup, version);
            auto v_layout = create_body_layout(ctx.cr, cw);
            auto fd = v_layout->get_font_description();
            fd.set_size(VER_PX * Pango::SCALE);
            fd.set_weight(Pango::Weight::BOLD);
            v_layout->set_font_description(fd);
            v_layout->set_markup(v_markup);
            int vw = 0, vh = 0;
            v_layout->get_pixel_size(vw, vh);
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_CHANGELOG_ICON),
                                   rgb_g(::ase::colors::CMS_CHANGELOG_ICON),
                                   rgb_b(::ase::colors::CMS_CHANGELOG_ICON));
            draw_layout(ctx.cr, v_layout, hx, hy);
            hx += vw + GAP;
        }
        if (date != nullptr) {
            std::string d_markup;
            ::ase::viewer::render::append_escaped(d_markup, date);
            auto d_layout = create_body_layout(ctx.cr, cw);
            auto dfd = d_layout->get_font_description();
            dfd.set_size(DATE_PX * Pango::SCALE);
            d_layout->set_font_description(dfd);
            d_layout->set_markup(d_markup);
            ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                                   rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                                   rgb_b(::ase::colors::TEXT_PANEL_MUTED));
            draw_layout(ctx.cr, d_layout, hx, hy + 2);
        }

        // Border-bottom under header
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::SURFACE_3),
                               rgb_g(::ase::colors::SURFACE_3),
                               rgb_b(::ase::colors::SURFACE_3));
        ctx.cr->set_line_width(1);
        ctx.cr->move_to(ctx.margin_left, ctx.y + hdr_h);
        ctx.cr->line_to(ctx.margin_left + cw, ctx.y + hdr_h);
        ctx.cr->stroke();
    }

    // Body
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                           rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                           rgb_b(::ase::colors::TEXT_PANEL_WHITE));
    draw_layout(ctx.cr, body_layout, ctx.margin_left + PAD_X, ctx.y + hdr_h + PAD_Y);

    // Container border
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::SURFACE_3),
                           rgb_g(::ase::colors::SURFACE_3),
                           rgb_b(::ase::colors::SURFACE_3));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, RADIUS);
    ctx.cr->stroke();

    return total_h + 16.0;
}

// ─────────────────────────────────────────────────────────────────────
// team / member
// ─────────────────────────────────────────────────────────────────────
namespace {

// Render a single ::member card at (x, y) with width w. Returns card height.
double render_member_card(RenderContext& ctx, const ase::markdown::Node* member,
                          double x, double y, int w) {
    constexpr double PAD       = 16.0;
    constexpr double AVATAR_D  = 64.0;
    constexpr double AVATAR_MB = 12.0;  // mb-3
    constexpr int    NAME_PX   = 14;
    constexpr int    ROLE_PX   = 12;
    constexpr int    BODY_PX   = 12;
    constexpr double RADIUS    = 4.0;

    const char* name_attr   = get_attr(member, "name");
    const char* avatar_attr = get_attr(member, "avatar");
    const char* role_attr   = get_attr(member, "role");

    const std::string body_md = build_inline_content(member);

    // Pre-measure body (text-xs, centered)
    auto body_layout = create_body_layout(ctx.cr, w - 2 * static_cast<int>(PAD));
    auto bfd = body_layout->get_font_description();
    bfd.set_size(BODY_PX * Pango::SCALE);
    body_layout->set_font_description(bfd);
    body_layout->set_markup(body_md);
    int bw = 0, bh = 0;
    if (!body_md.empty()) body_layout->get_pixel_size(bw, bh);

    double content_h = 2 * PAD;
    if (avatar_attr != nullptr) content_h += AVATAR_D + AVATAR_MB;
    if (name_attr != nullptr)   content_h += NAME_PX;
    if (role_attr != nullptr)   content_h += ROLE_PX + 4;
    if (!body_md.empty())       content_h += bh + 8;

    // Background
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_TEAM_BG),
                           rgb_g(::ase::colors::CMS_TEAM_BG),
                           rgb_b(::ase::colors::CMS_TEAM_BG));
    rounded_rect(ctx.cr, x, y, w, content_h, RADIUS);
    ctx.cr->fill();

    // Border
    ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                            rgb_g(::ase::colors::BORDER_A20),
                            rgb_b(::ase::colors::BORDER_A20),
                            rgb_a(::ase::colors::BORDER_A20));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, x, y, w, content_h, RADIUS);
    ctx.cr->stroke();

    double cy = y + PAD;

    // Avatar (round image with PANEL_CYAN border)
    if (avatar_attr != nullptr) {
        const std::string path = ::ase::viewer::render::resolve_image_path(avatar_attr);
        const double ax = x + (w - AVATAR_D) / 2.0;
        ctx.cr->save();
        ctx.cr->arc(ax + AVATAR_D / 2.0, cy + AVATAR_D / 2.0,
                    AVATAR_D / 2.0, 0, 2 * M_PI);
        ctx.cr->clip();
        ::ase::imgcache::render(ctx.cr, path, ax, cy, static_cast<int>(AVATAR_D));
        ctx.cr->restore();
        // Cyan ring
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::PANEL_CYAN),
                               rgb_g(::ase::colors::PANEL_CYAN),
                               rgb_b(::ase::colors::PANEL_CYAN));
        ctx.cr->set_line_width(2);
        ctx.cr->arc(ax + AVATAR_D / 2.0, cy + AVATAR_D / 2.0,
                    AVATAR_D / 2.0, 0, 2 * M_PI);
        ctx.cr->stroke();
        cy += AVATAR_D + AVATAR_MB;
    }

    // Name (centered)
    if (name_attr != nullptr) {
        std::string n_markup;
        ::ase::viewer::render::append_escaped(n_markup, name_attr);
        auto n_layout = create_body_layout(ctx.cr, w - 2 * static_cast<int>(PAD));
        auto fd = n_layout->get_font_description();
        fd.set_size(NAME_PX * Pango::SCALE);
        fd.set_weight(Pango::Weight::BOLD);
        n_layout->set_font_description(fd);
        n_layout->set_markup(n_markup);
        int nw = 0, nh = 0;
        n_layout->get_pixel_size(nw, nh);
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_b(::ase::colors::TEXT_PANEL_WHITE));
        draw_layout(ctx.cr, n_layout, x + (w - nw) / 2.0, cy);
        cy += NAME_PX;
    }

    // Role (centered, mt-1)
    if (role_attr != nullptr) {
        std::string r_markup;
        ::ase::viewer::render::append_escaped(r_markup, role_attr);
        auto r_layout = create_body_layout(ctx.cr, w - 2 * static_cast<int>(PAD));
        auto fd = r_layout->get_font_description();
        fd.set_size(ROLE_PX * Pango::SCALE);
        r_layout->set_font_description(fd);
        r_layout->set_markup(r_markup);
        int rw = 0, rh = 0;
        r_layout->get_pixel_size(rw, rh);
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_b(::ase::colors::TEXT_PANEL_MUTED));
        draw_layout(ctx.cr, r_layout, x + (w - rw) / 2.0, cy + 4);
        cy += ROLE_PX + 4;
    }

    // Body bio (centered, mt-2)
    if (!body_md.empty()) {
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_b(::ase::colors::TEXT_PANEL_MUTED));
        draw_layout(ctx.cr, body_layout, x + (w - bw) / 2.0, cy + 8);
    }

    return content_h;
}

}  // namespace

double render_team(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsTeamBlock.tsx):
    //   container: my-4, grid gap 1rem (16)
    //   default cols=4
    constexpr double GAP = 16.0;

    int cols = get_attr_int(node, "cols", 4);
    if (cols < 1) cols = 1;

    const uint32_t total = count_children_named(node, "member");
    if (total == 0) return 16.0;

    const int col_w = (cw - static_cast<int>(GAP) * (cols - 1)) / cols;

    double row_y = ctx.y;
    int idx = 0;
    double total_h = 0.0;

    while (idx < static_cast<int>(total)) {
        const ase::markdown::Node* iter = first_child_named(node, "member");
        for (int skip = 0; iter != nullptr && skip < idx; ++skip) {
            iter = next_child_named(iter, "member");
        }
        const ase::markdown::Node* row_first = iter;

        int row_count = 0;
        while (iter != nullptr && row_count < cols) {
            iter = next_child_named(iter, "member");
            ++row_count;
        }

        double max_h = 0.0;
        iter = row_first;
        for (int col = 0; col < row_count && iter != nullptr; ++col) {
            const double cx = ctx.margin_left + col * (col_w + GAP);
            const double h = render_member_card(ctx, iter, cx, row_y, col_w);
            if (h > max_h) max_h = h;
            iter = next_child_named(iter, "member");
        }

        row_y   += max_h + GAP;
        total_h += max_h + GAP;
        idx     += row_count;
    }

    return total_h - GAP + 16.0;
}

double render_author(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsAuthorBlock.tsx):
    //   container: my-4, flex items-start gap-4, p-4, bg CMS_AUTHOR_BG,
    //              border 1 BORDER_A20, radius 6
    //   avatar: 56×56 round, border 2 CMS_AUTHOR_BORDER
    //   name:   text-sm bold TEXT_PANEL_WHITE
    //   role:   text-2xs uppercase tracking-widest mt-0.5 CMS_AUTHOR_BORDER
    //   bio:    text-xs mt-2 TEXT_PANEL_MUTED
    constexpr double PAD       = 16.0;
    constexpr double GAP       = 16.0;  // gap-4
    constexpr double AVATAR_D  = 56.0;
    constexpr int    NAME_PX   = 14;
    constexpr int    ROLE_PX   = 10;   // text-2xs
    constexpr int    BODY_PX   = 12;
    constexpr double RADIUS    = 6.0;

    const char* name_attr   = get_attr(node, "name");
    const char* avatar_attr = get_attr(node, "avatar");
    const char* role_attr   = get_attr(node, "role");

    const std::string body_md = build_inline_content(node);

    const int text_w = cw - 2 * static_cast<int>(PAD) - static_cast<int>(AVATAR_D) - static_cast<int>(GAP);

    auto body_layout = create_body_layout(ctx.cr, text_w);
    auto bfd = body_layout->get_font_description();
    bfd.set_size(BODY_PX * Pango::SCALE);
    body_layout->set_font_description(bfd);
    body_layout->set_markup(body_md);
    int bw = 0, bh = 0;
    if (!body_md.empty()) body_layout->get_pixel_size(bw, bh);

    double text_h = 0;
    if (name_attr) text_h += NAME_PX;
    if (role_attr) text_h += ROLE_PX + 2;  // mt-0.5
    if (!body_md.empty()) text_h += bh + 8;  // mt-2

    const double content_h = (AVATAR_D > text_h ? AVATAR_D : text_h);
    const double total_h = content_h + 2 * PAD;

    // Background
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_AUTHOR_BG),
                           rgb_g(::ase::colors::CMS_AUTHOR_BG),
                           rgb_b(::ase::colors::CMS_AUTHOR_BG));
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, RADIUS);
    ctx.cr->fill();

    // Border
    ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                            rgb_g(::ase::colors::BORDER_A20),
                            rgb_b(::ase::colors::BORDER_A20),
                            rgb_a(::ase::colors::BORDER_A20));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, cw, total_h, RADIUS);
    ctx.cr->stroke();

    // Avatar (round, CMS_AUTHOR_BORDER ring)
    if (avatar_attr != nullptr) {
        const std::string path = ::ase::viewer::render::resolve_image_path(avatar_attr);
        const double ax = ctx.margin_left + PAD;
        const double ay = ctx.y + PAD;
        ctx.cr->save();
        ctx.cr->arc(ax + AVATAR_D / 2.0, ay + AVATAR_D / 2.0,
                    AVATAR_D / 2.0, 0, 2 * M_PI);
        ctx.cr->clip();
        ::ase::imgcache::render(ctx.cr, path, ax, ay, static_cast<int>(AVATAR_D));
        ctx.cr->restore();
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_AUTHOR_BORDER),
                               rgb_g(::ase::colors::CMS_AUTHOR_BORDER),
                               rgb_b(::ase::colors::CMS_AUTHOR_BORDER));
        ctx.cr->set_line_width(2);
        ctx.cr->arc(ax + AVATAR_D / 2.0, ay + AVATAR_D / 2.0,
                    AVATAR_D / 2.0, 0, 2 * M_PI);
        ctx.cr->stroke();
    }

    // Text column
    const double tx = ctx.margin_left + PAD + AVATAR_D + GAP;
    double ty = ctx.y + PAD;

    if (name_attr != nullptr) {
        std::string n_markup;
        ::ase::viewer::render::append_escaped(n_markup, name_attr);
        auto n_layout = create_body_layout(ctx.cr, text_w);
        auto fd = n_layout->get_font_description();
        fd.set_size(NAME_PX * Pango::SCALE);
        fd.set_weight(Pango::Weight::BOLD);
        n_layout->set_font_description(fd);
        n_layout->set_markup(n_markup);
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                               rgb_b(::ase::colors::TEXT_PANEL_WHITE));
        draw_layout(ctx.cr, n_layout, tx, ty);
        ty += NAME_PX + 2;
    }
    if (role_attr != nullptr) {
        std::string r_markup;
        ::ase::viewer::render::append_escaped(r_markup, role_attr);
        auto r_layout = create_body_layout(ctx.cr, text_w);
        auto fd = r_layout->get_font_description();
        fd.set_size(ROLE_PX * Pango::SCALE);
        r_layout->set_font_description(fd);
        r_layout->set_markup(r_markup);
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_AUTHOR_BORDER),
                               rgb_g(::ase::colors::CMS_AUTHOR_BORDER),
                               rgb_b(::ase::colors::CMS_AUTHOR_BORDER));
        draw_layout(ctx.cr, r_layout, tx, ty);
        ty += ROLE_PX + 2;
    }
    if (!body_md.empty()) {
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_b(::ase::colors::TEXT_PANEL_MUTED));
        draw_layout(ctx.cr, body_layout, tx, ty + 6);
    }

    return total_h + 16.0;  // my-4 outer
}

}  // namespace ase::viewer::render::dsl
