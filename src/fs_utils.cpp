/**
 * @file        fs_utils.cpp
 * @brief       POSIX filesystem helper implementations
 * @description Thin C-wrappers around stat(), opendir(), readdir(), fopen(),
 *              readlink() and mkdir(). No std::filesystem, no <fstream>, no
 *              exceptions - matches the ECS validator's allow-list for client
 *              code.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/fs_utils.hpp>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <vector>

namespace ase::viewer::fs {

bool path_exists(const std::string& path) {
    struct stat st;
    return ::stat(path.c_str(), &st) == 0;
}

bool is_directory(const std::string& path) {
    struct stat st;
    if (::stat(path.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

bool is_regular_file(const std::string& path) {
    struct stat st;
    if (::stat(path.c_str(), &st) != 0) return false;
    return S_ISREG(st.st_mode);
}

std::string parent_dir(const std::string& path) {
    if (path.empty()) return std::string{};
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos) return std::string{};
    if (pos == 0) return std::string{"/"};
    return path.substr(0, pos);
}

std::string base_name(const std::string& path) {
    if (path.empty()) return std::string{};
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

std::string path_join(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    if (a.back() == '/') return a + b;
    return a + "/" + b;
}

void make_directories(const std::string& path) {
    if (path.empty()) return;
    if (path_exists(path)) return;
    make_directories(parent_dir(path));
    ::mkdir(path.c_str(), 0755);
}

bool read_file_to_string(const std::string& path, std::string& out) {
    out.clear();
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) return false;
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (size > 0) {
        out.resize(static_cast<size_t>(size));
        size_t read = std::fread(out.data(), 1, static_cast<size_t>(size), f);
        out.resize(read);
    }
    std::fclose(f);
    return true;
}

bool write_file_from_string(const std::string& path, const std::string& content) {
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (f == nullptr) return false;
    std::fwrite(content.data(), 1, content.size(), f);
    std::fclose(f);
    return true;
}

std::string read_symlink_target(const std::string& path) {
    char buf[4096];
    ssize_t n = ::readlink(path.c_str(), buf, sizeof(buf) - 1);
    if (n <= 0) return std::string{};
    buf[n] = '\0';
    return std::string{buf};
}

void walk_markdown_files_impl(const std::string& root,
                              sigc::slot<void(const std::string&)> on_file) {
    std::vector<std::string> stack;
    stack.push_back(root);
    while (!stack.empty()) {
        const std::string dir = std::move(stack.back());
        stack.pop_back();

        DIR* d = ::opendir(dir.c_str());
        if (d == nullptr) continue;

        struct dirent* de = nullptr;
        while ((de = ::readdir(d)) != nullptr) {
            const std::string name{de->d_name};
            if (name.empty() || name[0] == '.') continue;
            const std::string full = path_join(dir, name);
            if (is_directory(full)) {
                stack.push_back(full);
            } else if (name.size() > 3 && name.substr(name.size() - 3) == ".md") {
                on_file(full);
            }
        }
        ::closedir(d);
    }
}

}  // namespace ase::viewer::fs
