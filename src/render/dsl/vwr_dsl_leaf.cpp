/**
 * DSL leaf directive renderers.
 *
 * Single-line leaf directives that produce small inline-style block
 * elements: ::badge, ::divider, ::spacer, ::progress, ::kbd.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_dsl_leaf.hpp"

namespace ase::viewer::render::dsl {

// Web 1:1 leaf primitives.
//   badge    px-2 py-0.5 text-xs (8/2/12)
//   divider  my-6 border-top BORDER_A30 (24)
//   spacer   my-8 (32)
//   progress h-2 (8) bar, label text-xs (12)
//   kbd      px-2 py-0.5 text-xs (8/2/12)

// Web 1:1 (cmsBadgeInline.tsx):
//   font-mono text-2xs (10) normal, uppercase, letter-spacing 0.05em,
//   padding 2 px vertical / 8 px horizontal, sharp corners (radius 0),
//   bg = CMS_BADGE_<color>_BG tinted, fg = PANEL_<color>
struct BadgePalette { uint32_t fg, bg; };

static BadgePalette badge_palette_for(const char* color) {
    if (color == nullptr) return {::ase::colors::CMS_BADGE_ACCENT, ::ase::colors::CMS_BADGE_CYAN_BG};
    if (::ase::viewer::render::streq(color, "cyan"))   return {::ase::colors::CMS_BADGE_ACCENT, ::ase::colors::CMS_BADGE_CYAN_BG};
    if (::ase::viewer::render::streq(color, "green"))  return {::ase::colors::PANEL_GREEN,  ::ase::colors::CMS_BADGE_GREEN_BG};
    if (::ase::viewer::render::streq(color, "orange")) return {::ase::colors::PANEL_ORANGE, ::ase::colors::CMS_BADGE_ORANGE_BG};
    if (::ase::viewer::render::streq(color, "purple")) return {::ase::colors::PANEL_PURPLE, ::ase::colors::CMS_BADGE_PURPLE_BG};
    if (::ase::viewer::render::streq(color, "red"))    return {::ase::colors::PANEL_RED,    ::ase::colors::CMS_BADGE_RED_BG};
    if (::ase::viewer::render::streq(color, "yellow")) return {::ase::colors::PANEL_YELLOW, ::ase::colors::CMS_BADGE_YELLOW_BG};
    return {::ase::colors::CMS_BADGE_ACCENT, ::ase::colors::CMS_BADGE_CYAN_BG};
}

double render_badge(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    (void)cw;
    constexpr double PAD_X   = 8.0;    // padding: 2px 8px
    constexpr double PAD_Y   = 2.0;
    constexpr int    TXT_PX  = 10;     // text-2xs
    constexpr double MR      = 6.0;    // mr-1.5

    const char* text_attr  = get_attr(node, "text");
    const char* color_attr = get_attr(node, "color");
    std::string text = (text_attr != nullptr) ? std::string{text_attr}
                                               : build_inline_content(node);
    if (text.empty()) text = "badge";

    // Uppercase — web uses CSS text-transform.
    for (char& c : text) {
        if (c >= 'a' && c <= 'z') c -= ('a' - 'A');
    }

    const BadgePalette pal = badge_palette_for(color_attr);

    // Tracking / letter-spacing 0.05em via Pango letter_spacing (in units
    // of 1/PANGO_SCALE px). 0.05 × TXT_PX × PANGO_SCALE ≈ 0.5 × SCALE.
    std::string markup = "<span letter_spacing=\"";
    markup.append(std::to_string(static_cast<int>(0.05 * TXT_PX * Pango::SCALE)));
    markup.append("\">");
    ::ase::viewer::render::append_escaped(markup, text.c_str(),
                                          static_cast<uint32_t>(text.size()));
    markup.append("</span>");

    auto layout = create_body_layout(ctx.cr, 400);
    auto fd = layout->get_font_description();
    fd.set_size(TXT_PX * Pango::SCALE);
    layout->set_font_description(fd);
    layout->set_markup(markup);
    int tw = 0, th = 0;
    layout->get_pixel_size(tw, th);

    const double w = tw + 2 * PAD_X;
    const double h = th + 2 * PAD_Y;

    // Tinted background, sharp corners (radius 0).
    ctx.cr->set_source_rgb(rgb_r(pal.bg), rgb_g(pal.bg), rgb_b(pal.bg));
    ctx.cr->rectangle(ctx.margin_left, ctx.y, w, h);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(rgb_r(pal.fg), rgb_g(pal.fg), rgb_b(pal.fg));
    draw_layout(ctx.cr, layout, ctx.margin_left + PAD_X, ctx.y + PAD_Y);

    return h + MR;
}

double render_divider(RenderContext& ctx, int cw) {
    // Web <hr> my-6 = 24 vertical, border-top BORDER_A30.
    // Dispatcher advances ctx.y by the return value, so we must NOT
    // mutate ctx.y here — draw at the midpoint of the reserved space
    // and return the full 24 px the divider occupies.
    constexpr double MY = 24.0;
    const double line_y = ctx.y + MY / 2.0;
    ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A30),
                            rgb_g(::ase::colors::BORDER_A30),
                            rgb_b(::ase::colors::BORDER_A30),
                            rgb_a(::ase::colors::BORDER_A30));
    ctx.cr->set_line_width(1);
    ctx.cr->move_to(ctx.margin_left, line_y);
    ctx.cr->line_to(ctx.margin_left + cw, line_y);
    ctx.cr->stroke();
    return MY;
}

double render_spacer(RenderContext& ctx, const ase::markdown::Node* node) {
    // Web `::spacer{h=N}` uses N rem (16 px per rem). Default rem is 2.
    // The cmsSpacerLeaf.tsx accepts string or number; the parser stores
    // every attribute as const char*, so parse once.
    (void)ctx;
    int rem_units = get_attr_int(node, "h", 2);
    if (rem_units < 0) rem_units = 0;
    return static_cast<double>(rem_units) * 16.0;
}

double render_progress(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    constexpr double BAR_H  = 8.0;   // h-2
    constexpr int    TXT_PX = 12;    // text-xs
    constexpr double GAP    = 4.0;

    int value = get_attr_int(node, "value", 65);
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    const double frac = value / 100.0;

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::SURFACE_1_ALT),
                           rgb_g(::ase::colors::SURFACE_1_ALT),
                           rgb_b(::ase::colors::SURFACE_1_ALT));
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y + GAP, cw, BAR_H, 4);
    ctx.cr->fill();

    ctx.cr->set_source_rgb(GREEN_R, GREEN_G, GREEN_B);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y + GAP, cw * frac, BAR_H, 4);
    ctx.cr->fill();

    const char* label_attr = get_attr(node, "label");
    if (label_attr != nullptr) {
        std::string label_markup;
        ::ase::viewer::render::append_escaped(label_markup, label_attr);
        label_markup.append(" <span foreground=\"");
        label_markup.append(color_hex(::ase::colors::DOCS_H1));
        label_markup.append("\">");
        label_markup.append(std::to_string(value));
        label_markup.append("%</span>");

        auto layout = create_body_layout(ctx.cr, cw);
        auto fd = layout->get_font_description();
        fd.set_size(TXT_PX * Pango::SCALE);
        layout->set_font_description(fd);
        layout->set_markup(label_markup);
        ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
        draw_layout(ctx.cr, layout, ctx.margin_left, ctx.y + GAP + BAR_H + GAP);
        return BAR_H + 32;
    }
    return BAR_H + 12;
}

double render_download(RenderContext& ctx, const ase::markdown::Node* node, int cw) {
    // Web 1:1 (cmsDownloadLeaf.tsx):
    //   inline-flex button: bg CMS_DOWNLOAD_BG, border 1 BORDER_A20,
    //                       radius 4, px-4 py-3 (16/12), gap-3 (12),
    //                       text-sm (14)
    //   icon: nf-fa-download (or attr), color TEXT_PANEL_MUTED
    //   label: bold TEXT_PANEL_WHITE
    //   size:  text-xs (12) TEXT_PANEL_MUTED below label
    constexpr double PAD_X    = 16.0;
    constexpr double PAD_Y    = 12.0;
    constexpr double GAP      = 12.0;
    constexpr int    LBL_PX   = 14;
    constexpr int    SIZE_PX  = 12;
    constexpr double RADIUS   = 4.0;
    constexpr double ICON_W   = 20.0;
    (void)cw;

    const char* file_attr  = get_attr(node, "file");
    const char* label_attr = get_attr(node, "label");
    const char* size_attr  = get_attr(node, "size");

    std::string display = (label_attr != nullptr) ? std::string{label_attr}
                        : (file_attr  != nullptr) ? std::string{file_attr}
                                                  : std::string{"Download"};

    auto lbl_layout = create_body_layout(ctx.cr, 600);
    auto fd = lbl_layout->get_font_description();
    fd.set_size(LBL_PX * Pango::SCALE);
    fd.set_weight(Pango::Weight::BOLD);
    lbl_layout->set_font_description(fd);
    {
        std::string m;
        ::ase::viewer::render::append_escaped(m, display.c_str());
        lbl_layout->set_markup(m);
    }
    int lw = 0, lh = 0;
    lbl_layout->get_pixel_size(lw, lh);

    Glib::RefPtr<Pango::Layout> size_layout;
    int sw_ = 0, sh_ = 0;
    if (size_attr != nullptr) {
        size_layout = create_body_layout(ctx.cr, 600);
        auto sfd = size_layout->get_font_description();
        sfd.set_size(SIZE_PX * Pango::SCALE);
        size_layout->set_font_description(sfd);
        std::string m;
        ::ase::viewer::render::append_escaped(m, size_attr);
        size_layout->set_markup(m);
        size_layout->get_pixel_size(sw_, sh_);
    }

    const int    text_w = (lw > sw_) ? lw : sw_;
    const double text_h = lh + (size_attr ? (4 + sh_) : 0);
    const double w = PAD_X + ICON_W + GAP + text_w + PAD_X;
    const double h = text_h + 2 * PAD_Y;

    // Background
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::CMS_DOWNLOAD_BG),
                           rgb_g(::ase::colors::CMS_DOWNLOAD_BG),
                           rgb_b(::ase::colors::CMS_DOWNLOAD_BG));
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, w, h, RADIUS);
    ctx.cr->fill();

    // Border
    ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                            rgb_g(::ase::colors::BORDER_A20),
                            rgb_b(::ase::colors::BORDER_A20),
                            rgb_a(::ase::colors::BORDER_A20));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, w, h, RADIUS);
    ctx.cr->stroke();

    // Download icon (NerdFont nf-fa-download = U+F019 = \xef\x80\x99)
    std::string icon_markup = "<span font_family=\"Symbols Nerd Font Mono\" foreground=\"";
    icon_markup.append(color_hex(::ase::colors::TEXT_PANEL_MUTED));
    icon_markup.append("\">\xef\x80\x99</span>");
    auto icon_layout = create_body_layout(ctx.cr, static_cast<int>(ICON_W));
    auto ifd = icon_layout->get_font_description();
    ifd.set_size(LBL_PX * Pango::SCALE);
    icon_layout->set_font_description(ifd);
    icon_layout->set_markup(icon_markup);
    draw_layout(ctx.cr, icon_layout,
                ctx.margin_left + PAD_X, ctx.y + (h - LBL_PX) / 2.0);

    // Label
    const double text_x = ctx.margin_left + PAD_X + ICON_W + GAP;
    const double text_y = ctx.y + (h - text_h) / 2.0;
    ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                           rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                           rgb_b(::ase::colors::TEXT_PANEL_WHITE));
    draw_layout(ctx.cr, lbl_layout, text_x, text_y);

    // Size
    if (size_layout) {
        ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_g(::ase::colors::TEXT_PANEL_MUTED),
                               rgb_b(::ase::colors::TEXT_PANEL_MUTED));
        draw_layout(ctx.cr, size_layout, text_x, text_y + lh + 4);
    }

    return h + 16.0;  // my-4 outer
}

double render_kbd(RenderContext& ctx, const ase::markdown::Node* node) {
    constexpr double PAD_X  = 8.0;   // px-2
    constexpr double PAD_Y  = 2.0;   // py-0.5
    constexpr int    TXT_PX = 12;    // text-xs

    const char* keys_attr = get_attr(node, "keys");
    std::string text = (keys_attr != nullptr) ? std::string{keys_attr}
                                              : build_inline_content(node);
    if (text.empty()) text = "Ctrl+K";

    auto layout = create_body_layout(ctx.cr, 200);
    auto fd = layout->get_font_description();
    fd.set_family("Fira Code");
    fd.set_size(TXT_PX * Pango::SCALE);
    layout->set_font_description(fd);
    layout->set_markup(text);
    int tw = 0, th = 0;
    layout->get_pixel_size(tw, th);

    const double w = tw + 2 * PAD_X;
    const double h = th + 2 * PAD_Y;

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::SURFACE_2),
                           rgb_g(::ase::colors::SURFACE_2),
                           rgb_b(::ase::colors::SURFACE_2));
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, w, h, 3);
    ctx.cr->fill();
    ctx.cr->set_source_rgba(rgb_r(::ase::colors::BORDER_A20),
                            rgb_g(::ase::colors::BORDER_A20),
                            rgb_b(::ase::colors::BORDER_A20),
                            rgb_a(::ase::colors::BORDER_A20));
    ctx.cr->set_line_width(1);
    rounded_rect(ctx.cr, ctx.margin_left, ctx.y, w, h, 3);
    ctx.cr->stroke();

    ctx.cr->set_source_rgb(rgb_r(::ase::colors::TEXT_PANEL_WHITE),
                           rgb_g(::ase::colors::TEXT_PANEL_WHITE),
                           rgb_b(::ase::colors::TEXT_PANEL_WHITE));
    draw_layout(ctx.cr, layout, ctx.margin_left + PAD_X, ctx.y + PAD_Y);

    return h + 4;
}

}  // namespace ase::viewer::render::dsl
