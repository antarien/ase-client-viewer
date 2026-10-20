/**
 * @file        preferences.cpp
 * @brief       Dashboard-style preferences window (cockpit layout)
 * @description Ported from clients/ase-client-explorer/src/settings_dialog.cpp.
 *              Drops AdwPreferencesPage entirely, which centres its content at
 *              roughly 600 px maximum width, in favour of dense full-width GTK
 *              pages added directly to an AdwViewStack. Every spacing and font
 *              size flows from ase::theme::* and every colour from
 *              ase::colors::* through the .ase-cfg-* CSS classes in theme.cpp.
 *
 *              Pages:
 *                APPEARANCE — colour scheme, font size, code font
 *                DOCUMENTS  — default root, default mode, language, live reload
 *                RENDERING  — line numbers, math rendering, mermaid diagrams
 *
 *              Each control writes its one ViewerSettings field in its own
 *              callback and saves at once, so the window needs no apply button
 *              and holds no settings of its own.
 *
 *              The cells, strips and rows come from ase::adp::adw::cockpit
 *              (usecase:101134 - lay out settings cells) and the root folder is
 *              chosen through ase::adp::adw::folder_picker (usecase:101129 -
 *              choose a folder): the explorer lays out and picks with the same
 *              two.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/ui/screen
 * @usecase     usecase:100008 - render the knowledge stock
 * @created     2026-04-13
 * @modified    2026-10-19
 * @version     00.00.36.00036 [core]
 */

#include <viewer/preferences.hpp>

#include <ase/adp/adw/adw.hpp>
#include <ase/adp/adw/cockpit.hpp>
#include <ase/adp/adw/folder_picker.hpp>

#include "design_tokens.hpp"

#include <adwaita.h>
#include <gtk/gtk.h>

#include <string>
#include <utility>

namespace ase::viewer {

namespace {

namespace t = ase::theme;

using ase::adp::adw::cockpit::make_cell;
using ase::adp::adw::cockpit::make_cell_row;
using ase::adp::adw::cockpit::make_dropdown;
using ase::adp::adw::cockpit::make_section_strip;
using ase::adp::adw::cockpit::make_toggle;

// Cockpit window default — same proportion math as the explorer, slightly
// smaller because the viewer has fewer settings to show.
constexpr int CFG_WIDTH  = t::space_2xl * 24 + t::space_md;   // 1168
constexpr int CFG_HEIGHT = t::space_2xl * 15 + t::space_xl;   // 752

// SHARED CONTEXT
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

// A spin button over [lo, hi] in `step`, set to `initial` - the one builder only the
// viewer's pages use; the shared cells stand in ase::adp::adw::cockpit.
GtkWidget* make_spin(int lo, int hi, int step, int initial) {
    GtkWidget* sp = gtk_spin_button_new_with_range(
        static_cast<double>(lo), static_cast<double>(hi),
        static_cast<double>(step));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sp), static_cast<double>(initial));
    gtk_widget_set_valign(sp, GTK_ALIGN_CENTER);
    return sp;
}

// FIELD-SPECIFIC CALLBACKS
//
// One free function per ViewerSettings field. Each reads ctx() and
// writes its one field, then saves and notifies.

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

// Fires once the user accepts a directory in the picker. `user_data` is the root entry
// the browse button carries; writing into it triggers cb_default_root_changed, which in
// turn updates ViewerSettings::default_root.
void cb_root_chosen(const std::string& path, void* user_data) {
    GtkWidget* entry = static_cast<GtkWidget*>(user_data);
    if (entry == nullptr) return;
    gtk_editable_set_text(GTK_EDITABLE(entry), path.c_str());
}

void cb_browse_root_clicked(GtkButton* btn, gpointer user_data) {
    GtkWidget*  entry  = GTK_WIDGET(user_data);
    GtkRoot*    root   = gtk_widget_get_root(GTK_WIDGET(btn));
    GtkWindow*  parent = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;

    const char* current = gtk_editable_get_text(GTK_EDITABLE(entry));
    std::string start = current != nullptr ? std::string{current} : std::string{};

    // The entry outlives the picker: the picker is modal over the preferences window
    // that owns the entry.
    ase::adp::adw::folder_picker::show(parent, start, cb_root_chosen, entry);
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

// PAGES

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

    GtkWidget* path_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, t::space_xs);
    gtk_widget_set_hexpand(path_row, TRUE);
    gtk_box_append(GTK_BOX(path_row), root_entry);

    GtkWidget* browse_btn = gtk_button_new_with_label("Browse…");
    gtk_widget_set_tooltip_text(browse_btn, "Pick a directory via folder picker");
    g_signal_connect(browse_btn, "clicked",
                     G_CALLBACK(cb_browse_root_clicked), root_entry);
    gtk_box_append(GTK_BOX(path_row), browse_btn);

    gtk_box_append(GTK_BOX(root_cell), path_row);

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

    // ESCAPE CLOSES THE PREFERENCES WINDOW
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

    // BODY: LEFT-ALIGNED TAB BAR ABOVE THE STACK
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
