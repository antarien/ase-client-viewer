#pragma once

/**
 * @file        file_tree_columns.hpp
 * @brief       Column record for the sidebar Gtk::TreeStore
 * @description Plain data carrier describing the columns of the tree store
 *              that backs the sidebar. Split out into its own header so the
 *              tree view implementation and any helpers can share the column
 *              definitions without pulling in each other's widget state.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <gtkmm/treemodelcolumn.h>
#include <glibmm/ustring.h>

#include <string>

namespace ase::viewer {

class FileTreeColumns : public Gtk::TreeModelColumnRecord {
public:
    FileTreeColumns() {
        add(col_display);
        add(col_name);
        add(col_path);
        add(col_is_dir);
        add(col_is_read);
    }

    Gtk::TreeModelColumn<Glib::ustring> col_display;
    Gtk::TreeModelColumn<Glib::ustring> col_name;
    Gtk::TreeModelColumn<std::string>   col_path;
    Gtk::TreeModelColumn<bool>          col_is_dir;
    Gtk::TreeModelColumn<bool>          col_is_read;
};

}  // namespace ase::viewer
