/**
 * @file        canvas.cpp
 * @brief       Canvas implementation - Cairo render + click dispatch + dump
 * @description on_draw walks the parsed AST through render_document and
 *              stretches the DrawingArea to match the resulting content
 *              height so the scrollwheel lands on the correct region.
 *              handle_click walks the hit region list produced during
 *              the most recent render and dispatches tab / accordion
 *              state changes.
 *
 *              dump_text walks the same AST and produces a bracketed
 *              plain text representation for clipboard export.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/canvas.hpp>

#include <ase/markdown/ast.hpp>
#include <ase/markdown/types.hpp>

#include <gtkmm/gestureclick.h>
#include <pangomm/layout.h>
#include <pangomm/fontdescription.h>

#include <gdk/gdk.h>

#include <string>

namespace ase::viewer {

namespace {

using namespace ase::markdown;

// ── AST text dump helpers (clipboard export path) ──────────────────

void append_indent(std::string& out, int depth) {
    for (int i = 0; i < depth; ++i) out += "  ";
}

void append_attrs(std::string& out, const Node* node) {
    if (node->attr_count == 0) return;
    for (uint16_t i = 0; i < node->attr_count; ++i) {
        out += ' ';
        if (node->attrs[i].key != nullptr) out += node->attrs[i].key;
        if (node->attrs[i].value != nullptr &&
            node->attrs[i].value != node->attrs[i].key) {
            out += '=';
            out += node->attrs[i].value;
        }
    }
}

void dump(const Node* node, std::string& out, int depth);

void dump_children(const Node* node, std::string& out, int depth) {
    for (const Node* c = node->first_child; c != nullptr; c = c->next_sibling) {
        dump(c, out, depth);
    }
}

void dump_heading(const Node* node, std::string& out, int depth) {
    append_indent(out, depth);
    out += "# ";
    dump_children(node, out, 0);
    out += '\n';
}

void dump_paragraph(const Node* node, std::string& out, int depth) {
    append_indent(out, depth);
    dump_children(node, out, 0);
    out += "\n\n";
}

void dump_text_node(const Node* node, std::string& out) {
    if (node->text != nullptr && node->text_len > 0) {
        out.append(node->text, node->text_len);
    }
}

void dump_inline_code(const Node* node, std::string& out) {
    out += '`';
    dump_text_node(node, out);
    out += '`';
}

void dump_code_block(const Node* node, std::string& out, int depth) {
    append_indent(out, depth);
    out += "```";
    if (node->language != nullptr) out += node->language;
    out += '\n';
    dump_text_node(node, out);
    out += "```\n\n";
}

void dump_blockquote(const Node* node, std::string& out, int depth) {
    append_indent(out, depth);
    out += "> ";
    dump_children(node, out, depth);
    out += '\n';
}

void dump_list(const Node* node, std::string& out, int depth) {
    dump_children(node, out, depth);
    out += '\n';
}

void dump_list_item(const Node* node, std::string& out, int depth) {
    append_indent(out, depth);
    out += "- ";
    dump_children(node, out, depth + 1);
}

void dump_thematic_break(std::string& out) {
    out += "\n---\n\n";
}

void dump_block_directive(const Node* node, std::string& out, int depth) {
    append_indent(out, depth);
    out += "[BLOCK:";
    out += (node->directive_name != nullptr ? node->directive_name : "?");
    append_attrs(out, node);
    out += "]\n";
    dump_children(node, out, depth + 1);
    append_indent(out, depth);
    out += "[/";
    out += (node->directive_name != nullptr ? node->directive_name : "?");
    out += "]\n\n";
}

void dump_leaf_directive(const Node* node, std::string& out, int depth) {
    append_indent(out, depth);
    out += "[LEAF:";
    out += (node->directive_name != nullptr ? node->directive_name : "?");
    append_attrs(out, node);
    out += "] ";
    dump_children(node, out, 0);
    out += '\n';
}

void dump(const Node* node, std::string& out, int depth) {
    if (node == nullptr) return;

    const uint8_t t = node->type;

    if (t == NODE_HEADING)         { dump_heading(node, out, depth);         return; }
    if (t == NODE_PARAGRAPH)       { dump_paragraph(node, out, depth);       return; }
    if (t == NODE_TEXT)            { dump_text_node(node, out);              return; }
    if (t == NODE_INLINE_CODE)     { dump_inline_code(node, out);            return; }
    if (t == NODE_CODE_BLOCK)      { dump_code_block(node, out, depth);      return; }
    if (t == NODE_BLOCKQUOTE)      { dump_blockquote(node, out, depth);      return; }
    if (t == NODE_LIST)            { dump_list(node, out, depth);            return; }
    if (t == NODE_LIST_ITEM)       { dump_list_item(node, out, depth);       return; }
    if (t == NODE_THEMATIC_BREAK)  { dump_thematic_break(out);               return; }
    if (t == NODE_BLOCK_DIRECTIVE) { dump_block_directive(node, out, depth); return; }
    if (t == NODE_LEAF_DIRECTIVE)  { dump_leaf_directive(node, out, depth);  return; }

    dump_children(node, out, depth);
}

}  // namespace

Canvas::Canvas() {
    m_scroll.add_css_class("content-area");
    m_scroll.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

    m_drawing_area.set_draw_func(sigc::mem_fun(*this, &Canvas::on_draw));
    m_drawing_area.set_hexpand(true);
    m_drawing_area.set_vexpand(true);

    auto click = Gtk::GestureClick::create();
    click->set_button(GDK_BUTTON_PRIMARY);
    click->signal_pressed().connect(
        [this](int /*n_press*/, double cx, double cy) {
            handle_click(cx, cy);
        });
    m_drawing_area.add_controller(click);

    m_scroll.set_child(m_drawing_area);
}

Canvas::~Canvas() {
    free_current_document();
}

void Canvas::load_document(const std::string& content, uint8_t mode) {
    free_current_document();

    m_content = content;
    m_mode    = mode;

    ase::markdown::ParseOptions opts{};
    opts.mode              = m_mode;
    opts.parse_frontmatter = 1;
    m_doc = ase::markdown::parse(m_content.c_str(),
                                 static_cast<uint32_t>(m_content.size()),
                                 opts);
    m_has_doc = true;

    m_interactive_state = render::InteractiveState{};
    m_drawing_area.queue_draw();
}

void Canvas::clear_document() {
    free_current_document();
    m_content.clear();
    m_interactive_state = render::InteractiveState{};
    m_drawing_area.queue_draw();
}

void Canvas::set_mode(uint8_t mode) {
    if (!m_has_doc) {
        m_mode = mode;
        return;
    }
    free_current_document();
    m_mode = mode;

    ase::markdown::ParseOptions opts{};
    opts.mode              = m_mode;
    opts.parse_frontmatter = 1;
    m_doc = ase::markdown::parse(m_content.c_str(),
                                 static_cast<uint32_t>(m_content.size()),
                                 opts);
    m_has_doc = true;
    m_interactive_state = render::InteractiveState{};
    m_drawing_area.queue_draw();
}

void Canvas::queue_draw() {
    m_drawing_area.queue_draw();
}

std::string Canvas::dump_text() const {
    if (!m_has_doc || m_doc.root == nullptr) return std::string{};

    std::string out;
    out.reserve(m_content.size() * 2);
    out += "=== MODE: ";
    out += (m_mode == 0 ? "TECH" : "DSGN");
    out += " ===\n\n";
    for (const Node* c = m_doc.root->first_child; c != nullptr; c = c->next_sibling) {
        dump(c, out, 0);
    }
    return out;
}

void Canvas::free_current_document() {
    if (m_has_doc) {
        ase::markdown::free_document(m_doc);
        m_has_doc = false;
    }
}

void Canvas::on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height) {
    (void)height;

    if (!m_has_doc) {
        auto layout = Pango::Layout::create(cr);
        Pango::FontDescription fd;
        fd.set_family("Fira Code");
        fd.set_weight(Pango::Weight::BOLD);
        fd.set_absolute_size(14 * Pango::SCALE);
        layout->set_font_description(fd);
        layout->set_text("ASE TECH & DESIGN Viewer");
        int lw = 0, lh = 0;
        layout->get_pixel_size(lw, lh);
        cr->set_source_rgb(0.35, 0.61, 0.72);
        cr->move_to((width - lw) / 2.0, 200);
        layout->show_in_cairo_context(cr);

        auto sub = Pango::Layout::create(cr);
        Pango::FontDescription sfd;
        sfd.set_family("Fira Code");
        sfd.set_absolute_size(11 * Pango::SCALE);
        sub->set_font_description(sfd);
        sub->set_text("Select a document from the sidebar");
        int sw = 0, sh = 0;
        sub->get_pixel_size(sw, sh);
        cr->set_source_rgb(0.35, 0.35, 0.35);
        cr->move_to((width - sw) / 2.0, 230);
        sub->show_in_cairo_context(cr);
        return;
    }

    render::RenderContext ctx;
    ctx.cr         = cr;
    ctx.width      = width;
    ctx.viewport_h = height;
    ctx.state      = &m_interactive_state;
    ctx.regions    = &m_hit_regions;
    const double total_height = render::render_document(ctx, m_doc);
    m_drawing_area.set_content_height(static_cast<int>(total_height));
}

void Canvas::handle_click(double cx, double cy) {
    for (uint32_t i = 0; i < m_hit_regions.count; ++i) {
        const auto& r = m_hit_regions.items[i];
        if (cx < r.x || cx > r.x + r.w || cy < r.y || cy > r.y + r.h) continue;

        if (r.kind == render::HIT_KIND_TAB_HEADER) {
            if (r.parent_idx < render::MAX_TABS_DIRECTIVES) {
                m_interactive_state.active_tab[r.parent_idx] = r.child_idx;
                m_drawing_area.queue_draw();
            }
            return;
        }
        if (r.kind == render::HIT_KIND_PANEL_HEADER) {
            if (r.parent_idx < render::MAX_ACCORDIONS &&
                r.child_idx  < render::MAX_PANELS_PER_ACRN) {
                auto& slot = m_interactive_state.panel_open[r.parent_idx][r.child_idx];
                slot = (slot == 0) ? 1 : 0;
                m_drawing_area.queue_draw();
            }
            return;
        }
    }
}

}  // namespace ase::viewer
