#pragma once

/**
 * @file        fs_utils.hpp
 * @brief       POSIX filesystem helpers - std::filesystem forbidden in clients
 * @description Replaces the ad-hoc POSIX helpers that used to live inline in
 *              main.cpp. Every call goes through stat/opendir/readdir so the
 *              client never pulls in the C++ filesystem TS (forbidden by the
 *              ECS validator) and never hits exception-throwing I/O paths.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <sigc++/slot.h>

#include <string>
#include <utility>

namespace ase::viewer::fs {

/** True if stat() succeeds for the given path. */
bool path_exists(const std::string& path);

/** True if path exists and is a directory. */
bool is_directory(const std::string& path);

/** True if path exists and is a regular file. */
bool is_regular_file(const std::string& path);

/** Return the parent directory component. "/a/b/c" -> "/a/b". */
std::string parent_dir(const std::string& path);

/** Return the final path component. "/a/b/c.md" -> "c.md". */
std::string base_name(const std::string& path);

/** Join two path segments with a single separator. */
std::string path_join(const std::string& a, const std::string& b);

/** Recursively create the directory (mkdir -p). No error reporting. */
void make_directories(const std::string& path);

/** Slurp a file into out. Returns false on fopen failure, clears out otherwise. */
bool read_file_to_string(const std::string& path, std::string& out);

/** Write a string to path, truncating. Returns false on fopen failure. */
bool write_file_from_string(const std::string& path, const std::string& content);

/** Resolve a symlink to its target. Returns empty on error. */
std::string read_symlink_target(const std::string& path);

/**
 * Recursively walk root and invoke the callback for every .md leaf file.
 * Hidden files (dotfiles) and entries whose name starts with '.' are
 * skipped. Iterative, so very deep trees do not blow the call stack.
 */
void walk_markdown_files_impl(const std::string& root,
                              sigc::slot<void(const std::string&)> on_file);

template <typename Callback>
void walk_markdown_files(const std::string& root, Callback&& on_file) {
    walk_markdown_files_impl(root,
        sigc::slot<void(const std::string&)>(
            [fn = std::forward<Callback>(on_file)](const std::string& p) { fn(p); }));
}

}  // namespace ase::viewer::fs
