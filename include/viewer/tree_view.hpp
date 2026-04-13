#pragma once

/**
 * @file        tree_view.hpp
 * @brief       Left sidebar file tree with search filter and read-marking
 * @description Owns a Gtk::TreeView over a Gtk::TreeStore, populated from the
 *              filesystem via the POSIX helpers in fs_utils. A Gtk::TreeModelFilter
 *              wraps the store for live search. Read-state for the displayed
 *              rows is queried from an external ReadTracker supplied by the
 *              orchestrator - this class owns no persistence code.
 *
 *              API:
 *                load(root)                - scan and repopulate from root
 *                refresh()                 - rescan, preserving expand state
 *                set_search(text)          - refilter with lowercased query
 *                mark_file_read(path)      - recolour one row after open
 *                mark_all_read_display()   - flip every leaf row to "read"
 *                on_file_activated(cb)     - slot fired on leaf activation
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/file_tree_columns.hpp>

#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treemodelfilter.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <sigc++/slot.h>

#include <string>
#include <utility>

namespace ase::viewer {

class ReadTracker;

class TreeView {
public:
    explicit TreeView(const ReadTracker& tracker);

    /** Return the scrolled container widget for insertion into a parent. */
    Gtk::ScrolledWindow& widget() noexcept { return m_scroll; }
    Gtk::Widget* native_widget() noexcept { return &m_scroll; }

    /** Rebuild the tree from root and store it as the new current root. */
    void load(const std::string& root);

    /** Rescan the current root, preserving expand state. */
    void refresh();

    /** Set the filter text (expected already lowercased) and refilter. */
    void set_search(const std::string& lowered_text);

    /** Update one row to "read" state. No-op if path is not in the tree. */
    void mark_file_read(const std::string& path);

    /** Flip every leaf row to "read" state. Caller must also persist. */
    void mark_all_read_display();

    /** Register a slot fired when the user activates a leaf (file). */
    template <typename Callback>
    void on_file_activated(Callback&& cb) {
        m_on_file_activated = sigc::slot<void(const std::string&)>(
            [fn = std::forward<Callback>(cb)](const std::string& path) { fn(path); });
    }

    /** The current root path last loaded. */
    const std::string& root_path() const noexcept { return m_root_path; }

    /** Move keyboard focus back to the tree widget. */
    void grab_focus();

private:
    void populate_tree(const std::string& root);
    void populate_node(const std::string& dir_path,
                       const Gtk::TreeModel::iterator& parent);
    bool filter_row(const Gtk::TreeModel::const_iterator& iter) const;
    void expand_matching_rows();
    void update_tree_display(const std::string& file_path);
    static Glib::ustring format_display(const std::string& name,
                                        bool is_dir,
                                        bool is_read);

    Gtk::ScrolledWindow                m_scroll;
    Gtk::TreeView                      m_tree_view;
    FileTreeColumns                    m_columns;
    Glib::RefPtr<Gtk::TreeStore>       m_tree_store;
    Glib::RefPtr<Gtk::TreeModelFilter> m_filter_model;

    std::string m_root_path;
    std::string m_search_text;

    const ReadTracker& m_tracker;

    sigc::slot<void(const std::string&)> m_on_file_activated;
};

}  // namespace ase::viewer
