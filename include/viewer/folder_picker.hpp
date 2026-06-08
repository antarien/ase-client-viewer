#pragma once

/**
 * @file        folder_picker.hpp
 * @brief       Custom modal folder picker — pure GTK4, no portal dependency
 * @description Minimal in-process folder browser written from scratch with
 *              AdwWindow + GtkListBox + POSIX opendir/readdir/stat (no
 *              std::filesystem — forbidden in clients). Mirrors the picker
 *              from ase-client-explorer to avoid GtkFileDialog/GtkPathBar
 *              fallback glitches on Hyprland sessions where no FileChooser
 *              portal backend is eligible.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <sigc++/slot.h>

#include <string>

typedef struct _GtkWindow GtkWindow;

namespace ase::viewer::folder_picker {

/**
 * Present a modal folder picker as a child of `parent`. Starts at
 * `start_path` (or HOME if empty/non-existent). Calls `on_selected` with
 * the absolute path when the user accepts; not called on cancel.
 */
void show(GtkWindow* parent,
          const std::string& start_path,
          sigc::slot<void(const std::string&)> on_selected);

}  // namespace ase::viewer::folder_picker
