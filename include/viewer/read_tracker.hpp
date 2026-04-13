#pragma once

/**
 * @file        read_tracker.hpp
 * @brief       Persistent "already read" file set
 * @description Loads a newline-delimited list of paths from the XDG config
 *              directory, offers O(log n) membership queries, and saves the
 *              set back on demand. The viewer's orchestrator owns one
 *              instance and consults it when formatting tree rows.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <set>
#include <string>

namespace ase::viewer {

class ReadTracker {
public:
    /** Load from config_dir/read-files.txt (missing file is not an error). */
    void load(const std::string& config_dir);

    /** Save the current set to config_dir/read-files.txt. */
    void save(const std::string& config_dir) const;

    /** Insert path into the set. Returns true if newly inserted. */
    bool mark_read(const std::string& path);

    /** True if path has already been read. */
    bool is_read(const std::string& path) const;

    /** Direct access to the backing set for bulk iteration. */
    const std::set<std::string>& entries() const noexcept { return m_paths; }

private:
    std::set<std::string> m_paths;
};

}  // namespace ase::viewer
