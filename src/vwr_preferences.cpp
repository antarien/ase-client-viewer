#include "vwr_preferences.hpp"

namespace ase::viewer {

namespace {

struct PrefContext {
    ViewerSettings* settings;
    std::string config_dir;
    std::function<void()> on_changed;
};

void save_and_notify(PrefContext* ctx) {
    save_settings(ctx->config_dir, *ctx->settings);
    if (ctx->on_changed) ctx->on_changed();
}

// Use GTK4/Adw C API since adwaitamm (C++ bindings) is not widely available
GtkWidget* build_appearance_page(PrefContext* ctx) {
    auto* page = adw_preferences_page_new();
    adw_preferences_page_set_title(ADW_PREFERENCES_PAGE(page), "Appearance");
    adw_preferences_page_set_icon_name(ADW_PREFERENCES_PAGE(page), "preferences-desktop-appearance-symbolic");

    auto* group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "Theme");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(group));

    // Color scheme: Dark / Light / System
    auto* scheme_row = adw_combo_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(scheme_row), "Color Scheme");
    auto* scheme_list = gtk_string_list_new(nullptr);
    gtk_string_list_append(scheme_list, "Dark");
    gtk_string_list_append(scheme_list, "Light");
    gtk_string_list_append(scheme_list, "System");
    adw_combo_row_set_model(ADW_COMBO_ROW(scheme_row), G_LIST_MODEL(scheme_list));
    adw_combo_row_set_selected(ADW_COMBO_ROW(scheme_row), ctx->settings->color_scheme);
    g_signal_connect(scheme_row, "notify::selected", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer data) {
        auto* c = static_cast<PrefContext*>(data);
        c->settings->color_scheme = static_cast<uint8_t>(adw_combo_row_get_selected(ADW_COMBO_ROW(obj)));
        save_and_notify(c);
    }), ctx);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), GTK_WIDGET(scheme_row));

    // Font size
    auto* font_group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(font_group), "Font");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(font_group));

    auto* font_row = adw_spin_row_new_with_range(8, 24, 1);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(font_row), "Font Size");
    adw_spin_row_set_value(ADW_SPIN_ROW(font_row), ctx->settings->font_size);
    g_signal_connect(font_row, "notify::value", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer data) {
        auto* c = static_cast<PrefContext*>(data);
        c->settings->font_size = static_cast<int>(adw_spin_row_get_value(ADW_SPIN_ROW(obj)));
        save_and_notify(c);
    }), ctx);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(font_group), GTK_WIDGET(font_row));

    // Code font family
    auto* code_font_row = adw_combo_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(code_font_row), "Code Font");
    auto* font_list = gtk_string_list_new(nullptr);
    gtk_string_list_append(font_list, "Fira Code");
    gtk_string_list_append(font_list, "JetBrains Mono");
    gtk_string_list_append(font_list, "Source Code Pro");
    adw_combo_row_set_model(ADW_COMBO_ROW(code_font_row), G_LIST_MODEL(font_list));
    adw_combo_row_set_selected(ADW_COMBO_ROW(code_font_row), ctx->settings->code_font);
    g_signal_connect(code_font_row, "notify::selected", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer data) {
        auto* c = static_cast<PrefContext*>(data);
        c->settings->code_font = static_cast<int>(adw_combo_row_get_selected(ADW_COMBO_ROW(obj)));
        save_and_notify(c);
    }), ctx);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(font_group), GTK_WIDGET(code_font_row));

    return GTK_WIDGET(page);
}

GtkWidget* build_documents_page(PrefContext* ctx) {
    auto* page = adw_preferences_page_new();
    adw_preferences_page_set_title(ADW_PREFERENCES_PAGE(page), "Documents");
    adw_preferences_page_set_icon_name(ADW_PREFERENCES_PAGE(page), "folder-documents-symbolic");

    auto* group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "Defaults");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(group));

    // Default mode
    auto* mode_row = adw_combo_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(mode_row), "Default Mode");
    auto* mode_list = gtk_string_list_new(nullptr);
    gtk_string_list_append(mode_list, "TECH");
    gtk_string_list_append(mode_list, "DSGN");
    adw_combo_row_set_model(ADW_COMBO_ROW(mode_row), G_LIST_MODEL(mode_list));
    adw_combo_row_set_selected(ADW_COMBO_ROW(mode_row), ctx->settings->default_mode);
    g_signal_connect(mode_row, "notify::selected", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer data) {
        auto* c = static_cast<PrefContext*>(data);
        c->settings->default_mode = static_cast<uint8_t>(adw_combo_row_get_selected(ADW_COMBO_ROW(obj)));
        save_and_notify(c);
    }), ctx);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), GTK_WIDGET(mode_row));

    // Language
    auto* lang_row = adw_combo_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(lang_row), "Language");
    auto* lang_list = gtk_string_list_new(nullptr);
    gtk_string_list_append(lang_list, "English");
    gtk_string_list_append(lang_list, "Deutsch");
    gtk_string_list_append(lang_list, "Portugues");
    adw_combo_row_set_model(ADW_COMBO_ROW(lang_row), G_LIST_MODEL(lang_list));
    adw_combo_row_set_selected(ADW_COMBO_ROW(lang_row), ctx->settings->language);
    g_signal_connect(lang_row, "notify::selected", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer data) {
        auto* c = static_cast<PrefContext*>(data);
        c->settings->language = static_cast<uint8_t>(adw_combo_row_get_selected(ADW_COMBO_ROW(obj)));
        save_and_notify(c);
    }), ctx);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), GTK_WIDGET(lang_row));

    // Live reload
    auto* reload_row = adw_switch_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(reload_row), "Live Reload");
    adw_switch_row_set_active(ADW_SWITCH_ROW(reload_row), ctx->settings->live_reload);
    g_signal_connect(reload_row, "notify::active", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer data) {
        auto* c = static_cast<PrefContext*>(data);
        c->settings->live_reload = adw_switch_row_get_active(ADW_SWITCH_ROW(obj));
        save_and_notify(c);
    }), ctx);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), GTK_WIDGET(reload_row));

    return GTK_WIDGET(page);
}

GtkWidget* build_viewer_page(PrefContext* ctx) {
    auto* page = adw_preferences_page_new();
    adw_preferences_page_set_title(ADW_PREFERENCES_PAGE(page), "Viewer");
    adw_preferences_page_set_icon_name(ADW_PREFERENCES_PAGE(page), "applications-graphics-symbolic");

    auto* group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "Rendering");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(group));

    // Line numbers
    auto* ln_row = adw_switch_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ln_row), "Line Numbers in Code");
    adw_switch_row_set_active(ADW_SWITCH_ROW(ln_row), ctx->settings->line_numbers);
    g_signal_connect(ln_row, "notify::active", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer data) {
        auto* c = static_cast<PrefContext*>(data);
        c->settings->line_numbers = adw_switch_row_get_active(ADW_SWITCH_ROW(obj));
        save_and_notify(c);
    }), ctx);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), GTK_WIDGET(ln_row));

    // Math rendering
    auto* math_row = adw_switch_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(math_row), "Math Rendering");
    adw_switch_row_set_active(ADW_SWITCH_ROW(math_row), ctx->settings->math_rendering);
    g_signal_connect(math_row, "notify::active", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer data) {
        auto* c = static_cast<PrefContext*>(data);
        c->settings->math_rendering = adw_switch_row_get_active(ADW_SWITCH_ROW(obj));
        save_and_notify(c);
    }), ctx);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), GTK_WIDGET(math_row));

    // Mermaid diagrams
    auto* mmrd_row = adw_switch_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(mmrd_row), "Mermaid Diagrams");
    adw_switch_row_set_active(ADW_SWITCH_ROW(mmrd_row), ctx->settings->mermaid_render);
    g_signal_connect(mmrd_row, "notify::active", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer data) {
        auto* c = static_cast<PrefContext*>(data);
        c->settings->mermaid_render = adw_switch_row_get_active(ADW_SWITCH_ROW(obj));
        save_and_notify(c);
    }), ctx);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), GTK_WIDGET(mmrd_row));

    return GTK_WIDGET(page);
}

}  // anonymous namespace

void show_preferences_window(
    Gtk::Window& parent,
    ViewerSettings& settings,
    const std::string& config_dir,
    const std::function<void()>& on_changed)
{
    // Static context — lives as long as the prefs window
    static PrefContext ctx;
    ctx.settings = &settings;
    ctx.config_dir = config_dir;
    ctx.on_changed = on_changed;

    auto* win = adw_preferences_window_new();
    adw_preferences_window_set_search_enabled(ADW_PREFERENCES_WINDOW(win), false);

    adw_preferences_window_add(ADW_PREFERENCES_WINDOW(win),
        ADW_PREFERENCES_PAGE(build_appearance_page(&ctx)));
    adw_preferences_window_add(ADW_PREFERENCES_WINDOW(win),
        ADW_PREFERENCES_PAGE(build_documents_page(&ctx)));
    adw_preferences_window_add(ADW_PREFERENCES_WINDOW(win),
        ADW_PREFERENCES_PAGE(build_viewer_page(&ctx)));

    gtk_window_set_transient_for(GTK_WINDOW(win), parent.gobj());
    gtk_window_set_modal(GTK_WINDOW(win), true);
    gtk_window_present(GTK_WINDOW(win));
}

}  // namespace ase::viewer
