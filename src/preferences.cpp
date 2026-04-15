/**
 * @file        preferences.cpp
 * @brief       Dashboard-style preferences window (cockpit layout)
 * @description Ported from clients/ase-client-explorer/src/settings_dialog.cpp.
 *              Drops AdwPreferencesPage entirely (centres content at ~600 px
 *              max width) in favour of dense, full-width GTK pages added
 *              directly to an AdwViewStack. Every spacing / font-size value
 *              flows from ase::theme::* and every colour from ase::colors::*
 *              through the .ase-cfg-* CSS classes in theme.cpp.
 *
 *              Pages:
 *                APPEARANCE — colour scheme, font size, code font
 *                DOCUMENTS  — default root, default mode, language, live reload
 *                RENDERING  — line numbers, math rendering, mermaid diagrams
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/preferences.hpp>

#include <ase/adp/adw/adw.hpp>

#include "design_tokens.hpp"

#include <adwaita.h>
#include <gtk/gtk.h>

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace ase::viewer {

namespace {

namespace t = ase::theme;

// Cockpit window default — same proportion math as the explorer, slightly
// smaller because the viewer has fewer settings to show.
constexpr int CFG_WIDTH  = t::space_2xl * 24 + t::space_md;   // 1168
constexpr int CFG_HEIGHT = t::space_2xl * 15 + t::space_xl;   // 752

// ── Shared context ─────────────────────────────────────────────────────
//
// Function-local static (Meyers singleton). Only one preferences window
// can be open at a time, and every callback is a free function that
// needs the ViewerSettings pointer. Using a file-scope global is blocked
// by the ECS validator; a function-local static wrapped in an accessor
// is the sanctioned alternative (matches the original preferences.cpp
// pattern before the dashboard rewrite).
struct PrefContext {
    ViewerSettings*    settings   = nullptr;
    std::string        config_dir;
    sigc::slot<void()> on_changed;
};

PrefContext& ctx() {
    static PrefContext instance;
    return instance;
}

void save_and_notify() {
    PrefContext& c = ctx();
    if (c.settings == nullptr) return;
    save_settings(c.config_dir, *c.settings);
    if (c.on_changed) c.on_changed();
}

// ── Small builders (mirror explorer settings_dialog.cpp) ──────────────

GtkWidget* make_section_strip(const char* title) {
    GtkWidget* label = gtk_label_new(title);
    gtk_widget_add_css_class(label, "ase-cfg-section-title");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_widget_set_hexpand(label, TRUE);
    return label;
}

// A flush "cell" hosting one labelled control. Stacks an uppercase label
// above the actual control — used by every page.
GtkWidget* make_cell(const char* label_text, GtkWidget* control) {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, t::space_xs);
    gtk_widget_add_css_class(box, "ase-cfg-cell");
    gtk_widget_set_hexpand(box, TRUE);
    gtk_widget_set_vexpand(box, FALSE);

    GtkWidget* label = gtk_label_new(label_text);
    gtk_widget_add_css_class(label, "ase-cfg-cell-label");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_box_append(GTK_BOX(box), label);

    GtkWidget* value_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, t::space_sm);
    gtk_widget_add_css_class(value_row, "ase-cfg-cell-value");
    gtk_widget_set_hexpand(value_row, TRUE);
    gtk_box_append(GTK_BOX(value_row), control);
    gtk_widget_set_hexpand(control, TRUE);
    gtk_widget_set_halign(control, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), value_row);

    return box;
}

GtkWidget* make_toggle(bool initial) {
    GtkWidget* sw = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(sw), initial ? TRUE : FALSE);
    gtk_widget_set_valign(sw, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(sw, GTK_ALIGN_START);
    return sw;
}

GtkWidget* make_dropdown(const std::vector<const char*>& choices, unsigned selected) {
    std::vector<const char*> with_null(choices);
    with_null.push_back(nullptr);
    GtkWidget* dd = gtk_drop_down_new_from_strings(with_null.data());
    gtk_drop_down_set_selected(GTK_DROP_DOWN(dd), selected);
    gtk_widget_set_valign(dd, GTK_ALIGN_CENTER);
    return dd;
}

GtkWidget* make_spin(int lo, int hi, int step, int initial) {
    GtkWidget* sp = gtk_spin_button_new_with_range(
        static_cast<double>(lo), static_cast<double>(hi),
        static_cast<double>(step));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sp), static_cast<double>(initial));
    gtk_widget_set_valign(sp, GTK_ALIGN_CENTER);
    return sp;
}

// A horizontal row of cells. Cells share the row evenly via hexpand.
GtkWidget* make_cell_row(std::initializer_list<GtkWidget*> cells) {
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(row, TRUE);
    for (GtkWidget* cell : cells) {
        gtk_box_append(GTK_BOX(row), cell);
    }
    return row;
}

// ── Field-specific callbacks ──────────────────────────────────────────
//
// One free function per ViewerSettings field. Each reads ctx() and
// writes its one field, then saves + notifies.

void cb_color_scheme_changed(GObject* obj, GParamSpec*, gpointer) {
    PrefContext& c = ctx();
    if (c.settings == nullptr) return;
    c.settings->color_scheme = static_cast<uint8_t>(
        gtk_drop_down_get_selected(GTK_DROP_DOWN(obj)));
    save_and_notify();
}

void cb_font_size_changed(GtkSpinButton* sb, gpointer) {
    PrefContext& c = ctx();
    if (c.settings == nullptr) return;
    c.settings->font_size = static_cast<int>(gtk_spin_button_get_value(sb));
    save_and_notify();
}

void cb_code_font_changed(GObject* obj, GParamSpec*, gpointer) {
    PrefContext& c = ctx();
    if (c.settings == nullptr) return;
    c.settings->code_font = static_cast<int>(
        gtk_drop_down_get_selected(GTK_DROP_DOWN(obj)));
    save_and_notify();
}

void cb_default_root_changed(GtkEditable* e, gpointer) {
    PrefContext& c = ctx();
    if (c.settings == nullptr) return;
    const char* text = gtk_editable_get_text(e);
    c.settings->default_root = text != nullptr ? std::string{text} : std::string{};
    save_and_notify();
}

void cb_default_mode_changed(GObject* obj, GParamSpec*, gpointer) {
    PrefContext& c = ctx();
    if (c.settings == nullptr) return;
    c.settings->default_mode = static_cast<uint8_t>(
        gtk_drop_down_get_selected(GTK_DROP_DOWN(obj)));
    save_and_notify();
}

void cb_language_changed(GObject* obj, GParamSpec*, gpointer) {
    PrefContext& c = ctx();
    if (c.settings == nullptr) return;
    c.settings->language = static_cast<uint8_t>(
        gtk_drop_down_get_selected(GTK_DROP_DOWN(obj)));
    save_and_notify();
}

void cb_live_reload_changed(GObject* obj, GParamSpec*, gpointer) {
    PrefContext& c = ctx();
    if (c.settings == nullptr) return;
    c.settings->live_reload =
        (gtk_switch_get_active(GTK_SWITCH(obj)) == TRUE);
    save_and_notify();
}

void cb_line_numbers_changed(GObject* obj, GParamSpec*, gpointer) {
    PrefContext& c = ctx();
    if (c.settings == nullptr) return;
    c.settings->line_numbers =
        (gtk_switch_get_active(GTK_SWITCH(obj)) == TRUE);
    save_and_notify();
}

void cb_math_render_changed(GObject* obj, GParamSpec*, gpointer) {
    PrefContext& c = ctx();
    if (c.settings == nullptr) return;
    c.settings->math_rendering =
        (gtk_switch_get_active(GTK_SWITCH(obj)) == TRUE);
    save_and_notify();
}

void cb_mermaid_render_changed(GObject* obj, GParamSpec*, gpointer) {
    PrefContext& c = ctx();
    if (c.settings == nullptr) return;
    c.settings->mermaid_render =
        (gtk_switch_get_active(GTK_SWITCH(obj)) == TRUE);
    save_and_notify();
}

// ── Pages ─────────────────────────────────────────────────────────────

GtkWidget* build_appearance_page() {
    PrefContext& c = ctx();

    GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(page, "ase-cfg-window");
    gtk_widget_set_hexpand(page, TRUE);
    gtk_widget_set_vexpand(page, TRUE);

    gtk_box_append(GTK_BOX(page), make_section_strip("THEME"));

    GtkWidget* scheme = make_dropdown(
        {"Dark", "Light", "System"}, c.settings->color_scheme);
    g_signal_connect(scheme, "notify::selected",
                     G_CALLBACK(cb_color_scheme_changed), nullptr);
    gtk_box_append(GTK_BOX(page), make_cell_row({
        make_cell("COLOUR SCHEME", scheme),
    }));

    gtk_box_append(GTK_BOX(page), make_section_strip("FONT"));

    GtkWidget* size = make_spin(8, 24, 1, c.settings->font_size);
    g_signal_connect(size, "value-changed",
                     G_CALLBACK(cb_font_size_changed), nullptr);

    GtkWidget* code = make_dropdown(
        {"Fira Code", "JetBrains Mono", "Source Code Pro"},
        static_cast<unsigned>(c.settings->code_font));
    g_signal_connect(code, "notify::selected",
                     G_CALLBACK(cb_code_font_changed), nullptr);

    gtk_box_append(GTK_BOX(page), make_cell_row({
        make_cell("SIZE",      size),
        make_cell("CODE FONT", code),
    }));

    GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(page), spacer);
    return page;
}

GtkWidget* build_documents_page() {
    PrefContext& c = ctx();

    GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(page, "ase-cfg-window");
    gtk_widget_set_hexpand(page, TRUE);
    gtk_widget_set_vexpand(page, TRUE);

    gtk_box_append(GTK_BOX(page), make_section_strip("ROOT DIRECTORY"));

    // Custom cell — the entry fills the row instead of halign=START so
    // the user can read the whole absolute path at once.
    GtkWidget* root_cell = gtk_box_new(GTK_ORIENTATION_VERTICAL, t::space_xs);
    gtk_widget_add_css_class(root_cell, "ase-cfg-cell");
    gtk_widget_set_hexpand(root_cell, TRUE);

    GtkWidget* root_label = gtk_label_new("DEFAULT ROOT PATH");
    gtk_widget_add_css_class(root_label, "ase-cfg-cell-label");
    gtk_label_set_xalign(GTK_LABEL(root_label), 0.0f);
    gtk_box_append(GTK_BOX(root_cell), root_label);

    GtkWidget* root_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(root_entry),
                          c.settings->default_root.c_str());
    gtk_widget_set_tooltip_text(root_entry,
        "Absolute path. Takes effect the next time a directory is loaded.");
    gtk_widget_set_hexpand(root_entry, TRUE);
    gtk_widget_set_halign(root_entry, GTK_ALIGN_FILL);
    g_signal_connect(root_entry, "changed",
                     G_CALLBACK(cb_default_root_changed), nullptr);
    gtk_box_append(GTK_BOX(root_cell), root_entry);

    gtk_box_append(GTK_BOX(page), root_cell);

    gtk_box_append(GTK_BOX(page), make_section_strip("DEFAULTS"));

    GtkWidget* mode = make_dropdown({"TECH", "DSGN"},
                                    c.settings->default_mode);
    g_signal_connect(mode, "notify::selected",
                     G_CALLBACK(cb_default_mode_changed), nullptr);

    GtkWidget* lang = make_dropdown(
        {"English", "Deutsch", "Português"},
        c.settings->language);
    g_signal_connect(lang, "notify::selected",
                     G_CALLBACK(cb_language_changed), nullptr);

    GtkWidget* live = make_toggle(c.settings->live_reload);
    g_signal_connect(live, "notify::active",
                     G_CALLBACK(cb_live_reload_changed), nullptr);

    gtk_box_append(GTK_BOX(page), make_cell_row({
        make_cell("DEFAULT MODE", mode),
        make_cell("LANGUAGE",     lang),
        make_cell("LIVE RELOAD",  live),
    }));

    GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(page), spacer);
    return page;
}

GtkWidget* build_rendering_page() {
    PrefContext& c = ctx();

    GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(page, "ase-cfg-window");
    gtk_widget_set_hexpand(page, TRUE);
    gtk_widget_set_vexpand(page, TRUE);

    gtk_box_append(GTK_BOX(page), make_section_strip("CODE BLOCKS"));

    GtkWidget* ln = make_toggle(c.settings->line_numbers);
    g_signal_connect(ln, "notify::active",
                     G_CALLBACK(cb_line_numbers_changed), nullptr);

    gtk_box_append(GTK_BOX(page), make_cell_row({
        make_cell("LINE NUMBERS", ln),
    }));

    gtk_box_append(GTK_BOX(page), make_section_strip("DIAGRAMS"));

    GtkWidget* math = make_toggle(c.settings->math_rendering);
    g_signal_connect(math, "notify::active",
                     G_CALLBACK(cb_math_render_changed), nullptr);

    GtkWidget* mmrd = make_toggle(c.settings->mermaid_render);
    g_signal_connect(mmrd, "notify::active",
                     G_CALLBACK(cb_mermaid_render_changed), nullptr);

    gtk_box_append(GTK_BOX(page), make_cell_row({
        make_cell("MATH (MICROTEX)", math),
        make_cell("MERMAID",         mmrd),
    }));

    GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(page), spacer);
    return page;
}

}  // namespace

void show_preferences_window(
    Gtk::Window& parent,
    ViewerSettings& settings,
    const std::string& config_dir,
    sigc::slot<void()> on_changed)
{
    // Reuse the function-local ctx() singleton — only one preferences
    // window exists at a time and every callback reads it directly.
    PrefContext& c = ctx();
    c.settings   = &settings;
    c.config_dir = config_dir;
    c.on_changed = std::move(on_changed);

    auto window = ase::adp::adw::Window::create();
    window.set_title("Preferences");
    window.set_default_size(CFG_WIDTH, CFG_HEIGHT);
    gtk_window_set_transient_for(GTK_WINDOW(window.native()), parent.gobj());
    window.set_modal(false);

    // ── Escape closes the preferences window ──
    {
        GtkEventController* esc = gtk_event_controller_key_new();
        g_signal_connect(esc, "key-pressed",
            G_CALLBACK(+[](GtkEventControllerKey*, guint keyval, guint,
                           GdkModifierType, gpointer w) -> gboolean {
                if (keyval == GDK_KEY_Escape) {
                    gtk_window_close(GTK_WINDOW(w));
                    return TRUE;
                }
                return FALSE;
            }), window.native());
        gtk_widget_add_controller(GTK_WIDGET(window.native()), esc);
    }

    auto toolbar = ase::adp::adw::ToolbarView::create();
    auto header  = ase::adp::adw::HeaderBar::create();
    auto stack   = ase::adp::adw::ViewStack::create();

    toolbar.add_top_bar(header);

    AdwViewStack* raw_stack = stack.native();

    GtkWidget* appearance_page = build_appearance_page();
    GtkWidget* documents_page  = build_documents_page();
    GtkWidget* rendering_page  = build_rendering_page();

    adw_view_stack_add_titled(raw_stack, appearance_page, "appearance", "APPEARANCE");
    adw_view_stack_add_titled(raw_stack, documents_page,  "documents",  "DOCUMENTS");
    adw_view_stack_add_titled(raw_stack, rendering_page,  "rendering",  "RENDERING");

    // ── Body: tab bar (left-aligned) above the stack ──
    GtkWidget* body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(body, "ase-cfg-window");
    gtk_widget_set_hexpand(body, TRUE);
    gtk_widget_set_vexpand(body, TRUE);

    AdwViewSwitcher* sw = ADW_VIEW_SWITCHER(adw_view_switcher_new());
    adw_view_switcher_set_stack(sw, raw_stack);
    adw_view_switcher_set_policy(sw, ADW_VIEW_SWITCHER_POLICY_WIDE);

    GtkWidget* tab_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(tab_bar, "ase-cfg-tab-bar");
    gtk_widget_set_halign(tab_bar, GTK_ALIGN_START);
    gtk_widget_set_hexpand(tab_bar, TRUE);
    gtk_box_append(GTK_BOX(tab_bar), GTK_WIDGET(sw));
    gtk_box_append(GTK_BOX(body), tab_bar);

    GtkWidget* stack_widget = GTK_WIDGET(raw_stack);
    gtk_widget_set_hexpand(stack_widget, TRUE);
    gtk_widget_set_vexpand(stack_widget, TRUE);
    gtk_box_append(GTK_BOX(body), stack_widget);

    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar.native_widget()), body);
    adw_window_set_content(ADW_WINDOW(window.native()), toolbar.native_widget());

    window.present();
}

}  // namespace ase::viewer
