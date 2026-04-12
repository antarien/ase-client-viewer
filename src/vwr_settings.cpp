#include "vwr_settings.hpp"
#include <fstream>
#include <filesystem>

// Minimal JSON parser/writer — no external deps needed for flat key-value

namespace ase::viewer {

namespace {

std::string trim(const std::string& s) {
    auto b = s.find_first_not_of(" \t\n\r\"");
    auto e = s.find_last_not_of(" \t\n\r\",");
    if (b == std::string::npos) return "";
    return s.substr(b, e - b + 1);
}

std::string extract_value(const std::string& line) {
    auto pos = line.find(':');
    if (pos == std::string::npos) return "";
    return trim(line.substr(pos + 1));
}

std::string extract_key(const std::string& line) {
    auto pos = line.find(':');
    if (pos == std::string::npos) return "";
    return trim(line.substr(0, pos));
}

}  // anonymous namespace

ViewerSettings load_settings(const std::string& config_dir) {
    ViewerSettings s;
    auto path = config_dir + "/settings.json";
    std::ifstream f(path);
    if (!f.is_open()) return s;

    std::string line;
    while (std::getline(f, line)) {
        auto key = extract_key(line);
        auto val = extract_value(line);
        if (key.empty() || val.empty()) continue;

        if (key == "color_scheme")   s.color_scheme   = static_cast<uint8_t>(std::stoi(val));
        else if (key == "font_size") s.font_size       = std::stoi(val);
        else if (key == "code_font") s.code_font       = std::stoi(val);
        else if (key == "default_root") s.default_root  = val;
        else if (key == "default_mode") s.default_mode  = static_cast<uint8_t>(std::stoi(val));
        else if (key == "language")     s.language       = static_cast<uint8_t>(std::stoi(val));
        else if (key == "live_reload")  s.live_reload    = val == "true";
        else if (key == "line_numbers") s.line_numbers   = val == "true";
        else if (key == "math_rendering") s.math_rendering = val == "true";
        else if (key == "mermaid_render") s.mermaid_render = val == "true";
    }
    return s;
}

void save_settings(const std::string& config_dir, const ViewerSettings& s) {
    namespace fs = std::filesystem;
    fs::create_directories(config_dir);
    auto path = config_dir + "/settings.json";
    std::ofstream f(path);
    f << "{\n";
    f << "  \"color_scheme\": " << static_cast<int>(s.color_scheme) << ",\n";
    f << "  \"font_size\": " << s.font_size << ",\n";
    f << "  \"code_font\": " << s.code_font << ",\n";
    f << "  \"default_root\": \"" << s.default_root << "\",\n";
    f << "  \"default_mode\": " << static_cast<int>(s.default_mode) << ",\n";
    f << "  \"language\": " << static_cast<int>(s.language) << ",\n";
    f << "  \"live_reload\": " << (s.live_reload ? "true" : "false") << ",\n";
    f << "  \"line_numbers\": " << (s.line_numbers ? "true" : "false") << ",\n";
    f << "  \"math_rendering\": " << (s.math_rendering ? "true" : "false") << ",\n";
    f << "  \"mermaid_render\": " << (s.mermaid_render ? "true" : "false") << "\n";
    f << "}\n";
}

}  // namespace ase::viewer
