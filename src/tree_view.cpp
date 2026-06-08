/**
 * @file        tree_view.cpp
 * @brief       TreeView implementation - populate, filter, update rows
 * @description The filesystem scan is POSIX (opendir/readdir/stat) so no
 *              <filesystem> or <fstream> is pulled in. Markup escaping goes
 *              through Glib::Markup::escape_text for the display column.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/tree_view.hpp>

#include <viewer/fs_utils.hpp>
#include <viewer/read_tracker.hpp>

#include <gtkmm/cellrenderertext.h>
#include <gtkmm/treeviewcolumn.h>
#include <glibmm/markup.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <ase/containers/vector.hpp>

#include <algorithm>
#include <cctype>

namespace ase::viewer {

namespace {

struct DirEntry {
    std::string name;
    std::string path;
    bool        is_dir;
};

ase::containers::Vector<DirEntry> list_markdown_dir(const std::string& dir_path) {
    ase::containers::Vector<DirEntry> entries;
    DIR* d = ::opendir(dir_path.c_str());
    if (d == nullptr) return entries;

    struct dirent* de = nullptr;
    while ((de = ::readdir(d)) != nullptr) {
        const std::string name{de->d_name};
        if (name.empty() || name[0] == '.') continue;
        const std::string full = fs::path_join(dir_path, name);
        const bool dir_flag = fs::is_directory(full);
        if (dir_flag) {
            entries.push_back({name, full, true});
        } else if (name.size() > 3 && name.substr(name.size() - 3) == ".md") {
            entries.push_back({name, full, false});
        }
    }
    ::closedir(d);

    std::sort(entries.begin(), entries.end(),
        [](const DirEntry& a, const DirEntry& b) {
            if (a.is_dir != b.is_dir) return a.is_dir > b.is_dir;
            return a.name < b.name;
        });
    return entries;
}

}  // namespace

TreeView::TreeView(const ReadTracker& tracker)
    : m_tracker(tracker)
{
    m_scroll.add_css_class("sidebar");
    m_scroll.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

    m_tree_store = Gtk::TreeStore::create(m_columns);
    m_filter_model = Gtk::TreeModelFilter::create(m_tree_store);
    m_filter_model->set_visible_func(
        [this](const Gtk::TreeModel::const_iterator& iter) -> bool {
            return filter_row(iter);
        });

    m_tree_view.set_model(m_filter_model);
    m_tree_view.set_headers_visible(false);
    auto* col = Gtk::make_managed<Gtk::TreeViewColumn>("");
    auto* cell = Gtk::make_managed<Gtk::CellRendererText>();
    col->pack_start(*cell, true);
    col->add_attribute(cell->property_markup(), m_columns.col_display);
    m_tree_view.append_column(*col);
    m_tree_view.set_activate_on_single_click(true);

    m_tree_view.signal_row_activated().connect(
        [this](const Gtk::TreeModel::Path& filter_path, Gtk::TreeViewColumn*) {
            auto iter = m_filter_model->get_iter(filter_path);
            if (!iter) return;
            const bool dir_flag = (*iter)[m_columns.col_is_dir];
            if (dir_flag) {
                if (m_tree_view.row_expanded(filter_path)) {
                    m_tree_view.collapse_row(filter_path);
                } else {
                    m_tree_view.expand_row(filter_path, false);
                }
            } else {
                const std::string file_path =
                    static_cast<std::string>((*iter)[m_columns.col_path]);
                if (m_on_file_activated) m_on_file_activated(file_path);
            }
        });

    m_scroll.set_child(m_tree_view);
}

void TreeView::load(const std::string& root) {
    m_root_path = root;
    populate_tree(root);
}

void TreeView::refresh() {
    if (m_root_path.empty()) return;

    ase::containers::Vector<std::string> expanded_paths;
    m_filter_model->foreach_iter(
        [this, &expanded_paths](const Gtk::TreeModel::const_iterator& iter) -> bool {
            auto path = m_filter_model->get_path(iter);
            if (m_tree_view.row_expanded(path)) {
                expanded_paths.push_back(
                    static_cast<std::string>((*iter)[m_columns.col_path]));
            }
            return false;
        });

    populate_tree(m_root_path);

    m_filter_model->foreach_iter(
        [this, &expanded_paths](const Gtk::TreeModel::const_iterator& iter) -> bool {
            const std::string file_path =
                static_cast<std::string>((*iter)[m_columns.col_path]);
            for (const auto& ep : expanded_paths) {
                if (ep == file_path) {
                    auto path = m_filter_model->get_path(iter);
                    m_tree_view.expand_row(path, false);
                    break;
                }
            }
            return false;
        });
}

void TreeView::set_search(const std::string& lowered_text) {
    m_search_text = lowered_text;
    if (m_filter_model) m_filter_model->refilter();
    if (!m_search_text.empty()) expand_matching_rows();
}

void TreeView::mark_file_read(const std::string& path) {
    update_tree_display(path);
}

void TreeView::mark_all_read_display() {
    m_tree_store->foreach_iter([this](const Gtk::TreeModel::iterator& iter) -> bool {
        const bool dir_flag = (*iter)[m_columns.col_is_dir];
        if (!dir_flag) {
            const Glib::ustring name = (*iter)[m_columns.col_name];
            (*iter)[m_columns.col_is_read] = true;
            (*iter)[m_columns.col_display] = format_display(name.raw(), false, true);
        }
        return false;
    });
}

void TreeView::grab_focus() {
    m_tree_view.grab_focus();
}

void TreeView::populate_tree(const std::string& root) {
    m_tree_store->clear();
    if (!fs::is_directory(root)) return;
    populate_node(root, Gtk::TreeModel::iterator());
}

void TreeView::populate_node(const std::string& dir_path,
                             const Gtk::TreeModel::iterator& parent) {
    const auto entries = list_markdown_dir(dir_path);

    for (const auto& e : entries) {
        Gtk::TreeModel::iterator iter;
        if (parent) iter = m_tree_store->append(parent->children());
        else        iter = m_tree_store->append();

        (*iter)[m_columns.col_name]    = e.name;
        (*iter)[m_columns.col_path]    = e.path;
        (*iter)[m_columns.col_is_dir]  = e.is_dir;
        const bool is_read = e.is_dir || m_tracker.is_read(e.path);
        (*iter)[m_columns.col_is_read] = is_read;
        (*iter)[m_columns.col_display] = format_display(e.name, e.is_dir, is_read);

        if (e.is_dir) populate_node(e.path, iter);
    }
}

bool TreeView::filter_row(const Gtk::TreeModel::const_iterator& iter) const {
    if (m_search_text.empty()) return true;
    const bool dir_flag = (*iter)[m_columns.col_is_dir];
    const Glib::ustring name = (*iter)[m_columns.col_name];
    std::string lower;
    for (auto ch : name) {
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    if (lower.find(m_search_text) != std::string::npos) return true;
    if (dir_flag) {
        auto children = iter->children();
        for (auto child = children.begin(); child != children.end(); ++child) {
            if (filter_row(child)) return true;
        }
    }
    return false;
}

void TreeView::expand_matching_rows() {
    m_filter_model->foreach_iter(
        [this](const Gtk::TreeModel::const_iterator& iter) -> bool {
            const bool dir_flag = (*iter)[m_columns.col_is_dir];
            if (dir_flag) {
                auto path = m_filter_model->get_path(iter);
                m_tree_view.expand_row(path, false);
            }
            return false;
        });
}

void TreeView::update_tree_display(const std::string& file_path) {
    m_tree_store->foreach_iter(
        [this, &file_path](const Gtk::TreeModel::iterator& iter) -> bool {
            const std::string path =
                static_cast<std::string>((*iter)[m_columns.col_path]);
            if (path == file_path) {
                const Glib::ustring name = (*iter)[m_columns.col_name];
                (*iter)[m_columns.col_is_read] = true;
                (*iter)[m_columns.col_display] = format_display(name.raw(), false, true);
                return true;
            }
            return false;
        });
}

Glib::ustring TreeView::format_display(const std::string& name,
                                       bool is_dir,
                                       bool is_read) {
    if (is_dir) {
        return Glib::ustring::compose(
            "<span foreground='#5A5A5A'>%1</span>",
            Glib::Markup::escape_text(name));
    }
    if (is_read) {
        return Glib::ustring::compose(
            "<span foreground='#3A3A3A'>%1</span>",
            Glib::Markup::escape_text(name));
    }
    return Glib::ustring::compose(
        "<span foreground='#5a9cb8'>\xe2\x97\x8f </span>"
        "<span foreground='#8A9A9A'>%1</span>",
        Glib::Markup::escape_text(name));
}

}  // namespace ase::viewer
