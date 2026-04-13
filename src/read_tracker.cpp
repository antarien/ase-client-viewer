/**
 * @file        read_tracker.cpp
 * @brief       Persistent read-state implementation
 * @description Serialises the set as a newline-delimited UTF-8 file, routed
 *              through the POSIX helpers in fs_utils so no <fstream> usage
 *              leaks into this translation unit.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/read_tracker.hpp>

#include <viewer/fs_utils.hpp>

namespace ase::viewer {

void ReadTracker::load(const std::string& config_dir) {
    m_paths.clear();
    const std::string path = config_dir + "/read-files.txt";
    std::string content;
    if (!fs::read_file_to_string(path, content)) return;
    size_t i = 0;
    while (i < content.size()) {
        size_t j = i;
        while (j < content.size() && content[j] != '\n') ++j;
        if (j > i) m_paths.insert(content.substr(i, j - i));
        i = j + 1;
    }
}

void ReadTracker::save(const std::string& config_dir) const {
    const std::string path = config_dir + "/read-files.txt";
    std::string content;
    for (const auto& p : m_paths) {
        content.append(p);
        content.push_back('\n');
    }
    fs::write_file_from_string(path, content);
}

bool ReadTracker::mark_read(const std::string& path) {
    return m_paths.insert(path).second;
}

bool ReadTracker::is_read(const std::string& path) const {
    return m_paths.count(path) > 0;
}

}  // namespace ase::viewer
