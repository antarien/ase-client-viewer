/**
 * @file        settings.cpp
 * @brief       Flat JSON loader/saver for ViewerSettings
 * @description POSIX I/O only - no std::filesystem, no <fstream>. The flat
 *              "key": value layout lets a line-based parser pick up each
 *              entry without a full JSON tokeniser.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/settings.hpp>

#include <viewer/fs_utils.hpp>

#include <cstdlib>
#include <string>

namespace ase::viewer {

namespace {

std::string trim_key(const std::string& s) {
    auto b = s.find_first_not_of(" \t\n\r\"");
    auto e = s.find_last_not_of(" \t\n\r\",");
    if (b == std::string::npos) return std::string{};
    return s.substr(b, e - b + 1);
}

std::string trim_value(const std::string& s) {
    return trim_key(s);
}

void parse_line(const std::string& line, ViewerSettings& s) {
    auto colon = line.find(':');
    if (colon == std::string::npos) return;
    const std::string key = trim_key(line.substr(0, colon));
    const std::string val = trim_value(line.substr(colon + 1));
    if (key.empty() || val.empty()) return;

    if (key == "color_scheme")        s.color_scheme   = static_cast<uint8_t>(std::atoi(val.c_str()));
    else if (key == "font_size")      s.font_size      = std::atoi(val.c_str());
    else if (key == "code_font")      s.code_font      = std::atoi(val.c_str());
    else if (key == "default_root")   s.default_root   = val;
    else if (key == "default_mode")   s.default_mode   = static_cast<uint8_t>(std::atoi(val.c_str()));
    else if (key == "language")       s.language       = static_cast<uint8_t>(std::atoi(val.c_str()));
    else if (key == "live_reload")    s.live_reload    = val == "true";
    else if (key == "line_numbers")   s.line_numbers   = val == "true";
    else if (key == "math_rendering") s.math_rendering = val == "true";
    else if (key == "mermaid_render") s.mermaid_render = val == "true";
}

std::string make_json(const ViewerSettings& s) {
    std::string out;
    out += "{\n";
    out += "  \"color_scheme\": "  + std::to_string(static_cast<int>(s.color_scheme)) + ",\n";
    out += "  \"font_size\": "     + std::to_string(s.font_size) + ",\n";
    out += "  \"code_font\": "     + std::to_string(s.code_font) + ",\n";
    out += "  \"default_root\": \"" + s.default_root + "\",\n";
    out += "  \"default_mode\": "  + std::to_string(static_cast<int>(s.default_mode)) + ",\n";
    out += "  \"language\": "      + std::to_string(static_cast<int>(s.language)) + ",\n";
    out += std::string{"  \"live_reload\": "}    + (s.live_reload    ? "true" : "false") + ",\n";
    out += std::string{"  \"line_numbers\": "}   + (s.line_numbers   ? "true" : "false") + ",\n";
    out += std::string{"  \"math_rendering\": "} + (s.math_rendering ? "true" : "false") + ",\n";
    out += std::string{"  \"mermaid_render\": "} + (s.mermaid_render ? "true" : "false") + "\n";
    out += "}\n";
    return out;
}

}  // namespace

ViewerSettings load_settings(const std::string& config_dir) {
    ViewerSettings s;
    const std::string path = config_dir + "/settings.json";
    std::string content;
    if (!fs::read_file_to_string(path, content)) return s;

    size_t i = 0;
    while (i < content.size()) {
        size_t j = i;
        while (j < content.size() && content[j] != '\n') ++j;
        if (j > i) parse_line(content.substr(i, j - i), s);
        i = j + 1;
    }
    return s;
}

void save_settings(const std::string& config_dir, const ViewerSettings& s) {
    fs::make_directories(config_dir);
    const std::string path = config_dir + "/settings.json";
    fs::write_file_from_string(path, make_json(s));
}

}  // namespace ase::viewer
